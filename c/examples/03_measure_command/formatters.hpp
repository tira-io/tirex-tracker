#ifndef STATFORMATTER_HPP
#define STATFORMATTER_HPP

#include <measure.h>

#include <functional>
#include <ostream>

namespace msr {
	using ResultFormatter = std::function<void(std::ostream& stream, const msrResult* result)>;

	extern void simpleFormatter(std::ostream& stream, const msrResult* result) noexcept;
	extern void jsonFormatter(std::ostream& stream, const msrResult* result) noexcept;
} // namespace msr

#endif