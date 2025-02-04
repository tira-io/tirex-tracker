package io.tira.measure

import com.sun.jna.*
import com.sun.jna.Structure.FieldOrder


enum class LogLevel(val id: Int) {
    TRACE(0),
    DEBUG(1),
    INFO(2),
    WARN(3),
    ERROR(4),
    ASSERT(5);

    companion object {
        fun fromInt(id: Int): LogLevel {
            if (entries.none { it.id == id }) {
                throw IllegalArgumentException("LogLevel with id '$id' does not exist.")
            }
            return entries.first { it.id == id }
        }
    }
}

enum class MeasureType(val id: String, val providerId: String) {
    TIME_ELAPSED_WALL_CLOCK("elapsed time.wallclock (ms)", "system") {
        override fun decode(pointer: Pointer): Any = pointer.getFloat(0)
    },
    TIME_ELAPSED_USER("elapsed time.user (ms)", "system") {
        override fun decode(pointer: Pointer): Any = pointer.getFloat(0)
    },
    TIME_ELAPSED_SYSTEM("elapsed time.system (ms)", "system") {
        override fun decode(pointer: Pointer): Any = pointer.getFloat(0)
    },
    CPU_MAX_USED_SYSTEM_PERCENT("system.CPU Utilization Max (%)", "system") {
        override fun decode(pointer: Pointer): Any = pointer.getByte(0)
    },
    CPU_AVAILABLE_SYSTEM_CORES("system.num cores", "system") {
        override fun decode(pointer: Pointer): Any = pointer.getInt(0)
    },
    RAM_MAX_USED_PROCESS_KB("resources.Max RAM used (KB)", "system") {
        override fun decode(pointer: Pointer): Any = pointer.getInt(0)
    },
    RAM_MAX_USED_SYSTEM_KB("system.Max RAM used (MB)", "system") {
        override fun decode(pointer: Pointer): Any = pointer.getInt(0)
    },
    RAM_AVAILABLE_SYSTEM_MB("system.RAM (MB)", "system") {
        override fun decode(pointer: Pointer): Any = pointer.getInt(0)
    };

    abstract fun decode(pointer: Pointer): Any

    companion object {
        fun fromId(id: String): MeasureType {
            if (entries.none { it.id == id }) {
                throw IllegalArgumentException("MeasureType with id '$id' does not exist.")
            }
            return entries.first { it.id == id }
        }
    }
}

private interface LogCallback : Callback {
    fun invoke(level: Int, component: String, message: String)
}

@Suppress("unused")
@FieldOrder("name", "description", "version")
class DataProvider : Structure() {
    @JvmField var name: String = ""
    @JvmField var description: String = ""
    @JvmField var version: String = ""
}

@Suppress("unused")
@FieldOrder("providers", "monitor", "pollIntervalMillis")
internal class Config(
    @JvmField var providers: Pointer,
    @JvmField var monitor: Boolean,
    @JvmField var pollIntervalMillis: Long,
) : Structure(), Structure.ByValue

typealias MeasurementRef = Pointer

internal typealias MeasurementResultRef = Pointer

@FieldOrder("name", "value")
internal class ResultEntry : Structure() {
    @JvmField var name: String = ""
    @JvmField var value: MeasurementResultRef? = null
}

private interface MeasureLibrary : Library {
    fun mapiGetDataProviders(buffer: Array<DataProvider>?, bufferSize: Int): Int
    fun mapiSetLogCallback(callback: LogCallback?)
    fun mapiStartMeasure(config: Config): MeasurementRef
    fun mapiStopMeasure(measure: MeasurementRef): MeasurementResultRef
    fun mapiResultGetValue(measurementResultRef: MeasurementResultRef, value: Pointer?): Boolean
    fun mapiResultGetEntries(
        measurementResultRef: MeasurementResultRef,
        buffer: Array<ResultEntry>?,
        bufferSize: Int
    ): Int

    fun mapiResultFree(measurementResultRef: MeasurementResultRef)
}

private object CMeasureLibrary : MeasureLibrary by Native.load("measureapi", MeasureLibrary::class.java)

val dataProviders: List<DataProvider>
    get() {
        val numProviders = CMeasureLibrary.mapiGetDataProviders(null, 0)
        if (numProviders == 0) {
            return emptyList()
        }
        @Suppress("UNCHECKED_CAST") val buffer = DataProvider().toArray(numProviders) as Array<DataProvider>
        CMeasureLibrary.mapiGetDataProviders(buffer, numProviders)
        return buffer.asList()
    }

private fun Iterable<String>.toStringArray() = StringArray(this.toList().toTypedArray())

fun startMeasurement(
    measures: Iterable<MeasureType>,
    pollIntervalMillis: Long? = null,
    logCallback: ((level: LogLevel, component: String, message: String) -> Unit)? = null,
): MeasurementRef {
    val internalLogCallback = if (logCallback != null) object : LogCallback {
        override fun invoke(level: Int, component: String, message: String) {
            logCallback(LogLevel.fromInt(level), component, message)
        }
    } else null
    CMeasureLibrary.mapiSetLogCallback(internalLogCallback)

    val providers = measures.map { it.providerId }.toSet().toStringArray()

    val config = if (pollIntervalMillis != null) Config(
        providers = providers,
        monitor = true,
        pollIntervalMillis = pollIntervalMillis,
    ) else Config(
        providers = providers,
        monitor = false,
        pollIntervalMillis = 0L,
    )

    val measurement = CMeasureLibrary.mapiStartMeasure(config)
    return measurement
}

private fun MeasurementResultRef.parse(name: String = ""): Map<MeasureType, Any> {
    val isLeaf = CMeasureLibrary.mapiResultGetValue(this, null)
    if (isLeaf) {
        val measureType = MeasureType.fromId(name)

        val pointerPointer = Memory(Native.POINTER_SIZE.toLong())
        CMeasureLibrary.mapiResultGetValue(this, pointerPointer)
        val valuePointer = pointerPointer.getPointer(0)

//        val value = measureType.decode(valuePointer)
        val value = valuePointer.getString(0)
        pointerPointer.close()

        return mapOf(measureType to value)
    } else {
        val namePrefix = if (name != "") "$name." else ""
        val size = CMeasureLibrary.mapiResultGetEntries(this, null, 0)
        if (size == 0) {
            return emptyMap()
        }

        @Suppress("UNCHECKED_CAST") val entries = ResultEntry().toArray(size) as Array<ResultEntry>
        CMeasureLibrary.mapiResultGetEntries(this, entries, size)

        return entries.flatMap {
            it.value?.parse("$namePrefix${it.name}")?.toList() ?: listOf()
        }.toMap()
    }
}

fun stopMeasurement(measurement: MeasurementRef): Map<MeasureType, Any> {
    val result = CMeasureLibrary.mapiStopMeasure(measurement)
    CMeasureLibrary.mapiSetLogCallback(null)
    val parsedResults = result.parse()
    CMeasureLibrary.mapiResultFree(result)
    return parsedResults
}

inline fun measuring(
    measures: Iterable<MeasureType>,
    pollIntervalMillis: Long? = null,
    noinline logCallback: ((level: LogLevel, component: String, message: String) -> Unit)? = null,
    crossinline block: () -> Unit,
): Map<MeasureType, Any> {
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

