
#include "particlezoo/IAEA/IAEAphspFile.h"

#include <filesystem>

namespace ParticleZoo::IAEAphspFile
{

    constexpr float energyUnits = MeV;
    constexpr float distanceUnits = cm;

    CLICommand IAEAHeaderTemplateCommand{ WRITER, "", "IAEA-header-template", "Path to an IAEA header file from which to copy the attributes of the phase space file", { CLI_STRING } };
    CLICommand IAEAIndexCommand{ WRITER, "", "IAEA-index", "Index string for the IAEA phase space file header", { CLI_STRING } };
    CLICommand IAEATitleCommand{ WRITER, "", "IAEA-title", "Title string for the IAEA phase space file header", { CLI_STRING } };
    CLICommand IAEAFileTypeCommand{ WRITER, "", "IAEA-file-type", "File type for the IAEA phase space file header (PHSP_FILE or PHSP_GENERATOR)", { CLI_STRING }, { std::string("PHSP_FILE") } };
    CLICommand IAEAAddIncHistNumberCommand{ WRITER, "", "IAEA-incrementals", "Include the incremental history number extra long in the IAEA phase space file", { CLI_VALUELESS } };
    CLICommand IAEAAddEGSLATCHCommand{ WRITER, "", "IAEA-latch", "Include the EGS LATCH extra long in the IAEA phase space file", { CLI_VALUELESS } };
    CLICommand IAEAAddPENELOPEILB5Command{ WRITER, "", "IAEA-ilb5", "Include the PENELOPE ILB5 extra long in the IAEA phase space file", { CLI_VALUELESS } };
    CLICommand IAEAAddPENELOPEILB4Command{ WRITER, "", "IAEA-ilb4", "Include the PENELOPE ILB4 extra long in the IAEA phase space file", { CLI_VALUELESS } };
    CLICommand IAEAAddPENELOPEILB3Command{ WRITER, "", "IAEA-ilb3", "Include the PENELOPE ILB3 extra long in the IAEA phase space file", { CLI_VALUELESS } };
    CLICommand IAEAAddPENELOPEILB2Command{ WRITER, "", "IAEA-ilb2", "Include the PENELOPE ILB2 extra long in the IAEA phase space file", { CLI_VALUELESS } };
    CLICommand IAEAAddPENELOPEILB1Command{ WRITER, "", "IAEA-ilb1", "Include the PENELOPE ILB1 extra long in the IAEA phase space file", { CLI_VALUELESS } };
    CLICommand IAEAAddXLASTCommand{ WRITER, "", "IAEA-xlast", "Include the XLAST extra float in the IAEA phase space file", { CLI_VALUELESS } };
    CLICommand IAEAAddYLASTCommand{ WRITER, "", "IAEA-ylast", "Include the YLAST extra float in the IAEA phase space file", { CLI_VALUELESS } };
    CLICommand IAEAAddZLASTCommand{ WRITER, "", "IAEA-zlast", "Include the ZLAST extra float in the IAEA phase space file", { CLI_VALUELESS } };
    CLICommand IAEAIgnoreChecksumCommand{ READER, "", "IAEA-ignore-checksum", "Ignore checksum errors when reading an IAEA phase space file", { CLI_VALUELESS } };

    // Implementations for the IAEAphspFileReader class

    const IAEAHeader initializeHeader(const UserOptions & options, const std::string & filename) {
        IAEAHeader header_ = IAEAHeader(IAEAHeader::DeterminePathToHeaderFile(filename));
        std::string phspPath = header_.getDataFilePath();
        bool ignoreChecksum = options.contains(IAEAIgnoreChecksumCommand);
        if (!header_.checksumIsValid()) {
            if (ignoreChecksum) {
                // try to do some repair on these values
                std::uint64_t checksum = header_.getChecksum();
                std::uint64_t particleCount = header_.getNumberOfParticles();

                const std::size_t   recordLength = header_.getRecordLength();
                const std::size_t   fileSize = static_cast<std::size_t>(std::filesystem::file_size(phspPath));

                // Check that the checksum matches the file size, if not update it
                if (checksum != fileSize) {
                    checksum = fileSize;
                    header_.setChecksum(checksum);
                }

                // Check that the number of particles matches the file size and record length, if not update it
                if (particleCount * recordLength != fileSize) {
                    particleCount = fileSize / recordLength;
                    header_.setNumberOfParticles(particleCount);
                }
            } else {
                throw std::runtime_error("The checksum in the IAEA header '" + header_.getHeaderFilePath() + "' is invalid. The file may be corrupted.");
            }
        }
        return header_;
    }

