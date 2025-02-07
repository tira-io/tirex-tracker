package io.tira.measure

import java.lang.Thread.sleep

import kotlin.test.Test
import kotlin.test.assertContains
import kotlin.test.assertIs
import kotlin.test.assertNotNull
import kotlin.test.assertTrue

class MeasuresTest {
    @Test
    fun testProvidersCanBeFetched() {
        val actual = providers

        assertNotNull(actual)
        assertIs<List<Provider>>(actual)
        assertTrue { actual.isNotEmpty() }
    }

    @Test
    fun testMeasureStartAndStop() {
        val ref = startMeasurement(setOf(Measure.TIME_ELAPSED_WALL_CLOCK))
        val actual: Map<Measure, String>
        try {
            sleep(100)
        } finally {
            actual = stopMeasurement(ref)
        }

        assertNotNull(actual)
        assertIs<Map<Measure, String>>(actual)
        assertTrue { actual.isNotEmpty() }
        actual.keys.forEach { assertIs<Measure>(it) }
        actual.values.forEach { assertIs<String>(it) }
        assertContains(actual.keys, Measure.TIME_ELAPSED_WALL_CLOCK)
        val timeElapsed = actual.getValue(Measure.TIME_ELAPSED_WALL_CLOCK).toFloat()
        assertTrue { timeElapsed > 0.0 }
    }

    @Test
    fun testMeasureUsingBlock() {
        val actual = measure(setOf(Measure.TIME_ELAPSED_WALL_CLOCK)) {
            sleep(100)
        }

        assertNotNull(actual)
        assertIs<Map<Measure, String>>(actual)
        assertTrue { actual.isNotEmpty() }
        actual.keys.forEach { assertIs<Measure>(it) }
        actual.values.forEach { assertIs<String>(it) }
        assertContains(actual.keys, Measure.TIME_ELAPSED_WALL_CLOCK)
        val timeElapsed = actual.getValue(Measure.TIME_ELAPSED_WALL_CLOCK).toFloat()
        assertTrue { timeElapsed > 0.0 }
    }

}
