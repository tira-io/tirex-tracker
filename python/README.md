# TIREx Tracker

[![CI](https://img.shields.io/github/actions/workflow/status/tira-io/tirex-tracker/ci.yml?branch=master&style=flat-square)](https://github.com/tira-io/tirex-tracker/actions/workflows/ci.yml)
[![Maintenance](https://img.shields.io/maintenance/yes/2025?style=flat-square)](https://github.com/tira-io/tirex-tracker/graphs/contributors) <!-- [![Code coverage](https://img.shields.io/codecov/c/github/tira-io/tirex-tracker?style=flat-square)](https://codecov.io/github/tira-io/tirex-tracker/) --> \
[![PyPi](https://img.shields.io/pypi/v/tirex-tracker?style=flat-square)](https://pypi.org/project/tirex-tracker/) [![Python](https://img.shields.io/pypi/pyversions/tirex-tracker?style=flat-square)](https://pypi.org/project/tirex-tracker/) [![Downloads](https://img.shields.io/pypi/dm/tirex-tracker?style=flat-square)](https://pypi.org/project/tirex-tracker/) \
[![Issues](https://img.shields.io/github/issues/tira-io/tirex-tracker?style=flat-square)](https://github.com/tira-io/tirex-tracker/issues) [![Commit activity](https://img.shields.io/github/commit-activity/m/tira-io/tirex-tracker?style=flat-square)](https://github.com/tira-io/tirex-tracker/commits) [![License](https://img.shields.io/github/license/tira-io/tirex-tracker?style=flat-square)](LICENSE)

The TIREx tracker is a command line tool and API to automatically track resource usage, hardware specifications, and other metadata when running information retrieval experiments.
It can be used easily in [Python applications](#python-api). For more information, refer to the [project's readme](https://github.com/tira-io/tirex-tracker?tab=readme-ov-file#readme).

## Python API

First, install the TIREx tracker Python package from [PyPI](https://pypi.org/project/tirex-tracker/):

```shell
pip install tirex-tracker
```

Now, you can track the hardware metrics and metadata of your Python code by using the [context manager](https://geeksforgeeks.org/context-manager-in-python/):

```python
from tirex_tracker import tracking

with tracking() as results:
    # Do something...

print(results)
```

Alternatively, you can track the hardware metrics and metadata of a Python function by using the [function decorator](https://geeksforgeeks.org/decorators-in-python/):

```python
from tirex_tracker import tracked

@tracked
def do_something():
    # Do something...

do_something()

print(do_something.results)
```

If you cannot use either the context manager or the function decorator from above, you can manually start and stop the tracking:

```python
from tirex_tracker import start_tracking, stop_tracking

handle = start_tracking()
try:
    # Do something...
finally:
    results = stop_tracking(handle)

print(results)
```

<!-- TODO: Explain parameters. -->

<!-- TODO: ir_metadata export instructions. -->

## Citation

The TIREx tracker's accompanying paper is under review. We will add citation instructions once it is published.

<!-- TODO: Add citation (written out and as BibTeX). -->

## License

The TIREx tracker code is licensed under the [MIT License](./LICENSE).
If you use the TIREx tracker in your experiments, we would appreciate you [citing](#citation) our paper.

## Abstract

> The reproducibility and transparency of retrieval experiments heavily depends on properly provided information on the experimental setup and conditions. But as manually curating such experiment metadata can be tedious, error-prone, and inconsistent, metadata should be systematically collected in an automatic way—similar to the collection of Python and git-specific settings in the `ir_metadata` reference implementation. To enable a platform-independent automatic metadata collection following the `ir_metadata` specification, we introduce the TIREx tracker: a tool realized via a lightweight C binary, pre-compiled with all dependencies for all major platforms to track hardware configurations, usage of power/CPUs/RAM/GPUs, and experiment/system versions. The TIREx tracker seamlessly integrates into Python, Java, or C/C++ workflows and can be easily incorporated in run submissions of shared tasks, which we showcase for the TIRA/TIREx platform. Code, binaries, and documentation are publicly available at <https://github.com/tira-io/tirex-tracker>.
