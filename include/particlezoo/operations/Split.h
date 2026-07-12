#pragma once

#include <string>

#include "particlezoo/utilities/argParse.h"

namespace ParticleZoo {

/**
 * @brief Options for the Split operation.
 *
 * All fields have sensible defaults. The input file path and split count
 * are passed as positional parameters to Split().
 */
struct SplitOptions {
    std::string inputFormat  = ""; ///< Force input format; empty = auto-detect from extension
    std::string outputFormat = ""; ///< Force output format; empty = same format as input
    UserOptions formatOptions{};   ///< Format-specific reader/writer options (e.g. EGS mode, MCNP title card)
};

/**
 * @brief Split a phase space file into multiple roughly equal-sized files.
 *
 * Divides the input file into @p splitNumber parts, respecting history
 * boundaries so that no history is split across files. Output files are
 * named automatically as @c stem_PartXX.ext (1-based, zero-padded index).
 * Fixed (constant) particle-property values are always preserved.
 *
 * @param inputFile   Path to the input phase space file.
 * @param splitNumber Number of output files to produce (must be > 1).
 * @param options     Split options (default: all defaults).
 * @throws std::runtime_error on invalid parameters or I/O errors.
 */
void Split(const std::string& inputFile,
           int splitNumber,
           const SplitOptions& options = SplitOptions{});

} // namespace ParticleZoo
