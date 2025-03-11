# TIREx Tracker

[![CI](https://img.shields.io/github/actions/workflow/status/tira-io/tirex-tracker/ci.yml?branch=master&style=flat-square)](https://github.com/tira-io/tirex-tracker/actions/workflows/ci.yml)
[![Maintenance](https://img.shields.io/maintenance/yes/2025?style=flat-square)](https://github.com/tira-io/tirex-tracker/graphs/contributors) [![Code coverage](https://img.shields.io/codecov/c/github/tira-io/tirex-tracker?style=flat-square)](https://codecov.io/github/tira-io/tirex-tracker/) \
[![Maven](https://img.shields.io/maven-central/v/io.tira/tirex-tracker?style=flat-square)](https://central.sonatype.com/artifact/io.tira/tirex-tracker) [![Java](https://img.shields.io/badge/java-8_%7C_11_%7C_17_%7C_21-blue?style=flat-square)](https://github.com/tira-io/tirex-tracker/packages/)  [![Javadoc](https://javadoc.io/badge2/io.tira/tirex-tracker/javadoc.svg?style=flat-square)](https://javadoc.io/doc/io.tira/tirex-tracker)  \
[![Issues](https://img.shields.io/github/issues/tira-io/tirex-tracker?style=flat-square)](https://github.com/tira-io/tirex-tracker/issues) [![Commit activity](https://img.shields.io/github/commit-activity/m/tira-io/tirex-tracker?style=flat-square)](https://github.com/tira-io/tirex-tracker/commits) [![License](https://img.shields.io/github/license/tira-io/tirex-tracker?style=flat-square)](LICENSE)

The TIREx tracker is a command line tool and API to automatically track resource usage, hardware specifications, and other metadata when running information retrieval experiments.
It can be used easily in [Java/Kotlin applications](#javakotlinjvm-api). For more information, refer to the [project's readme](https://github.com/tira-io/tirex-tracker?tab=readme-ov-file#readme).

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

> [!TIP]
> An exhaustive documentation of the TIREx tracker's Java API can be found in its [Javadoc](https://javadoc.io/doc/io.tira/tirex-tracker).

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

The TIREx tracker's accompanying paper is under review. We will add citation instructions once it is published.

<!-- TODO: Add citation (written out and as BibTeX). -->

## License

The TIREx tracker code is licensed under the [MIT License](./LICENSE).
If you use the TIREx tracker in your experiments, we would appreciate you [citing](#citation) our paper.

## Abstract

> The reproducibility and transparency of retrieval experiments heavily depends on properly provided information on the experimental setup and conditions. But as manually curating such experiment metadata can be tedious, error-prone, and inconsistent, metadata should be systematically collected in an automatic way—similar to the collection of Python and git-specific settings in the `ir_metadata` reference implementation. To enable a platform-independent automatic metadata collection following the `ir_metadata` specification, we introduce the TIREx tracker: a tool realized via a lightweight C binary, pre-compiled with all dependencies for all major platforms to track hardware configurations, usage of power/CPUs/RAM/GPUs, and experiment/system versions. The TIREx tracker seamlessly integrates into Python, Java, or C/C++ workflows and can be easily incorporated in run submissions of shared tasks, which we showcase for the TIRA/TIREx platform. Code, binaries, and documentation are publicly available at <https://github.com/tira-io/tirex-tracker>.
