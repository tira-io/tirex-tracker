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

	/**
	 * @brief The `details` namespace contains implementation-specific could and is not part of the public API.
	 */
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
		details::libhandle handle; /**< A native handle to the loaded shared library. **/

		SharedLib(const SharedLib& other) = delete;			   /** Copying does not make sense here. */
		SharedLib& operator=(const SharedLib& other) = delete; /** Copying does not make sense here. */

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
		/**
		 * @brief Construct a new, uninitialized library object.
		 * @brief SharedLib::good will be false in this instance.
		 */
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

		/**
		 * @brief Checks if the library is successfully initialized.
		 * 
		 * @return true iff the library is successfully initialized.
		 */
		bool good() const noexcept { return handle != nullptr; }

		operator bool() const noexcept { return good(); }
	};

} // namespace tirex::utils

#endif