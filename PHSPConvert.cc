
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
#include <filesystem>

#include "particlezoo/utilities/argParse.h"
#include "particlezoo/utilities/formats.h"
#include "particlezoo/utilities/progress.h"
#include "particlezoo/PhaseSpaceFileReader.h"
#include "particlezoo/PhaseSpaceFileWriter.h"

using namespace ParticleZoo;


namespace {

    // Usage message
    const std::string usageMessage = "Usage: PHSPConvert [OPTIONS] <inputfile> <outputfile>\n"
                                "\n"
                                "Convert particle phase space files between different formats.\n"
                                "\n"
                                "Required Arguments:\n"
                                "  <inputfile>               Input phase space file to convert\n"
                                "  <outputfile>              Output file path (must be different from input)\n"
                                "\n"
                                "Examples:\n"
                                "  PHSPConvert input.egsphsp output.IAEAphsp\n"
                                "  PHSPConvert --maxParticles 500000 simulation.phsp converted.egsphsp\n"
                                "  PHSPConvert --inputFormat TOPAS --outputFormat IAEA input.phsp output.IAEAphsp\n"
                                "  PHSPConvert --formats";


    // Custom command line arguments
    const CLICommand MAX_PARTICLES_COMMAND = CLICommand(NONE, "", "maxParticles", "Maximum number of particles to process (default: unlimited)", { CLI_INT });
    const CLICommand INPUT_FORMAT_COMMAND = CLICommand(NONE, "", "inputFormat", "Force input file format (default: auto-detect from extension)", { CLI_STRING });
    const CLICommand OUTPUT_FORMAT_COMMAND = CLICommand(NONE, "", "outputFormat", "Force output file format (default: auto-detect from extension)", { CLI_STRING });
    const CLICommand PROJECT_TO_X_COMMAND = CLICommand(NONE, "", "projectToX", "Project particles along their direction to this X position in cm", { CLI_FLOAT });
    const CLICommand PROJECT_TO_Y_COMMAND = CLICommand(NONE, "", "projectToY", "Project particles along their direction to this Y position in cm", { CLI_FLOAT });
    const CLICommand PROJECT_TO_Z_COMMAND = CLICommand(NONE, "", "projectToZ", "Project particles along their direction to this Z position in cm", { CLI_FLOAT });
    const CLICommand PRESERVE_CONSTANTS_COMMAND = CLICommand(NONE, "", "preserveConstants", "Preserve constant values from input files if present", { CLI_BOOL }, { true });
    const CLICommand PHOTONS_ONLY_COMMAND = CLICommand(NONE, "", "photonsOnly", "Only convert photon particles, rejecting all others", { CLI_VALUELESS });
    const CLICommand ELECTRONS_ONLY_COMMAND = CLICommand(NONE, "", "electronsOnly", "Only convert electron particles, rejecting all others", { CLI_VALUELESS });
    const CLICommand FILTER_BY_PDG_COMMAND = CLICommand(NONE, "", "filterByPDG", "Only convert particles with the specified PDG code", { CLI_INT });
    const CLICommand MINIMUM_ENERGY_COMMAND = CLICommand(NONE, "", "minEnergy", "Only convert particles with kinetic energy greater than or equal to this value in MeV", { CLI_FLOAT });
    const CLICommand MAXIMUM_ENERGY_COMMAND = CLICommand(NONE, "", "maxEnergy", "Only convert particles with kinetic energy less than or equal to this value in MeV", { CLI_FLOAT });


    // App configuration state
    struct AppConfig {
        const std::string   inputFile;
        const std::string   outputFile;
        const std::string   inputFormat;
        const std::string   outputFormat;
        const std::uint64_t maxParticles;
        const bool          preserveConstants;
        const bool          projectToX;
        const bool          projectToY;
        const bool          projectToZ;
        const float         projectToXValue;
        const float         projectToYValue;
        const float         projectToZValue;
        const ParticleType  filterByParticle;
        const bool          filterByEnergy;
        const float         minimumEnergy;
        const float         maximumEnergy;

