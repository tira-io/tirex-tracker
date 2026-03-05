# Building from Source

This page explains how to build TIREx Tracker from source for development.

## Prerequisites

- **Git** — to clone the repository
- Language-specific toolchains described below

Clone the repository first:

```shell
git clone https://github.com/tira-io/tirex-tracker.git
cd tirex-tracker
```

---

## C / C++ library

### Requirements

- **CMake** ≥ 3.24
- A C++20-compliant compiler:
  - GCC 13+ (Linux)
  - LLVM/Clang 19+ (Linux, macOS)
  - Apple Clang (macOS)
  - MSVC (Windows, via Visual Studio 2022+)
- **Ninja** (recommended) or Make
- **Doxygen** (optional, for documentation)

### Configure and build

```shell
cmake -S c/ -B c/build/ \
  -G "Ninja Multi-Config" \
  -D BUILD_SHARED_LIBS=YES \
  -D TIREX_TRACKER_BUILD_EXAMPLES=YES \
  -D TIREX_TRACKER_BUILD_TESTS=YES
```

Then build a specific target:

```shell
cmake --build c/build/ --config Release --target tirex_tracker
```

### CMake options

| Option | Default | Description |
|---|---|---|
| `BUILD_SHARED_LIBS` | `YES` | Build a shared library (`.so`/`.dylib`/`.dll`). |
| `TIREX_TRACKER_BUILD_EXAMPLES` | `OFF` | Build the example programs. |
| `TIREX_TRACKER_BUILD_TESTS` | `OFF` | Build the test suite. |
| `TIREX_TRACKER_BUILD_DOCS` | `OFF` | Generate Doxygen documentation. |
| `TIREX_TRACKER_BUILD_DEB` | `OFF` | Build a Debian package. |
| `TIREX_TRACKER_EXTENSION_IR` | `ON` | Include the IR metadata export extension. |
| `TIREX_TRACKER_ONLY_DOCS` | `OFF` | Build documentation only (skips library compilation). |

### Available build targets

| Target | Type | Description |
|---|---|---|
| `tirex_tracker` | library | Shared library with all extensions. |
| `measure` | executable | The `tirex-tracker` CLI tool. |
| `tirex_tracker_docs` | docs | Doxygen HTML documentation. |
| `01_tracking` | example | Basic tracking example. |
| `02_list_measures` | example | Measure enumeration example. |
| `04_ir_extension` | example | IR metadata extension example. |
| `package` | package | Debian `.deb` package (requires `TIREX_TRACKER_BUILD_DEB=YES`). |

### Running C tests

```shell
cmake --build c/build/ --config Release --target tirex_tracker_tests
ctest --test-dir c/build/ -C Release --output-on-failure
```

### Generating Doxygen documentation

```shell
cmake -S c/ -B c/build/ -D TIREX_TRACKER_BUILD_DOCS=YES
cmake --build c/build/ --target tirex_tracker_docs
# Open c/build/docs/html/index.html
```

### CMake presets

The project includes CMake presets for common configurations. List available presets:

```shell
cmake --list-presets -S c/
```

---

## Python wrapper

### Requirements

- Python 3.8+
- pip
- The compiled C library must be available. Build `tirex_tracker` first (see above), then:

```shell
# Make the shared library discoverable (Linux example)
export LD_LIBRARY_PATH="$(pwd)/c/build/Release:$LD_LIBRARY_PATH"
```

Alternatively, install the published wheel from PyPI (which bundles the native library):

```shell
pip install tirex-tracker
```

### Development install

```shell
python3 -m venv venv/
source venv/bin/activate   # Windows: venv\Scripts\activate
pip install -e python/[tests,examples]
```

### Quality checks

```shell
ruff check python/          # linting and format checks
mypy python/                # static type checking
bandit -c python/pyproject.toml -r python/  # security scanning
```

### Running Python tests

```shell
pytest python/ -v
```

### Building the docs locally

```shell
pip install -r docs/requirements.txt
mkdocs serve
# Open http://127.0.0.1:8000
```

!!! note "Native library required"
    `mkdocstrings` imports the `tirex_tracker` Python module to generate API docs. The native library must be available in the Python path, or `tirex-tracker` must be installed from PyPI.

---

## JVM (Kotlin / Java) wrapper

### Requirements

- JDK 8, 11, 17, or 21
- Gradle (via the included wrapper — no separate install needed)

The native library is downloaded automatically from the GitHub release artifacts during the Gradle build.

### Build

```shell
jvm/gradlew --project-dir jvm/ build
```

### Quality checks and tests

```shell
jvm/gradlew --project-dir jvm/ check
```

### Generate Javadoc / Dokka HTML

```shell
jvm/gradlew --project-dir jvm/ dokkaHtml
# Output: jvm/library/build/dokka/html/
```

---

## Development container

A fully configured development container is available in [.devcontainer/](.devcontainer/) with all compilers, tools, and language runtimes pre-installed. Open the project in VS Code and select **"Reopen in Container"** for an instant development environment.
