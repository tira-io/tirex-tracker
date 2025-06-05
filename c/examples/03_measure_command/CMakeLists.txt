
add_executable(measure
    main.cpp
    formatters.cpp
    logging.cpp
)

target_compile_features(measure PUBLIC cxx_std_20)
target_link_libraries(measure tirex_tracker_full_static)

option(TIREX_TRACKER_BUILD_CMD_DEB "Build the debian package for the measure command" OFF)


##########################################################################################
# Libraries
##########################################################################################
include(FetchContent)
# spdlog for logging
FetchContent_Declare(spdlog GIT_REPOSITORY https://github.com/gabime/spdlog.git GIT_TAG v1.15.3 EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(spdlog)
set_property(TARGET spdlog PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(measure spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>)

# CLI11
FetchContent_Declare(cli11 GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git GIT_TAG v2.4.2 EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(cli11)
target_link_libraries(measure CLI11::CLI11)


##########################################################################################
# Debian Package
##########################################################################################
# Debian Packaging only available if this is the main app
if(NOT TIREX_TRACKER_ONLY_DOCS AND (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME) AND TIREX_TRACKER_BUILD_CMD_DEB)
	install(TARGETS measure RUNTIME DESTINATION bin COMPONENT measurecmd)

	SET(CPACK_GENERATOR "DEB")
	SET(CPACK_PACKAGE_NAME "tirex-tracker")
	SET(CPACK_SET_DESTDIR TRUE)
	SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "not-set")
	SET(CPACK_PACKAGE_VERSION "0.0.1rc")
	include(GNUInstallDirs)
	SET(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Automatic resource and metadata tracking")
	SET(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/../../../README.md")
	SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../../../LICENSE")
	SET(CPACK_DEBIAN_PACKAGE_DEPENDS "")
	SET(CPACK_PACKAGE_VENDOR "")
	include(CPack)
	cpack_add_component(measurecmd)
endif()