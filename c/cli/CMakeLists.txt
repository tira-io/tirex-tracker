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
# Set the C++ standard to C++20.
target_compile_features(tirex_tracker_cli PRIVATE cxx_std_20)
# Link required libraries.
target_link_libraries(tirex_tracker_cli PRIVATE tirex_tracker_full_static)


#########################
# Third-party libraries #
#########################

# Install spdlog library from GitHub.
set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
FetchContent_Declare(spdlog GIT_REPOSITORY https://github.com/gabime/spdlog.git GIT_TAG v1.14.1)
FetchContent_MakeAvailable(spdlog)
set_property(TARGET spdlog PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(tirex_tracker_cli PRIVATE spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>)

# Instal CLI11 library from GitHub.
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
	
	# Install the CLI binary.
	install(TARGETS tirex_tracker_cli RUNTIME DESTINATION bin COMPONENT tirex_tracker_cli)

	# Configure the Debian package.
	SET(CPACK_GENERATOR "DEB")
	SET(CPACK_PACKAGE_NAME "tirex-tracker")
	SET(CPACK_SET_DESTDIR TRUE)
	SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Tim Hagen")
	SET(CPACK_PACKAGE_VERSION "${TIREX_TRACKER_VERSION}")
	include(GNUInstallDirs)
	SET(CPACK_DEBIAN_PACKAGE_DESCRIPTION "TODO")
	SET(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/../../../README.md")
	SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../../../LICENSE")
	SET(CPACK_DEBIAN_PACKAGE_DEPENDS "")
	SET(CPACK_PACKAGE_VENDOR "")

	# Add the CLI component to the Debian package.
	cpack_add_component(tirex_tracker_cli)
endif()