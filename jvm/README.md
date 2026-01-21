# TIREx Tracker

[![CI](https://img.shields.io/github/actions/workflow/status/tira-io/tirex-tracker/ci.yml?branch=master&style=flat-square)](https://github.com/tira-io/tirex-tracker/actions/workflows/ci.yml)
[![Maintenance](https://img.shields.io/maintenance/yes/2025?style=flat-square)](https://github.com/tira-io/tirex-tracker/graphs/contributors)
[![Code coverage](https://img.shields.io/codecov/c/github/tira-io/tirex-tracker?style=flat-square)](https://codecov.io/github/tira-io/tirex-tracker/)
\
[![Release](https://img.shields.io/github/v/tag/tira-io/tirex-tracker?style=flat-square&label=library)](https://github.com/tira-io/tirex-tracker/releases/)
[![Ubuntu](https://img.shields.io/badge/ubuntu-18.04_%7C_20.04_%7C_22.04_%7C_24.04_%7C_25.04-blue?style=flat-square)](https://github.com/tira-io/tirex-tracker/releases/)
[![macOS](https://img.shields.io/badge/macos-13_%7C_14_%7C_15-blue?style=flat-square)](https://github.com/tira-io/tirex-tracker/releases/)
[![Windows](https://img.shields.io/badge/windows-2022_%7C_2025-blue?style=flat-square)](https://github.com/tira-io/tirex-tracker/releases/)
\
[![Maven](https://img.shields.io/maven-central/v/io.tira/tirex-tracker?style=flat-square)](https://central.sonatype.com/artifact/io.tira/tirex-tracker)
[![Java](https://img.shields.io/badge/java-8_%7C_11_%7C_17_%7C_21-blue?style=flat-square)](https://github.com/tira-io/tirex-tracker/packages/)
\
[![Issues](https://img.shields.io/github/issues/tira-io/tirex-tracker?style=flat-square)](https://github.com/tira-io/tirex-tracker/issues)
[![Commit activity](https://img.shields.io/github/commit-activity/m/tira-io/tirex-tracker?style=flat-square)](https://github.com/tira-io/tirex-tracker/commits)
[![License](https://img.shields.io/github/license/tira-io/tirex-tracker?style=flat-square)](LICENSE)

## Java/Kotlin/JVM API

The Java/Kotlin API can be installed via Gradle or Maven from the [Maven Central Repository](https://central.sonatype.com/artifact/io.tira/tirex-tracker).
After installing the package, you can use the TIREx tracker JVM API in your [Java](#java-usage) or [Kotlin](#kotlin-usage) projects.

<details><summary>Alternative: GitHub Packages</summary>

Alternatively to the Maven Central Repository, the TIREx tracker JVM API is also published to [GitHub Packages](https://github.com/tira-io/tirex-tracker/packages/).
To use GitHub Packages, you must first authenticate ([Maven instructions](https://docs.github.com/en/packages/working-with-a-github-packages-registry/working-with-the-apache-maven-registry#authenticating-to-github-packages), [Gradle instructions](https://docs.github.com/en/packages/working-with-a-github-packages-registry/working-with-the-gradle-registry#authenticating-to-github-packages)).

</details>

### Gradle Dependency

For Gradle, add the following line to the `dependencies` block of your `build.gradle` (or `build.gradle.kts`) file:

```gradle
dependencies {
    implementation("io.tira:tirex-tracker:x.x.x")
}
```

### Maven Dependency

For Maven projects, add these lines to your `pom.xml` file:

```xml
<dependency>
  <groupId>io.tira</groupId>
  <artifactId>tirex-tracker</artifactId>
  <version>x.x.x</version>
</dependency>
```

Replace the version placeholder (`x.x.x`) with the [latest available version tag](https://central.sonatype.com/artifact/io.tira/tirex-tracker).

You can now use the TIREx tracker JVM API in your [Java](#java-usage) or [Kotlin](#kotlin-usage) projects.

### Java Usage

For pure Java projects, the easiest way is to use the `track` function and pass your code to be tracked as a lambda like this:

```java
package io.tira.tirex.tracker.example;

import io.tira.tirex.tracker.*;

void main() {
    var result = Tracker.track(() -> {
        // Do something...
    });

    System.out.println(result);
}
```

Alternatively, use the [try-with-resources syntax](https://baeldung.com/java-try-with-resources) like this:

```java
Tracked tracked = new Tracked();
try (tracked) {
    // Do something...
}
    
System.out.println(tracked.result);
```

<!-- TODO: Explain parameters. -->

### Kotlin Usage

In Kotlin projects, the `track` function takes an [inline block](https://kotlinlang.org/docs/inline-functions.html) so that your code can be tracked like this:

```kotlin
package io.tira.tirex.tracker.example

import io.tira.tirex.tracker.*

fun main() {
  val result = track {
    // Do something...
  }

  println(result)
}
```

<!-- TODO: Explain parameters. -->

<!-- TODO: ir_metadata export instructions. -->

## Citation

If you find our work useful and reference or use it in a paper, please cite us.

```bibtex
@inproceedings{tirextracker2025,
    author = {Hagen, Tim and Fr{\"o}be, Maik and Merker, Jan Heinrich and Scells, Harrisen and Hagen, Matthias and Potthast, Martin},
    booktitle = {48th International ACM SIGIR Conference on Research and Development in Information Retrieval (SIGIR 2025)},
    month = jul,
    publisher = {ACM},
    title = {{TIREx Tracker: The Information Retrieval Experiment Tracker}},
    year = {2025}
}
```

You can also use the [`CITATION.cff`](https://github.com/tira-io/tirex-tracker/blob/master/CITATION.cff) to generate a citation in other formats.

## License

The TIREx tracker code is licensed under the [MIT License](https://github.com/tira-io/tirex-tracker/blob/master/LICENSE).
If you use the TIREx tracker in your experiments, we would appreciate you [citing](#citation) our paper.

## Abstract

> The reproducibility and transparency of retrieval experiments heavily depends on properly provided information on the experimental setup and conditions. But as manually curating such experiment metadata can be tedious, error-prone, and inconsistent, metadata should be systematically collected in an automatic way—similar to the collection of Python and git-specific settings in the `ir_metadata` reference implementation. To enable a platform-independent automatic metadata collection following the `ir_metadata` specification, we introduce the TIREx tracker: a tool realized via a lightweight C binary, pre-compiled with all dependencies for all major platforms to track hardware configurations, usage of power/CPUs/RAM/GPUs, and experiment/system versions. The TIREx tracker seamlessly integrates into Python, Java, or C/C++ workflows and can be easily incorporated in run submissions of shared tasks, which we showcase for the TIRA/TIREx platform.
