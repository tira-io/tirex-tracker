from __future__ import annotations

import contextlib
import os


@contextlib.contextmanager
def ChangeWorkingDir(path: os.PathLike):
    oldworkdir = os.getcwd()
    os.chdir(os.path.abspath(path))
    try:
        yield
    finally:
        os.chdir(oldworkdir)
