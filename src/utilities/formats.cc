
#include "particlezoo/utilities/formats.h"

#include <iostream>

#include "particlezoo/Particle.h"
#include "particlezoo/PhaseSpaceFileReader.h"
#include "particlezoo/PhaseSpaceFileWriter.h"
#include "particlezoo/egs/egsphspFile.h"
#include "particlezoo/IAEA/IAEAphspFile.h"
#include "particlezoo/TOPAS/TOPASphspFile.h"
#include "particlezoo/peneasy/penEasyphspFile.h"

#ifdef USE_ROOT
#include "particlezoo/ROOT/ROOTphsp.h"
#endif

#ifdef PZ_USE_EXT
#include "particlezoo/ext_formats.h"
#endif

namespace ParticleZoo
{

    void FormatRegistry::RegisterStandardFormats()
    {
        static bool standardFormatsRegistered = false;
        if (standardFormatsRegistered) return;
        standardFormatsRegistered = true;

        // Register IAEAphsp format
        SupportedFormat iaeaFormat{"IAEA", "IAEA Phase Space File Format", ".IAEAphsp"};
        auto iaeaReaderCommands = IAEAphspFile::Reader::getFormatSpecificCLICommands();
        auto iaeaWriterCommands = IAEAphspFile::Writer::getFormatSpecificCLICommands();
        ArgParser::RegisterCommands(iaeaReaderCommands);
        ArgParser::RegisterCommands(iaeaWriterCommands);
        RegisterFormat(iaeaFormat,
                       [](const std::string& filename, const UserOptions & options) {
                           return std::make_unique<ParticleZoo::IAEAphspFile::Reader>(filename, options);
                       },
                       [](const std::string& filename, const UserOptions & options, const FixedValues & fixedValues) {
                           return std::make_unique<ParticleZoo::IAEAphspFile::Writer>(filename, options, fixedValues);
                       });

        // Register TOPAS format
        SupportedFormat topasFormat{"TOPAS", "TOPAS Phase Space File Formats (Binary, ASCII and Limited)", ".phsp"};
        auto topasReaderCommands = TOPASphspFile::Reader::getFormatSpecificCLICommands();
        auto topasWriterCommands = TOPASphspFile::Writer::getFormatSpecificCLICommands();
        ArgParser::RegisterCommands(topasReaderCommands);
        ArgParser::RegisterCommands(topasWriterCommands);
        RegisterFormat(topasFormat,
                       [](const std::string& filename, const UserOptions & options) {
                           return std::make_unique<ParticleZoo::TOPASphspFile::Reader>(filename, options);
                       },
                       [](const std::string& filename, const UserOptions & options, const FixedValues &) {
                           return std::make_unique<ParticleZoo::TOPASphspFile::Writer>(filename, options);
                       });
        
        // Register penEasy format
        SupportedFormat peneasyFormat{"penEasy", "penEasy ASCII Phase Space File Format", ".dat"};
        RegisterFormat(peneasyFormat,
                       [](const std::string& filename, const UserOptions & options) {
                           return std::make_unique<ParticleZoo::penEasyphspFile::Reader>(filename, options);
                       },
                       [](const std::string& filename, const UserOptions & options, const FixedValues &) {
                           return std::make_unique<ParticleZoo::penEasyphspFile::Writer>(filename, options);
                       });

        // Register EGS format
        SupportedFormat egsFormat{"EGS", "EGS Phase Space File Formats (MODE0 and MODE2)", ".egsphsp", FILE_EXTENSION_CAN_HAVE_SUFFIX};
        auto egsReaderCommands = EGSphspFile::Reader::getFormatSpecificCLICommands();
        auto egsWriterCommands = EGSphspFile::Writer::getFormatSpecificCLICommands();
        ArgParser::RegisterCommands(egsReaderCommands);
        ArgParser::RegisterCommands(egsWriterCommands);
        RegisterFormat(egsFormat,
                       [](const std::string& filename, const UserOptions & options) {
                           return std::make_unique<ParticleZoo::EGSphspFile::Reader>(filename, options);
                       },
                       [](const std::string& filename, const UserOptions & options, const FixedValues &) {
                           return std::make_unique<ParticleZoo::EGSphspFile::Writer>(filename, options);
                       });

    #ifdef USE_ROOT
        // Register ROOT format
        SupportedFormat rootFormat{"ROOT", "ROOT Phase Space File Format (TOPAS and OpenGATE templates available)", ".root"};
        auto rootReaderCommands = ROOT::Reader::getFormatSpecificCLICommands();
        auto rootWriterCommands = ROOT::Writer::getFormatSpecificCLICommands();
        ArgParser::RegisterCommands(rootReaderCommands);
        ArgParser::RegisterCommands(rootWriterCommands);
        RegisterFormat(rootFormat,
                       [](const std::string& filename, const UserOptions & options) {
                           return std::make_unique<ParticleZoo::ROOT::Reader>(filename, options);
                       },
                       [](const std::string& filename, const UserOptions & options, const FixedValues &) {
                           return std::make_unique<ParticleZoo::ROOT::Writer>(filename, options);
                       });
    #endif

    #ifdef PZ_USE_EXT
        RegisterExternalFormats();
    #endif

    }


