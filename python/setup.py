"""Build the ``gfsk8`` extension with the repository's CMake project.

``pip install <repo>/python`` (or ``pip install --no-build-isolation`` on a
box without network) configures and builds ``gfsk8_pymod`` for the
installing interpreter and places the resulting ``gfsk8*.so`` in the
wheel. No hand copies: the AfterNet ``make dev`` and image ``build.sh``
both call exactly this.
"""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
from pathlib import Path

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext

ROOT = Path(__file__).resolve().parent.parent


class CMakeExtension(Extension):
    def __init__(self, name: str) -> None:
        super().__init__(name, sources=[])


class CMakeBuild(build_ext):
    def build_extension(self, ext: Extension) -> None:  # noqa: D102
        build_dir = Path(self.build_temp).resolve() / "cmake"
        build_dir.mkdir(parents=True, exist_ok=True)
        jobs = str(os.cpu_count() or 2)
        subprocess.check_call(
            [
                "cmake",
                "-S", str(ROOT),
                "-B", str(build_dir),
                "-DCMAKE_BUILD_TYPE=Release",
                "-DBUILD_PYTHON_MODULE=ON",
                f"-DPython3_EXECUTABLE={sys.executable}",
            ]
        )
        subprocess.check_call(
            ["cmake", "--build", str(build_dir), "--target", "gfsk8_pymod", "-j", jobs]
        )
        built = sorted((build_dir / "python").glob("gfsk8*.so"))
        if len(built) != 1:
            raise RuntimeError(f"expected one gfsk8 .so in {build_dir / 'python'}, found {built}")
        dest = Path(self.get_ext_fullpath(ext.name))
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(built[0], dest)


setup(ext_modules=[CMakeExtension("gfsk8")], cmdclass={"build_ext": CMakeBuild})
