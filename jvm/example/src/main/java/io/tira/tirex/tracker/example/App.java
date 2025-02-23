
package io.tira.tirex.tracker.example;

import io.tira.tirex.tracker.LogLevel;
import io.tira.tirex.tracker.Measure;
import io.tira.tirex.tracker.ResultEntry;
import io.tira.tirex.tracker.Tracker;

import java.util.Map;
import java.util.Set;

class App {
    private static void logCallback(LogLevel level, String component, String message) {
        System.out.printf("[%s] [%s] %s%n", level, component, message);
    }

    public static void main(String[] args) {
        System.out.println(Tracker.getProviderInfos());
        Set<Measure> measures = Set.of(Measure.TIME_ELAPSED_WALL_CLOCK_MS);
        Map<Measure, ResultEntry> result = Tracker.track(measures, 100L, App::logCallback, () -> {
            try {
                Thread.sleep(2000);
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
        });
        System.out.println(result);
    }
}
