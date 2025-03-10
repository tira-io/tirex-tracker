@file:JvmName("Tracker")

package io.tira.tirex.tracker

import com.sun.jna.*
import com.sun.jna.Structure.FieldOrder
import com.sun.jna.platform.unix.LibCAPI
import kotlinx.serialization.json.Json
import org.yaml.snakeyaml.DumperOptions
import org.yaml.snakeyaml.LoaderOptions
import org.yaml.snakeyaml.Yaml
import java.io.Closeable
import java.io.File
import java.nio.file.Path
import java.util.zip.GZIPOutputStream

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
    GIT_TAGS(39), GIT_REMOTE_ORIGIN(40), GIT_UNCOMMITTED_CHANGES(41), GIT_UNPUSHED_CHANGES(42), GIT_UNCHECKED_FILES(43), JAVA_VERSION(
        2001
    ),
    JAVA_VERSION_DATE(2002), JAVA_VENDOR(2003), JAVA_VENDOR_URL(2004), JAVA_VENDOR_VERSION(2005), JAVA_HOME(2006), JAVA_VM_SPECIFICATION_VERSION(
        2007
    ),

    JAVA_VM_SPECIFICATION_VENDOR(2008), JAVA_VM_SPECIFICATION_NAME(2009), JAVA_VM_VERSION(2010), JAVA_VM_VENDOR(2011), JAVA_VM_NAME(
        2012
    ),
    JAVA_SPECIFICATION_VERSION(2013), JAVA_SPECIFICATION_MAINTENANCE_VERSION(2014), JAVA_SPECIFICATION_VENDOR(2015), JAVA_SPECIFICATION_NAME(
        2016
    ),
    JAVA_CLASS_VERSION(2017), JAVA_CLASS_PATH(2018), JAVA_LIBRARY_PATH(2019), JAVA_IO_TMPDIR(2020);

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

@Suppress("unused")
@JvmField
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

@Suppress("unused")
@JvmField
val ALL_AGGREGATIONS = Aggregation.entries.toSet()

//internal class NativeTrackingHandle : Structure()

enum class ResultType(val value: Int) {
    STRING(0), INTEGER(1), FLOATING(2), STRING_LIST(3), BOOLEAN(4);

    companion object {
        internal fun fromValue(value: Int): ResultType {
            if (entries.none { it.value == value }) {
                throw IllegalArgumentException("${ResultType::class.simpleName} with value '$value' does not exist.")
            }
            return entries.first { it.value == value }
        }
    }
}

//internal class NativeResult : Structure()

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

fun noopLogCallback(
    @Suppress("unused") level: LogLevel,
    @Suppress("unused") component: String,
    @Suppress("unused") message: String
) = Unit

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


private val JAVA_PROVIDER = ProviderInfo(
    name = "Python",
    description = "Python-specific measures.",
    version = Build.VERSION,
)


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

