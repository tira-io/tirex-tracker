include(FetchContent)
include(ExternalProject)

add_library(tirex_tracker SHARED)
add_library(tirex_tracker_static STATIC)
add_library(tirex::tracker ALIAS tirex_tracker)
add_library(tirex::tracker_static ALIAS tirex_tracker_static)

target_sources(tirex_tracker
	PUBLIC
		FILE_SET tirex_tracker_headers
		TYPE HEADERS
		BASE_DIRS
			${CMAKE_CURRENT_LIST_DIR}/../include
			${CMAKE_CURRENT_BINARY_DIR}/
		FILES
			${CMAKE_CURRENT_LIST_DIR}/../include/tirex_tracker.h
			${CMAKE_CURRENT_BINARY_DIR}/tirex_tracker_export.h
	PRIVATE
		measureapi.cpp
		measureinfo.cpp
		measureresult.cpp
		logging.cpp
		measure/stats/provider.cpp

		measure/stats/energystats.cpp
		measure/stats/gitstats.cpp
		measure/stats/nvmlstats.cpp
		measure/stats/systemstats.cpp
		measure/stats/systemstats_linux.cpp
		measure/stats/systemstats_macos.cpp
		measure/stats/systemstats_windows.cpp
)

target_compile_features(tirex_tracker PUBLIC cxx_std_20)
target_compile_features(tirex_tracker PUBLIC c_std_99)


target_sources(tirex_tracker_static
	PUBLIC
		FILE_SET tirex_tracker_headers
		TYPE HEADERS
		BASE_DIRS
			${CMAKE_CURRENT_LIST_DIR}/../include
			${CMAKE_CURRENT_BINARY_DIR}/
		FILES
			${CMAKE_CURRENT_LIST_DIR}/../include/tirex_tracker.h
			${CMAKE_CURRENT_BINARY_DIR}/tirex_tracker_export.h
	PRIVATE
		measureapi.cpp
		measureinfo.cpp
		measureresult.cpp
		logging.cpp
		measure/stats/provider.cpp

		measure/stats/energystats.cpp
		measure/stats/gitstats.cpp
		measure/stats/nvmlstats.cpp
		measure/stats/systemstats.cpp
		measure/stats/systemstats_linux.cpp
		measure/stats/systemstats_macos.cpp
		measure/stats/systemstats_windows.cpp
)
target_compile_features(tirex_tracker_static PUBLIC cxx_std_20)
target_compile_features(tirex_tracker PUBLIC c_std_99)

if (LINUX)
target_link_libraries(tirex_tracker PUBLIC ${CMAKE_DL_LIBS})  # dlopen, dlclose, ...
target_link_libraries(tirex_tracker_static PUBLIC ${CMAKE_DL_LIBS})  # dlopen, dlclose, ...
elseif(WIN32)
target_link_libraries(tirex_tracker PUBLIC Powrprof)
target_link_libraries(tirex_tracker_static PUBLIC Powrprof)
endif()

include(GenerateExportHeader)
generate_export_header(tirex_tracker BASE_NAME tirex_tracker)
# generate_export_header(tirex_tracker_static BASE_NAME tirex_tracker)
# set_target_properties(tirex_tracker_static PROPERTIES COMPILE_FLAGS -DTIREX_TRACKER_STATIC_DEFINE)
target_compile_definitions(tirex_tracker_static PUBLIC TIREX_TRACKER_STATIC_DEFINE)

set_target_properties(tirex_tracker PROPERTIES
	SOVERSION ${TIREX_TRACKER_VER_MAJOR}
	VERSION   ${TIREX_TRACKER_VERSION}
	CXX_VISIBILITY_PRESET hidden
	C_VISIBILITY_PRESET hidden
	VISIBILITY_INLINES_HIDDEN YES
)
set_target_properties(tirex_tracker_static PROPERTIES
	SOVERSION ${TIREX_TRACKER_VER_MAJOR}
	VERSION   ${TIREX_TRACKER_VERSION}
	CXX_VISIBILITY_PRESET hidden
	C_VISIBILITY_PRESET hidden
	VISIBILITY_INLINES_HIDDEN YES
)

