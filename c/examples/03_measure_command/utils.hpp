#ifndef UTILS_HPP
#define UTILS_HPP

#include "logging.hpp"

#include <tirex_tracker.h>

#include <string>
#include <vector>

#if defined(__GNUC__)
#if defined(__clang__)
static constexpr const char* compiler = "clang " __VERSION__;
#else
static constexpr const char* compiler = "gcc " __VERSION__;
#endif
#elif defined(_MSC_VER)
#define STRINGIFY(L) #L
#define MAKESTRING(M, L) M(L)
#define STRINGIZE(X) MAKESTRING(STRINGIFY, X)
static constexpr const char* compiler = "msvc " STRINGIZE(_MSC_FULL_VER);
#else
static constexpr const char* compiler = "unknown compiler";
#endif

static std::string buildVersionString() {
	// Uncomment and use std::format once we can assume enough adoption
	// auto versionString = std::format("{}\nBuilt with {} for C++ {}", tirex::getVersionStr(), compiler, __cplusplus);
	auto versionString = std::string(tirex::getVersionStr()) + "\nBuilt with " + compiler + " for C++ " +
						 std::to_string(__cplusplus);
	auto numProviders = tirexDataProviderGetAll(nullptr, 0);
	std::vector<tirexDataProvider> buf{numProviders};
	tirexDataProviderGetAll(buf.data(), buf.size());
	for (const auto& provider : buf)
		if (provider.version)
			versionString += std::string("\n") + provider.version;
	return versionString;
}

#endif