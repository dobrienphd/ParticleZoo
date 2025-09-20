#pragma once

#include "particlezoo/PhaseSpaceFileReader.h"
#include "particlezoo/PhaseSpaceFileWriter.h"
#include "particlezoo/IAEA/IAEAHeader.h"

namespace ParticleZoo::IAEAphspFile
{
    constexpr std::string_view IAEAphspFileExtension = ".IAEAphsp";

    extern CLICommand IAEAHeaderTemplateCommand;
    extern CLICommand IAEAIndexCommand;
    extern CLICommand IAEATitleCommand;
    extern CLICommand IAEAFileTypeCommand;
    extern CLICommand IAEAAddIncHistNumberCommand;
    extern CLICommand IAEAAddEGSLATCHCommand;
    extern CLICommand IAEAAddPENELOPEILB5Command;
    extern CLICommand IAEAAddPENELOPEILB4Command;
    extern CLICommand IAEAAddPENELOPEILB3Command;
    extern CLICommand IAEAAddPENELOPEILB2Command;
    extern CLICommand IAEAAddPENELOPEILB1Command;
    extern CLICommand IAEAAddXLASTCommand;
    extern CLICommand IAEAAddYLASTCommand;
    extern CLICommand IAEAAddZLASTCommand;

    class Reader : public PhaseSpaceFileReader
    {
        public:
            Reader(const std::string &filename, const UserOptions & options = UserOptions{});

            std::uint64_t getNumberOfParticles() const override;
            std::uint64_t getNumberOfOriginalHistories() const override;
            std::uint64_t getNumberOfParticles(ParticleType particleType) const;

            const IAEAHeader & getHeader() const;
            static std::vector<CLICommand> getFormatSpecificCLICommands();

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
            Writer(const std::string &filename, const UserOptions & userOptions = UserOptions{}, const FixedValues & fixedValues = FixedValues{});
            Writer(const std::string & filename, const IAEAHeader & templateHeader);

            void setNumberOfOriginalHistories(std::uint64_t numberOfHistories);

            std::uint64_t getMaximumSupportedParticles() const override;
            IAEAHeader & getHeader();
            static std::vector<CLICommand> getFormatSpecificCLICommands();

        protected:
            std::size_t getParticleRecordLength() const override;
            void writeHeaderData(ByteBuffer & buffer) override;
            void writeBinaryParticle(ByteBuffer & buffer, Particle & particle) override;

            bool canHaveConstantX() const override {return true;}
            bool canHaveConstantY() const override {return true;}
            bool canHaveConstantZ() const override {return true;}
            bool canHaveConstantPx() const override {return true;}
            bool canHaveConstantPy() const override {return true;}
            bool canHaveConstantPz() const override {return true;}
            bool canHaveConstantWeight() const override {return true;}

            void fixedValuesHaveChanged() override;

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
    inline void Writer::fixedValuesHaveChanged() {
        // Update the header
        if (isXConstant()) header_.setConstantX(getConstantX());
        if (isYConstant()) header_.setConstantY(getConstantY());
        if (isZConstant()) header_.setConstantZ(getConstantZ());
        if (isPxConstant()) header_.setConstantU(getConstantPx());
        if (isPyConstant()) header_.setConstantV(getConstantPy());
        if (isPzConstant()) header_.setConstantW(getConstantPz());
        if (isWeightConstant()) header_.setConstantWeight(getConstantWeight());
    }

} // namespace ParticleZoo::IAEAphspFile