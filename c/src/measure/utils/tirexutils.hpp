#ifndef MEASURE_UTILS_TIREX_HPP
#define MEASURE_UTILS_TIREX_HPP

#include <tirex_tracker.h>

namespace tirex::utils {
	namespace details {
		template <typename T>
		struct tirexvaluetype;

		template <>
		struct tirexvaluetype<const char*> {
			static const tirexResultType value = tirexResultType::TIREX_STRING;
		};

		template <>
		struct tirexvaluetype<const wchar_t*> {
			static const tirexResultType value = tirexResultType::TIREX_WSTRING;
		};
	} // namespace details

	template <typename T>
	constexpr tirexResultType getResultType() noexcept {
		return details::tirexvaluetype<T>::value;
	}
} // namespace tirex::utils

#endif