package io.tira.tirex.tracker.example

import io.tira.tirex.tracker.LogLevel
import io.tira.tirex.tracker.Measure
import io.tira.tirex.tracker.providerInfos
import io.tira.tirex.tracker.track
import java.lang.Thread.sleep

private fun logCallback(level: LogLevel, component: String, message: String) {
    println("[${level.name}] [$component] $message")
}

fun main() {
    println(providerInfos)
    val result = track(setOf(Measure.TIME_ELAPSED_WALL_CLOCK_MS), 100, ::logCallback) {
        sleep(2000)
    }
    println(result)
}
