"""Run the link module against a mock HAL with a native C compiler."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument("--cc", default=os.environ.get("CC") or shutil.which("gcc"))
args = parser.parse_args()
compiler = args.cc
if not compiler and Path("D:/Programs/msys2/ucrt64/bin/gcc.exe").is_file():
    compiler = "D:/Programs/msys2/ucrt64/bin/gcc.exe"
if not compiler:
    parser.error("Native GCC required; pass --cc <path-to-gcc>.")
root = Path(__file__).resolve().parent.parent
sources = sorted((root / "tests").glob("test_*.c"))
env = os.environ.copy()
env["PATH"] = str(Path(compiler).resolve().parent) + os.pathsep + env.get("PATH", "")
with tempfile.TemporaryDirectory(prefix=root.name + "-tests-") as output:
    for source in sources:
        executable = Path(output) / (source.stem + (".exe" if os.name == "nt" else ""))
        subprocess.run([compiler, "-std=c11", "-Wall", "-Wextra", "-Werror",
                        "-I", str(root / "tests/mocks"), "-I", str(root / "Core/Inc"),
                        str(source), "-o", str(executable)], check=True, env=env)
        subprocess.run([str(executable)], check=True, env=env)
