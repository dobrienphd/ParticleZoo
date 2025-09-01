
#include "particlezoo/IAEA/IAEAphspFile.h"

namespace ParticleZoo::IAEAphspFile
{

    // Implementations for the IAEAphspFileReader class

    Reader::Reader(const std::string & filename, const UserOptions & options)
        : PhaseSpaceFileReader("IAEA", filename, options), header_(IAEAHeader(IAEAHeader::DeterminePathToHeaderFile(filename)))
    {
        if (!header_.xIsStored()) setConstantX(header_.getConstantX());
        if (!header_.yIsStored()) setConstantY(header_.getConstantY());
        if (!header_.zIsStored()) setConstantZ(header_.getConstantZ());
        if (!header_.uIsStored()) setConstantPx(header_.getConstantU());
        if (!header_.vIsStored()) setConstantPy(header_.getConstantV());
        if (!header_.wIsStored()) setConstantPz(header_.getConstantW());
        if (!header_.weightIsStored()) setConstantWeight(header_.getConstantWeight());
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

        float x, y, z, u, v, w, weight;
        if (header_.xIsStored()) x = buffer.read<float>(); else x = header_.getConstantX();
        if (header_.yIsStored()) y = buffer.read<float>(); else y = header_.getConstantY();
        if (header_.zIsStored()) z = buffer.read<float>(); else z = header_.getConstantZ();
        if (header_.uIsStored()) u = buffer.read<float>(); else u = header_.getConstantU();
        if (header_.vIsStored()) v = buffer.read<float>(); else v = header_.getConstantV();
        if (header_.wIsStored()) {
            float uuvv = std::min(1.f, u*u+v*v);
            w = is * std::sqrt(1.f - uuvv);
        } else w = header_.getConstantW();
        if (header_.weightIsStored()) weight = buffer.read<float>(); else weight = header_.getConstantWeight();

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
            IntPropertyType extraLongType = IAEAHeader::translateExtraLongType(IAEAextraLongType);
            particle.setIntProperty(extraLongType, extraLong);
        }

        return particle;
    }


    // Implementations for the IAEAphspFileWriter class


    Writer::Writer(const std::string & filename, const UserOptions & userOptions, const FixedValues & fixedValues)
        : PhaseSpaceFileWriter("IAEA", filename, userOptions, FormatType::BINARY, fixedValues),
          header_([&]() {
            IAEAHeader header = userOptions.contains("IAEAHeaderTemplate") ? 
                IAEAHeader(IAEAHeader(userOptions.at("IAEAHeaderTemplate").front())) : 
                IAEAHeader(IAEAHeader::DeterminePathToHeaderFile(filename), true);

            if (userOptions.contains("IAEAIndex")) {
                header.setIAEAIndex(userOptions.at("IAEAIndex").front());
            }
            if (userOptions.contains("IAEATitle")) {
                header.setTitle(userOptions.at("IAEATitle").front());
            }
            if (userOptions.contains("IAEAFileType")) {
                std::string fileType = userOptions.at("IAEAFileType").front();
                if (fileType == "PHSP_FILE") {
                    header.setFileType(IAEAHeader::FileType::PHSP_FILE);
                } else if (fileType == "PHSP_GENERATOR") {
                    header.setFileType(IAEAHeader::FileType::PHSP_GENERATOR);
                } else {
                    throw std::invalid_argument("Invalid IAEA file type specified: " + fileType);
                }
            }
            if (userOptions.contains("IAEAAddIncHistNumber")) {
                if (userOptions.at("IAEAAddIncHistNumber").front() == "true") {
                    header.addExtraLong(IAEAHeader::EXTRA_LONG_TYPE::INCREMENTAL_HISTORY_NUMBER);
                }
            }
            if (userOptions.contains("IAEAAddEGSLATCH")) {
                if (userOptions.at("IAEAAddEGSLATCH").front() == "true") {
                    header.addExtraLong(IAEAHeader::EXTRA_LONG_TYPE::EGS_LATCH);
                }
            }
            if (userOptions.contains("IAEAAddPENELOPEILB5")) {
                if (userOptions.at("IAEAAddPENELOPEILB5").front() == "true") {
                    header.addExtraLong(IAEAHeader::EXTRA_LONG_TYPE::PENELOPE_ILB5);
                }
            }
            if (userOptions.contains("IAEAAddPENELOPEILB4")) {
                if (userOptions.at("IAEAAddPENELOPEILB4").front() == "true") {
                    header.addExtraLong(IAEAHeader::EXTRA_LONG_TYPE::PENELOPE_ILB4);
                }
            }
            if (userOptions.contains("IAEAAddPENELOPEILB3")) {
                if (userOptions.at("IAEAAddPENELOPEILB3").front() == "true") {
                    header.addExtraLong(IAEAHeader::EXTRA_LONG_TYPE::PENELOPE_ILB3);
                }
            }
            if (userOptions.contains("IAEAAddPENELOPEILB2")) {
                if (userOptions.at("IAEAAddPENELOPEILB2").front() == "true") {
                    header.addExtraLong(IAEAHeader::EXTRA_LONG_TYPE::PENELOPE_ILB2);
                }
            }
            if (userOptions.contains("IAEAAddPENELOPEILB1")) {
                if (userOptions.at("IAEAAddPENELOPEILB1").front() == "true") {
                    header.addExtraLong(IAEAHeader::EXTRA_LONG_TYPE::PENELOPE_ILB1);
                }
            }
            if (userOptions.contains("IAEAAddXLast")) {
                if (userOptions.at("IAEAAddXLast").front() == "true") {
                    header.addExtraFloat(IAEAHeader::EXTRA_FLOAT_TYPE::XLAST);
                }
            }
            if (userOptions.contains("IAEAAddYLast")) {
                if (userOptions.at("IAEAAddYLast").front() == "true") {
                    header.addExtraFloat(IAEAHeader::EXTRA_FLOAT_TYPE::YLAST);
                }
            }
            if (userOptions.contains("IAEAAddZLast")) {
                if (userOptions.at("IAEAAddZLast").front() == "true") {
                    header.addExtraFloat(IAEAHeader::EXTRA_FLOAT_TYPE::ZLAST);
                }
            }
            return header;
          }())
    {
        // update constants in the header
        fixedValuesHaveChanged();
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

        float kineticEnergy = particle.getKineticEnergy();
        if (particle.isNewHistory()) kineticEnergy *= -1.f;

        buffer.write<signed_byte>(typeCode);
        buffer.write<float>(kineticEnergy);

        if (header_.xIsStored()) buffer.write<float>(particle.getX());
        if (header_.yIsStored()) buffer.write<float>(particle.getY());
        if (header_.zIsStored()) buffer.write<float>(particle.getZ());
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