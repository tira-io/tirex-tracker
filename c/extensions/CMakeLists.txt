add_library(measureext_ir
    irtracker.cpp
)
target_include_directories(measureext_ir PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(measureext_ir PUBLIC tirex_tracker)


add_library(measure_full SHARED
    irtracker.cpp
)
target_include_directories(measure_full PUBLIC ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/../include)
# For now see if the LINK_LIBRARY generator works with old systems (requires 3.24). If it is too new, uncomment the
# following line:
# target_link_libraries(measure_full PRIVATE "-Wl,--whole-archive" tirex_tracker_static "-Wl,--no-whole-archive")
target_link_libraries(measure_full PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,tirex_tracker_static>")
target_compile_definitions(measure_full PRIVATE TIREX_TRACKER_LIB_EXPORT)
