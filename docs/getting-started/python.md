# Python

## Installation

```shell
pip install tirex-tracker
```

The package ships with a precompiled native library for Linux (x86\_64, ARM64), macOS, and Windows — no build step required.

**Supported Python versions:** 3.8 – 3.13

## Usage patterns

TIREx Tracker offers three usage patterns. Choose the one that fits your workflow.

### Context manager (recommended)

Wrap the code you want to measure in a `with tracking()` block:

```python
from tirex_tracker import tracking

with tracking() as results:
    # Your experiment code here
    train_model()

print(results)
```

The results are available immediately after the `with` block exits and can be accessed like a dictionary:

```python
from tirex_tracker import Measure, tracking

with tracking() as results:
    train_model()

wall_ms = results[Measure.TIME_ELAPSED_WALL_CLOCK_MS]
cpu_max = results[Measure.CPU_USED_PROCESS_PERCENT]
print(f"Elapsed: {wall_ms.value} ms, peak CPU: {cpu_max.value}%")
```

### Function decorator

Annotate functions whose every call should be tracked:

```python
from tirex_tracker import tracked

@tracked
def train_model():
    # Your experiment code here
    ...

train_model()
print(train_model.results)
```

After each call the results are stored in `function.results`.

### Manual start / stop

When you need full control over the tracking lifetime:

```python
from tirex_tracker import start_tracking, stop_tracking

handle = start_tracking()
try:
    train_model()
finally:
    results = stop_tracking(handle)

print(results)
```

## Selecting measures

By default all available measures are tracked. Pass a set of `Measure` values to restrict tracking:

```python
from tirex_tracker import Measure, tracking

measures = {
    Measure.TIME_ELAPSED_WALL_CLOCK_MS,
    Measure.CPU_USED_PROCESS_PERCENT,
    Measure.RAM_USED_PROCESS_KB,
}
with tracking(measures=measures) as results:
    train_model()
```

See [Tracked Measures](../guides/measures.md) for the complete list.

## Poll interval

Time-series measures (CPU, RAM, GPU) are sampled at a configurable interval (default: 100 ms):

```python
with tracking(poll_interval_ms=500) as results:
    train_model()
```

## Exporting results

### IR Metadata format

Export results directly to an [`ir_metadata`](../guides/ir-metadata.md) YAML file:

```python
from tirex_tracker import ExportFormat, tracking

with tracking(export_format=ExportFormat.IR_METADATA,
              export_file_path="ir_metadata.yml") as results:
    train_model()
```

### Custom metadata

Attach arbitrary key-value metadata before or during tracking:

```python
from tirex_tracker import register_metadata, tracking

register_metadata("dataset", "msmarco-v2")
register_metadata("model", "bm25")

with tracking(export_format=ExportFormat.IR_METADATA,
              export_file_path="ir_metadata.yml") as results:
    train_model()
```

### Register files

Include additional files (e.g., a config or results file) in the exported archive:

```python
from tirex_tracker import register_file, tracking

register_file("config.yaml")

with tracking(export_format=ExportFormat.IR_METADATA,
              export_file_path="ir_metadata.yml") as results:
    train_model()
```

## Logging

Configure a log callback to see internal messages from the tracker:

```python
from tirex_tracker import LogLevel, set_log_callback

def my_logger(level: LogLevel, component: str, message: str) -> None:
    print(f"[{level.name}][{component}] {message}")

set_log_callback(my_logger)
```

## Python-specific measures

In addition to the platform measures tracked by the C core, the Python wrapper collects:

| Measure | Description |
|---|---|
| `PYTHON_VERSION` | Python interpreter version |
| `PYTHON_MODULES` | Modules visible in the current environment |
| `PYTHON_INSTALLED_PACKAGES` | Installed pip packages and their versions |
| `PYTHON_IS_INTERACTIVE` | Whether the interpreter is running interactively |
| `PYTHON_NOTEBOOK_FILE_PATH_IN_CODE_ARCHIVE` | Notebook path inside the code archive (Jupyter) |

## API reference

See the [Python API reference](../api/python.md) for complete documentation of all functions, classes, and parameters.
