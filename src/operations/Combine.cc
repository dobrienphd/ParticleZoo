#include "particlezoo/operations/Combine.h"

#include <iostream>
#include <chrono>
#include <limits>
#include <memory>
#include <stdexcept>

#include "particlezoo/utilities/formats.h"
#include "particlezoo/utilities/progress.h"
#include "particlezoo/PhaseSpaceFileReader.h"
#include "particlezoo/PhaseSpaceFileWriter.h"

namespace ParticleZoo {

void Combine(const std::vector<std::string>& inputFiles,
             const std::string& outputFile,
             const CombineOptions& options)
{
    if (inputFiles.empty()) throw std::runtime_error("No input files provided.");
    if (outputFile.empty()) throw std::runtime_error("No output file specified.");

    uint64_t particlesSoFar = 0;

    FixedValues fixedValues;
    if (options.preserveConstants) {
        std::unique_ptr<PhaseSpaceFileReader> firstReader;
        if (options.inputFormat.empty()) {
            firstReader = FormatRegistry::CreateReader(inputFiles[0]);
        } else {
            firstReader = FormatRegistry::CreateReader(options.inputFormat, inputFiles[0]);
        }
        if (!firstReader) throw std::runtime_error("Failed to create reader for file: " + inputFiles[0]);
        fixedValues = firstReader->getFixedValues();
        firstReader->close();
    }

    std::unique_ptr<PhaseSpaceFileWriter> writer;
    if (options.outputFormat.empty()) {
        writer = FormatRegistry::CreateWriter(outputFile, {}, fixedValues);
    } else {
        writer = FormatRegistry::CreateWriter(options.outputFormat, outputFile, {}, fixedValues);
    }

    try {
        std::cout << "Combining phase space data..." << std::endl;
        auto start_time = std::chrono::steady_clock::now();

        for (const auto& inputFile : inputFiles) {
            if (particlesSoFar >= options.maxParticles) {
                std::cout << "Maximum particle limit reached. Stopping further processing." << std::endl;
                break;
            }

            std::unique_ptr<PhaseSpaceFileReader> reader;
            if (options.inputFormat.empty()) {
                reader = FormatRegistry::CreateReader(inputFile);
            } else {
                reader = FormatRegistry::CreateReader(options.inputFormat, inputFile);
            }

            if (!reader) throw std::runtime_error("Failed to create reader for file: " + inputFile);

            if (options.preserveConstants) {
                FixedValues currentFixedValues = reader->getFixedValues();
                if (currentFixedValues != fixedValues) {
                    throw std::runtime_error("Inconsistent constant values found in file: " + inputFile);
                }
            }

            try {
                uint64_t particlesInFile = reader->getNumberOfParticles();
                uint64_t particlesToRead = particlesInFile > options.maxParticles - particlesSoFar
                                         ? options.maxParticles - particlesSoFar
                                         : particlesInFile;

                if (particlesToRead == 0) {
                    std::cout << "\rWARNING: " << inputFile << " has no particles to read... skipped." << std::endl;
                    continue;
                }

                uint64_t particlesSoFarThisFile = 0;
                uint64_t initialHistoryCount = writer->getHistoriesWritten();

                uint64_t onePercentInterval = particlesToRead >= 100 ? particlesToRead / 100 : 1;

                Progress<uint64_t> progress(particlesToRead);
                progress.Start("Reading " + inputFile);

                while (reader->hasMoreParticles() && particlesSoFar + particlesSoFarThisFile < options.maxParticles) {
                    Particle particle = reader->getNextParticle();
                    writer->writeParticle(particle);

                    particlesSoFarThisFile = reader->getParticlesRead();
                    if (particlesSoFarThisFile % onePercentInterval == 0) {
                        progress.Update(particlesSoFarThisFile, "Processed " + std::to_string(writer->getHistoriesWritten()) + " histories.");
                    }
                }
                particlesSoFar += particlesSoFarThisFile;

                uint64_t historiesInOriginalFile = particlesToRead < particlesInFile
                                                  ? reader->getHistoriesRead()
                                                  : reader->getNumberOfOriginalHistories();
                uint64_t historiesWritten = writer->getHistoriesWritten() - initialHistoryCount;
                if (historiesWritten < historiesInOriginalFile) {
                    writer->addAdditionalHistories(historiesInOriginalFile - historiesWritten);
                } else if (historiesWritten > historiesInOriginalFile) {
                    progress.Complete("Error occurred.");
                    throw std::runtime_error("The number of histories written (" + std::to_string(historiesWritten) + ") exceeds the number of histories in the original file's metadata (" + std::to_string(historiesInOriginalFile) + "). The metadata may be incorrect. The output file will reflect the number of histories actually written.");
                }

                progress.Complete("done. Processed " + std::to_string(writer->getHistoriesWritten()) + " histories.");
            }
            catch (const std::exception& e) {
                if (reader) reader->close();
                throw;
            }

            if (reader) reader->close();
        }

        auto end_time = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(end_time - start_time).count();
        std::cout << "Time taken: " << elapsed << " seconds" << std::endl;
    }
    catch (const std::exception&) {
        if (writer) writer->close();
        throw;
    }

    if (writer) writer->close();
}

} // namespace ParticleZoo
