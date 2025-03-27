#ifndef MEASURE_UTILS_RANGEUTILS_HPP
#define MEASURE_UTILS_RANGEUTILS_HPP

#include <ranges>
#include <sstream>
#include <string>

/** For compatibiltiy with clang-18: implement operator<< for std::chrono::duration **/
#include <fmt/chrono.h>
#include <fmt/core.h>

template <typename _CharT, typename _Traits, typename _Rep, typename _Period>
inline std::basic_ostream<_CharT, _Traits>&
operator<<(std::basic_stringstream<_CharT, _Traits>& os, const std::chrono::duration<_Rep, _Period>& d) {
	return os << fmt::format("{}", d);
}
/** End of compatibility **/

namespace tirex::utils {
	/**
	 * @brief Joins a range using the specified delimiting character.
	 * @details This method is functionally identical to calling `range | std::join_with(delimiter)` and should be used
	 * since not all compilers that we want to support support ranges well enough yet.
	 * 
	 * @param range The range to join the values of.
	 * @param delimiter The delimiter to be placed between values.
	 * @return A string representation of the range, where the delimiter is placed between entries.
	 */
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