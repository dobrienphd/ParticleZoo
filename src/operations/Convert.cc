#include "particlezoo/operations/Convert.h"

#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>

#include "particlezoo/utilities/formats.h"
#include "particlezoo/utilities/progress.h"
#include "particlezoo/PhaseSpaceFileReader.h"
#include "particlezoo/PhaseSpaceFileWriter.h"
#include "particlezoo/PDGParticleCodes.h"

#include "GenerationFilter.h"

namespace ParticleZoo {

// Internal helpers kept in anonymous namespace
namespace {

struct InternalConvertConfig {
    const bool          projectToX;
    const bool          projectToY;
    const bool          projectToZ;
    const float         projectToXValue;
    const float         projectToYValue;
    const float         projectToZValue;
    const ParticleType  filterByParticle;
    const bool          filterByEnergy;
    const bool          filterByPosition;
    const bool          filterByRadius;
    const GenerationFilter generationFilter;
    const float         minimumEnergy;
    const float         maximumEnergy;
    const float         minimumX;
    const float         maximumX;
    const float         minimumY;
    const float         maximumY;
    const float         minimumZ;
    const float         maximumZ;
    const float         minimumRadius;
    const float         maximumRadius;

    explicit InternalConvertConfig(const ConvertOptions& opts)
        : projectToX(opts.projectToX.has_value()),
          projectToY(opts.projectToY.has_value()),
          projectToZ(opts.projectToZ.has_value()),
          projectToXValue(opts.projectToX.value_or(0.0f)),
          projectToYValue(opts.projectToY.value_or(0.0f)),
          projectToZValue(opts.projectToZ.value_or(0.0f)),
          filterByParticle(determineParticleFilter(opts)),
          filterByEnergy(opts.minEnergy.has_value() || opts.maxEnergy.has_value()),
          filterByPosition(opts.minX.has_value() || opts.maxX.has_value() ||
                           opts.minY.has_value() || opts.maxY.has_value() ||
                           opts.minZ.has_value() || opts.maxZ.has_value()),
          filterByRadius(opts.minRadius.has_value() || opts.maxRadius.has_value()),
          generationFilter(GenerationFilter::Resolve(opts.primariesOnly, opts.excludePrimaries, opts.generations)),
          minimumEnergy(opts.minEnergy.value_or(0.0f)),
          maximumEnergy(opts.maxEnergy.value_or(std::numeric_limits<float>::max())),
          minimumX(opts.minX.value_or(std::numeric_limits<float>::lowest())),
          maximumX(opts.maxX.value_or(std::numeric_limits<float>::max())),
          minimumY(opts.minY.value_or(std::numeric_limits<float>::lowest())),
          maximumY(opts.maxY.value_or(std::numeric_limits<float>::max())),
          minimumZ(opts.minZ.value_or(std::numeric_limits<float>::lowest())),
          maximumZ(opts.maxZ.value_or(std::numeric_limits<float>::max())),
          minimumRadius(opts.minRadius.value_or(0.0f)),
          maximumRadius(opts.maxRadius.value_or(std::numeric_limits<float>::max()))
    {}

