from pathlib import Path
import shutil
import subprocess
import re
import sys
from setuptools import setup
from pybind11.setup_helpers import Pybind11Extension, build_ext as build_ext_pybind

root = Path(__file__).resolve().parent
proj = root.parent
cpp = root / "cpp"


def sync_cpp_sources():
    """Vendor the C++ sources into python/cpp so the package is self-contained.

    When building from a git checkout the canonical sources live one directory
    up; they are copied here so that sdists (which cannot reference files
    outside the package root) ship everything needed to compile. When building
    from an sdist the vendored copy is already present and the parent
    directories don't exist, so this is a no-op.
    """
    if not ((proj / "src").is_dir() and (proj / "include").is_dir()):
        return
    for sub in ("src", "include"):
        dst = cpp / sub
        if dst.exists():
            shutil.rmtree(dst)
        shutil.copytree(proj / sub, dst)
    if (proj / "LICENSE").is_file():
        shutil.copy2(proj / "LICENSE", root / "LICENSE")


sync_cpp_sources()

include_dirs = [str(cpp / "include")]

src = Path("cpp") / "src"

# Minimal subset of sources needed for IAEA reader & core utilities
sources = [
    str(src / "PhaseSpaceFileReader.cc"),
    str(src / "PhaseSpaceFileWriter.cc"),
    str(src / "utilities" / "argParse.cc"),
    str(src / "utilities" / "formats.cc"),
    # Formats needed by the registry (non-ROOT)
    str(src / "egs" / "egsphspFile.cc"),
    str(src / "peneasy" / "penEasyphspFile.cc"),
    str(src / "topas" / "TOPASHeader.cc"),
    str(src / "topas" / "TOPASphspFile.cc"),
    str(src / "IAEA" / "IAEAHeader.cc"),
    str(src / "IAEA" / "IAEAphspFile.cc"),
    # Parallel readers
    str(src / "parallel" / "HistoryBalancedParallelReader.cc"),
    str(src / "parallel" / "ParticleBalancedParallelReader.cc"),
]

define_macros = [("PYBIND11_DETAILED_ERROR_MESSAGES", "1")]

if sys.platform == "win32":
    # MSVC: optimization and C++ standard flags are supplied by distutils
    # and Pybind11Extension (cxx_std) respectively.
    extra_compile_args = []
    extra_link_args = []
else:
    extra_compile_args = ["-O3", "-fvisibility=hidden"]
    extra_link_args = []

# Fix for macOS: std::filesystem requires macOS 10.15+
if sys.platform == "darwin":
    extra_compile_args.append("-mmacosx-version-min=13.3")
    extra_link_args.append("-mmacosx-version-min=13.3")

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
            sources.append(str(src / "ROOT" / "ROOTphsp.cc"))
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
            sources.append(str(src / "ROOT" / "ROOTphsp.cc"))
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
