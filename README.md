# ParticleZoo
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](./LICENSE)

A high-performance C++20 library for reading, writing, and manipulating particle phase space files across multiple Monte Carlo simulation ecosystems. ParticleZoo provides a unified API that abstracts away format-specific details, enabling seamless interoperability between different simulation codes and workflows.

## Overview

ParticleZoo serves as a universal translator and processor for particle phase space data, which represents the position, momentum, energy, and other properties of particles at specific locations in Monte Carlo simulations. The library is designed around a common `Particle` data model that can represent particles from any supported format, with automatic format detection and conversion capabilities.

### Key Features

- **Unified API**: Single interface to work with multiple phase space formats
- **Format Transparency**: Automatic format detection with explicit override options
- **High Performance**: Efficient binary I/O with configurable buffering
- **Parallel Processing**: Built-in support for multi-threaded reading with history or particle-balanced partitioning
- **Random Access**: Seek to specific particles within phase space files
- **Extensible Architecture**: Plugin-style registry system for adding new formats
- **Unit Consistency**: Comprehensive unit system ensures proper dimensional handling
- **Memory Efficient**: Streaming interfaces for processing large files
- **Cross-Platform**: Windows, Linux and macOS support with standard build tools
- **Python Bindings**: Optional Python package for scripting and rapid prototyping

## Supported Formats

The library includes built-in support for major Monte Carlo simulation formats:

- **EGS** (EGSnrc): `.egsphsp` files in MODE0 and MODE2, including suffixed variants (`.egsphsp1`, etc.)
- **IAEA**: `.IAEAphsp` International Atomic Energy Agency format with header files
- **TOPAS**: `.phsp` files in Binary, ASCII, and Limited variants
- **penEasy**: `.dat` ASCII format from the PENELOPE simulation code
- **ROOT** (optional): `.root` files generated with the CERN ROOT framework. Includes build-in templates for TOPAS and OpenGATE generated files. Also supports custom branch mappings

Additional formats can be added through the extensible registry system without modifying core library code.

## Architecture

### Core Components

**`Particle` Class**: The central data model representing a particle with position, momentum, energy, weight, and type information. Supports both standard properties and format-specific extensions through a flexible property system.

**`PhaseSpaceFileReader`**: Abstract base class for reading phase space files. Implementations handle format-specific parsing while presenting a common streaming interface.

**`PhaseSpaceFileWriter`**: Abstract base class for writing phase space files. Handles format-specific serialization with proper history counting.

**`FormatRegistry`**: Plugin-style system for registering and creating readers/writers. Enables runtime format discovery and automatic format detection from file extensions.

**`ByteBuffer`**: High-performance binary I/O buffer for efficient reading of large files with configurable buffering strategies.

**`HistoryBalancedParallelReader`**: Multi-threaded reader that partitions phase space files by history count for parallel processing. Each thread receives an approximately equal share of histories.

**`ParticleBalancedParallelReader`**: Multi-threaded reader that partitions phase space files by particle count for parallel processing. Each thread receives an approximately equal share of particles (with history boundaries respected).

### Data Model

The `Particle` class provides access to:

- **Spatial coordinates**: Position (x, y, z) in configurable units
- **Momentum**: Direction cosines and kinetic energy
- **Particle properties**: PDG particle codes, statistical weight
- **History tracking**: Original history numbers and incremental counters
- **Generation tracking**: Primary/secondary status and generation number (where available)
- **Format-specific data**: Extensible property system for specialized information

### Unit System

ParticleZoo includes a comprehensive unit system that ensures dimensional consistency across different formats. The unit system covers length, area, volume, energy, mass, time, frequency, force, pressure, charge, density, dose, and angle:

```cpp
// Length units: nm, um, mm, cm, m, km, in, ft
float x_in_cm = particle.getX() / cm;
float y_in_mm = particle.getY() / mm;

// Energy units: eV, keV, MeV, GeV, TeV, J
float energy_MeV = particle.getKineticEnergy() / MeV;

// Dose units: Gy, cGy, rad, Sv, mSv, rem
float dose_cGy = absorbed_dose / cGy;

// Angle units: radian, deg
float angle_deg = angle / deg;
```

## Building and Installation

