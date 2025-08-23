# ParticleZoo
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](./LICENSE)

A high-performance C++20 library for reading, writing, and manipulating particle phase space files across multiple Monte Carlo simulation ecosystems. ParticleZoo provides a unified API that abstracts away format-specific details, enabling seamless interoperability between different simulation codes and workflows.

## Overview

ParticleZoo serves as a universal translator and processor for particle phase space data, which represents the position, momentum, energy, and other properties of particles at specific locations in Monte Carlo simulations. The library is designed around a common `Particle` data model that can represent particles from any supported format, with automatic format detection and conversion capabilities.

### Key Features

- **Unified API**: Single interface to work with multiple phase space formats
- **Format Transparency**: Automatic format detection with explicit override options
- **High Performance**: Efficient binary I/O with configurable buffering
- **Extensible Architecture**: Plugin-style registry system for adding new formats
- **Unit Consistency**: Built-in unit system ensures proper dimensional handling
- **Memory Efficient**: Streaming interfaces for processing large files
- **Cross-Platform**: Linux and macOS support with standard build tools

## Supported Formats

The library includes built-in support for major Monte Carlo simulation formats:

- **EGS** (EGSnrc): `.egsphsp` files in MODE0 and MODE2, including suffixed variants (`.egsphsp1`, etc.)
- **IAEA**: `.IAEAphsp` International Atomic Energy Agency format with header files
- **TOPAS**: `.phsp` files in Binary, ASCII, and Limited variants
- **penEasy**: `.dat` ASCII format from the PENELOPE simulation code
- **ROOT** (optional): `.root` files with TOPAS/OpenGATE templates or custom branch mappings

Additional formats can be added through the extensible registry system without modifying core library code.

## Architecture

### Core Components

**`Particle` Class**: The central data model representing a particle with position, momentum, energy, weight, and type information. Supports both standard properties and format-specific extensions through a flexible property system.

**`PhaseSpaceFileReader`**: Abstract base class for reading phase space files. Implementations handle format-specific parsing while presenting a common streaming interface.

**`PhaseSpaceFileWriter`**: Abstract base class for writing phase space files. Handles format-specific serialization with proper history counting.

**`FormatRegistry`**: Plugin-style system for registering and creating readers/writers. Enables runtime format discovery and automatic format detection from file extensions.

**`ByteBuffer`**: High-performance binary I/O buffer for efficient reading of large files with configurable buffering strategies.

### Data Model

The `Particle` class provides access to:

- **Spatial coordinates**: Position (x, y, z) in configurable units
- **Momentum**: Direction cosines and kinetic energy
- **Particle properties**: PDG particle codes, statistical weight
- **History tracking**: Original history numbers and incremental counters
- **Format-specific data**: Extensible property system for specialized information

### Unit System

ParticleZoo includes a comprehensive unit system that ensures dimensional consistency across different formats:

```cpp
// Length units: mm, cm, m
float x_in_cm = particle.getX() / cm;
float y_in_mm = particle.getY() / mm;

// Energy units: eV, keV, MeV, GeV
float energy_MeV = particle.getKineticEnergy() / MeV;
```

## Building and Installation

### Prerequisites

- **Operating System**: Linux or macOS
- **Compiler**: C++20 compatible compiler (GCC 10+, Clang 13+)
- **Build Tools**: GNU Make
- **Optional Dependencies**: 
  - CERN ROOT (for ROOT format support) - requires `root-config` in PATH

### Build Process

```bash
# Configure the build system (auto-detects compiler and dependencies)
./configure [--prefix=/your/installation/prefix]

# Build the library and tools
make          # Release build (default)
make debug    # Debug build with symbols
make release  # Explicitly build release version

# Install (optional)
make install PREFIX=/usr/local
```

### Build Outputs

The build system creates the following artifacts:

**Release build** (`build/gcc/release/`):
- `libparticlezoo.a` - Static library
- Command-line tools: `PHSPConvert`, `PHSPCombine`, `PHSPImage`

**Debug build** (`build/gcc/debug/`):
- `libparticlezoo.a` - Static library with debug symbols
- Debug versions of command-line tools

**Installation** (if `make install` is run):
- Headers: `$PREFIX/include/particlezoo/`
- Library: `$PREFIX/lib/libparticlezoo.a`
- Executables: `$PREFIX/bin/PHSPConvert`, etc.

### Configuration Options

The `configure` script accepts the following options:

- `--prefix=PATH` - Installation prefix (default: `/usr/local`)
- `--no-root` - Disable ROOT support even if available

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
UserOptions options;
options["maxParticles"] = {"1000000"};
options["rootFormat"] = {"TOPAS"};

// Create reader with explicit format and options
auto reader = FormatRegistry::CreateReader("ROOT", "simulation.root", options);
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

// Particle type
int pdg_code = p.getPDGCode();     // PDG particle code
std::string name = p.getParticleName();  // Human-readable name

// History information
uint64_t history = p.getIncrementalHistoryNumber();
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

Converts phase space files between different formats:

```bash
# Auto-detect formats from file extensions
PHSPConvert input.egsphsp output.IAEAphsp

# Explicitly specify formats
PHSPConvert --inputFormat EGS --outputFormat IAEA input.file output.file

# Limit particle count
PHSPConvert --maxParticles 1000000 input.IAEAphsp output.phsp
```

