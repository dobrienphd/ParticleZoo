
/*
 * PHSPCombine - Particle Phase Space File Combiner
 * 
 * PURPOSE:
 * This application combines multiple particle phase space files into a single output file.
 * It supports various Monte Carlo simulation output formats and can handle format conversion
 * during the combination process. The tool is useful for merging multiple simulation runs
 * or combining phase space files from different sources.
 * 
 * SUPPORTED FORMATS:
 * - IAEA: International Atomic Energy Agency phase space format (.IAEAphsp)
 * - EGS: EGSnrc phase space format (.egsphsp, supports MODE0 and MODE2)
 * - TOPAS: TOPAS phase space format (.phsp, Binary/ASCII/Limited variants)
 * - penEasy: penEasy ASCII phase space format (.dat)
 * - ROOT: ROOT phase space format (.root) - if compiled with ROOT support
 * 
 * COMMAND LINE OPTIONS:
 * Required Arguments:
 *   --outputFile <file>       Specify the output file path where combined data will be written
 *   <inputfiles>              One or more input phase space files to be combined
 * 
 * Optional Arguments:
 *   --maxParticles <N>        Limit the maximum number of particles to process across all files
 *                             (default: unlimited - process all particles from all input files)
 *   --inputFormat <format>    Force a specific input file format instead of auto-detection
 *                             Valid formats: IAEA, EGS, TOPAS, penEasy, ROOT
 *                             (default: auto-detect format from file extension)
 *   --outputFormat <format>   Force a specific output file format instead of auto-detection
 *                             Valid formats: IAEA, EGS, TOPAS, penEasy, ROOT
 *                             (default: auto-detect format from file extension)
 *   --formats                 Display a list of all supported file formats and exit
 *   --preserveConstants <true|false>  Preserve constant values from input files if present (default: false)
 * 
 * USAGE EXAMPLES:
 *   # Combine two EGS files into an IAEA format output
 *   PHSPCombine --outputFile combined.IAEAphsp input1.egsphsp input2.egsphsp
 * 
 *   # Limit processing to first 1 million particles across all files
 *   PHSPCombine --outputFile result.phsp --maxParticles 1000000 file1.phsp file2.phsp
 * 
 *   # Force specific input/output formats (useful when extensions are ambiguous)
 *   PHSPCombine --inputFormat IAEA --outputFormat EGS --outputFile out.egsphsp in1.IAEAphsp in2.IAEAphsp
 * 
 *   # Show supported formats
 *   PHSPCombine --formats
 * 
 * BEHAVIOR:
 * - Input and output formats do not need to match - automatic conversion is performed
 * - Progress is displayed for each input file being processed
 * - Processing stops early if maxParticles limit is reached
 * - History counts are preserved and properly combined from all input files
 * - Files are processed sequentially in the order specified on command line
 * - Errors in individual files are reported and prevent the processing of remaining files
 */

