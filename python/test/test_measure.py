from time import sleep
from typing import Collection, Mapping

from pytest import fixture

from tira_measure import (
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
    for measure in ALL_MEASURES:
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
    result_entry = actual[Measure.OS_NAME]
    assert result_entry is not None
    assert result_entry.source is not None
    assert result_entry.source is Measure.OS_NAME
    assert result_entry.type is not None
    assert result_entry.type is ResultType.STRING
    assert result_entry.value is not None
    assert len(result_entry.value) > 0


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
    result_entry = actual[Measure.TIME_ELAPSED_WALL_CLOCK_MS]
    assert result_entry is not None
    assert result_entry.source is not None
    assert result_entry.source is Measure.TIME_ELAPSED_WALL_CLOCK_MS
    assert result_entry.type is not None
    # FIXME:
    # assert result_entry.type is ResultType.FLOATING
    assert result_entry.value is not None
    time_elapsed = float(result_entry.value)
    assert time_elapsed > 0.0


def test_measure_using_with_statement() -> None:
    with measuring([Measure.TIME_ELAPSED_WALL_CLOCK_MS]) as actual:
        sleep(0.1)
    for key in actual.keys():
        assert isinstance(key, Measure)
    for value in actual.values():
        assert isinstance(value, ResultEntry)
    assert Measure.TIME_ELAPSED_WALL_CLOCK_MS in actual.keys()
    result_entry = actual[Measure.TIME_ELAPSED_WALL_CLOCK_MS]
    assert result_entry is not None
    assert result_entry.source is not None
    assert result_entry.source is Measure.TIME_ELAPSED_WALL_CLOCK_MS
    assert result_entry.type is not None
    # FIXME:
    # assert result_entry.type is ResultType.FLOATING
    assert result_entry.value is not None
    time_elapsed = float(result_entry.value)
    assert time_elapsed > 0.0


def test_measure_using_function_decorator() -> None:
    @measured([Measure.TIME_ELAPSED_WALL_CLOCK_MS])
    def sleep_and_measure(time: float) -> None:
        sleep(0.1)

    sleep_and_measure(0.1)  # type: ignore

    actual = sleep_and_measure.results  # type: ignore

    assert actual is not None
    assert isinstance(actual, Mapping)
    assert len(actual) > 0
    for key in actual.keys():
        assert isinstance(key, Measure)
    for value in actual.values():
        assert isinstance(value, ResultEntry)
    assert Measure.TIME_ELAPSED_WALL_CLOCK_MS in actual.keys()
    result_entry = actual[Measure.TIME_ELAPSED_WALL_CLOCK_MS]
    assert result_entry is not None
    assert result_entry.source is not None
    assert result_entry.source is Measure.TIME_ELAPSED_WALL_CLOCK_MS
    assert result_entry.type is not None
    # FIXME:
    # assert result_entry.type is ResultType.FLOATING
    assert result_entry.value is not None
    time_elapsed = float(result_entry.value)
    assert time_elapsed > 0.0


@fixture(params=list(ALL_MEASURES))
def measure() -> Measure:
    return Measure.TIME_ELAPSED_WALL_CLOCK_MS


# FIXME:
# def test_measure_type_matches(measure: Measure) -> None:
#     with measuring(measures=[measure]) as actual:
#         sleep(0.01)

#     assert actual is not None
#     assert isinstance(actual, Mapping)
#     assert len(actual) == 1
#     assert measure in actual.keys()
#     result_entry = actual[measure]
#     assert isinstance(result_entry, ResultEntry)
#     assert result_entry.source is measure
