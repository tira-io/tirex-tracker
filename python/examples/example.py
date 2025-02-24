from tirex_tracker import tracking, set_log_callback, LogLevel

def log_callback(level: LogLevel, component: str, message: str) -> None:
    print(f'[{level}] [{component}] {message}')

set_log_callback(log_callback)

with tracking() as results:
    from time import sleep
    sleep(1)
print(results)