### PHSPCombine - File Merging

Combines multiple phase space files into a single output:

```bash
# Combine multiple files
PHSPCombine --outputFile combined.IAEAphsp file1.egsphsp file2.egsphsp file3.egsphsp

# Mix formats during combination
PHSPCombine --outputFile result.phsp input1.IAEAphsp input2.egsphsp
```

### PHSPImage - Visualization

Creates 2D particle fluence or energy fluence images from phase space data. Can output either a detailed TIFF image with raw fluence data stored in 32-bit floats (default), or in a simple bitmap BMP image with automatic constrast for easy visualization:

```bash
# Generate XY plane image at Z=0
PHSPImage 0.0 beam.egsphsp fluence_map.tiff

# Custom plane and energy weighting
PHSPImage --outputFormat BMP --plane XZ --energyWeighted true 5.0 simulation.IAEAphsp dose_profile.bmp
```

## Extending the Library

### Adding New Formats

To add support for a new phase space format:

1. **Implement Reader**: Inherit from `PhaseSpaceFileReader` and implement virtual methods
2. **Implement Writer**: Inherit from `PhaseSpaceFileWriter` and implement virtual methods  
3. **Register Format**: Add registration call to connect file extensions with your implementations

Example registration:

```cpp
FormatRegistry::RegisterFormat(
    "MyFormat",                    // Format name
    {".myext"},                    // File extensions
    [](const std::string& file, const UserOptions& opts) -> std::unique_ptr<PhaseSpaceFileReader> {
        return std::make_unique<MyFormatReader>(file, opts);
    },
    [](const std::string& file, const UserOptions& opts) -> std::unique_ptr<PhaseSpaceFileWriter> {
        return std::make_unique<MyFormatWriter>(file, opts);
    }
);
```

### Custom Particle Properties

The `Particle` class supports custom properties through the property system:

```cpp
// Add custom integer property
particle.setIntProperty(IntPropertyType::CUSTOM, "my_custom_int", 42);

// Add custom float property  
particle.setFloatProperty(FloatPropertyType::CUSTOM, "my_custom_float", 3.14f);

// Add custom boolean property
particle.setBoolProperty(BoolPropertyType::CUSTOM, "my_custom_bool", true);
```

## ROOT Format Support (Optional)

When compiled with ROOT support, ParticleZoo can read and write ROOT-based phase space files using predefined templates or custom branch mappings.

### Predefined Templates

```bash
# Use TOPAS template
PHSPConvert --inputFormat ROOT --rootFormat TOPAS input.root output.IAEAphsp

# Use OpenGATE template  
PHSPConvert --inputFormat ROOT --rootFormat OpenGATE simulation.root converted.egsphsp
```

### Custom Branch Mapping

For ROOT files with non-standard branch names:

```bash
PHSPConvert --inputFormat ROOT \
  --rootTreeName MyTree \
  --rootEnergy E_kin \
  --rootPositionX pos_x \
  --rootPositionY pos_y \
  --rootPositionZ pos_z \
  --rootWeight stat_weight \
  input.root output.phsp
```

Available branch mapping options:
- `--rootTreeName <name>` - ROOT tree name
- `--rootEnergy <branch>` - Energy branch
- `--rootWeight <branch>` - Statistical weight branch  
- `--rootPositionX/Y/Z <branch>` - Position branches
- `--rootDirectionalCosineX/Y/Z <branch>` - Direction cosines
- `--rootNegativeZCosine <branch>` - Boolean flag for Z-direction sign
- `--rootPDGCode <branch>` - Particle type identifier

## Performance Considerations

### Memory Usage

- ParticleZoo uses streaming I/O to minimize memory footprint
- Configurable buffer sizes for optimal performance vs. memory trade-offs
- Large files can be processed with constant memory usage

### Optimization Tips

- Use binary formats when possible for faster I/O
- Consider particle limits (`--maxParticles`) for testing and prototyping
- Enable compiler optimizations (`make release`) for production use
- ROOT format may be slower due to tree structure overhead

## Troubleshooting

### Common Issues

**Build Problems:**
- *"config.status not found"* → Run `./configure` before `make`
- *"The C++ standard in this build does not match ROOT configuration"* → The ROOT installation on your system was compiled with a different C++ standard than ParticleZoo. It may still work, but it cannot be guaranteed. Either rebuild both with the same C++ standard or use at your own risk.
- *"ROOT support: no"* → Ensure `root-config` is in PATH and re-run `./configure`
- *"checking whether g++ accepts -std=c++20... no"* → Update compiler (GCC 10+ or Clang 13+)

**Runtime Issues:**
- *"Unknown format"* → Use `--inputFormat` to explicitly specify format
- *"File not found"* → Check file paths and permissions

**Performance Issues:**
- Large files processing slowly → Consider using `--maxParticles` for testing
- Memory usage too high → Check if streaming interface is being used properly

### Getting Help

For additional support:
1. Use `--formats` option to verify supported formats at runtime
2. Try explicit format specification with `--inputFormat`/`--outputFormat`
3. Verify file integrity using third-party format-specific validation tools if available

## License

This project is licensed under the MIT License. See the [LICENSE](./LICENSE) file for details.

Copyright (c) 2025 Daniel O'Brien
