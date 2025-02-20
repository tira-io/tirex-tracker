include(FetchContent)
include(ExternalProject)

# Must be shared since it should interface with other languages as well (TODO: add measureapi and measureapi_shared)
add_library(measureapi SHARED
	measureapi.cpp
	measureinfo.cpp
	measureresult.cpp
	logging.cpp
	measure/stats/provider.cpp

	measure/stats/energystats.cpp
	measure/stats/gitstats.cpp
	measure/stats/gpustats.cpp
	measure/stats/systemstats.cpp
	measure/stats/systemstats_linux.cpp
	measure/stats/systemstats_macos.cpp
)

target_compile_features(measureapi PUBLIC cxx_std_20)
target_include_directories(measureapi PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../include)
set_property(TARGET measureapi PROPERTY POSITION_INDEPENDENT_CODE ON)


add_library(measureapi_static STATIC
	measureapi.cpp
	measureinfo.cpp
	measureresult.cpp
	logging.cpp
	measure/stats/provider.cpp

	measure/stats/energystats.cpp
	measure/stats/gitstats.cpp
	measure/stats/gpustats.cpp
	measure/stats/systemstats.cpp
	measure/stats/systemstats_linux.cpp
	measure/stats/systemstats_macos.cpp
)
target_compile_features(measureapi_static PUBLIC cxx_std_20)
target_include_directories(measureapi_static PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../include)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	# Statically link standard library for c++
	# target_link_options(measureapi PRIVATE -static-libgcc -static-libstdc++)
endif()

target_link_libraries(measureapi PUBLIC dl)  # dlopen, dlclose, ...


