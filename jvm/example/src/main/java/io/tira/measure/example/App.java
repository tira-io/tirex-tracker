
package io.tira.measure.example;

import io.tira.measure.*;

import java.util.Map;
import java.util.Set;

class App {
    private static void logCallback(LogLevel level, String component, String message) {
        System.out.printf("[%s] [%s] %s%n", level, component, message);
    }

    public static void main(String[] args) {
        System.out.println(Measures.getProviderInfos());
        Set<Measure> measures = Set.of(Measure.TIME_ELAPSED_WALL_CLOCK_MS);
        Map<Measure, ResultEntry> result = Measures.measure(measures, 100L, App::logCallback, () -> {
            try {
                Thread.sleep(2000);
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
        });
        System.out.println(result);
    }
}
