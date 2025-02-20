#include <measure.h>

#include "measure/stats/provider.hpp"
#include "measure/utils/rangeutils.hpp"

#if __cpp_lib_format
#include <format>
namespace _fmt = std;
#else
#include <fmt/core.h>
namespace _fmt = fmt;
#endif

#include <iostream>

struct msrResult_st {
public:
	std::vector<std::pair<msrMeasure, std::string>> value;

public:
	explicit msrResult_st(std::vector<std::pair<msrMeasure, std::string>>&& val) : value(std::move(val)) {}
};

msrError msrResultEntryGetByIndex(const msrResult* result, size_t index, msrResultEntry* entry) {
	if (result == nullptr || index >= result->value.size())
		return msrError::MSR_INVALID_ARGUMENT;
	const auto& [source, value] = result->value.at(index);
	*entry = {.source = source, .value = value.c_str(), .type = msrResultType::MSR_STRING};
	return msrError::MSR_SUCCESS;
}

msrError msrResultEntryNum(const msrResult* result, size_t* num) {
	if (result == nullptr)
		return msrError::MSR_INVALID_ARGUMENT;
	*num = result->value.size();
	return msrError::MSR_SUCCESS;
}

void msrResultFree(msrResult* result) { delete result; }

template <class... Ts>
struct overloaded : Ts... {
	using Ts::operator()...;
};

template <typename T>
static std::string toYAML(const msr::TimeSeries<T>& timeseries) {
	const auto& [timestamps, values] = timeseries.timeseries();
	return _fmt::format(
			"{{max: {}, min: {}, avg: {}, timeseries: {{timestamps: [{}], values: [{}]}}}}", timeseries.maxValue(),
			timeseries.minValue(), timeseries.avgValue(), msr::utils::join(timestamps, ','),
			msr::utils::join(values, ',')
	);
}

extern msrResult_st* msr::createMsrResultFromStats(msr::Stats&& stats) {
	std::vector<std::pair<msrMeasure, std::string>> result;
	for (auto&& [key, value] : stats) {
		std::visit(
				overloaded{
						[key, &result](std::string& str) { result.emplace_back(key, std::move(str)); },
						[key, &result](const msr::TimeSeries<unsigned>& timeseries) {
							result.emplace_back(key, toYAML(timeseries));
						}
				},
				value
		);
	}
	return new msrResult_st(std::move(result));
}