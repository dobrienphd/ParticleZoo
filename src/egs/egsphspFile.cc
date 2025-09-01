
#include "particlezoo/egs/egsphspFile.h"

#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <iostream>

#include "particlezoo/ByteBuffer.h"

namespace ParticleZoo::EGSphspFile
{

    Reader::Reader(const std::string & fileName, const UserOptions & options)
    : PhaseSpaceFileReader("EGS", fileName, options), particleZValue_(0), mode_(EGSMODE::MODE0)
    {
        bool ignoreHeaderParticleCount = false;
        if (options.contains("EGSIgnoreHeaderCount")) {
            ignoreHeaderParticleCount = options.at("EGSIgnoreHeaderCount").front() == "true";
        }
        if (options.contains("EGSParticleZValue")) {
            particleZValue_ = std::stof(options.at("EGSParticleZValue").front());
        }

        setConstantZ(particleZValue_);

        readHeader(ignoreHeaderParticleCount);
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
        maxKineticEnergy_ = headerBuffer.read<float>();
        minElectronEnergy_ = headerBuffer.read<float>();
        numberOfOriginalHistories_ = headerBuffer.read<float>();
    }

    Particle Reader::readBinaryParticle(ByteBuffer & buffer)
    {
        unsigned int LATCH = buffer.read<unsigned int>();
        float energy = buffer.read<float>();
        float x = buffer.read<float>();
        float y = buffer.read<float>();
        float z = particleZValue_; // EGS format does not store the particle z value
        float u = buffer.read<float>();
        float v = buffer.read<float>();

        float uuvv = std::min(1.f, u*u+v*v);
        float w = std::sqrt(1.f - uuvv);

        float weight = buffer.read<float>();

        bool isNewHistory = energy < 0;
        if (isNewHistory) { energy = -energy; }
        
        bool isMultiPasser = ((LATCH >> 31) & 1) == 1;

        unsigned int particleChargeBits    = (LATCH >> 29) & 3;

        ParticleType type;
        switch (particleChargeBits) {
            case 0: // Neutral particle
                type = ParticleType::Photon;
                break;
            case 1: // Negative particle
                type = ParticleType::Electron;
                energy -= ELECTRON_REST_MASS; // Convert to kinetic energy
                break;
            case 2: // Positive particle
                type = ParticleType::Positron;
                energy -= ELECTRON_REST_MASS; // Convert to kinetic energy
                break;
            default: // Error
                throw std::runtime_error("Invalid particle charge bits.");
        };

        Particle particle(type, energy, x, y, z, u, v, w, isNewHistory, weight);
        particle.setIntProperty(IntPropertyType::EGS_LATCH, LATCH);

        if (mode_ == EGSMODE::MODE2) {
            float ZLAST = buffer.read<float>();
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

        if (options.contains("EGSMode")) {
            std::string modeStr = options.at("EGSMode").front();
            if (modeStr == "MODE0") {
                mode_ = EGSMODE::MODE0;
            } else if (modeStr == "MODE2") {
                mode_ = EGSMODE::MODE2;
            } else {
                throw std::runtime_error("Unsupported EGS phase-space file mode: " + modeStr);
            }
        }
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
        buffer.write<float>(maxKineticEnergy_);
        buffer.write<float>(minElectronEnergy_);

        std::uint64_t historiesRecorded = getHistoriesWritten();
        if (historiesRecorded > numberOfOriginalHistories_) {
            numberOfOriginalHistories_ = historiesRecorded;
        }

        buffer.write<float>(numberOfOriginalHistories_);
    }

    void Writer::writeBinaryParticle(ByteBuffer & buffer, Particle & particle)
    {
        numberOfParticles_++;
        if (particle.getType() == ParticleType::Photon) {
            numberOfPhotons_++;
        }

        float energy = particle.getKineticEnergy();
        float x = particle.getX();
        float y = particle.getY();
        // EGS doesn't store the Z value
        float u = particle.getPx();
        float v = particle.getPy();
        float weight = particle.getWeight();

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
        
        unsigned int particleChargeBits;
        switch (particle.getType()) {
            case ParticleType::Photon:
                particleChargeBits = 0;
                break;
            case ParticleType::Electron:
                particleChargeBits = 1;
                energy += ELECTRON_REST_MASS; // Convert to total energy
                break;
            case ParticleType::Positron:
                particleChargeBits = 2;
                energy += ELECTRON_REST_MASS; // Convert to total energy
                break;
            default:
                throw std::runtime_error("Particle type not supported by EGS phase-space file format.");
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
                ZLAST = particle.getFloatProperty(FloatPropertyType::ZLAST);
            } else {
                throw std::runtime_error("Missing ZLAST property for particle which is required for writting MODE2 EGS phase space files.");
            }
            buffer.write<float>(ZLAST);
        }
    }

}