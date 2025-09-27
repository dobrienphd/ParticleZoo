
#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <cstdlib>
#include <cstddef>
#include <vector>
#include <variant>
#include <array>

namespace ParticleZoo {

    /// @brief Type alias for command line argument values.
    using CLIValue = std::variant<float,int,std::string,bool>;

    /// @brief Enumeration for command line argument types.
    enum CLIArgType { CLI_FLOAT, CLI_INT, CLI_STRING, CLI_BOOL, CLI_VALUELESS };

    enum CLIArgContext { READER, WRITER, BOTH, NONE };


    /// @brief Structure representing a command line argument.
    struct CLICommand
    {
        CLIArgContext context; // indicates if the command is for reader, writer or both
        std::string shortName; // used with single dash, e.g. -h, empty string means that the shortName isn't used
        std::string longName; // used with double dash, e.g. --help, empty string means that the longName isn't used
        std::string description;
        std::vector<CLIArgType> argTypes;
        std::vector<CLIValue> defaultValues;

        CLICommand(CLIArgContext context,
                const std::string& shortName,
                const std::string& longName,
                const std::string& description,
                std::initializer_list<CLIArgType> types,
                std::initializer_list<CLIValue> defaults = {})
        : context(context),
            shortName(shortName),
            longName(longName),
            description(description),
            argTypes(types),
            defaultValues(defaults)
            {
                if (argTypes.size() != defaultValues.size() && !defaultValues.empty()) {
                    throw std::runtime_error("Number of default values does not match number of argument types for command: " 
                                             + (shortName.empty() ? "" : "-" + shortName)
                                             + (shortName.empty() || longName.empty() ? "" : ", ")
                                             + (longName.empty() ? "" : "--" + longName));
                }
            }
        
        CLICommand(const CLICommand&) = default;
        CLICommand& operator=(const CLICommand&) = default;
        CLICommand(CLICommand&&) = default;
        CLICommand& operator=(CLICommand&&) = default;

        bool operator==(CLICommand const& o) const noexcept {
            return shortName == o.shortName && longName == o.longName;
        }
    };

}

// Hash for CLICommand
namespace std {
    template<>
    struct hash<ParticleZoo::CLICommand> {
        std::size_t operator()(const ParticleZoo::CLICommand& cmd) const noexcept {
            // Combine hashes of shortName and longName
            std::size_t h1 = std::hash<std::string>{}(cmd.shortName);
            std::size_t h2 = std::hash<std::string>{}(cmd.longName);
            return h1 ^ (h2 << 1);
        }
    };
}

namespace ParticleZoo {

    /// @brief Type alias for user options map.
    using UserOptions = std::unordered_map<CLICommand, std::vector<CLIValue>>;

    const CLICommand CLI_POSITIONALS { BOTH, "", "positionals", "", { CLI_STRING } };

    /// @brief Singleton class for parsing command line arguments.
    class ArgParser {
        private:
            // Registered commands
            std::unordered_set<CLICommand> commands;
            UserOptions setOptions;

            // Private constructor for singleton pattern
            ArgParser() {};

            // Delete copy and move constructors and assign operators
            ArgParser(const ArgParser&) = delete;
            ArgParser& operator=(const ArgParser&) = delete;
            ArgParser(ArgParser&&) = delete;
            ArgParser& operator=(ArgParser&&) = delete;

            // Retrieve singleton instance
            static ArgParser& Instance() {
                static ArgParser instance;
                return instance;
            }

            // Reserve commands that cannot be used for user options
            // to avoid conflicts with built-in functionality
            inline static const std::array<std::string, 7> reservedCommands { "h", "help", "f", "formats", "v", "version", "positionals" };
        public:
            ~ArgParser() = default;

            /// @brief Registers a command for argument parsing.
            /// @param command The command to register.
            static void RegisterCommand(const CLICommand& command);

            /// @brief Registers multiple commands for argument parsing.
            /// @param commands The commands to register.
            static void RegisterCommands(const std::vector<CLICommand>& commands);

            /// @brief Prints usage information for the command line interface.
            static void PrintUsage(std::string usageMessage, int exitCode = 1);

            /// @brief Parses command line arguments based on registered commands.
            /// @param argc The number of command line arguments.
            /// @param argv The command line arguments.
            /// @param usageMessage The usage message to display.
            /// @param minimumPositionalArgs The minimum number of positional arguments required.
            /// @return A map of user options parsed from the command line arguments.
            static const UserOptions ParseArgs(int argc, char* argv[], const std::string & usageMessage, std::size_t minimumPositionalArgs = 0);
    };

} // namespace ParticleZoo