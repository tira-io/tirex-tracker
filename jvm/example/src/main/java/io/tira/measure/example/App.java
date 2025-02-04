
package io.tira.measure.example;

import io.tira.measure.*;

import java.util.Map;
import java.util.Set;

import static io.tira.measure.Measures.getProviders;

class App {
    private static void logCallback(LogLevel level, String component, String message) {
        System.out.printf("[%s] [%s] %s%n", level, component, message);
    }

    public static void main(String[] args) {
        System.out.println(getProviders());
        Set<Measure> measures = Set.of(Measure.TIME_ELAPSED_WALL_CLOCK);
        Map<Measure, Object> result = Measures.measure(measures, 100L, App::logCallback, () -> {
            try {
                Thread.sleep(2000);
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
        });
        System.out.println(result);
    }
}
