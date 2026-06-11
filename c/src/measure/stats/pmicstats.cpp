#include "pmicstats.hpp"

#include "../../logging.hpp"

#include <cstdlib>
#include <map>
#include <string>

using tirex::PmicReader;

#if defined(__linux__)
#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <vector>

namespace {
	// VideoCore mailbox property interface, as used by `vcgencmd` (raspberrypi/utils).
	constexpr unsigned MAILBOX_GENCMD_TAG = 0x00030080u;	  // GET_GENCMD_RESULT
	constexpr int MAILBOX_BUFFER_BYTES = 4 * 1024;			  // vcgencmd's MAX_STRING
	const unsigned long MAILBOX_IOCTL_PROPERTY = _IOWR(100, 0, char*);

	int openMailbox() {
		for (const char* dev : {"/dev/vcio_gencmd", "/dev/vcio"}) {
			const int fd = open(dev, O_RDWR);
			if (fd >= 0)
				return fd;
		}
		return -1;
	}

	/**
	 * @brief Runs a firmware gencmd (e.g. "pmic_read_adc") via the mailbox and returns the text
	 * response, or an empty string on failure. Mirrors the buffer layout of vcgencmd exactly.
	 */
	std::string runGencmd(const char* command) {
		const int fd = openMailbox();
		if (fd < 0)
			return {};

		const size_t len = std::strlen(command);
		if (len + 1 >= static_cast<size_t>(MAILBOX_BUFFER_BYTES)) {
			close(fd);
			return {};
		}

		constexpr int words = (MAILBOX_BUFFER_BYTES >> 2) + 7;
		std::vector<unsigned> p(words, 0u);
		int i = 0;
		p[i++] = 0;						// total size (set below)
		p[i++] = 0;						// process request
		p[i++] = MAILBOX_GENCMD_TAG;	// tag
		p[i++] = MAILBOX_BUFFER_BYTES;	// value buffer length
		p[i++] = 0;						// request length
		p[i++] = 0;						// error / result word (p[5])
		std::memcpy(p.data() + i, command, len + 1); // command string at p[6]
		i += MAILBOX_BUFFER_BYTES >> 2;
		p[i++] = 0;						// end tag
		p[0] = static_cast<unsigned>(i * sizeof(unsigned));

		const int rc = ioctl(fd, MAILBOX_IOCTL_PROPERTY, p.data());
		close(fd);
		if (rc < 0)
			return {};

		// Response string starts at word offset 6; the firmware NUL-terminates it.
		const char* resp = reinterpret_cast<const char*>(p.data() + 6);
		const size_t cap = static_cast<size_t>(MAILBOX_BUFFER_BYTES);
		return std::string(resp, ::strnlen(resp, cap));
	}
} // namespace

std::optional<PmicReader::Sample> PmicReader::readOnce() { return parseAdc(runGencmd("pmic_read_adc")); }

bool PmicReader::available() {
	static const bool avail = []() {
		const auto sample = readOnce();
		const bool ok = sample.has_value() && (sample->coreW > 0.0 || sample->ramW > 0.0);
		if (ok)
			tirex::log::info("pmic", "Raspberry Pi PMIC detected; using it for CPU/RAM energy");
		return ok;
	}();
	return avail;
}
#else
std::optional<PmicReader::Sample> PmicReader::readOnce() { return std::nullopt; }
bool PmicReader::available() { return false; }
#endif

std::optional<PmicReader::Sample> PmicReader::parseAdc(std::string_view output) {
	// `pmic_read_adc` prints one line per rail, e.g.:
	//   VDD_CORE_A current(7)=1.89316000A
	//   VDD_CORE_V volt(15)=0.91142770V
	// Collect current (suffix _A) and voltage (suffix _V) per rail and form the power U*I.
	if (output.empty())
		return std::nullopt;

	std::map<std::string, double> amp, volt;
	size_t pos = 0;
	while (pos < output.size()) {
		size_t nl = output.find('\n', pos);
		const std::string_view line =
				output.substr(pos, nl == std::string_view::npos ? std::string_view::npos : nl - pos);
		pos = (nl == std::string_view::npos) ? output.size() : nl + 1;

		const auto eq = line.find('=');
		if (eq == std::string_view::npos)
			continue;
		const auto begin = line.find_first_not_of(" \t");
		if (begin == std::string_view::npos)
			continue;
		const auto ws = line.find_first_of(" \t", begin);
		if (ws == std::string_view::npos || ws > eq)
			continue;
		const std::string_view token = line.substr(begin, ws - begin); // e.g. "VDD_CORE_A"
		if (token.size() < 3 || token[token.size() - 2] != '_')
			continue;
		const char kind = token.back();
		if (kind != 'A' && kind != 'V')
			continue;
		const std::string rail(token.substr(0, token.size() - 2)); // strip trailing "_A"/"_V"
		const double value = std::strtod(std::string(line.substr(eq + 1)).c_str(), nullptr);
		(kind == 'A' ? amp : volt)[rail] = value;
	}

	if (amp.empty() && volt.empty())
		return std::nullopt;

	const auto power = [&](const char* rail) -> double {
		const auto a = amp.find(rail);
		const auto v = volt.find(rail);
		return (a == amp.end() || v == volt.end()) ? 0.0 : a->second * v->second;
	};

	Sample sample{};
	sample.coreW = power("VDD_CORE");
	sample.ramW = power("DDR_VDD2") + power("DDR_VDDQ");
	return sample;
}

void PmicReader::start() {
	coreJ = ramJ = 0.0;
	hasPrev = false;
	step(); // seed the first sample
}

void PmicReader::step() {
	const auto sample = readOnce();
	const auto now = std::chrono::steady_clock::now();
	if (sample && hasPrev) {
		const double dt = std::chrono::duration<double>(now - prevTime).count();
		coreJ += 0.5 * (prev.coreW + sample->coreW) * dt;
		ramJ += 0.5 * (prev.ramW + sample->ramW) * dt;
	}
	if (sample) {
		prev = *sample;
		prevTime = now;
		hasPrev = true;
	}
}

void PmicReader::stop() { step(); }
