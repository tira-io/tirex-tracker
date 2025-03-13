cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

# Include helper module.
include(FetchContent)


##################
# Shared library #
##################
add_library(tirex_tracker SHARED
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
	measure/stats/systemstats_windows.cpp
)
# Set the C++ standard to C++17.
target_compile_features(tirex_tracker PRIVATE cxx_std_17)
# Include the header files.
target_include_directories(tirex_tracker PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../include)
# Configure the library build.
target_compile_definitions(tirex_tracker PRIVATE TIREX_TRACKER_LIB_EXPORT)
target_compile_definitions(tirex_tracker PUBLIC TIREX_TRACKER_VERSION=${TIREX_TRACKER_VERSION})
set_target_properties(tirex_tracker PROPERTIES POSITION_INDEPENDENT_CODE ON)


##################
# Static library #
##################
add_library(tirex_tracker_static STATIC
	measure/stats/energystats.cpp
	measure/stats/gitstats.cpp
	measure/stats/gpustats.cpp
	measure/stats/provider.cpp
	measure/stats/systemstats_linux.cpp
	measure/stats/systemstats_macos.cpp
	measure/stats/systemstats_windows.cpp
	measure/stats/systemstats.cpp
	logging.cpp
	measureapi.cpp
	measureinfo.cpp
	measureresult.cpp
)
# Set the C++ standard to C++17.
target_compile_features(tirex_tracker_static PRIVATE cxx_std_17)
# Include the header files.
target_include_directories(tirex_tracker_static PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../include)
target_compile_definitions(tirex_tracker_static PRIVATE TIREX_TRACKER_STATIC)
target_compile_definitions(tirex_tracker_static PUBLIC TIREX_TRACKER_VERSION=${TIREX_TRACKER_VERSION})
set_target_properties(tirex_tracker_static PROPERTIES POSITION_INDEPENDENT_CODE ON)


#########################
# Link system libraries #
#########################
if (UNIX AND NOT APPLE)
	# On Linux, link dl for: dlopen, dlclose, etc.
	target_link_libraries(tirex_tracker PRIVATE ${CMAKE_DL_LIBS}) 
	target_link_libraries(tirex_tracker_static PRIVATE ${CMAKE_DL_LIBS})
endif()
if(WIN32)
	# On Windows, link Powrprof to get CallNtPowerInformation for power usage.
	target_link_libraries(tirex_tracker PRIVATE Powrprof)
	target_link_libraries(tirex_tracker_static PRIVATE Powrprof)
endif()
if (UNIX AND APPLE)
	# On macOS, link CoreFoundation to get IOReport for system information.
	target_link_libraries(tirex_tracker PRIVATE "$<LINK_LIBRARY:FRAMEWORK,CoreFoundation>")
	target_link_libraries(tirex_tracker_static PRIVATE "$<LINK_LIBRARY:FRAMEWORK,CoreFoundation>")
	# TODO: Describe why we link IOKit, or remove it.
	target_link_libraries(tirex_tracker PRIVATE "$<LINK_LIBRARY:FRAMEWORK,IOKit>")
	target_link_libraries(tirex_tracker_static PRIVATE "$<LINK_LIBRARY:FRAMEWORK,IOKit>")
endif()


#########################
# Third-party libraries #
#########################

# Install CPPJoules library from GitHub.
if (NOT (UNIX AND APPLE)) # CPPJoules does not support macos right now
	FetchContent_Declare(cppjoules GIT_REPOSITORY https://github.com/TheMrSheldon/CPPJoules.git GIT_TAG main) # FIXME: switch to GIT_TAG v1.0 once it is released.
	FetchContent_MakeAvailable(cppjoules)
	set_target_properties(CPP_Joules PROPERTIES POSITION_INDEPENDENT_CODE ON)
	target_link_libraries(tirex_tracker PRIVATE CPP_Joules)
	target_link_libraries(tirex_tracker_static PRIVATE CPP_Joules)
endif()

# Install libgit2 library from GitHub.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_CLI OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_FUZZERS OFF CACHE BOOL "" FORCE)
set(USE_THREADS OFF CACHE BOOL "" FORCE)
set(USE_SSH OFF CACHE BOOL "" FORCE)
set(USE_HTTPS OFF CACHE BOOL "" FORCE)
set(USE_REGEX "builtin" CACHE STRING "" FORCE)
set(USE_COMPRESSION "builtin" CACHE STRING "" FORCE)
FetchContent_Declare(libgit GIT_REPOSITORY https://github.com/libgit2/libgit2.git GIT_TAG v1.8.4)
FetchContent_MakeAvailable(libgit)
# set_target_properties(libgit2 PROPERTIES POSITION_INDEPENDENT_CODE ON)
# set_target_properties(libgit2package PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(tirex_tracker PRIVATE libgit2 libgit2package)
target_link_libraries(tirex_tracker_static PRIVATE libgit2 libgit2package)

# Install NVML headers from GitHub.
# Note: We load the go-nvml bindings since there does not seem to be a repo with just the header.
FetchContent_Declare(nvml GIT_REPOSITORY https://github.com/NVIDIA/go-nvml.git GIT_TAG v0.12.4-0)
FetchContent_MakeAvailable(nvml)
target_include_directories(tirex_tracker PRIVATE ${nvml_SOURCE_DIR}/gen/)
target_include_directories(tirex_tracker_static PRIVATE ${nvml_SOURCE_DIR}/gen/)

# Install {fmt} library from GitHub.
# TODO: Remove in the future when std::format can be assumed to be available.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(FMT_DOC OFF CACHE BOOL "" FORCE)
set(FMT_TEST OFF CACHE BOOL "" FORCE)
set(FMT_FUZZ OFF CACHE BOOL "" FORCE)
set(FMT_CUDA_TEST OFF CACHE BOOL "" FORCE)
FetchContent_Declare(fmt GIT_REPOSITORY https://github.com/fmtlib/fmt.git GIT_TAG 11.1.3)
FetchContent_MakeAvailable(fmt)
set_target_properties(fmt PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(tirex_tracker PRIVATE fmt)
target_link_libraries(tirex_tracker_static PRIVATE fmt)

# Install cpuinfo library from GitHub.
set(CPUINFO_LIBRARY_TYPE static CACHE STRING "" FORCE)
set(CPUINFO_RUNTIME_TYPE static CACHE STRING "" FORCE)
set(CPUINFO_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(CPUINFO_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(CPUINFO_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(CPUINFO_BUILD_MOCK_TESTS OFF CACHE BOOL "" FORCE)
set(CPUINFO_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(CPUINFO_BUILD_PKG_CONFIG OFF CACHE BOOL "" FORCE)
FetchContent_Declare(cpuinfo GIT_REPOSITORY https://github.com/pytorch/cpuinfo.git GIT_TAG aaac07ee499895770c89163ce0920ef8bb41ed23)
FetchContent_MakeAvailable(cpuinfo)
set_target_properties(cpuinfo PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(tirex_tracker PRIVATE cpuinfo)
target_link_libraries(tirex_tracker_static PRIVATE cpuinfo)

# Install SHA-1 library from GitHub.
FetchContent_Declare(sha1 GIT_REPOSITORY https://github.com/vog/sha1.git GIT_TAG 1.4)
FetchContent_MakeAvailable(sha1)
target_include_directories(tirex_tracker PRIVATE ${sha1_SOURCE_DIR})
target_include_directories(tirex_tracker_static PRIVATE ${sha1_SOURCE_DIR})
