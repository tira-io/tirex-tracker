cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

#################################
# Example 3: ir_metadata export #
#################################
add_executable(03_ir_metadata_export
    main.c
)
# Set the C++ standard to C++20.
target_compile_features(03_ir_metadata_export PRIVATE cxx_std_20)
# Link required libraries.
target_link_libraries(03_ir_metadata_export PRIVATE tirex_tracker_full)
