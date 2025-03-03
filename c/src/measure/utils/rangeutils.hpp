#ifndef MEASURE_UTILS_RANGEUTILS_HPP
#define MEASURE_UTILS_RANGEUTILS_HPP

#include <ranges>
#include <sstream>
#include <string>

#if __cplusplus < 202002L
/**
 * Implementation copied from chrono_io.h to support the operator on clang-18.
 */
#include <chrono>
namespace std {
	template <typename _CharT, typename _Traits, typename _Rep, typename _Period>
	inline basic_ostream<_CharT, _Traits>&
	operator<<(std::basic_ostream<_CharT, _Traits>& __os, const duration<_Rep, _Period>& __d) {
		using _Out = ostreambuf_iterator<_CharT, _Traits>;
		using period = typename _Period::type;
		std::basic_ostringstream<_CharT, _Traits> __s;
		__s.flags(__os.flags());
		__s.imbue(__os.getloc());
		__s.precision(__os.precision());
		__s << __d.count();
		__detail::__fmt_units_suffix<period, _CharT>(_Out(__s));
		__os << std::move(__s).str();
		return __os;
	}
} // namespace std
#endif

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