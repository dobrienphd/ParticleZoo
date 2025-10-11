#pragma once

#include <fstream>
#include <string>
#include <cstdint>
#include <memory>

#include "particlezoo/utilities/argParse.h"
#include "particlezoo/Particle.h"
#include "particlezoo/ByteBuffer.h"

namespace ParticleZoo
{
    extern CLICommand ConstantXCommand;
    extern CLICommand ConstantYCommand;
    extern CLICommand ConstantZCommand;
    extern CLICommand ConstantPxCommand;
    extern CLICommand ConstantPyCommand;
    extern CLICommand ConstantPzCommand;
    extern CLICommand ConstantWeightCommand;

    class PhaseSpaceFileWriter
    {
        public:
            PhaseSpaceFileWriter(const std::string & phspFormat, const std::string & fileName, const UserOptions & userOptions, FormatType formatType = FormatType::BINARY, const FixedValues fixedValues = FixedValues(), unsigned int bufferSize = DEFAULT_BUFFER_SIZE);
            virtual ~PhaseSpaceFileWriter();

            const std::string           getPHSPFormat() const;

            virtual void                writeParticle(Particle particle);
            virtual std::uint64_t       getMaximumSupportedParticles() const = 0;
            virtual std::uint64_t       getHistoriesWritten() const;
                    std::uint64_t       getParticlesWritten() const;

            void                        addAdditionalHistories(std::uint64_t additionalHistories);

            const std::string           getFileName() const;
            ByteOrder                   getByteOrder() const;

            bool                        isXConstant() const;
            bool                        isYConstant() const;
            bool                        isZConstant() const;
            bool                        isPxConstant() const;
            bool                        isPyConstant() const;
            bool                        isPzConstant() const;
            bool                        isWeightConstant() const;

            float                       getConstantX() const;
            float                       getConstantY() const;
            float                       getConstantZ() const;
            float                       getConstantPx() const;
            float                       getConstantPy() const;
            float                       getConstantPz() const;
            float                       getConstantWeight() const;

            const FixedValues           getFixedValues() const;

            static std::vector<CLICommand> getCLICommands();

            void                        close();

        protected:

            virtual bool                canHaveConstantX() const;
            virtual bool                canHaveConstantY() const;
            virtual bool                canHaveConstantZ() const;
            virtual bool                canHaveConstantPx() const;
            virtual bool                canHaveConstantPy() const;
            virtual bool                canHaveConstantPz() const;
            virtual bool                canHaveConstantWeight() const;

            void                        setConstantX(float X);
            void                        setConstantY(float Y);
            void                        setConstantZ(float Z);
            void                        setConstantPx(float Px);
            void                        setConstantPy(float Py);
            void                        setConstantPz(float Pz);
            void                        setConstantWeight(float weight);

            virtual void                fixedValuesHaveChanged(){};

            virtual std::size_t         getParticleRecordStartOffset() const;
            virtual std::size_t         getParticleRecordLength() const; // must be implemented for binary formatted files
            virtual size_t              getMaximumASCIILineLength() const; // must be implemented for ASCII formatted files

            virtual void                writeHeaderData(ByteBuffer & buffer) = 0;
            virtual void                writeBinaryParticle(ByteBuffer & buffer, Particle & particle); // not const to allow internal counters to be updated in derived classes
            virtual const std::string   writeASCIIParticle(Particle & particle); // not const to allow internal counters to be updated in derived classes
            virtual void                writeParticleManually(Particle & particle);

            // accountForAdditionalHistories() handles accounting for simulation histories that produced no particles to write.
            // It is called by addAdditionalHistories().
            // Some file formats need special handling for empty histories (e.g., writing pseudo-particles
            // or updating header counters). Returns true if the base class should automatically
            // increment the history counter, false if the derived class handles it manually (e.g. by writing additional pseudoparticles).
            virtual bool                accountForAdditionalHistories(std::uint64_t additionalHistories);

            virtual bool                canWritePseudoParticlesExplicitly() const;

            void                        setByteOrder(ByteOrder byteOrder);

            const UserOptions&          getUserOptions() const;

        private:
            void                        writeNextBlock();
            void                        writeHeaderToFile();
            ByteBuffer *                getParticleBuffer();

            const std::string phspFormat_;
            const std::string fileName_;
            const UserOptions userOptions_;
            const unsigned int BUFFER_SIZE;
            FormatType formatType_;
            std::ofstream file_;
            std::uint64_t historiesWritten_;
            std::uint64_t particlesWritten_;
            std::size_t particleRecordLength_;

            ByteBuffer buffer_;
            std::unique_ptr<ByteBuffer> particleBuffer_;
            int writeParticleDepth_; // to track nested calls to writeParticle

            FixedValues fixedValues_;
    };


    // Inline implementations for the PhaseSpaceFileWriter class


