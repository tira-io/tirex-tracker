@file:JvmName("Tracker")

package io.tira.tirex.tracker

import com.sun.jna.*
import com.sun.jna.Structure.FieldOrder
import com.sun.jna.platform.unix.LibCAPI


private const val ENCODING = "ascii"


enum class Error(val value: Int) {
    SUCCESS(0), INVALID_ARGUMENT(1);

    companion object {
        internal fun fromValue(value: Int): Error {
            if (entries.none { it.value == value }) {
                throw IllegalArgumentException("${Error::class.simpleName} with value '$value' does not exist.")
            }
            return entries.first { it.value == value }
        }
    }
}

enum class Measure(val value: Int) {
    OS_NAME(0), OS_KERNEL(1), TIME_ELAPSED_WALL_CLOCK_MS(2), TIME_ELAPSED_USER_MS(3), TIME_ELAPSED_SYSTEM_MS(4), CPU_USED_PROCESS_PERCENT(
        5
    ),
    CPU_USED_SYSTEM_PERCENT(6), CPU_AVAILABLE_SYSTEM_CORES(7), CPU_ENERGY_SYSTEM_JOULES(8), CPU_FEATURES(9), CPU_FREQUENCY_MHZ(
        10
    ),
    CPU_FREQUENCY_MIN_MHZ(11), CPU_FREQUENCY_MAX_MHZ(12), CPU_VENDOR_ID(13), CPU_BYTE_ORDER(14), CPU_ARCHITECTURE(15), CPU_MODEL_NAME(
        16
    ),
    CPU_CORES_PER_SOCKET(17), CPU_THREADS_PER_CORE(18), CPU_CACHES(19), CPU_VIRTUALIZATION(20), RAM_USED_PROCESS_KB(21), RAM_USED_SYSTEM_MB(
        22
    ),
    RAM_AVAILABLE_SYSTEM_MB(23), RAM_ENERGY_SYSTEM_JOULES(24), GPU_SUPPORTED(25), GPU_MODEL_NAME(26), GPU_AVAILABLE_SYSTEM_CORES(
        27
    ), // aka. GPU_NUM_CORES
    GPU_USED_PROCESS_PERCENT(28), GPU_USED_SYSTEM_PERCENT(29), GPU_VRAM_USED_PROCESS_MB(30), GPU_VRAM_USED_SYSTEM_MB(31), GPU_VRAM_AVAILABLE_SYSTEM_MB(
        32
    ),
    GPU_ENERGY_SYSTEM_JOULES(33), GIT_IS_REPO(34), GIT_HASH(35), GIT_LAST_COMMIT_HASH(36), GIT_BRANCH(37), GIT_BRANCH_UPSTREAM(
        38
    ),
    GIT_TAGS(39), GIT_REMOTE_ORIGIN(40), GIT_UNCOMMITTED_CHANGES(41), GIT_UNPUSHED_CHANGES(42), GIT_UNCHECKED_FILES(43);

    companion object {
        internal fun fromValue(value: Int): Measure {
            if (entries.none { it.value == value }) {
                throw IllegalArgumentException("${Measure::class.simpleName} with value '$value' does not exist.")
            }
            return entries.first { it.value == value }
        }
    }
}

private const val INVALID_MEASURE = -1

val ALL_MEASURES = Measure.entries.toSet()

enum class Aggregation(val value: Int) {
    NO(1 shl 0), MAX(1 shl 1), MIN(1 shl 2), MEAN(1 shl 3);

    companion object {
        internal fun fromValue(value: Int): Aggregation {
            if (entries.none { it.value == value }) {
                throw IllegalArgumentException("${Aggregation::class.simpleName} with value '$value' does not exist.")
            }
            return entries.first { it.value == value }
        }
    }
}

private const val INVALID_AGGREGATION = -1

val ALL_AGGREGATIONS = Aggregation.entries.toSet()

class MeasurementHandle : Structure()

enum class ResultType(val value: Int) {
    STRING(0), INTEGER(1), FLOATING(2);

    companion object {
        internal fun fromValue(value: Int): ResultType {
            if (entries.none { it.value == value }) {
                throw IllegalArgumentException("${ResultType::class.simpleName} with value '$value' does not exist.")
            }
            return entries.first { it.value == value }
        }
    }
}

internal class Result : Structure()

