#ifndef UTILS_HPP
#define UTILS_HPP

#include "logging.hpp"

#include <measure.h>

#include <format>
#include <string>

#if defined(__GNUC__)
#if defined(__clang__)
static constexpr const char* compiler = "clang " __VERSION__;
#else
static constexpr const char* compiler = "gcc " __VERSION__;
#endif
#elif defined(_MSC_VER)
static constexpr const char* compiler = "msvc " _MSC_FULL_VER;
#else
static constexpr const char* compiler = "unknown compiler";
#endif

static std::string buildVersionString() {
	auto versionString = std::format("{}\nBuilt with {} for C++ {}", msr::getVersionStr(), compiler, __cplusplus);
	auto numProviders = msrDataProviderGetAll(nullptr, 0);
	/** I don't know how compatible this is accross compilers; otherwise use a vector. **/
	msrDataProvider buf[numProviders];
	msrDataProviderGetAll(buf, sizeof(buf) / sizeof(buf[0]));
	for (const auto& provider : buf)
		if (provider.version)
			versionString += std::string("\n") + provider.version;
	return versionString;
}

#endif