### Prerequisites

- **Operating System**: Linux, macOS, or Windows
- **Compiler / Toolchain**:
    - Linux/macOS: C++20 compatible compiler (GCC 10+, Clang 13+)
    - Windows: Visual Studio 2019 or later with C++ development tools (MSVC)
- **Build Tools**:
    - Linux/macOS: GNU Make
    - Windows: Windows Command Prompt or PowerShell (uses `build.bat`)
- **Optional Dependencies**:
    - CERN ROOT (for ROOT format support)
        - Requires `root-config` in PATH on all platforms

### Build Process

#### Linux and macOS
```bash
# Configure the build system (auto-detects compiler and dependencies)
./configure [--prefix=/your/installation/prefix]

# Build the library and tools
make          # Release build (default)
make debug    # Debug build with symbols
make release  # Explicitly build release version

# Install (optional)
make install  # defaults to /usr/local for the install PREFIX
make install PREFIX=/usr/local

# Python bindings (optional)
make install-python      # Standard installation
make install-python-dev  # Editable/development installation
make uninstall-python    # Remove Python bindings
```

#### Windows
```powershell
# Configure, build, and optionally install
build.bat [--prefix=C:\path\to\install] [debug|release]
build.bat install [--prefix=C:\path\to\install] [debug|release]
```

### Build Outputs

The build system creates the following artifacts:

**Release build**
- Linux/macOS: `build/gcc/release/`
- Windows:   `build/msvc/release/`
    - Static library:
        - Linux/macOS: `libparticlezoo.a`
        - Windows:   `libparticlezoo.lib`
    - Executables:
        - Linux/macOS: `PHSPConvert`, `PHSPCombine`, `PHSPImage`, `PHSPSplit`
        - Windows:   `PHSPConvert.exe`, `PHSPCombine.exe`, `PHSPImage.exe`, `PHSPSplit.exe`
    - Dynamic library (Windows only): `build/msvc/release/bin/particlezoo.dll`

**Debug build**
- Linux/macOS: `build/gcc/debug/`
- Windows:   `build/msvc/debug/`
    - Static library with debug symbols:
        - Linux/macOS: `libparticlezoo.a`
        - Windows:   `libparticlezoo.lib`
    - Debug executables:
        - Linux/macOS: `PHSPConvert`, etc.
        - Windows:   `PHSPConvert.exe`, etc.
    - Dynamic library (Windows only): `build/msvc/debug/bin/particlezoo.dll`

**Installation** (optional)
- Linux/macOS (with `make install`):
    - Headers: `$PREFIX/include/particlezoo/`
    - Static Library: `$PREFIX/lib/libparticlezoo.a`
    - Executables: `$PREFIX/bin/PHSPConvert`, etc.
- Windows (with `build.bat install`):
    - Headers: `%PREFIX%\include\particlezoo\`
    - Static Library: `%PREFIX%\lib\libparticlezoo.lib`
    - Executables and DLL: `%PREFIX%\bin\PHSPConvert.exe`, etc.

### Configuration Options

The `configure` script (Linux/macOS) accepts the following options:

- `--prefix=PATH` - Installation prefix (default: `/usr/local`)
- `--no-root` - Disable ROOT support even if available

The `build.bat` script (Windows) accepts the following options:

- `--prefix=PATH` - Installation prefix (default: `%LOCALAPPDATA%\particlezoo`)
- `--no-root` - Disable ROOT support even if available
- `-j N` or `--jobs=N` - Number of parallel compilation jobs


## Using the Library

### Basic Usage

Here's a simple example showing how to read from one format and write to another:

```cpp
#include <particlezoo/PhaseSpaceFileReader.h>
#include <particlezoo/PhaseSpaceFileWriter.h>
#include <particlezoo/utilities/formats.h>

using namespace ParticleZoo;

