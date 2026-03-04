# Tracked Measures

TIREx Tracker collects 51 measures from the C core, plus additional language-specific measures when using the Python or JVM wrappers.

Each measure has a **constant name** used in the C API (`TIREX_*`), a Python enum member (`Measure.*`), and a JVM enum member (`Measure.*`).

Measures are either:

- **Static** — collected once at the start or end of tracking (hardware specs, Git state, timestamps).
- **Dynamic** — sampled periodically during tracking and reduced to a time series with aggregation (CPU usage, RAM usage, GPU utilization).
- **Accumulated** — measured as a total over the tracking period (energy in joules).

---

## Operating System

| Constant | Description | Type | Example |
|---|---|---|---|
| `TIREX_OS_NAME` | Name and version of the operating system. | string | `Fedora Linux 41 (Workstation Edition)` |
| `TIREX_OS_KERNEL` | Kernel version string. | string | `Linux 6.12.8-200.fc41.x86_64 x86_64` |

---

## Time

| Constant | Description | Type | Example |
|---|---|---|---|
| `TIREX_TIME_START` | ISO 8601 timestamp when tracking started. | string | `2025-04-17T08:00:50.996022428+0000` |
| `TIREX_TIME_STOP` | ISO 8601 timestamp when tracking stopped. | string | `2025-04-17T08:00:55.666974375+0000` |
| `TIREX_TIME_ELAPSED_WALL_CLOCK_MS` | Wall-clock ("real") time elapsed in milliseconds. | string | `4670` |
| `TIREX_TIME_ELAPSED_USER_MS` | Time the tracked process spent in user mode (ms). | string | `1234` |
| `TIREX_TIME_ELAPSED_SYSTEM_MS` | Time the tracked process spent in kernel/system mode (ms). | string | `89` |

---

## CPU

| Constant | Kind | Description | Example |
|---|---|---|---|
| `TIREX_CPU_MODEL_NAME` | static | CPU model name. | `Intel(R) Core(TM) i7-10750H` |
| `TIREX_CPU_VENDOR_ID` | static | CPU vendor name. | `Intel Corporation` |
| `TIREX_CPU_ARCHITECTURE` | static | CPU architecture (x86\_64, aarch64, …). | `x86_64` |
| `TIREX_CPU_BYTE_ORDER` | static | Endianness. | `Little Endian` |
| `TIREX_CPU_AVAILABLE_SYSTEM_CORES` | static | Total logical CPU cores. | `8` |
| `TIREX_CPU_CORES_PER_SOCKET` | static | Physical cores per socket. | `4` |
| `TIREX_CPU_THREADS_PER_CORE` | static | Logical threads per core (HyperThreading). | `2` |
| `TIREX_CPU_CACHES` | static | L1/L2/L3 cache sizes. | `{"l1i": "384 KiB", "l2": "3072 KiB", …}` |
| `TIREX_CPU_FREQUENCY_MIN_MHZ` | static | Minimum CPU frequency in MHz. | `400` |
| `TIREX_CPU_FREQUENCY_MAX_MHZ` | static | Maximum CPU frequency in MHz. | `3801` |
| `TIREX_CPU_FEATURES` | static | CPU instruction set flags. | `avx2 avx512f sse4_2 …` |
| `TIREX_CPU_VIRTUALIZATION` | static | Virtualization support (VT-x / AMD-V). | `VT-x` |
| `TIREX_CPU_FREQUENCY_MHZ` | dynamic | Current CPU frequency (time series). | `{"max": 3801, "avg": 2400, …}` |
| `TIREX_CPU_USED_PROCESS_PERCENT` | dynamic | CPU usage of the tracked process (%). | `{"max": 117, "avg": 55, …}` |
| `TIREX_CPU_USED_SYSTEM_PERCENT` | dynamic | CPU usage of the entire system (%). | `{"max": 35, "avg": 12, …}` |
| `TIREX_CPU_ENERGY_SYSTEM_JOULES` | accumulated | Energy consumed by the CPU (joules). | `2970137` |

---

## RAM

| Constant | Kind | Description | Example |
|---|---|---|---|
| `TIREX_RAM_AVAILABLE_SYSTEM_MB` | static | Total system RAM in MB. | `32888` |
| `TIREX_RAM_USED_PROCESS_KB` | dynamic | RAM used by the tracked process (KB). | `{"max": 21630, "avg": 18000, …}` |
| `TIREX_RAM_USED_SYSTEM_MB` | dynamic | RAM used by the entire system (MB). | `{"max": 22040, "avg": 20000, …}` |
| `TIREX_RAM_ENERGY_SYSTEM_JOULES` | accumulated | Energy consumed by DRAM (joules). | `297013` |

---

## GPU

GPU measures require an NVIDIA GPU and NVML support. `TIREX_GPU_SUPPORTED` is always collected; the remaining measures are only meaningful when it equals `1`.

