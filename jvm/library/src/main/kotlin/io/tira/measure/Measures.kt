@file:JvmName("Measures")

package io.tira.measure

import com.sun.jna.*
import com.sun.jna.Structure.FieldOrder


enum class LogLevel(val id: Int) {
    TRACE(0), DEBUG(1), INFO(2), WARN(3), ERROR(4), ASSERT(5);

    companion object {
        fun fromInt(id: Int): LogLevel {
            if (entries.none { level -> level.id == id }) {
                throw IllegalArgumentException("${LogLevel::class.simpleName} with id '$id' does not exist.")
            }
            return entries.first { level -> level.id == id }
        }
    }
}

enum class Measure(val id: String, val providerId: String) {
    TIME_ELAPSED_WALL_CLOCK("elapsed time.wallclock (ms)", "system"),
    TIME_ELAPSED_USER("elapsed time.user (ms)", "system"),
    TIME_ELAPSED_SYSTEM("elapsed time.system (ms)", "system"),
    CPU_MAX_USED_SYSTEM_PERCENT("system.CPU Utilization Max (%)", "system"),
    CPU_AVAILABLE_SYSTEM_CORES("system.num cores", "system"),
    RAM_MAX_USED_PROCESS_KB("resources.Max RAM used (KB)", "system"),
    RAM_MAX_USED_SYSTEM_KB("system.Max RAM used (MB)", "system"),
    RAM_AVAILABLE_SYSTEM_MB("system.RAM (MB)", "system");

    companion object {
        val ALL = setOf(
            TIME_ELAPSED_WALL_CLOCK,
            TIME_ELAPSED_USER,
            TIME_ELAPSED_SYSTEM,
            CPU_MAX_USED_SYSTEM_PERCENT,
            CPU_AVAILABLE_SYSTEM_CORES,
            RAM_MAX_USED_PROCESS_KB,
            RAM_MAX_USED_SYSTEM_KB,
            RAM_AVAILABLE_SYSTEM_MB,
        )

        fun fromId(id: String): Measure {
            if (entries.none { measure -> measure.id == id }) {
                throw IllegalArgumentException("${Measure::class.simpleName} with id '$id' does not exist.")
            }
            return entries.first { measure -> measure.id == id }
        }
    }
}

private interface LoggingCallback : Callback {
    fun invoke(level: Int, component: String, message: String)
}

@FieldOrder("name", "description", "version")
internal class DataProvider : Structure() {
    @JvmField
    var name: String = ""

    @JvmField
    var description: String = ""

    @JvmField
    var version: String? = ""

    fun toProvider(): Provider {
        return Provider(name, description, version)
    }
}


data class Provider(
    val name: String,
    val description: String,
    val version: String?,
)

@FieldOrder("providers", "monitor", "pollIntervalMillis")
internal class Config(
    @Suppress("unused") @JvmField var providers: Pointer,
    @Suppress("unused") @JvmField var monitor: Boolean,
    @Suppress("unused") @JvmField var pollIntervalMillis: Long,
) : Structure(), Structure.ByValue

typealias MeasurementRef = Pointer

internal typealias MeasurementResultRef = Pointer

@FieldOrder("name", "value")
internal class ResultEntry : Structure() {
    @JvmField
    var name: String = ""

    @JvmField
    var value: MeasurementResultRef? = null
}

private interface MeasureLibrary : Library {
    fun mapiGetDataProviders(buffer: Array<DataProvider>?, bufferSize: Int): Int
    fun mapiSetLogCallback(callback: LoggingCallback?)
    fun mapiStartMeasure(config: Config): MeasurementRef
    fun mapiStopMeasure(measure: MeasurementRef): MeasurementResultRef
    fun mapiResultGetValue(measurementResultRef: MeasurementResultRef, value: Pointer?): Boolean
    fun mapiResultGetEntries(
        measurementResultRef: MeasurementResultRef, buffer: Array<ResultEntry>?, bufferSize: Int
    ): Int

    fun mapiResultFree(measurementResultRef: MeasurementResultRef)
}

private object NativeMeasureLibrary : MeasureLibrary by Native.load("measureapi", MeasureLibrary::class.java)

val providers: List<Provider>
    get() {
        val numProviders = NativeMeasureLibrary.mapiGetDataProviders(null, 0)
        if (numProviders == 0) {
            return emptyList()
        }
        @Suppress("UNCHECKED_CAST") val buffer = DataProvider().toArray(numProviders) as Array<DataProvider>
        NativeMeasureLibrary.mapiGetDataProviders(buffer, numProviders)
        return buffer.map { provider -> provider.toProvider() }
    }

