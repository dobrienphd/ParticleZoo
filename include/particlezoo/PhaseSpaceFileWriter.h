#pragma once

#include "particlezoo/Particle.h"

#include <fstream>
#include <string>
#include <cstdint>
#include <memory>

#include "particlezoo/ByteBuffer.h"

namespace ParticleZoo
{
    using UserOptions = std::unordered_map<std::string, std::vector<std::string>>;

    class PhaseSpaceFileWriter
    {
        public:
            PhaseSpaceFileWriter(const std::string & fileName, const UserOptions & userOptions, FormatType formatType = FormatType::BINARY, unsigned int bufferSize = DEFAULT_BUFFER_SIZE);
            virtual ~PhaseSpaceFileWriter();

            virtual void                writeParticle(Particle particle);
            virtual std::uint64_t       getMaximumSupportedParticles() const = 0;
            virtual std::uint64_t       getHistoriesWritten() const;

            void                        addAdditionalHistories(std::uint64_t additionalHistories);

            const std::string           getFileName() const;
            ByteOrder                   getByteOrder() const;

            void                        close();

        protected:
            virtual std::size_t         getParticleRecordStartOffset() const;
            virtual std::size_t         getParticleRecordLength() const; // must be implemented for binary formatted files
            virtual size_t              getMaximumASCIILineLength() const; // must be implemented for ASCII formatted files

            virtual void                writeHeaderData(ByteBuffer & buffer) = 0;
            virtual void                writeBinaryParticle(ByteBuffer & buffer, Particle & particle); // not const to allow internal counters to be updated in derived classes
            virtual const std::string   writeASCIIParticle(Particle & particle); // not const to allow internal counters to be updated in derived classes
            virtual void                writeParticleManually(Particle & particle);

            virtual bool                accountForAdditionalHistories(std::uint64_t additionalHistories);

            void                        setByteOrder(ByteOrder byteOrder);

            const UserOptions&          getUserOptions() const;

        private:
            void                        writeNextBlock();
            void                        writeHeaderToFile();
            ByteBuffer *                getParticleBuffer();

            std::string fileName_;
            const UserOptions userOptions_;
            const unsigned int BUFFER_SIZE;
            FormatType formatType_;
            std::ofstream file_;
            std::uint64_t historiesWritten_;
            std::size_t particleRecordLength_;

            ByteBuffer buffer_;
            std::unique_ptr<ByteBuffer> particleBuffer_;
            int writeParticleDepth_; // to track nested calls to writeParticle
    };

    // Inline implementations for the PhaseSpaceFileWriter class

    inline std::uint64_t PhaseSpaceFileWriter::getHistoriesWritten() const { return historiesWritten_; }

    inline const std::string PhaseSpaceFileWriter::getFileName() const { return fileName_; }
    inline ByteOrder PhaseSpaceFileWriter::getByteOrder() const { return buffer_.getByteOrder(); }
    
    inline std::size_t PhaseSpaceFileWriter::getParticleRecordStartOffset() const { return 0; }

    inline void PhaseSpaceFileWriter::setByteOrder(ByteOrder byteOrder) {
        buffer_.setByteOrder(byteOrder);
        getParticleBuffer()->setByteOrder(byteOrder);
    }

    inline const UserOptions& PhaseSpaceFileWriter::getUserOptions() const { return userOptions_; }

    inline ByteBuffer * PhaseSpaceFileWriter::getParticleBuffer() {
        if (!particleBuffer_) {
            std::size_t particleRecordLength = getParticleRecordLength();
            particleBuffer_ = std::make_unique<ByteBuffer>(particleRecordLength, buffer_.getByteOrder());
        }
        return particleBuffer_.get();
    }

    inline std::size_t PhaseSpaceFileWriter::getParticleRecordLength() const {
        throw std::runtime_error("getParticleRecordLength() must be implemented for binary formatted file writers.");
    }

    inline size_t PhaseSpaceFileWriter::getMaximumASCIILineLength() const {
        throw std::runtime_error("getMaximumASCILineLength() must be implemented for ASCII formatted file writers.");
    }

    inline void PhaseSpaceFileWriter::writeBinaryParticle(ByteBuffer & buffer, Particle & particle) {
        (void)buffer;
        (void)particle;
        throw std::runtime_error("writeBinaryParticle() must be implemented for binary formatted file writers.");
    }

    inline const std::string PhaseSpaceFileWriter::writeASCIIParticle(Particle & particle) {
        (void)particle;
        throw std::runtime_error("writeASCIIParticle() must be implemented for ASCII formatted file writers.");
    }

    inline void PhaseSpaceFileWriter::writeParticleManually(Particle & particle) {
        (void)particle;
        throw std::runtime_error("writeParticleManually() must be implemented for manual particle writing.");
    }

    inline bool PhaseSpaceFileWriter::accountForAdditionalHistories(std::uint64_t additionalHistories) {
        (void)additionalHistories; // unused in this implementation
        return true;
    }

    inline void PhaseSpaceFileWriter::addAdditionalHistories(std::uint64_t additionalHistories) {
        if (accountForAdditionalHistories(additionalHistories)) {
            historiesWritten_ += additionalHistories;
        }
    }

} // namespace ParticleZoo