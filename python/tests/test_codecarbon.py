from faulthandler import enable as faulthandler_enable
from json import loads
from time import sleep
from typing import Mapping

from pytest import skip

from tirex_tracker import (
    Measure,
    ResultType,
    set_log_callback,
    start_tracking,
    stop_tracking,
    track,
    tracked,
    tracking,
)
from tirex_tracker._compat import BaseEmissionsTracker

faulthandler_enable()

set_log_callback(lambda level, component, message: print(f"[{level}][{component}] {message}"))


def test_codecarbon_start_and_stop() -> None:
    # Skip if CodeCarbon is not available.
    if BaseEmissionsTracker is NotImplemented:
        skip("CodeCarbon is not available.")

    ref = start_tracking([Measure.CODECARBON_EMISSIONS])
    try:
        sleep(0.1)
    finally:
        actual = stop_tracking(ref)

    assert actual is not None
    assert isinstance(actual, Mapping)
    assert Measure.CODECARBON_EMISSIONS in actual
    result_entry = actual[Measure.CODECARBON_EMISSIONS]
    assert result_entry is not None
    assert result_entry.source is not None
    assert result_entry.source is Measure.CODECARBON_EMISSIONS
    assert result_entry.type is not None
    assert result_entry.type is ResultType.FLOATING
    assert result_entry.value is not None
    emissions = loads(result_entry.value)
    assert emissions > 0.0


def test_codecarbon_using_with_statement() -> None:
    # Skip if CodeCarbon is not available.
    if BaseEmissionsTracker is NotImplemented:
        skip("CodeCarbon is not available.")

    with tracking([Measure.CODECARBON_EMISSIONS]) as actual:
        sleep(0.1)

    assert actual is not None
    assert isinstance(actual, Mapping)
    assert Measure.CODECARBON_EMISSIONS in actual
    result_entry = actual[Measure.CODECARBON_EMISSIONS]
    assert result_entry is not None
    assert result_entry.source is not None
    assert result_entry.source is Measure.CODECARBON_EMISSIONS
    assert result_entry.type is not None
    assert result_entry.type is ResultType.FLOATING
    assert result_entry.value is not None
    emissions = loads(result_entry.value)
    assert emissions > 0.0


def test_codecarbon_using_lambda() -> None:
    # Skip if CodeCarbon is not available.
    if BaseEmissionsTracker is NotImplemented:
        skip("CodeCarbon is not available.")

    actual = track(lambda: sleep(0.1), [Measure.CODECARBON_EMISSIONS])

    assert actual is not None
    assert isinstance(actual, Mapping)
    assert Measure.CODECARBON_EMISSIONS in actual
    result_entry = actual[Measure.CODECARBON_EMISSIONS]
    assert result_entry is not None
    assert result_entry.source is not None
    assert result_entry.source is Measure.CODECARBON_EMISSIONS
    assert result_entry.type is not None
    assert result_entry.type is ResultType.FLOATING
    assert result_entry.value is not None
    emissions = loads(result_entry.value)
    assert emissions > 0.0


def test_codecarbon_using_function_decorator() -> None:
    # Skip if CodeCarbon is not available.
    if BaseEmissionsTracker is NotImplemented:
        skip("CodeCarbon is not available.")

    @tracked([Measure.CODECARBON_EMISSIONS])
    def sleep_and_measure(time: float) -> None:
        sleep(0.1)

    sleep_and_measure(0.1)  # type: ignore

    actual = sleep_and_measure.results  # type: ignore

    assert actual is not None
    assert isinstance(actual, Mapping)
    assert Measure.CODECARBON_EMISSIONS in actual
    result_entry = actual[Measure.CODECARBON_EMISSIONS]
    assert result_entry is not None
    assert result_entry.source is not None
    assert result_entry.source is Measure.CODECARBON_EMISSIONS
    assert result_entry.type is not None
    assert result_entry.type is ResultType.FLOATING
    assert result_entry.value is not None
    emissions = loads(result_entry.value)
    assert emissions > 0.0
