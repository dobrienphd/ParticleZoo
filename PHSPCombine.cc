
#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <limits>
#include <memory>

#include "particlezoo/utilities/argParse.h"
#include "particlezoo/utilities/formats.h"
#include "particlezoo/PhaseSpaceFileReader.h"
#include "particlezoo/PhaseSpaceFileWriter.h"

int main(int argc, char* argv[]) {

    /*
     * This program reads a particle phase space file in any supported format and converts it into another format.
     * Mandatory parameters are:
     * --output: the file to write to
     * and a list of input files to read from.
     * 
     * By default, the format of both the input and output files is determined by the file extension.
     * However, you can specify the format explicitly by using the --inputFormat and --outputFormat option.
     * 
     * The supported formats can be checked using the --formats option.
     * 
     * Example usage:
     * ./PHSPCombine --output outputfile.IAEAphsp input1.egsphsp input2.egsphsp ... inputN.egsphsp
     */

    // Parse command line arguments based on the description above.

    using namespace ParticleZoo;

    FormatRegistry::RegisterStandardFormats();

    std::string usageMessage = "Usage: PHSPCombine [--<option> <value> ...] --output <outputfile> inputFile1 inputFile2 ... inputFileN";
    auto args = parseArgs(argc, argv, usageMessage, 1);

    // validate mandatory parameters
    if (args["outputFile"].empty()) {
        throw std::runtime_error("No output file specified.");
    }

    std::string inputFormat = args["inputFormat"].empty() ? "" : args["inputFormat"][0];    
    std::string outputFormat = args["outputFormat"].empty() ? "" : args["outputFormat"][0];
    
    std::vector<std::string> inputFiles = args["positionals"];
    if (inputFiles.empty()) {
        throw std::runtime_error("No input files provided.");
    }

    int errorCode = 0;

    std::unique_ptr<PhaseSpaceFileWriter> writer;
    if (outputFormat.empty()) {
        writer = FormatRegistry::CreateWriter(args["outputFile"][0], args);
    } else {
        writer = FormatRegistry::CreateWriter(outputFormat, args["outputFile"][0], args);
    }

    try {

        std::cout << "Combining phase space data..." << std::endl;
        uint64_t maxParticles = args["maxParticles"].empty() ? std::numeric_limits<uint64_t>::max() : std::stoull(args["maxParticles"][0]);
        uint64_t particlesSoFar = 0;

        auto start_time = std::chrono::steady_clock::now();

        for (const auto& inputFile : inputFiles) {
            if (particlesSoFar >= maxParticles) {
                std::cout << "Maximum particle limit reached. Stopping further processing." << std::endl;
                break;
            }

            std::unique_ptr<PhaseSpaceFileReader> reader;
            if (inputFormat.empty()) {
                reader = FormatRegistry::CreateReader(inputFile, args);
            } else {
                reader = FormatRegistry::CreateReader(inputFormat, inputFile, args);
            }

            if (!reader) {
                throw std::runtime_error("Failed to create reader for file: " + inputFile);
            }

            try {
                int lastPercentageProgress = 0;
                std::cout << inputFile << " [--------------------] 0% complete" << std::flush;

                uint64_t particlesToRead = reader->getNumberOfParticles();
                if (particlesToRead > maxParticles - particlesSoFar) {
                    particlesToRead = maxParticles - particlesSoFar;
                }
                if (particlesToRead == 0) {
                    std::cout << "\r" << inputFile << " [####################] 100% complete" << std::endl;
                    continue; // No particles to read, skip to next file
                }

                uint64_t particlesSoFarThisFile = 0;

                uint64_t onePercentInterval = particlesToRead >= 100 
                                            ? particlesToRead / 100 
                                            : 1;

                std::uint64_t initialHistoryCount = writer->getHistoriesWritten();
                for ( ; reader->hasMoreParticles() && particlesSoFar < maxParticles ; particlesSoFar++) {

                    Particle particle = reader->getNextParticle();
                    writer->writeParticle(particle);

                    // Update progress bar every 1% of particles read
                    particlesSoFarThisFile++;
                    if (particlesSoFarThisFile % onePercentInterval == 0) {
                        int percentageProgress = static_cast<int>(particlesSoFarThisFile * 100 / particlesToRead);
                        int progressBarBlocks = percentageProgress / 5; // 20 steps for 100%
                        if (percentageProgress != lastPercentageProgress) {
                            lastPercentageProgress = percentageProgress;
                            // Print progress bar
                            std::cout << "\r" << inputFile << " [";
                            std::cout << std::string(progressBarBlocks, '#') << std::string(20 - progressBarBlocks, '-') << "] "
                                    << percentageProgress << "% complete" << std::flush;
                        }
                    }
                }
                std::uint64_t historiesInOriginalFile = reader->getHistoriesRead();
                std::uint64_t historiesWritten = writer->getHistoriesWritten() - initialHistoryCount;
                if (historiesWritten < historiesInOriginalFile) {
                    writer->addAdditionalHistories(historiesInOriginalFile - historiesWritten);
                }
                std::cout << "\r" << inputFile << " [####################] 100% complete" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error reading file " << inputFile << ": " << e.what() << std::endl;
                errorCode = 1;
            }
            if (reader) reader->close();
        }

        auto end_time = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(end_time - start_time).count();

        std::cout << "Time taken: " << elapsed << " seconds" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error occurred: " << e.what() << std::endl;
        errorCode = 1;
    }

    // Ensure that the reader and writer are closed even if an exception occurs
    if (writer) writer->close();

    return errorCode;
}