        // Constructor to initialize from user options
        AppConfig(const UserOptions & userOptions)
        :   inputFile(userOptions.extractPositional(0)),
            outputFile(userOptions.extractPositional(1)),
            inputFormat(userOptions.extractStringOption(INPUT_FORMAT_COMMAND)),
            outputFormat(userOptions.extractStringOption(OUTPUT_FORMAT_COMMAND)),
            maxParticles(static_cast<std::uint64_t>(userOptions.extractIntOption(MAX_PARTICLES_COMMAND, std::numeric_limits<uint64_t>::max()))),
            preserveConstants(userOptions.extractBoolOption(PRESERVE_CONSTANTS_COMMAND, true)),
            projectToX(userOptions.contains(PROJECT_TO_X_COMMAND)),
            projectToY(userOptions.contains(PROJECT_TO_Y_COMMAND)),
            projectToZ(userOptions.contains(PROJECT_TO_Z_COMMAND)),
            projectToXValue(projectToX ? userOptions.extractFloatOption(PROJECT_TO_X_COMMAND, 0.0f) * cm : 0.0f),
            projectToYValue(projectToY ? userOptions.extractFloatOption(PROJECT_TO_Y_COMMAND, 0.0f) * cm : 0.0f),
            projectToZValue(projectToZ ? userOptions.extractFloatOption(PROJECT_TO_Z_COMMAND, 0.0f) * cm : 0.0f),
            filterByParticle(determineParticleFilter(userOptions)),
            filterByEnergy(userOptions.contains(MINIMUM_ENERGY_COMMAND) || userOptions.contains(MAXIMUM_ENERGY_COMMAND)),
            minimumEnergy(userOptions.contains(MINIMUM_ENERGY_COMMAND) ? userOptions.extractFloatOption(MINIMUM_ENERGY_COMMAND, 0.0f) * MeV : 0.0f),
            maximumEnergy(userOptions.contains(MAXIMUM_ENERGY_COMMAND) ? userOptions.extractFloatOption(MAXIMUM_ENERGY_COMMAND, std::numeric_limits<float>::max()) * MeV : std::numeric_limits<float>::max())
        {
            // Validate the configuration
            validate(userOptions);
        }

        bool useProjection() const { return projectToX || projectToY || projectToZ; }

    private:
        ParticleType determineParticleFilter(const UserOptions& userOptions) const {
            if (userOptions.contains(PHOTONS_ONLY_COMMAND)) {
                return ParticleType::Photon;
            } else if (userOptions.contains(ELECTRONS_ONLY_COMMAND)) {
                return ParticleType::Electron;
            } else if (userOptions.contains(FILTER_BY_PDG_COMMAND)) {
                int pdgCode = std::get<int>(userOptions.at(FILTER_BY_PDG_COMMAND)[0]);
                return getParticleTypeFromPDGID(static_cast<std::int32_t>(pdgCode));
            }
            return ParticleType::Unsupported;
        }

        void validate(const UserOptions& userOptions) const {
            // Validate parameters
            if (inputFile.empty()) throw std::runtime_error("No input file specified.");
            if (outputFile.empty()) throw std::runtime_error("No output file specified.");
            if (inputFile == outputFile) throw std::runtime_error("Input and output files must be different.");
            if (userOptions.contains(FILTER_BY_PDG_COMMAND) && filterByParticle == ParticleType::Unsupported)
            {
                throw std::runtime_error("Invalid PDG code specified for particle filter.");
            }
            if (filterByEnergy && minimumEnergy > maximumEnergy)
            {
                throw std::runtime_error("Minimum energy cannot be greater than maximum energy for energy filter.");
            }
            if ((userOptions.contains(PHOTONS_ONLY_COMMAND) && userOptions.contains(ELECTRONS_ONLY_COMMAND))
                || (userOptions.contains(PHOTONS_ONLY_COMMAND) && userOptions.contains(FILTER_BY_PDG_COMMAND))
                || (userOptions.contains(ELECTRONS_ONLY_COMMAND) && userOptions.contains(FILTER_BY_PDG_COMMAND)))
            {
                throw std::runtime_error("Conflicting particle filter options specified.");
            }
        }
    };

