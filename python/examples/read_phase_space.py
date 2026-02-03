#!/usr/bin/env python3
"""
Example: List supported formats and read a phase space file.

Usage:
  python read_phase_space.py <file> [options]
  python read_phase_space.py --formats-only

Examples:
  python read_phase_space.py file.IAEAphsp
  python read_phase_space.py file.root --ROOT-format OpenGATE
  python read_phase_space.py file.root --ROOT-format TOPAS --limit 10

This script passes all command line arguments through to ParticleZoo's C++ argument
parser, which handles format-specific options automatically.

Note: The filename must come first, before any optional arguments.

Common options:
  --limit N           Print at most N particles (default: all).
  --formats-only      Only list formats and exit.
  
Format-specific options are automatically supported (e.g., --ROOT-format, --iaea-ignore-zlast, etc.)
Run with --help to see all available options.
"""

from __future__ import annotations
import sys

import particlezoo as pz


def print_supported_formats() -> None:
    fmts = pz.FormatRegistry.supported_formats()
    print("Supported formats ({}):".format(len(fmts)))
    for f in fmts:
        print("- {name:<8} ext={ext:<12} {desc}".format(
            name=f.name, ext=f.file_extension, desc=f.description
        ))


def print_particle_summary(p: pz.Particle, idx: int) -> None:
    tname = pz.get_particle_type_name(p.type)
    # Convert to human-readable units for display
    ke_mev = p.kinetic_energy / pz.MeV
    x_cm = p.x / pz.cm
    y_cm = p.y / pz.cm
    z_cm = p.z / pz.cm
    
    print(
        f"[{idx}] type={tname:<20} KE={ke_mev:.6g} MeV"
        f" pos=({x_cm:.6g},{y_cm:.6g},{z_cm:.6g}) cm"
        f" dir=({p.px:.6g},{p.py:.6g},{p.pz:.6g})"
        f" w={p.weight:.6g} newHist={p.is_new_history}"
    )



def main(argv: list[str]) -> int:
    # Parse arguments using ParticleZoo's C++ argument parser
    # This will handle --help, --formats, and all format-specific options automatically
    usage_message = (
        "Usage: read_phase_space.py <file> [options]\n\n"
        "Example script to read phase space files.\n\n"
        "Options:\n"
        "  --limit N           Print at most N particles\n"
        "  --formats-only      Only list formats and exit\n"
    )
    
    # Check for --formats-only flag manually before parsing
    if "--formats-only" in argv:
        print_supported_formats()
        return 0
    
    # Extract the limit option manually (not a format-specific option)
    limit = None
    clean_argv = []
    skip_next = False
    for i, arg in enumerate(argv):
        if skip_next:
            skip_next = False
            continue
        if arg == "--limit":
            if i + 1 < len(argv):
                try:
                    limit = int(argv[i + 1])
                    skip_next = True
                    continue
                except ValueError:
                    pass
        clean_argv.append(arg)
    
    # Parse remaining arguments through C++ parser (requires at least 1 positional arg: the file path)
    options = pz.ArgParser.parse_args(clean_argv, usage_message, min_positional_args=1)
    
    # Extract the file path (first positional argument)
    filepath = options.extract_positional(0)
    
    print_supported_formats()
    print()

    # Create reader with parsed options - format detection and options handled automatically
    reader = pz.create_reader(filepath, options)

    # File summary
    try:
        n = reader.get_number_of_particles()
    except Exception:
        # Some formats may compute this lazily: fall back to counting
        n = None
    try:
        h = reader.get_number_of_original_histories()
    except Exception:
        h = None
    print(
        f"\nReading: {reader.get_file_name()} (format={reader.get_phsp_format()})\n"
        f"File size: {reader.get_file_size()} bytes\n"
        f"Particles: {n if n is not None else 'unknown'}\n"
        f"Original histories: {h if h is not None else 'unknown'}\n"
    )

    # Iterate particles
    count = 0
    for idx, p in enumerate(reader):
        print_particle_summary(p, idx)
        count += 1
        if limit is not None and count >= limit:
            break

    print(f"\nRead {count} particles. Histories read: {reader.get_histories_read()}.")
    reader.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
