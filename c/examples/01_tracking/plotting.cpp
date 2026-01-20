#include <tirex_tracker.h>

#include <ascii/ascii.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

extern "C" {
void plot(tirexResult* result);
}

bool tirexResultEntryByMeasure(tirexResult* result, tirexMeasure measure, tirexResultEntry& entry) {
	size_t num;
	if (tirexResultEntryNum(result, &num) != tirexError::TIREX_SUCCESS)
		std::abort();
	for (size_t i = 0; i < num; ++i) {
		if (tirexResultEntryGetByIndex(result, i, &entry) == tirexError::TIREX_SUCCESS && entry.source == measure)
			return true;
	}
	return false;
}

std::vector<double> parseTimeseries(const char* json) {
	auto parsed = json::parse(json);
	auto timeseries = parsed["timeseries"];
	// We ignore the timestamps here. This will result in a somewhat skewed graph since the timesteps may not be equidistant
	auto values = timeseries["values"];
	std::vector<double> data;
	data.reserve(values.size());
	for (auto& item : values) {
		assert(item.is_number());
		data.push_back(item.get<double>());
	}
	return data;
}

std::vector<double> readTimeseriesMeasure(tirexResult* result, tirexMeasure measure) {
	tirexResultEntry entry;
	if (!tirexResultEntryByMeasure(result, measure, entry))
		std::abort();

	return parseTimeseries(static_cast<const char*>(entry.value));
}

void plot(tirexResult* result) {
	// Note: these two timeseries may not necessarily align in their time-axis
	{
		auto cpu = readTimeseriesMeasure(result, tirexMeasure::TIREX_CPU_USED_PROCESS_PERCENT);
		std::cout << "CPU usage over time:" << std::endl;
		ascii::Asciichart asciichart(std::vector<std::vector<double>>{cpu});
		std::cout << '\n' << asciichart.height(10).Plot() << '\n';
	}
	{
		auto ram = readTimeseriesMeasure(result, tirexMeasure::TIREX_RAM_USED_PROCESS_KB);
		std::cout << "RAM usage over time:" << std::endl;
		ascii::Asciichart asciichart(std::vector<std::vector<double>>{ram});
		std::cout << '\n' << asciichart.height(10).Plot() << '\n';
	}
}