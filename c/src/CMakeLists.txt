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
# Set the C++ standard to C++20.
target_compile_features(tirex_tracker PRIVATE cxx_std_20)
# Include the header files.
target_include_directories(tirex_tracker PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../include)
# Configure the library build.
set_property(TARGET tirex_tracker PROPERTY POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(tirex_tracker PRIVATE TIREX_TRACKER_LIB_EXPORT)
target_compile_definitions(tirex_tracker PUBLIC TIREX_TRACKER_VERSION=${TIREX_TRACKER_VERSION})


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
# Set the C++ standard to C++20.
target_compile_features(tirex_tracker_static PRIVATE cxx_std_20)
# Include the header files.
target_include_directories(tirex_tracker_static PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../include)
target_compile_definitions(tirex_tracker_static PRIVATE TIREX_TRACKER_STATIC)
target_compile_definitions(tirex_tracker PUBLIC TIREX_TRACKER_VERSION=${TIREX_TRACKER_VERSION})


####################
# Platform options #
####################
set(BUILD_SHARED_LIBS OFF)
set_property(TARGET tirex_tracker_static PROPERTY POSITION_INDEPENDENT_CODE ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (LINUX)
	# Needed for: dlopen, dlclose, ...
	target_link_libraries(tirex_tracker PRIVATE dl) 
	target_link_libraries(tirex_tracker_static PRIVATE dl)
elseif(WIN32)
	target_link_libraries(tirex_tracker PRIVATE Powrprof)
	target_link_libraries(tirex_tracker_static PRIVATE Powrprof)
endif()

# On macOS, bind to CoreFoundation to get IOReport for system information.
if (UNIX AND APPLE)
	target_link_libraries(tirex_tracker PRIVATE "$<LINK_LIBRARY:FRAMEWORK,CoreFoundation>")
	target_link_libraries(tirex_tracker PRIVATE "$<LINK_LIBRARY:FRAMEWORK,IOKit>")
	target_link_libraries(tirex_tracker_static PRIVATE "$<LINK_LIBRARY:FRAMEWORK,CoreFoundation>")
	target_link_libraries(tirex_tracker_static PRIVATE "$<LINK_LIBRARY:FRAMEWORK,IOKit>")
endif()


#########################
# Third-party libraries #
#########################

# Install CPPJoules library from GitHub.
if (NOT (UNIX AND APPLE)) # CPPJoules does not support macos right now
	FetchContent_Declare(cppjoule GIT_REPOSITORY https://github.com/TheMrSheldon/CPPJoules.git GIT_TAG main) # FIXME: switch to GIT_TAG v1.0 once it is released.
	FetchContent_MakeAvailable(cppjoule)
	set_property(TARGET CPP_Joules PROPERTY POSITION_INDEPENDENT_CODE ON)
	target_link_libraries(tirex_tracker PRIVATE CPP_Joules)
	target_link_libraries(tirex_tracker_static PRIVATE CPP_Joules)
endif()

# Install libgit2 library from GitHub.
set(USE_SSH OFF CACHE BOOL "" FORCE)
set(USE_HTTPS OFF CACHE BOOL "" FORCE)
set(REGEX_BACKEND "builtin" CACHE STRING "" FORCE)
set(BUILD_CLI OFF CACHE BOOL "" FORCE) # Do not build the Git CLI.
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(libgit GIT_REPOSITORY https://github.com/libgit2/libgit2.git GIT_TAG v1.8.4)
FetchContent_MakeAvailable(libgit)
set_property(TARGET libgit2 PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET libgit2package PROPERTY POSITION_INDEPENDENT_CODE ON)
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
FetchContent_Declare(fmt GIT_REPOSITORY https://github.com/fmtlib/fmt.git GIT_TAG 11.1.3)
FetchContent_MakeAvailable(fmt)
set_property(TARGET fmt PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(tirex_tracker PRIVATE fmt::fmt)
target_link_libraries(tirex_tracker_static PRIVATE fmt::fmt)

# Install cpuinfo library from GitHub.
FetchContent_Declare(cpuinfo GIT_REPOSITORY https://github.com/pytorch/cpuinfo.git GIT_TAG aaac07ee499895770c89163ce0920ef8bb41ed23)
FetchContent_MakeAvailable(cpuinfo)
set(CPUINFO_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(CPUINFO_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(CPUINFO_BUILD_MOCK_TESTS OFF CACHE BOOL "" FORCE)
set(CPUINFO_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set_property(TARGET cpuinfo PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(tirex_tracker PRIVATE cpuinfo::cpuinfo)
target_link_libraries(tirex_tracker_static PRIVATE cpuinfo::cpuinfo)

# Install SHA-1 library from GitHub.
FetchContent_Declare(sha1 GIT_REPOSITORY https://github.com/vog/sha1.git GIT_TAG 1.4)
FetchContent_MakeAvailable(sha1)
target_include_directories(tirex_tracker PRIVATE ${sha1_SOURCE_DIR})
target_include_directories(tirex_tracker_static PRIVATE ${sha1_SOURCE_DIR})
