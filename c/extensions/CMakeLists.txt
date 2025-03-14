cmake_minimum_required(VERSION 3.24 FATAL_ERROR)


######################################
# Shared library with all extensions #
######################################
add_library(tirex_tracker_full SHARED
    irtracker.cpp
)
# Set the C++ standard to C++17.
target_compile_features(tirex_tracker_full PRIVATE cxx_std_17)
# Include the header files.
target_include_directories(tirex_tracker_full PUBLIC ${CMAKE_CURRENT_LIST_DIR})
# Link required libraries.
target_link_libraries(tirex_tracker_full PUBLIC "$<LINK_LIBRARY:WHOLE_ARCHIVE,tirex_tracker_static>")
# Configure the library build.
target_compile_definitions(tirex_tracker_full PUBLIC TIREX_TRACKER_LIB_EXPORT)
target_compile_definitions(tirex_tracker_full PUBLIC TIREX_TRACKER_VERSION=${TIREX_TRACKER_VERSION})


######################################
# Static library with all extensions #
######################################
add_library(tirex_tracker_full_static STATIC
    irtracker.cpp
)
# Set the C++ standard to C++17.
target_compile_features(tirex_tracker_full_static PRIVATE cxx_std_17)
# Include the header files.
target_include_directories(tirex_tracker_full_static PUBLIC ${CMAKE_CURRENT_LIST_DIR})
# Link required libraries.
target_link_libraries(tirex_tracker_full_static PUBLIC "$<LINK_LIBRARY:WHOLE_ARCHIVE,tirex_tracker_static>")
# Configure the library build.
target_compile_definitions(tirex_tracker_full_static PUBLIC TIREX_TRACKER_VERSION=${TIREX_TRACKER_VERSION})
