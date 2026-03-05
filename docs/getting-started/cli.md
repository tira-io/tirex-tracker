# CLI Tool

The TIREx Tracker CLI wraps any shell command and records hardware metrics and metadata while it runs.

## Installation

Download a prebuilt binary for your operating system and architecture from the [latest GitHub release](https://github.com/tira-io/tirex-tracker/releases/latest).

Supported platforms:

| OS | Architectures |
|---|---|
| Ubuntu 18.04 – 25.04 | x86\_64, ARM64 |
| macOS 14 – 15 | ARM64 (Apple Silicon), x86\_64 |
| Windows 2022, 2025 | x86\_64, ARM64 |

After downloading, make the binary executable (Linux/macOS) and verify the installation:

```shell
chmod +x tirex-tracker
./tirex-tracker --help
```

## Basic usage

```shell
tirex-tracker "<command>"
```

Replace `<command>` with the shell command whose execution you want to measure. TIREx Tracker starts tracking immediately before the command runs and stops as soon as the command exits.

### Example

```shell
tirex-tracker "python train.py --epochs 10"
```

This records CPU usage, RAM usage, wall-clock time, GPU metrics (if available), energy consumption, and Git repository state for the duration of `python train.py --epochs 10`.

## Output format

By default, TIREx Tracker prints a YAML summary to standard output after the command completes. To export in the [`ir_metadata`](../guides/ir-metadata.md) format instead, use the `--output` flag:

```shell
tirex-tracker --fromat irmetadata -o ir_metadata.yml "python train.py"
```

## Selecting measures

By default, all available measures are collected. To restrict tracking to specific groups, use the `--measures` flag:

```shell
tirex-tracker --source system --source energy "python train.py"
```

Available groups: `system`, `git`, `energy`, `gpu`, `devcontainer`.

## Poll interval

Time-series measures (CPU usage, RAM usage, GPU utilization) are sampled at a configurable interval. The default is 100 ms. Increase it to reduce overhead, decrease it for finer-grained data:

```shell
tirex-tracker --poll-interval 500 "python train.py"
```

## Full option reference

```
tirex-tracker --help
```
