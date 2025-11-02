
#include "particlezoo/utilities/argParse.h"
#include "particlezoo/utilities/formats.h"
#include "particlezoo/utilities/version.h"

#include "particlezoo/PhaseSpaceFileReader.h"
#include "particlezoo/PhaseSpaceFileWriter.h"

#include <algorithm>
#include <iomanip>
#include <cctype>

namespace ParticleZoo {

    /// @brief Registers a command for argument parsing.
    /// @param command The command to register.
    void ArgParser::RegisterCommand(const CLICommand& command)
    {
        // Retrieve the singleton instance
        ArgParser& parser = Instance();
        auto& commands = parser.commands;

        // Early exit if command is already registered
        if (commands.contains(command)) {
            return;
        }

        // Ensure command has at least one name
        if (command.shortName.empty() && command.longName.empty()) {
            throw std::runtime_error("Command must have at least a short or long name");
        }

        // Check against reserved names
        for (const auto& reserved : reservedCommands) {
            if (command.shortName == reserved || command.longName == reserved) {
                throw std::runtime_error("Command name '" + reserved + "' is reserved and cannot be used");
            }
        }

        // Check for duplicates
        for (const auto& cmd : commands) {
            if (cmd == command) {
                throw std::runtime_error("Command with same short or long name already registered: " 
                                         + (command.shortName.empty() ? "" : "-" + command.shortName)
                                         + (command.shortName.empty() || command.longName.empty() ? "" : ", ")
                                         + (command.longName.empty() ? "" : "--" + command.longName));
            }
        }

        // Add command to the list
        commands.insert(command);

        // Store default values if provided
        if (!command.defaultValues.empty()) {
            parser.setOptions[command] = command.defaultValues;
        }
    }

    /// @brief Registers multiple commands for argument parsing.
    /// @param commands The commands to register.
    void ArgParser::RegisterCommands(const std::vector<CLICommand>& commands) {
        for (const auto& cmd : commands) {
            RegisterCommand(cmd);
        }
    }

