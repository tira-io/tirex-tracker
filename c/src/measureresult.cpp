#include <tirex_tracker.h>

#include "measure/stats/provider.hpp"
#include "measure/utils/rangeutils.hpp"
#include "measure/utils/tirexutils.hpp"

#if __cpp_lib_format and __cplusplus >= 202207L // __cplusplus >= 202207L required for std::format_string
#include <format>
namespace _fmt = std;
#else
#include <fmt/core.h>
namespace _fmt = fmt;
#endif

#include <variant>

template <class... Ts>
struct overloaded : Ts... {
	using Ts::operator()...;
};

template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

struct tirexResult_st {
public:
	using ValueType = std::vector<std::pair<tirexMeasure, std::variant<std::string, tirex::TmpFile>>>;
	ValueType value;

public:
	explicit tirexResult_st(ValueType&& val) : value(std::move(val)) {}
};

tirexError tirexResultEntryGetByIndex(const tirexResult* result, size_t index, tirexResultEntry* entry) {
	if (result == nullptr || index >= result->value.size())
		return tirexError::TIREX_INVALID_ARGUMENT;
	const auto& [source, value] = result->value.at(index);
	std::visit(
			overloaded{
					[&](const std::string& str) {
						*entry = {.source = source, .value = str.c_str(), .type = tirex::utils::getResultType<decltype(str.c_str())>()};
					},
					[&](const tirex::TmpFile& file) {
						*entry = {
								.source = source,
								.value = file.path.c_str(),
								.type = tirex::utils::getResultType<decltype(file.path.c_str())>()
						};
					},
			},
			value
	);
	return tirexError::TIREX_SUCCESS;
}

tirexError tirexResultEntryNum(const tirexResult* result, size_t* num) {
	if (result == nullptr)
		return tirexError::TIREX_INVALID_ARGUMENT;
	*num = result->value.size();
	return tirexError::TIREX_SUCCESS;
}

void tirexResultFree(tirexResult* result) { delete result; }

template <typename T>
static std::string toYAML(const tirex::TimeSeries<T>& timeseries) {
	const auto& [timestamps, values] = timeseries.timeseries();
	static_assert(std::is_same_v<decltype(timestamps), const std::vector<std::chrono::milliseconds>&>);
	return _fmt::format(
			"{{\"max\": {}, \"min\": {}, \"avg\": {}, \"timeseries\": {{\"timestamps\": [\"{}\"], \"values\": [{}]}}}}",
			timeseries.maxValue(), timeseries.minValue(), timeseries.avgValue(),
			tirex::utils::join(timestamps, "\", \""), tirex::utils::join(values, ", ")
	);
}

extern tirexResult_st* tirex::createMsrResultFromStats(tirex::Stats&& stats) {
	tirexResult_st::ValueType result;
	for (auto it = std::make_move_iterator(stats.begin()), end = std::make_move_iterator(stats.end()); it != end;
		 ++it) {
		std::visit(
				overloaded{
						[&](const std::string& str) { result.emplace_back((*it).first, std::move(str)); },
						[&](tirex::TmpFile&& file) { result.emplace_back((*it).first, std::move(file)); },
						[&](const tirex::TimeSeries<unsigned>& timeseries) {
							result.emplace_back((*it).first, toYAML(timeseries));
						}
				},
				std::move((*it).second)
		);
	}
	return new tirexResult_st(std::move(result));
}