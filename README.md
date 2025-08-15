# ParticleZoo
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](./LICENSE)

High-performance C++20 library and command-line tools for reading, writing, converting, combining, and visualizing particle phase space files across multiple Monte Carlo ecosystems.

Supported ecosystems include EGS, IAEA, TOPAS, penEasy, and optionally ROOT. Extensions can add more formats without modifying core code.

## Features

- Unified API to read/write multiple phase space formats
- Auto format detection from file extension with explicit overrides
- Fast converters with progress bars and history handling
- Combine multiple inputs into a single output while preserving histories
- Generate slice images (TIFF/BMP) of particle fluence in XY/XZ/YZ planes
- Optional ROOT I/O (TOPAS/OpenGATE layouts or custom branches)
- Extensible via lightweight plugin-style registry

## Supported formats

Built-in formats (auto-registered):

- EGS: .egsphsp (MODE0 and MODE2, supports suffixed extensions like `.egsphsp1`)
- IAEA: .IAEAphsp
- TOPAS: .phsp (Binary, ASCII and Limited)
- penEasy: .dat (ASCII)
- ROOT (optional): .root (TOPAS and OpenGATE templates; custom branch names supported)

List formats at runtime:

```bash
PHSPConvert --formats
```

## Build

Prerequisites:

- Linux or macOS
- C++20 compiler (g++ or clang++)
- make
- Optional: CERN ROOT (with `root-config` on PATH) to enable ROOT I/O

Steps:

```bash
# Configure (detects compiler and optional ROOT)
./configure [--prefix=/your/prefix]

# Build release (default) or debug
make          # same as: make release
make debug

# Optionally install binaries, headers, and static library
make install PREFIX=/usr/local
```

Binaries and library are placed in:

- Release: `build/gcc/release/`
- Debug: `build/gcc/debug/`

Installed files:

- Binaries: `PHSPConvert`, `PHSPCombine`, `PHSPImage` → `$PREFIX/bin`
- Library: `libparticlezoo.a` → `$PREFIX/lib`
- Headers: `include/particlezoo/**` → `$PREFIX/include`

Tip: Run `./configure` after changing your environment (e.g., enabling ROOT).

## Command-line tools

All tools accept arbitrary `--key value` options. Most options are optional; formats are detected from extensions unless overridden.

Common options:

- `--inputFormat <NAME>`: Force reader format (EGS, IAEA, TOPAS, penEasy, ROOT)
- `--outputFormat <NAME>`: Force writer format (where applicable)
- `--maxParticles <N>`: Limit number of particles processed
- `--formats`: Print the list of supported formats and exit

### PHSPConvert

Convert between formats.

Usage:

```bash
PHSPConvert [--<option> <value> ...] inputfile outputfile
```

Examples:

```bash
# Auto-detect formats by extension
PHSPConvert input.egsphsp output.IAEAphsp

# Explicitly set formats
PHSPConvert --inputFormat EGS --outputFormat IAEA input.egsphsp output.IAEAphsp

# Limit to first 1e7 particles
PHSPConvert --maxParticles 10000000 input.IAEAphsp output.phsp
```

### PHSPCombine

Combine multiple inputs into a single output.

Usage:

```bash
PHSPCombine [--<option> <value> ...] --output <outputfile> input1 input2 ... inputN
```

Examples:

```bash
PHSPCombine --output combined.IAEAphsp beam1.egsphsp beam2.egsphsp
PHSPCombine --output out.phsp --inputFormat IAEA --outputFormat TOPAS a.IAEAphsp b.IAEAphsp
```

Notes:

- Preserves/aligns histories; adds any missing histories to match original counts
- `--maxParticles` applies across all inputs cumulatively

### PHSPImage

Create a 2D image (TIFF or BMP) from a slice through the phase space.

Usage:

```bash
PHSPImage [--<option> <value> ...] planeLocation inputfile outputfile
```

Key options:

- `--plane XY|XZ|YZ` (default: `XY`)
- `--imageWidth <px>` (default: 1024)
- `--imageHeight <px>` (default: 1024)
- Region extents (cm):
  - For XY: `--minX --maxX --minY --maxY`
  - For XZ: `--minX --maxX --minZ --maxZ`
  - For YZ: `--minY --maxY --minZ --maxZ`
- Slice half-width (cm): `--widthX|--widthY|--widthZ` (applied to the non-imaged axis)
- `--energyWeighted true|false` (default: false)
- Reader overrides: `--inputFormat <NAME>` (same as other tools)
- Image output format: `--outputFormat tiff|bmp` (default: tiff)

Examples:

```bash
# XY plane at z = 0 cm
PHSPImage --plane XY 0.0 input.IAEAphsp out.tiff

# XZ plane at y = 1 cm, custom ROI and resolution, energy-weighted
PHSPImage --plane XZ 1.0 --minX -10 --maxX 10 --minZ -10 --maxZ 10 \
  --imageWidth 2048 --imageHeight 2048 --energyWeighted true input.egsphsp out.bmp
```

## ROOT-specific options (optional)

If ROOT is enabled (`root-config` found during configure), you can read/write ROOT trees using predefined templates or custom mappings.

Templates:

- `--rootFormat TOPAS`
- `--rootFormat OpenGATE`

Branch overrides (reader and writer):

- `--rootTreeName <name>`
- `--rootEnergy <branch>`
- `--rootWeight <branch>`
- `--rootPositionX <branch>`
- `--rootPositionY <branch>`
- `--rootPositionZ <branch>`
- `--rootDirectionalCosineX <branch>`
- `--rootDirectionalCosineY <branch>`
- `--rootDirectionalCosineZ <branch>`
- `--rootNegativeZCosine <branch>` (bool)
- `--rootPDGCode <branch>`

Notes:

- If `Z` directional cosine is not stored, it is reconstructed from X/Y and the sign (if available)
- Units are handled via the template mapping; custom mappings may apply scale factors internally

## Extending with new formats

Formats are registered through a simple registry:

- Add your reader/writer implementation
- Call `FormatRegistry::RegisterFormat(...)` at startup

## Using the library

Link against the static library and include headers:

```cpp
#include <particlezoo/PhaseSpaceFileReader.h>
#include <particlezoo/PhaseSpaceFileWriter.h>
#include <particlezoo/utilities/formats.h>

using namespace ParticleZoo;

int main() {
  FormatRegistry::RegisterStandardFormats();
  auto reader = FormatRegistry::CreateReader("beam.IAEAphsp");
  auto writer = FormatRegistry::CreateWriter("beam.egsphsp");

  while (reader->hasMoreParticles()) {
    auto p = reader->getNextParticle();
    writer->writeParticle(p);
  }
  writer->close();
  reader->close();
}
```

Link `libparticlezoo.a` (plus ROOT libs if enabled) during build.

## Troubleshooting

- "config.status not found": Run `./configure` before `make`
- No ROOT in supported formats: Ensure root is installed on your system and `root-config` is on PATH, then re-run `./configure`
- Format detection errors: Specify `--inputFormat`/`--outputFormat` explicitly

## License

This project is licensed under the MIT License. See the [LICENSE](./LICENSE) file for details.

Copyright (c) 2025 Daniel O'Brien
