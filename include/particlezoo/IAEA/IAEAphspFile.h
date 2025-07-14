#ifndef IAEAPHSPFILE_H
#define IAEAPHSPFILE_H

#include "particlezoo/PhaseSpaceFileReader.h"
#include "particlezoo/PhaseSpaceFileWriter.h"
#include "particlezoo/IAEA/IAEAHeader.h"

namespace ParticleZoo::IAEAphspFile
{
    constexpr std::string_view IAEAphspFileExtension = ".IAEAphsp";

    class Reader : public PhaseSpaceFileReader
    {
        public:
            Reader(const std::string &filename, const UserOptions & options = UserOptions{});

            std::uint64_t getNumberOfParticles() const override;
            std::uint64_t getNumberOfOriginalHistories() const override;
            std::uint64_t getNumberOfParticles(ParticleType particleType) const;

            const IAEAHeader & getHeader() const;

        protected:
            std::size_t   getParticleRecordStartOffset() const override;
            std::size_t   getParticleRecordLength() const override;

            Particle      readBinaryParticle(ByteBuffer & buffer) override;

        private:
            const IAEAHeader header_;

    };

    class Writer : public PhaseSpaceFileWriter
    {
        public:
            Writer(const std::string &filename, const UserOptions & userOptions = UserOptions{});
            Writer(const std::string & filename, const IAEAHeader & templateHeader);

            void setNumberOfOriginalHistories(std::uint64_t numberOfHistories);

            std::uint64_t getMaximumSupportedParticles() const override;
            IAEAHeader & getHeader();

        protected:
            std::size_t getParticleRecordLength() const override;
            void writeHeaderData(ByteBuffer & buffer) override;
            void writeBinaryParticle(ByteBuffer & buffer, Particle & particle) override;

        private:
            IAEAHeader header_;
            bool useCustomHistoryCount_{false};
            std::uint64_t custumNumberOfHistories_{};

    };

    // Inline implementations for the IAEAphspFileReader class
    inline const IAEAHeader & Reader::getHeader() const { return header_; }
    inline std::uint64_t Reader::getNumberOfParticles() const { return header_.getNumberOfParticles(); }
    inline std::uint64_t Reader::getNumberOfOriginalHistories() const { return header_.getOriginalHistories(); }
    inline std::uint64_t Reader::getNumberOfParticles(ParticleType particleType) const { return header_.getNumberOfParticles(particleType); }
    inline std::size_t Reader::getParticleRecordStartOffset() const { return 0; }
    inline std::size_t Reader::getParticleRecordLength() const { return header_.getRecordLength(); }

    // Inline implementations for the IAEAphspFileWriter class
    inline IAEAHeader & Writer::getHeader() { return header_; }
    inline std::uint64_t Writer::getMaximumSupportedParticles() const { return std::numeric_limits<std::uint64_t>::max(); }
    inline std::size_t Writer::getParticleRecordLength() const { return header_.getRecordLength(); }
}

#endif // IAEAPHSPFILE_H