    inline const std::string PhaseSpaceFileWriter::getPHSPFormat() const { return phspFormat_; }
    inline std::uint64_t PhaseSpaceFileWriter::getHistoriesWritten() const { return historiesWritten_; }
    inline std::uint64_t PhaseSpaceFileWriter::getParticlesWritten() const { return particlesWritten_; }
    inline const std::string PhaseSpaceFileWriter::getFileName() const { return fileName_; }
    inline ByteOrder PhaseSpaceFileWriter::getByteOrder() const { return buffer_.getByteOrder(); }

    inline bool PhaseSpaceFileWriter::isXConstant() const { return fixedValues_.xIsConstant; }
    inline bool PhaseSpaceFileWriter::isYConstant() const { return fixedValues_.yIsConstant; }
    inline bool PhaseSpaceFileWriter::isZConstant() const { return fixedValues_.zIsConstant; }
    inline bool PhaseSpaceFileWriter::isPxConstant() const { return fixedValues_.pxIsConstant; }
    inline bool PhaseSpaceFileWriter::isPyConstant() const { return fixedValues_.pyIsConstant; }
    inline bool PhaseSpaceFileWriter::isPzConstant() const { return fixedValues_.pzIsConstant; }
    inline bool PhaseSpaceFileWriter::isWeightConstant() const { return fixedValues_.weightIsConstant; }

    inline float PhaseSpaceFileWriter::getConstantX() const { if (!fixedValues_.xIsConstant) throw std::runtime_error("X is not a constant"); return fixedValues_.constantX; }
    inline float PhaseSpaceFileWriter::getConstantY() const { if (!fixedValues_.yIsConstant) throw std::runtime_error("Y is not a constant"); return fixedValues_.constantY; }
    inline float PhaseSpaceFileWriter::getConstantZ() const { if (!fixedValues_.zIsConstant) throw std::runtime_error("Z is not a constant"); return fixedValues_.constantZ; }
    inline float PhaseSpaceFileWriter::getConstantPx() const { if (!fixedValues_.pxIsConstant) throw std::runtime_error("Px is not a constant"); return fixedValues_.constantPx; }
    inline float PhaseSpaceFileWriter::getConstantPy() const { if (!fixedValues_.pyIsConstant) throw std::runtime_error("Py is not a constant"); return fixedValues_.constantPy; }
    inline float PhaseSpaceFileWriter::getConstantPz() const { if (!fixedValues_.pzIsConstant) throw std::runtime_error("Pz is not a constant"); return fixedValues_.constantPz; }
    inline float PhaseSpaceFileWriter::getConstantWeight() const { if (!fixedValues_.weightIsConstant) throw std::runtime_error("Weight is not a constant"); return fixedValues_.constantWeight; }

    inline bool PhaseSpaceFileWriter::canHaveConstantX() const { return false; }
    inline bool PhaseSpaceFileWriter::canHaveConstantY() const { return false; }
    inline bool PhaseSpaceFileWriter::canHaveConstantZ() const { return false; }
    inline bool PhaseSpaceFileWriter::canHaveConstantPx() const { return false; }
    inline bool PhaseSpaceFileWriter::canHaveConstantPy() const { return false; }
    inline bool PhaseSpaceFileWriter::canHaveConstantPz() const { return false; }
    inline bool PhaseSpaceFileWriter::canHaveConstantWeight() const { return false; }

    inline void PhaseSpaceFileWriter::setConstantX(float X) { fixedValues_.xIsConstant = true; fixedValues_.constantX = X; fixedValuesHaveChanged(); }
    inline void PhaseSpaceFileWriter::setConstantY(float Y) { fixedValues_.yIsConstant = true; fixedValues_.constantY = Y; fixedValuesHaveChanged(); }
    inline void PhaseSpaceFileWriter::setConstantZ(float Z) { fixedValues_.zIsConstant = true; fixedValues_.constantZ = Z; fixedValuesHaveChanged(); }
    inline void PhaseSpaceFileWriter::setConstantPx(float Px) { fixedValues_.pxIsConstant = true; fixedValues_.constantPx = Px; fixedValuesHaveChanged(); }
    inline void PhaseSpaceFileWriter::setConstantPy(float Py) { fixedValues_.pyIsConstant = true; fixedValues_.constantPy = Py; fixedValuesHaveChanged(); }
    inline void PhaseSpaceFileWriter::setConstantPz(float Pz) { fixedValues_.pzIsConstant = true; fixedValues_.constantPz = Pz; fixedValuesHaveChanged(); }
    inline void PhaseSpaceFileWriter::setConstantWeight(float weight) { fixedValues_.weightIsConstant = true; fixedValues_.constantWeight = weight; fixedValuesHaveChanged(); }

    inline const FixedValues PhaseSpaceFileWriter::getFixedValues() const { return fixedValues_; }

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

    inline bool PhaseSpaceFileWriter::canWritePseudoParticlesExplicitly() const {
        return false;
    }

} // namespace ParticleZoo