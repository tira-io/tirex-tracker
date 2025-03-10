from tirex_tracker import tracking, ExportFormat

with tracking(export_format = ExportFormat.IR_METADATA, export_file_path="ir_metadata.yml") as results:
    from time import sleep
    sleep(1)