    Reader::Reader(const std::string & filename, const UserOptions & options)
        : PhaseSpaceFileReader("IAEA", filename, options), header_(initializeHeader(options, filename))
    {
        if (!header_.xIsStored()) setConstantX(header_.getConstantX());
        if (!header_.yIsStored()) setConstantY(header_.getConstantY());
        if (!header_.zIsStored()) setConstantZ(header_.getConstantZ());
        if (!header_.uIsStored()) setConstantPx(header_.getConstantU());
        if (!header_.vIsStored()) setConstantPy(header_.getConstantV());
        if (!header_.wIsStored()) setConstantPz(header_.getConstantW());
        if (!header_.weightIsStored()) setConstantWeight(header_.getConstantWeight());
    }

    std::vector<CLICommand> Reader::getFormatSpecificCLICommands() {
        return { IAEAIgnoreChecksumCommand };
    }

    Particle Reader::readBinaryParticle(ByteBuffer & buffer)
    {
        signed_byte typeCode = buffer.read<signed_byte>();

        short is;
        if (typeCode < 0)
        {
            is = -1;
            typeCode = -typeCode;
        } else is = 1;

        ParticleType particleType;
        switch (typeCode)
        {
            case 1: particleType = ParticleType::Photon; break;
            case 2: particleType = ParticleType::Electron; break;
            case 3: particleType = ParticleType::Positron; break;
            case 4: particleType = ParticleType::Neutron; break;
            case 5: particleType = ParticleType::Proton; break;
            default: throw std::runtime_error("Unsupported particle type in IAEAphsp file.");
        }

        float kineticEnergy = buffer.read<float>();
        bool isNewHistory;
        if (kineticEnergy < 0)
        {
            isNewHistory = true;
            kineticEnergy = -kineticEnergy;
        } else isNewHistory = false;

        kineticEnergy *= energyUnits;

        float x, y, z, u, v, w, weight;
        if (header_.xIsStored()) x = buffer.read<float>() * distanceUnits; else x = header_.getConstantX();
        if (header_.yIsStored()) y = buffer.read<float>() * distanceUnits; else y = header_.getConstantY();
        if (header_.zIsStored()) z = buffer.read<float>() * distanceUnits; else z = header_.getConstantZ();
        if (header_.uIsStored()) u = buffer.read<float>(); else u = header_.getConstantU();
        if (header_.vIsStored()) v = buffer.read<float>(); else v = header_.getConstantV();
        if (header_.wIsStored()) w = is * calcThirdUnitComponent(u, v); else w = header_.getConstantW();
        if (header_.weightIsStored()) weight = buffer.read<float>(); else weight = header_.getConstantWeight();

        if (weight < 0) {
            throw std::runtime_error("Negative particle weight read from IAEA phase space file, which is not allowed.");
        }

        Particle particle(particleType, kineticEnergy, x, y, z, u, v, w, isNewHistory, weight);

        unsigned int N_extraFloats = header_.getNumberOfExtraFloats();
        for (unsigned int i = 0; i < N_extraFloats; i++)
        {
            float extraFloat = buffer.read<float>();
            IAEAHeader::EXTRA_FLOAT_TYPE IAEAextraFloatType = header_.getExtraFloatType(i);
            FloatPropertyType extraFloatType = IAEAHeader::translateExtraFloatType(IAEAextraFloatType);
            particle.setFloatProperty(extraFloatType, extraFloat);
        }

        unsigned int N_extraLongs = header_.getNumberOfExtraLongs();
        for (unsigned int i = 0; i < N_extraLongs; i++)
        {
            std::int32_t extraLong = buffer.read<std::int32_t>();
            IAEAHeader::EXTRA_LONG_TYPE IAEAextraLongType = header_.getExtraLongType(i);

            if (!isNewHistory && IAEAextraLongType == IAEAHeader::EXTRA_LONG_TYPE::INCREMENTAL_HISTORY_NUMBER && extraLong > 0) {
                // This indicates the start of a new history, despite the kinetic energy not being negative (can help recover a malformed file)
                isNewHistory = true;
                particle.setNewHistory(true);
            }

            IntPropertyType extraLongType = IAEAHeader::translateExtraLongType(IAEAextraLongType);
            particle.setIntProperty(extraLongType, extraLong);
        }

        return particle;
    }


