"""
A short note on how we perform error handling.
What we want to achieve: The following code should abort execution of do_something() if an error occurs during the
tracking:

```py
with tracking():
    # do something
```

This is difficult since all tracking happens in a separate thread. This means that we can not simply throw an exception
as this exception would only be handled in the local thread. The solution is that the error callback from TIREx (which
is run within the tracking thread when an error occurs) signals the monitored thread. This interrupts do_something()
asynchronously. We must have added a signal handler before do_something() runs which in turn throws an exception. That
is, for error handling, the following steps are necessary:
1. Instate signal handler (__instate_abort_signal_handler)
2. Set the abort callback to signal the tracked thread (__instate_abort_callback)
3. Start tracking
4. Perform the work...
5. Deinitialize the signal handler and callback (deinit_error_handling)

Step 1 & 2 are combined in init_error_handling.
"""

import os
import signal
from ctypes import CFUNCTYPE, c_char_p
from types import FrameType
from typing import Any, List, Optional, Tuple, Union

from .constants import ENCODING
from .library import LIBRARY

__SIGNALS_TO_TRY = [signal.Signals.SIGUSR1, signal.Signals.SIGUSR2]
ABORT_HANDLE = Tuple[signal.Signals, Any]


def __instate_abort_signal_handler(handler: "signal._HANDLER") -> Optional[signal.Signals]:
    for sig in __SIGNALS_TO_TRY:
        if signal.getsignal(sig) not in (signal.SIG_IGN, signal.SIG_DFL, None):
            continue  # Signal already in use by someone else
        signal.signal(sig, handler)
        return sig
    return None


def __instate_abort_callback(sig: signal.Signals, strref: List[str]) -> Any:
    pid = os.getpid()

    @CFUNCTYPE(None, c_char_p)
    def __abort_callback(message: bytes) -> None:
        strref[0] = message.decode(ENCODING)
        # os.kill does seem weird here but it actually only sends a signal instead of killing the thread:
        # https://docs.python.org/3/library/os.html#os.kill: "Send signal sig to process pid".
        os.kill(pid, sig)

    LIBRARY.tirexSetAbortCallback(__abort_callback)
    # We need to return __abort_callback here since we need to keep it referenced as long as it is set as the abort
    # callback.
    return __abort_callback


def init_error_handling() -> ABORT_HANDLE:
    strref = [""]

    def sig_handler(signum: int, frame: Union[FrameType, None]) -> None:
        raise RuntimeError(f"TIREx Tracker encountered a problem: {strref[0]}")

    sig = __instate_abort_signal_handler(sig_handler)
    if sig is None:
        raise RuntimeError(
            f"Failed to instantiate a signal handler, all tested signals ({__SIGNALS_TO_TRY}) are already in use"
        )
    return sig, __instate_abort_callback(sig, strref)


def deinit_error_handling(handle: ABORT_HANDLE):
    sig, _ = handle
    signal.signal(sig, signal.SIG_DFL)
    LIBRARY.tirexSetAbortCallback(None)
