#!/usr/bin/env python3
"""
Example: List supported formats and read a phase space file.

Usage:
  python read_phase_space.py /path/to/file.IAEAphsp [--format IAEA] [--limit N]
  python read_phase_space.py --formats-only

Arguments:
  path                Path to a phase space file to read. If omitted with --formats-only,
                      the script just lists formats and exits.

Options:
  --format NAME       Explicit format name (e.g., IAEA, EGS, TOPAS, penEasy).
  --limit N           Print at most N particles (default: all).
  --formats-only      Only list formats and exit.
"""

from __future__ import annotations
import argparse
import sys
from typing import Optional

import particlezoo as pz


def print_supported_formats() -> None:
    fmts = pz.FormatRegistry.supported_formats()
    print("Supported formats ({}):".format(len(fmts)))
    for f in fmts:
        print("- {name:<8} ext={ext:<12} {desc}".format(
            name=f.name, ext=f.file_extension, desc=f.description
        ))


def create_reader(path: str, fmt: Optional[str]) -> pz.PhaseSpaceFileReader:
    if fmt:
        return pz.create_reader_for_format(fmt, path)
    return pz.create_reader(path)


def print_particle_summary(p: pz.Particle, idx: int) -> None:
    tname = pz.get_particle_type_name(p.type)
    print(
        f"[{idx}] type={tname:<20} KE={p.kinetic_energy:.6g}"
        f" pos=({p.x:.6g},{p.y:.6g},{p.z:.6g})"
        f" dir=({p.px:.6g},{p.py:.6g},{p.pz:.6g})"
        f" w={p.weight:.6g} newHist={p.is_new_history}"
    )


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description="List formats and read a phase space file")
    ap.add_argument("path", nargs="?", help="Path to phase space file")
    ap.add_argument("--format", dest="format", help="Explicit format name to use")
    ap.add_argument("--limit", type=int, default=None, help="Print at most N particles")
    ap.add_argument("--formats-only", action="store_true", help="Only list formats and exit")
    args = ap.parse_args(argv)

    print_supported_formats()
    if args.formats_only and not args.path:
        return 0

    if not args.path:
        ap.error("path is required unless --formats-only is provided")

    # Create reader
    reader = create_reader(args.path, args.format)

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
        if args.limit is not None and count >= args.limit:
            break

    print(f"\nRead {count} particles. Histories read: {reader.get_histories_read()}.")
    reader.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
