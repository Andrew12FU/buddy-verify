"""固定结果目录：成功替换当前版本，失败恢复，日常运行不累计快照。"""

from contextlib import contextmanager
import fcntl
import json
from pathlib import Path
import shutil


@contextmanager
def output_directory(path: Path, *, replace: bool):
    """Prepare a run directory and keep only one verified result afterwards.

    Args:
        path: Fixed managed slot, or an explicitly selected diagnostic directory.
        replace: True only for the built-in current slot. Explicit output paths
            must be new/empty and retain their diagnostics, including failures.
    Yields:
        None. The runner writes directly to path so stored absolute paths stay
        correct. A status of converted commits the candidate; every other exit
        restores the previous result. A temporary sibling backup exists only
        during the run, not as another stored version.
    Raises:
        OSError/ValueError: Busy slot, interrupted backup, unmanaged contents or
            unsafe paths. Never delete arbitrary explicit output directories.
    """
    if path.is_symlink() or (path.exists() and not path.is_dir()):
        raise ValueError(f"Output must be a real directory: {path}")
    path.parent.mkdir(parents=True, exist_ok=True)
    if not replace:
        if path.exists() and any(path.iterdir()):
            raise ValueError("Explicit output directory must be new or empty")
        path.mkdir(exist_ok=True)
        yield
        return

    backup = path.with_name("." + path.name + ".previous")
    with path.with_name("." + path.name + ".lock").open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        if backup.exists() or backup.is_symlink():
            raise ValueError(f"An interrupted run needs recovery from {backup}; refusing to overwrite it")
        if path.exists() and any(path.iterdir()):
            summary = path / "summary.json"
            if not summary.is_file() or json.loads(summary.read_text()).get("status") != "converted":
                raise ValueError(f"Refusing to replace an unmanaged/unverified result: {path}")
        existed = path.exists()
        if existed:
            path.rename(backup)
        committed = False
        try:
            path.mkdir()
            yield
            summary = path / "summary.json"
            committed = summary.is_file() and json.loads(summary.read_text()).get("status") == "converted"
        finally:
            if not committed:
                if path.exists():
                    shutil.rmtree(path)
                if existed:
                    backup.rename(path)
            elif existed:
                shutil.rmtree(backup)
