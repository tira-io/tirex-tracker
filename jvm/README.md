# Using `measure` from Java and Kotlin (or the JVM in general)

## Java

```java
TODO
```

## Kotlin

```kotlin
import io.tira.measure.Environment

var environment = Environment(polling_ms=1000, "git.hash", "elapsed_time.wall_clock", ...)

// Then measure with a lambda...
environment.measure {
  // do something
}

// ...or manually...
environment.start_measuring()
// do something
environment.stop_measuring()

print("start git hash")
println(environment.measurements["git.hash"][0])
print("end git hash")
println(environment.measurements["git.hash"][-1])
print("elapsed time ")
println(environment.measurements["elapsed_time.wall_clock"])
```
