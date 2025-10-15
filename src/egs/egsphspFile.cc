
#include "particlezoo/egs/egsphspFile.h"

#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <iostream>

#include "particlezoo/ByteBuffer.h"

namespace ParticleZoo::EGSphspFile
{

    CLICommand EGSIgnoreHeaderCountCommand{ READER, "", "EGS-ignore-header-count", "Ignore the number of particles specified in the header and calculate it from the file size", { CLI_VALUELESS } };
    CLICommand EGSParticleZValueCommand{ READER, "", "EGS-particleZ", "Specify the Z value for all particles in the EGS phase space file", { CLI_FLOAT }, { 0.0f } };
    CLICommand EGSModeCommand{ WRITER, "", "EGS-mode", "Specify the EGS phase space file mode (MODE0 or MODE2)", { CLI_STRING }, { std::string("MODE0") } };

    constexpr float ELECTRON_REST_MASS_MEV = 0.5109989461f;    ///< Electron rest mass, stored explicitly in MeV for operating directly on the energy value in the file before conversion to internal units

    Reader::Reader(const std::string & fileName, const UserOptions & options)
    : PhaseSpaceFileReader("EGS", fileName, options), particleZValue_(0), mode_(EGSMODE::MODE0)
    {
        bool ignoreHeaderParticleCount = false;

        if (options.contains(EGSIgnoreHeaderCountCommand)) {
            CLIValue ignoreHeaderParticleCountValue = options.at(EGSIgnoreHeaderCountCommand)[0];
            ignoreHeaderParticleCount = std::get<bool>(ignoreHeaderParticleCountValue);
        }

        if (options.contains(EGSParticleZValueCommand)) {
            CLIValue particleZValue = options.at(EGSParticleZValueCommand)[0];
            particleZValue_ = std::get<float>(particleZValue) * cm;
        }

        setConstantZ(particleZValue_);

        readHeader(ignoreHeaderParticleCount);
    }

    std::vector<CLICommand> Reader::getFormatSpecificCLICommands() {
        return { EGSIgnoreHeaderCountCommand, EGSParticleZValueCommand };
    }

    void Reader::readHeader(bool ignoreHeaderParticleCount)
    {
        ByteBuffer headerBuffer = getHeaderData();

        std::string modeString = headerBuffer.readString(4);
        if (modeString != "MODE") {
            throw std::runtime_error("Invalid EGS phase-space file.");
        }

        byte modeByte5 = headerBuffer.read<byte>();
        if (modeByte5 == '0') {
            mode_ = EGSMODE::MODE0;
        } else if (modeByte5 == '2') {
            mode_ = EGSMODE::MODE2;
        } else {
            throw std::runtime_error("Unsupported EGS phase-space file mode.");
        }

        numberOfParticles_ = headerBuffer.read<unsigned int>();

        if (ignoreHeaderParticleCount) {
            std::cout << "Overriding header particle count " << numberOfParticles_;
            numberOfParticles_ = (unsigned int) (getFileSize() - getParticleRecordStartOffset()) / getParticleRecordLength();
            std::cout << " with " << numberOfParticles_ << std::endl;
        }

        numberOfPhotons_ = headerBuffer.read<unsigned int>();
        maxKineticEnergy_ = headerBuffer.read<float>() * MeV;
        minElectronEnergy_ = headerBuffer.read<float>() * MeV;
        numberOfOriginalHistories_ = headerBuffer.read<float>();
    }

    Particle Reader::readBinaryParticle(ByteBuffer & buffer)
    {
        unsigned int LATCH = buffer.read<unsigned int>();
        float energy = buffer.read<float>(); // keep in explicit MeV for now
        float x = buffer.read<float>() * cm;
        float y = buffer.read<float>() * cm;
        float z = particleZValue_; // EGS format does not store the particle z value
        float u = buffer.read<float>();
        float v = buffer.read<float>();
        float w = calcThirdUnitComponent(u, v);

        float weight = buffer.read<float>();
        bool wSignIsNegative = weight < 0;
        if (wSignIsNegative) {
            w = -w; // restore w directional component sign
            weight = -weight; // restore weight to positive
        }

        bool isNewHistory = energy < 0;
        if (isNewHistory) { energy = -energy; }
        
        bool isMultiPasser = ((LATCH >> 31) & 1) == 1;

        unsigned int particleChargeBits = (LATCH >> 29) & 3;

        ParticleType type;
        switch (particleChargeBits) {
            case 0: // Neutral particle
                type = ParticleType::Photon;
                break;
            case 1: // Negative particle
                type = ParticleType::Electron;
                energy -= ELECTRON_REST_MASS_MEV; // Convert to kinetic energy
                break;
            case 2: // Positive particle
                type = ParticleType::Positron;
                energy -= ELECTRON_REST_MASS_MEV; // Convert to kinetic energy
                break;
            default: // Error
                throw std::runtime_error("Invalid particle charge bits.");
        };
        energy *= MeV; // Convert to internal units

        Particle particle(type, energy, x, y, z, u, v, w, isNewHistory, weight);
        particle.setIntProperty(IntPropertyType::EGS_LATCH, LATCH);

        if (mode_ == EGSMODE::MODE2) {
            float ZLAST = buffer.read<float>() * cm;
            particle.setFloatProperty(FloatPropertyType::ZLAST, ZLAST);
        }

        particle.setBoolProperty(BoolPropertyType::IS_MULTIPLE_CROSSER, isMultiPasser);

        return particle;
    }


