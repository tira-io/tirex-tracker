#ifndef MEASURE_UTILS_SHAREDLIB_HPP
#define MEASURE_UTILS_SHAREDLIB_HPP

#include <string>

#if defined(__linux__) || defined(__APPLE__)
#include <dlfcn.h>
#elif defined(_WINDOWS) || defined(_WIN32) || defined(WIN32)
#define NOGDI // Otherwise we get problems with logging
#include <windows.h>
#else
#error "Unsupported OS"
#endif

namespace tirex::utils {

	namespace details {
#if defined(__linux__) || defined(__APPLE__)
		using libhandle = void*;
		inline libhandle openlib(const std::string& path) noexcept { return dlopen(path.c_str(), RTLD_LAZY); }
		inline void closelib(libhandle handle) noexcept { dlclose(handle); }
		template <typename T>
		inline T loadfromlib(libhandle handle, const std::string& name) noexcept {
			return reinterpret_cast<T>(dlsym(handle, name.c_str()));
		}
#elif defined(_WINDOWS) || defined(_WIN32) || defined(WIN32)
		using libhandle = HINSTANCE;
		inline libhandle openlib(const std::string& path) { return LoadLibrary((LPCSTR)path.c_str()); }
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
		explicit SharedLib(const std::string& path) noexcept : handle(details::openlib(path)) {}
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