from time import sleep
from typing import Collection, Mapping

from pytest import fixture

from measure import (
    provider_infos,
    measure_infos,
    fetch_info,
    start_measurement,
    stop_measurement,
    measuring,
    measured,
    Measure,
    ResultEntry,
    ResultType,
    ALL_MEASURES,
)

import faulthandler

faulthandler.enable()


def test_provider_infos() -> None:
    actual = provider_infos()

    assert actual is not None
    assert isinstance(actual, Collection)
    assert len(actual) > 0


def test_measure_infos() -> None:
    actual = measure_infos()

    assert actual is not None
    assert isinstance(actual, Mapping)
    assert len(actual) > 0
    for measure in Measure:
        assert measure in actual.keys()


def test_fetch_info() -> None:
    actual = fetch_info([Measure.OS_NAME])

    assert actual is not None
    assert isinstance(actual, Mapping)
    assert len(actual) > 0
    for key in actual.keys():
        assert isinstance(key, Measure)
    for value in actual.values():
        assert isinstance(value, ResultEntry)
    assert Measure.OS_NAME in actual.keys()
    assert actual[Measure.OS_NAME].source is not None
    assert actual[Measure.OS_NAME].source is Measure.OS_NAME
    assert actual[Measure.OS_NAME].type is not None
    assert actual[Measure.OS_NAME].type is ResultType.STRING
    assert actual[Measure.OS_NAME].value is not None
    assert len(actual[Measure.OS_NAME].value) > 0


def test_measure_start_and_stop() -> None:
    ref = start_measurement([Measure.TIME_ELAPSED_WALL_CLOCK_MS])
    try:
        sleep(0.1)
    finally:
        actual = stop_measurement(ref)

    assert actual is not None
    assert isinstance(actual, Mapping)
    assert len(actual) > 0
    for key in actual.keys():
        assert isinstance(key, Measure)
    for value in actual.values():
        assert isinstance(value, ResultEntry)
    assert Measure.TIME_ELAPSED_WALL_CLOCK_MS in actual.keys()
    assert actual[Measure.TIME_ELAPSED_WALL_CLOCK_MS].source is not None
    assert (
        actual[Measure.TIME_ELAPSED_WALL_CLOCK_MS].source
        is Measure.TIME_ELAPSED_WALL_CLOCK_MS
    )
    assert actual[Measure.TIME_ELAPSED_WALL_CLOCK_MS].type is not None
    # FIXME
    # assert actual[Measure.TIME_ELAPSED_WALL_CLOCK_MS].type is ResultType.FLOATING
    assert actual[Measure.TIME_ELAPSED_WALL_CLOCK_MS].value is not None
    time_elapsed = float(actual[Measure.TIME_ELAPSED_WALL_CLOCK_MS].value)
    assert time_elapsed > 0.0


def test_measure_using_with_statement() -> None:
    with measuring([Measure.TIME_ELAPSED_WALL_CLOCK_MS]) as actual:
        sleep(0.1)
    for key in actual.keys():
        assert isinstance(key, Measure)
    for value in actual.values():
        assert isinstance(value, ResultEntry)
    assert Measure.TIME_ELAPSED_WALL_CLOCK_MS in actual.keys()
    assert actual[Measure.TIME_ELAPSED_WALL_CLOCK_MS].source is not None
    assert (
        actual[Measure.TIME_ELAPSED_WALL_CLOCK_MS].source
        is Measure.TIME_ELAPSED_WALL_CLOCK_MS
    )
    assert actual[Measure.TIME_ELAPSED_WALL_CLOCK_MS].type is not None
    # FIXME
    # assert actual[Measure.TIME_ELAPSED_WALL_CLOCK_MS].type is ResultType.FLOATING
    assert actual[Measure.TIME_ELAPSED_WALL_CLOCK_MS].value is not None
    time_elapsed = float(actual[Measure.TIME_ELAPSED_WALL_CLOCK_MS].value)
    assert time_elapsed > 0.0


def test_measure_using_function_decorator() -> None:
    @measured([Measure.TIME_ELAPSED_WALL_CLOCK_MS])
    def sleep_and_measure(time: float) -> None:
        sleep(0.1)

    sleep_and_measure(0.1)

    actual = sleep_and_measure.results

    assert actual is not None
    assert isinstance(actual, Mapping)
    assert len(actual) > 0
    for key in actual.keys():
        assert isinstance(key, Measure)
    for value in actual.values():
        assert isinstance(value, ResultEntry)
    assert Measure.TIME_ELAPSED_WALL_CLOCK_MS in actual.keys()
    assert actual[Measure.TIME_ELAPSED_WALL_CLOCK_MS].source is not None
    assert (
        actual[Measure.TIME_ELAPSED_WALL_CLOCK_MS].source
        is Measure.TIME_ELAPSED_WALL_CLOCK_MS
    )
    assert actual[Measure.TIME_ELAPSED_WALL_CLOCK_MS].type is not None
    # FIXME
    # assert actual[Measure.TIME_ELAPSED_WALL_CLOCK_MS].type is ResultType.FLOATING
    assert actual[Measure.TIME_ELAPSED_WALL_CLOCK_MS].value is not None
    time_elapsed = float(actual[Measure.TIME_ELAPSED_WALL_CLOCK_MS].value)
    assert time_elapsed > 0.0


@fixture(params=list(ALL_MEASURES))
def measure() -> Measure:
    return Measure.TIME_ELAPSED_WALL_CLOCK_MS


# FIXME
# def test_measure_type_matches(measure: Measure) -> None:
#     with measuring(measures=[measure]) as actual:
#         sleep(0.01)

#     assert actual is not None
#     assert isinstance(actual, Mapping)
#     assert len(actual) == 1
#     assert measure in actual.keys()
#     result = actual[measure]
#     assert isinstance(result, ResultEntry)
#     assert result.source is measure
