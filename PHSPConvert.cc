
#include <iostream>
#include <string>
#include <chrono>

#include "particlezoo/utilities/argParse.h"
#include "particlezoo/utilities/formats.h"
#include "particlezoo/utilities/progress.h"
#include "particlezoo/PhaseSpaceFileReader.h"
#include "particlezoo/PhaseSpaceFileWriter.h"

int main(int argc, char* argv[]) {

    /*
     * This program reads a particle phase space file in any supported format and converts it into another format.
     * Mandatory parameters are:
     * -input: the file to read from
     * -output: the file to write to
     * By default, the format of both the input and output files is determined by the file extension.
     * However, you can specify the format explicitly by using the --inputFormat and --outputFormat option.
     * 
     * The supported formats can be checked using the --formats option.
     * 
     * Example usages:
     * ./PHSPConvert inputfile.egsphsp outputfile.IAEAphsp
     * ./PHSPConvert --inputFormat egsphsp inputfile.egsphsp outputfile.IAEAphsp
     * ./PHSPConvert --inputFormat egsphsp --outputFormat IAEAphsp inputfile.egsphsp outputfile.IAEAphsp
     * ./PHSPConvert --outputFormat IAEAphsp inputfile.egsphsp outputfile.IAEAphsp
     */

    using namespace ParticleZoo;

    FormatRegistry::RegisterStandardFormats();

    std::string usageMessage = "Usage: PHSPConvert [--<option> <value> ...] inputfile outputfile";
    auto args = parseArgs(argc, argv, usageMessage, 2);

    // validate mandatory parameters
    std::string inputFile = args["positionals"][0];
    std::string outputFile = args["positionals"][1];
    if (inputFile.empty()) {
        throw std::runtime_error("No input file specified.");
    }
    if (outputFile.empty()) {
        throw std::runtime_error("No output file specified.");
    }
    if (inputFile == outputFile) {
        throw std::runtime_error("Input and output files must be different.");
    }    
    std::string inputFormat = args["inputFormat"].empty() ? "" : args["inputFormat"][0];
    std::string outputFormat = args["outputFormat"].empty() ? "" : args["outputFormat"][0];

    int errorCode = 0;

    std::unique_ptr<PhaseSpaceFileReader> reader;
    if (inputFormat.empty()) {
        reader = FormatRegistry::CreateReader(inputFile, args);
    } else {
        reader = FormatRegistry::CreateReader(inputFormat, inputFile, args);
    }

    std::unique_ptr<PhaseSpaceFileWriter> writer;
    if (outputFormat.empty()) {
        writer = FormatRegistry::CreateWriter(outputFile, args);
    } else {
        writer = FormatRegistry::CreateWriter(outputFormat, outputFile, args);
    }

    try {
        std::cout << "Converting particles from " 
                  << inputFile << " (" << reader->getPHSPFormat() << ") to "
                  << outputFile << " (" << writer->getPHSPFormat() << ")..." << std::endl;

        int lastPercentageProgress = 0;
        std::cout << "[--------------------] 0% complete" << std::flush;

        uint64_t particlesInFile = reader->getNumberOfParticles();
        uint64_t maxParticles = args["maxParticles"].empty() ? particlesInFile : std::stoull(args["maxParticles"][0]);
        uint64_t particlesToRead = particlesInFile > maxParticles ? maxParticles : particlesInFile;

        uint64_t onePercentInterval = particlesToRead >= 100 
                                    ? particlesToRead / 100 
                                    : 1;

        auto start_time = std::chrono::steady_clock::now();

        if (particlesToRead > 0) {
            Progress<uint64_t> progress(particlesToRead);
            progress.Start("Converting:");
            for (uint64_t particlesSoFar = 1 ; reader->hasMoreParticles() && particlesSoFar <= particlesToRead ; particlesSoFar++) {

                Particle particle = reader->getNextParticle();
                writer->writeParticle(particle);

                // Update progress bar every 1% of particles read
                if (particlesSoFar % onePercentInterval == 0) {
                    progress.Update(particlesSoFar, "Processed " + std::to_string(writer->getHistoriesWritten()) + " histories.");
                }
            }

            std::uint64_t historiesInOriginalFile = particlesToRead < particlesInFile ? reader->getHistoriesRead() : reader->getNumberOfOriginalHistories();
            std::uint64_t historiesWritten = writer->getHistoriesWritten();
            if (historiesWritten < historiesInOriginalFile) {
                writer->addAdditionalHistories(historiesInOriginalFile - historiesWritten);
            } else if (historiesWritten > historiesInOriginalFile) {
                throw std::runtime_error("The number of histories written (" + std::to_string(historiesWritten) + ") exceeds the number of histories in the original file (" + std::to_string(historiesInOriginalFile) + ").");
            }

            progress.Complete("Conversion complete. Processed " + std::to_string(writer->getHistoriesWritten()) + " histories.");
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
    if (reader) reader->close();

    return errorCode;
}