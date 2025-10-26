# Python bindings for ParticleZoo

This directory contains a Python package that exposes the C++ ParticleZoo API using pybind11.

## Build prerequisites
- Python 3.8+
- A C++20 compiler (g++ or clang++)
- pybind11 (will be installed automatically by pip)

## Setup
Create a new Python virtual environment and activate it by running the following commands:

```bash
python -m venv .venv
. .venv/bin/activate
python -m pip install -U pip setuptools wheel pybind11
```

## Build and install (editable)
From the repository root or from this folder:

```bash
python -m pip install -e python
```

This will compile an extension module and install a `particlezoo` Python package that links against the library sources in this repo.

## Quickstart
```python
import particlezoo as pz

print(pz.get_particle_type_name(pz.ParticleType.Photon))

p = pz.Particle(pz.ParticleType.Electron, 6.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0)
print(p.kinetic_energy)

reader = pz.IAEAReader("/path/to/file.IAEAphsp")
for i, part in enumerate(reader):
    # Do something with part
    if i > 10:
        break
```

## Notes
- The extension compiles its own copy of the C++ sources; it does not require installing the static library first.