private val JAVA_MEASURES: Map<Measure, MeasureInfo> = mapOf(
    Measure.JAVA_VERSION to MeasureInfo(
        description = "Java Runtime Environment version.",
        dataType = ResultType.STRING,
        example = Json.encodeToString("21.0.6"),
    ),
    Measure.JAVA_VERSION_DATE to MeasureInfo(
        description = "Java Runtime Environment version date, in ISO-8601 YYYY-MM-DD format.",
        dataType = ResultType.STRING,
        example = Json.encodeToString("2025-01-21"),
    ),
    Measure.JAVA_VENDOR to MeasureInfo(
        description = "Java Runtime Environment vendor.",
        dataType = ResultType.STRING,
        example = Json.encodeToString("Ubuntu"),
    ),
    Measure.JAVA_VENDOR_URL to MeasureInfo(
        description = "Java Runtime Environment vendor URL.",
        dataType = ResultType.STRING,
        example = Json.encodeToString("https://ubuntu.com/"),
    ),
    Measure.JAVA_VENDOR_VERSION to MeasureInfo(
        description = "Java Runtime Environment vendor version.",
        dataType = ResultType.STRING,
        example = Json.encodeToString("21.0.6+7-Ubuntu-124.04.1"),
    ),
    Measure.JAVA_HOME to MeasureInfo(
        description = "Java installation directory.",
        dataType = ResultType.STRING,
        example = Json.encodeToString("/usr/lib/jvm/java-21-openjdk-amd64"),
    ),
    Measure.JAVA_VM_SPECIFICATION_VERSION to MeasureInfo(
        description = "Java Virtual Machine specification version.",
        dataType = ResultType.STRING,
        example = Json.encodeToString("21"),
    ),
    Measure.JAVA_VM_SPECIFICATION_VENDOR to MeasureInfo(
        description = "Java Virtual Machine specification vendor.",
        dataType = ResultType.STRING,
        example = Json.encodeToString("Oracle Corporation"),
    ),
    Measure.JAVA_VM_SPECIFICATION_NAME to MeasureInfo(
        description = "Java Virtual Machine specification name.",
        dataType = ResultType.STRING,
        example = Json.encodeToString("Java Virtual Machine Specification"),
    ),
    Measure.JAVA_VM_VERSION to MeasureInfo(
        description = "Java Virtual Machine implementation version.",
        dataType = ResultType.STRING,
        example = Json.encodeToString("21.0.6+7-Ubuntu-124.04.1"),
    ),
    Measure.JAVA_VM_VENDOR to MeasureInfo(
        description = "Java Virtual Machine implementation vendor.",
        dataType = ResultType.STRING,
        example = Json.encodeToString("Ubuntu"),
    ),
    Measure.JAVA_VM_NAME to MeasureInfo(
        description = "Java Virtual Machine implementation name.",
        dataType = ResultType.STRING,
        example = Json.encodeToString("OpenJDK 64-Bit Server VM"),
    ),
    Measure.JAVA_SPECIFICATION_VERSION to MeasureInfo(
        description = "Java Runtime Environment specification version.",
        dataType = ResultType.STRING,
        example = Json.encodeToString("21"),
    ),
    Measure.JAVA_SPECIFICATION_MAINTENANCE_VERSION to MeasureInfo(
        description = "Java Runtime Environment specification maintenance version.",
        dataType = ResultType.STRING,
        example = Json.encodeToString("1"),
    ),
    Measure.JAVA_SPECIFICATION_VENDOR to MeasureInfo(
        description = "Java Runtime Environment specification vendor.",
        dataType = ResultType.STRING,
        example = Json.encodeToString("Oracle Corporation"),
    ),
    Measure.JAVA_SPECIFICATION_NAME to MeasureInfo(
        description = "Java Runtime Environment specification name.",
        dataType = ResultType.STRING,
        example = Json.encodeToString("Java Platform API Specification"),
    ),
    Measure.JAVA_CLASS_VERSION to MeasureInfo(
        description = "Latest Java class file format version recognized by the Java runtime.",
        dataType = ResultType.STRING,
        example = Json.encodeToString("65.0"),
    ),
    Measure.JAVA_CLASS_PATH to MeasureInfo(
        description = "Java class path.",
        dataType = ResultType.STRING_LIST,
        example = Json.encodeToString(
            listOf<String>(
                "/path/to/build/classes/java/main",
                "/path/to/build/classes/kotlin/main",
                "/path/to/kotlin-stdlib-2.1.10.jar",
                "/path/to/jna-5.16.0.jar",
                "/pth/to/jna-platform-5.16.0.jar",
            )
        ),
    ),
    Measure.JAVA_LIBRARY_PATH to MeasureInfo(
        description = "List of paths to search when loading libraries.",
        dataType = ResultType.STRING_LIST,
        example = Json.encodeToString(
            listOf<String>(
                "/usr/lib/x86_64-linux-gnu/jni",
                "/lib/x86_64-linux-gnu",
                "/usr/lib/x86_64-linux-gnu",
                "/usr/lib/jni",
                "/lib",
                "/usr/lib",
            )
        ),
    ),
    Measure.JAVA_IO_TMPDIR to MeasureInfo(
        description = "Default temp file path.",
        dataType = ResultType.STRING,
        example = Json.encodeToString("/tmp"),
    ),
)


