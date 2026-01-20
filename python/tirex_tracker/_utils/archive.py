from __future__ import annotations

import shutil
import tempfile
import zipfile
from contextlib import redirect_stdout
from pathlib import Path
from typing import Optional

from IPython import get_ipython


def add_python_notebook_to_archive(archive: zipfile.ZipFile) -> Optional[tuple[str, str]]:
    ipython = get_ipython()
    if ipython is None:
        # Do nothing when not in a notebook session
        return None
    with tempfile.TemporaryDirectory() as tmpdir:
        notebook_file = Path(tmpdir) / "notebook.ipynb"
        script_file = Path(tmpdir) / "script.py"
        with redirect_stdout(None):
            ipython.run_line_magic("save", f"-f {script_file} 1-9999")
            ipython.run_line_magic("notebook", str(notebook_file))
        with archive.open("notebook.ipynb", mode="w") as dst, notebook_file.open(mode="rb") as src:
            shutil.copyfileobj(src, dst)
        with archive.open("script.py", mode="w") as dst, script_file.open(mode="rb") as src:
            shutil.copyfileobj(src, dst)
        return ("notebook.ipynb", "script.py")
