#pragma once

#include "particlezoo/Particle.h"

#include <fstream>
#include <string>
#include <cstdint>
#include <list>

#include "particlezoo/ByteBuffer.h"

namespace ParticleZoo
{
    using UserOptions = std::unordered_map<std::string, std::vector<std::string>>;

    class PhaseSpaceFileReader
    {
        public:
            PhaseSpaceFileReader(const std::string & phspFormat, const std::string & fileName, const UserOptions & options, FormatType formatType = FormatType::BINARY, unsigned int bufferSize = DEFAULT_BUFFER_SIZE);
            virtual ~PhaseSpaceFileReader();

            Particle              getNextParticle();
            virtual bool          hasMoreParticles();

            const std::string     getPHSPFormat() const;

            virtual std::uint64_t getNumberOfParticles() const = 0;
            virtual std::uint64_t getNumberOfOriginalHistories() const = 0;

            virtual std::uint64_t getHistoriesRead() const;

            std::uint64_t         getFileSize() const;
            const std::string     getFileName() const;

            void                  setCommentMarkers(const std::vector<std::string> & commentMarkers);

            void                  close();
        
        protected:
            Particle              getNextParticle(bool countParticleInStatistics);

            virtual std::size_t   getParticleRecordStartOffset() const;
            virtual std::size_t   getParticleRecordLength() const; // must be implemented for binary formatted files

            virtual Particle      readBinaryParticle(ByteBuffer & buffer); // not pure virtual to allow for ASCII format
            virtual Particle      readASCIIParticle(const std::string & line); // not pure virtual to allow for binary format
            virtual Particle      readParticleManually();
            virtual std::size_t   getMaximumASCIILineLength() const; // must be implemented for ASCII formatted files


            const ByteBuffer      getHeaderData();
            void                  setByteOrder(ByteOrder byteOrder);

            const UserOptions&    getUserOptions() const;

        private:
            void                  readNextBlock();
            void                  bufferNextASCIILine();

            const std::string phspFormat_;
            const std::string fileName_;
            const UserOptions userOptions_;
            const FormatType formatType_;
            const int BUFFER_SIZE;
            std::ifstream file_;

            std::list<std::string> asciiLineBuffer_;
            std::vector<std::string> asciiCommentMarkers_;

            const std::uint64_t bytesInFile_;
            std::uint64_t bytesRead_;
            std::uint64_t particlesRead_;
            std::uint64_t numberOfParticlesToRead_;
            std::size_t particleRecordLength_;
            ByteBuffer buffer_;
    };

    // Special runtime exception class to catch EOF for ASCII formatted files
    class EndOfFileException : public std::runtime_error {
        public:
            explicit EndOfFileException(const std::string & message) : std::runtime_error(message) {}
    };

    // Inline implementations for the PhaseSpaceFileReader class

    inline const std::string PhaseSpaceFileReader::getPHSPFormat() const { return phspFormat_; }

    inline Particle PhaseSpaceFileReader::getNextParticle() {
        return getNextParticle(true); // count particle in statistics by default
    }
    inline std::uint64_t PhaseSpaceFileReader::getFileSize() const { return bytesInFile_; }
    inline const std::string PhaseSpaceFileReader::getFileName() const { return fileName_; }
    inline std::size_t PhaseSpaceFileReader::getParticleRecordStartOffset() const { return 0; }
    inline void PhaseSpaceFileReader::setByteOrder(ByteOrder byteOrder) { buffer_.setByteOrder(byteOrder); }
    inline const UserOptions& PhaseSpaceFileReader::getUserOptions() const { return userOptions_; }

    inline std::uint64_t PhaseSpaceFileReader::getHistoriesRead() const {
        // default implementation estimates histories read based on particles read
        static const std::uint64_t TOTAL_PARTICLES = getNumberOfParticles();
        static const std::uint64_t TOTAL_HISTORIES = getNumberOfOriginalHistories();
        if (particlesRead_ == TOTAL_PARTICLES) { return TOTAL_HISTORIES; }
        static const float HISTORIES_PER_PARTICLE = static_cast<float>(TOTAL_HISTORIES) / static_cast<float>(TOTAL_PARTICLES);
        return static_cast<std::uint64_t>(HISTORIES_PER_PARTICLE * particlesRead_);
    }

    inline void PhaseSpaceFileReader::setCommentMarkers(const std::vector<std::string> & commentMarkers) {
        asciiCommentMarkers_ = commentMarkers;
    }

    inline std::size_t PhaseSpaceFileReader::getParticleRecordLength() const {
        throw std::runtime_error("getParticleRecordLength() must be implemented for binary formatted file writers.");
    }

    inline size_t PhaseSpaceFileReader::getMaximumASCIILineLength() const {
        throw std::runtime_error("getMaximumASCILineLength() must be implemented for ASCII formatted file readers.");
    }

    inline Particle PhaseSpaceFileReader::readBinaryParticle(ByteBuffer & buffer) {
        (void)buffer;
        throw std::runtime_error("readBinaryParticle() must be implemented for binary formatted file readers.");
    }

    inline Particle PhaseSpaceFileReader::readASCIIParticle(const std::string & line) {
        (void)line;
        throw std::runtime_error("readASCIIParticle() must be implemented for ASCII formatted file readers.");
    }

    inline Particle PhaseSpaceFileReader::readParticleManually() {
        throw std::runtime_error("readParticleManually() must be implemented for manual particle reading.");
    }
}