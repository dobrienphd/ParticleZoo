#!/bin/bash
# Script to generate the doxygen documentation and copy the README and LICENSE files to the docs folder
# Prerequisites: doxygen, LaTeX, gawk
#   On Debian based systems, you can install these with:
#     sudo apt-get install doxygen graphviz texlive-latex-base texlive-latex-recommended texlive-latex-extra texlive-fonts-recommended gawk
# Run from the docs/scripts folder
# Usage: ./gendocs.sh

# Exit on any error
set -e

# Navigate to the root of the project
cd ../../

# Before continuing, confirm we are in the root of the project by checking for Doxyfile and for include/particlezoo
if [ ! -d "include/particlezoo" ] || [ ! -f "Doxyfile" ]; then
  echo "Error: This script must be run from the docs/scripts folder within the ParticleZoo project."
  exit 1
fi

# Copy README and LICENSE to docs folder
cp README.md docs/
cp LICENSE docs/License

# Remove the license badge and license section from README.md in docs
sed -i '/\[\!\[License: MIT\]/d' docs/README.md
sed -i '/^## License$/,$d' docs/README.md

# Extract version number from version.h
VERSION=$(gawk '
/MAJOR_VERSION.*=/ {major=$NF; gsub(/[^0-9]/,"",major)} 
/MINOR_VERSION.*=/ {minor=$NF; gsub(/[^0-9]/,"",minor)} 
/PATCH_VERSION.*=/ {patch=$NF; gsub(/[^0-9]/,"",patch)} 
/CAVEAT.*=/ {
    # Extract everything between quotes
    if (match($0, /"([^"]*)"/, arr)) {
        caveat = arr[1]
    }
} 
END {
    version = "v" major "." minor "." patch
    if (caveat != "" && caveat != " ") version = version " " caveat
    print version
}' include/particlezoo/utilities/version.h)

# Trim any additional whitespace from VERSION
VERSION=$(echo "$VERSION" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')

# Update PROJECT_NUMBER in Doxyfile with the current version
sed -i "s/^PROJECT_NUMBER.*=.*/PROJECT_NUMBER         = $VERSION/" Doxyfile

# Update fancyfoot text in custom_doxygen.sty with the current version
sed -i "s/ParticleZoo Reference Manual v[0-9.]*[^}]*/ParticleZoo Reference Manual $VERSION/g" docs/scripts/custom_doxygen.sty

# Generate Python API documentation
echo "Generating Python API documentation..."
python3 docs/scripts/gen_python_api.py
if [ $? -ne 0 ]; then
  echo "Warning: Python API generation had issues, continuing anyway..."
fi

# Clean up old docs (no errors if they don't exist)
rm -f docs/*.pdf
rm -rf docs/latex
rm -rf docs/html

# Generate the docs
doxygen Doxyfile && make -C docs/latex LATEX_CMD="pdflatex -interaction=nonstopmode" clean all
if [ $? -eq 0 ]; then
    cp docs/latex/refman.pdf "docs/ParticleZoo Reference Manual $VERSION.pdf"
    # Clean up intermediate files
    rm -f docs/README.md docs/License
    rm -rf docs/latex
    rm docs/scripts/python_appendix.tex
    echo "----------------------------------------"
    echo "Documentation generated successfully: docs/ParticleZoo Reference Manual $VERSION.pdf"
else
    echo "----------------------------------------"
    echo "ERROR: PDF compilation failed. Log preserved in docs/latex/refman.log"
    exit 1
fi