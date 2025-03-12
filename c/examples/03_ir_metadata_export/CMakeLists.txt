add_executable(03_ir_metadata_export
    main.c
)

target_link_libraries(03_ir_metadata_export tirex_tracker tirex_tracker_extension_ir_metadata)