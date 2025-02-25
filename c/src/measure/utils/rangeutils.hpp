#ifndef MEASURE_UTILS_RANGEUTILS_HPP
#define MEASURE_UTILS_RANGEUTILS_HPP

#include <ranges>
#include <sstream>
#include <string>

namespace tirex::utils {
	inline std::string join(const std::ranges::range auto& range, char delimiter = ',') {
#ifdef __cpp_lib_ranges_join_with
		return range | std::join_with(delimiter);
#else
		std::stringstream stream;
		bool first = true;
		for (auto& elem : range) {
			if (!first)
				stream << delimiter;
			stream << elem;
			first = false;
		}
		return stream.str();
#endif
	}
} // namespace tirex::utils

#endif