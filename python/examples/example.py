from tirex_tracker import tracking
with tracking() as results:
    from time import sleep
    sleep(1)
print(results)