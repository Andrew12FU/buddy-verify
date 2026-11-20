#!/usr/bin/env python3
from pathlib import Path
import runpy
import sys
root = Path(__file__).resolve().parent / "01_frontend/FramaClangFrontend"
sys.path.insert(0, str(root))
runpy.run_path(str(root / 'build_frontend.py'), run_name="__main__")
