# Java / Kotlin

## Installation

The JVM library is published to [Maven Central](https://central.sonatype.com/artifact/io.tira/tirex-tracker).
It bundles a precompiled native library for Linux, macOS, and Windows — no separate install required.

**Supported Java versions:** 8, 11, 17, 21

=== "Gradle (Kotlin DSL)"

    ```kotlin
    dependencies {
        implementation("io.tira:tirex-tracker:x.x.x")
    }
    ```

=== "Gradle (Groovy)"

    ```groovy
    dependencies {
        implementation 'io.tira:tirex-tracker:x.x.x'
    }
    ```

=== "Maven"

    ```xml
    <dependency>
      <groupId>io.tira</groupId>
      <artifactId>tirex-tracker</artifactId>
      <version>x.x.x</version>
    </dependency>
    ```

Replace `x.x.x` with the [latest version tag](https://central.sonatype.com/artifact/io.tira/tirex-tracker).

??? note "GitHub Packages (alternative)"

    The library is also published to [GitHub Packages](https://github.com/tira-io/tirex-tracker/packages/).
    GitHub Packages requires authentication ([Gradle instructions](https://docs.github.com/en/packages/working-with-a-github-packages-registry/working-with-the-gradle-registry#authenticating-to-github-packages), [Maven instructions](https://docs.github.com/en/packages/working-with-a-github-packages-registry/working-with-the-apache-maven-registry#authenticating-to-github-packages)).

## Usage

=== "Kotlin"

    ### Lambda block

    ```kotlin
    import io.tira.tirex.tracker.*

    fun main() {
        val result = track {
            // Your experiment code here
            trainModel()
        }
        println(result)
    }
    ```

    ### With specific measures

    ```kotlin
    import io.tira.tirex.tracker.*

    val result = track(
        measures = setOf(
            Measure.TIME_ELAPSED_WALL_CLOCK_MS,
            Measure.CPU_USED_PROCESS_PERCENT,
            Measure.RAM_USED_PROCESS_KB,
        ),
        pollIntervalMs = 100L
    ) {
        trainModel()
    }
    ```

=== "Java"

    ### Lambda

    ```java
    import io.tira.tirex.tracker.*;
    import java.util.Map;

    Map<Measure, ResultEntry> result = Tracker.track(() -> {
        // Your experiment code here
        trainModel();
    });
    System.out.println(result);
    ```

    ### Try-with-resources

    ```java
    try (Tracked tracked = new Tracked()) {
        trainModel();
    }
    System.out.println(tracked.result);
    ```

    ### With specific measures

    ```java
    import java.util.Set;

    var measures = Set.of(
        Measure.TIME_ELAPSED_WALL_CLOCK_MS,
        Measure.CPU_USED_PROCESS_PERCENT,
        Measure.RAM_USED_PROCESS_KB
    );
    var result = Tracker.track(measures, 100L, () -> trainModel());
    ```

## Reading results

The result is a `Map<Measure, ResultEntry>`. Each `ResultEntry` has:

- `value` — the measured value as a `String`
- `type` — the `ResultType` (`STRING`, `INTEGER`, `FLOATING`, `BOOLEAN`, …)

=== "Kotlin"

    ```kotlin
    val wallMs = result[Measure.TIME_ELAPSED_WALL_CLOCK_MS]?.value
    println("Elapsed: $wallMs ms")
    ```

=== "Java"

    ```java
    var wallMs = result.get(Measure.TIME_ELAPSED_WALL_CLOCK_MS).value();
    System.out.println("Elapsed: " + wallMs + " ms");
    ```

## Logging

=== "Kotlin"

    ```kotlin
    import io.tira.tirex.tracker.*

    setLogCallback { level, component, message ->
        println("[$level][$component] $message")
    }
    ```

=== "Java"

    ```java
    Tracker.setLogCallback((level, component, message) ->
        System.out.printf("[%s][%s] %s%n", level, component, message)
    );
    ```

## Java-specific measures

In addition to the platform measures tracked by the C core, the JVM wrapper collects:

| Measure | Description |
|---|---|
| `JAVA_VERSION` | JVM version string |
| `JAVA_VENDOR` | JVM vendor |
| `JAVA_HOME` | JVM installation directory |
| `JAVA_CLASS_PATH` | Java class path |
| `JAVA_CLASS_VERSION` | Class file format version |
| `JAVA_SPECIFICATION_VERSION` | Java specification version |
| `JAVA_RUNTIME_VERSION` | Runtime version string |
| `JAVA_VM_VERSION` | VM implementation version |
| `JAVA_VM_VENDOR` | VM implementation vendor |
| `JAVA_VM_NAME` | VM implementation name |

## API reference

Full Javadoc is available at [javadoc.io/doc/io.tira/tirex-tracker](https://javadoc.io/doc/io.tira/tirex-tracker).
