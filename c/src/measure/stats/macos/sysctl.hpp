#ifndef STATS_MACOS_SYSCTL_HPP
#define STATS_MACOS_SYSCTL_HPP

#include <cassert>
#include <string>

#include <sys/sysctl.h>

template <typename T>
inline T getSysctl(const char* name);

template <>
inline std::string getSysctl<std::string>(const char* name) {
	size_t len;
	sysctlbyname(name, nullptr, &len, nullptr, 0);
	char str[len];
	sysctlbyname(name, str, &len, nullptr, 0);
	assert(str[len - 1] == '\0');
	return {str, str + len - 1};
}

template <typename T>
inline T getSysctl(const char* name) {
	T val;
	size_t size = sizeof(val);
	if (sysctlbyname(name, &val, &size, nullptr, 0) == -1)
		abort(); /** \todo handle more gracefully **/
	return val;
}

#endif