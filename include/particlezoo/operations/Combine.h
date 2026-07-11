#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <limits>

namespace ParticleZoo {

/**
 * @brief Options for the Combine operation.
 *
 * All fields have sensible defaults; only fields that deviate from the
 * default need to be set. File paths are passed as positional parameters
 * to Combine() rather than being stored here.
 */
struct CombineOptions {
    uint64_t maxParticles   = std::numeric_limits<uint64_t>::max(); ///< Maximum particles to process across all files (default: unlimited)
    std::string inputFormat  = "";                                   ///< Force input format; empty = auto-detect from extension
    std::string outputFormat = "";                                   ///< Force output format; empty = auto-detect from extension
    bool preserveConstants   = true;                                 ///< Preserve constant particle-property values from the input files
};

/**
 * @brief Combine multiple phase space files into a single output file.
 *
 * Reads particles from each input file in order and writes them to the
 * output file, optionally converting between formats. History counts are
 * properly accumulated from all input files. Processing stops early if
 * @p options.maxParticles is reached.
 *
 * @param inputFiles  One or more input phase space file paths.
 * @param outputFile  Output file path.
 * @param options     Combine options (default: all defaults).
 * @throws std::runtime_error on invalid parameters or I/O errors.
 */
void Combine(const std::vector<std::string>& inputFiles,
             const std::string& outputFile,
             const CombineOptions& options = CombineOptions{});

} // namespace ParticleZoo