    // Implementations for the IAEAphspFileWriter class


    Writer::Writer(const std::string & filename, const UserOptions & userOptions, const FixedValues & fixedValues)
        : PhaseSpaceFileWriter("IAEA", filename, userOptions, FormatType::BINARY, fixedValues),
          header_([&]() {
            CLIValue headerFileTemplateValue = userOptions.contains(IAEAHeaderTemplateCommand) ? 
                userOptions.at(IAEAHeaderTemplateCommand)[0] : std::string{};

            std::string headerFileTemplatePath = std::get<std::string>(headerFileTemplateValue);

            IAEAHeader header = headerFileTemplatePath.length() > 0 ? 
                IAEAHeader(IAEAHeader(headerFileTemplatePath), IAEAHeader::DeterminePathToHeaderFile(filename)) : 
                IAEAHeader(IAEAHeader::DeterminePathToHeaderFile(filename), true);

            if (headerFileTemplatePath.length() > 0) {
                header.setFilePath(IAEAHeader::DeterminePathToHeaderFile(filename));
            }

            if (userOptions.contains(IAEAIndexCommand)) {
                CLIValue indexValue = userOptions.at(IAEAIndexCommand).front();
                header.setIAEAIndex(std::get<std::string>(indexValue));
            }
            if (userOptions.contains(IAEATitleCommand)) {
                CLIValue titleValue = userOptions.at(IAEATitleCommand).front();
                header.setTitle(std::get<std::string>(titleValue));
            }
            if (userOptions.contains(IAEAFileTypeCommand)) {
                CLIValue fileTypeValue = userOptions.at(IAEAFileTypeCommand).front();
                std::string fileType = std::get<std::string>(fileTypeValue);
                if (fileType == "PHSP_FILE") {
                    header.setFileType(IAEAHeader::FileType::PHSP_FILE);
                } else if (fileType == "PHSP_GENERATOR") {
                    header.setFileType(IAEAHeader::FileType::PHSP_GENERATOR);
                } else {
                    throw std::invalid_argument("Invalid IAEA file type specified: " + fileType);
                }
            }
            if (userOptions.contains(IAEAAddIncHistNumberCommand)) {
                CLIValue incHistNumberValue = userOptions.at(IAEAAddIncHistNumberCommand).front();
                if (std::get<bool>(incHistNumberValue)) {
                    header.addExtraLong(IAEAHeader::EXTRA_LONG_TYPE::INCREMENTAL_HISTORY_NUMBER);
                }
            }
            if (userOptions.contains(IAEAAddEGSLATCHCommand)) {
                CLIValue egsLatchValue = userOptions.at(IAEAAddEGSLATCHCommand).front();
                if (std::get<bool>(egsLatchValue)) {
                    header.addExtraLong(IAEAHeader::EXTRA_LONG_TYPE::EGS_LATCH);
                }
            }
            if (userOptions.contains(IAEAAddPENELOPEILB5Command)) {
                CLIValue penelopeILB5Value = userOptions.at(IAEAAddPENELOPEILB5Command).front();
                if (std::get<bool>(penelopeILB5Value)) {
                    header.addExtraLong(IAEAHeader::EXTRA_LONG_TYPE::PENELOPE_ILB5);
                }
            }
            if (userOptions.contains(IAEAAddPENELOPEILB4Command)) {
                CLIValue penelopeILB4Value = userOptions.at(IAEAAddPENELOPEILB4Command).front();
                if (std::get<bool>(penelopeILB4Value)) {
                    header.addExtraLong(IAEAHeader::EXTRA_LONG_TYPE::PENELOPE_ILB4);
                }
            }
            if (userOptions.contains(IAEAAddPENELOPEILB3Command)) {
                CLIValue penelopeILB3Value = userOptions.at(IAEAAddPENELOPEILB3Command).front();
                if (std::get<bool>(penelopeILB3Value)) {
                    header.addExtraLong(IAEAHeader::EXTRA_LONG_TYPE::PENELOPE_ILB3);
                }
            }
            if (userOptions.contains(IAEAAddPENELOPEILB2Command)) {
                CLIValue penelopeILB2Value = userOptions.at(IAEAAddPENELOPEILB2Command).front();
                if (std::get<bool>(penelopeILB2Value)) {
                    header.addExtraLong(IAEAHeader::EXTRA_LONG_TYPE::PENELOPE_ILB2);
                }
            }
            if (userOptions.contains(IAEAAddPENELOPEILB1Command)) {
                CLIValue penelopeILB1Value = userOptions.at(IAEAAddPENELOPEILB1Command).front();
                if (std::get<bool>(penelopeILB1Value)) {
                    header.addExtraLong(IAEAHeader::EXTRA_LONG_TYPE::PENELOPE_ILB1);
                }
            }
            if (userOptions.contains(IAEAAddXLASTCommand)) {
                CLIValue xLastValue = userOptions.at(IAEAAddXLASTCommand).front();
                if (std::get<bool>(xLastValue)) {
                    header.addExtraFloat(IAEAHeader::EXTRA_FLOAT_TYPE::XLAST);
                }
            }
            if (userOptions.contains(IAEAAddYLASTCommand)) {
                CLIValue yLastValue = userOptions.at(IAEAAddYLASTCommand).front();
                if (std::get<bool>(yLastValue)) {
                    header.addExtraFloat(IAEAHeader::EXTRA_FLOAT_TYPE::YLAST);
                }
            }
            if (userOptions.contains(IAEAAddZLASTCommand)) {
                CLIValue zLastValue = userOptions.at(IAEAAddZLASTCommand).front();
                if (std::get<bool>(zLastValue)) {
                    header.addExtraFloat(IAEAHeader::EXTRA_FLOAT_TYPE::ZLAST);
                }
            }
            return header;
          }())
    {
        // update constants in the header
        fixedValuesHaveChanged();
    }

