add_executable(05_live_tracking
    main.cpp
)

target_link_libraries(05_live_tracking tirex::tracker nlohmann_json::nlohmann_json)
target_compile_features(05_live_tracking PRIVATE cxx_std_20)


##########################################################################################
# Libraries
##########################################################################################
# FTXUI for interactive terminal UI
CPMAddPackage(
    URI "gh:ArthurSonzogni/FTXUI@6.1.9"
    EXCLUDE_FROM_ALL YES
    OPTIONS "FTXUI_BUILD_EXAMPLES OFF" "FTXUI_BUILD_DOCS OFF" "FTXUI_ENABLE_INSTALL OFF"
)
target_link_libraries(05_live_tracking ftxui::component ftxui::dom ftxui::screen)
