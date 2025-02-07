# The Measure Python API

Install via:

```
pip3 install 'git+https://github.com/tira-io/measure.git@python-language-structure#egg=py_measure&subdirectory=python'
```

Usage:

```py
from py_measure import Environment

environment = Environment(["git", "system"], ..., polling_ms=1000)  # defaults would be all measures, and some reasonable polling interval.

# Then measure with a context manager...
with environment.measure():
  # do something

# ...or a function decorator...
@environment.measure
def do_something():
  # do something
do_something()

# ...or manually...
environment.start_measuring()
# do something
environment.stop_measuring()

print("start git hash", environment.measurements[0]["git"])
```



Run tests via:
```
pytest
```