#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <limits>
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
    uint64_t particlesSoFar = 0;
    bool preserveConstants = false;

    // Custom command line arguments
    const CLICommand MAX_PARTICLES_COMMAND = CLICommand(NONE, "", "maxParticles", "Maximum number of particles to process (default: unlimited)", { CLI_INT });
    const CLICommand INPUT_FORMAT_COMMAND = CLICommand(NONE, "", "inputFormat", "Force input file format (default: auto-detect from extension)", { CLI_STRING });
    const CLICommand OUTPUT_FORMAT_COMMAND = CLICommand(NONE, "", "outputFormat", "Force output file format (default: auto-detect from extension)", { CLI_STRING });
    const CLICommand OUTPUT_FILE_COMMAND = CLICommand(NONE, "", "outputFile", "Output file path", { CLI_STRING });
    const CLICommand PRESERVE_CONSTANTS_COMMAND = CLICommand(NONE, "", "preserveConstants", "Preserve constant values from input files if present", { CLI_VALUELESS });
    ArgParser::RegisterCommand(MAX_PARTICLES_COMMAND);
    ArgParser::RegisterCommand(INPUT_FORMAT_COMMAND);
    ArgParser::RegisterCommand(OUTPUT_FORMAT_COMMAND);
    ArgParser::RegisterCommand(OUTPUT_FILE_COMMAND);
    ArgParser::RegisterCommand(PRESERVE_CONSTANTS_COMMAND);

    // Define usage message and parse command line arguments
    std::string usageMessage = "Usage: PHSPCombine [OPTIONS] --outputFile <outputfile> <inputfile1> <inputfile2> ... <inputfileN>\n"
                            "\n"
                            "Combine multiple particle phase space files into a single output file.\n"
                            "Supports multiple file formats. Input and output formats do not need to be the same.\n"
                            "\n"
                            "Required Arguments:\n"
                            "  --outputFile <file>       Output file path\n"
                            "  <inputfiles>              One or more input phase space files\n"
                            "\n"
                            "Examples:\n"
                            "  PHSPCombine --outputFile combined.IAEAphsp input1.egsphsp input2.egsphsp\n"
                            "  PHSPCombine --outputFile result.phsp --maxParticles 1000000 file1.phsp file2.phsp\n"
                            "  PHSPCombine --inputFormat IAEA --outputFormat EGS --output out.egsphsp in1.IAEAphsp in2.IAEAphsp";
    auto userOptions = ArgParser::ParseArgs(argc, argv, usageMessage, 1);

    // Validate parameters
    std::uint64_t maxParticles = userOptions.contains(MAX_PARTICLES_COMMAND) ? std::get<int>(userOptions.at(MAX_PARTICLES_COMMAND)[0]) : std::numeric_limits<uint64_t>::max();
    std::string inputFormat = userOptions.contains(INPUT_FORMAT_COMMAND) ? (userOptions.at(INPUT_FORMAT_COMMAND).empty() ? "" : std::get<std::string>(userOptions.at(INPUT_FORMAT_COMMAND)[0])) : "";
    std::string outputFormat = userOptions.contains(OUTPUT_FORMAT_COMMAND) ? (userOptions.at(OUTPUT_FORMAT_COMMAND).empty() ? "" : std::get<std::string>(userOptions.at(OUTPUT_FORMAT_COMMAND)[0])) : "";
    std::string outputFile = userOptions.contains(OUTPUT_FILE_COMMAND) ? (userOptions.at(OUTPUT_FILE_COMMAND).empty() ? "" : std::get<std::string>(userOptions.at(OUTPUT_FILE_COMMAND)[0])) : "";
    std::vector<CLIValue> positionals = userOptions.contains(CLI_POSITIONALS) ? userOptions.at(CLI_POSITIONALS) : std::vector<CLIValue>{};
    std::vector<std::string> inputFiles(positionals.size());
    for (size_t i = 0; i < positionals.size(); i++) {
        inputFiles[i] = std::get<std::string>(positionals[i]);
    }
    if (inputFiles.empty()) throw std::runtime_error("No input files provided.");
    if (outputFile == "") throw std::runtime_error("No output file specified.");
    if (userOptions.contains(PRESERVE_CONSTANTS_COMMAND)) {
        preserveConstants = true;
    }

    FixedValues fixedValues; // start with default fixed values
    if (preserveConstants) {
        // If preserving constants, we need to determine which values are constant by checking the first file.
        // Throw an error during the reading loop later if a subsequent file has a different set of constants.
        std::unique_ptr<PhaseSpaceFileReader> firstReader;
        if (inputFormat.empty()) {
            firstReader = FormatRegistry::CreateReader(inputFiles[0], userOptions);
        } else {
            firstReader = FormatRegistry::CreateReader(inputFormat, inputFiles[0], userOptions);
        }
        if (!firstReader) {
            throw std::runtime_error("Failed to create reader for file: " + inputFiles[0]);
        }
        fixedValues = firstReader->getFixedValues();
        firstReader->close();
    }

    // Create the writer
    std::unique_ptr<PhaseSpaceFileWriter> writer;
    if (outputFormat.empty()) {
        writer = FormatRegistry::CreateWriter(outputFile, userOptions, fixedValues);
    } else {
        writer = FormatRegistry::CreateWriter(outputFormat, outputFile, userOptions, fixedValues);
    }

    // Error handling for the writer
    try {
        // Start the process and start the timer
        std::cout << "Combining phase space data..." << std::endl;
        auto start_time = std::chrono::steady_clock::now();

        // Iterate through all the provided input files
        for (const auto& inputFile : inputFiles) {
            // Check if maximum particle limit has been reached
            if (particlesSoFar >= maxParticles) {
                std::cout << "Maximum particle limit reached. Stopping further processing." << std::endl;
                break;
            }

            // Create the reader for the current input file
            std::unique_ptr<PhaseSpaceFileReader> reader;
            if (inputFormat.empty()) {
                reader = FormatRegistry::CreateReader(inputFile, userOptions);
            } else {
                reader = FormatRegistry::CreateReader(inputFormat, inputFile, userOptions);
            }

            // Check if reader was created successfully
            if (!reader) {
                throw std::runtime_error("Failed to create reader for file: " + inputFile);
            }

            if (preserveConstants) {
                FixedValues currentFixedValues = reader->getFixedValues();
                if (currentFixedValues != fixedValues) {
                    throw std::runtime_error("Inconsistent constant values found in file: " + inputFile);
                }
            }

            // Error handling for the reader
            try {
                // Determine how many particles to read - capping out at maxParticles if a limit has been set
                uint64_t particlesInFile = reader->getNumberOfParticles();
                uint64_t particlesToRead = particlesInFile > maxParticles - particlesSoFar ? maxParticles - particlesSoFar : particlesInFile;

                // Check if there are particles to read
                if (particlesToRead == 0) {
                    std::cout << "\rWARNING: " << inputFile << " has no particles to read... skipped." << std::endl;
                    continue; // No particles to read, skip to next file
                }

                // Initialize counters for this file
                uint64_t particlesSoFarThisFile = 0;
                std::uint64_t initialHistoryCount = writer->getHistoriesWritten();

                // Determine progress update interval
                uint64_t onePercentInterval = particlesToRead >= 100 
                                            ? particlesToRead / 100 
                                            : 1;

                // Set up the progress bar for the current file
                Progress<uint64_t> progress(particlesToRead);
                progress.Start("Reading " + inputFile);

                // Read the particles from the current file and write them into the output file
                while (reader->hasMoreParticles() && particlesSoFar+particlesSoFarThisFile < maxParticles) {
                    Particle particle = reader->getNextParticle();
                    writer->writeParticle(particle);

                    // Update progress bar every 1% of particles read
                    particlesSoFarThisFile = reader->getParticlesRead();
                    if (particlesSoFarThisFile % onePercentInterval == 0) {
                        progress.Update(particlesSoFarThisFile, "Processed " + std::to_string(writer->getHistoriesWritten()) + " histories.");
                    }
                }
                particlesSoFar += particlesSoFarThisFile;

                // Finalize history counts, if the original file contained more histories than have been written then add the difference (this can happen if uneventful histories occurred after the final particle was recorded)
                std::uint64_t historiesInOriginalFile = particlesToRead < particlesInFile ? reader->getHistoriesRead() : reader->getNumberOfOriginalHistories();
                std::uint64_t historiesWritten = writer->getHistoriesWritten() - initialHistoryCount;
                if (historiesWritten < historiesInOriginalFile) {
                    writer->addAdditionalHistories(historiesInOriginalFile - historiesWritten);
                } else if (historiesWritten > historiesInOriginalFile) {
                    progress.Complete("Error occurred.");
                    throw std::runtime_error("The number of histories written (" + std::to_string(historiesWritten) + ") exceeds the number of histories in the original file's metadata (" + std::to_string(historiesInOriginalFile) + "). The metadata may be incorrect. The output file will reflect the number of histories actually written.");
                }

                // Complete the progress bar
                progress.Complete("done. Processed " + std::to_string(writer->getHistoriesWritten()) + " histories.");
            }
            catch (const std::exception& e) {
                std::cerr << "Error reading file " << inputFile << ": " << e.what() << std::endl;
                errorCode = 1;
            }

            // Ensure that the reader is closed even if an exception occurs
            if (reader) reader->close();

            // Stop processing further files if an error occurred
            if (errorCode != 0) break;
        }

        // Measure elapsed time and report it
        auto end_time = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(end_time - start_time).count();
        std::cout << "Time taken: " << elapsed << " seconds" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << std::endl << "Error occurred: " << e.what() << std::endl;
        errorCode = 1;
    }

    // Ensure that the writer is closed even if an exception occurs
    if (writer) writer->close();

    // Return the error code
    return errorCode;
}