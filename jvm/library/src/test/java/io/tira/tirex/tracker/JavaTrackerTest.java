package io.tira.tirex.tracker;

import org.junit.Test;
import org.junit.Before;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.util.*;

import static io.tira.tirex.tracker.Tracker.*;
import static org.junit.jupiter.api.Assertions.*;

public class JavaTrackerTest {
    @Before
    public void init() {
        setLogCallback((LogLevel level, String component, String message) -> {
            System.out.println(String.format("[%s][%s] %s", level.name(), component, message));
        });
    }

    @Test
    public void testProviderInfos() {
        Collection<ProviderInfo> actual = getProviderInfos();

        assertNotNull(actual);
        assertInstanceOf(Collection.class, actual);
        assertFalse(actual.isEmpty());
    }

    @Test
    public void testMeasureInfos() {
        Map<Measure, MeasureInfo> actual = getMeasureInfos();

        assertNotNull(actual);
        assertInstanceOf(Map.class, actual);
        assertFalse(actual.isEmpty());
        for (Measure measure : ALL_MEASURES) {
            assertTrue(actual.containsKey(measure));
        }
    }

    @Test
    public void testFetchInfo() {
        Map<Measure, ResultEntry> actual = fetchInfo(Collections.singleton(Measure.OS_NAME));

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
    public void testMeasureStartAndStop() {
        TrackingHandle handle = startTracking(Collections.singleton(Measure.TIME_ELAPSED_WALL_CLOCK_MS));
        Map<Measure, ResultEntry> actual;
        try {
            try {
                Thread.sleep(100);
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
        } finally {
            actual = stopTracking(handle);
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
        // FIXME:
        // assertEquals(ResultType.FLOATING, resultEntry.getType());
        assertNotNull(resultEntry.getValue());
        assertFalse(resultEntry.getValue().isEmpty());
        float timeElapsed = Float.parseFloat(resultEntry.getValue());
        assertTrue(timeElapsed > 0.0);
    }

    @Test
    public void testMeasureUsingTryWithResources() {
        Map<Measure, ResultEntry> actual;

        try (TrackingHandle tracked = TrackingHandle.start(Collections.singleton(Measure.TIME_ELAPSED_WALL_CLOCK_MS))) {
            Thread.sleep(100);

            actual = tracked.getResults();
        } catch (InterruptedException e) {
            throw new RuntimeException(e);
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
        // FIXME:
        // assertEquals(ResultType.FLOATING, resultEntry.getType());
        assertNotNull(resultEntry.getValue());
        assertFalse(resultEntry.getValue().isEmpty());
        float timeElapsed = Float.parseFloat(resultEntry.getValue());
        assertTrue(timeElapsed > 0.0);
    }

    @Test
    public void testMeasureUsingBlock() {
        // Note: Using an anonymous class here to make it compatible with earlier Java
        // versions.
        // noinspection Convert2Lambda
        Map<Measure, ResultEntry> actual = track(Collections.singleton(Measure.TIME_ELAPSED_WALL_CLOCK_MS),
                new BlockCallback() {
                    @Override
                    public void invoke() {
                        try {
                            Thread.sleep(100);
                        } catch (InterruptedException e) {
                            throw new RuntimeException(e);
                        }
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
        // FIXME:
        // assertEquals(ResultType.FLOATING, resultEntry.getType());
        assertNotNull(resultEntry.getValue());
        assertFalse(resultEntry.getValue().isEmpty());
        float timeElapsed = Float.parseFloat(resultEntry.getValue());
        assertTrue(timeElapsed > 0.0);
    }

    @Test
    public void testMeasureExportIrMetadata() {
        File tmpDir;
        try {
            tmpDir = Files.createTempDirectory(null).toFile();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }

        assertTrue(tmpDir.exists());
        assertTrue(tmpDir.isDirectory());

        File exportFilePath = new File(tmpDir, ".ir_metadata");
        assertFalse(exportFilePath.exists());

        Map<Measure, ResultEntry> actual = track(
                ALL_MEASURES,
                100L,
                "Test",
                "A description of Test.",
                exportFilePath,
                ExportFormat.IR_METADATA,
                () -> {
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
        // FIXME:
        // assertEquals(ResultType.FLOATING, resultEntry.getType());
        assertNotNull(resultEntry.getValue());
        assertFalse(resultEntry.getValue().isEmpty());
        float timeElapsed = Float.parseFloat(resultEntry.getValue());
        assertTrue(timeElapsed > 0.0);

        assertTrue(exportFilePath.exists());
        assertTrue(exportFilePath.isFile());
        assertTrue(exportFilePath.length() > 0L);

        // noinspection ResultOfMethodCallIgnored
        exportFilePath.delete();
        // noinspection ResultOfMethodCallIgnored
        tmpDir.delete();
    }
}
