#include "pmicreader.hpp"

#include "../../../../logging.hpp"

#include <cstdlib>
#include <functional>
#include <map>
#include <regex>
#include <string>

using tirex::PmicReader;

#if defined(__linux__)
#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <array>

// Adapted from the Raspberry Pi vcgencmd utility:
// https://github.com/raspberrypi/utils/blob/master/vcgencmd/vcgencmd.c
namespace {
	// VideoCore mailbox property interface, as used by `vcgencmd` (raspberrypi/utils).
	constexpr unsigned MailboxGencmdTag = 0x00030080u;	  // GET_GENCMD_RESULT
	constexpr unsigned MailboxBufferBytes = 4 * 1024;		  // vcgencmd's MAX_STRING
	constexpr unsigned long MailboxIoctlProperty = _IOWR(100, 0, char*);

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
		if (len + 1 >= static_cast<size_t>(MailboxBufferBytes)) {
			close(fd);
			return {};
		}

		constexpr int words = (MailboxBufferBytes >> 2) + 7;
		std::array<unsigned, words> p{};
		size_t i = 0;
		p[i++] = 0;						// total size (set below)
		p[i++] = 0;						// process request
		p[i++] = MailboxGencmdTag;		// tag
		p[i++] = MailboxBufferBytes;	// value buffer length
		p[i++] = 0;						// request length
		p[i++] = 0;						// error / result word (p[5])
		std::memcpy(p.data() + i, command, len + 1); // command string at p[6]
		i += MailboxBufferBytes >> 2;
		p[i++] = 0;						// end tag
		p[0] = static_cast<unsigned>(i * sizeof(unsigned));

		const int rc = ioctl(fd, MailboxIoctlProperty, p.data());
		close(fd);
		if (rc < 0)
			return {};

		// Response string starts at word offset 6; the firmware NUL-terminates it.
		const char* resp = reinterpret_cast<const char*>(p.data() + 6);
		return std::string(resp, ::strnlen(resp, static_cast<size_t>(MailboxBufferBytes)));
	}
}

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
	if (output.empty())
		return std::nullopt;

	static const std::regex re(R"((\w+)_([AV])\s[^=\n]*=\s*([-0-9.eE+]+))");
	std::map<std::string, double, std::less<>> amp, volt; 
	for (std::cregex_iterator it(output.data(), output.data() + output.size(), re), end; it != end; ++it)
		(((*it)[2] == "A") ? amp : volt)[(*it)[1].str()] = std::strtod((*it)[3].str().c_str(), nullptr);

	if (amp.empty() && volt.empty())
		return std::nullopt;

	const auto power = [&](std::string_view rail) -> double {
		const auto a = amp.find(rail);
		const auto v = volt.find(rail);
		return (a == amp.end() || v == volt.end()) ? 0.0 : a->second * v->second;
	};

	return Sample{
			.coreW = power("VDD_CORE"),
			.ramW = power("DDR_VDD2") + power("DDR_VDDQ"),
	};
}

void PmicReader::start() {
	coreJ = ramJ = 0.0;
	hasPrev = false;
	step(); // seed the first sample
}

void PmicReader::step() {
	const auto sample = readOnce();
	if (!sample)
		return;
	const auto now = std::chrono::steady_clock::now();
	if (hasPrev) {
		const double dt = std::chrono::duration<double>(now - prevTime).count();
		coreJ += 0.5 * (prev.coreW + sample->coreW) * dt;
		ramJ += 0.5 * (prev.ramW + sample->ramW) * dt;
	}
	prev = *sample;
	prevTime = now;
	hasPrev = true;
}

void PmicReader::stop() { step(); }
