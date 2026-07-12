#include "particlezoo/operations/Split.h"

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <filesystem>
#include <memory>
#include <stdexcept>

#include "particlezoo/utilities/formats.h"
#include "particlezoo/utilities/progress.h"
#include "particlezoo/PhaseSpaceFileReader.h"
#include "particlezoo/PhaseSpaceFileWriter.h"

namespace ParticleZoo {

void Split(const std::string& inputFile,
           int splitNumber,
           const SplitOptions& options)
{
    if (inputFile.empty()) throw std::runtime_error("No input file specified.");
    if (splitNumber <= 1)  throw std::runtime_error("Invalid split number (" + std::to_string(splitNumber) + "). Must be an integer > 1.");

    auto inputPath = std::filesystem::path(inputFile);
    auto fileStem  = inputPath.stem().string();
    auto fileExt   = options.outputFormat.empty()
                   ? inputPath.extension().string()
                   : FormatRegistry::ExtensionForFormat(options.outputFormat);

    if (fileStem.empty() || fileExt.empty())
        throw std::runtime_error("Invalid input file name (" + inputFile + ").");

    auto getSplitFilePath = [&](int index) {
        index++;
        std::ostringstream ss;
        ss << std::setw(static_cast<int>(std::to_string(splitNumber).length()))
           << std::setfill('0') << index;
        std::string numbered = fileStem + "_Part" + ss.str() + fileExt;
        return (inputPath.parent_path() / numbered).string();
    };

    auto startTime = std::chrono::high_resolution_clock::now();

    std::unique_ptr<PhaseSpaceFileReader> reader;
    std::unique_ptr<PhaseSpaceFileWriter> writer;

    reader = FormatRegistry::CreateReader(options.inputFormat, inputFile, options.formatOptions);

    FixedValues fixedValues = reader->getFixedValues();

    uint64_t totalParticles = reader->getNumberOfParticles();
    if (totalParticles == 0) {
        reader->close();
        throw std::runtime_error("Input file contains no particles.");
    }
    if (totalParticles < static_cast<uint64_t>(splitNumber)) {
        reader->close();
        throw std::runtime_error("Input file contains fewer particles (" + std::to_string(totalParticles) +
                                 ") than the requested split number (" + std::to_string(splitNumber) + ").");
    }

    uint64_t particlesPerSplit = totalParticles / splitNumber;

    std::cout << "Splitting particles from "
              << inputFile << " (" << reader->getPHSPFormat() << ") into "
              << splitNumber << " parts..." << std::endl;

    int filesSplit = 0;
    std::string outputFilePath = getSplitFilePath(0);

    writer = FormatRegistry::CreateWriter(options.outputFormat, outputFilePath, options.formatOptions, fixedValues);

    std::cout << "  Format: " << writer->getPHSPFormat() << std::endl;

    uint64_t onePercentInterval = particlesPerSplit >= 100 ? particlesPerSplit / 100 : 1;

    uint64_t totalHistoriesWritten = 0;
    uint64_t particlesWrittenAtStartOfSplit = 0;

    try {
        for (filesSplit = 0; filesSplit < splitNumber; filesSplit++) {
            bool isNewHistory = false;
            bool belowLimit   = true;
            bool isLastFile   = (filesSplit == splitNumber - 1);

            Progress<uint64_t> progress(particlesPerSplit);
            progress.Start(outputFilePath);

            Particle particle;
            bool hasBufferedParticle = false;

            for (uint64_t j = particlesWrittenAtStartOfSplit;
                 reader->hasMoreParticles() && (belowLimit || !isNewHistory || isLastFile);
                 j++) {
                particle    = reader->getNextParticle();
                isNewHistory = particle.isNewHistory();

                if (belowLimit || !isNewHistory || isLastFile) {
                    writer->writeParticle(particle);
                } else {
                    hasBufferedParticle = true;
                }

                belowLimit = (j + 1 < particlesPerSplit);

                if (j % onePercentInterval == 0) {
                    progress.Update(j, "Processed " + std::to_string(writer->getHistoriesWritten()) + " histories.");
                }
            }

            totalHistoriesWritten += writer->getHistoriesWritten();

            if (isLastFile) {
                uint64_t totalOriginalHistories = reader->getNumberOfOriginalHistories();
                if (totalOriginalHistories > totalHistoriesWritten) {
                    writer->addAdditionalHistories(totalOriginalHistories - totalHistoriesWritten);
                    totalHistoriesWritten = totalOriginalHistories;
                } else if (totalHistoriesWritten > totalOriginalHistories) {
                    progress.Complete("Error occurred.");
                    throw std::runtime_error("The number of histories written (" + std::to_string(totalHistoriesWritten) +
                                             ") exceeds the number of histories in the original file's metadata (" +
                                             std::to_string(totalOriginalHistories) +
                                             "). The metadata may be incorrect. The output file will reflect the number of histories actually written.");
                }
            }

            progress.Complete("Done. Processed " + std::to_string(writer->getHistoriesWritten()) + " histories.");

            writer->close();
            writer = nullptr;

            if (!isLastFile) {
                outputFilePath = getSplitFilePath(filesSplit + 1);
                writer = FormatRegistry::CreateWriter(options.outputFormat, outputFilePath, options.formatOptions, fixedValues);
                if (hasBufferedParticle) {
                    writer->writeParticle(particle);
                    particlesWrittenAtStartOfSplit = 1;
                } else {
                    particlesWrittenAtStartOfSplit = 0;
                }
            }
        }
    } catch (const std::exception&) {
        if (writer) writer->close();
        if (reader) reader->close();
        throw;
    }

    if (reader) reader->close();

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = endTime - startTime;
    std::cout << "Split completed in " << elapsed.count() << " seconds\n";
    std::cout << totalHistoriesWritten << " total histories written across " << filesSplit << " files\n";
}

} // namespace ParticleZoo