int main() {
    // Register standard formats
    FormatRegistry::RegisterStandardFormats();
    
    // Create readers and writers - format auto-detected from extension
    auto reader = FormatRegistry::CreateReader("input.IAEAphsp");
    auto writer = FormatRegistry::CreateWriter("output.egsphsp");
    
    // Process all particles
    while (reader->hasMoreParticles()) {
        Particle particle = reader->getNextParticle();
        
        // Optionally modify particle properties
        // particle.setWeight(particle.getWeight() * 2.0);
        
        writer->writeParticle(particle);
    }
    
    // Clean up
    writer->close();
    reader->close();
    
    return 0;
}
```

### Advanced Usage with Options

Many format readers and writers accept configuration options:

```cpp
// Create options map for custom behavior
// Requires: #include <particlezoo/ROOT/ROOTphsp.h>
UserOptions options;
// Example: select predefined ROOT template when reading ROOT files
options[ParticleZoo::ROOT::ROOTFormatCommand] = { std::string("TOPAS") };

// Create reader with our user options
auto reader = FormatRegistry::CreateReader("ROOT", "simulation.root", options);
```

### Advanced Usage with Fixed Values

Many formats support holding certain values (e.g. X, Y, Z) constant across all particles to reduce file sizes.

```cpp
// Create default options map
UserOptions options;

// Create the flags for the fixed values and set the Z value to be constant at 100 cm
FixedValues fixedValues;
fixedValues.zIsConstant = true;
fixedValues.constantZ = 100 * cm;

// Create writer with explicit format, options, and a fixed Z value
auto writer = FormatRegistry::CreateWriter("IAEA", "simulation.IAEAphsp", options, fixedValues);
```

### Working with Particles

The `Particle` class provides extensive access to particle properties:

```cpp
Particle p = reader->getNextParticle();

// Basic properties
float x = p.getX() / cm;           // Position in cm
float y = p.getY() / cm;
float z = p.getZ() / cm;

float energy = p.getKineticEnergy() / MeV;  // Energy in MeV
float weight = p.getWeight();               // Statistical weight

// Direction (unit vector)
float dx = p.getDirectionalCosineX();
float dy = p.getDirectionalCosineY();
float dz = p.getDirectionalCosineZ();

// Generation information
bool isPrimary = p.isPrimary();
if (p.hasIntProperty(IntPropertyType::GENERATION)) {
    int generation = p.getIntProperty(IntPropertyType::GENERATION);
}
```

### Format-Specific Features

Different formats support different features. The library provides access to format-specific properties:

```cpp
// EGS-specific latch information
if (p.hasIntProperty(IntPropertyType::EGS_LATCH)) {
    int latch = p.getIntProperty(IntPropertyType::EGS_LATCH);
}

// PENELOPE-specific interaction flags  
if (p.hasIntProperty(IntPropertyType::PENELOPE_ILB1)) {
    int ilb1 = p.getIntProperty(IntPropertyType::PENELOPE_ILB1);
}
```

### Error Handling

The library uses standard C++ exception handling:

```cpp
try {
    auto reader = FormatRegistry::CreateReader("nonexistent.phsp");
    // ... process particles
} catch (const std::runtime_error& e) {
    std::cerr << "Error: " << e.what() << std::endl;
} catch (const std::exception& e) {
    std::cerr << "Unexpected error: " << e.what() << std::endl;
}
```

## Command-Line Tools

ParticleZoo includes several command-line utilities that demonstrate the library's capabilities:

### PHSPConvert - Format Conversion

Converts phase space files between different formats with optional filtering and transformations:

```bash
# Auto-detect formats from file extensions
PHSPConvert input.egsphsp output.IAEAphsp

# Explicitly specify formats
PHSPConvert --inputFormat EGS --outputFormat IAEA input.file output.file

# Limit particle count
PHSPConvert --maxParticles 1000000 input.IAEAphsp output.phsp

# Project particles to a plane during conversion
PHSPConvert --projectToZ 100.0 input.phsp output.IAEAphsp

# Filter by particle type
PHSPConvert --photonsOnly input.egsphsp photons_only.IAEAphsp
PHSPConvert --electronsOnly input.egsphsp electrons_only.IAEAphsp
PHSPConvert --filterByPDG 2212 input.phsp protons_only.phsp

# Filter by energy range (in MeV)
PHSPConvert --minEnergy 1.0 --maxEnergy 10.0 input.phsp filtered.phsp

# Filter by position (in cm)
PHSPConvert --minX -5.0 --maxX 5.0 --minY -5.0 --maxY 5.0 input.phsp central.phsp

