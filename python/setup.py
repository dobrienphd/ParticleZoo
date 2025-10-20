from pathlib import Path
from setuptools import setup
from setuptools.command.build_ext import build_ext
from pybind11.setup_helpers import Pybind11Extension, build_ext as build_ext_pybind

root = Path(__file__).resolve().parent
proj = root.parent

include_dirs = [str(proj / "include")]

# Minimal subset of sources needed for IAEA reader & core utilities
sources = [
    str(Path("..") / "src" / "PhaseSpaceFileReader.cc"),
    str(Path("..") / "src" / "PhaseSpaceFileWriter.cc"),
    str(Path("..") / "src" / "utilities" / "argParse.cc"),
    str(Path("..") / "src" / "utilities" / "formats.cc"),
    # Formats needed by the registry (non-ROOT)
    str(Path("..") / "src" / "egs" / "egsphspFile.cc"),
    str(Path("..") / "src" / "peneasy" / "penEasyphspFile.cc"),
    str(Path("..") / "src" / "topas" / "TOPASHeader.cc"),
    str(Path("..") / "src" / "topas" / "TOPASphspFile.cc"),
    str(Path("..") / "src" / "IAEA" / "IAEAHeader.cc"),
    str(Path("..") / "src" / "IAEA" / "IAEAphspFile.cc"),
]

ext_modules = [
    Pybind11Extension(
        "particlezoo._pz",
        sources=[str(Path("src/pybind/module.cpp"))] + sources,
        include_dirs=include_dirs,
        # Use C++20
        cxx_std=20,
        define_macros=[("PYBIND11_DETAILED_ERROR_MESSAGES", "1")],
        extra_compile_args=["-O3", "-fvisibility=hidden", "-std=c++20"],
    )
]

setup(
    cmdclass={"build_ext": build_ext_pybind},
    ext_modules=ext_modules,
)
