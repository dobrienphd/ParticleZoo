
/*
 * PHSPConvert - Particle Phase Space File Format Converter
 * 
 * PURPOSE:
 * This application converts particle phase space files from one format to another.
 * It supports various Monte Carlo simulation output formats and provides seamless
 * conversion between different phase space file types while preserving particle
 * data and history information.
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
 *   inputfile                 Input phase space file to be converted
 *   outputfile                Output file path where converted data will be written
 *                             (must be different from input file)
 * 
 * Optional Arguments:
 *   --maxParticles <N>        Limit the maximum number of particles to convert
 *                             (default: convert all particles from input file)
 *   --inputFormat <format>    Force a specific input file format instead of auto-detection
 *                             Valid formats: IAEA, EGS, TOPAS, penEasy, ROOT
 *                             (default: auto-detect format from file extension)
 *   --outputFormat <format>   Force a specific output file format instead of auto-detection  
 *                             Valid formats: IAEA, EGS, TOPAS, penEasy, ROOT
 *                             (default: auto-detect format from file extension)
 *   --formats                 Display a list of all supported file formats and exit
 * 
 * USAGE EXAMPLES:
 *   # Convert EGS format to IAEA format (formats auto-detected from extensions)
 *   PHSPConvert input.egsphsp output.IAEAphsp
 * 
 *   # Convert with particle limit (only convert first 500,000 particles)
 *   PHSPConvert --maxParticles 500000 simulation.phsp converted.egsphsp
 * 
 *   # Force specific input/output formats (useful when extensions are ambiguous)
 *   PHSPConvert --inputFormat TOPAS --outputFormat IAEA input.phsp output.IAEAphsp
 * 
 *   # Show supported formats
 *   PHSPConvert --formats
 * 
 * BEHAVIOR:
 * - Input and output formats are automatically detected from file extensions
 * - Progress is displayed during conversion with percentage completion
 * - History counts are preserved from the original file
 * - Processing can be limited using --maxParticles option
 * - Input and output files must have different names
 * - Conversion maintains basic particle properties (position, direction, energy, etc.)
 * - Time taken for conversion is reported upon completion
 */

#include <iostream>
#include <string>
#include <chrono>

#include "particlezoo/utilities/argParse.h"
#include "particlezoo/utilities/formats.h"
#include "particlezoo/utilities/progress.h"
#include "particlezoo/PhaseSpaceFileReader.h"
#include "particlezoo/PhaseSpaceFileWriter.h"

