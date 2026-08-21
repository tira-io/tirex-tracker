#ifndef STATS_WINDOWS_NTDLL_HPP
#define STATS_WINDOWS_NTDLL_HPP

#include "../../../utils/sharedlib.hpp"

#define NOGDI // Otherwise we get problems with logging
#include <windows.h>
#include <winternl.h>

/**
 * @brief Lazily resolves undocumented ntdll.dll exports that have no public declaration in the Windows SDK.
 */
struct NTDLL final : tirex::utils::SharedLib {
public:
	using QUERY_INFORMATION_PROCESS = NTSTATUS (*)(
			HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation,
			ULONG ProcessInformationLength, PULONG ReturnLength
	);
	QUERY_INFORMATION_PROCESS queryInformationProcess = load<QUERY_INFORMATION_PROCESS>({"NtQueryInformationProcess"});

	NTDLL() : tirex::utils::SharedLib(std::string{"ntdll.dll"}) {}
};

/**
 * @brief The process-wide NTDLL instance. Guaranteed by the language to be a single, lazily-initialized instance
 * shared across every translation unit that calls this function, regardless of how many include this header.
 */
inline NTDLL& ntdll() {
	static NTDLL instance;
	return instance;
}

#endif