    /// @brief Parses command line arguments based on registered commands.
    /// @param argc Argument count.
    /// @param argv Argument vector.
    /// @param usageMessage Message to display for usage information.
    /// @param minimumPositionalArgs Minimum required positional arguments.
    /// @return A map of parsed user options.
    const UserOptions ArgParser::ParseArgs(int argc, char* argv[], const std::string_view & usageMessage, std::size_t minimumPositionalArgs)
    {
        // Retrieve the singleton instance
        ArgParser& parser = Instance();

        static bool hasBeenCalled = false;
        if (hasBeenCalled) return parser.setOptions; // Prevent re-parsing
        hasBeenCalled = true;

        // Register the --help and --formats command
        parser.commands.insert(CLICommand(BOTH, "h", "help",    "Display this help message",       { CLI_VALUELESS }));
        parser.commands.insert(CLICommand(BOTH, "f", "formats", "List all supported file formats", { CLI_VALUELESS }));
        parser.commands.insert(CLICommand(BOTH, "v", "version", "Display version information",     { CLI_VALUELESS }));

        auto readerCommands = PhaseSpaceFileReader::getCLICommands();
        auto writerCommands = PhaseSpaceFileWriter::getCLICommands();
        parser.RegisterCommands(readerCommands);
        parser.RegisterCommands(writerCommands);

        // Ensure all standard formats are registered so that their command line options are available
        FormatRegistry::RegisterStandardFormats();

        // Get registered commands
        auto& commands = parser.commands;

        // Lambda to print supported formats and exit
        auto printFormats = []() {
            FormatRegistry::PrintSupportedFormats();
            std::exit(0);
        };

        // Lambda to print version information and exit
        auto printVersion = []() {
            std::cout << Version::GetVersionString() << std::endl;
            std::exit(0);
        };

        // Lambda to parse a value based on expected type
        auto parseValue = [](const std::string& value, CLIArgType type) -> CLIValue {
            switch (type) {
                case CLI_FLOAT:
                    return std::stof(value);
                case CLI_INT:
                    return std::stoi(value);
                case CLI_STRING:
                    return value;
                case CLI_BOOL:
                    return (value == "true" || value == "1" || value == "yes");
                case CLI_VALUELESS:
                    return true; // presence of flag indicates true
                default:
                    throw std::runtime_error("Invalid CLI argument type");
            }
        };

        // Lambda to find a command by name
        auto findCommand = [&commands](const std::string& name, bool isShort) -> const CLICommand* {
            for (const auto& cmd : commands) {
                if (isShort && !cmd.shortName.empty() && cmd.shortName == name) {
                    return &cmd;
                }
                if (!isShort && !cmd.longName.empty() && cmd.longName == name) {
                    return &cmd;
                }
            }
            return nullptr;
        };

        std::vector<CLIValue> positional;
        UserOptions & opts = parser.setOptions; // Start with default options

        for(int i = 1; i < argc; i++) {
            std::string arg{argv[i]};
            
            if (arg.rfind("--", 0) == 0) {

                // Long option (double dash)
                std::string optName = arg.substr(2);

                // Handle help command
                if (optName == "help") {
                    PrintUsage(usageMessage, 0);
                }

                // Handle special case for formats command
                if (optName == "formats") {
                    printFormats();
                }

                // Handle special case for version command
                if (optName == "version") {
                    printVersion();
                }

                const CLICommand* cmd = findCommand(optName, false);
                
                if (!cmd) {
                    std::cerr << "Unknown option: --" << optName << std::endl;
                    PrintUsage(usageMessage);
                }

                std::vector<CLIValue> values;
                
                // Parse expected arguments based on command definition
                for (auto argType : cmd->argTypes) {
                    if (argType == CLI_VALUELESS) {
                        values.push_back(true);
                    } else {
                        if (++i >= argc) {
                            std::cerr << "Option --" << optName << " requires an argument" << std::endl;
                            PrintUsage(usageMessage);
                        }
                        try {
                            CLIValue value = parseValue(argv[i], argType);
                            values.push_back(value);
                        } catch (const std::exception&) {
                            std::cerr << "Invalid value for option --" << optName << ": " << argv[i] << std::endl;
                            PrintUsage(usageMessage);
                        }
                    }
                }
                
                opts[*cmd] = values;

            } else if (arg.rfind("-", 0) == 0 && arg.length() > 1) {
                
                // Short option (single dash)
                std::string optName = arg.substr(1);

                // Handle help command
                if (optName == "h") {
                    PrintUsage(usageMessage, 0);
                }

                // Handle special case for formats command
                if (optName == "f") {
                    printFormats();
                }

                // Handle special case for version command
                if (optName == "v") {
                    printVersion();
                }

                const CLICommand* cmd = findCommand(optName, true);
                
                if (!cmd) {
                    std::cerr << "Unknown option: -" << optName << std::endl;
                    PrintUsage(usageMessage);
                }

                std::vector<CLIValue> values;
                
                // Parse expected arguments based on command definition
                for (auto argType : cmd->argTypes) {
                    if (argType == CLI_VALUELESS) {
                        values.push_back(true);
                    } else {
                        if (++i >= argc) {
                            std::cerr << "Option -" << optName << " requires an argument" << std::endl;
                            PrintUsage(usageMessage);
                        }
                        try {
                            CLIValue value = parseValue(argv[i], argType);
                            values.push_back(value);
                        } catch (const std::exception&) {
                            std::cerr << "Invalid value for option -" << optName << ": " << argv[i] << std::endl;
                            PrintUsage(usageMessage);
                        }
                    }
                }
                
                opts[*cmd] = values;
                
            } else {
                // Positional argument
                positional.push_back(arg);
            }
        }

        // Check for required positional arguments
        if (positional.size() < minimumPositionalArgs) {
            std::cerr << "Expected at least " << minimumPositionalArgs << " positional arguments, got " << positional.size() << std::endl;
            PrintUsage(usageMessage);
        }

        // Store positional arguments
        opts[CLI_POSITIONALS] = positional;

        return opts;
    }

