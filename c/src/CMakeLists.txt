include(FetchContent)
include(ExternalProject)

if (TIREX_TRACKER_BUILD_SHARED_LIB)
	add_library(tirex_tracker SHARED)
else()
	add_library(tirex_tracker STATIC)
endif()

add_library(tirex::tracker ALIAS tirex_tracker)

target_sources(tirex_tracker
	PUBLIC
		FILE_SET tirex_tracker_headers
		TYPE HEADERS
		BASE_DIRS
			${CMAKE_CURRENT_LIST_DIR}/../include
			${CMAKE_CURRENT_BINARY_DIR}/
			${TIREX_EXT_INCLUDE_DIRS}
		FILES
			${CMAKE_CURRENT_LIST_DIR}/../include/tirex_tracker.h
			${CMAKE_CURRENT_BINARY_DIR}/tirex_tracker_export.h
			${TIREX_EXT_INCLUDE_FILES}
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

		${TIREX_EXT_SOURCES}
)

target_compile_features(tirex_tracker PUBLIC cxx_std_20)

if (LINUX)
target_link_libraries(tirex_tracker PUBLIC ${CMAKE_DL_LIBS})  # dlopen, dlclose, ...
elseif(WIN32)
target_link_libraries(tirex_tracker PUBLIC Powrprof)
endif()

include(GenerateExportHeader)
generate_export_header(tirex_tracker BASE_NAME tirex_tracker)
if (NOT TIREX_TRACKER_BUILD_SHARED_LIB)
	# set_target_properties(tirex_tracker_static PROPERTIES COMPILE_FLAGS -DTIREX_TRACKER_STATIC_DEFINE)
	target_compile_definitions(tirex_tracker PUBLIC TIREX_TRACKER_STATIC_DEFINE)
endif()

set_target_properties(tirex_tracker PROPERTIES
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
set_property(TARGET tirex_tracker PROPERTY POSITION_INDEPENDENT_CODE ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# CPPJoule
if (NOT (UNIX AND APPLE)) # CPPJouls does not support macos right now
	CPMAddPackage(URI "gh:TheMrSheldon/CPPJoules#pcm" EXCLUDE_FROM_ALL YES)
	target_link_libraries(tirex_tracker PRIVATE CPP_Joules)
endif()

# LibGit
CPMAddPackage(
	URI "gh:libgit2/libgit2@1.8.4"
	OPTIONS "USE_SSH OFF" "USE_HTTPS OFF" "REGEX_BACKEND builtin" "USE_BUNDLED_ZLIB ON" "BUILD_CLI OFF" "BUILD_TESTS OFF" "DEPRECATE_HARD OFF"
	EXCLUDE_FROM_ALL YES
)
#target_link_libraries(tirex_tracker PRIVATE libgit2package)
#target_include_directories(tirex_tracker PRIVATE $<TARGET_PROPERTY:libgit2,INCLUDE_DIRECTORIES>)
target_link_libraries(tirex_tracker PRIVATE libgit2 libgit2package)

# libzip
CPMAddPackage(
	URI "gh:nih-at/libzip@1.11.3"
	OPTIONS "ENABLE_OPENSSL OFF" "BUILD_TOOLS OFF" "BUILD_REGRESS OFF" "BUILD_OSSFUZZ OFF" "BUILD_EXAMPLES OFF" "BUILD_DOC OFF"
	EXCLUDE_FROM_ALL YES
)
target_link_libraries(tirex_tracker PRIVATE zip)


# NVML (We load the go-nvml bindings since there does not seem to be a repo with just the header)
FetchContent_Declare(nvml GIT_REPOSITORY https://github.com/NVIDIA/go-nvml.git GIT_TAG v0.12.4-0 EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(nvml)
target_include_directories(tirex_tracker PRIVATE ${nvml_SOURCE_DIR}/gen/)

# FMT (remove in the future when std::format can be assumed)
CPMAddPackage(URI "gh:fmtlib/fmt#11.1.3" EXCLUDE_FROM_ALL YES)
target_link_libraries(tirex_tracker PRIVATE fmt::fmt)

# expected (remove in the future when std::expected can be assumed)
set(EXPECTED_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(EXPECTED_BUILD_PACKAGE_DEB OFF CACHE BOOL "" FORCE)
FetchContent_Declare(expected GIT_REPOSITORY https://github.com/TartanLlama/expected.git GIT_TAG v1.1.0 EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(expected)
target_link_libraries(tirex_tracker PRIVATE tl::expected)
target_link_libraries(tirex_tracker_static PRIVATE tl::expected)


# cpuinfo
CPMAddPackage(
	URI "gh:pytorch/cpuinfo#5e3d2445e6a84d9599bee2bf78edbb4d80865e1d"
	OPTIONS "CPUINFO_BUILD_TOOLS OFF" "CPUINFO_BUILD_UNIT_TESTS OFF" "CPUINFO_BUILD_MOCK_TESTS OFF"
			"CPUINFO_BUILD_BENCHMARKS OFF" # "CPUINFO_LOG_TO_STDIO OFF"
	EXCLUDE_FROM_ALL YES
)
target_link_libraries(tirex_tracker PRIVATE cpuinfo::cpuinfo)

# Mac Only: Bind to CoreFoundation to get IOReport for system information
if (UNIX AND APPLE)
	target_link_libraries(tirex_tracker PRIVATE "$<LINK_LIBRARY:FRAMEWORK,CoreFoundation>")
	target_link_libraries(tirex_tracker PRIVATE "$<LINK_LIBRARY:FRAMEWORK,IOKit>")
endif()

# Hash
FetchContent_Declare(hash GIT_REPOSITORY https://github.com/Chocobo1/Hash.git GIT_TAG 250fd19b9b6f3fda714f5071ab477057615fb6d0 EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(hash)
target_include_directories(tirex_tracker PRIVATE ${hash_SOURCE_DIR}/src)