int main(int argc, char* argv[]) {

    // Initial setup
    using namespace ParticleZoo;
    int errorCode = 0;
    FormatRegistry::RegisterStandardFormats();

    // Define usage message and parse command line arguments
    std::string usageMessage = "Usage: PHSPConvert [OPTIONS] <inputfile> <outputfile>\n"
                            "\n"
                            "Convert particle phase space files between different formats.\n"
                            "\n"
                            "Required Arguments:\n"
                            "  <inputfile>               Input phase space file to convert\n"
                            "  <outputfile>              Output file path (must be different from input)\n"
                            "\n"
                            "Optional Arguments:\n"
                            "  --maxParticles <N>        Maximum number of particles to convert (default: all)\n"
                            "  --inputFormat <format>    Force input file format (default: auto-detect from extension)\n"
                            "  --outputFormat <format>   Force output file format (default: auto-detect from extension)\n"
                            "  --formats                 List all supported file formats\n"
                            "\n"
                            "Supported Formats:\n"
                            "  IAEA, EGS, TOPAS, penEasy, ROOT (if compiled with ROOT support)\n"
                            "\n"
                            "Examples:\n"
                            "  PHSPConvert input.egsphsp output.IAEAphsp\n"
                            "  PHSPConvert --maxParticles 500000 simulation.phsp converted.egsphsp\n"
                            "  PHSPConvert --inputFormat TOPAS --outputFormat IAEA input.phsp output.IAEAphsp\n"
                            "  PHSPConvert --formats";
    auto args = parseArgs(argc, argv, usageMessage, 2);

    // Validate parameters
    std::string inputFile = args["positionals"][0];
    std::string outputFile = args["positionals"][1];
    std::string inputFormat = args["inputFormat"].empty() ? "" : args["inputFormat"][0];
    std::string outputFormat = args["outputFormat"].empty() ? "" : args["outputFormat"][0];
    if (inputFile.empty()) throw std::runtime_error("No input file specified.");
    if (outputFile.empty()) throw std::runtime_error("No output file specified.");
    if (inputFile == outputFile) throw std::runtime_error("Input and output files must be different.");

    // Create the reader for the input file
    std::unique_ptr<PhaseSpaceFileReader> reader;
    if (inputFormat.empty()) {
        reader = FormatRegistry::CreateReader(inputFile, args);
    } else {
        reader = FormatRegistry::CreateReader(inputFormat, inputFile, args);
    }

    // Create the writer for the output file
    std::unique_ptr<PhaseSpaceFileWriter> writer;
    if (outputFormat.empty()) {
        writer = FormatRegistry::CreateWriter(outputFile, args);
    } else {
        writer = FormatRegistry::CreateWriter(outputFormat, outputFile, args);
    }

    // Error handling for both reader and writer
    try {
        // Start the process
        std::cout << "Converting particles from " 
                  << inputFile << " (" << reader->getPHSPFormat() << ") to "
                  << outputFile << " (" << writer->getPHSPFormat() << ")..." << std::endl;

        // Determine how many particles to read - capping out at maxParticles if a limit has been set
        uint64_t particlesInFile = reader->getNumberOfParticles();
        uint64_t maxParticles = args["maxParticles"].empty() ? particlesInFile : std::stoull(args["maxParticles"][0]);
        uint64_t particlesToRead = particlesInFile > maxParticles ? maxParticles : particlesInFile;

        // Determine progress update interval
        uint64_t onePercentInterval = particlesToRead >= 100 
                                    ? particlesToRead / 100 
                                    : 1;

        // Start the timer
        auto start_time = std::chrono::steady_clock::now();

        // Check if there are particles to read
        if (particlesToRead > 0) {

            // Set up the progress bar for the current file
            Progress<uint64_t> progress(particlesToRead);
            progress.Start("Converting:");

            // Read the particles from the input file and write them into the output file
            for (uint64_t particlesSoFar = 1 ; reader->hasMoreParticles() && particlesSoFar <= particlesToRead ; particlesSoFar++) {
                Particle particle = reader->getNextParticle();
                writer->writeParticle(particle);

                // Update progress bar every 1% of particles read
                if (particlesSoFar % onePercentInterval == 0) {
                    progress.Update(particlesSoFar, "Processed " + std::to_string(writer->getHistoriesWritten()) + " histories.");
                }
            }

            // Finalize history counts, if the original file contained more histories than have been written then add the difference (this can happen if uneventful histories occurred after the final particle was recorded)
            std::uint64_t historiesInOriginalFile = particlesToRead < particlesInFile ? reader->getHistoriesRead() : reader->getNumberOfOriginalHistories();
            std::uint64_t historiesWritten = writer->getHistoriesWritten();
            if (historiesWritten < historiesInOriginalFile) {
                writer->addAdditionalHistories(historiesInOriginalFile - historiesWritten);
            } else if (historiesWritten > historiesInOriginalFile) {
                throw std::runtime_error("The number of histories written (" + std::to_string(historiesWritten) + ") exceeds the number of histories in the original file (" + std::to_string(historiesInOriginalFile) + ").");
            }

            // Complete the progress bar
            progress.Complete("Conversion complete. Processed " + std::to_string(writer->getHistoriesWritten()) + " histories.");
        }

        // Measure elapsed time and report it
        auto end_time = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(end_time - start_time).count();
        std::cout << "Time taken: " << elapsed << " seconds" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error occurred: " << e.what() << std::endl;
        errorCode = 1;
    }

    // Ensure that the reader and writer are closed even if an exception occurs
    if (writer) writer->close();
    if (reader) reader->close();

    // Return the error code
    return errorCode;
}