| Constant | Kind | Description | Example |
|---|---|---|---|
| `TIREX_GPU_SUPPORTED` | static | `1` if a GPU is detected and supported; `0` otherwise. | `1` |
| `TIREX_GPU_MODEL_NAME` | static | GPU model name. | `NVIDIA GeForce RTX 2060` |
| `TIREX_GPU_NUM_CORES` | static | Number of GPU shader cores. | `1920` |
| `TIREX_GPU_VRAM_AVAILABLE_SYSTEM_MB` | static | Total GPU VRAM in MB. | `6442` |
| `TIREX_GPU_USED_PROCESS_PERCENT` | dynamic | GPU utilization of the tracked process (%). | `{"max": 95, "avg": 60, …}` |
| `TIREX_GPU_USED_SYSTEM_PERCENT` | dynamic | GPU utilization of the entire system (%). | `{"max": 95, "avg": 60, …}` |
| `TIREX_GPU_VRAM_USED_PROCESS_MB` | dynamic | VRAM used by the tracked process (MB). | `{"max": 1557, "avg": 1200, …}` |
| `TIREX_GPU_VRAM_USED_SYSTEM_MB` | dynamic | VRAM used by the entire system (MB). | `{"max": 1557, "avg": 1200, …}` |
| `TIREX_GPU_ENERGY_SYSTEM_JOULES` | accumulated | Energy consumed by the GPU (joules). | `2970137` |

---

## Git

Git measures are collected from the working directory at the time tracking starts. If the current directory is not inside a Git repository, `TIREX_GIT_IS_REPO` will be `0` and all other Git measures will be absent.

| Constant | Description | Example |
|---|---|---|
| `TIREX_GIT_IS_REPO` | `1` if a Git repository was detected; `0` otherwise. | `1` |
| `TIREX_GIT_HASH` | SHA1 hash of all files checked into the repository (tree hash). | `aa5fba7…` |
| `TIREX_GIT_LAST_COMMIT_HASH` | Hash of the most recent commit. | `ff52eaf…` |
| `TIREX_GIT_BRANCH` | Currently checked-out branch name. | `main` |
| `TIREX_GIT_BRANCH_UPSTREAM` | Upstream branch name. | `origin/main` |
| `TIREX_GIT_TAGS` | Tags at the current commit (JSON array). | `["v1.2.0"]` |
| `TIREX_GIT_REMOTE_ORIGIN` | URL of the `origin` remote. | `git@github.com:org/repo.git` |
| `TIREX_GIT_UNCOMMITTED_CHANGES` | `1` if there are uncommitted changes. | `0` |
| `TIREX_GIT_UNPUSHED_CHANGES` | `1` if there are commits not yet pushed. | `1` |
| `TIREX_GIT_UNCHECKED_FILES` | `1` if there are untracked, non-ignored files. | `0` |
| `TIREX_GIT_ROOT` | Absolute path to the repository root. | `/home/user/project` |
| `TIREX_GIT_ARCHIVE_PATH` | Path to a temporary zip archive of the working tree. Deleted when the result is freed. | `/tmp/rzfa9i` |

---

## Tracker metadata

| Constant | Description | Example |
|---|---|---|
| `TIREX_VERSION_MEASURE` | Version of TIREx Tracker used for this measurement. | `0.0.11` |
| `TIREX_INVOCATION` | Command line used to launch the tracked process. | `python train.py --epochs 10` |
| `TIREX_DEVCONTAINER_CONF_PATHS` | Paths to devcontainer configuration files, if found (JSON array). | `[".devcontainer/devcontainer.json"]` |

---

## Python-specific measures

These measures are only available when using the Python wrapper.

| Python Enum | Description |
|---|---|
| `Measure.PYTHON_VERSION` | Python interpreter version string. |
| `Measure.PYTHON_MODULES` | Modules visible in the current environment (JSON array). |
| `Measure.PYTHON_INSTALLED_PACKAGES` | Installed packages with versions (JSON array). |
| `Measure.PYTHON_IS_INTERACTIVE` | `true` if the interpreter is running interactively (e.g., Jupyter). |
| `Measure.PYTHON_NOTEBOOK_FILE_PATH_IN_CODE_ARCHIVE` | Path of the executed notebook inside the code archive. |

---

## Java / Kotlin-specific measures

These measures are only available when using the JVM wrapper.

| JVM Enum | System property | Description |
|---|---|---|
| `Measure.JAVA_VERSION` | `java.version` | JVM version. |
| `Measure.JAVA_VENDOR` | `java.vendor` | JVM vendor. |
| `Measure.JAVA_HOME` | `java.home` | JVM installation directory. |
| `Measure.JAVA_CLASS_PATH` | `java.class.path` | Java class path. |
| `Measure.JAVA_CLASS_VERSION` | `java.class.version` | Class file format version. |
| `Measure.JAVA_SPECIFICATION_VERSION` | `java.specification.version` | Java specification version. |
| `Measure.JAVA_RUNTIME_VERSION` | `java.runtime.version` | Runtime version string. |
| `Measure.JAVA_VM_VERSION` | `java.vm.version` | VM version. |
| `Measure.JAVA_VM_VENDOR` | `java.vm.vendor` | VM vendor. |
| `Measure.JAVA_VM_NAME` | `java.vm.name` | VM name. |
