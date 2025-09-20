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
    class FixedValues;

    struct SupportedFormat
    {
        const std::string name; // e.g. "egsphsp", "IAEAphsp", etc.
        const std::string description; // e.g. "EGS Phase Space File Format"
        const std::string fileExtension; // e.g. ".egsphsp", ".IAEAphsp", etc.
        const bool fileExtensionCanHaveSuffix{false}; // true if the file extension can have a suffix like ".egsphsp1"
    };

    class FormatRegistry
    {
    public:
        constexpr static bool FILE_EXTENSION_CAN_HAVE_SUFFIX = true;

        using ReaderFactoryFn = std::function<std::unique_ptr<PhaseSpaceFileReader>(const std::string& filename, const UserOptions& options)>;
        using WriterFactoryFn = std::function<std::unique_ptr<PhaseSpaceFileWriter>(const std::string& filename, const UserOptions& options, const FixedValues& fixedValues)>;

        static void RegisterFormat(const SupportedFormat& fmt, ReaderFactoryFn reader, WriterFactoryFn writer);

        static const std::vector<SupportedFormat> SupportedFormats();

        static std::unique_ptr<PhaseSpaceFileReader> CreateReader(const std::string& filename, const UserOptions & options = {});
        static std::unique_ptr<PhaseSpaceFileReader> CreateReader(const std::string& name, const std::string& filename, const UserOptions & options = {});

        static std::unique_ptr<PhaseSpaceFileWriter> CreateWriter(const std::string& filename, const UserOptions & options = {}, const FixedValues & fixedValues = {});
        static std::unique_ptr<PhaseSpaceFileWriter> CreateWriter(const std::string& name, const std::string& filename, const UserOptions & options = {}, const FixedValues & fixedValues = {});

        static std::vector<SupportedFormat> FormatsForExtension(const std::string& extension);

        static void PrintSupportedFormats();

        static void RegisterStandardFormats();

    private:
        FormatRegistry() = default;
        static FormatRegistry& instance();

        std::vector<SupportedFormat> formats_;
        std::map<std::string, ReaderFactoryFn> readerFactories_;
        std::map<std::string, WriterFactoryFn> writerFactories_;
        mutable std::mutex mutex_;
    };

} // namespace ParticleZoo