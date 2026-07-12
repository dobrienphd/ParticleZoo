
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
 *   --preserveConstants <true|false>  Preserve constant values from input files if present (default: true)
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
#include <vector>

#include "particlezoo/utilities/argParse.h"
#include "particlezoo/utilities/formats.h"
#include "particlezoo/operations/Combine.h"

int main(int argc, char* argv[]) {

    using namespace ParticleZoo;

    // Custom command line arguments
    const CLICommand MAX_PARTICLES_COMMAND = CLICommand(NONE, "", "maxParticles", "Maximum number of particles to process (default: unlimited)", { CLI_UINT });
    const CLICommand INPUT_FORMAT_COMMAND = CLICommand(NONE, "", "inputFormat", "Force input file format (default: auto-detect from extension)", { CLI_STRING });
    const CLICommand OUTPUT_FORMAT_COMMAND = CLICommand(NONE, "", "outputFormat", "Force output file format (default: auto-detect from extension)", { CLI_STRING });
    const CLICommand OUTPUT_FILE_COMMAND = CLICommand(NONE, "", "outputFile", "Output file path", { CLI_STRING });
    const CLICommand PRESERVE_CONSTANTS_COMMAND = CLICommand(NONE, "", "preserveConstants", "Preserve constant values from input files if present", { CLI_BOOL }, { true });
    ArgParser::RegisterCommand(MAX_PARTICLES_COMMAND);
    ArgParser::RegisterCommand(INPUT_FORMAT_COMMAND);
    ArgParser::RegisterCommand(OUTPUT_FORMAT_COMMAND);
    ArgParser::RegisterCommand(OUTPUT_FILE_COMMAND);
    ArgParser::RegisterCommand(PRESERVE_CONSTANTS_COMMAND);

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

    // Build options struct
    CombineOptions options;
    options.formatOptions = userOptions;
    if (userOptions.contains(MAX_PARTICLES_COMMAND))
        options.maxParticles = static_cast<uint64_t>(userOptions.extractUIntOption(MAX_PARTICLES_COMMAND));
    options.inputFormat  = userOptions.extractStringOption(INPUT_FORMAT_COMMAND);
    options.outputFormat = userOptions.extractStringOption(OUTPUT_FORMAT_COMMAND);
    options.preserveConstants = userOptions.extractBoolOption(PRESERVE_CONSTANTS_COMMAND, options.preserveConstants);

    std::string outputFile = userOptions.contains(OUTPUT_FILE_COMMAND)
                           ? userOptions.extractStringOption(OUTPUT_FILE_COMMAND)
                           : "";

    std::vector<CLIValue> positionals = userOptions.contains(CLI_POSITIONALS)
                                      ? userOptions.at(CLI_POSITIONALS)
                                      : std::vector<CLIValue>{};
    std::vector<std::string> inputFiles(positionals.size());
    for (size_t i = 0; i < positionals.size(); i++) {
        inputFiles[i] = std::get<std::string>(positionals[i]);
    }

    try {
        Combine(inputFiles, outputFile, options);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << std::endl << "Error occurred: " << e.what() << std::endl;
        return 1;
    }
}
