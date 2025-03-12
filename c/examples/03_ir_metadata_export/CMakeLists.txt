add_executable(04_ir_extension
    main.c
)

target_link_libraries(04_ir_extension tirex_tracker tirex_tracker_extension_irmetadata)