package io.tira.measure.example

import io.tira.measure.*
import java.lang.Thread.sleep

private fun logCallback(level: LogLevel, component: String, message: String) {
    println("[${level.name}] [$component] $message")
}

fun main() {
    println(providerInfos)
    val result = measure(setOf(Measure.TIME_ELAPSED_WALL_CLOCK_MS), 100, ::logCallback) {
        sleep(2000)
    }
    println(result)
}
