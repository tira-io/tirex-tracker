#include <tirex_tracker.h>

#include "measure/stats/provider.hpp"

#if __cpp_lib_format and __cplusplus >= 202207L // __cplusplus >= 202207L required for std::format_string
#include <format>
namespace _fmt = std;
#else
#include <fmt/core.h>
namespace _fmt = fmt;
#endif

#include <iostream>
#include <sstream>

struct tirexResult_st {
public:
	std::vector<std::pair<tirexMeasure, std::string>> value;

public:
	explicit tirexResult_st(std::vector<std::pair<tirexMeasure, std::string>>&& val) : value(std::move(val)) {}
};

tirexError tirexResultEntryGetByIndex(const tirexResult* result, size_t index, tirexResultEntry* entry) {
	if (result == nullptr || index >= result->value.size())
		return tirexError::TIREX_INVALID_ARGUMENT;
	const auto& [source, value] = result->value.at(index);
	*entry = {.source = source, .value = value.c_str(), .type = tirexResultType::TIREX_STRING};
	return tirexError::TIREX_SUCCESS;
}

tirexError tirexResultEntryNum(const tirexResult* result, size_t* num) {
	if (result == nullptr)
		return tirexError::TIREX_INVALID_ARGUMENT;
	*num = result->value.size();
	return tirexError::TIREX_SUCCESS;
}

void tirexResultFree(tirexResult* result) { delete result; }

static std::string toYAML(const tirex::TimeSeries<unsigned>& timeseries) {
	const auto& [timestamps, values] = timeseries.timeseries();

	std::stringstream timestampsStream;
	timestampsStream << "[";
	bool firstTImestamp = true;
	for (auto& elem : timestamps) {
		if (!firstTImestamp)
			timestampsStream << ",";
		timestampsStream << "\"";
		timestampsStream << elem.count();
		timestampsStream << "\"";
		firstTImestamp = false;
	}
	timestampsStream << "]";

	std::stringstream valuesStream;
	valuesStream << "[";
	bool firstValue = true;
	for (auto& elem : values) {
		if (!firstValue)
			valuesStream << ",";
		valuesStream << "\"";
		valuesStream << elem;
		valuesStream << "\"";
		firstValue = false;
	}
	valuesStream << "]";

	return _fmt::format(
			"{{\"max\":{},\"min\":{},\"avg\":{},\"timeseries\": {{\"timestamps\": [{}], \"values\": [{}]}}}}",
			timeseries.maxValue(), timeseries.minValue(), timeseries.avgValue(), timestampsStream.str(),
			valuesStream.str()
	);
}

extern tirexResult_st* tirex::createMsrResultFromStats(tirex::Stats&& stats) {
	std::vector<std::pair<tirexMeasure, std::string>> result;
	for (auto&& [key, value] : stats) {
		std::visit(
				overloaded{
						[key, &result](std::string& str) { result.emplace_back(key, std::move(str)); },
						[key, &result](const tirex::TimeSeries<unsigned>& timeseries) {
							result.emplace_back(key, toYAML(timeseries));
						}
				},
				value
		);
	}
	return new tirexResult_st(std::move(result));
}