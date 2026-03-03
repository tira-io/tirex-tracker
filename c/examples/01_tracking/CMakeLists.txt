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

# nlohmann::json
# CPMAddPackage(URI "gh:nlohmann/json@3.12.0" OPTIONS "JSON_ImplicitConversions OFF" EXCLUDE_FROM_ALL YES)
target_link_libraries(01_tracking nlohmann_json::nlohmann_json)