data class ResultEntry(
    val source: Measure,
    val value: String?,
    val type: ResultType,
)

@FieldOrder("source", "value", "type")
internal open class NativeResultEntry(pointer: Pointer? = null) : Structure(pointer), Structure.ByReference {
    @JvmField
    var source: Int? = null

    @JvmField
    var value: String? = null

    @JvmField
    var type: Int? = null

    fun toResultEntry(): ResultEntry {
        autoRead()
        return ResultEntry(
            source = Measure.fromValue(requireNotNull(source)),
            value = requireNotNull(value),
            type = ResultType.fromValue(requireNotNull(type)),
        )
    }
}

data class MeasureConfiguration(
    val measure: Measure,
    val aggregation: Aggregation,
)

@FieldOrder("measure", "aggregation")
internal open class NativeMeasureConfiguration() : Structure() {
    @JvmField
    var measure: Int? = null

    @JvmField
    var aggregation: Int? = null
}

private val NULL_MEASURE_CONFIGURATION = NativeMeasureConfiguration().also {
    it.measure = INVALID_MEASURE
    it.aggregation = INVALID_AGGREGATION
}


enum class LogLevel(val value: Int) {
    TRACE(0), DEBUG(1), INFO(2), WARN(3), ERROR(4), ASSERT(5);

    companion object {
        internal fun fromValue(value: Int): LogLevel {
            if (entries.none { it.value == value }) {
                throw IllegalArgumentException("${LogLevel::class.simpleName} with value '$value' does not exist.")
            }
            return entries.first { it.value == value }
        }
    }
}

interface LogCallback {
    operator fun invoke(level: LogLevel, component: String, message: String)
}

private object NoopLogCallback : LogCallback {
    override fun invoke(level: LogLevel, component: String, message: String) = Unit
}

fun noopLogCallback(level: LogLevel, component: String, message: String) = Unit

internal interface NativeLogCallback : Callback {
    fun invoke(level: Int, component: String, message: String)
}


private fun ((level: LogLevel, component: String, message: String) -> Unit).toNativeLogCallback(): NativeLogCallback {
    return object : NativeLogCallback {
        override fun invoke(level: Int, component: String, message: String) {
            this@toNativeLogCallback(LogLevel.fromValue(level), component, message)
        }
    }
}

private fun LogCallback.toNativeLogCallback(): NativeLogCallback {
    return object : NativeLogCallback {
        override fun invoke(level: Int, component: String, message: String) {
            this@toNativeLogCallback(LogLevel.fromValue(level), component, message)
        }
    }
}

data class ProviderInfo(
    val name: String,
    val description: String,
    val version: String?,
)

@FieldOrder("name", "description", "version")
internal open class NativeProviderInfo(pointer: Pointer? = null) : Structure(pointer) {
    @JvmField
    val name: String? = null

    @JvmField
    val description: String? = null

    @JvmField
    val version: String? = null

    fun toProvider(): ProviderInfo {
        autoRead()
        return ProviderInfo(
            name = requireNotNull(name),
            description = requireNotNull(description),
            version = version,
        )
    }
}


data class MeasureInfo(
    val description: String,
    val dataType: ResultType,
    val example: String,
)


@FieldOrder("description", "dataType", "example")
internal open class NativeMeasureInfo(pointer: Pointer? = null) : Structure(pointer) {
    @JvmField
    var description: String? = null

    @JvmField
    var dataType: Int? = null

    @JvmField
    var example: String? = null

    fun toMeasureInfo(): MeasureInfo {
        autoRead()
        return MeasureInfo(
            description = requireNotNull(description),
            dataType = ResultType.fromValue(requireNotNull(dataType)),
            example = requireNotNull(example),
        )
    }
}


private interface TrackerLibrary : Library {
    fun msrResultEntryGetByIndex(result: Pointer, index: LibCAPI.size_t, entry: Pointer): Int
    fun msrResultEntryNum(result: Pointer, num: Pointer): Int
    fun msrResultFree(result: Pointer)
    fun msrFetchInfo(measures: Array<NativeMeasureConfiguration>, result: Pointer): Int
    fun msrStartMeasure(measures: Array<NativeMeasureConfiguration>, pollIntervalMs: LibCAPI.size_t, handle: Pointer): Int
    fun msrStopMeasure(handle: Pointer, result: Pointer): Int