set(BUILD_SHARED_LIBS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


##########################################################################################
# Libraries
##########################################################################################
# CPPJoule
if (NOT (UNIX AND APPLE)) # CPPJouls does not support macos right now
	FetchContent_Declare(cppjoule GIT_REPOSITORY https://github.com/TheMrSheldon/CPPJoules.git GIT_TAG main)#GIT_TAG v1.0)
	FetchContent_MakeAvailable(cppjoule)
	set_property(TARGET CPP_Joules PROPERTY POSITION_INDEPENDENT_CODE ON)
	target_link_libraries(measureapi PRIVATE CPP_Joules)
	target_link_libraries(measureapi_static PRIVATE CPP_Joules)
endif()

# LibGit
option(USE_HTTPS OFF)
option(BUILD_CLI OFF) # Do not build the Git CLI
option(BUILD_TESTS OFF)
FetchContent_Declare(libgit GIT_REPOSITORY https://github.com/libgit2/libgit2.git GIT_TAG v1.8.4)
FetchContent_MakeAvailable(libgit)
set_property(TARGET libgit2 PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET libgit2package PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(measureapi PRIVATE libgit2 libgit2package)
target_link_libraries(measureapi_static PRIVATE libgit2 libgit2package)

# NVML (We load the go-nvml bindings since there does not seem to be a repo with just the header)
FetchContent_Declare(nvml GIT_REPOSITORY https://github.com/NVIDIA/go-nvml.git GIT_TAG v0.12.4-0)
FetchContent_MakeAvailable(nvml)
target_include_directories(measureapi PRIVATE ${nvml_SOURCE_DIR}/gen/)
target_include_directories(measureapi_static PRIVATE ${nvml_SOURCE_DIR}/gen/)

# FMT (remove in the future when std::format can be assumed)
FetchContent_Declare(fmt GIT_REPOSITORY https://github.com/fmtlib/fmt.git GIT_TAG 11.1.3)
FetchContent_MakeAvailable(fmt)
set_property(TARGET fmt PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(measureapi PRIVATE fmt::fmt)
target_link_libraries(measureapi_static PRIVATE fmt::fmt)

# lscpu
# option(CPUINFO_LOG_TO_STDIO OFF)
option(CPUINFO_BUILD_TOOLS OFF)
option(CPUINFO_BUILD_UNIT_TESTS OFF)
option(CPUINFO_BUILD_MOCK_TESTS OFF)
option(CPUINFO_BUILD_BENCHMARKS OFF)
# set(CPUINFO_LOG_LEVEL "debug")
FetchContent_Declare(cpuinfo GIT_REPOSITORY https://github.com/pytorch/cpuinfo.git GIT_TAG aaac07ee499895770c89163ce0920ef8bb41ed23)
FetchContent_MakeAvailable(cpuinfo)
set_property(TARGET cpuinfo PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(measureapi PRIVATE cpuinfo::cpuinfo)
target_link_libraries(measureapi_static PRIVATE cpuinfo::cpuinfo)

# Mac Only: Bind to CoreFoundation to get IOReport for system information
if (UNIX AND APPLE)
	#if(CMAKE_OBJCXX_LINK_LIBRARY_USING_FRAMEWORK_SUPPORTED)
		target_link_libraries(measureapi PRIVATE "$<LINK_LIBRARY:FRAMEWORK,CoreFoundation>")
		target_link_libraries(measureapi_static PRIVATE "$<LINK_LIBRARY:FRAMEWORK,CoreFoundation>")
	#else()
	#	message(FATAL_ERROR "failed to link to corefoundation")
	#endif()
endif()

# OpenSSL (https://stackoverflow.com/a/72187679)
set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL COMPONENTS SSL Crypto)
if (NOT ${OPENSSL_FOUND})
	set(OPENSSL_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/openssl-src)
	set(OPENSSL_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/openssl)
	set(OPENSSL_INCLUDE_DIR ${OPENSSL_INSTALL_DIR}/include)
	set(OPENSSL_CONFIGURE_COMMAND ${OPENSSL_SOURCE_DIR}/config)
	ExternalProject_Add(
		OpenSSL
		SOURCE_DIR ${OPENSSL_SOURCE_DIR}
		GIT_REPOSITORY https://github.com/openssl/openssl.git
		GIT_TAG OpenSSL_1_1_1n
		USES_TERMINAL_DOWNLOAD TRUE
		CONFIGURE_COMMAND
			${OPENSSL_CONFIGURE_COMMAND}
			--prefix=${OPENSSL_INSTALL_DIR}
			--openssldir=${OPENSSL_INSTALL_DIR}
		BUILD_COMMAND make
		TEST_COMMAND ""
		INSTALL_COMMAND make install
		INSTALL_DIR ${OPENSSL_INSTALL_DIR}
	)
	file(MAKE_DIRECTORY ${OPENSSL_INCLUDE_DIR})
	set(OPENSSL_LIBRARY_SUFFIX a)

	add_library(OpenSSL::SSL STATIC IMPORTED GLOBAL)
	set_property(TARGET OpenSSL::SSL PROPERTY IMPORTED_LOCATION ${OPENSSL_INSTALL_DIR}/lib/libssl.${OPENSSL_LIBRARY_SUFFIX})
	set_property(TARGET OpenSSL::SSL PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${OPENSSL_INCLUDE_DIR})
	add_dependencies(OpenSSL::SSL OpenSSL)

	add_library(OpenSSL::Crypto STATIC IMPORTED GLOBAL)
	set_property(TARGET OpenSSL::Crypto PROPERTY IMPORTED_LOCATION ${OPENSSL_INSTALL_DIR}/lib/libcrypto.${OPENSSL_LIBRARY_SUFFIX})
	set_property(TARGET OpenSSL::Crypto PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${OPENSSL_INCLUDE_DIR})
	add_dependencies(OpenSSL::Crypto OpenSSL)

	target_link_libraries(measureapi PRIVATE OpenSSL::SSL OpenSSL::Crypto)
	target_link_libraries(measureapi_static PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

target_link_libraries(measureapi PRIVATE ${OPENSSL_LIBRARIES})
target_link_libraries(measureapi_static PRIVATE ${OPENSSL_LIBRARIES})