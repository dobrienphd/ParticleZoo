
#pragma once

#include <string>
#include <unordered_map>
#include <iostream>
#include <cstdlib>
#include <cstddef>
#include <vector>
#include <variant>

#include "particlezoo/utilities/formats.h"

namespace ParticleZoo {

    using CLIValue = std::variant<float,int,std::string,bool>;

    struct CLICommand
    {
        enum CLIArgType { CLI_FLOAT = 1, CLI_INT = 2, CLI_STRING = 3, CLI_BOOL = 4 };

        std::string shortName;
        std::string longName;
        std::string description;
        std::vector<CLIArgType> argTypes;
        
        CLICommand(const std::string& shortName,
                const std::string& longName,
                const std::string& description,
                std::initializer_list<CLIArgType> types)
        : shortName(shortName),
            longName(longName),
            description(description),
            argTypes(types)
            {}
        
        bool operator==(CLICommand const& o) const noexcept {
            return shortName == o.shortName && longName == o.longName;
        }
    };

    static std::unordered_map<std::string, std::vector<std::string>>
    parseArgs(int argc, char* argv[], std::string & usageMessage, std::size_t minimumPositionalArgs = 0)
    {
        auto printUsage = [usageMessage](){
            std::cerr << usageMessage << "\n";
            std::exit(1);
        };

        auto printFormats = []() {
            FormatRegistry::PrintSupportedFormats();
            std::exit(0);
        };

        std::vector<std::string> positional;
        std::string input_fmt, output_fmt;
        std::unordered_map<std::string, std::vector<std::string>> opts;

        // collect arbitrary --key value pairs
        for(int i = 1; i < argc; ++i) {
            std::string a{argv[i]};
            if (a.rfind("--", 0) == 0) {
                std::string key = a.substr(2);
                if (key == "formats") printFormats();
                if (++i >= argc) printUsage();
                std::string val = argv[i];
                opts[key] = { val };
            }
            else if (a.rfind("-", 0) == 0) {
                // unknown single-dash flag
                printUsage();
            }
            else {
                positional.push_back(a);
            }
        }

        // must have exactly two positional arguments
        if (positional.size() < minimumPositionalArgs) printUsage();

        // all positionals are inputs
        opts["positionals"] = positional;

        return opts;
    }

}