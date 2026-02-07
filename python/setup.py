from pathlib import Path
import subprocess
import re
import sys
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

define_macros = [("PYBIND11_DETAILED_ERROR_MESSAGES", "1")]
extra_compile_args = ["-O3", "-fvisibility=hidden", "-std=c++20"]
extra_link_args = []

# Fix for macOS: std::filesystem requires macOS 10.15+
if sys.platform == "darwin":
    extra_compile_args.extend(["-mmacosx-version-min=13.3"])
    extra_link_args.extend(["-mmacosx-version-min=13.3"])

# Try to read ROOT configuration from config.status
config_status = proj / "config.status"
use_root = False

if config_status.exists():
    print(f"Reading configuration from {config_status}")
    config_vars = {}
    with open(config_status, "r") as f:
        for line in f:
            # Parse Makefile-style variable assignments: VAR = value
            match = re.match(r'^(\w+)\s*=\s*(.*)$', line.strip())
            if match:
                var_name, var_value = match.groups()
                config_vars[var_name] = var_value.strip()
    
    if config_vars.get("USE_ROOT") == "1":
        use_root = True
        root_cflags = config_vars.get("ROOT_CFLAGS", "").split()
        root_libs = config_vars.get("ROOT_LIBS", "").split()
        
        if root_cflags or root_libs:
            print("ROOT support enabled (from config.status)")
            define_macros.append(("USE_ROOT", "1"))
            sources.append(str(Path("..") / "src" / "ROOT" / "ROOTphsp.cc"))
            extra_compile_args.extend(root_cflags)
            extra_link_args.extend(root_libs)
        else:
            print("WARNING: USE_ROOT=1 but no ROOT flags found")
            use_root = False

if not use_root and not config_status.exists():
    # Fallback: try to detect ROOT directly if config.status doesn't exist
    print("config.status not found, attempting to detect ROOT...")
    try:
        root_cflags = subprocess.check_output(
            ["root-config", "--cflags"], 
            stderr=subprocess.DEVNULL, 
            text=True
        ).strip().split()
        root_libs = subprocess.check_output(
            ["root-config", "--libs"], 
            stderr=subprocess.DEVNULL, 
            text=True
        ).strip().split()
        
        if root_cflags and root_libs:
            print("ROOT detected - enabling ROOT support")
            define_macros.append(("USE_ROOT", "1"))
            sources.append(str(Path("..") / "src" / "ROOT" / "ROOTphsp.cc"))
            extra_compile_args.extend(root_cflags)
            extra_link_args.extend(root_libs)
        else:
            print("ROOT found but flags empty - building without ROOT support")
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("ROOT not found - building without ROOT support")

if not use_root and config_status.exists():
    print("ROOT support disabled (per config.status)")

ext_modules = [
    Pybind11Extension(
        "particlezoo._pz",
        sources=[str(Path("src/pybind/module.cpp"))] + sources,
        include_dirs=include_dirs,
        cxx_std=20,
        define_macros=define_macros,
        extra_compile_args=extra_compile_args,
        extra_link_args=extra_link_args,
    )
]

setup(
    cmdclass={"build_ext": build_ext_pybind},
    ext_modules=ext_modules,
)