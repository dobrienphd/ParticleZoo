
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

    /**
     * @brief Type alias for command line argument values.
     * 
     * A variant type that can hold different types of values that can be passed
     * as command line arguments: floating point numbers, integers, strings, or booleans.
     */
    using CLIValue = std::variant<float,int,std::string,bool>;

    /**
     * @brief Enumeration for command line argument types.
     * 
     * Defines the different data types that command line arguments can accept.
     * Used for validation and type conversion during argument parsing.
     */
    enum CLIArgType { 
        CLI_FLOAT,      ///< Floating point number argument
        CLI_INT,        ///< Integer number argument  
        CLI_STRING,     ///< String argument
        CLI_BOOL,       ///< Boolean flag argument
        CLI_VALUELESS   ///< Flag with no associated value
    };

    /**
     * @brief Enumeration for command line argument context.
     * 
     * Specifies whether a command line argument is applicable to readers,
     * writers, both, or neither (general purpose).
     */
    enum CLIArgContext { 
        READER,     ///< Argument applies only to file readers
        WRITER,     ///< Argument applies only to file writers
        BOTH,       ///< Argument applies to both readers and writers
        NONE        ///< General purpose argument (not specific to readers/writers)
    };


    /**
     * @brief Structure representing a command line argument definition.
     * 
     * Defines a command line argument including its names, human-readable
     * description, expected argument types, default values, and context of use.
     * Supports both single-dash short names (e.g., -h) and double-dash long names (e.g., --help).
     */
    struct CLICommand
    {
        CLIArgContext context;                  ///< Context where this command applies (reader/writer/both/none)
        std::string shortName;                  ///< Short name used with single dash (e.g., "h" for -h)
        std::string longName;                   ///< Full name used with double dash (e.g., "help" for --help)
        std::string description;                ///< Human-readable description of the command
        std::vector<CLIArgType> argTypes;       ///< Types of arguments this command expects
        std::vector<CLIValue> defaultValues;    ///< Default values for the arguments (optional)

        /**
         * @brief Construct a new CLICommand object.
         * 
         * @param context The context where this command applies
         * @param shortName The short name for single-dash usage (empty string if not used)
         * @param longName The long name for double-dash usage (empty string if not used)
         * @param description Human-readable description of the command
         * @param types List of argument types this command expects
         * @param defaults List of default values (must match types if provided)
         * @throws std::runtime_error if default values don't match argument types
         */
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
        
        /**
         * @brief Copy constructor.
         */
        CLICommand(const CLICommand&) = default;
        
        /**
         * @brief Copy assignment operator.
         */
        CLICommand& operator=(const CLICommand&) = default;
        
        /**
         * @brief Move constructor.
         */
        CLICommand(CLICommand&&) = default;
        
        /**
         * @brief Move assignment operator.
         */
        CLICommand& operator=(CLICommand&&) = default;

        /**
         * @brief Equality comparison operator.
         * 
         * Two commands are considered equal if they have the same short or long names.
         * 
         * @param o The other CLICommand to compare with
         * @return true if commands are equal, false otherwise
         */
        bool operator==(CLICommand const& o) const noexcept {
            return shortName == o.shortName && longName == o.longName;
        }
    };

}

/**
 * @brief Hash function specialization for CLICommand.
 * 
 * Enables CLICommand to be used as a key in hash-based containers like
 * std::unordered_map and std::unordered_set.
 */
namespace std {
    template<>
    struct hash<ParticleZoo::CLICommand> {
        /**
         * @brief Compute hash value for a CLICommand.
         * 
         * @param cmd The CLICommand to hash
         * @return std::size_t The computed hash value
         */
        std::size_t operator()(const ParticleZoo::CLICommand& cmd) const noexcept {
            // Combine hashes of shortName and longName
            std::size_t h1 = std::hash<std::string>{}(cmd.shortName);
            std::size_t h2 = std::hash<std::string>{}(cmd.longName);
            return h1 ^ (h2 << 1);
        }
    };
}

namespace ParticleZoo {

