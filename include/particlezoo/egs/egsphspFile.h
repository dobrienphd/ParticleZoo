
#ifndef EGSPHSPFILE_H
#define EGSPHSPFILE_H

#include "particlezoo/PhaseSpaceFileReader.h"
#include "particlezoo/PhaseSpaceFileWriter.h"

#include <string>
#include <limits>

#include "particlezoo/Particle.h"

namespace ParticleZoo
{
    namespace EGSphspFile {

        constexpr std::size_t MINIMUM_HEADER_DATA_LENGTH = 25;
        enum class EGSMODE { MODE0 = 28, MODE2 = 32 };
        const double ELECTRON_REST_MASS = 0.5109989461f; // MeV

        class Reader : public PhaseSpaceFileReader
        {
            public:
                Reader(const std::string & fileName, const UserOptions & options = UserOptions{});

                std::uint64_t getNumberOfParticles() const override { return static_cast<std::uint64_t>(numberOfParticles_); };
                std::uint64_t getNumberOfOriginalHistories() const override { return static_cast<std::uint64_t>(numberOfOriginalHistories_); };

                EGSMODE getMode() const { return mode_; };
                unsigned int getNumberOfPhotons() const { return numberOfPhotons_; };
                float getMaxKineticEnergy() const { return maxKineticEnergy_; };
                float getMinElectronEnergy() const { return minElectronEnergy_; };

            protected:
                std::size_t getParticleRecordLength() const override { return std::max(static_cast<std::size_t>(mode_), MINIMUM_HEADER_DATA_LENGTH); };
                std::size_t getParticleRecordStartOffset() const override { return getParticleRecordLength(); };

                Particle readBinaryParticle(ByteBuffer & buffer) override;

            private:
                EGSMODE mode_;
                unsigned int numberOfParticles_{};
                unsigned int numberOfPhotons_{};
                float maxKineticEnergy_{};
                float minElectronEnergy_{};
                float numberOfOriginalHistories_{};
                float particleZValue_;

                void readHeader(bool ignoreHeaderParticleCount);
        };
    


        class Writer : public PhaseSpaceFileWriter
        {
            public:
                Writer(const std::string & fileName, const UserOptions & options = UserOptions{});

                std::uint64_t getMaximumSupportedParticles() const override { return static_cast<unsigned int>(numberOfParticles_); };

                void setNumberOfOriginalHistories(unsigned int numberOfOriginalHistories) {
                    numberOfOriginalHistories_ = (float)numberOfOriginalHistories;
                    historyCountManualSet_ = true;
                }

            protected:
                std::size_t getParticleRecordLength() const override { return static_cast<std::size_t>(mode_); };
                std::size_t getParticleRecordStartOffset() const override { return getParticleRecordLength(); };
                virtual void writeHeaderData(ByteBuffer & buffer) override;
                virtual void writeBinaryParticle(ByteBuffer & buffer, Particle & particle) override;

            private:
                EGSMODE mode_;
                unsigned int numberOfParticles_{};
                unsigned int numberOfPhotons_{};
                float maxKineticEnergy_{};
                float minElectronEnergy_{std::numeric_limits<float>::infinity()};
                float numberOfOriginalHistories_{};
                bool historyCountManualSet_{false};
        };

    } // namespace EGSphspFile

} // namespace ParticleZoo

#endif