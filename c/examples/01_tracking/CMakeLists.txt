add_executable(01_tracking
    main.c
    plotting.cpp
)

target_link_libraries(01_tracking tirex::tracker)


##########################################################################################
# Libraries
##########################################################################################
# asciichart for simple plotting
CPMAddPackage(URI "gh:Civitasv/asciichart#7464831085adcc51f7389fd6a825cc648a99e82f" EXCLUDE_FROM_ALL YES)
target_include_directories(01_tracking PRIVATE ${asciichart_SOURCE_DIR}/include)

# cJSON for lightweight parsing of the results
CPMAddPackage(URI "gh:DaveGamble/cJSON@1.7.19" EXCLUDE_FROM_ALL YES PATCHES "${CMAKE_CURRENT_LIST_DIR}/cjson.patch")
target_link_libraries(01_tracking cjson)
target_include_directories(01_tracking PRIVATE ${cJSON_SOURCE_DIR}) # cJSON does not seem to set this properly so we have to