private fun Iterable<String>.toStringArray() = StringArray(this.toList().toTypedArray())

fun startMeasurement(
    measures: Iterable<Measure>,
    pollIntervalMillis: Long? = null,
    logCallback: ((level: LogLevel, component: String, message: String) -> Unit)? = null,
): MeasurementRef {
    val internalLogCallback = if (logCallback != null) object : LoggingCallback {
        override fun invoke(level: Int, component: String, message: String) {
            logCallback(LogLevel.fromInt(level), component, message)
        }
    } else null
    NativeMeasureLibrary.mapiSetLogCallback(internalLogCallback)

    val providers = measures.map { measure -> measure.providerId }.toSet().toStringArray()

    val config = if (pollIntervalMillis != null) Config(
        providers = providers,
        monitor = true,
        pollIntervalMillis = pollIntervalMillis,
    ) else Config(
        providers = providers,
        monitor = false,
        pollIntervalMillis = 0L,
    )

    val measurement = NativeMeasureLibrary.mapiStartMeasure(config)
    return measurement
}

private fun MeasurementResultRef.parse(name: String = ""): Map<Measure, Any> {
    val isLeaf = NativeMeasureLibrary.mapiResultGetValue(this, null)
    if (isLeaf) {
        val measure = Measure.fromId(name)

        val pointerPointer = Memory(Native.POINTER_SIZE.toLong())
        NativeMeasureLibrary.mapiResultGetValue(this, pointerPointer)
        val valuePointer = pointerPointer.getPointer(0)

        val value = valuePointer.getString(0)
        pointerPointer.close()

        return mapOf(measure to value)
    } else {
        val namePrefix = if (name != "") "$name." else ""
        val size = NativeMeasureLibrary.mapiResultGetEntries(this, null, 0)
        if (size == 0) {
            return emptyMap()
        }

        @Suppress("UNCHECKED_CAST") val entries = ResultEntry().toArray(size) as Array<ResultEntry>
        NativeMeasureLibrary.mapiResultGetEntries(this, entries, size)

        return entries.flatMap { entry ->
            requireNotNull(entry.value).parse("$namePrefix${entry.name}").toList()
        }.toMap()
    }
}

fun stopMeasurement(measurement: MeasurementRef): Map<Measure, Any> {
    val result = NativeMeasureLibrary.mapiStopMeasure(measurement)
    NativeMeasureLibrary.mapiSetLogCallback(null)
    val parsedResults = result.parse()
    NativeMeasureLibrary.mapiResultFree(result)
    return parsedResults
}

inline fun measure(
    measures: Iterable<Measure> = Measure.ALL,
    pollIntervalMillis: Long? = null,
    noinline logCallback: ((level: LogLevel, component: String, message: String) -> Unit)? = null,
    crossinline block: () -> Unit,
): Map<Measure, Any> {
    val measurement = startMeasurement(
        measures = measures,
        pollIntervalMillis = pollIntervalMillis,
        logCallback = logCallback,
    )
    try {
        block()
    } catch (e: Throwable) {
        stopMeasurement(measurement)
        throw e
    }
    return stopMeasurement(measurement)
}

interface JvmLogCallback {
    fun invoke(level: LogLevel, component: String, message: String)
}

interface JvmBlockCallback {
    fun invoke()
}

fun measure(
    measures: Iterable<Measure> = Measure.ALL,
    pollIntervalMillis: Long? = null,
    logCallback: JvmLogCallback? = null,
    block: JvmBlockCallback,
): Map<Measure, Any> {
    return measure(
        measures = measures,
        pollIntervalMillis = pollIntervalMillis,
        logCallback = if (logCallback != null) {
            { level: LogLevel, component: String, message: String ->
                logCallback.invoke(level, component, message)
            }
        } else null,
        block = {
            block.invoke()
        },
    )
}

fun measure(
    measures: Iterable<Measure> = Measure.ALL,
    pollIntervalMillis: Long? = null,
    block: JvmBlockCallback,
): Map<Measure, Any> {
    return measure(
        measures = measures,
        pollIntervalMillis = pollIntervalMillis,
        logCallback = null,
        block = block,
    )
}

fun measure(
    measures: Iterable<Measure> = Measure.ALL,
    block: JvmBlockCallback,
): Map<Measure, Any> {
    return measure(
        measures = measures,
        pollIntervalMillis = null,
        block = block,
    )
}

fun measure(
    block: JvmBlockCallback,
): Map<Measure, Any> {
    return measure(
        measures = Measure.ALL,
        block = block,
    )
}
