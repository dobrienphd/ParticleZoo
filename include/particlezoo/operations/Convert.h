#pragma once

#include <string>
#include <optional>
#include <utility>
#include <cstdint>
#include <limits>

namespace ParticleZoo {

/**
 * @brief Options for the Convert operation.
 *
 * All fields have sensible defaults. File paths are passed as positional
 * parameters to Convert(). Numeric values that represent physical quantities
 * use the internal unit system — multiply by the constants in
 * particlezoo/utilities/units.h (e.g. @c 5.0f * cm, @c 1.0f * MeV).
 */
struct ConvertOptions {
    uint64_t maxParticles    = std::numeric_limits<uint32_t>::max(); ///< Maximum particles to convert (default: unlimited)
    std::string inputFormat  = "";                                   ///< Force input format; empty = auto-detect
    std::string outputFormat = "";                                   ///< Force output format; empty = auto-detect
    bool preserveConstants   = true;                                 ///< Preserve constant particle-property values from the input file

    // --- Projection (internal units, use * cm) ---
    std::optional<float> projectToX; ///< Project particles to this X coordinate
    std::optional<float> projectToY; ///< Project particles to this Y coordinate
    std::optional<float> projectToZ; ///< Project particles to this Z coordinate

    // --- Particle type filter (at most one may be set) ---
    bool photonsOnly   = false;          ///< Keep only photons
    bool electronsOnly = false;          ///< Keep only electrons
    std::optional<int32_t> filterByPDG; ///< Keep only particles with this PDG code

    // --- Energy filter (internal units, use * MeV) ---
    std::optional<float> minEnergy; ///< Minimum kinetic energy (inclusive)
    std::optional<float> maxEnergy; ///< Maximum kinetic energy (inclusive)

    // --- Position filter (internal units, use * cm) ---
    std::optional<float> minX; ///< Minimum X position (inclusive)
    std::optional<float> maxX; ///< Maximum X position (inclusive)
    std::optional<float> minY; ///< Minimum Y position (inclusive)
    std::optional<float> maxY; ///< Maximum Y position (inclusive)
    std::optional<float> minZ; ///< Minimum Z position (inclusive)
    std::optional<float> maxZ; ///< Maximum Z position (inclusive)

    // --- Radius filter (internal units, use * cm; XY-plane radius) ---
    std::optional<float> minRadius; ///< Minimum radial distance in the XY plane (inclusive)
    std::optional<float> maxRadius; ///< Maximum radial distance in the XY plane (inclusive)

    // --- Generation filter (at most one may be set) ---
    bool primariesOnly    = false; ///< Keep only primary particles (generation == 1)
    bool excludePrimaries = false; ///< Exclude primary particles
    std::optional<std::pair<int,int>> generations; ///< Keep particles whose generation is in [first, second]

    bool errorOnWarning = false; ///< Throw an exception for warnings instead of printing them
};

/**
 * @brief Convert a phase space file, optionally filtering or projecting particles.
 *
 * Reads particles from @p inputFile, applies any requested projections and
 * filters, then writes the accepted particles to @p outputFile. History
 * counts are preserved from the original file.
 *
 * @param inputFile   Input phase space file path.
 * @param outputFile  Output file path (must differ from @p inputFile).
 * @param options     Convert options (default: all defaults).
 * @throws std::runtime_error on invalid parameters, conflicting filters, or I/O errors.
 */
void Convert(const std::string& inputFile,
             const std::string& outputFile,
             const ConvertOptions& options = ConvertOptions{});

} // namespace ParticleZoo
