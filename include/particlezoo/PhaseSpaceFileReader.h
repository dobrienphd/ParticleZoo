#pragma once

#include <fstream>
#include <string>
#include <cstdint>
#include <list>
#include <vector>

#include "particlezoo/utilities/argParse.h"
#include "particlezoo/Particle.h"
#include "particlezoo/ByteBuffer.h"

namespace ParticleZoo
{

    class PhaseSpaceFileReader
    {
        public:
            PhaseSpaceFileReader(const std::string & phspFormat, const std::string & fileName, const UserOptions & userOptions, FormatType formatType = FormatType::BINARY, const FixedValues fixedValues = FixedValues(), unsigned int bufferSize = DEFAULT_BUFFER_SIZE);
            virtual ~PhaseSpaceFileReader();

            Particle              getNextParticle();
            virtual bool          hasMoreParticles();

            const std::string     getPHSPFormat() const;

            virtual std::uint64_t getNumberOfParticles() const = 0;
            virtual std::uint64_t getNumberOfOriginalHistories() const = 0;

            virtual std::uint64_t getHistoriesRead();
            virtual std::uint64_t getParticlesRead();

            std::uint64_t         getFileSize() const;
            const std::string     getFileName() const;

            void                  setCommentMarkers(const std::vector<std::string> & commentMarkers);

            bool                  isXConstant() const;
            bool                  isYConstant() const;
            bool                  isZConstant() const;
            bool                  isPxConstant() const;
            bool                  isPyConstant() const;
            bool                  isPzConstant() const;
            bool                  isWeightConstant() const;

            float                 getConstantX() const;
            float                 getConstantY() const;
            float                 getConstantZ() const;
            float                 getConstantPx() const;
            float                 getConstantPy() const;
            float                 getConstantPz() const;
            float                 getConstantWeight() const;

            const FixedValues     getFixedValues() const;

            static std::vector<CLICommand> getCLICommands();

            void                  close();

        protected:

            void                  setConstantX(float X);
            void                  setConstantY(float Y);
            void                  setConstantZ(float Z);
            void                  setConstantPx(float Px);
            void                  setConstantPy(float Py);
            void                  setConstantPz(float Pz);
            void                  setConstantWeight(float weight);

            Particle              getNextParticle(bool countParticleInStatistics);

            virtual std::size_t   getParticleRecordStartOffset() const;
            virtual std::size_t   getParticleRecordLength() const; // must be implemented for binary formatted files

            virtual Particle      readBinaryParticle(ByteBuffer & buffer); // not pure virtual to allow for ASCII format
            virtual Particle      readASCIIParticle(const std::string & line); // not pure virtual to allow for binary format
            virtual Particle      readParticleManually();
            virtual std::size_t   getMaximumASCIILineLength() const; // must be implemented for ASCII formatted files


            const ByteBuffer      getHeaderData();
            const ByteBuffer      getHeaderData(std::size_t headerSize);
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
            std::uint64_t historiesRead_;
            std::uint64_t numberOfParticlesToRead_;
            std::size_t particleRecordLength_;
            ByteBuffer buffer_;

            FixedValues fixedValues_;
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

    inline bool PhaseSpaceFileReader::isXConstant() const { return fixedValues_.xIsConstant; }
    inline bool PhaseSpaceFileReader::isYConstant() const { return fixedValues_.yIsConstant; }
    inline bool PhaseSpaceFileReader::isZConstant() const { return fixedValues_.zIsConstant; }
    inline bool PhaseSpaceFileReader::isPxConstant() const { return fixedValues_.pxIsConstant; }
    inline bool PhaseSpaceFileReader::isPyConstant() const { return fixedValues_.pyIsConstant; }
    inline bool PhaseSpaceFileReader::isPzConstant() const { return fixedValues_.pzIsConstant; }
    inline bool PhaseSpaceFileReader::isWeightConstant() const { return fixedValues_.weightIsConstant; }

    inline float PhaseSpaceFileReader::getConstantX() const { if (!fixedValues_.xIsConstant) throw std::runtime_error("X is not a constant"); return fixedValues_.constantX; }
    inline float PhaseSpaceFileReader::getConstantY() const { if (!fixedValues_.yIsConstant) throw std::runtime_error("Y is not a constant"); return fixedValues_.constantY; }
    inline float PhaseSpaceFileReader::getConstantZ() const { if (!fixedValues_.zIsConstant) throw std::runtime_error("Z is not a constant"); return fixedValues_.constantZ; }
    inline float PhaseSpaceFileReader::getConstantPx() const { if (!fixedValues_.pxIsConstant) throw std::runtime_error("Px is not a constant"); return fixedValues_.constantPx; }
    inline float PhaseSpaceFileReader::getConstantPy() const { if (!fixedValues_.pyIsConstant) throw std::runtime_error("Py is not a constant"); return fixedValues_.constantPy; }
    inline float PhaseSpaceFileReader::getConstantPz() const { if (!fixedValues_.pzIsConstant) throw std::runtime_error("Pz is not a constant"); return fixedValues_.constantPz; }
    inline float PhaseSpaceFileReader::getConstantWeight() const { if (!fixedValues_.weightIsConstant) throw std::runtime_error("Weight is not a constant"); return fixedValues_.constantWeight; }

    inline void PhaseSpaceFileReader::setConstantX(float X) { fixedValues_.xIsConstant = true; fixedValues_.constantX = X; }
    inline void PhaseSpaceFileReader::setConstantY(float Y) { fixedValues_.yIsConstant = true; fixedValues_.constantY = Y; }
    inline void PhaseSpaceFileReader::setConstantZ(float Z) { fixedValues_.zIsConstant = true; fixedValues_.constantZ = Z; }
    inline void PhaseSpaceFileReader::setConstantPx(float Px) { fixedValues_.pxIsConstant = true; fixedValues_.constantPx = Px; }
    inline void PhaseSpaceFileReader::setConstantPy(float Py) { fixedValues_.pyIsConstant = true; fixedValues_.constantPy = Py; }
    inline void PhaseSpaceFileReader::setConstantPz(float Pz) { fixedValues_.pzIsConstant = true; fixedValues_.constantPz = Pz; }
    inline void PhaseSpaceFileReader::setConstantWeight(float weight) { fixedValues_.weightIsConstant = true; fixedValues_.constantWeight = weight; }

    inline const FixedValues PhaseSpaceFileReader::getFixedValues() const { return fixedValues_; }

    inline std::uint64_t PhaseSpaceFileReader::getHistoriesRead() {
        if (!hasMoreParticles()) historiesRead_ = getNumberOfOriginalHistories();
        return historiesRead_;
    }

    inline std::uint64_t PhaseSpaceFileReader::getParticlesRead() { return particlesRead_; }

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
} // namespace ParticleZoo