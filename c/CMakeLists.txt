cmake_minimum_required(VERSION 3.24 FATAL_ERROR)


include(FeatureSummary)
option(TIREX_TRACKER_BUILD_DEB "Build debian package" OFF)
option(TIREX_TRACKER_ONLY_DOCS "Build only documentation -- this disables tests and others" OFF)
option(TIREX_TRACKER_BUILD_EXAMPLES "Build the examples" OFF)
option(TIREX_TRACKER_BUILD_DOCS "Build the documentation" OFF)
option(TIREX_TRACKER_BUILD_TESTS "Build the tests" OFF)
option(TIREX_TRACKER_BUILD_SHARED_LIB "(possible values: ON/OFF/inherit)" "inherit")
option(TIREX_TRACKER_EXTENSION_IR "Build the IR extension for TIREx Tracker" ON)

if (TIREX_TRACKER_BUILD_SHARED_LIB EQUAL "inherit")
	set(TIREX_TRACKER_BUILD_SHARED_LIB BUILD_SHARED_LIBS)
endif()

add_feature_info(package "TIREX_TRACKER_BUILD_DEB AND NOT TIREX_TRACKER_ONLY_DOCS" "Build debian package")
add_feature_info(examples "TIREX_TRACKER_BUILD_EXAMPLES AND NOT TIREX_TRACKER_ONLY_DOCS" "Build TIREx tracker examples")
add_feature_info(documentation TIREX_TRACKER_BUILD_DOCS "Build TIREx tracker documentation")
add_feature_info(tests "TIREX_TRACKER_BUILD_TESTS AND NOT TIREX_TRACKER_ONLY_DOCS" "Build TIREx tracker tests")
add_feature_info(extension_ir TIREX_TRACKER_EXTENSION_IR "Build the IR extension for TIREx Tracker")
feature_summary(WHAT ALL)


set(TIREX_TRACKER_VER_MAJOR "0")
set(TIREX_TRACKER_VER_MINOR "2")
set(TIREX_TRACKER_VER_PATCH "15")
set(TIREX_TRACKER_VERSION "${TIREX_TRACKER_VER_MAJOR}.${TIREX_TRACKER_VER_MINOR}.${TIREX_TRACKER_VER_PATCH}")


project(tirex_tracker VERSION ${TIREX_TRACKER_VERSION} LANGUAGES C CXX)

if (NOT TIREX_TRACKER_ONLY_DOCS)
	# Load CPM
	set(CPM_DOWNLOAD_VERSION 0.42.0)
	set(CPM_HASH_SUM "2020b4fc42dba44817983e06342e682ecfc3d2f484a581f11cc5731fbe4dce8a")

	if(CPM_SOURCE_CACHE)
	set(CPM_DOWNLOAD_LOCATION "${CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
	elseif(DEFINED ENV{CPM_SOURCE_CACHE})
	set(CPM_DOWNLOAD_LOCATION "$ENV{CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
	else()
	set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
	endif()

	# Expand relative path. This is important if the provided path contains a tilde (~)
	get_filename_component(CPM_DOWNLOAD_LOCATION ${CPM_DOWNLOAD_LOCATION} ABSOLUTE)

	file(DOWNLOAD
		https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
		${CPM_DOWNLOAD_LOCATION} EXPECTED_HASH SHA256=${CPM_HASH_SUM}
		TLS_VERIFY OFF # Sometimes broken in the GitHub Action; we have the checksum to verify that there is nothing malicious going on; try at some point to remove this line
	)

	include(${CPM_DOWNLOAD_LOCATION})
	message("Loaded CPM v${CURRENT_CPM_VERSION}")

	# Populates the TIREX_EXT_INCLUDE_DIRS, TIREX_EXT_INCLUDE_FILES, and TIREX_EXT_SOURCES variables
    add_subdirectory(extensions)
	# Add Source code
    add_subdirectory(src)

	if (TIREX_TRACKER_BUILD_EXAMPLES)
		add_subdirectory(examples)
	endif()
endif()

##########################################################################################
# Tests
##########################################################################################
if (TIREX_TRACKER_BUILD_TESTS AND NOT TIREX_TRACKER_ONLY_DOCS)
	add_subdirectory(tests)
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
	FetchContent_Declare(doxawesome GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css GIT_TAG v2.3.4 EXCLUDE_FROM_ALL)
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
	include(CMakePackageConfigHelpers)
	write_basic_package_version_file(
		tirex_trackerConfigVersion.cmake
		VERSION ${TIREX_TRACKER_VERSION}
		COMPATIBILITY SameMajorVersion
	)

	include(GNUInstallDirs)

	install(TARGETS tirex_tracker
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
			COMPONENT tirex_tracker
	)
	install(DIRECTORY include DESTINATION include COMPONENT tirex_tracker)

	SET(CPACK_GENERATOR "DEB")
	SET(CPACK_PACKAGE_NAME "tirex-tracker")
	SET(CPACK_SET_DESTDIR TRUE)
	SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "not-set")
	SET(CPACK_PACKAGE_VERSION ${TIREX_TRACKER_VERSION})
	SET(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Automatic resource and metadata tracking")
	SET(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/../README.md")
	SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE")
	SET(CPACK_DEBIAN_PACKAGE_DEPENDS "")
	SET(CPACK_PACKAGE_VENDOR "")
	include(CPack)
	cpack_add_component(tirex_tracker)
endif()