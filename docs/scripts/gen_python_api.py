#!/usr/bin/env python3
"""
Automated Python API documentation generation for pybind11 bindings.
Generates a LaTeX chapter that can be appended to Doxygen documentation.
"""

import subprocess
import sys
import os
import re
import inspect
from pathlib import Path

def run_command(cmd, cwd=None, check=True):
    """Run a command and return output"""
    result = subprocess.run(cmd, shell=True, cwd=cwd, capture_output=True, text=True, check=False)
    if check and result.returncode != 0:
        raise subprocess.CalledProcessError(result.returncode, cmd, result.stdout, result.stderr)
    return result.stdout, result.stderr, result.returncode

def main():
    root_dir = Path(__file__).parent.parent.parent
    python_dir = root_dir / "python"
    docs_dir = root_dir / "docs"
    
    print("="*60)
    print("AUTOMATED PYTHON API DOCUMENTATION GENERATION")
    print("="*60)
    
    # Step 1: Build the Python module
    print("\n[1/5] Building Python module...")
    os.chdir(python_dir)
    stdout, stderr, code = run_command(f"{sys.executable} -m pip install --break-system-packages -e . -q", check=False)
    if code != 0:
        print(f"ERROR: Failed to build module\n{stderr}")
        print(f"STDOUT: {stdout}")
        print(f"STDERR: {stderr}")        
        return 1
    print("✓ Module built successfully")
    
    # Step 2: Generate stub files with pybind11-stubgen
    print("\n[2/5] Generating type stubs...")
    run_command(f"{sys.executable} -m pip install --break-system-packages pybind11-stubgen -q")
    
    stub_dir = python_dir / "stubs"
    stub_dir.mkdir(exist_ok=True)
    
    stdout, stderr, code = run_command(
        f"{sys.executable} -m pybind11_stubgen particlezoo._pz -o {stub_dir} --ignore-invalid=all",
        check=False
    )
    if code != 0:
        print(f"WARNING: Stubgen had issues, continuing anyway...")
    print("✓ Type stubs generated")
    
    # Step 3: Create Sphinx documentation setup
    print("\n[3/5] Setting up Sphinx documentation...")
    run_command(f"{sys.executable} -m pip install --break-system-packages sphinx sphinx-rtd-theme -q")
    
    sphinx_dir = docs_dir / "python_sphinx_tmp"
    sphinx_dir.mkdir(exist_ok=True, parents=True)
    
    # Create conf.py
    conf_py = sphinx_dir / "conf.py"
    conf_py.write_text(f'''
import sys
import os

# Add paths
sys.path.insert(0, os.path.abspath('{python_dir}'))
sys.path.insert(0, os.path.abspath('{stub_dir}'))

project = 'ParticleZoo Python API'
extensions = ['sphinx.ext.autodoc', 'sphinx.ext.napoleon', 'sphinx.ext.autosummary']

# Mock the C++ extension module
autodoc_mock_imports = ['particlezoo._pz']

# Autodoc settings
autodoc_default_options = {{
    'members': True,
    'undoc-members': True,
    'show-inheritance': True,
    'member-order': 'bysource',
}}

autosummary_generate = True

# LaTeX output
latex_documents = [
    ('index', 'python_api.tex', 'Python API', 'ParticleZoo', 'manual'),
]

latex_elements = {{
    'preamble': r'\\\\usepackage{{multicol}}',
    'printindex': '',
    'tableofcontents': '',
}}
''')
    
    # Create index.rst with actual module introspection
    print("  Introspecting module...")
    
    try:
        import particlezoo as pz
        
        # Get all public members
        all_members = [name for name in dir(pz) if not name.startswith('_')]
        
        # Categorize
        classes = []
        enums = []
        functions = []
        constants = []
        
        for name in all_members:
            obj = getattr(pz, name)
            obj_type = type(obj).__name__
            
            if 'class' in str(type(obj)).lower() and not 'enum' in str(type(obj)).lower():
                classes.append(name)
            elif 'enum' in str(type(obj)).lower():
                enums.append(name)
            elif callable(obj):
                functions.append(name)
            elif isinstance(obj, (int, float, str)):
                constants.append(name)
        
        # Generate RST
        index_rst = sphinx_dir / "index.rst"
        rst_content = """ParticleZoo Python API Reference
===================================

This documentation is automatically generated from the Python bindings.

Installation
------------

.. code-block:: bash

   pip install -e python/

Quick Start
-----------

.. code-block:: python

   import particlezoo as pz
   
   # Create a particle
   particle = pz.Particle(
       pz.ParticleType.Electron,
       6.0,  # MeV kinetic energy
       0.0, 0.0, 0.0,  # position (cm)
       0.0, 0.0, 1.0   # direction cosines
   )
   
   # Read phase space file
   reader = pz.create_reader("file.IAEAphsp")
   for p in reader:
       print(f"Energy: {p.kinetic_energy} MeV")

"""
        
        if classes:
            rst_content += "\nClasses\n-------\n\n"
            for cls in sorted(classes):
                rst_content += f".. autoclass:: particlezoo.{cls}\n"
                rst_content += "   :members:\n"
                rst_content += "   :undoc-members:\n"
                rst_content += "   :show-inheritance:\n\n"
        
        if enums:
            rst_content += "\nEnumerations\n------------\n\n"
            for enum in sorted(enums):
                rst_content += f".. autoclass:: particlezoo.{enum}\n"
                rst_content += "   :members:\n"
                rst_content += "   :undoc-members:\n\n"
        
        if functions:
            rst_content += "\nFunctions\n---------\n\n"
            for func in sorted(functions):
                rst_content += f".. autofunction:: particlezoo.{func}\n\n"
        
        if constants:
            rst_content += "\nPhysical Units and Constants\n----------------------------\n\n"
            rst_content += "The following constants are available for unit conversions:\n\n"
            rst_content += ".. code-block:: python\n\n"
            for const in sorted(constants)[:20]:  # Show first 20
                val = getattr(pz, const)
                rst_content += f"   pz.{const} = {val}\n"
            if len(constants) > 20:
                rst_content += f"\n   ... and {len(constants) - 20} more\n"
        
        index_rst.write_text(rst_content)
        print("  ✓ Module introspected successfully")
        
    except Exception as e:
        print(f"  WARNING: Could not introspect module: {e}")
        print("  Using basic template...")
        index_rst = sphinx_dir / "index.rst"
        index_rst.write_text("""
ParticleZoo Python API Reference
===================================

.. automodule:: particlezoo
   :members:
   :undoc-members:
   :show-inheritance:
""")
    
    print("✓ Sphinx configuration ready")
    
    # Step 4: Build LaTeX
    print("\n[4/5] Building LaTeX documentation...")
    stdout, stderr, code = run_command(
        f"{sys.executable} -m sphinx -b latex {sphinx_dir} {sphinx_dir}/_build/latex -q",
        check=False
    )
    if code != 0:
        print(f"WARNING: Sphinx build had warnings/errors:\n{stderr}")
    
    latex_file = sphinx_dir / "_build/latex/python_api.tex"
    if not latex_file.exists():
        print(f"ERROR: LaTeX file not generated at {latex_file}")
        return 1
    print("✓ LaTeX documentation built")
    
    # Step 5: Generate simple LaTeX chapter from module
    print("\n[5/5] Generating LaTeX chapter...")
    
    try:
        import particlezoo as pz
        
        # Get all public members
        all_members = [name for name in dir(pz) if not name.startswith('_')]
        
        # Categorize
        classes = []
        enums = []
        functions = []
        
        for name in all_members:
            obj = getattr(pz, name)
            obj_type = str(type(obj))
            
            # Skip all units (numeric constants - int or float)
            if isinstance(obj, (int, float)):
                continue
            
            # Skip submodules
            if inspect.ismodule(obj):
                continue
            
            # Skip ParticleType enum members (they're attributes of ParticleType class)
            if 'ParticleType' in obj_type:
                continue
            
            # Skip the ParticleType class itself (contains ~200+ enum members in docstring)
            if name == 'ParticleType':
                continue
            
            # Check callable first, then enums, then classes
            if callable(obj) and not ('class' in obj_type.lower() and 'type' in obj_type.lower()):
                # It's a function/method (callable but not a class/type itself)
                functions.append(name)
            elif 'enum' in obj_type.lower():
                enums.append(name)
            elif 'class' in obj_type.lower() or inspect.isclass(obj):
                classes.append(name)
        
        print(f"  Found {len(classes)} classes, {len(enums)} enums, {len(functions)} functions")
        
        # Function to escape LaTeX special characters
        def escape_latex(text):
            """Escape special LaTeX characters in text."""
            if not text:
                return text
            # Order matters - backslash first
            replacements = [
                ('\\', r'\textbackslash{}'),
                ('&', r'\&'),
                ('%', r'\%'),
                ('$', r'\$'),
                ('#', r'\#'),
                ('_', r'\_'),
                ('{', r'\{'),
                ('}', r'\}'),
                ('~', r'\textasciitilde{}'),
                ('^', r'\textasciicircum{}'),
                ('<', r'\textless{}'),
                ('>', r'\textgreater{}'),
            ]
            for old, new in replacements:
                text = text.replace(old, new)
            return text
        
        # Generate LaTeX content
        output = r'\chapter{Python API Reference}' + '\n'
        output += r'\label{chap:python_api}' + '\n\n'
        output += 'This chapter documents the Python bindings for ParticleZoo.\\\\[1em]\n\n'
        
        output += r'\section{Installation}' + '\n\n'
        output += 'The python module \'particlezoo\' can be installed either in a virtual environment or in the user site-packages. In other case, you can do the following from the project root directory:\n\n'
        output += r'\begin{verbatim}' + '\n'
        output += './configure\n'
        output += 'make install-python      # Standard install\n'
        output += '# or\n'
        output += 'make install-python-dev  # Development mode (editable install)\n'
        output += r'\end{verbatim}' + '\n\n'
        output += 'Note: These commands will prompt before using \\texttt{--break-system-packages} if no virtual environment is detected.\n\n'
        
        output += r'\section{Quick Start}' + '\n\n'
        output += r'\subsection{Reading a Phase Space File}' + '\n\n'
        output += r'\begin{verbatim}' + '\n'
        output += 'import particlezoo as pz\n\n'
        output += '# Read phase space file with automatic format detection\n'
        output += 'reader = pz.create_reader("input.egsphsp")\n'
        output += 'for p in reader:\n'
        output += '    ke_mev = p.kinetic_energy / pz.MeV\n'
        output += '    print(f"Energy: {ke_mev:.3f} MeV")\n'
        output += 'reader.close()\n'
        output += r'\end{verbatim}' + '\n\n'
        output += r'\subsection{Writing a Phase Space File}' + '\n\n'
        output += r'\begin{verbatim}' + '\n'
        output += 'import particlezoo as pz\n\n'
        output += '# Create a writer with automatic format detection\n'
        output += 'writer = pz.create_writer("output.IAEAphsp")\n\n'
        output += '# Create and write particles\n'
        output += 'for i in range(100):\n'
        output += '    particle = pz.particle_from_pdg(\n'
        output += '        pdg=11,                    # electron\n'
        output += '        kineticEnergy=6.0 * pz.MeV,\n'
        output += '        x=0.0 * pz.cm,\n'
        output += '        y=0.0 * pz.cm,\n'
        output += '        z=0.0 * pz.cm,\n'
        output += '        px=0.0, py=0.0, pz=1.0,\n'
        output += '        isNewHistory=(i == 0),\n'
        output += '        weight=1.0\n'
        output += '    )\n'
        output += '    writer.write_particle(particle)\n\n'
        output += 'writer.close()\n'
        output += 'print(f"Wrote {writer.get_particles_written()} particles")\n'
        output += r'\end{verbatim}' + '\n\n'
        
        if classes:
            output += r'\section{Classes}' + '\n\n'
            for cls in sorted(classes):
                cls_escaped = escape_latex(cls)
                output += r'\subsection{' + cls_escaped + '}' + '\n\n'
                try:
                    cls_obj = getattr(pz, cls)
                    doc = cls_obj.__doc__
                    if doc:
                        output += escape_latex(doc.strip()) + '\n\n'
                    
                    # Check if this is an enum class by checking if members are instances of the class itself
                    is_enum = False
                    public_members = [name for name in dir(cls_obj) if not name.startswith('_')]
                    if public_members:
                        # Sample a few members to check if they're enum values
                        for name in public_members[:3]:
                            if name not in ['name', 'value']:  # Skip common enum methods
                                try:
                                    member = getattr(cls_obj, name)
                                    if type(member).__name__ == cls:
                                        is_enum = True
                                        break
                                except:
                                    pass
                    
                    # For enums, the "Members" section was already printed in the docstring
                    # So we skip extracting individual properties/methods
                    if is_enum:
                        # Enum members are already documented in the class docstring with "Members:" section
                        pass
                    else:
                        # Get class members (properties, methods, etc.) for non-enum classes
                        members = []
                        for name in dir(cls_obj):
                            # Skip private/internal members
                            if name.startswith('_'):
                                continue
                            try:
                                member = getattr(cls_obj, name)
                                member_doc = getattr(member, '__doc__', None)
                                if member_doc:
                                    members.append((name, member, member_doc))
                            except:
                                pass
                        
                        if members:
                            # Separate into properties/attributes and methods
                            properties = []
                            methods = []
                            for name, member, doc in members:
                                # Check if it's a method (callable) or property
                                if callable(member):
                                    methods.append((name, doc))
                                else:
                                    properties.append((name, doc))
                            
                            # Document properties
                            if properties:
                                output += r'\subsubsection*{Properties}' + '\n\n'
                                output += r'\begin{itemize}' + '\n'
                                for prop_name, prop_doc in sorted(properties):
                                    prop_escaped = escape_latex(prop_name)
                                    # Clean up the docstring
                                    clean_doc = prop_doc.strip().split('\n')[0] if prop_doc else ''
                                    doc_escaped = escape_latex(clean_doc)
                                    output += f"  \\item \\texttt{{{prop_escaped}}} -- {doc_escaped}\n"
                                output += r'\end{itemize}' + '\n\n'
                            
                            # Document methods
                            if methods:
                                output += r'\subsubsection*{Methods}' + '\n\n'
                                for method_name, method_doc in sorted(methods):
                                    method_escaped = escape_latex(method_name)
                                    output += f"\\textbf{{{method_escaped}()}}\\\\[0.5em]\n\n"
                                    
                                    # Parse docstring - first line is usually signature
                                    doc_lines = method_doc.strip().split('\n')
                                    if len(doc_lines) > 1:
                                        # Skip the signature line (often autogenerated by pybind11)
                                        description_lines = [line for line in doc_lines[1:] if line.strip()]
                                        if description_lines:
                                            description = ' '.join(description_lines)
                                            output += escape_latex(description.strip()) + '\n\n'
                                    else:
                                        # Just one line
                                        output += escape_latex(doc_lines[0].strip()) + '\n\n'
                
                except Exception as e:
                    print(f"Warning: Could not document class {cls}: {e}")
                    pass
        
        if enums:
            output += r'\section{Enumerations}' + '\n\n'
            for enum in sorted(enums):
                enum_escaped = escape_latex(enum)
                output += r'\subsection{' + enum_escaped + '}' + '\n\n'
                try:
                    doc = getattr(pz, enum).__doc__
                    if doc:
                        output += escape_latex(doc.strip()) + '\n\n'
                except:
                    pass
        
        if functions:
            output += r'\section{Functions}' + '\n\n'
            for func in sorted(functions):
                func_escaped = escape_latex(func)
                output += r'\subsection{' + func_escaped + '()}' + '\n\n'
                try:
                    doc = getattr(pz, func).__doc__
                    if doc:
                        # Split docstring into signature and description
                        lines = doc.strip().split('\n')
                        if lines:
                            # First line is usually the signature - keep it simpler
                            signature = lines[0]
                            # For very long signatures, just show basic info
                            if len(signature) > 80:
                                # Extract just the function name and basic params
                                if '(' in signature:
                                    func_name = signature.split('(')[0]
                                    output += f"\\textbf{{{escape_latex(func_name)}}}\\\\[0.5em]\n\n"
                                else:
                                    output += f"{escape_latex(signature)}\\\\[0.5em]\n\n"
                            else:
                                output += f"{escape_latex(signature)}\\\\[0.5em]\n\n"
                            # Add description if there's more than just the signature
                            if len(lines) > 2:
                                desc = '\n'.join(lines[2:]).strip()
                                if desc:
                                    output += escape_latex(desc) + '\n\n'
                except:
                    pass
        
        output_file = docs_dir / "scripts/python_appendix.tex"
        output_file.write_text(output)
        
        print(f"✓ Python API appendix generated: {output_file}")
        
    except Exception as e:
        print(f"ERROR: Could not generate simple LaTeX: {e}")
        return 1
    
    # Cleanup
    import shutil
    shutil.rmtree(sphinx_dir, ignore_errors=True)
    shutil.rmtree(stub_dir, ignore_errors=True)
    
    print("\n" + "="*60)
    print("SUCCESS! Appendix ready to include in Doxygen PDF")
    print("="*60)
    
    return 0

if __name__ == '__main__':
    sys.exit(main())