#pragma once

#include "particlezoo/PhaseSpaceFileReader.h"
#include "particlezoo/PhaseSpaceFileWriter.h"
#include "particlezoo/IAEA/IAEAHeader.h"

namespace ParticleZoo::IAEAphspFile
{
    /// @brief Standard file extension for IAEA phase space data files
    constexpr std::string_view IAEAphspFileExtension = ".IAEAphsp";

    // Command-line interface commands for IAEA format configuration
    extern CLICommand IAEAHeaderTemplateCommand;      ///< Template header file specification
    extern CLICommand IAEAIndexCommand;               ///< IAEA index string configuration
    extern CLICommand IAEATitleCommand;               ///< Phase space title configuration
    extern CLICommand IAEAFileTypeCommand;            ///< File type specification
    extern CLICommand IAEAAddIncHistNumberCommand;    ///< Include incremental history numbers
    extern CLICommand IAEAAddEGSLATCHCommand;         ///< Include EGS LATCH values
    extern CLICommand IAEAAddPENELOPEILB5Command;     ///< Include PENELOPE ILB5 values
    extern CLICommand IAEAAddPENELOPEILB4Command;     ///< Include PENELOPE ILB4 values
    extern CLICommand IAEAAddPENELOPEILB3Command;     ///< Include PENELOPE ILB3 values
    extern CLICommand IAEAAddPENELOPEILB2Command;     ///< Include PENELOPE ILB2 values
    extern CLICommand IAEAAddPENELOPEILB1Command;     ///< Include PENELOPE ILB1 values
    extern CLICommand IAEAAddXLASTCommand;            ///< Include XLAST position values
    extern CLICommand IAEAAddYLASTCommand;            ///< Include YLAST position values
    extern CLICommand IAEAAddZLASTCommand;            ///< Include ZLAST position values
    extern CLICommand IAEAIgnoreChecksumCommand;      ///< Ignore checksum validation errors

    /**
     * @brief Reader for IAEA format phase space files
     * 
     * Provides functionality to read phase space data from IAEA format files,
     * handling both header parsing and binary particle data extraction.
     * Supports all standard IAEA features including extra float/long data types.
     */
    class Reader : public PhaseSpaceFileReader
    {
        public:
            /**
             * @brief Construct reader for IAEA phase space file
             * 
             * @param filename Path to the IAEA phase space data file (.IAEAphsp)
             * @param options User-specified options for reading behavior
             * @throws std::runtime_error if file cannot be opened or header is invalid
             */
            Reader(const std::string &filename, const UserOptions & options = UserOptions{});

            /**
             * @brief Get the total number of particles in the phase space
             * @return Total particle count across all types
             */
            std::uint64_t getNumberOfParticles() const override;

            /**
             * @brief Get the number of original simulation histories
             * @return Count of primary histories used in the simulation
             */
            std::uint64_t getNumberOfOriginalHistories() const override;

            /**
             * @brief Get the number of particles of a specific type
             * @param particleType Type of particle to count
             * @return Number of particles of the specified type
             */
            std::uint64_t getNumberOfParticles(ParticleType particleType) const;

            /**
             * @brief Get access to the IAEA header information
             * @return Reference to the header containing file metadata
             */
            const IAEAHeader & getHeader() const;

            /**
             * @brief Get format-specific command-line options
             * @return Vector of CLI commands supported by IAEA reader
             */
            static std::vector<CLICommand> getFormatSpecificCLICommands();

        protected:
            /**
             * @brief Get the byte offset where particle records start
             * @return Starting offset for particle data (0 for IAEA format)
             */
            std::size_t   getParticleRecordStartOffset() const override;

            /**
             * @brief Get the length of each particle record in bytes
             * @return Size of each particle record as defined in header
             */
            std::size_t   getParticleRecordLength() const override;

            /**
             * @brief Read and decode a single particle from binary data
             * @param buffer Binary buffer containing particle data
             * @return Decoded Particle object with all properties
             * @throws std::runtime_error if particle data is corrupted or invalid
             */
            Particle      readBinaryParticle(ByteBuffer & buffer) override;

