
#include <iostream>
#include <string>

#include "particlezoo/utilities/argParse.h"
#include "particlezoo/utilities/formats.h"
#include "particlezoo/operations/Split.h"

int main(int argc, char* argv[]) {

    using namespace ParticleZoo;

    const CLICommand SPLIT_NUMBER_COMMAND = CLICommand(NONE, "n", "splitNumber", "Number of files to split this phase space file into", { CLI_INT });
    const CLICommand INPUT_FORMAT_COMMAND = CLICommand(NONE, "", "inputFormat", "Force input file format (default: auto-detect from extension)", { CLI_STRING });
    const CLICommand OUTPUT_FORMAT_COMMAND = CLICommand(NONE, "", "outputFormat", "Force output file format (default: auto-detect from extension)", { CLI_STRING });
    ArgParser::RegisterCommand(SPLIT_NUMBER_COMMAND);
    ArgParser::RegisterCommand(INPUT_FORMAT_COMMAND);
    ArgParser::RegisterCommand(OUTPUT_FORMAT_COMMAND);

    std::string usageMessage =
        "Usage: PHSPSplit [OPTIONS] <inputfile>\n"
        "\n"
        "Split a single phase space file into multiple (roughly) equally sized phase space files\n"
        "History boundaries will be respected so that no history is split across files, this can result in files of marginally different sizes.\n"
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

    std::vector<CLIValue> positionals = userOptions.contains(CLI_POSITIONALS)
                                      ? userOptions.at(CLI_POSITIONALS)
                                      : std::vector<CLIValue>{ "" };
    std::string inputFile = std::get<std::string>(positionals[0]);

    int splitNumber = userOptions.contains(SPLIT_NUMBER_COMMAND)
                    ? (userOptions.at(SPLIT_NUMBER_COMMAND).empty() ? -1
                                                                    : std::get<int>(userOptions.at(SPLIT_NUMBER_COMMAND)[0]))
                    : -1;

    SplitOptions options;
    options.formatOptions = userOptions;
    options.inputFormat  = userOptions.contains(INPUT_FORMAT_COMMAND)
                         ? (userOptions.at(INPUT_FORMAT_COMMAND).empty() ? "" : std::get<std::string>(userOptions.at(INPUT_FORMAT_COMMAND)[0]))
                         : "";
    options.outputFormat = userOptions.contains(OUTPUT_FORMAT_COMMAND)
                         ? (userOptions.at(OUTPUT_FORMAT_COMMAND).empty() ? "" : std::get<std::string>(userOptions.at(OUTPUT_FORMAT_COMMAND)[0]))
                         : "";

    try {
        Split(inputFile, splitNumber, options);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << std::endl << "Error occurred: " << e.what() << std::endl;
        return 1;
    }
}
