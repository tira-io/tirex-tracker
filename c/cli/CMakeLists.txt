cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

# Include helper module.
include(FetchContent)


##########################
# Command line interface #
##########################
add_executable(tirex_tracker_cli
    formatters.cpp
    logging.cpp
    main.cpp
)
set_target_properties(tirex_tracker_cli PROPERTIES OUTPUT_NAME "tirex-tracker-${TIREX_TRACKER_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
# Set the C++ standard to C++17.
target_compile_features(tirex_tracker_cli PRIVATE cxx_std_17)
# Link required libraries.
target_link_libraries(tirex_tracker_cli PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,tirex_tracker_full_static>")


#########################
# Third-party libraries #
#########################

# Install spdlog library from GitHub.
set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_NO_THREAD_ID ON CACHE BOOL "" FORCE)
set(SPDLOG_NO_TLS ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS_OLD "${BUILD_SHARED_LIBS}")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(spdlog GIT_REPOSITORY https://github.com/gabime/spdlog.git GIT_TAG v1.14.1)
FetchContent_MakeAvailable(spdlog)
set(BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS_OLD}" CACHE BOOL "" FORCE)
set_property(TARGET spdlog PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(tirex_tracker_cli PRIVATE spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>)

# Instal CLI11 library from GitHub.
set(CLI11_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(CLI11_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CLI11_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CLI11_BUILD_EXAMPLES_JSON OFF CACHE BOOL "" FORCE)
set(CLI11_SINGLE_FILE_TESTS OFF CACHE BOOL "" FORCE)
set(CLI11_INSTALL OFF CACHE BOOL "" FORCE)
set(CLI11_CUDA_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(cli11 GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git GIT_TAG v2.4.2)
FetchContent_MakeAvailable(cli11)
target_link_libraries(tirex_tracker_cli PRIVATE CLI11::CLI11)


##################
# Debian Package #
##################
# The Debian Packaging build target is only available if this is the root project
if(NOT TIREX_TRACKER_ONLY_DOCS AND (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME) AND TIREX_TRACKER_BUILD_DEB)
	# Include helper module.
	include(CPack)
	include(GNUInstallDirs)  # FIXME: Is this needed?
	
	# Install the CLI binary.
	install(TARGETS tirex_tracker_cli RUNTIME DESTINATION bin COMPONENT tirex_tracker_cli)

	# Configure the Debian package.
	set(CPACK_GENERATOR "DEB" CACHE STRING "" FORCE)
	set(CPACK_PACKAGE_NAME "tirex-tracker" CACHE STRING "" FORCE)
	set(CPACK_SET_DESTDIR TRUE CACHE PATH "" FORCE)
	set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Tim Hagen" CACHE STRING "" FORCE)
	set(CPACK_PACKAGE_VERSION "${TIREX_TRACKER_VERSION}" CACHE STRING "" FORCE)
	set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Automatic resource and metadata tracking for information retrieval experiments." CACHE STRING "" FORCE)
	set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/../../README.md" CACHE PATH "" FORCE)
	set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../../LICENSE" CACHE PATH "" FORCE)
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "" CACHE STRING "" FORCE)
	set(CPACK_PACKAGE_VENDOR "" CACHE STRING "" FORCE)

	# Add the CLI component to the Debian package.
	cpack_add_component(tirex_tracker_cli)
endif()