    bool useProjection() const { return projectToX || projectToY || projectToZ; }

private:
    static ParticleType determineParticleFilter(const ConvertOptions& opts) {
        if (opts.photonsOnly)   return ParticleType::Photon;
        if (opts.electronsOnly) return ParticleType::Electron;
        if (opts.filterByPDG.has_value())
            return getParticleTypeFromPDGID(opts.filterByPDG.value());
        return ParticleType::Unsupported;
    }

};

bool applyFilters(const Particle& particle, const InternalConvertConfig& config) {
    if (config.filterByParticle != ParticleType::Unsupported && config.filterByParticle != particle.getType())
        return false;

    if (config.filterByEnergy) {
        const float energy = particle.getKineticEnergy();
        if (energy < config.minimumEnergy || energy > config.maximumEnergy)
            return false;
    }

    if (config.filterByPosition) {
        const float x = particle.getX();
        const float y = particle.getY();
        const float z = particle.getZ();
        if (x < config.minimumX || x > config.maximumX ||
            y < config.minimumY || y > config.maximumY ||
            z < config.minimumZ || z > config.maximumZ)
            return false;
    }

    if (config.filterByRadius) {
        const float x = particle.getX();
        const float y = particle.getY();
        const float radius = std::sqrt(x*x + y*y);
        if (radius < config.minimumRadius || radius > config.maximumRadius)
            return false;
    }

    if (config.generationFilter.useFilter) {
        if (particle.hasIntProperty(IntPropertyType::GENERATION)) {
            const int generation = particle.getIntProperty(IntPropertyType::GENERATION);
            if (generation < config.generationFilter.minimumGeneration ||
                generation > config.generationFilter.maximumGeneration)
                return false;
        } else {
            return false;
        }
    }

    return true;
}

void validateOptions(const std::string& inputFile, const std::string& outputFile, const ConvertOptions& opts) {
    if (inputFile.empty())  throw std::runtime_error("No input file specified.");
    if (outputFile.empty()) throw std::runtime_error("No output file specified.");
    if (inputFile == outputFile) throw std::runtime_error("Input and output files must be different.");

    if (opts.filterByPDG.has_value() &&
        getParticleTypeFromPDGID(opts.filterByPDG.value()) == ParticleType::Unsupported)
        throw std::runtime_error("Invalid PDG code specified for particle filter.");

    int typeFilters = (opts.photonsOnly ? 1 : 0) + (opts.electronsOnly ? 1 : 0) + (opts.filterByPDG.has_value() ? 1 : 0);
    if (typeFilters > 1) throw std::runtime_error("Conflicting particle filter options specified.");

    if (opts.minEnergy.has_value() && opts.maxEnergy.has_value() && opts.minEnergy.value() > opts.maxEnergy.value())
        throw std::runtime_error("Minimum energy cannot be greater than maximum energy for energy filter.");
    if (opts.minX.has_value() && opts.maxX.has_value() && opts.minX.value() > opts.maxX.value())
        throw std::runtime_error("Minimum X position cannot be greater than maximum X position for position filter.");
    if (opts.minY.has_value() && opts.maxY.has_value() && opts.minY.value() > opts.maxY.value())
        throw std::runtime_error("Minimum Y position cannot be greater than maximum Y position for position filter.");
    if (opts.minZ.has_value() && opts.maxZ.has_value() && opts.minZ.value() > opts.maxZ.value())
        throw std::runtime_error("Minimum Z position cannot be greater than maximum Z position for position filter.");
    if (opts.minRadius.has_value() && opts.maxRadius.has_value() && opts.minRadius.value() > opts.maxRadius.value())
        throw std::runtime_error("Minimum radius cannot be greater than maximum radius for radius filter.");

    // Generation-filter conflicts and range are validated by GenerationFilter::Resolve
    // when the internal config is constructed (before any file I/O).
}

} // anonymous namespace


void Convert(const std::string& inputFile,
             const std::string& outputFile,
             const ConvertOptions& options)
{
    constexpr uint64_t MAX_PERCENTAGE = 100;

    validateOptions(inputFile, outputFile, options);
    const InternalConvertConfig config(options);

    std::unique_ptr<PhaseSpaceFileReader> reader;
    std::unique_ptr<PhaseSpaceFileWriter> writer;
    std::vector<std::string> errorMessages;
    std::vector<std::string> warningMessages;

    try {
        reader = FormatRegistry::CreateReader(options.inputFormat, inputFile, options.formatOptions);

        const FixedValues fixedValues = options.preserveConstants ? reader->getFixedValues() : FixedValues{};

        writer = FormatRegistry::CreateWriter(options.outputFormat, outputFile, options.formatOptions, fixedValues);

        std::cout << "Converting particles from "
                  << inputFile << " (" << reader->getPHSPFormat() << ") to "
                  << outputFile << " (" << writer->getPHSPFormat() << ")..." << std::endl;

        uint64_t particlesInFile = reader->getNumberOfParticles();
        uint64_t particlesToRead = std::min(static_cast<uint64_t>(options.maxParticles), particlesInFile);
        uint64_t particlesRejected = 0;
        uint64_t particlesRejectedByProjection = 0;
        bool readPartialFile = particlesToRead < particlesInFile;

        uint64_t progressUpdateInterval = particlesToRead >= MAX_PERCENTAGE
                                        ? particlesToRead / MAX_PERCENTAGE
                                        : 1;

        auto startTime = std::chrono::steady_clock::now();

        if (particlesToRead > 0) {
            Progress<uint64_t> progress(particlesToRead);
            progress.Start("Converting:");

            while (reader->hasMoreParticles() && (!readPartialFile || reader->getParticlesRead() < particlesToRead)) {
                Particle particle = reader->getNextParticle();
                bool particleRejected = false;

                if (config.useProjection()) {
                    bool projectionSuccess = particle.getType() != ParticleType::PseudoParticle;
                    if (config.projectToX && projectionSuccess) projectionSuccess = particle.projectToXValue(config.projectToXValue);
                    if (config.projectToY && projectionSuccess) projectionSuccess = particle.projectToYValue(config.projectToYValue);
                    if (config.projectToZ && projectionSuccess) projectionSuccess = particle.projectToZValue(config.projectToZValue);
                    if (!projectionSuccess) {
                        particleRejected = true;
                        particlesRejectedByProjection++;
                    }
                }

                if (!particleRejected) particleRejected = !applyFilters(particle, config);

                if (particleRejected) {
                    if (particle.isNewHistory()) {
                        uint32_t incrementalHistories = particle.getIncrementalHistories();
                        writer->addAdditionalHistories(incrementalHistories);
                    }
                    particlesRejected++;
                } else {
                    writer->writeParticle(particle);
                }

                uint64_t particlesSoFar = reader->getParticlesRead();
                if (particlesSoFar % progressUpdateInterval == 0) {
                    progress.Update(particlesSoFar, "Processed " + std::to_string(writer->getHistoriesWritten()) + " histories.");
                }
            }

            uint64_t particlesExpected = particlesToRead - particlesRejected;
            uint64_t particlesWritten  = writer->getParticlesWritten();
            if (particlesWritten != particlesExpected) {
                warningMessages.push_back("The number of particles written (" + std::to_string(particlesWritten) + ") does not match the number of particles expected (" + std::to_string(particlesExpected) + "). The output file will reflect the number of particles actually written.");
            }

            uint64_t historiesInOriginalFile = readPartialFile ? reader->getHistoriesRead() : reader->getNumberOfOriginalHistories();
            uint64_t historiesWritten = writer->getHistoriesWritten();
            if (historiesWritten < historiesInOriginalFile) {
                writer->addAdditionalHistories(historiesInOriginalFile - historiesWritten);
            } else if (historiesWritten > historiesInOriginalFile) {
                warningMessages.push_back("The number of histories written (" + std::to_string(historiesWritten) + ") exceeds the number of histories in the original file's metadata (" + std::to_string(historiesInOriginalFile) + "). The metadata may be incorrect. The output file will reflect the number of histories actually written.");
            }

            progress.Complete("Conversion complete.");
        }

        auto endTime = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(endTime - startTime).count();
        std::cout << "Processed " << std::to_string(writer->getHistoriesWritten()) << " histories with " << std::to_string(writer->getParticlesWritten()) << " particles in " << std::to_string(elapsed) << " seconds" << std::endl;

        if (particlesRejected > 0) {
            std::cout << "Note: " << particlesRejected << " particles were rejected during conversion." << std::endl;
            if (particlesRejectedByProjection > 0) std::cout << "      " << particlesRejectedByProjection << " plane-parallel particles were rejected during projection." << std::endl;
        }

    } catch (const std::exception& e) {
        errorMessages.push_back(e.what());
    }

    try { if (reader) reader->close(); } catch (const std::exception& e) { errorMessages.push_back("Error closing reader: " + std::string(e.what())); }
    try { if (writer) writer->close(); } catch (const std::exception& e) { errorMessages.push_back("Error closing writer: " + std::string(e.what())); }

    for (const auto& warning : warningMessages)
        std::cerr << "Warning: " << warning << std::endl;

    if (!errorMessages.empty()) {
        for (const auto& error : errorMessages)
            std::cerr << "Error: " << error << std::endl;
        throw std::runtime_error(errorMessages[0]);
    }

    if (options.errorOnWarning && !warningMessages.empty()) {
        throw std::runtime_error("Warnings occurred during conversion (errorOnWarning is set): " + warningMessages[0]);
    }
}

} // namespace ParticleZoo
