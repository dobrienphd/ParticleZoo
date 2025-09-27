
#include <iostream>
#include <string>
#include <chrono>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <memory>

#include "particlezoo/utilities/argParse.h"
#include "particlezoo/utilities/formats.h"
#include "particlezoo/utilities/progress.h"
#include "particlezoo/PhaseSpaceFileReader.h"
#include "particlezoo/PhaseSpaceFileWriter.h"

int main(int argc, char* argv[]) {

    // Initial setup
    using namespace ParticleZoo;
    int errorCode = 0;

    // Custom command line arguments
    const CLICommand SPLIT_NUMBER_COMMAND = CLICommand(NONE, "n", "splitNumber", "Number of files to split this phase space file into", { CLI_INT });
    const CLICommand INPUT_FORMAT_COMMAND = CLICommand(NONE, "", "inputFormat", "Force input file format (default: auto-detect from extension)", { CLI_STRING });
    const CLICommand OUTPUT_FORMAT_COMMAND = CLICommand(NONE, "", "outputFormat", "Force output file format (default: auto-detect from extension)", { CLI_STRING });
    ArgParser::RegisterCommand(SPLIT_NUMBER_COMMAND);
    ArgParser::RegisterCommand(INPUT_FORMAT_COMMAND);
    ArgParser::RegisterCommand(OUTPUT_FORMAT_COMMAND);
    
    // Define usage message and parse command line arguments
    std::string usageMessage = "Usage: PHSPSplit [OPTIONS] <inputfile>\n"
                            "\n"
                            "Split a single phase space file into multiple equally sized phase space files\n"
                            "\n"
                            "Required Arguments:\n"
                            "  --splitNumber             Number of files to split this phase space file into\n"
                            "  <inputfile>               Input phase space file to split\n"
                            "\n"
                            "Examples:\n"
                            "  PHSPSplit --splitNumber 10 input.egsphsp\n"
                            "  PHSPSplit -n 10 input.egsphsp\n"
                            "  PHSPSplit --outputFormat EGS -n 5 input.IAEAphsp\n"
                            "  PHSPSplit --formats";
    auto userOptions = ArgParser::ParseArgs(argc, argv, usageMessage, 1);

    // Validate parameters
    std::vector<CLIValue> positionals = userOptions.contains(CLI_POSITIONALS) ? userOptions.at(CLI_POSITIONALS) : std::vector<CLIValue>{ "" };
    std::string inputFile = std::get<std::string>(positionals[0]);
    std::string inputFormat = userOptions.contains(INPUT_FORMAT_COMMAND) ? (userOptions.at(INPUT_FORMAT_COMMAND).empty() ? "" : std::get<std::string>(userOptions.at(INPUT_FORMAT_COMMAND)[0])) : "";
    std::string outputFormat = userOptions.contains(OUTPUT_FORMAT_COMMAND) ? (userOptions.at(OUTPUT_FORMAT_COMMAND).empty() ? "" : std::get<std::string>(userOptions.at(OUTPUT_FORMAT_COMMAND)[0])) : "";
    int splitNumber = userOptions.contains(SPLIT_NUMBER_COMMAND) ? (userOptions.at(SPLIT_NUMBER_COMMAND).empty() ? -1 : std::get<int>(userOptions.at(SPLIT_NUMBER_COMMAND)[0])) : -1;

    if (inputFile.empty()) {
        std::cerr << "Error: No input file specified\n";
        errorCode = 1;
        return errorCode;
    }

    if (splitNumber <= 1) {
        std::cerr << "Error: Invalid split number (" << splitNumber << "). Must be an integer > 1\n";
        errorCode = 1;
        return errorCode;
    }

    // Check file name before extension and get output file extension based on phase space format
    auto inputPath = std::filesystem::path(inputFile);
    auto fileStem = inputPath.stem().string(); // get filename without extension
    auto fileExt = outputFormat.length() == 0 ? inputPath.extension().string() : FormatRegistry::ExtensionForFormat(outputFormat);

    if (fileStem.empty() || fileExt.empty()) {
        std::cerr << "Error: Invalid input file name (" << inputFile << ")\n";
        errorCode = 1;
        return errorCode;
    }

    auto GetSplitFilePath = [&](int index) {
        index++; // make 1-based index

        // Construct new filename with numbered suffix before the extension
        // The number should have leading zeros based on the total number of files
        std::ostringstream ss;
        ss << std::setw(std::to_string(splitNumber).length()) << std::setfill('0') << index;
        std::string numbered = fileStem + "_Part" + ss.str() + fileExt;

        return (inputPath.parent_path() / numbered).string();
    };

    // Start timer
    auto startTime = std::chrono::high_resolution_clock::now();

    // Create reader and writer pointers
    std::unique_ptr<PhaseSpaceFileReader> reader;
    std::unique_ptr<PhaseSpaceFileWriter> writer;

    try {

        // Create the reader for the input file
        if (inputFormat.empty()) {
            reader = FormatRegistry::CreateReader(inputFile, userOptions);
        } else {
            reader = FormatRegistry::CreateReader(inputFormat, inputFile, userOptions);
        }

        // Try to keep the same constant values in the new phase space file if it supports them
        FixedValues fixedValues = reader->getFixedValues();

        std::uint64_t totalParticles = reader->getNumberOfParticles();
        if (totalParticles == 0) {
            std::cerr << "Error: Input file contains no particles\n";
            errorCode = 1;
            if (reader) reader->close();
            return errorCode;
        }
        if (totalParticles < static_cast<std::uint64_t>(splitNumber)) {
            std::cerr << "Error: Input file contains fewer particles (" << totalParticles << ") than the requested split number (" << splitNumber << ")\n";
            errorCode = 1;
            if (reader) reader->close();
            return errorCode;
        }

        // Calculate the number of particles per split file
        std::uint64_t particlesPerSplit = totalParticles / splitNumber;

        // Split the phase space file
        int filesSplit = 0;

        std::string outputFilePath = GetSplitFilePath(0);

        if (outputFormat.empty()) {
            writer = FormatRegistry::CreateWriter(outputFilePath, userOptions, fixedValues);
        } else {
            writer = FormatRegistry::CreateWriter(outputFormat, outputFilePath, userOptions, fixedValues);
        }

        // Determine progress update interval
        uint64_t onePercentInterval = particlesPerSplit >= 100 
                                    ? particlesPerSplit / 100 
                                    : 1;

        std::cout << "Splitting particles from " 
                  << inputFile << " (" << reader->getPHSPFormat() << ") into "
                  << splitNumber << " parts each with format " << writer->getPHSPFormat() << "..." << std::endl;

        std::uint64_t totalHistoriesWritten = 0;

        std::uint64_t particlesWrittenAtStartOfSplit = 0;
        // Loop over the number of files to create
        for (filesSplit = 0; filesSplit < splitNumber; filesSplit++) {
            // Reset counters and flags for this split
            bool isNewHistory = false;
            bool belowLimit = true;
            bool isLastFile = (filesSplit == splitNumber - 1);

            Progress<uint64_t> progress(particlesPerSplit);
            progress.Start(outputFilePath);

            // Buffer the last particle read so that it can be written to the next file if needed
            Particle particle;
            bool hasBufferedParticle = false;

            // Loop until we reach the particle limit for this file, ensuring we don't split a history across files and exceeding the limit for the last file if there are remaining particles
            for (std::uint64_t j = particlesWrittenAtStartOfSplit; reader->hasMoreParticles() && (belowLimit || !isNewHistory || isLastFile); j++) {
                // Read the next particle
                particle = reader->getNextParticle();
                // Check if this is a new history
                isNewHistory = particle.isNewHistory();
                // Write the particle if we are below the limit, or if we're not below the limit but it's not a new history (to avoid splitting histories), or if it's the last file (to write any remaining particles)
                if (belowLimit || !isNewHistory || isLastFile) {
                    writer->writeParticle(particle);
                } else {
                    // Buffer this particle to write to the next file
                    hasBufferedParticle = true;
                }
                // Update the below limit flag for the next iteration
                belowLimit = (j+1 < particlesPerSplit);

                if (j % onePercentInterval == 0) {
                    progress.Update(j, "Processed " + std::to_string(writer->getHistoriesWritten()) + " histories.");
                }
            }

            totalHistoriesWritten += writer->getHistoriesWritten();
            if (isLastFile) {
                std::uint64_t totalOriginalHistories = reader->getNumberOfOriginalHistories();
                if (totalOriginalHistories > totalHistoriesWritten) {
                    writer->addAdditionalHistories(totalOriginalHistories - totalHistoriesWritten);
                    totalHistoriesWritten = totalOriginalHistories;
                } else if (totalHistoriesWritten > totalOriginalHistories) {
                    progress.Complete("Error occurred.");
                    throw std::runtime_error("The number of histories written (" + std::to_string(totalHistoriesWritten) + ") exceeds the number of histories in the original file's metadata (" + std::to_string(totalOriginalHistories) + "). The metadata may be incorrect. The output file will reflect the number of histories actually written.");
                }
            }

            // Complete the progress bar for this file
            progress.Complete("Done. Processed " + std::to_string(writer->getHistoriesWritten()) + " histories.");

            // Close the current output file
            writer->close();
            writer = nullptr;

            // If this is not the last file, create a new writer for the next output file and write the last buffered particle to it
            if (!isLastFile) {
                // Prepare for next output file
                outputFilePath = GetSplitFilePath(filesSplit+1);
                if (outputFormat.empty()) {
                    writer = FormatRegistry::CreateWriter(outputFilePath, userOptions, fixedValues);
                } else {
                    writer = FormatRegistry::CreateWriter(outputFormat, outputFilePath, userOptions, fixedValues);
                }
                // Write the last buffered particle to the new file
                if (hasBufferedParticle) {
                    writer->writeParticle(particle);
                    particlesWrittenAtStartOfSplit = 1;
                } else {
                    particlesWrittenAtStartOfSplit = 0;
                }
            }
        }

        // End timer and print elapsed time
        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsedSeconds = endTime - startTime;
        std::cout << "Split completed in " << elapsedSeconds.count() << " seconds\n";
        std::cout << totalHistoriesWritten << " total histories written across " << filesSplit << " files\n";

    } catch (const std::exception & e) {
        std::cerr << std::endl << "Error occurred: " << e.what() << std::endl;
        errorCode = 1;
    }

    // Close reader
    if (reader) reader->close();
    if (writer) writer->close();

    // Return appropriate error code
    return errorCode;
}