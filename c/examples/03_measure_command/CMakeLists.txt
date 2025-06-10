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
# Load CPM
file(
	DOWNLOAD https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.42.0/CPM.cmake
	${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

# spdlog for logging
CPMAddPackage(
	URI "gh:gabime/spdlog@1.15.3"
	OPTIONS "SPDLOG_FMT_EXTERNAL ON"
	EXCLUDE_FROM_ALL YES
)
set_property(TARGET spdlog PROPERTY POSITION_INDEPENDENT_CODE ON)  # Is this needed?
target_link_libraries(measure spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>)

# CLI11
CPMAddPackage(URI "gh:CLIUtils/CLI11@2.4.2" EXCLUDE_FROM_ALL YES)
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
