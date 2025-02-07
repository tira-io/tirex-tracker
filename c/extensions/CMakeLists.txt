add_library(measureext_ir
    irmeasure.cpp
)
target_include_directories(measureext_ir PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(measureext_ir PUBLIC measureapi)