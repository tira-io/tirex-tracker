import os
import unittest
from ctypes import pointer
from threading import Event, Thread

from tirex_tracker import (
    _LIBRARY,
    _NULL_MEASURE_CONFIGURATION,
    _TIREX_PID_SELF,
    Aggregation,
    LogLevel,
    _MeasureConfiguration,
    _Result,
    _TrackingConf,
    set_log_callback,
)
from tirex_tracker._utils.errorhandling import deinit_error_handling, init_error_handling

set_log_callback(lambda level, component, message: print(f"[{level}][{component}] {message}"))


class TestErrorHandling(unittest.TestCase):
    def test_error_thrown(self):
        _LIBRARY.tirexSetAbortLevel(LogLevel.WARN)
        handle = init_error_handling()
        set_log_callback(lambda level, component, message: print(f"[{level}][{component}] {message}"))

        def should_raise():
            latch_stopped = Event()
            print("Parent:", os.getpid())

            def do_work():
                print("Child:", os.getpid())
                measures = [_MeasureConfiguration(-7, Aggregation.NO.value), _NULL_MEASURE_CONFIGURATION]

                configs = (_MeasureConfiguration * (len(measures)))(*measures)
                tracking_conf = _TrackingConf(
                    measures=configs,
                    pid=_TIREX_PID_SELF,
                    trackSubprocesses=False,
                    pollIntervalMs=0,
                )
                resultptr = pointer(_Result())
                _LIBRARY.tirexFetchInfo(tracking_conf, pointer(resultptr))
                print(resultptr.contents)
                latch_stopped.set()

            Thread(target=do_work, daemon=True).start()
            latch_stopped.wait()

        self.assertRaises(RuntimeError, should_raise)
        deinit_error_handling(handle)
