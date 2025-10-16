#pragma once

#include <string>
#include <memory>
#include <map>
#include <vector>
#include <algorithm>
#include <functional>
#include <mutex>
#include <filesystem>

#include "particlezoo/utilities/argParse.h"
#include "particlezoo/Particle.h"

namespace ParticleZoo
{

    class PhaseSpaceFileReader;
    class PhaseSpaceFileWriter;
    struct FixedValues;

    /**
     * @brief Structure describing a supported phase space file format.
     * 
     * Contains metadata about a file format including its name, description,
     * file extension, and whether the extension can have numeric suffixes.
     */
    struct SupportedFormat
    {
        const std::string name;                                   ///< Format name (e.g., "IAEA", "EGS", "TOPAS")
        const std::string description;                            ///< Human-readable description of the format
        const std::string fileExtension;                          ///< Standard file extension (e.g., ".egsphsp", ".IAEAphsp")
        const bool fileExtensionCanHaveSuffix{false};             ///< True if extension can have numeric suffixes (e.g., ".egsphsp1")
    };

    /**
     * @brief Singleton registry for managing phase space file format readers and writers.
     * 
     * The FormatRegistry provides a centralized thread-safe system for registering,
     * discovering, and creating phase space file readers and writers for different
     * simulation formats. It supports automatic format detection based on file extensions
     * and provides factory methods for creating appropriate reader/writer instances.
     * 
     * The registry is typically populated during application startup by calling
     * RegisterStandardFormats(), which registers all built-in format handlers.
     */
    class FormatRegistry
    {
    public:
        /**
         * @brief Constant indicating that a file extension can have numeric suffixes.
         * 
         * Used when registering formats that support numbered file extensions
         * (e.g., ".egsphsp1", ".egsphsp2").
         */
        constexpr static bool FILE_EXTENSION_CAN_HAVE_SUFFIX = true;

        /**
         * @brief Function type for creating phase space file readers.
         * 
         * @param filename The path to the file to read
         * @param options User options for configuring the reader
         * @return std::unique_ptr<PhaseSpaceFileReader> A unique pointer to the created reader
         */
        using ReaderFactoryFn = std::function<std::unique_ptr<PhaseSpaceFileReader>(const std::string& filename, const UserOptions& options)>;
        
        /**
         * @brief Function type for creating phase space file writers.
         * 
         * @param filename The path to the file to write
         * @param options User options for configuring the writer
         * @param fixedValues Fixed values for constant particle properties
         * @return std::unique_ptr<PhaseSpaceFileWriter> A unique pointer to the created writer
         */
        using WriterFactoryFn = std::function<std::unique_ptr<PhaseSpaceFileWriter>(const std::string& filename, const UserOptions& options, const FixedValues& fixedValues)>;

        /**
         * @brief Register a new phase space file format with reader and writer factories.
         * 
         * Registers a format with the global registry, making it available for automatic
         * detection and creation. The format name must be unique.
         * 
         * @param fmt The format metadata (name, description, extension, etc.)
         * @param reader Factory function for creating readers of this format
         * @param writer Factory function for creating writers of this format
         * @throws std::invalid_argument if format parameters are invalid
         * @throws std::runtime_error if format name is already registered
         */
        static void RegisterFormat(const SupportedFormat& fmt, ReaderFactoryFn reader, WriterFactoryFn writer);

        /**
         * @brief Get a list of all registered formats.
         * 
         * Returns a copy of all currently registered format metadata.
         * 
         * @return const std::vector<SupportedFormat> Vector of all supported formats
         */
        static const std::vector<SupportedFormat> SupportedFormats();

        /**
         * @brief Create a reader for a file using automatic format detection.
         * 
         * Determines the appropriate format based on the file extension and creates
         * a reader instance. Requires a unique format match for the extension.
         * 
         * @param filename The path to the file to read (must have a recognized extension)
         * @param options User options for configuring the reader (default: empty)
         * @return std::unique_ptr<PhaseSpaceFileReader> A unique pointer to the created reader
         * @throws std::runtime_error if no extension found, no format matches, or multiple formats match
         */
        static std::unique_ptr<PhaseSpaceFileReader> CreateReader(const std::string& filename, const UserOptions & options = {});
        