    // Function to apply filters to a particle based on the application configuration
    inline bool applyFilters(const Particle & particle, const AppConfig & config)
    {
        // Apply particle type filter if specified
        if (config.filterByParticle != ParticleType::Unsupported && config.filterByParticle != particle.getType()) {
            return false;
        }

        // Apply energy filters
        if (config.filterByEnergy) {
            float energy = particle.getKineticEnergy();
            if (energy < config.minimumEnergy || energy > config.maximumEnergy) {
                return false;
            }
        }
        
        return true;
    }

}


// Main function
int main(int argc, char* argv[]) {

    // Set default error code
    int errorCode = 0;

    // Register custom command line arguments
    ArgParser::RegisterCommand(MAX_PARTICLES_COMMAND);
    ArgParser::RegisterCommand(INPUT_FORMAT_COMMAND);
    ArgParser::RegisterCommand(OUTPUT_FORMAT_COMMAND);
    ArgParser::RegisterCommand(PROJECT_TO_X_COMMAND);
    ArgParser::RegisterCommand(PROJECT_TO_Y_COMMAND);
    ArgParser::RegisterCommand(PROJECT_TO_Z_COMMAND);
    ArgParser::RegisterCommand(PRESERVE_CONSTANTS_COMMAND);
    ArgParser::RegisterCommand(PHOTONS_ONLY_COMMAND);
    ArgParser::RegisterCommand(ELECTRONS_ONLY_COMMAND);
    ArgParser::RegisterCommand(FILTER_BY_PDG_COMMAND);
    ArgParser::RegisterCommand(MINIMUM_ENERGY_COMMAND);
    ArgParser::RegisterCommand(MAXIMUM_ENERGY_COMMAND);
    
    // Define usage message and parse command line arguments
    auto userOptions = ArgParser::ParseArgs(argc, argv, usageMessage, 2);
    const AppConfig config(userOptions);

    // Create the reader for the input file
    std::unique_ptr<PhaseSpaceFileReader> reader;
    if (config.inputFormat.empty()) {
        reader = FormatRegistry::CreateReader(config.inputFile, userOptions);
    } else {
        reader = FormatRegistry::CreateReader(config.inputFormat, config.inputFile, userOptions);
    }

    // If requested, try to keep the same constant values in the new phase space file if it supports them
    const FixedValues fixedValues = config.preserveConstants ? reader->getFixedValues() : FixedValues{};

    // Create the writer for the output file
    std::unique_ptr<PhaseSpaceFileWriter> writer;
    if (config.outputFormat.empty()) {
        writer = FormatRegistry::CreateWriter(config.outputFile, userOptions, fixedValues);
    } else {
        writer = FormatRegistry::CreateWriter(config.outputFormat, config.outputFile, userOptions, fixedValues);
    }

    // Keep a list of errors
    std::list<std::string> errorMessages;

    // Error handling for both reader and writer
    try {

        // Report the conversion details
        std::cout << "Converting particles from " 
                  << config.inputFile << " (" << reader->getPHSPFormat() << ") to "
                  << config.outputFile << " (" << writer->getPHSPFormat() << ")..." << std::endl;

        // Determine how many particles to read - capping out at maxParticles if a limit has been set
        uint64_t particlesInFile = reader->getNumberOfParticles();
        uint64_t particlesToRead = particlesInFile > config.maxParticles ? config.maxParticles : particlesInFile;
        uint64_t particlesRejected = 0;
        uint64_t particlesRejectedByProjection = 0;
        bool readPartialFile = particlesToRead < particlesInFile;

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
            while (reader->hasMoreParticles() && (!readPartialFile || reader->getParticlesRead() < particlesToRead)) {
                Particle particle = reader->getNextParticle();

                // Apply filters
                bool particleRejected = !applyFilters(particle, config);

                // Handle particle projection if requested
                if (!particleRejected && config.useProjection()) {
                    // Project the particle if projection is enabled
                    // If the projection fails (e.g. particle direction is parallel to the projection plane) then skip writing this particle
                    // If we don't write the particle and it is a new history then we need to adjust the history count
                    bool projectionSuccess = particle.getType() != ParticleType::PseudoParticle; // Do not project pseudo-particles
                    if (config.projectToX && projectionSuccess) projectionSuccess = particle.projectToXValue(config.projectToXValue);
                    if (config.projectToY && projectionSuccess) projectionSuccess = particle.projectToYValue(config.projectToYValue);
                    if (config.projectToZ && projectionSuccess) projectionSuccess = particle.projectToZValue(config.projectToZValue);
                    if (!projectionSuccess) {
                        // Projection failed, reject the particle
                        particleRejected = true;
                        particlesRejectedByProjection++;
                    }
                }

                // Either write or reject the particle
                if (particleRejected) {
                    // If this is a new history, account for the missing histories
                    if (particle.isNewHistory()) {
                        uint32_t incrementalHistories = particle.getIncrementalHistories();
                        writer->addAdditionalHistories(incrementalHistories);
                    }
                    particlesRejected++;
                } else {
                    // Write the particle to the output file
                    writer->writeParticle(particle);
                }

                // Update progress bar every 1% of particles read
                std::uint64_t particlesSoFar = reader->getParticlesRead();
                if (particlesSoFar % onePercentInterval == 0) {
                    progress.Update(particlesSoFar, "Processed " + std::to_string(writer->getHistoriesWritten()) + " histories.");
                }
            }

            // Check that the number of particles written matches the expected number
            std::uint64_t particlesWritten = writer->getParticlesWritten();
            if (particlesWritten != particlesToRead - particlesRejected) {
                errorMessages.push_back("The number of particles written (" + std::to_string(particlesWritten) + ") does not match the number of particles expected (" + std::to_string(particlesToRead - particlesRejected) + "). The output file will reflect the number of particles actually written.");
            }

            // Finalize history counts, if the original file contained more histories than have been written then add the difference (this can happen if uneventful histories occurred after the final particle was recorded)
            std::uint64_t historiesInOriginalFile = readPartialFile ? reader->getHistoriesRead() : reader->getNumberOfOriginalHistories();
            std::uint64_t historiesWritten = writer->getHistoriesWritten();
            if (historiesWritten < historiesInOriginalFile) {
                writer->addAdditionalHistories(historiesInOriginalFile - historiesWritten);
            } else if (historiesWritten > historiesInOriginalFile) {
                errorMessages.push_back("The number of histories written (" + std::to_string(historiesWritten) + ") exceeds the number of histories in the original file's metadata (" + std::to_string(historiesInOriginalFile) + "). The metadata may be incorrect. The output file will reflect the number of histories actually written.");
            }

            // Complete the progress bar
            if (errorMessages.empty()) {
                progress.Complete("Conversion complete.");
            } else {
                progress.Complete("Conversion complete with " + std::to_string(errorMessages.size()) + " warnings.");
            }
        }

        // Measure elapsed time and report it
        auto end_time = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(end_time - start_time).count();
        std::cout << "Processed " + std::to_string(writer->getHistoriesWritten()) + " histories with " + std::to_string(writer->getParticlesWritten()) + " particles in " + std::to_string(elapsed) + " seconds" << std::endl;

        // Report any rejected particles
        if (particlesRejected > 0) {
            std::cout << "Note: " << particlesRejected << " particles were rejected during conversion." << std::endl;
            if (particlesRejectedByProjection > 0) std::cout << "      " << particlesRejectedByProjection << " plane-parallel particles were rejected during projection." << std::endl;
        }

    } catch (const std::exception& e) {
        // Catch any exceptions and report them
        errorMessages.push_back(e.what());
        errorCode = 1;
    }

    // Ensure that the reader and writer are closed even if an exception occurs
    if (writer) writer->close();
    if (reader) reader->close();

    // Output any error messages
    for (const auto& error : errorMessages) {
        std::cerr << "Warning: " << error << std::endl;
    }

    // Set error code to 1 if there were any error messages
    errorCode = errorMessages.empty() ? errorCode : 1;

    // Return the error code
    return errorCode;
}