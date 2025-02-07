cmake_minimum_required(VERSION 3.14 FATAL_ERROR)


option(MEASURECMD_BUILD_DEB "Build debian package" ON)
option(MEASURECMD_ONLY_DOCS "Build only documentation -- this disables tests and others" OFF)
option(MEASURECMD_BUILD_EXAMPLES "Build the examples" OFF)

project(measurecmd VERSION 0.0.1 LANGUAGES C CXX)

if (NOT MEASURECMD_ONLY_DOCS)
    add_subdirectory(src)
    add_subdirectory(extensions)

	if (MEASURECMD_BUILD_EXAMPLES)
		add_subdirectory(examples)
	endif()
endif()

##########################################################################################
# Debian Package
##########################################################################################
# Debian Packaging only available if this is the main app
if(NOT MEASURECMD_ONLY_DOCS AND (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME) AND MEASURECMD_BUILD_DEB)
	if (NOT MEASURECMD_BUILD_EXAMPLES)
		add_subdirectory(examples/03_measure_command)
	endif()
	
	install(TARGETS measure RUNTIME DESTINATION bin COMPONENT command)

	SET(CPACK_GENERATOR "DEB")
	SET(CPACK_PACKAGE_NAME "measure")
	SET(CPACK_SET_DESTDIR TRUE)
	SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "not-set")
	SET(CPACK_PACKAGE_VERSION "0.0.1rc")
	include(GNUInstallDirs)
	SET(CPACK_DEBIAN_PACKAGE_DESCRIPTION "TODO")
	SET(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/../README.md")
	SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE")
	SET(CPACK_DEBIAN_PACKAGE_DEPENDS "")
	SET(CPACK_PACKAGE_VENDOR "")
	include(CPack)
	cpack_add_component(command)
endif()