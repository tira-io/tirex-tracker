from json import loads
from subprocess import Popen  # nosec
from sys import executable
from typing import Mapping

from tirex_tracker import Measure, ResultEntry, start_tracking, stop_tracking

# Busy-loops (as opposed to sleeping) for ~0.6s in a separate interpreter, to register as real CPU time in a
# process that is not this test process itself.
_BUSY_LOOP_SCRIPT = """
import time
deadline = time.monotonic() + 0.6
while time.monotonic() < deadline:
    pass
"""


def _max_value(results: Mapping[Measure, ResultEntry], measure: Measure) -> float:
    return loads(results[measure].value)["max"]


def test_start_tracking_targets_explicit_pid_not_the_caller() -> None:
    process = Popen([executable, "-c", _BUSY_LOOP_SCRIPT])  # nosec
    try:
        handle = start_tracking(
            measures=[Measure.CPU_USED_PROCESS_PERCENT],
            poll_intervall_ms=50,
            pid=process.pid,
            track_subprocesses=False,
        )
        process.wait()
        results = stop_tracking(handle)
    finally:
        if process.poll() is None:
            process.kill()
            process.wait()

    max_cpu_percent = _max_value(results, Measure.CPU_USED_PROCESS_PERCENT)
    assert max_cpu_percent > 50.0


def test_track_subprocesses_aggregates_a_childs_cpu_usage() -> None:
    process = Popen([executable, "-c", _BUSY_LOOP_SCRIPT])  # nosec
    try:
        handle = start_tracking(
            measures=[Measure.CPU_USED_PROCESS_PERCENT],
            poll_intervall_ms=50,
            track_subprocesses=True,
        )
        process.wait()
        results = stop_tracking(handle)
    finally:
        if process.poll() is None:
            process.kill()
            process.wait()

    max_cpu_percent = _max_value(results, Measure.CPU_USED_PROCESS_PERCENT)
    assert max_cpu_percent > 50.0


def test_track_subprocesses_disabled_does_not_include_a_childs_cpu_usage() -> None:
    process = Popen([executable, "-c", _BUSY_LOOP_SCRIPT])  # nosec
    try:
        handle = start_tracking(
            measures=[Measure.CPU_USED_PROCESS_PERCENT],
            poll_intervall_ms=50,
            track_subprocesses=False,
        )
        process.wait()
        results = stop_tracking(handle)
    finally:
        if process.poll() is None:
            process.kill()
            process.wait()

    max_cpu_percent = _max_value(results, Measure.CPU_USED_PROCESS_PERCENT)
    assert max_cpu_percent < 50.0