    void ArgParser::PrintUsage(const std::string_view & usageMessage, const int exitCode) {
        // Retrieve the singleton instance
        ArgParser& parser = Instance();
        auto& commands = parser.commands;

        // Sort commands
        std::vector<CLICommand> sortedCommands(commands.begin(), commands.end());
        std::sort(sortedCommands.begin(), sortedCommands.end(), 
                [](const CLICommand& a, const CLICommand& b) {
                    // Sort by long name if available, otherwise by short name
                    std::string nameA = a.longName.empty() ? a.shortName : a.longName;
                    std::string nameB = b.longName.empty() ? b.shortName : b.longName;
                  
                    // Convert to lowercase for case-insensitive comparison
                    std::transform(nameA.begin(), nameA.end(), nameA.begin(),
                                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                    std::transform(nameB.begin(), nameB.end(), nameB.begin(),
                                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                        return nameA < nameB;
                });

        // Helper lambda to convert CLIValue to string
        auto valueToString = [](const CLIValue& value) -> std::string {
            return std::visit([](auto&& arg) -> std::string {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, std::string>) {
                    return arg;
                } else if constexpr (std::is_same_v<T, bool>) {
                    return arg ? "true" : "false";
                } else {
                    return std::to_string(arg);
                }
            }, value);
        };

        // Helper lambda to convert CLIArgType to string
        auto typeToString = [](CLIArgType type) -> std::string {
            switch (type) {
                case CLI_FLOAT: return "number";
                case CLI_INT: return "integer";
                case CLI_STRING: return "text";
                case CLI_BOOL: return "true/false";
                case CLI_VALUELESS: return "";
                default: return "value";
            }
        };

        // Calculate maximum width for alignment
        size_t maxWidth = 0;
        for (const auto& cmd : sortedCommands) {
            std::string shortOpt = cmd.shortName.empty() ? "" : " -" + cmd.shortName;
            std::string longOpt = cmd.longName.empty() ? "" : "--" + cmd.longName;
            std::string opts = shortOpt;
            if (!shortOpt.empty() && !longOpt.empty()) opts += ", ";
            opts += longOpt;

            // Add parameter types to the option string
            for (size_t i = 0; i < cmd.argTypes.size(); ++i) {
                std::string typeStr = typeToString(cmd.argTypes[i]);
                if (!typeStr.empty()) {
                    opts += " <" + typeStr + ">";
                }
            }

            maxWidth = std::max(maxWidth, opts.length());
        }

        // Print usage information
        if (!usageMessage.empty()) std::cout << usageMessage << "\n\n";
        std::cout << "Available options:\n";
        for (const auto& cmd : sortedCommands) {
            std::string shortOpt = cmd.shortName.empty() ? "" : " -" + cmd.shortName;
            std::string longOpt = cmd.longName.empty() ? "" : "--" + cmd.longName;
            std::string opts = shortOpt;
            if (!shortOpt.empty() && !longOpt.empty()) opts += ", ";
            opts += longOpt;

            // Add parameter types to the option string
            for (size_t i = 0; i < cmd.argTypes.size(); ++i) {
                std::string typeStr = typeToString(cmd.argTypes[i]);
                if (!typeStr.empty()) {
                    opts += " <" + typeStr + ">";
                }
            }

            // Build description with default values if they exist
            std::string description = cmd.description;
            if (!cmd.defaultValues.empty()) {
                description += " (default: ";
                for (size_t i = 0; i < cmd.defaultValues.size(); ++i) {
                    if (i > 0) description += ", ";
                    description += valueToString(cmd.defaultValues[i]);
                }
                description += ")";
            }

            std::cout << "  " << std::left << std::setw(maxWidth) << opts 
                    << "  " << description << "\n";
        }
        std::exit(exitCode);
    }

} // namespace ParticleZoo