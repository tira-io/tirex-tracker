from tirex_tracker import ExportFormat, set_log_callback, tracking

set_log_callback(lambda level, component, msg: print(f"[{level}][{component}] {msg}"))

with tracking(export_format=ExportFormat.IR_METADATA, export_file_path="ir_metadata.yml") as results:
    from time import sleep

    sleep(1)
