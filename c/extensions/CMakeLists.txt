set(TIREX_EXT_INCLUDE_DIRS "" PARENT_SCOPE)
set(TIREX_EXT_INCLUDE_FILES "" PARENT_SCOPE)
set(TIREX_EXT_SOURCES "" PARENT_SCOPE)

if (TIREX_TRACKER_EXTENSION_IR)
    list(APPEND TIREX_EXT_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR})
    list(APPEND TIREX_EXT_INCLUDE_FILES ${CMAKE_CURRENT_LIST_DIR}/irtracker.h)
    list(APPEND TIREX_EXT_SOURCES ${CMAKE_CURRENT_LIST_DIR}/irtracker.cpp)
    set(TIREX_EXT_INCLUDE_DIRS "${TIREX_EXT_INCLUDE_DIRS}" PARENT_SCOPE)
    set(TIREX_EXT_INCLUDE_FILES "${TIREX_EXT_INCLUDE_FILES}" PARENT_SCOPE)
    set(TIREX_EXT_SOURCES "${TIREX_EXT_SOURCES}" PARENT_SCOPE)
endif()