    // Writer class implementation
    Writer::Writer(const std::string & fileName, const UserOptions & options)
    : PhaseSpaceFileWriter("EGS", fileName, options)
    {
        mode_ = EGSMODE::MODE0; // Default mode, MODE2 requires source particles to include ZLAST information

        if (options.contains(EGSModeCommand)) {
            CLIValue modeValue = options.at(EGSModeCommand)[0];
            std::string modeStr = std::get<std::string>(modeValue);
            if (modeStr == "MODE0") {
                mode_ = EGSMODE::MODE0;
            } else if (modeStr == "MODE2") {
                mode_ = EGSMODE::MODE2;
            } else {
                throw std::runtime_error("Unsupported EGS phase-space file mode: " + modeStr);
            }
        }
    }

    std::vector<CLICommand> Writer::getFormatSpecificCLICommands() {
        return { EGSModeCommand };
    }

    void Writer::writeHeaderData(ByteBuffer & buffer)
    {
        switch (mode_) {
            case EGSMODE::MODE0:
                buffer.writeString("MODE0");
                break;
            case EGSMODE::MODE2:
                buffer.writeString("MODE2");
                break;
            default:
                throw std::runtime_error("Unsupported EGS phase-space file mode.");
        }
        buffer.write<unsigned int>(numberOfParticles_);
        buffer.write<unsigned int>(numberOfPhotons_);
        buffer.write<float>(maxKineticEnergy_ / MeV);
        buffer.write<float>(minElectronEnergy_ / MeV);

        std::uint64_t historiesRecorded = getHistoriesWritten();
        if (historiesRecorded > numberOfOriginalHistories_) {
            numberOfOriginalHistories_ = static_cast<float>(historiesRecorded);
        }

        buffer.write<float>(numberOfOriginalHistories_);
    }

    void Writer::writeBinaryParticle(ByteBuffer & buffer, Particle & particle)
    {
        numberOfParticles_++;
        if (particle.getType() == ParticleType::Photon) {
            numberOfPhotons_++;
        }

        constexpr float inv_cm = 1.0f / cm;
        constexpr float inv_MeV = 1.0f / MeV;

        float energy = particle.getKineticEnergy(); // keep in internal units for now
        float x = particle.getX() * inv_cm;
        float y = particle.getY() * inv_cm;
        // EGS doesn't store the Z value
        float u = particle.getDirectionalCosineX();
        float v = particle.getDirectionalCosineY();
        float w = particle.getDirectionalCosineZ();
        float weight = particle.getWeight();
        if (w < 0) {
            weight = -weight; // store weight as negative to indicate negative w direction
        }

        // Update energy stats in internal units
        if (energy > maxKineticEnergy_) {
            maxKineticEnergy_ = energy;
        }
        if (particle.getType() == ParticleType::Electron && energy < minElectronEnergy_) {
            minElectronEnergy_ = energy;
        }

        unsigned int LATCH;
        if (particle.hasIntProperty(IntPropertyType::EGS_LATCH)) {
            LATCH = (unsigned int) particle.getIntProperty(IntPropertyType::EGS_LATCH);
        } else {
            LATCH = 0;
        }

        if (particle.hasBoolProperty(BoolPropertyType::IS_MULTIPLE_CROSSER) && particle.getBoolProperty(BoolPropertyType::IS_MULTIPLE_CROSSER)) {
            LATCH |= (1 << 31);
        } else {
            LATCH &= ~(1 << 31);
        }
        
        energy *= inv_MeV; // Convert to MeV before adding rest mass if needed

        unsigned int particleChargeBits;
        switch (particle.getType()) {
            case ParticleType::Photon:
                particleChargeBits = 0;
                break;
            case ParticleType::Electron:
                particleChargeBits = 1;
                energy += ELECTRON_REST_MASS_MEV; // Convert to total energy
                break;
            case ParticleType::Positron:
                particleChargeBits = 2;
                energy += ELECTRON_REST_MASS_MEV; // Convert to total energy
                break;
            default:
                throw std::runtime_error("Particle type " + std::string(getParticleTypeName(particle.getType())) + " not supported by EGS phase-space file format.");
        }
        LATCH |= (particleChargeBits << 29);

        if (particle.isNewHistory()) {
            energy *= -1;
            if (!historyCountManualSet_) numberOfOriginalHistories_++;
        }

        buffer.write<unsigned int>(LATCH);
        buffer.write<float>(energy);
        buffer.write<float>(x);
        buffer.write<float>(y);
        buffer.write<float>(u);
        buffer.write<float>(v);
        buffer.write<float>(weight);

        if (mode_ == EGSMODE::MODE2) {
            float ZLAST;
            if (particle.hasFloatProperty(FloatPropertyType::ZLAST)) {
                ZLAST = particle.getFloatProperty(FloatPropertyType::ZLAST) * inv_cm;
            } else {
                throw std::runtime_error("Missing ZLAST property for particle which is required for writting MODE2 EGS phase space files.");
            }
            buffer.write<float>(ZLAST);
        }
    }

}