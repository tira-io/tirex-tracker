package io.tira.tirex.tracker

import org.junit.Test
import kotlin.io.path.ExperimentalPathApi
import kotlin.io.path.createTempDirectory
import kotlin.io.path.deleteRecursively
import kotlin.io.path.exists
import kotlin.io.path.fileSize
import kotlin.io.path.isDirectory
import kotlin.io.path.isRegularFile
import kotlin.test.*

class TrackerTest {
    @Test
    fun testProviderInfos() {
        val actual = providerInfos

        assertNotNull(actual)
        assertIs<Collection<ProviderInfo>>(actual)
        assertTrue { actual.isNotEmpty() }
    }

    @Test
    fun testMeasureInfos() {
        val actual = measureInfos

        assertNotNull(actual)
        assertIs<Map<Measure, MeasureInfo>>(actual)
        assertTrue { actual.isNotEmpty() }
        for (measure in ALL_MEASURES) {
            assertContains(actual, measure)
        }
    }

    @Test
    fun testFetchInfo() {
        val actual = fetchInfo(setOf(Measure.OS_NAME))

        assertNotNull(actual)
        assertIs<Map<Measure, ResultEntry>>(actual)
        assertTrue { actual.isNotEmpty() }
        for (key in actual.keys) {
            assertIs<Measure>(key)
        }
        for (value in actual.values) {
            assertIs<ResultEntry>(value)
        }
        assertContains(actual.keys, Measure.OS_NAME)
        val resultEntry = actual[Measure.OS_NAME]
        assertNotNull(resultEntry)
        assertNotNull(resultEntry.source)
        assertEquals(resultEntry.source, Measure.OS_NAME)
        assertNotNull(resultEntry.type)
        assertEquals(resultEntry.type, ResultType.STRING)
        assertNotNull(resultEntry.value)
        assertTrue { resultEntry.value.isNotEmpty() }
    }

    @Test
    fun testMeasureStartAndStop() {
        val ref = startTracking(setOf(Measure.TIME_ELAPSED_WALL_CLOCK_MS))
        val actual: Map<Measure, ResultEntry>
        try {
            Thread.sleep(100)
        } finally {
            actual = stopTracking(ref)
        }

        assertNotNull(actual)
        assertIs<Map<Measure, ResultEntry>>(actual)
        assertTrue { actual.isNotEmpty() }
        for (key in actual.keys) {
            assertIs<Measure>(key)
        }
        for (value in actual.values) {
            assertIs<ResultEntry>(value)
        }
        assertContains(actual.keys, Measure.TIME_ELAPSED_WALL_CLOCK_MS)
        val resultEntry = actual[Measure.TIME_ELAPSED_WALL_CLOCK_MS]
        assertNotNull(resultEntry)
        assertNotNull(resultEntry.source)
        assertEquals(resultEntry.source, Measure.TIME_ELAPSED_WALL_CLOCK_MS)
        assertNotNull(resultEntry.type)
        //FIXME:
        //assertEquals(resultEntry.type, ResultType.FLOATING)
        assertNotNull(resultEntry.value)
        val timeElapsed = resultEntry.value.toFloat()
        assertTrue { timeElapsed > 0.0 }
    }

    @Test
    fun testMeasureUsingUseStatement() {
        val tracked = TrackingHandle.start(setOf(Measure.TIME_ELAPSED_WALL_CLOCK_MS))
        tracked.use {
            Thread.sleep(100)
        }

        val actual = tracked.results

        assertNotNull(actual)
        assertIs<Map<Measure, ResultEntry>>(actual)
        assertTrue { actual.isNotEmpty() }
        for (key in actual.keys) {
            assertIs<Measure>(key)
        }
        for (value in actual.values) {
            assertIs<ResultEntry>(value)
        }
        assertContains(actual.keys, Measure.TIME_ELAPSED_WALL_CLOCK_MS)
        val resultEntry = actual[Measure.TIME_ELAPSED_WALL_CLOCK_MS]
        assertNotNull(resultEntry)
        assertNotNull(resultEntry.source)
        assertEquals(resultEntry.source, Measure.TIME_ELAPSED_WALL_CLOCK_MS)
        assertNotNull(resultEntry.type)
        //FIXME:
        //assertEquals(resultEntry.type, ResultType.FLOATING)
        assertNotNull(resultEntry.value)
        val timeElapsed = resultEntry.value.toFloat()
        assertTrue { timeElapsed > 0.0 }
    }

    @Test
    fun testMeasureUsingBlock() {
        val actual = track(setOf(Measure.TIME_ELAPSED_WALL_CLOCK_MS)) {
            Thread.sleep(100)
        }

        assertNotNull(actual)
        assertIs<Map<Measure, ResultEntry>>(actual)
        assertTrue { actual.isNotEmpty() }
        for (key in actual.keys) {
            assertIs<Measure>(key)
        }
        for (value in actual.values) {
            assertIs<ResultEntry>(value)
        }
        assertContains(actual.keys, Measure.TIME_ELAPSED_WALL_CLOCK_MS)
        val resultEntry = actual[Measure.TIME_ELAPSED_WALL_CLOCK_MS]
        assertNotNull(resultEntry)
        assertNotNull(resultEntry.source)
        assertEquals(resultEntry.source, Measure.TIME_ELAPSED_WALL_CLOCK_MS)
        assertNotNull(resultEntry.type)
        //FIXME:
        //assertEquals(resultEntry.type, ResultType.FLOATING)
        assertNotNull(resultEntry.value)
        val timeElapsed = resultEntry.value.toFloat()
        assertTrue { timeElapsed > 0.0 }
    }

    @OptIn(ExperimentalPathApi::class)
    @Test
    fun testMeasureExportIrMetadata() {
        val tmpDir = createTempDirectory()
        assertTrue { tmpDir.exists() }
        assertTrue { tmpDir.isDirectory() }

        val exportFilePath = tmpDir.resolve(".ir_metadata")
        assertFalse { exportFilePath.exists() }

        val actual = track(
            exportFilePath = exportFilePath.toFile(),
            exportFormat = ExportFormat.IR_METADATA,
        ) {
            Thread.sleep(100)
        }

        assertNotNull(actual)
        assertIs<Map<Measure, ResultEntry>>(actual)
        assertTrue { actual.isNotEmpty() }
        for (key in actual.keys) {
            assertIs<Measure>(key)
        }
        for (value in actual.values) {
            assertIs<ResultEntry>(value)
        }
        assertContains(actual.keys, Measure.TIME_ELAPSED_WALL_CLOCK_MS)
        val resultEntry = actual[Measure.TIME_ELAPSED_WALL_CLOCK_MS]
        assertNotNull(resultEntry)
        assertNotNull(resultEntry.source)
        assertEquals(resultEntry.source, Measure.TIME_ELAPSED_WALL_CLOCK_MS)
        assertNotNull(resultEntry.type)
        //FIXME:
        //assertEquals(resultEntry.type, ResultType.FLOATING)
        assertNotNull(resultEntry.value)
        val timeElapsed = resultEntry.value.toFloat()
        assertTrue { timeElapsed > 0.0 }

        assertTrue { exportFilePath.exists() }
        assertTrue { exportFilePath.isRegularFile() }
        assertTrue { exportFilePath.fileSize() > 0L }

        tmpDir.deleteRecursively()
    }

    // TODO: Add test to check that the result type matches the measure info's data type.

    // TODO: Add test to check that exactly and only the requested measures are returned.
}

