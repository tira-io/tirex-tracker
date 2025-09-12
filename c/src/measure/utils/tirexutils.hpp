#ifndef MEASURE_UTILS_TIREX_HPP
#define MEASURE_UTILS_TIREX_HPP

#include <tirex_tracker.h>

namespace tirex::utils {
	namespace details {
		template <typename T>
		struct tirexvaluetype;

		/** Defines the datatype-resulttype mapping for `const char*`. */
		template <>
		struct tirexvaluetype<const char*> {
			static const tirexResultType value = tirexResultType::TIREX_STRING;
		};

		/** Defines the datatype-resulttype mapping for `const wchar_t*`. */
		template <>
		struct tirexvaluetype<const wchar_t*> {
			static const tirexResultType value = tirexResultType::TIREX_WSTRING;
		};
	} // namespace details

	/**
	 * @brief Returns the corresponding tirexResultType for the given template argument, \p T .
	 * 
	 * @tparam T The tpe for which the corresponding tirexResultType is requested.
	 * @return The tirexResultType that represents a value of type \p T .
	 */
	template <typename T>
	constexpr tirexResultType getResultType() noexcept {
		return details::tirexvaluetype<T>::value;
	}
} // namespace tirex::utils

#endif