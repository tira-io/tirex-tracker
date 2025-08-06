add_executable(01_tracking
    main.c
)

target_link_libraries(01_tracking tirex::tracker)