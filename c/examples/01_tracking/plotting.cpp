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

struct TimeSeriesData {
	std::vector<double> timestampsMs;
	std::vector<double> values;
};

// Parses a chrono::milliseconds duration formatted via fmt (e.g. "30ms") back into a plain number.
double parseMillis(const std::string& formatted) {
	static constexpr std::string_view suffix = "ms";
	assert(formatted.size() >= suffix.size() &&
		   formatted.compare(formatted.size() - suffix.size(), suffix.size(), suffix) == 0);
	return std::stod(formatted.substr(0, formatted.size() - suffix.size()));
}

TimeSeriesData parseTimeseries(const char* json) {
	auto parsed = json::parse(json);
	auto timeseries = parsed["timeseries"];
	auto values = timeseries["values"];
	auto timestamps = timeseries["timestamps"];

	TimeSeriesData data;
	data.values.reserve(values.size());
	for (auto& item : values) {
		assert(item.is_number());
		data.values.push_back(item.get<double>());
	}
	data.timestampsMs.reserve(timestamps.size());
	for (auto& item : timestamps) {
		assert(item.is_string());
		data.timestampsMs.push_back(parseMillis(item.get<std::string>()));
	}
	return data;
}

/**
 * @brief Resamples a piecewise-constant (timestamp, value) series onto `resolution` evenly-spaced points in time.
 * @details The timeseries stores one point per compressed segment, and segments can cover wildly different spans of
 * real time (that's the point of the algorithm: a long flat stretch becomes one wide segment, a noisy/spiky stretch
 * stays dense). Plotting `values` directly by index — as asciichart requires, since it only understands evenly-spaced
 * series — would draw every segment with identical on-screen width regardless of how much time it actually spans,
 * hiding exactly the adaptive resolution this is meant to show. Resampling onto a uniform time grid first (holding each
 * segment's value up to its own reported timestamp, matching how it was exported) fixes that: a wide segment fills many
 * grid slots and a narrow one fills few, same as the real data.
 */
std::vector<double> resample(const TimeSeriesData& data, size_t resolution = 120) {
	if (data.values.empty())
		return {};
	if (data.values.size() == 1)
		return std::vector<double>(resolution, data.values.front());

	/* 0, not timestampsMs.front(): every reported timestamp is a segment's *end*, so the first point's real coverage
	 * span is [0, timestampsMs[0]], not starting at timestampsMs[0]. Anchoring the grid there instead of at 0 would
	 * crop that whole opening span before the grid even begins. 0 is always (approximately) the true start of tracking,
	 * since these timestamps are relative to when TimeSeries::currentTimestamp() started counting for this measure. */
	double start = 0;
	double end = data.timestampsMs.back();
	std::vector<double> out;
	out.reserve(resolution);
	size_t idx = 0;
	for (size_t i = 0; i < resolution; ++i) {
		double t = start + (end - start) * (static_cast<double>(i) / static_cast<double>(resolution - 1));
		while (idx + 1 < data.timestampsMs.size() && data.timestampsMs[idx] < t)
			++idx;
		out.push_back(data.values[idx]);
	}
	return out;
}

std::vector<double> readTimeseriesMeasure(tirexResult* result, tirexMeasure measure) {
	tirexResultEntry entry;
	if (!tirexResultEntryByMeasure(result, measure, entry))
		std::abort();

	return resample(parseTimeseries(static_cast<const char*>(entry.value)));
}

void plot(tirexResult* result) {
	// Note: these two timeseries may not necessarily align in their time-axis
	{
		auto cpu = readTimeseriesMeasure(result, tirexMeasure::TIREX_CPU_USED_PROCESS_PERCENT);
		std::cout << "CPU [%] usage over time:" << std::endl;
		ascii::Asciichart asciichart(std::vector<std::vector<double>>{cpu});
		std::cout << '\n' << asciichart.height(10).Plot() << '\n';
	}
	{
		auto ram = readTimeseriesMeasure(result, tirexMeasure::TIREX_RAM_USED_PROCESS_KB);
		std::cout << "RAM [KB] usage over time:" << std::endl;
		ascii::Asciichart asciichart(std::vector<std::vector<double>>{ram});
		std::cout << '\n' << asciichart.height(10).Plot() << '\n';
	}
}