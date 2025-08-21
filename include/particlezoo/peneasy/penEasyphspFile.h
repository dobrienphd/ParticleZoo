#ifndef PENEASYPHSPFILEWRITER_H
#define PENEASYPHSPFILEWRITER_H

#include <string>
#include <limits>
#include <list>

#include "particlezoo/PhaseSpaceFileWriter.h"
#include "particlezoo/PhaseSpaceFileReader.h"
#include "particlezoo/Particle.h"
#include "particlezoo/ByteBuffer.h"

namespace ParticleZoo
{
    namespace penEasyphspFile
    {
        // Constants for the penEasyphsp file format
        static constexpr std::size_t HEADER_LENGTH = 112;
        static constexpr std::size_t MAX_ASCII_LINE_LENGTH = 205;
        static constexpr std::string_view FILE_HEADER = "# [PHASE SPACE FILE FORMAT penEasy v.2008-05-15]\n# KPAR : E : X : Y : Z : U : V : W : WGHT : DeltaN : ILB(1..5)\n";

        constexpr IntPropertyType PROPERTY_PENELOPE_ILB[5] = {
            IntPropertyType::PENELOPE_ILB1,
            IntPropertyType::PENELOPE_ILB2,
            IntPropertyType::PENELOPE_ILB3,
            IntPropertyType::PENELOPE_ILB4,
            IntPropertyType::PENELOPE_ILB5
        };

        class Writer : public PhaseSpaceFileWriter
        {
            public:
                Writer(const std::string & fileName, const UserOptions & options = UserOptions{});

                std::uint64_t getMaximumSupportedParticles() const override;

            protected:
                std::size_t getParticleRecordStartOffset() const override;

                void writeHeaderData(ByteBuffer & buffer) override;
                const std::string writeASCIIParticle(Particle & particle) override;
                size_t getMaximumASCIILineLength() const override;
        };

        inline std::uint64_t Writer::getMaximumSupportedParticles() const { return std::numeric_limits<std::uint64_t>::max(); }
        inline std::size_t Writer::getParticleRecordStartOffset() const { return HEADER_LENGTH; }
        inline size_t Writer::getMaximumASCIILineLength() const { return MAX_ASCII_LINE_LENGTH; }


        class Reader : public PhaseSpaceFileReader
        {
            public:
                Reader(const std::string & fileName, const UserOptions & options = UserOptions{});
    
                std::uint64_t getNumberOfParticles() const override;
                std::uint64_t getNumberOfOriginalHistories() const override;
                std::uint64_t getNumberOfParticlesRead() const;
    
            protected:
                Particle readASCIIParticle(const std::string & line) override;
                size_t getMaximumASCIILineLength() const override;

            private:
                std::uint64_t numberOfParticles_;
                std::uint64_t numberOfParticlesRead_;
                std::uint64_t numberOfOriginalHistoriesRead_;
        };

        inline std::uint64_t Reader::getNumberOfOriginalHistories() const { return numberOfOriginalHistoriesRead_; }
        inline std::uint64_t Reader::getNumberOfParticlesRead() const { return numberOfParticlesRead_; }
        inline size_t Reader::getMaximumASCIILineLength() const { return MAX_ASCII_LINE_LENGTH; }

    }
}

#endif // PENEASYPHSPFILEWRITER_H