# Filter by radial distance from central axis (in cm)
PHSPConvert --maxRadius 10.0 input.phsp within_radius.phsp

# Filter by generation (primary, secondary, etc.)
PHSPConvert --primariesOnly input.phsp primaries.phsp
PHSPConvert --excludePrimaries input.phsp secondaries.phsp
PHSPConvert --generations 1 2 input.phsp first_two_generations.phsp
```

### PHSPCombine - File Merging

Combines multiple phase space files into a single output:

```bash
# Combine multiple files
PHSPCombine --outputFile combined.IAEAphsp file1.egsphsp file2.egsphsp file3.egsphsp

# Mix formats during combination
PHSPCombine --outputFile result.phsp input1.IAEAphsp input2.egsphsp

# Preserve constant values in the output file if all input files have the same constant values
PHSPCombine --preserveConstants --outputFile result.IAEAphsp input1.IAEAphsp input2.IAEAphsp
```

### PHSPImage - Visualization and Third Party Analysis

Creates 2D particle fluence or energy fluence images from phase space data. Can output either a detailed TIFF image with raw fluence data stored in 32-bit floats (default) which can be analyzed directly in third party tools like ImageJ, or a simple bitmap BMP image with automatic contrast for easy visualization:

```bash
# Generate a flattened XY plane image (default)
PHSPImage beam.egsphsp fluence_map.tiff

# Project particles to a specific XY plane (e.g. 100 cm or isocenter)
PHSPImage --projectTo 100 beam.egsphsp projection.tiff

# Custom plane and energy weighting, particles are not relocated, only particles located at
# Y = 5 cm +- a default margin of 0.25 cm will be counted (margin for XZ plane can be changed
# with the --tolerance parameter)
PHSPImage --outputFormat BMP --projectionType none --plane XZ --planeLocation 5.0 --energyWeighted simulation.IAEAphsp dose_profile.bmp

# Score different quantities (count, energy, xDir, yDir, zDir)
PHSPImage --score energy input.phsp energy_fluence.tiff

# Filter by generation
PHSPImage --primariesOnly input.phsp primaries_fluence.tiff
PHSPImage --generations 2 3 input.phsp secondaries_fluence.tiff

# Custom image dimensions and spatial boundaries
PHSPImage --imageWidth 2048 --imageHeight 2048 --square 20.0 input.phsp high_res.tiff
PHSPImage --minX -10 --maxX 10 --minY -10 --maxY 10 input.phsp custom_bounds.tiff

# EGS LATCH filtering (for EGS format files)
PHSPImage --EGS-latch-filter 0x00000001 input.egsphsp latch_filtered.tiff
```

### PHSPSplit - File Splitting

Splits a single phase space file into multiple (roughly) equally sized output files. History boundaries are respected, so individual files may differ slightly in size.

```bash
# Split a file into multiple parts
PHSPSplit --splitNumber 10 input.egsphsp

# Use short flag and specify output format
PHSPSplit -n 5 --outputFormat IAEA input.egsphsp
```

## Examples

The `examples/` directory contains reference implementations showing how to integrate ParticleZoo into external simulation frameworks and scripting workflows.

### Python Scripts (`examples/python/`)

Ready-to-run Python scripts demonstrating core ParticleZoo functionality:

- **`read_phase_space.py`** — Lists supported formats and reads particles from any phase space file. Supports all command-line options (format overrides, particle limits, ROOT branch mappings, etc.).
- **`write_phase_space.py`** — Creates a synthetic phase space file by generating sample particles programmatically. Demonstrates writer creation, particle construction, and format-specific options.

```bash
# List supported formats
python examples/python/read_phase_space.py --formats-only

# Read a phase space file
python examples/python/read_phase_space.py beam.IAEAphsp --limit 10

