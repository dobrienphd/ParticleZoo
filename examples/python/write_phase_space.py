"""
Example python script to use ParticleZoo to write a phase space file.
Distributed as part of ParticleZoo. https://www.github.com/dobrienphd/ParticleZoo

MIT License

Copyright (c) 2025 Daniel O'Brien

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.


Usage:
  python write_phase_space.py <output_file> [options]

Examples:
  python write_phase_space.py output.IAEAphsp
  python write_phase_space.py output.root --ROOT-format OpenGATE
  python write_phase_space.py output.phsp --TOPAS-format binary

This script demonstrates how to create a phase space file writer and write particles.
"""

from __future__ import annotations
import sys
import math

import particlezoo as pz


def create_sample_particles(count: int = 100) -> list[pz.Particle]:
    """Create a list of sample particles for demonstration."""
    particles = []
    
    for i in range(count):
        # Create electrons with varying energies and positions
        # Simulate a simple beam diverging from the origin
        angle = (i / count) * 2 * pz.PI
        energy = (1.0 + (i % 10) * 0.5) * pz.MeV  # 1.0 to 5.5 MeV
        
        # Position on a plane at z=0
        x = math.cos(angle) * 0.5 * pz.cm  # 0.5 cm radius
        y = math.sin(angle) * 0.5 * pz.cm
        z = 0.0 * pz.cm
        
        # Direction slightly diverging
        theta = (i / count) * 0.1 * pz.radian  # small divergence angle
        phi = angle
        dir_x = math.sin(theta) * math.cos(phi)
        dir_y = math.sin(theta) * math.sin(phi)
        dir_z = math.cos(theta)
        
        # Create electron (PDG code 11)
        particle = pz.particle_from_pdg(
            pdg=11,
            kineticEnergy=energy,
            x=x, y=y, z=z,
            px=dir_x, py=dir_y, pz=dir_z,
            isNewHistory=(i % 5 == 0),  # New history every 5 particles
            weight=1.0
        )
        particles.append(particle)
    
    return particles


def main(argv: list[str]) -> int:
    usage_message = (
        "Usage: write_phase_space.py <output_file> [options]\n\n"
        "Example script to write phase space files.\n"
    )
    
    # Extract particle count option manually (not a format-specific option)
    particle_count = 100
    clean_argv = []
    skip_next = False
    for i, arg in enumerate(argv):
        if skip_next:
            skip_next = False
            continue
        if arg == "--count":
            if i + 1 < len(argv):
                try:
                    particle_count = int(argv[i + 1])
                    skip_next = True
                    continue
                except ValueError:
                    pass
        clean_argv.append(arg)
    
    # Parse remaining arguments through C++ parser (requires at least 1 positional arg: the file path)
    options = pz.ArgParser.parse_args(clean_argv, usage_message, min_positional_args=1)
    
    # Extract the file path (first positional argument)
    output_path = options.extract_positional(0)
    
    print(f"Creating writer for: {output_path}")
    
    # Optional: Set up fixed values (constant properties for all particles)
    # This is useful for optimization when certain values don't change
    fixed_values = pz.FixedValues()
    # Example: If all particles have the same Z position
    fixed_values.z_is_constant = True
    fixed_values.constant_z = 0.0 * pz.cm
    
    # Create writer with parsed options - format detection and options handled automatically
    writer = pz.create_writer(output_path, options, fixed_values)
    
    print(f"Format: {writer.get_phsp_format()}")
    print(f"Maximum supported particles: {writer.get_maximum_supported_particles()}")
    print(f"\nGenerating and writing {particle_count} particles...")
    
    # Create and write sample particles
    batch_size = 1000  # Adjust based on memory constraints
    batches = particle_count // batch_size
    remainder = particle_count % batch_size
    for _ in range(batches):
        particles = create_sample_particles(batch_size)
        for particle in particles:
            writer.write_particle(particle)
    if remainder > 0:
        particles = create_sample_particles(remainder)
        for particle in particles:
            writer.write_particle(particle)

    # Close the writer (flushes buffers and finalizes the file)
    writer.close()
    
    print(f"\nSuccessfully wrote {writer.get_particles_written()} particles")
    print(f"Total histories: {writer.get_histories_written()}")
    print(f"Output file: {writer.get_file_name()}")
    
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
