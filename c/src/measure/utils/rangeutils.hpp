#ifndef MEASURE_UTILS_RANGEUTILS_HPP
#define MEASURE_UTILS_RANGEUTILS_HPP

#include <ranges>
#include <string>

namespace msr::utils {
	inline std::string join(const std::ranges::range auto& range, char delimiter = ',') {
#ifdef __cpp_lib_ranges_join_with
		return range | std::join_with(delimiter);
#else
		std::string joined = "";
		for (auto& elem : range) {
			if (!joined.empty())
				joined += delimiter;
			joined += elem;
		}
		return joined;
#endif
	}
} // namespace msr::utils

#endif