    FormatRegistry& FormatRegistry::instance()
    {
        static FormatRegistry inst;
        return inst;
    }

    void FormatRegistry::RegisterFormat(const SupportedFormat& fmt, ReaderFactoryFn reader, WriterFactoryFn writer)
    {
        FormatRegistry& registry = instance();
        std::unique_lock lock(registry.mutex_);
        // Validate the format registration
        if (fmt.name.empty() || fmt.fileExtension.empty() || reader == nullptr || writer == nullptr) {
            throw std::invalid_argument("Invalid format registration");
        }

        // Check if the format is already registered
        if (std::find_if(registry.formats_.begin(), registry.formats_.end(),
            [&fmt](const SupportedFormat& f) { return f.name == fmt.name; }) != registry.formats_.end())
        {
            throw std::runtime_error("Format already registered: " + fmt.name);
        }

        // Register the format
        registry.readerFactories_[fmt.name] = std::move(reader);
        registry.writerFactories_[fmt.name] = std::move(writer);
        registry.formats_.push_back(fmt);
    }

    const std::vector<SupportedFormat> FormatRegistry::SupportedFormats() {
        auto& registry = FormatRegistry::instance();
        std::unique_lock lock(registry.mutex_);
        return registry.formats_;
    }

    std::unique_ptr<PhaseSpaceFileReader> FormatRegistry::CreateReader(const std::string& filename, const UserOptions & options)
    {
        std::filesystem::path p{filename};
        auto ext = p.extension().string();
        if (ext.empty()) {
            throw std::runtime_error("Filename does not have an extension: " + filename);
        }
        auto supportedFormats = FormatsForExtension(ext);
        if (supportedFormats.empty() || supportedFormats.size() > 1) {
            throw std::runtime_error("No unique format found for file extension: " + ext);
        }
        return CreateReader(supportedFormats[0].name, filename, options);
    }

    std::unique_ptr<PhaseSpaceFileReader> FormatRegistry::CreateReader(const std::string& name, const std::string& filename, const UserOptions & options)
    {
        FormatRegistry& registry = instance();
        std::unique_lock lock(registry.mutex_);
        auto it = registry.readerFactories_.find(name);
        if (it == registry.readerFactories_.end()) {
            throw std::runtime_error("Unsupported format: " + name);
        }
        return it->second(filename, options);
    }

    std::unique_ptr<PhaseSpaceFileWriter> FormatRegistry::CreateWriter(const std::string& filename, const UserOptions & options, const FixedValues & fixedValues)
    {
        std::filesystem::path p{filename};
        auto ext = p.extension().string();
        if (ext.empty()) {
            throw std::runtime_error("Filename does not have an extension: " + filename);
        }
        auto supportedFormats = FormatsForExtension(ext);
        if (supportedFormats.empty() || supportedFormats.size() > 1) {
            throw std::runtime_error("No unique format found for file extension: " + ext);
        }
        return CreateWriter(supportedFormats[0].name, filename, options, fixedValues);
    }

    std::unique_ptr<PhaseSpaceFileWriter> FormatRegistry::CreateWriter(const std::string& name, const std::string& filename, const UserOptions & options, const FixedValues & fixedValues)
    {
        FormatRegistry& registry = instance();
        std::unique_lock lock(registry.mutex_);
        auto it = registry.writerFactories_.find(name);
        if (it == registry.writerFactories_.end()) {
            throw std::runtime_error("Unsupported format: " + name);
        }
        return it->second(filename, options, fixedValues);
    }

    std::vector<SupportedFormat> FormatRegistry::FormatsForExtension(const std::string& extension)
    {
        FormatRegistry& registry = instance();
        std::unique_lock lock(registry.mutex_);

        // Convert the extensions to lowercase for case-insensitive comparison
        auto toLower = [](const std::string& stringToLower) {
            std::string out;
            out.reserve(stringToLower.size());
            for (unsigned char c : stringToLower) out.push_back(static_cast<char>(std::tolower(c)));
            return out;
        };

        std::string extLower = toLower(extension);
        std::vector<SupportedFormat> result;
        for (auto const& fmt : registry.formats_)
        {
            std::string fmtExtLower = toLower(fmt.fileExtension);
            if (fmtExtLower == extLower
                || (fmt.fileExtensionCanHaveSuffix
                    && extLower.rfind(fmtExtLower, 0) == 0))
            {
                result.push_back(fmt);
            }
        }
        return result;
    }

    void FormatRegistry::PrintSupportedFormats()
    {
        auto formats = SupportedFormats();
        if (formats.empty()) {
            std::cout << "No supported formats registered." << std::endl;
            return;
        }

        std::cout << "Supported Phase Space File Formats:" << std::endl;
        for (const auto& fmt : formats) {
            std::cout << " - " << fmt.name << ": " << fmt.description
                      << " (extension: " << fmt.fileExtension << ")" << std::endl;
        }
    }

}