private fun getJavaInfo(measures: Iterable<Measure>): Pair<Map<Measure, ResultEntry>, Iterable<Measure>> {
    val results = mutableMapOf<Measure, ResultEntry>()

    for (measure in measures) {
        if (measure !in JAVA_MEASURES.keys) continue

        val propertyName = measure.name.lowercase().replace("_", ".")
        val property: String? = System.getProperty(propertyName)

        val resultEntry = when (measure) {
            Measure.JAVA_CLASS_PATH, Measure.JAVA_LIBRARY_PATH -> ResultEntry(
                source = measure,
                value = Json.encodeToString(property?.split(File.pathSeparator) ?: listOf()),
                type = ResultType.STRING_LIST,
            )

            else -> ResultEntry(
                source = measure,
                value = Json.encodeToString(property),
                type = ResultType.STRING,
            )
        }

        results[measure] = resultEntry
    }

    val filteredMeasures = measures.filterNot { it in JAVA_MEASURES.keys }

    return results to filteredMeasures
}

enum class ExportFormat(val value: String) {
    IR_METADATA("ir_metadata");
}


private interface TrackerLibrary : Library {
    fun tirexResultEntryGetByIndex(result: Pointer, index: LibCAPI.size_t, entry: Pointer): Int
    fun tirexResultEntryNum(result: Pointer, num: Pointer): Int
    fun tirexResultFree(result: Pointer)
    fun tirexFetchInfo(measures: Array<NativeMeasureConfiguration>, result: Pointer): Int
    fun tirexStartTracking(
        measures: Array<NativeMeasureConfiguration>, pollIntervalMs: LibCAPI.size_t, handle: Pointer
    ): Int

    fun tirexStopTracking(handle: Pointer, result: Pointer): Int
    fun tirexSetLogCallback(callback: NativeLogCallback)
    fun tirexDataProviderGetAll(buffer: Array<NativeProviderInfo>?, bufferSize: LibCAPI.size_t): LibCAPI.size_t
    fun tirexMeasureInfoGet(measure: Int, info: Pointer): Int
    fun tirexResultExportIrMetadata(info: Pointer, result: Pointer, filePath: String): Int
}

private val LIBRARY = Native.load(
    "tirex_tracker_full",
    TrackerLibrary::class.java,
    mapOf(Library.OPTION_STRING_ENCODING to ENCODING),
)

private inline fun <R> usePointer(block: (Pointer) -> R): R = Memory(Native.POINTER_SIZE.toLong()).use(block)


