#ifndef STATFORMATTER_HPP
#define STATFORMATTER_HPP

#include <tirex_tracker.h>

#include <functional>
#include <ostream>

namespace tirex {
	using ResultFormatter =
			std::function<void(std::ostream& stream, const tirexResult* info, const tirexResult* result)>;

	extern void simpleFormatter(std::ostream& stream, const tirexResult* info, const tirexResult* result) noexcept;
	extern void jsonFormatter(std::ostream& stream, const tirexResult* info, const tirexResult* result) noexcept;
	extern void irmetadataFormatter(std::ostream& stream, const tirexResult* info, const tirexResult* result) noexcept;
} // namespace tirex

#endif