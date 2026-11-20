"""Repository-relative data paths and explicit/discovered external tool defaults."""
import os
from pathlib import Path
import shutil
import subprocess

FRONTEND_ROOT = Path(__file__).resolve().parents[1]
REPO = FRONTEND_ROOT.parents[1]
WORKSPACE = REPO
MINI_ROOT = FRONTEND_ROOT.parent / "MiniMLIR"

def llvm_prefix():
    if os.environ.get("LLVM_PREFIX"):
        return Path(os.environ["LLVM_PREFIX"]).expanduser()
    for name in ("llvm-config-18", "llvm-config"):
        executable = shutil.which(name)
        if executable:
            result = subprocess.run([executable, "--prefix"], text=True, capture_output=True)
            if result.returncode == 0:
                return Path(result.stdout.strip())
    return Path("/usr")

def executable_path(variable, name):
    value = os.environ.get(variable) or shutil.which(name)
    return Path(value).expanduser() if value else Path(name)

FRONTEND_LLVM = llvm_prefix()
CONVERSION_LLVM = Path(os.environ.get("FRAMA_LLVM_PREFIX", str(FRONTEND_LLVM)))
SYSTEM_PREFIX = None
CONVERSION_CIRGEN = executable_path("FRAMA_CIRGEN", "framaCIRGen")
FRONTEND_TOOL = FRONTEND_ROOT / "build-llvm18/buddy-framac-rewrite"
