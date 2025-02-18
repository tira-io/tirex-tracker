from tira_measure import measuring
with measuring() as results:
    from time import sleep
    sleep(1)
print(results)