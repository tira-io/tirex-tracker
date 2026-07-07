#include "temperaturestats.hpp"

#include "../../logging.hpp"

using tirex::Stats;
using tirex::TemperatureStats;

const char* TemperatureStats::version = nullptr;
const std::set<tirexMeasure> TemperatureStats::measures{TIREX_CPU_TEMPERATURE_CELSIUS};

#if defined(__linux__)
#include <filesystem>
#include <fstream>
#include <string>

/**
 * @brief Locates the sysfs thermal zone that reports the CPU package temperature (in millidegree Celsius).
 * @details Searches /sys/class/thermal for a zone whose type identifies the CPU/SoC (e.g., x86_pkg_temp on Intel or
 * cpu-thermal on the Raspberry Pi).
 */
static std::filesystem::path findCPUTemperatureSensor() {
	namespace fs = std::filesystem;
	static constexpr auto zonetypes = std::set<std::string_view>({"x86_pkg_temp", "cpu-thermal", "cpu_thermal", "soc-thermal", "soc_thermal"});
	std::string type;
	for (std::error_code ec; const auto& entry : fs::directory_iterator("/sys/class/thermal", ec)) {
		if (!entry.path().filename().native().starts_with("thermal_zone"))
			continue;
		std::ifstream stream(entry.path() / "type");
		if (!std::getline(stream, type))
			continue;
		if (zonetypes.contains(type))
			return entry.path() / "temp";
	}
	return {};
}

std::optional<unsigned> TemperatureStats::readTemperature() {
	static const std::filesystem::path sensor = []() {
		auto path = findCPUTemperatureSensor();
		if (path.empty())
			tirex::log::info("temperature", "No CPU temperature sensor found; temperature will not be tracked");
		else
			tirex::log::info("temperature", "Reading the CPU temperature from {}", path.string());
		return path;
	}();
	if (sensor.empty())
		return std::nullopt;
	std::ifstream stream(sensor);
	if (long millidegrees; stream >> millidegrees)
		return static_cast<unsigned>((millidegrees + 500) / 1000);
	return std::nullopt;
}
#else
std::optional<unsigned> TemperatureStats::readTemperature() {
	return std::nullopt; /** \todo not implemented on Windows and macOS **/
}
#endif

std::set<tirexMeasure> TemperatureStats::providedMeasures() noexcept {
	// Only claim the temperature measure if the system actually exposes a CPU temperature sensor.
	return readTemperature().has_value() ? measures : std::set<tirexMeasure>{};
}

void TemperatureStats::step() {
	if (auto temp = readTemperature(); temp.has_value())
		temperature.addValue(temp.value());
}

Stats TemperatureStats::getStats() {
	return makeFilteredStats(enabled, std::pair{TIREX_CPU_TEMPERATURE_CELSIUS, std::cref(temperature)});
}