# Write a synthetic phase space file
python examples/python/write_phase_space.py output.egsphsp
```

### Geant4 Phase Space Source (`examples/geant4/`)

`G4PHSPSourceAction` is a ready-to-use `G4VUserPrimaryGeneratorAction` that reads particles from any ParticleZoo-supported phase space file and injects them as primary vertices into a Geant4 simulation. Key features include:

- **Multi-threaded support** via a shared `ParticleBalancedParallelReader`, with each Geant4 worker thread processing its own partition of the file
- **Incremental history handling** to correctly reproduce the original history structure
- **Particle recycling** with automatic weight adjustment
- **Spatial transformations** — apply global translations and rotations (with configurable center of rotation) to model gantry angles, collimator rotations, etc.

Typical setup in your `ActionInitialization`:

```cpp
// Master thread: create a shared parallel reader
auto reader = std::make_shared<ParticleZoo::ParticleBalancedParallelReader>(
    "beam.IAEAphsp", UserOptions{}, numberOfThreads);

// Worker threads: create one G4PHSPSourceAction per thread
void MyActionInitialization::Build() const {
    auto action = new ParticleZoo::G4PHSPSourceAction(reader, G4Threading::G4GetThreadId());
    action->SetTranslation(G4ThreeVector(0, 0, -100*cm));
    action->SetRecycleNumber(5);
    SetUserAction(action);
}
```

## Extending the Library

### Adding New Formats

To add support for a new phase space format:

1. **Implement Reader**: Inherit from `PhaseSpaceFileReader` and implement virtual methods
2. **Implement Writer**: Inherit from `PhaseSpaceFileWriter` and implement virtual methods  
3. **Register Format**: Add registration call to connect file extensions with your implementations

Example registration:

```cpp
SupportedFormat myFmt{"MyFormat", "My custom phase space format", ".myext"};
FormatRegistry::RegisterFormat(
    myFmt,
    [](const std::string& file, const UserOptions& opts) -> std::unique_ptr<PhaseSpaceFileReader> {
        return std::make_unique<MyFormatReader>(file, opts);
    },
    [](const std::string& file, const UserOptions& opts, const FixedValues & fixedValues) -> std::unique_ptr<PhaseSpaceFileWriter> {
        return std::make_unique<MyFormatWriter>(file, opts, fixedValues);
    }
);
```

### Custom Particle Properties

The `Particle` class supports custom properties through the property system:

```cpp
// Add custom integer property
particle.setIntProperty(IntPropertyType::CUSTOM, 42);

// Add custom float property  
particle.setFloatProperty(FloatPropertyType::CUSTOM, 3.14f);

// Add custom boolean property
particle.setBoolProperty(BoolPropertyType::CUSTOM, true);
```

## ROOT Format Support (Optional)

When compiled with ROOT support, ParticleZoo can read and write ROOT-based phase space files using predefined templates or custom branch mappings.

### Predefined Templates

```bash
# Use TOPAS template
PHSPConvert --inputFormat ROOT --ROOT-format TOPAS input.root output.IAEAphsp

# Use OpenGATE template  
PHSPConvert --inputFormat ROOT --ROOT-format OpenGATE simulation.root converted.egsphsp
```

### Custom Branch Mapping

For ROOT files with non-standard branch names, you can configure custom branch mappings using the `BranchInfo` structure. Each mapping specifies the branch name and its unit conversion factor:

```cpp
#include <particlezoo/ROOT/ROOTphsp.h>

using namespace ParticleZoo;

// Create reader with custom mappings via UserOptions
UserOptions options;
options[ROOT::ROOTTreeNameCommand] = { std::string("MyTree") };
options[ROOT::ROOTEnergyCommand] = { std::string("E_kin") };
options[ROOT::ROOTPositionXCommand] = { std::string("pos_x") };
// ... additional mappings as needed

auto reader = FormatRegistry::CreateReader("ROOT", "custom_file.root", options);
```

The command-line tools also support custom branch mapping via options:

```bash
PHSPConvert --inputFormat ROOT \
    --ROOT-tree-name MyTree \
    --ROOT-energy E_kin \
    --ROOT-position-x pos_x \
    --ROOT-position-y pos_y \
    --ROOT-position-z pos_z \
    --ROOT-weight stat_weight \
    input.root output.phsp