    //FIXME:
    //fun msrSetLogCallback(callback: NativeLogCallback)
    fun msrDataProviderGetAll(buffer: Array<NativeProviderInfo>?, bufferSize: LibCAPI.size_t): LibCAPI.size_t
    fun msrMeasureInfoGet(measure: Int, info: Pointer): Int
}

private val LIBRARY = Native.load(
    "measureapi",
    TrackerLibrary::class.java,
    mapOf(Library.OPTION_STRING_ENCODING to ENCODING),
)

private inline fun <R> usePointer(block: (Pointer) -> R): R = Memory(Native.POINTER_SIZE.toLong()).use(block)


private inline fun <T: Structure, R> T.use(block: (T) -> R): R {
    try {
        return block(this)
    } finally {
        this.clear()
    }
}

private fun handleError(error: Error) {
    return when (error) {
        Error.SUCCESS -> Unit
        Error.INVALID_ARGUMENT -> throw IllegalArgumentException("Invalid argument in native call.")
    }
}

private fun handleError(error: Int) = handleError(Error.fromValue(error))


val providerInfos: Collection<ProviderInfo> by lazy {
    val numProviders = LIBRARY.msrDataProviderGetAll(null, LibCAPI.size_t(0)).toInt()
    if (numProviders == 0) {
        emptyList<ProviderInfo>()
    }
    @Suppress("UNCHECKED_CAST") val buffer = NativeProviderInfo().toArray(numProviders) as Array<NativeProviderInfo>
    LIBRARY.msrDataProviderGetAll(buffer, LibCAPI.size_t(numProviders.toLong()))
    buffer.map { it.toProvider() }
}

val measureInfos: Map<Measure, MeasureInfo> by lazy {
    ALL_MEASURES.associateWith { measure ->
        usePointer { measureInfoPointer ->
            val errorInt = LIBRARY.msrMeasureInfoGet(measure.value, measureInfoPointer)
            handleError(errorInt)
            NativeMeasureInfo(measureInfoPointer.getPointer(0)).toMeasureInfo()
        }
    }
}

// TODO: Add aggregation(s) (mapping) parameter.
// TODO: Maybe rename this function.
fun fetchInfo(
    measures: Iterable<Measure> = ALL_MEASURES,
    logCallback: ((level: LogLevel, component: String, message: String) -> Unit) = ::noopLogCallback,
): Map<Measure, ResultEntry> {
    //FIXME:
    //LIBRARY.msrSetLogCallback(logCallback.toNativeLogCallback())

    val configs = measures.map { measure ->
        NativeMeasureConfiguration().also {
            it.measure = measure.value
            it.aggregation = Aggregation.NO.value
        }
    } + NULL_MEASURE_CONFIGURATION
    @Suppress("UNCHECKED_CAST") val configArray = NativeMeasureConfiguration().toArray(configs.size) as Array<NativeMeasureConfiguration>
    configs.forEachIndexed { i, config ->
        configArray[i].measure = config.measure
        configArray[i].aggregation = config.aggregation
        configArray[i].write()
    }

    val result: Pointer = usePointer { resultPointer ->
        val errorInt = LIBRARY.msrFetchInfo(configArray, resultPointer)
        handleError(errorInt)
        resultPointer.getPointer(0)
    }

    //FIXME:
    //LIBRARY.msrSetLogCallback(::noopLogCallback.toNativeLogCallback())

    val numEntries: Long = usePointer { numEntriesPointer ->
        val errorInt = LIBRARY.msrResultEntryNum(result, numEntriesPointer)
        handleError(errorInt)
        LibCAPI.size_t.ByReference().also {
            it.pointer = numEntriesPointer
        }.longValue()
    }
    val entries = (0 until numEntries).map { index ->
        NativeResultEntry().use { resultEntry ->
            val errorInt = LIBRARY.msrResultEntryGetByIndex(result, LibCAPI.size_t(index), resultEntry.pointer)
            handleError(errorInt)
            resultEntry.toResultEntry()
        }
    }
    LIBRARY.msrResultFree(result)
    return entries.associate { entry ->
        entry.source to entry
    }
}

