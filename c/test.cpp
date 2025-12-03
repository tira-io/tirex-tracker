#include <windows.h>
#include <winternl.h>

#ifndef MEASURE_UTILS_SHAREDLIB_HPP
#define MEASURE_UTILS_SHAREDLIB_HPP

#include <filesystem>

#if defined(__linux__) || defined(__APPLE__)
#include <dlfcn.h>
#elif defined(_WINDOWS) || defined(_WIN32) || defined(WIN32)
#include <windows.h>
#else
#error "Unsupported OS"
#endif

namespace tirex::utils {

	namespace details {
#if defined(__linux__) || defined(__APPLE__)
		using libhandle = void*;
		inline libhandle openlib(const std::filesystem::path& path) noexcept { return dlopen(path.c_str(), RTLD_LAZY); }
		inline void closelib(libhandle handle) noexcept { dlclose(handle); }
		template <typename T>
		inline T loadfromlib(libhandle handle, const std::string& name) noexcept {
			return reinterpret_cast<T>(dlsym(handle, name.c_str()));
		}
#elif defined(_WINDOWS) || defined(_WIN32) || defined(WIN32)
		using libhandle = HINSTANCE;
		inline libhandle openlib(const std::filesystem::path& path) { return LoadLibrary((LPCSTR)path.c_str()); }
		inline void closelib(libhandle handle) noexcept { /** Not possible */ }
		template <typename T>
		inline T loadfromlib(libhandle handle, const std::string& name) noexcept {
			return reinterpret_cast<T>(GetProcAddress(handle, name.c_str()));
		}
#else
#error "Unsupported OS"
#endif
	}; // namespace details

	struct SharedLib {
	private:
		details::libhandle handle;

		SharedLib(const SharedLib& other) = delete;
		SharedLib& operator=(const SharedLib& other) = delete;

		void destroy() {
			if (handle == nullptr)
				return;
			details::closelib(handle);
			handle = nullptr;
		}

	protected:
		template <typename T>
		constexpr T load(const std::string& name) noexcept {
			return details::loadfromlib<T>(handle, name);
		}

	public:
		SharedLib() noexcept : handle(nullptr) {}
		SharedLib(const std::filesystem::path& path) noexcept : handle(details::openlib(path)) {}
		SharedLib(SharedLib&& other) noexcept : handle(std::move(other.handle)) { other.handle = nullptr; }

		virtual ~SharedLib() { destroy(); }

		SharedLib& operator=(SharedLib&& other) noexcept {
			destroy();
			handle = std::move(other.handle);
			other.handle = nullptr;
			return *this;
		}

		bool good() const noexcept { return handle != nullptr; }

		operator bool() const noexcept { return good(); }
	};

} // namespace tirex::utils

#endif

using tirex::utils::SharedLib;

struct NTDLL final : tirex::utils::SharedLib {
public:
	using QUERY_INFORMATION_PROCESS = NTSTATUS (*)(
			HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation,
			ULONG ProcessInformationLength, PULONG ReturnLength
	);
	QUERY_INFORMATION_PROCESS queryInformationProcess = load<QUERY_INFORMATION_PROCESS>({"NtQueryInformationProcess"});

	NTDLL() : tirex::utils::SharedLib("ntdll.dll") {}
};

int main(int argc, char* argv[]) {
	NTDLL nt;
	if (!nt.good()) {
		std::cout << "Error loading ntdll.dll" << std::endl;
		return 1;
	}

	PULONG bufsize;
	char buffer[4096];
	HANDLE handle = GetCurrentProcess();
	NTSTATUS status = nt.queryInformationProcess(
			handle, static_cast<PROCESSINFOCLASS>(60) /*ProcessCommandLineInformation*/, buffer, sizeof(buffer),
			&bufsize
	);
	if (status != 0) {
		std::cout << "NtQueryInformationProcess failed with code " << status << std::endl;
		return 2;
	}
	std::cout << std::string_view{buffer, buffer + bufsize} << std::endl;
}