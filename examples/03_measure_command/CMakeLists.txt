
add_executable(measure
    main.cpp
    formatters.cpp
    logging.cpp
)

target_compile_features(measure PUBLIC cxx_std_20)
target_link_libraries(measure measureapi_static)


##########################################################################################
# Libraries
##########################################################################################
include(FetchContent)
# spdlog for logging
FetchContent_Declare(spdlog GIT_REPOSITORY https://github.com/gabime/spdlog.git GIT_TAG v1.14.1)
FetchContent_MakeAvailable(spdlog)
set_property(TARGET spdlog PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(measure spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>)

# CLI11
FetchContent_Declare(cli11 GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git GIT_TAG v2.4.2)
FetchContent_MakeAvailable(cli11)
target_link_libraries(measure CLI11::CLI11)
