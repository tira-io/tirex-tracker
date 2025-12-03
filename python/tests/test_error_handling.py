import os
import unittest
from ctypes import pointer
from threading import Condition, Thread

from tirex_tracker import (
    _LIBRARY,
    _NULL_MEASURE_CONFIGURATION,
    Aggregation,
    LogLevel,
    _MeasureConfiguration,
    _Result,
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
            latch_started = Condition()
            latch_stopped = Condition()
            print("Parent:", os.getpid())

            def do_work():
                latch_started.wait()
                print("Child:", os.getpid())
                measures = [_MeasureConfiguration(-7, Aggregation.NO.value), _NULL_MEASURE_CONFIGURATION]

                configs = (_MeasureConfiguration * (len(measures)))(*measures)
                resultptr = pointer(_Result())
                _LIBRARY.tirexFetchInfo(configs, pointer(resultptr))
                print(resultptr.contents)
                latch_stopped.notify_all()

            latch_started.notify_all()
            Thread(target=do_work, daemon=True).start()
            latch_stopped.wait()

        self.assertRaises(RuntimeError, should_raise)
        deinit_error_handling(handle)
