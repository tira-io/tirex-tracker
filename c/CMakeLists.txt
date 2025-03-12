cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

option(TIREX_TRACKER_ONLY_DOCS "Build only the documentation and disable tests and others." OFF)
option(TIREX_TRACKER_BUILD_CLI "Build the command line interface." OFF)
option(TIREX_TRACKER_BUILD_DEB "Build the Debian package." OFF)
option(TIREX_TRACKER_BUILD_EXAMPLES "Build the examples." OFF)
option(TIREX_TRACKER_BUILD_DOCS "Build the documentation." OFF)

# Infer version from Git tag.
find_program(GIT_EXECUTABLE git)
if(GIT_EXECUTABLE)
	# Generate a git-describe version string from Git repository tags
	execute_process(
		COMMAND ${GIT_EXECUTABLE} describe --tags --dirty --match "*.*.*"
		WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
		OUTPUT_VARIABLE GIT_DESCRIBE_VERSION
		RESULT_VARIABLE GIT_DESCRIBE_ERROR_CODE
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	if(NOT GIT_DESCRIBE_ERROR_CODE)
		set(TIREX_TRACKER_VERSION ${GIT_DESCRIBE_VERSION})
	endif()
endif()
if(NOT DEFINED TIREX_TRACKER_VERSION)
	set(TIREX_TRACKER_VERSION 0.0.0-unknown)
	message(WARNING "Failed to determine TIREX_TRACKER_VERSION from Git tag.")
endif()
string(REGEX MATCH "^[0-9]+\.[0-9]+\.[0-9]+" TIREX_TRACKER_VERSION_PLAIN ${TIREX_TRACKER_VERSION})
message(DEBUG "The TIREx tracker version is ${TIREX_TRACKER_VERSION} (${TIREX_TRACKER_VERSION_PLAIN})")

project(tirex_tracker VERSION ${TIREX_TRACKER_VERSION_PLAIN} LANGUAGES C CXX)

if (NOT TIREX_TRACKER_ONLY_DOCS)
    add_subdirectory(src)
    add_subdirectory(extensions)

	if (TIREX_TRACKER_BUILD_CLI)
		add_subdirectory(cli)
	endif()

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
