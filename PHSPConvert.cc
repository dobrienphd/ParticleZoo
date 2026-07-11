
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
#include <string_view>

#include "particlezoo/utilities/argParse.h"
#include "particlezoo/utilities/formats.h"
#include "particlezoo/utilities/units.h"
#include "particlezoo/operations/Convert.h"


// Anonymous namespace for CLI command definitions
namespace {

    using namespace ParticleZoo;

    constexpr std::string_view usageMessage =
        "Usage: PHSPConvert [OPTIONS] <inputfile> <outputfile>\n"
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

    const CLICommand MAX_PARTICLES_COMMAND      = CLICommand(NONE, "", "maxParticles",      "Maximum number of particles to process (default: unlimited)",                             { CLI_UINT });
    const CLICommand INPUT_FORMAT_COMMAND       = CLICommand(NONE, "", "inputFormat",       "Force input file format (default: auto-detect from extension)",                          { CLI_STRING });
    const CLICommand OUTPUT_FORMAT_COMMAND      = CLICommand(NONE, "", "outputFormat",      "Force output file format (default: auto-detect from extension)",                         { CLI_STRING });
    const CLICommand PROJECT_TO_X_COMMAND       = CLICommand(NONE, "", "projectToX",       "Project particles along their direction to this X position in cm",                       { CLI_FLOAT });
    const CLICommand PROJECT_TO_Y_COMMAND       = CLICommand(NONE, "", "projectToY",       "Project particles along their direction to this Y position in cm",                       { CLI_FLOAT });
    const CLICommand PROJECT_TO_Z_COMMAND       = CLICommand(NONE, "", "projectToZ",       "Project particles along their direction to this Z position in cm",                       { CLI_FLOAT });
    const CLICommand PRESERVE_CONSTANTS_COMMAND = CLICommand(NONE, "", "preserveConstants","Preserve constant values from input files if present",                                   { CLI_BOOL }, { true });
    const CLICommand PHOTONS_ONLY_COMMAND       = CLICommand(NONE, "", "photonsOnly",      "Only convert photon particles, rejecting all others",                                    { CLI_VALUELESS });
    const CLICommand ELECTRONS_ONLY_COMMAND     = CLICommand(NONE, "", "electronsOnly",    "Only convert electron particles, rejecting all others",                                  { CLI_VALUELESS });
    const CLICommand FILTER_BY_PDG_COMMAND      = CLICommand(NONE, "", "filterByPDG",      "Only convert particles with the specified PDG code",                                     { CLI_INT });
    const CLICommand MINIMUM_ENERGY_COMMAND     = CLICommand(NONE, "", "minEnergy",        "Only convert particles with kinetic energy >= this value in MeV",                        { CLI_FLOAT });
    const CLICommand MAXIMUM_ENERGY_COMMAND     = CLICommand(NONE, "", "maxEnergy",        "Only convert particles with kinetic energy <= this value in MeV",                        { CLI_FLOAT });
    const CLICommand MAXIMUM_X_COMMAND          = CLICommand(NONE, "", "maxX",             "Maximum X position in cm for particles to be converted",                                 { CLI_FLOAT });
    const CLICommand MAXIMUM_Y_COMMAND          = CLICommand(NONE, "", "maxY",             "Maximum Y position in cm for particles to be converted",                                 { CLI_FLOAT });
    const CLICommand MAXIMUM_Z_COMMAND          = CLICommand(NONE, "", "maxZ",             "Maximum Z position in cm for particles to be converted",                                 { CLI_FLOAT });
    const CLICommand MINIMUM_X_COMMAND          = CLICommand(NONE, "", "minX",             "Minimum X position in cm for particles to be converted",                                 { CLI_FLOAT });
    const CLICommand MINIMUM_Y_COMMAND          = CLICommand(NONE, "", "minY",             "Minimum Y position in cm for particles to be converted",                                 { CLI_FLOAT });
    const CLICommand MINIMUM_Z_COMMAND          = CLICommand(NONE, "", "minZ",             "Minimum Z position in cm for particles to be converted",                                 { CLI_FLOAT });
    const CLICommand MAXIMUM_RADIUS_COMMAND     = CLICommand(NONE, "", "maxRadius",        "Maximum radial distance in cm (XY plane) for particles to be converted",                 { CLI_FLOAT });
    const CLICommand MINIMUM_RADIUS_COMMAND     = CLICommand(NONE, "", "minRadius",        "Minimum radial distance in cm (XY plane) for particles to be converted",                 { CLI_FLOAT });
    const CLICommand PRIMARIES_ONLY_COMMAND     = CLICommand(NONE, "", "primariesOnly",    "Only process primary particles from the phase space file",                               { CLI_VALUELESS });
    const CLICommand EXCLUDE_PRIMARIES_COMMAND  = CLICommand(NONE, "", "excludePrimaries", "Exclude primary particles from processing",                                              { CLI_VALUELESS });
    const CLICommand GENERATION_FILTER_COMMAND  = CLICommand(NONE, "", "generations",      "Filter particles by generation range (min and max)",                                     { CLI_INT, CLI_INT });
    const CLICommand ERROR_ON_WARNING_COMMAND   = CLICommand(NONE, "", "errorOnWarning",   "Treat warnings as errors when returning exit code",                                      { CLI_VALUELESS });

} // anonymous namespace


