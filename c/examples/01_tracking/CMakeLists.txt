cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

##############################
# Example 1: Simple tracking #
##############################
add_executable(01_tracking
    main.c
)
# Set the C++ standard to C++17.
target_compile_features(01_tracking PRIVATE cxx_std_17)
# Link required libraries.
target_link_libraries(01_tracking PRIVATE tirex_tracker)