##########################################################################################
# Libraries
##########################################################################################
# All our dependencies must be static libraries and be compiled with PIC on
set(BUILD_SHARED_LIBS OFF)
set_property(TARGET tirex_tracker_static PROPERTY POSITION_INDEPENDENT_CODE ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# CPPJoule
if (NOT (UNIX AND APPLE)) # CPPJouls does not support macos right now
	FetchContent_Declare(cppjoule GIT_REPOSITORY https://github.com/TheMrSheldon/CPPJoules.git GIT_TAG pcm EXCLUDE_FROM_ALL)#GIT_TAG v1.0)
	FetchContent_MakeAvailable(cppjoule)
	target_link_libraries(tirex_tracker PRIVATE CPP_Joules)
	target_link_libraries(tirex_tracker_static PRIVATE CPP_Joules)
endif()

# LibGit
set(USE_SSH OFF CACHE BOOL "" FORCE)
set(USE_HTTPS OFF CACHE BOOL "" FORCE)
set(REGEX_BACKEND "builtin" CACHE STRING "" FORCE)
set(BUILD_CLI OFF CACHE BOOL "" FORCE) # Do not build the Git CLI
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(libgit GIT_REPOSITORY https://github.com/libgit2/libgit2.git GIT_TAG v1.8.4 EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(libgit)
#target_link_libraries(tirex_tracker PRIVATE libgit2package)
#target_link_libraries(tirex_tracker_static PRIVATE libgit2package)
#target_include_directories(tirex_tracker PRIVATE $<TARGET_PROPERTY:libgit2,INCLUDE_DIRECTORIES>)
#target_include_directories(tirex_tracker_static PRIVATE $<TARGET_PROPERTY:libgit2,INCLUDE_DIRECTORIES>)
target_link_libraries(tirex_tracker PRIVATE libgit2 libgit2package)
target_link_libraries(tirex_tracker_static PRIVATE libgit2 libgit2package)

# NVML (We load the go-nvml bindings since there does not seem to be a repo with just the header)
FetchContent_Declare(nvml GIT_REPOSITORY https://github.com/NVIDIA/go-nvml.git GIT_TAG v0.12.4-0 EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(nvml)
target_include_directories(tirex_tracker PRIVATE ${nvml_SOURCE_DIR}/gen/)
target_include_directories(tirex_tracker_static PRIVATE ${nvml_SOURCE_DIR}/gen/)

# FMT (remove in the future when std::format can be assumed)
FetchContent_Declare(fmt GIT_REPOSITORY https://github.com/fmtlib/fmt.git GIT_TAG 11.1.3 EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(fmt)
target_link_libraries(tirex_tracker PRIVATE fmt::fmt)
target_link_libraries(tirex_tracker_static PRIVATE fmt::fmt)

# cpuinfo
# option(CPUINFO_LOG_TO_STDIO OFF)
option(CPUINFO_BUILD_TOOLS OFF)
option(CPUINFO_BUILD_UNIT_TESTS OFF)
option(CPUINFO_BUILD_MOCK_TESTS OFF)
option(CPUINFO_BUILD_BENCHMARKS OFF)
# set(CPUINFO_LOG_LEVEL "debug")
FetchContent_Declare(cpuinfo GIT_REPOSITORY https://github.com/pytorch/cpuinfo.git GIT_TAG 5e3d2445e6a84d9599bee2bf78edbb4d80865e1d EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(cpuinfo)
target_link_libraries(tirex_tracker PRIVATE cpuinfo::cpuinfo)
target_link_libraries(tirex_tracker_static PRIVATE cpuinfo::cpuinfo)

# Mac Only: Bind to CoreFoundation to get IOReport for system information
if (UNIX AND APPLE)
	target_link_libraries(tirex_tracker PRIVATE "$<LINK_LIBRARY:FRAMEWORK,CoreFoundation>")
	target_link_libraries(tirex_tracker PRIVATE "$<LINK_LIBRARY:FRAMEWORK,IOKit>")
	target_link_libraries(tirex_tracker_static PRIVATE "$<LINK_LIBRARY:FRAMEWORK,CoreFoundation>")
	target_link_libraries(tirex_tracker_static PRIVATE "$<LINK_LIBRARY:FRAMEWORK,IOKit>")
endif()

# Hash
FetchContent_Declare(hash GIT_REPOSITORY https://github.com/Chocobo1/Hash.git GIT_TAG 250fd19b9b6f3fda714f5071ab477057615fb6d0 EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(hash)
target_include_directories(tirex_tracker PRIVATE ${hash_SOURCE_DIR}/src)
target_include_directories(tirex_tracker_static PRIVATE ${hash_SOURCE_DIR}/src)
