package io.tira.tirex.tracker;

import com.sun.jna.Pointer;
import org.junit.jupiter.api.Test;

import java.util.*;

import static io.tira.tirex.tracker.Tracker.*;
import static org.junit.jupiter.api.Assertions.*;

public class JavaTrackerTest {
    @Test
    void testProviderInfos() {
         Collection< ProviderInfo> actual = getProviderInfos();

        assertNotNull(actual);
        assertInstanceOf(Collection.class, actual);
        assertFalse(actual.isEmpty());
    }

    @Test
    void testMeasureInfos() {
        Map<Measure, MeasureInfo> actual = getMeasureInfos();

        assertNotNull(actual);
        assertInstanceOf(Map.class, actual);
        assertFalse(actual.isEmpty());
        for (Measure measure : ALL_MEASURES) {
            assertTrue(actual.containsKey(measure));
        }
    }

    @Test
    void testFetchInfo() {
        Map< Measure,  ResultEntry> actual = fetchInfo(Collections.singleton(Measure.OS_NAME));

        assertNotNull(actual);
        assertInstanceOf(Map.class, actual);
        assertFalse(actual.isEmpty());
        for (Measure key : actual.keySet()) {
            assertInstanceOf(Measure.class, key);
        }
        for (ResultEntry value : actual.values()) {
            assertInstanceOf(ResultEntry.class, value);
        }
        assertTrue(actual.containsKey(Measure.OS_NAME));
        ResultEntry resultEntry = actual.get(Measure.OS_NAME);
        assertNotNull(resultEntry);
        assertNotNull(resultEntry.getSource());
        assertEquals(Measure.OS_NAME, resultEntry.getSource());
        assertNotNull(resultEntry.getType());
        assertEquals(ResultType.STRING, resultEntry.getType());
        assertNotNull(resultEntry.getValue());
        assertFalse(resultEntry.getValue().isEmpty());
    }

    @Test
    void testMeasureStartAndStop() {
        Pointer ref = startTracking(Collections.singleton(Measure.TIME_ELAPSED_WALL_CLOCK_MS));
        Map< Measure,  ResultEntry>  actual;
        try {
            try {
                Thread.sleep(100);
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
        } finally {
            actual = stopTracking(ref);
        }

        assertNotNull(actual);
        assertInstanceOf(Map.class, actual);
        assertFalse(actual.isEmpty());
        for (Measure key : actual.keySet()) {
            assertInstanceOf(Measure.class, key);
        }
        for (ResultEntry value : actual.values()) {
            assertInstanceOf(ResultEntry.class, value);
        }
        assertTrue(actual.containsKey(Measure.TIME_ELAPSED_WALL_CLOCK_MS));
        ResultEntry resultEntry = actual.get(Measure.TIME_ELAPSED_WALL_CLOCK_MS);
        assertNotNull(resultEntry);
        assertNotNull(resultEntry.getSource());
        assertEquals(Measure.TIME_ELAPSED_WALL_CLOCK_MS, resultEntry.getSource());
        assertNotNull(resultEntry.getType());
        //FIXME:
        //assertEquals(ResultType.FLOATING, resultEntry.getType());
        assertNotNull(resultEntry.getValue());
        assertFalse(resultEntry.getValue().isEmpty());
        float timeElapsed = Float.parseFloat(resultEntry.getValue());
        assertTrue(timeElapsed > 0.0);
    }

    @Test
    void testMeasureUsingBlock() {
        Map< Measure,  ResultEntry> actual = track(Collections.singleton(Measure.TIME_ELAPSED_WALL_CLOCK_MS), () -> {
            try {
                Thread.sleep(100);
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
        });

        assertNotNull(actual);
        assertInstanceOf(Map.class, actual);
        assertFalse(actual.isEmpty());
        for (Measure key : actual.keySet()) {
            assertInstanceOf(Measure.class, key);
        }
        for (ResultEntry value : actual.values()) {
            assertInstanceOf(ResultEntry.class, value);
        }
        assertTrue(actual.containsKey(Measure.TIME_ELAPSED_WALL_CLOCK_MS));
        ResultEntry resultEntry = actual.get(Measure.TIME_ELAPSED_WALL_CLOCK_MS);
        assertNotNull(resultEntry);
        assertNotNull(resultEntry.getSource());
        assertEquals(Measure.TIME_ELAPSED_WALL_CLOCK_MS, resultEntry.getSource());
        assertNotNull(resultEntry.getType());
        //FIXME:
        //assertEquals(ResultType.FLOATING, resultEntry.getType());
        assertNotNull(resultEntry.getValue());
        assertFalse(resultEntry.getValue().isEmpty());
        float timeElapsed = Float.parseFloat(resultEntry.getValue());
        assertTrue(timeElapsed > 0.0);
    }

    @Test
    void testMeasureUsingTryWithResources() {
        Tracked tracked = new Tracked(Collections.singleton(Measure.TIME_ELAPSED_WALL_CLOCK_MS));
        try (tracked) {
            Thread.sleep(100);
        } catch (InterruptedException e) {
            throw new RuntimeException(e);
        }

        Map< Measure,  ResultEntry> actual = tracked.getResults();

        assertNotNull(actual);
        assertInstanceOf(Map.class, actual);
        assertFalse(actual.isEmpty());
        for (Measure key : actual.keySet()) {
            assertInstanceOf(Measure.class, key);
        }
        for (ResultEntry value : actual.values()) {
            assertInstanceOf(ResultEntry.class, value);
        }
        assertTrue(actual.containsKey(Measure.TIME_ELAPSED_WALL_CLOCK_MS));
        ResultEntry resultEntry = actual.get(Measure.TIME_ELAPSED_WALL_CLOCK_MS);
        assertNotNull(resultEntry);
        assertNotNull(resultEntry.getSource());
        assertEquals(Measure.TIME_ELAPSED_WALL_CLOCK_MS, resultEntry.getSource());
        assertNotNull(resultEntry.getType());
        //FIXME:
        //assertEquals(ResultType.FLOATING, resultEntry.getType());
        assertNotNull(resultEntry.getValue());
        assertFalse(resultEntry.getValue().isEmpty());
        float timeElapsed = Float.parseFloat(resultEntry.getValue());
        assertTrue(timeElapsed > 0.0);
    }
}
