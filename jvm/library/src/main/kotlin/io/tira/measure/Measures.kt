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

        fun fromId(id: String): Measure {
            if (entries.none { measure -> measure.id == id }) {
                throw IllegalArgumentException("${Measure::class.simpleName} with id '$id' does not exist.")
            }
            return entries.first { measure -> measure.id == id }
        }
    }
}

val ALL_MEASURES = Measure.entries.toSet()

private interface NativeLogCallback : Callback {
    fun invoke(level: Int, component: String, message: String)
}

data class Provider(
    val name: String,
    val description: String,
    val version: String?,
)

@FieldOrder("name", "description", "version")
internal class NativeProvider : Structure() {
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
    fun mapiGetDataProviders(buffer: Array<NativeProvider>?, bufferSize: Int): Int
    fun mapiSetLogCallback(callback: NativeLogCallback?)
    fun mapiStartMeasure(config: Config): MeasurementRef
    fun mapiStopMeasure(measure: MeasurementRef): MeasurementResultRef
    fun mapiResultGetValue(measurementResultRef: MeasurementResultRef, value: Pointer?): Boolean
    fun mapiResultGetEntries(
        measurementResultRef: MeasurementResultRef, buffer: Array<ResultEntry>?, bufferSize: Int
    ): Int

    fun mapiResultFree(measurementResultRef: MeasurementResultRef)
}

private val LIBRARY = Native.load(
    "measureapi",
    MeasureLibrary::class.java,
    mapOf(Library.OPTION_STRING_ENCODING to "ascii"),
)

val providers: List<Provider>
    get() {
        val numProviders = LIBRARY.mapiGetDataProviders(null, 0)
        if (numProviders == 0) {
            return emptyList()
        }
        @Suppress("UNCHECKED_CAST") val buffer = NativeProvider().toArray(numProviders) as Array<NativeProvider>
        LIBRARY.mapiGetDataProviders(buffer, numProviders)
        return buffer.map { provider -> provider.toProvider() }
    }

private fun Iterable<String>.toStringArray() = StringArray(this.toList().toTypedArray())

fun startMeasurement(
    measures: Iterable<Measure> = ALL_MEASURES,
    pollIntervalMillis: Long? = null,
    logCallback: ((level: LogLevel, component: String, message: String) -> Unit)? = null,
): MeasurementRef {
    val internalLogCallback = if (logCallback != null) object : NativeLogCallback {
        override fun invoke(level: Int, component: String, message: String) {
            logCallback(LogLevel.fromInt(level), component, message)
        }
    } else null
    LIBRARY.mapiSetLogCallback(internalLogCallback)

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

    val measurement = LIBRARY.mapiStartMeasure(config)
    return measurement
}

private fun MeasurementResultRef.parse(name: String = ""): Map<Measure, String> {
    val isLeaf = LIBRARY.mapiResultGetValue(this, null)
    if (isLeaf) {
        val measure = Measure.fromId(name)

        val pointerPointer = Memory(Native.POINTER_SIZE.toLong())
        LIBRARY.mapiResultGetValue(this, pointerPointer)
        val valuePointer = pointerPointer.getPointer(0)

        val value = valuePointer.getString(0)
        pointerPointer.close()

        return mapOf(measure to value)
    } else {
        val namePrefix = if (name != "") "$name." else ""
        val numEntries = LIBRARY.mapiResultGetEntries(this, null, 0)
        if (numEntries == 0) {
            return emptyMap()
        }

        @Suppress("UNCHECKED_CAST") val entries = ResultEntry().toArray(numEntries) as Array<ResultEntry>
        LIBRARY.mapiResultGetEntries(this, entries, numEntries)

        return entries.flatMap { entry ->
            requireNotNull(entry.value).parse("$namePrefix${entry.name}").toList()
        }.toMap()
    }
}

fun stopMeasurement(measurement: MeasurementRef): Map<Measure, String> {
    val result = LIBRARY.mapiStopMeasure(measurement)
    LIBRARY.mapiSetLogCallback(null)
    val parsedResults = result.parse()
    LIBRARY.mapiResultFree(result)
    return parsedResults
}

inline fun measure(
    measures: Iterable<Measure> = ALL_MEASURES,
    pollIntervalMillis: Long? = null,
    noinline logCallback: ((level: LogLevel, component: String, message: String) -> Unit)? = null,
    crossinline block: () -> Unit,
): Map<Measure, String> {
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
    measures: Iterable<Measure> = ALL_MEASURES,
    pollIntervalMillis: Long? = null,
    logCallback: JvmLogCallback? = null,
    block: JvmBlockCallback,
): Map<Measure, String> {
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
    measures: Iterable<Measure> = ALL_MEASURES,
    pollIntervalMillis: Long? = null,
    block: JvmBlockCallback,
): Map<Measure, String> {
    return measure(
        measures = measures,
        pollIntervalMillis = pollIntervalMillis,
        logCallback = null,
        block = block,
    )
}

fun measure(
    measures: Iterable<Measure> = ALL_MEASURES,
    block: JvmBlockCallback,
): Map<Measure, String> {
    return measure(
        measures = measures,
        pollIntervalMillis = null,
        block = block,
    )
}

fun measure(
    block: JvmBlockCallback,
): Map<Measure, String> {
    return measure(
        measures = ALL_MEASURES,
        block = block,
    )
}
