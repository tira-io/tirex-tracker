cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

######################################
# Example 2: List available measures #
######################################
add_executable(02_list_measures
    main.c
)
# Set the C++ standard to C++20.
target_compile_features(02_list_measures PRIVATE cxx_std_20)
# Link required libraries.
target_link_libraries(02_list_measures PRIVATE tirex_tracker_static)