        /**
         * @brief Create a reader for a specific format and file.
         * 
         * Creates a reader instance for the specified format, bypassing automatic detection.
         * 
         * @param formatName The name of the format to use (must be registered)
         * @param filename The path to the file to read
         * @param options User options for configuring the reader (default: empty)
         * @return std::unique_ptr<PhaseSpaceFileReader> A unique pointer to the created reader
         * @throws std::runtime_error if the format is not registered
         */
        static std::unique_ptr<PhaseSpaceFileReader> CreateReader(const std::string& formatName, const std::string& filename, const UserOptions & options = {});

        /**
         * @brief Create a writer for a file using automatic format detection.
         * 
         * Determines the appropriate format based on the file extension and creates
         * a writer instance. Requires a unique format match for the extension.
         * 
         * @param filename The path to the file to write (must have a recognized extension)
         * @param options User options for configuring the writer (default: empty)
         * @param fixedValues Fixed values for constant particle properties (default: empty)
         * @return std::unique_ptr<PhaseSpaceFileWriter> A unique pointer to the created writer
         * @throws std::runtime_error if no extension found, no format matches, or multiple formats match
         */
        static std::unique_ptr<PhaseSpaceFileWriter> CreateWriter(const std::string& filename, const UserOptions & options = {}, const FixedValues & fixedValues = {});
        
        /**
         * @brief Create a writer for a specific format and file.
         * 
         * Creates a writer instance for the specified format, bypassing automatic detection.
         * 
         * @param formatName The name of the format to use (must be registered)
         * @param filename The path to the file to write
         * @param options User options for configuring the writer (default: empty)
         * @param fixedValues Fixed values for constant particle properties (default: empty)
         * @return std::unique_ptr<PhaseSpaceFileWriter> A unique pointer to the created writer
         * @throws std::runtime_error if the format is not registered
         */
        static std::unique_ptr<PhaseSpaceFileWriter> CreateWriter(const std::string& formatName, const std::string& filename, const UserOptions & options = {}, const FixedValues & fixedValues = {});

        /**
         * @brief Find all formats that support a given file extension.
         * 
         * Performs case-insensitive matching of file extensions. Also handles formats
         * that support extensions with numeric suffixes (e.g., ".egsphsp1" matching ".egsphsp").
         * 
         * @param extension The file extension to search for (including the dot)
         * @return std::vector<SupportedFormat> Vector of formats supporting the extension
         */
        static std::vector<SupportedFormat> FormatsForExtension(const std::string& extension);
        
        /**
         * @brief Get the standard file extension for a specific format.
         * 
         * @param formatName The name of the format to query
         * @return const std::string The standard file extension for the format
         * @throws std::runtime_error if the format is not registered
         */
        static const std::string ExtensionForFormat(const std::string& formatName);

        /**
         * @brief Print a list of all supported formats to standard output.
         * 
         * Displays format names, descriptions, and file extensions in a human-readable format.
         */
        static void PrintSupportedFormats();

        /**
         * @brief Register all standard built-in phase space file formats.
         * 
         * This method registers the standard formats supported by ParticleZoo including:
         * - IAEA format (.IAEAphsp)
         * - TOPAS format (.phsp)
         * - penEasy format (.dat)
         * - EGS format (.egsphsp with suffixes)
         * - ROOT format (.root) - if compiled with ROOT support
         * 
         * This method is safe to call multiple times (uses internal flag to prevent duplicate registration).
         * Should be called during application initialization before using the registry.
         */
        static void RegisterStandardFormats();

    private:
        /**
         * @brief Private constructor to enforce singleton pattern.
         */
        FormatRegistry() = default;
        
        /**
         * @brief Get the singleton instance of the FormatRegistry.
         * 
         * @return FormatRegistry& Reference to the singleton instance
         */
        static FormatRegistry& instance();

        std::vector<SupportedFormat> formats_;                     ///< List of registered formats
        std::map<std::string, ReaderFactoryFn> readerFactories_;   ///< Map of format names to reader factory functions
        std::map<std::string, WriterFactoryFn> writerFactories_;   ///< Map of format names to writer factory functions
        mutable std::mutex mutex_;                                 ///< Mutex for thread-safe access to registry data
    };

} // namespace ParticleZoo