```

Available branch mapping options:
- `--ROOT-tree-name <name>` - ROOT tree name
- `--ROOT-energy <branch>` - Energy branch
- `--ROOT-weight <branch>` - Statistical weight branch  
- `--ROOT-position-x/y/z <branch>` - Position branches
- `--ROOT-cosine-x/y/z <branch>` - Direction cosines
- `--ROOT-cosine-z-sign <branch>` - Boolean flag for Z-direction sign
- `--ROOT-pdg-code <branch>` - Particle type identifier
- `--ROOT-history-number <branch>` - History counter

### Random Access

Readers support random access to particles within phase space files:

```cpp
auto reader = FormatRegistry::CreateReader("simulation.IAEAphsp");

// Move to a specific particle index (zero-based)
reader->moveToParticle(1000000);

// Continue reading from that position
while (reader->hasMoreParticles()) {
    Particle p = reader->getNextParticle();
    // ...
}
```

### Parallel Processing

For large-scale processing, use the parallel readers to distribute work across multiple threads:

```cpp
#include <particlezoo/parallel/ParticleBalancedParallelReader.h>
#include <thread>
#include <vector>

// Create a parallel reader with 8 threads
const size_t numThreads = 8;
ParticleBalancedParallelReader parallelReader("large_file.IAEAphsp", {}, numThreads);

// Each thread processes its partition independently
std::vector<std::thread> threads;
for (size_t threadId = 0; threadId < numThreads; threadId++) {
    threads.emplace_back([&parallelReader, threadId]() {
        while (parallelReader.hasMoreParticles(threadId)) {
            Particle p = parallelReader.getNextParticle(threadId);
            // Process particle...
        }
    });
}

// Wait for all threads to complete
for (auto& t : threads) {
    t.join();
}
```

## Python Bindings

ParticleZoo includes optional Python bindings for scripting and rapid prototyping.

### Installation

#### Linux/macOS (using Makefile)

```bash
# Standard installation (uses virtual env if active, otherwise user site-packages)
make install-python

# Development/editable installation
make install-python-dev

# Uninstall
make uninstall-python
```

#### Windows (using pip)

```powershell
# From the repository root
python -m pip install python        # Standard install
python -m pip install -e python     # Editable install
```

See the [python/README.md](python/README.md) for detailed installation options and full API documentation.

```python
import particlezoo as pz

# Create a particle programmatically
p = pz.Particle(pz.ParticleType.Electron, 6.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0)
print(f"Created particle with energy: {p.kinetic_energy} MeV")

# Read particles from one format and write to another
reader = pz.create_reader("/path/to/input.IAEAphsp")
writer = pz.create_writer("/path/to/output.egsphsp")

for i, particle in enumerate(reader):
    # Optionally modify the particle
    # particle.weight *= 2.0
    
    # Write to output file
    writer.write_particle(particle)
    
    if i % 100000 == 0:
        print(f"Processed {i} particles...")

writer.close()
print("Conversion complete!")
```

## Performance Considerations

### Memory Usage

- ParticleZoo uses streaming I/O to minimize memory footprint
- Configurable buffer sizes for optimal performance vs. memory trade-offs
- Large files can be processed with constant memory usage

### Optimization Tips

- Use binary formats when possible for faster I/O
- Consider particle limits (`--maxParticles`) for testing and prototyping
- Enable compiler optimizations (`make release`) for production use
- Use parallel readers for multi-threaded processing of large files
- ROOT format may be slower due to tree structure overhead

## Troubleshooting

### Common Issues

**Build Problems:**
- *"config.status not found"* → Run `./configure` before `make`
- *"The C++ standard in this build does not match ROOT configuration"* → The ROOT installation on your system was compiled with a different C++ standard than ParticleZoo. It may still work, but it is not supported. Either rebuild both with the same C++ standard or accept the risk.
- *"ROOT support: no"* → Ensure `root-config` is in PATH and re-run `./configure`
- *"checking whether g++ accepts -std=c++20... no"* → Update compiler (GCC 10+ or Clang 13+)

**Runtime Issues:**
- *"Unknown format"* → Use `--inputFormat` to explicitly specify format
- *"File not found"* → Check file paths and permissions

### Getting Help

For additional support:
1. Use `--formats` option to verify supported formats at runtime
2. Try explicit format specification with `--inputFormat`/`--outputFormat`

## License

This project is licensed under the MIT License. See the [LICENSE](./LICENSE) file for details.

Copyright (c) 2025-2026 Daniel O'Brien