// TODO: Add aggregation(s) (mapping) parameter.
fun startTracking(
    measures: Iterable<Measure> = ALL_MEASURES,
    pollIntervalMillis: Long = -1,
    logCallback: ((level: LogLevel, component: String, message: String) -> Unit) = ::noopLogCallback,
): Pointer {
    //FIXME:
    //LIBRARY.msrSetLogCallback(logCallback.toNativeLogCallback())

    val configs = measures.map { measure ->
        NativeMeasureConfiguration().also {
            it.measure = measure.value
            it.aggregation = Aggregation.NO.value
        }
    } + NULL_MEASURE_CONFIGURATION
    @Suppress("UNCHECKED_CAST") val configArray = NativeMeasureConfiguration().toArray(configs.size) as Array<NativeMeasureConfiguration>
    configs.forEachIndexed { i, config ->
        configArray[i].measure = config.measure
        configArray[i].aggregation = config.aggregation
        configArray[i].write()
    }

    return usePointer { measurementHandlePointer ->
        val errorInt =
            LIBRARY.msrStartMeasure(configArray, LibCAPI.size_t(pollIntervalMillis), measurementHandlePointer)
        handleError(errorInt)
        measurementHandlePointer.getPointer(0)
    }
}


fun stopTracking(measureHandle: Pointer): Map<Measure, ResultEntry> {
    val result: Pointer = usePointer { resultPointer ->
        val errorInt = LIBRARY.msrStopMeasure(measureHandle, resultPointer)
        handleError(errorInt)
        resultPointer.getPointer(0)
    }

    //FIXME:
    //LIBRARY.msrSetLogCallback(::noopLogCallback.toNativeLogCallback())

    val numEntries: Long = usePointer { numEntriesPointer ->
        val errorInt = LIBRARY.msrResultEntryNum(result, numEntriesPointer)
        handleError(errorInt)
        LibCAPI.size_t.ByReference().also {
            it.pointer = numEntriesPointer
        }.longValue()
    }
    val entries = (0 until numEntries).map { index ->
        NativeResultEntry().use { resultEntry ->
            val errorInt = LIBRARY.msrResultEntryGetByIndex(result, LibCAPI.size_t(index), resultEntry.pointer)
            handleError(errorInt)
            resultEntry.toResultEntry()
        }
    }
    LIBRARY.msrResultFree(result)
    return entries.associate { entry ->
        entry.source to entry
    }
}

inline fun track(
    measures: Iterable<Measure> = ALL_MEASURES,
    pollIntervalMillis: Long = -1,
    noinline logCallback: (level: LogLevel, component: String, message: String) -> Unit = ::noopLogCallback,
    crossinline block: () -> Unit,
): Map<Measure, ResultEntry> {
    val measurement = startMeasurement(
        measures = measures,
        pollIntervalMillis = pollIntervalMillis,
        logCallback = logCallback,
    )
    try {
        block()
    } catch (e: Throwable) {
        stopTracking(measurement)
        throw e
    }
    return stopTracking(measurement)
}


interface BlockCallback {
    fun invoke()
}

fun track(
    measures: Iterable<Measure> = ALL_MEASURES,
    pollIntervalMillis: Long = -1,
    logCallback: LogCallback = NoopLogCallback,
    block: BlockCallback,
): Map<Measure, ResultEntry> {
    return track(
        measures = measures,
        pollIntervalMillis = pollIntervalMillis,
        logCallback = if (logCallback is NoopLogCallback) {
            ::noopLogCallback
        } else {
            { level: LogLevel, component: String, message: String ->
                logCallback.invoke(level, component, message)
            }
        },
        block = {
            block.invoke()
        },
    )
}

fun track(
    measures: Iterable<Measure> = ALL_MEASURES,
    pollIntervalMillis: Long = -1,
    block: BlockCallback,
): Map<Measure, ResultEntry> {
    return track(
        measures = measures,
        pollIntervalMillis = pollIntervalMillis,
        logCallback = NoopLogCallback,
        block = block,
    )
}

fun track(
    measures: Iterable<Measure> = ALL_MEASURES,
    block: BlockCallback,
): Map<Measure, ResultEntry> {
    return track(
        measures = measures,
        pollIntervalMillis = -1,
        block = block,
    )
}

fun track(
    block: BlockCallback,
): Map<Measure, ResultEntry> {
    return track(
        measures = ALL_MEASURES,
        block = block,
    )
}
