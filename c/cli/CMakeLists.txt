
add_executable(tirex_tracker_cli
    main.cpp
    formatters.cpp
    logging.cpp
)

target_compile_features(tirex_tracker_cli PUBLIC cxx_std_20)
target_link_libraries(tirex_tracker_cli tirex_tracker_full_static)

option(TIREX_TRACKER_BUILD_CMD_DEB "Build the Debian package for the TIREx tracker CLI." OFF)


##########################################################################################
# Libraries
##########################################################################################
include(FetchContent)
# spdlog for logging
FetchContent_Declare(spdlog GIT_REPOSITORY https://github.com/gabime/spdlog.git GIT_TAG v1.14.1)
FetchContent_MakeAvailable(spdlog)
set_property(TARGET spdlog PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(tirex_tracker_cli spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>)

# CLI11
FetchContent_Declare(cli11 GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git GIT_TAG v2.4.2)
FetchContent_MakeAvailable(cli11)
target_link_libraries(tirex_tracker_cli CLI11::CLI11)


##########################################################################################
# Debian Package
##########################################################################################
# Debian Packaging only available if this is the main app
if(NOT TIREX_TRACKER_ONLY_DOCS AND (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME) AND TIREX_TRACKER_BUILD_CMD_DEB)
	install(TARGETS tirex_tracker_cli RUNTIME DESTINATION bin COMPONENT tirex_tracker_cli_component)

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
	include(CPack)
	cpack_add_component(tirex_tracker_cli_component)
endif()