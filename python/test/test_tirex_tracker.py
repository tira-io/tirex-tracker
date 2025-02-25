from pathlib import Path
from tempfile import TemporaryDirectory
from time import sleep
from typing import Collection, Mapping

from tirex_tracker import (
    provider_infos,
    measure_infos,
    fetch_info,
    start_tracking,
    stop_tracking,
    tracking,
    tracked,
    track,
    Measure,
    ResultEntry,
    ResultType,
    ALL_MEASURES,
    ExportFormat,
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
    ref = start_tracking([Measure.TIME_ELAPSED_WALL_CLOCK_MS])
    try:
        sleep(0.1)
    finally:
        actual = stop_tracking(ref)

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
    with tracking([Measure.TIME_ELAPSED_WALL_CLOCK_MS]) as actual:
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


def test_measure_using_lambda() -> None:
    actual = track(lambda: sleep(0.1), [Measure.TIME_ELAPSED_WALL_CLOCK_MS])

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
    @tracked([Measure.TIME_ELAPSED_WALL_CLOCK_MS])
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


def test_tracking_export_ir_metadata() -> None:
    with TemporaryDirectory() as tmp_dir:
        tmp_dir_path = Path(tmp_dir)
        assert tmp_dir_path.exists()
        assert tmp_dir_path.is_dir()

        export_file_path = tmp_dir_path / ".ir_metadata"
        assert not export_file_path.exists()

        with tracking(
            export_file_path=export_file_path,
            export_format=ExportFormat.IR_METADATA,
        ) as actual:
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

        assert export_file_path.exists()
        assert export_file_path.is_file()
        assert export_file_path.stat().st_size > 0


# TODO: Add test to check that the result type matches the measure info's data type.

# TODO: Add test to check that exactly and only the requested measures are returned.