    /**
     * @brief Type alias for user options map.
     * 
     * Maps CLICommand objects to vectors of CLIValue objects, representing
     * the parsed command line arguments. Each command can have multiple values
     * if it accepts multiple arguments.
     */
    using UserOptions = std::unordered_map<CLICommand, std::vector<CLIValue>>;

    /**
     * @brief Special command representing positional arguments.
     * 
     * This predefined command is used internally to handle positional arguments
     * (arguments without flags) passed on the command line.
     */
    const CLICommand CLI_POSITIONALS { BOTH, "", "positionals", "", { CLI_STRING } };

    /**
     * @brief Singleton class for parsing command line arguments.
     * 
     * ArgParser provides a centralized system for registering command line argument
     * definitions and parsing command line input. It uses the singleton pattern to
     * maintain a global registry of available commands and their configurations.
     * 
     * Designed to provide a consistent interface for command line parsing across
     * the bundled tools in ParticleZoo.
     * 
     * The class is designed to be used statically - register commands during
     * initialization and parse arguments when needed.
     */
    class ArgParser {
        private:
            std::unordered_set<CLICommand> commands;    ///< Registry of all registered commands
            UserOptions setOptions;                     ///< Currently parsed user options

            /**
             * @brief Private constructor for singleton pattern.
             */
            ArgParser() {};

            /**
             * @brief Deleted copy constructor to prevent copying.
             */
            ArgParser(const ArgParser&) = delete;
            
            /**
             * @brief Deleted copy assignment operator to prevent copying.
             */
            ArgParser& operator=(const ArgParser&) = delete;
            
            /**
             * @brief Deleted move constructor to prevent moving.
             */
            ArgParser(ArgParser&&) = delete;
            
            /**
             * @brief Deleted move assignment operator to prevent moving.
             */
            ArgParser& operator=(ArgParser&&) = delete;

            /**
             * @brief Retrieve the singleton instance of ArgParser.
             * 
             * @return ArgParser& Reference to the singleton instance
             */
            static ArgParser& Instance() {
                static ArgParser instance;
                return instance;
            }

            /**
             * @brief Reserved command names that cannot be used for user options.
             * 
             * These commands are reserved for built-in functionality to avoid conflicts.
             */
            inline static const std::array<std::string, 7> reservedCommands { "h", "help", "f", "formats", "v", "version", "positionals" };
        public:
            /**
             * @brief Default destructor.
             */
            ~ArgParser() = default;

            /**
             * @brief Registers a single command for argument parsing.
             * 
             * Adds a command definition to the global registry, making it available
             * for parsing. The command must not conflict with reserved commands.
             * 
             * @param command The command definition to register
             * @throws std::runtime_error if the command conflicts with reserved names
             */
            static void RegisterCommand(const CLICommand& command);

            /**
             * @brief Registers multiple commands for argument parsing.
             * 
             * Convenience method to register a vector of commands at once.
             * Each command must not conflict with reserved commands.
             * 
             * @param commands The vector of command definitions to register
             * @throws std::runtime_error if any command conflicts with reserved names
             */
            static void RegisterCommands(const std::vector<CLICommand>& commands);

            /**
             * @brief Prints usage information for the command line interface.
             * 
             * Displays the usage message along with all registered commands and their
             * descriptions. Exits the program with the specified exit code.
             * 
             * @param usageMessage The main usage message to display
             * @param exitCode The exit code to use when terminating the program (default: 1)
             */
            static void PrintUsage(std::string usageMessage, int exitCode = 1);

            /**
             * @brief Parses command line arguments based on registered commands.
             * 
             * Processes the command line arguments according to the registered command
             * definitions. Validates argument types and counts, handles both short and
             * long command forms, and extracts positional arguments.
             * 
             * @param argc The number of command line arguments
             * @param argv The array of command line argument strings
             * @param usageMessage The usage message to display on error
             * @param minimumPositionalArgs The minimum number of positional arguments required (default: 0)
             * @return const UserOptions A map of parsed commands and their values
             * @throws Calls PrintUsage() and exits on parsing errors
             */
            static const UserOptions ParseArgs(int argc, char* argv[], const std::string & usageMessage, std::size_t minimumPositionalArgs = 0);
    };

} // namespace ParticleZoo