    std::vector<CLICommand> Writer::getFormatSpecificCLICommands() {
        return {
            IAEAHeaderTemplateCommand,
            IAEAIndexCommand,
            IAEATitleCommand,
            IAEAFileTypeCommand,
            IAEAAddIncHistNumberCommand,
            IAEAAddEGSLATCHCommand,
            IAEAAddPENELOPEILB5Command,
            IAEAAddPENELOPEILB4Command,
            IAEAAddPENELOPEILB3Command,
            IAEAAddPENELOPEILB2Command,
            IAEAAddPENELOPEILB1Command,
            IAEAAddXLASTCommand,
            IAEAAddYLASTCommand,
            IAEAAddZLASTCommand
        };
    }

    void Writer::setNumberOfOriginalHistories(std::uint64_t numberOfHistories) {
        useCustomHistoryCount_ = true;
        custumNumberOfHistories_ = numberOfHistories;
        header_.setOriginalHistories(numberOfHistories);
    }

    void Writer::writeBinaryParticle(ByteBuffer & buffer, Particle & particle) {
        signed_byte typeCode = 0;
        switch (particle.getType()) {
            case ParticleType::Photon: typeCode = 1; break;
            case ParticleType::Electron: typeCode = 2; break;
            case ParticleType::Positron: typeCode = 3; break;
            case ParticleType::Neutron: typeCode = 4; break;
            case ParticleType::Proton: typeCode = 5; break;
            default: throw std::runtime_error("Unsupported particle type in IAEAphsp file.");
        }

        // The sign of the type code indicates the direction of the particle.
        float wValue = particle.getDirectionalCosineZ();
        if (wValue < 0.f) {
            typeCode = -typeCode;
        }

        constexpr float inverseEnergyUnits = 1.f / energyUnits;
        float kineticEnergy = particle.getKineticEnergy() * inverseEnergyUnits;
        if (particle.isNewHistory()) kineticEnergy *= -1.f;

        buffer.write<signed_byte>(typeCode);
        buffer.write<float>(kineticEnergy);

        constexpr float inverseDistanceUnits = 1.f / distanceUnits;
        if (header_.xIsStored()) buffer.write<float>(particle.getX() * inverseDistanceUnits);
        if (header_.yIsStored()) buffer.write<float>(particle.getY() * inverseDistanceUnits);
        if (header_.zIsStored()) buffer.write<float>(particle.getZ() * inverseDistanceUnits);
        if (header_.uIsStored()) buffer.write<float>(particle.getDirectionalCosineX());
        if (header_.vIsStored()) buffer.write<float>(particle.getDirectionalCosineY());
        if (header_.weightIsStored()) buffer.write<float>(particle.getWeight());

        unsigned int N_extraFloats = header_.getNumberOfExtraFloats();
        unsigned int customFloatIndex = 0;
        const std::vector<float> & customFloatProperties = particle.getCustomFloatProperties();
        for (unsigned int i = 0; i < N_extraFloats; i++)
        {
            float extraFloatValue;
            IAEAHeader::EXTRA_FLOAT_TYPE IAEAFloatType = header_.getExtraFloatType(i);
            FloatPropertyType floatType = IAEAHeader::translateExtraFloatType(IAEAFloatType);
            if (floatType == FloatPropertyType::CUSTOM)
            {
                if (customFloatIndex >= customFloatProperties.size()) {
                    extraFloatValue = 0.f;
                } else {
                    extraFloatValue = customFloatProperties[customFloatIndex++];
                }
            } else {
                if (particle.hasFloatProperty(floatType))
                    extraFloatValue = particle.getFloatProperty(floatType);
                else
                    extraFloatValue = 0.f; // Default value if the property is not set
            }
            buffer.write<float>(extraFloatValue);
        }

        unsigned int N_extraLongs = header_.getNumberOfExtraLongs();
        unsigned int customLongIndex = 0;
        const std::vector<std::int32_t> & customLongProperties = particle.getCustomIntProperties();
        for (unsigned int i = 0; i < N_extraLongs; i++)
        {
            std::int32_t extraLongValue;
            IAEAHeader::EXTRA_LONG_TYPE IAEALongType = header_.getExtraLongType(i);
            IntPropertyType longType = IAEAHeader::translateExtraLongType(IAEALongType);
            if (longType == IntPropertyType::CUSTOM)
            {
                if (customLongIndex >= customLongProperties.size()) {
                    extraLongValue = 0;
                } else {
                    extraLongValue = customLongProperties[customLongIndex++];
                }
            } else {
                if (particle.hasIntProperty(longType)) {
                    extraLongValue = particle.getIntProperty(longType);
                } else {
                    if (longType == IntPropertyType::INCREMENTAL_HISTORY_NUMBER) {
                        // Special case for INCREMENTAL_HISTORY_NUMBER, which should be set based on the isNewHistory flag if it is not defined
                        extraLongValue = particle.isNewHistory() ? 1 : 0; // Incremental history number starts at 1 for new histories
                    } else {
                        // For other types, use the default value of zero
                        extraLongValue = 0;
                    }
                }
            }
            buffer.write<std::int32_t>(extraLongValue);
        }

        // Update the header with the particle statistics
        header_.countParticleStats(particle);
    }

    void Writer::writeHeaderData(ByteBuffer & buffer) {
        (void)buffer; // unused in this implementation
        if (useCustomHistoryCount_) {
            header_.setOriginalHistories(custumNumberOfHistories_);
        } else {
            std::uint64_t historiesRecorded = getHistoriesWritten();
            std::uint64_t historiesInHeader = header_.getOriginalHistories();
            if (historiesRecorded > historiesInHeader) {
                header_.setOriginalHistories(historiesRecorded);
            }
        }
        header_.writeHeader();
    }
}