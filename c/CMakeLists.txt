cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(TIREX_TRACKER_BUILD_DEB "Build debian package" OFF)
option(TIREX_TRACKER_ONLY_DOCS "Build only documentation -- this disables tests and others" OFF)
option(TIREX_TRACKER_BUILD_EXAMPLES "Build the examples" OFF)
option(TIREX_TRACKER_BUILD_DOCS "Build the documentation" OFF)

project(tirex_tracker VERSION 0.0.1 LANGUAGES C CXX)

if (NOT TIREX_TRACKER_ONLY_DOCS)
    add_subdirectory(src)
    add_subdirectory(extensions)

	if (TIREX_TRACKER_BUILD_EXAMPLES)
		add_subdirectory(examples)
	endif()
endif()


##########################################################################################
# Documentation
##########################################################################################
# Documentation only available if this is the main app
if((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME) AND TIREX_TRACKER_BUILD_DOCS)
	find_package(Doxygen REQUIRED)
	include(FetchContent)
	## Set Doxygen configuration options (THIS REPLACES THE DOXYFILE!)
	set(DOXYGEN_PROJECT_BRIEF "Measuring what really matters")
	set(DOXYGEN_PROJECT_LOGO "")
	set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/documentation)
	set(DOXYGEN_STRIP_FROM_PATH "include")
	set(DOXYGEN_EXTRACT_PRIVATE "NO")
	set(DOXYGEN_EXTRACT_STATIC "NO")
	set(DOXYGEN_USE_MATHJAX "NO")
	set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
	set(DOXYGEN_SOURCE_BROWSER "NO")
	set(DOXYGEN_INLINE_SOURCES "NO")
	set(DOXYGEN_TYPEDEF_HIDES_STRUCT "YES")
	set(DOXYGEN_SHOW_INCLUDE_FILES "NO")
	set(DOXYGEN_SHOW_FILES "NO")
	set(DOXYGEN_SEPARATE_MEMBER_PAGES "YES")
	set(DOXYGEN_ENABLE_PREPROCESSING "YES")
	set(DOXYGEN_MACRO_EXPANSION "YES")
	set(DOXYGEN_EXPAND_ONLY_PREDEF "YES")
	set(DOXYGEN_PREDEFINED "TIREX_EXPORT")

	set(DOXYGEN_GENERATE_HTML "YES")

    ## Install and configure the "Doxygen Awesome" theme
	FetchContent_Declare(doxawesome GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css GIT_TAG v2.3.4)
	FetchContent_MakeAvailable(doxawesome)
	set(DOXYGEN_GENERATE_TREEVIEW "NO")
	set(DOXYGEN_DISABLE_INDEX "NO")
	set(DOXYGEN_FULL_SIDEBAR "NO")
	set(DOXYGEN_HTML_EXTRA_STYLESHEET ${doxawesome_SOURCE_DIR}/doxygen-awesome.css ${doxawesome_SOURCE_DIR}/doxygen-awesome-darkmode-toggle.css ${doxawesome_SOURCE_DIR}/doxygen-awesome-tabs.js ${doxawesome_SOURCE_DIR}/doxygen-custom/custom.css)
	set(DOXYGEN_HTML_COLORSTYLE "LIGHT")
	set(DOXYGEN_HTML_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/doxygen-header.html)
	set(DOXYGEN_HTML_EXTRA_FILES ${doxawesome_SOURCE_DIR}/doxygen-awesome-darkmode-toggle.js ${doxawesome_SOURCE_DIR}/doxygen-awesome-fragment-copy-button.js ${doxawesome_SOURCE_DIR}/doxygen-awesome-interactive-toc.js ${doxawesome_SOURCE_DIR}/doxygen-awesome-paragraph-link.js)

	## Build Documentation
	doxygen_add_docs(
		tirex_tracker_docs
		${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/README.md ${CMAKE_CURRENT_SOURCE_DIR}/docs
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	)
endif()


##########################################################################################
# Debian Package
##########################################################################################
# Debian Packaging only available if this is the main app
if(NOT TIREX_TRACKER_ONLY_DOCS AND (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME) AND TIREX_TRACKER_BUILD_DEB)
	install(TARGETS tirex_tracker ARCHIVE DESTINATION lib COMPONENT tirex_tracker)
	install(DIRECTORY include DESTINATION include COMPONENT tirex_tracker)

	SET(CPACK_GENERATOR "DEB")
	SET(CPACK_PACKAGE_NAME "tirex-tracker")
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
	cpack_add_component(tirex_tracker)
endif()