private inline fun <T : Structure, R> T.use(block: (T) -> R): R {
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


@Suppress("unused")
@JvmOverloads
fun setLogCallback(
    logCallback: ((level: LogLevel, component: String, message: String) -> Unit) = ::noopLogCallback,
) {
    LIBRARY.tirexSetLogCallback(logCallback.toNativeLogCallback())
}

@Suppress("unused")

fun setLogCallback(
    logCallback: LogCallback,
) {
    LIBRARY.tirexSetLogCallback(logCallback.toNativeLogCallback())
}

val providerInfos: Collection<ProviderInfo> by lazy {
    val numProviders = LIBRARY.tirexDataProviderGetAll(null, LibCAPI.size_t(0)).toInt()
    if (numProviders == 0) {
        emptyList<ProviderInfo>()
    }
    @Suppress("UNCHECKED_CAST") val buffer = NativeProviderInfo().toArray(numProviders) as Array<NativeProviderInfo>
    LIBRARY.tirexDataProviderGetAll(buffer, LibCAPI.size_t(numProviders.toLong()))
    buffer.map { it.toProvider() } + listOf(JAVA_PROVIDER)
}

val measureInfos: Map<Measure, MeasureInfo> by lazy {
    ALL_MEASURES.associateWith { measure ->
        if (measure in JAVA_MEASURES.keys) {
            JAVA_MEASURES.getValue(measure)
        } else {
            usePointer { measureInfoPointer ->
                val errorInt = LIBRARY.tirexMeasureInfoGet(measure.value, measureInfoPointer)
                handleError(errorInt)
                NativeMeasureInfo(measureInfoPointer.getPointer(0)).toMeasureInfo()
            }
        }
    }
}

private fun parseResults(result: Pointer): Map<Measure, ResultEntry> {
    val numEntries: Long = usePointer { numEntriesPointer ->
        val errorInt = LIBRARY.tirexResultEntryNum(result, numEntriesPointer)
        handleError(errorInt)
        LibCAPI.size_t.ByReference().also {
            it.pointer = numEntriesPointer
        }.longValue()
    }
    val entries = (0 until numEntries).map { index ->
        NativeResultEntry().use { resultEntry ->
            val errorInt = LIBRARY.tirexResultEntryGetByIndex(result, LibCAPI.size_t(index), resultEntry.pointer)
            handleError(errorInt)
            resultEntry.toResultEntry()
        }
    }
    LIBRARY.tirexResultFree(result)
    return entries.associate { entry ->
        entry.source to entry
    }
}

private fun prepareMeasureConfigurations(measures: Iterable<Measure>): Array<NativeMeasureConfiguration> {
    val configs = measures.map { measure ->
        NativeMeasureConfiguration().also {
            it.measure = measure.value
            it.aggregation = Aggregation.NO.value
        }
    } + NULL_MEASURE_CONFIGURATION
    @Suppress("UNCHECKED_CAST") val configArray =
        NativeMeasureConfiguration().toArray(configs.size) as Array<NativeMeasureConfiguration>
    configs.forEachIndexed { i, config ->
        configArray[i].measure = config.measure
        configArray[i].aggregation = config.aggregation
        configArray[i].write()
    }
    return configArray
}

// TODO: Add aggregation(s) (mapping) parameter.
@JvmOverloads
fun fetchInfo(
    measures: Iterable<Measure> = ALL_MEASURES,
): Map<Measure, ResultEntry> {
    // Get Java info first, and then strip Java measures from the list.
    val (javaInfo, remainingMeasures) = getJavaInfo(measures)

    // Prepare the measure configurations.
    val configArray = prepareMeasureConfigurations(remainingMeasures)

    val result: Pointer = usePointer { resultPointer ->
        val errorInt = LIBRARY.tirexFetchInfo(configArray, resultPointer)
        handleError(errorInt)
        resultPointer.getPointer(0)
    }

    return parseResults(result) + javaInfo
}


class TrackingHandle private constructor(
    private val fetchInfoResult: Pointer,
    private val trackingHandle: Pointer,
    private val javaInfo: Map<Measure, ResultEntry>,
    private val systemName: String?,
    private val systemDescription: String?,
    private val exportFilePath: File?,
    private val exportFormat: ExportFormat?,
    val results: MutableMap<Measure, ResultEntry>,
) : AutoCloseable, Closeable, Map<Measure, ResultEntry> by results {

    companion object {
        // TODO: Add aggregation(s) (mapping) parameter.
        @JvmStatic
        @JvmOverloads
        fun start(
            measures: Iterable<Measure> = ALL_MEASURES,
            pollIntervalMillis: Long = 1000,
            systemName: String? = null,
            systemDescription: String? = null,
            exportFilePath: File? = null,
            exportFormat: ExportFormat? = null,
        ): TrackingHandle {
            // Get Java info first, and then strip Java measures from the list.
            val (javaInfo, remainingMeasures) = getJavaInfo(measures)

            // Prepare the measure configurations.
            val configArray = prepareMeasureConfigurations(remainingMeasures)

            // Get other info, first, before starting the tracking.
            val fetchInfoResult: Pointer = usePointer { resultPointer ->
                val errorInt = LIBRARY.tirexFetchInfo(configArray, resultPointer)
                handleError(errorInt)
                resultPointer.getPointer(0)
            }

            // Start the tracking.
            val trackingHandle = usePointer { measurementHandlePointer ->
                val errorInt =
                    LIBRARY.tirexStartTracking(configArray, LibCAPI.size_t(pollIntervalMillis), measurementHandlePointer)
                handleError(errorInt)
                measurementHandlePointer.getPointer(0)
            }

            return TrackingHandle(
                fetchInfoResult = fetchInfoResult,
                trackingHandle = trackingHandle,
                javaInfo = javaInfo,
                systemName = systemName,
                systemDescription = systemDescription,
                exportFilePath = exportFilePath,
                exportFormat = exportFormat,
                results = mutableMapOf(),
            )
        }

        @Suppress("unused")
        @JvmStatic
        @JvmOverloads
        fun start(
            measures: Iterable<Measure> = ALL_MEASURES,
            pollIntervalMillis: Long = 1000,
            systemName: String? = null,
            systemDescription: String? = null,
            exportFilePath: Path,
            exportFormat: ExportFormat? = null,
        ): TrackingHandle = start(
            measures = measures,
            pollIntervalMillis = pollIntervalMillis,
            systemName = systemName,
            systemDescription = systemDescription,
            exportFilePath = exportFilePath.toFile(),
            exportFormat = exportFormat,
        )
    }

    fun stop(): Map<Measure, ResultEntry> {
        val result: Pointer = usePointer { resultPointer ->
            val errorInt = LIBRARY.tirexStopTracking(trackingHandle, resultPointer)
            handleError(errorInt)
            resultPointer.getPointer(0)
        }

        export(result)

        results += parseResults(fetchInfoResult)
        results += javaInfo
        results += parseResults(result)
        return results
    }

    override fun close() {
        stop()
    }

    // Note: The `Map` member methods are provided via delegate.

    private fun export(result: Pointer) {
        if (exportFilePath == null) return
        when (exportFormat) {
            null -> exportGuessedFormat(result)
            ExportFormat.IR_METADATA -> exportIrMetadata(result)
        }
    }

    private fun exportGuessedFormat(result: Pointer) {
        if (exportFilePath == null) return
        else if (
            listOf(
                ".ir_metadata",
                ".ir-metadata",
                ".ir_metadata.yml",
                ".ir-metadata.yml",
                ".ir_metadata.yaml",
                ".ir-metadata.yaml",
                ".ir_metadata.gz",
                ".ir-metadata.gz",
                ".ir_metadata.yml.gz",
                ".ir-metadata.yml.gz",
                ".ir_metadata.yaml.gz",
                ".ir-metadata.yaml.gz",
            ).any { exportFilePath.name.endsWith(it) }
        ) exportIrMetadata(result)
    }

    @Suppress("UNCHECKED_CAST")
    private fun exportIrMetadata(result: Pointer) {
        if (exportFilePath == null) return

        if (exportFilePath.exists()) {
            throw IllegalArgumentException("Metadata file already exists.")
        }

        // Run the C-internal ir_metadata export.
        LIBRARY.tirexResultExportIrMetadata(
            fetchInfoResult,
            result,
            exportFilePath.toString(),
        )

        // Parse the initial ir_metadata.
        val yaml = Yaml(
            LoaderOptions().apply {
            },
            DumperOptions().apply {
                defaultFlowStyle = DumperOptions.FlowStyle.AUTO
            },
        )
        val irMetadataYamlString = exportFilePath.readText()
            .removePrefix("ir_metadata.start\n") // Remove optional ir_metadata prefix line.
            .removeSuffix("ir_metadata.end\n") // Remove optional ir_metadata suffix line.
            .replace(
                Regex("""caches: (.*),"""),
                """caches: {\1}"""
            ) // FIXME: There's a bug in the YAML output format (https://github.com/tira-io/tirex-tracker/issues/42) that we work around here.
        val irMetadata: MutableMap<String, Any?> = yaml.load<MutableMap<String, Any?>>(irMetadataYamlString)

        // Add user-provided metadata.
        val method: MutableMap<String, Any?> =
            irMetadata.getOrPut("method") { mutableMapOf<String, Any?>() } as MutableMap<String, Any?>
        if (systemName != null) {
            method["name"] = systemName
        }
        if (systemDescription != null) {
            method["description"] = systemDescription
        }

        // Add Java-specific metadata.
        val json = Json {
            coerceInputValues = true
        }
        val implementation: MutableMap<String, Any?> =
            irMetadata.getOrPut("implementation") { mutableMapOf<String, Any?>() } as MutableMap<String, Any?>
        val java: MutableMap<String, Any?> =
            implementation.getOrPut("java") { mutableMapOf<String, Any?>() } as MutableMap<String, Any?>
        java["version"] = javaInfo.getValue(Measure.JAVA_VERSION).value?.let { json.decodeFromString<String?>(it) }
        java["version date"] = javaInfo.getValue(Measure.JAVA_VERSION_DATE).value?.let { json.decodeFromString<String?>(it) }
        java["vendor"] = javaInfo.getValue(Measure.JAVA_VENDOR).value?.let { json.decodeFromString<String?>(it) }
        java["vendor URL"] = javaInfo.getValue(Measure.JAVA_VENDOR_URL).value?.let { json.decodeFromString<String?>(it) }
        java["vendor version"] = javaInfo.getValue(Measure.JAVA_VENDOR_VERSION).value?.let { json.decodeFromString<String?>(it) }
        java["home"] = javaInfo.getValue(Measure.JAVA_HOME).value?.let { json.decodeFromString<String?>(it) }
        java["class version"] = javaInfo.getValue(Measure.JAVA_CLASS_VERSION).value?.let { json.decodeFromString<String?>(it) }
        java["class path"] = javaInfo.getValue(Measure.JAVA_CLASS_PATH).value?.let { json.decodeFromString<List<String>?>(it) }
        java["library path"] = javaInfo.getValue(Measure.JAVA_LIBRARY_PATH).value?.let { json.decodeFromString<List<String>?>(it) }
        java["temporary dir"] = javaInfo.getValue(Measure.JAVA_IO_TMPDIR).value?.let { json.decodeFromString<String?>(it) }
        val javaVm: MutableMap<String, Any?> =
            java.getOrPut("vm") { mutableMapOf<String, Any?>() } as MutableMap<String, Any?>
        javaVm["version"] = javaInfo.getValue(Measure.JAVA_VM_VERSION).value?.let { json.decodeFromString<String?>(it) }
        javaVm["vendor"] = javaInfo.getValue(Measure.JAVA_VM_VENDOR).value?.let { json.decodeFromString<String?>(it) }
        javaVm["name"] = javaInfo.getValue(Measure.JAVA_VM_NAME).value?.let { json.decodeFromString<String?>(it) }
        val javaVmSpecification: MutableMap<String, Any?> =
            javaVm.getOrPut("vm") { mutableMapOf<String, Any?>() } as MutableMap<String, Any?>
        javaVmSpecification["version"] =
            javaInfo.getValue(Measure.JAVA_VM_SPECIFICATION_VERSION).value?.let { json.decodeFromString<String?>(it) }
        javaVmSpecification["vendor"] =
            javaInfo.getValue(Measure.JAVA_VM_SPECIFICATION_VENDOR).value?.let { json.decodeFromString<String?>(it) }
        javaVmSpecification["name"] =
            javaInfo.getValue(Measure.JAVA_VM_SPECIFICATION_NAME).value?.let { json.decodeFromString<String?>(it) }
        val javaSpecification: MutableMap<String, Any?> =
            java.getOrPut("vm") { mutableMapOf<String, Any?>() } as MutableMap<String, Any?>
        javaSpecification["version"] =
            javaInfo.getValue(Measure.JAVA_SPECIFICATION_VERSION).value?.let { json.decodeFromString<String?>(it) }
        javaSpecification["maintenance version"] =
            javaInfo.getValue(Measure.JAVA_SPECIFICATION_MAINTENANCE_VERSION).value?.let { json.decodeFromString<String?>(it) }
        javaSpecification["vendor"] =
            javaInfo.getValue(Measure.JAVA_SPECIFICATION_VENDOR).value?.let { json.decodeFromString<String?>(it) }
        javaSpecification["name"] =
            javaInfo.getValue(Measure.JAVA_SPECIFICATION_NAME).value?.let { json.decodeFromString<String?>(it) }

        // Serialize the updated ir_metadata.
        val stream = exportFilePath.outputStream().let {
            if (exportFilePath.name.endsWith(".gz")) {
                GZIPOutputStream(it)
            } else it
        }
        val writePrefixSuffix = listOf(
            ".ir_metadata",
            ".ir-metadata",
            ".ir_metadata.gz",
            ".ir-metadata.gz",
        ).any { exportFilePath.name.endsWith(it) }
        stream.bufferedWriter().use { writer ->
            if (writePrefixSuffix) {
                writer.write("ir_metadata.start\n")
            }
            yaml.dump(irMetadata, writer)
            if (writePrefixSuffix) {
                writer.write("\n")
                writer.write("ir_metadata.end\n")
            }
        }
    }
}


@JvmOverloads
fun startTracking(
    measures: Iterable<Measure> = ALL_MEASURES,
    pollIntervalMillis: Long = 1000,
    systemName: String? = null,
    systemDescription: String? = null,
    exportFilePath: File? = null,
    exportFormat: ExportFormat? = null,
) = TrackingHandle.start(
    measures = measures,
    pollIntervalMillis = pollIntervalMillis,
    systemName = systemName,
    systemDescription = systemDescription,
    exportFilePath = exportFilePath,
    exportFormat = exportFormat,
)

@Suppress("unused")
@JvmOverloads
fun startTracking(
    measures: Iterable<Measure> = ALL_MEASURES,
    pollIntervalMillis: Long = 1000,
    systemName: String? = null,
    systemDescription: String? = null,
    exportFilePath: Path,
    exportFormat: ExportFormat? = null,
) = startTracking(
    measures = measures,
    pollIntervalMillis = pollIntervalMillis,
    systemName = systemName,
    systemDescription = systemDescription,
    exportFilePath = exportFilePath.toFile(),
    exportFormat = exportFormat,
)


fun stopTracking(trackingHandle: TrackingHandle) = trackingHandle.stop()


@JvmOverloads
fun tracking(
    measures: Iterable<Measure> = ALL_MEASURES,
    pollIntervalMillis: Long = 1000,
    systemName: String? = null,
    systemDescription: String? = null,
    exportFilePath: File? = null,
    exportFormat: ExportFormat? = null,
) = TrackingHandle.start(
    measures = measures,
    pollIntervalMillis = pollIntervalMillis,
    systemName = systemName,
    systemDescription = systemDescription,
    exportFilePath = exportFilePath,
    exportFormat = exportFormat,
)

@Suppress("unused")
@JvmOverloads
fun tracking(
    measures: Iterable<Measure> = ALL_MEASURES,
    pollIntervalMillis: Long = 1000,
    systemName: String? = null,
    systemDescription: String? = null,
    exportFilePath: Path,
    exportFormat: ExportFormat? = null,
) = tracking(
    measures = measures,
    pollIntervalMillis = pollIntervalMillis,
    systemName = systemName,
    systemDescription = systemDescription,
    exportFilePath = exportFilePath.toFile(),
    exportFormat = exportFormat,
)

@JvmOverloads
inline fun track(
    measures: Iterable<Measure> = ALL_MEASURES,
    pollIntervalMillis: Long = 1000,
    systemName: String? = null,
    systemDescription: String? = null,
    exportFilePath: File? = null,
    exportFormat: ExportFormat? = null,
    crossinline block: () -> Unit,
): Map<Measure, ResultEntry> {
    val measurement = startTracking(
        measures = measures,
        pollIntervalMillis = pollIntervalMillis,
        systemName = systemName,
        systemDescription = systemDescription,
        exportFilePath = exportFilePath,
        exportFormat = exportFormat,
    )
    try {
        block()
    } catch (e: Throwable) {
        stopTracking(measurement)
        throw e
    }
    return stopTracking(measurement)
}

@Suppress("unused")
@JvmOverloads
inline fun track(
    measures: Iterable<Measure> = ALL_MEASURES,
    pollIntervalMillis: Long = 1000,
    systemName: String? = null,
    systemDescription: String? = null,
    exportFilePath: Path,
    exportFormat: ExportFormat? = null,
    crossinline block: () -> Unit,
) = track(
    measures = measures,
    pollIntervalMillis = pollIntervalMillis,
    systemName = systemName,
    systemDescription = systemDescription,
    exportFilePath = exportFilePath.toFile(),
    exportFormat = exportFormat,
    block = block,
)

interface BlockCallback {
    fun invoke()
}

@JvmOverloads
fun track(
    measures: Iterable<Measure> = ALL_MEASURES,
    pollIntervalMillis: Long = 1000,
    systemName: String? = null,
    systemDescription: String? = null,
    exportFilePath: File? = null,
    exportFormat: ExportFormat? = null,
    block: BlockCallback,
) = track(
    measures = measures,
    pollIntervalMillis = pollIntervalMillis,
    systemName = systemName,
    systemDescription = systemDescription,
    exportFilePath = exportFilePath,
    exportFormat = exportFormat,
    block = {
        block.invoke()
    },
)

@Suppress("unused")
@JvmOverloads
fun track(
    measures: Iterable<Measure> = ALL_MEASURES,
    pollIntervalMillis: Long = 1000,
    systemName: String? = null,
    systemDescription: String? = null,
    exportFilePath: Path,
    exportFormat: ExportFormat? = null,
    block: BlockCallback,
) = track(
    measures = measures,
    pollIntervalMillis = pollIntervalMillis,
    systemName = systemName,
    systemDescription = systemDescription,
    exportFilePath = exportFilePath.toFile(),
    exportFormat = exportFormat,
    block = block,
)