int main(int argc, char* argv[]) {

    using namespace ParticleZoo;

    constexpr int SUCCESS_CODE = 0;
    constexpr int ERROR_CODE   = 1;
    constexpr int MINIMUM_REQUIRED_POSITIONAL_ARGS = 2;

    ArgParser::RegisterCommands({
        MAX_PARTICLES_COMMAND,
        INPUT_FORMAT_COMMAND,
        OUTPUT_FORMAT_COMMAND,
        PROJECT_TO_X_COMMAND,
        PROJECT_TO_Y_COMMAND,
        PROJECT_TO_Z_COMMAND,
        PRESERVE_CONSTANTS_COMMAND,
        PHOTONS_ONLY_COMMAND,
        ELECTRONS_ONLY_COMMAND,
        FILTER_BY_PDG_COMMAND,
        MINIMUM_ENERGY_COMMAND,
        MAXIMUM_ENERGY_COMMAND,
        MAXIMUM_X_COMMAND,
        MAXIMUM_Y_COMMAND,
        MAXIMUM_Z_COMMAND,
        MINIMUM_X_COMMAND,
        MINIMUM_Y_COMMAND,
        MINIMUM_Z_COMMAND,
        MINIMUM_RADIUS_COMMAND,
        MAXIMUM_RADIUS_COMMAND,
        PRIMARIES_ONLY_COMMAND,
        EXCLUDE_PRIMARIES_COMMAND,
        GENERATION_FILTER_COMMAND,
        ERROR_ON_WARNING_COMMAND
    });

    auto userOptions = ArgParser::ParseArgs(argc, argv, usageMessage, MINIMUM_REQUIRED_POSITIONAL_ARGS);

    std::string inputFile  = userOptions.extractPositional(0);
    std::string outputFile = userOptions.extractPositional(1);

    // Build ConvertOptions from parsed CLI args
    ConvertOptions options;
    if (userOptions.contains(MAX_PARTICLES_COMMAND))
        options.maxParticles = static_cast<uint64_t>(userOptions.extractUIntOption(MAX_PARTICLES_COMMAND));
    options.inputFormat       = userOptions.extractStringOption(INPUT_FORMAT_COMMAND);
    options.outputFormat      = userOptions.extractStringOption(OUTPUT_FORMAT_COMMAND);
    options.preserveConstants = userOptions.extractBoolOption(PRESERVE_CONSTANTS_COMMAND, options.preserveConstants);

    if (userOptions.contains(PROJECT_TO_X_COMMAND)) options.projectToX = userOptions.extractFloatOption(PROJECT_TO_X_COMMAND) * cm;
    if (userOptions.contains(PROJECT_TO_Y_COMMAND)) options.projectToY = userOptions.extractFloatOption(PROJECT_TO_Y_COMMAND) * cm;
    if (userOptions.contains(PROJECT_TO_Z_COMMAND)) options.projectToZ = userOptions.extractFloatOption(PROJECT_TO_Z_COMMAND) * cm;

    options.photonsOnly   = userOptions.contains(PHOTONS_ONLY_COMMAND);
    options.electronsOnly = userOptions.contains(ELECTRONS_ONLY_COMMAND);
    if (userOptions.contains(FILTER_BY_PDG_COMMAND))
        options.filterByPDG = std::get<int>(userOptions.at(FILTER_BY_PDG_COMMAND)[0]);

    if (userOptions.contains(MINIMUM_ENERGY_COMMAND)) options.minEnergy = userOptions.extractFloatOption(MINIMUM_ENERGY_COMMAND) * MeV;
    if (userOptions.contains(MAXIMUM_ENERGY_COMMAND)) options.maxEnergy = userOptions.extractFloatOption(MAXIMUM_ENERGY_COMMAND) * MeV;
    if (userOptions.contains(MINIMUM_X_COMMAND)) options.minX = userOptions.extractFloatOption(MINIMUM_X_COMMAND) * cm;
    if (userOptions.contains(MAXIMUM_X_COMMAND)) options.maxX = userOptions.extractFloatOption(MAXIMUM_X_COMMAND) * cm;
    if (userOptions.contains(MINIMUM_Y_COMMAND)) options.minY = userOptions.extractFloatOption(MINIMUM_Y_COMMAND) * cm;
    if (userOptions.contains(MAXIMUM_Y_COMMAND)) options.maxY = userOptions.extractFloatOption(MAXIMUM_Y_COMMAND) * cm;
    if (userOptions.contains(MINIMUM_Z_COMMAND)) options.minZ = userOptions.extractFloatOption(MINIMUM_Z_COMMAND) * cm;
    if (userOptions.contains(MAXIMUM_Z_COMMAND)) options.maxZ = userOptions.extractFloatOption(MAXIMUM_Z_COMMAND) * cm;
    if (userOptions.contains(MINIMUM_RADIUS_COMMAND)) options.minRadius = userOptions.extractFloatOption(MINIMUM_RADIUS_COMMAND) * cm;
    if (userOptions.contains(MAXIMUM_RADIUS_COMMAND)) options.maxRadius = userOptions.extractFloatOption(MAXIMUM_RADIUS_COMMAND) * cm;

    options.primariesOnly    = userOptions.contains(PRIMARIES_ONLY_COMMAND);
    options.excludePrimaries = userOptions.contains(EXCLUDE_PRIMARIES_COMMAND);
    if (userOptions.contains(GENERATION_FILTER_COMMAND)) {
        auto range = userOptions.extractValues(GENERATION_FILTER_COMMAND);
        options.generations = { std::get<int>(range[0]), std::get<int>(range[1]) };
    }

    options.errorOnWarning = userOptions.contains(ERROR_ON_WARNING_COMMAND);

    try {
        Convert(inputFile, outputFile, options);
        return SUCCESS_CODE;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return ERROR_CODE;
    }
}