        private:
            const IAEAHeader header_;  ///< Header information for the phase space file

    };

    /**
     * @brief Writer for IAEA format phase space files
     * 
     * Provides functionality to write phase space data to IAEA format files,
     * handling header generation and binary particle data encoding.
     * Supports all standard IAEA features and optional data types.
     */
    class Writer : public PhaseSpaceFileWriter
    {
        public:
            /**
             * @brief Construct writer for new IAEA phase space file
             * 
             * @param filename Path for the new IAEA phase space data file (.IAEAphsp)
             * @param userOptions User-specified options for writing behavior
             * @param fixedValues Constant values to optimize storage
             * @throws std::runtime_error if file cannot be created
             */
            Writer(const std::string &filename, const UserOptions & userOptions = UserOptions{}, const FixedValues & fixedValues = FixedValues{});

            /**
             * @brief Construct writer using an existing header as template
             * 
             * @param filename Path for the new IAEA phase space data file (.IAEAphsp)
             * @param templateHeader Existing header to copy configuration from
             */
            Writer(const std::string & filename, const IAEAHeader & templateHeader);

            /**
             * @brief Set the number of original simulation histories
             * 
             * @param numberOfHistories Number of original histories to record in header
             */
            void setNumberOfOriginalHistories(std::uint64_t numberOfHistories);

            /**
             * @brief Get the maximum number of particles this format can store
             * @return Maximum particle count (effectively unlimited for IAEA)
             */
            std::uint64_t getMaximumSupportedParticles() const override;

            /**
             * @brief Get access to the IAEA header for configuration
             * @return Reference to the header
             */
            IAEAHeader & getHeader();

            /**
             * @brief Get format-specific command-line options
             * @return Vector of CLI commands supported by IAEA writer
             */
            static std::vector<CLICommand> getFormatSpecificCLICommands();

        protected:
            /**
             * @brief Get the length of each particle record in bytes
             * @return Size of each particle record as configured in header
             */
            std::size_t getParticleRecordLength() const override;

            /**
             * @brief Write header data to the output buffer (not used for IAEA)
             * @param buffer Binary buffer for header information (not used for IAEA)
             */
            void writeHeaderData(ByteBuffer & buffer) override;

            /**
             * @brief Encode and write a single particle to binary data
             * @param buffer Binary buffer to write particle data to
             * @param particle Particle object to encode and store
             * @throws std::runtime_error if particle type is unsupported
             */
            void writeBinaryParticle(ByteBuffer & buffer, Particle & particle) override;

            /**
             * @brief Check if constant X coordinates are supported
             * @return true (IAEA format supports constant X values)
             */
            bool canHaveConstantX() const override {return true;}

            /**
             * @brief Check if constant Y coordinates are supported
             * @return true (IAEA format supports constant Y values)
             */
            bool canHaveConstantY() const override {return true;}

            /**
             * @brief Check if constant Z coordinates are supported
             * @return true (IAEA format supports constant Z values)
             */
            bool canHaveConstantZ() const override {return true;}

            /**
             * @brief Check if constant X momentum components are supported
             * @return true (IAEA format supports constant U direction cosines)
             */
            bool canHaveConstantPx() const override {return true;}

            /**
             * @brief Check if constant Y momentum components are supported
             * @return true (IAEA format supports constant V direction cosines)
             */
            bool canHaveConstantPy() const override {return true;}

            /**
             * @brief Check if constant Z momentum components are supported
             * @return true (IAEA format supports constant W direction cosines)
             */
            bool canHaveConstantPz() const override {return true;}

            /**
             * @brief Check if constant particle weights are supported
             * @return true (IAEA format supports constant weight values)
             */
            bool canHaveConstantWeight() const override {return true;}

            /**
             * @brief Handle changes to fixed/constant values
             * 
             * Updates the IAEA header when constant values are modified,
             * ensuring the header reflects the current optimization settings.
             */
            void fixedValuesHaveChanged() override;

        private:
            IAEAHeader header_;                        ///< Header configuration
            bool useCustomHistoryCount_{false};        ///< Flag for custom history count override
            std::uint64_t custumNumberOfHistories_{};  ///< Custom history count value

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