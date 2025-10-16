
#include "particlezoo/PhaseSpaceFileReader.h"
#include "particlezoo/PhaseSpaceFileWriter.h"
#include "particlezoo/TOPAS/TOPASHeader.h"

#include <cmath>

namespace ParticleZoo::TOPASphspFile
{
    /// @brief Maximum length for ASCII particle record lines in TOPAS format
    constexpr std::size_t TOPASmaxASCIILineLength = 1024;

    /// @brief Command-line option for specifying TOPAS format type
    extern CLICommand TOPASFormatCommand;

    /**
     * @brief Reader for TOPAS phase space files
     * 
     * Provides functionality to read phase space data from TOPAS format files,
     * supporting ASCII, BINARY, and LIMITED formats. Handles TOPAS-specific
     * features including pseudo-particles for empty history tracking and
     * extensive metadata columns.
     */
    class Reader : public PhaseSpaceFileReader
    {
        public:
            /**
             * @brief Construct reader for TOPAS phase space file
             * 
             * Automatically detects the format (ASCII, BINARY, or LIMITED) by reading
             * the header file and configures the reader accordingly.
             * 
             * @param filename Path to the TOPAS phase space file (.phsp or .header)
             * @param options User-specified options for reading behavior
             * @throws std::runtime_error if file cannot be opened or format is invalid
             */
            Reader(const std::string &filename, const UserOptions &options = UserOptions{});

            /**
             * @brief Get the total number of particles in the phase space
             * @return Total particle count as recorded in the header
             */
            std::uint64_t  getNumberOfParticles() const override;

            /**
             * @brief Get the number of original simulation histories
             * @return Count of primary histories used in the simulation
             */
            std::uint64_t  getNumberOfOriginalHistories() const override;

            /**
             * @brief Get the TOPAS format type of this file
             * @return TOPASFormat enum indicating ASCII, BINARY, or LIMITED
             */
            TOPASFormat    getTOPASFormat() const;

            /**
             * @brief Get access to the TOPAS header information
             * @return Reference to the header containing file metadata and column definitions
             */
            const Header & getHeader() const;

            /**
             * @brief Enable or disable detailed reading of extended particle properties
             * 
             * When enabled, reads all additional columns beyond the standard 10 columns.
             * When disabled, only reads the core particle data for improved performance.
             * 
             * @param enable true to read all columns, false for core data only
             */
            void setDetailedReading(bool enable);

            /**
             * @brief Get format-specific command-line options
             * @return Vector of CLI commands supported by TOPAS reader (currently empty)
             */
            static std::vector<CLICommand> getFormatSpecificCLICommands();

        protected:
            /**
             * @brief Get the length of each particle record in bytes
             * @return Record length as defined by the TOPAS header
             */
            std::size_t    getParticleRecordLength() const override;

            /**
             * @brief Read and decode a single particle from binary data
             * 
             * Handles format-specific binary reading including pseudo-particle processing
             * for empty history tracking in BINARY format.
             * 
             * @param buffer Binary buffer containing particle data
             * @return Decoded Particle object with all properties
             * @throws std::runtime_error if format is unsupported or data is corrupted
             */
            Particle       readBinaryParticle(ByteBuffer & buffer) override;

            /**
             * @brief Parse a single ASCII line into a Particle object
             * 
             * Parses TOPAS ASCII format with configurable columns. Supports both
             * core particle data and extended properties based on header column definitions.
             * 
             * @param line ASCII line containing particle data
             * @return Parsed Particle object with properties set according to column types
             * @throws std::runtime_error if line cannot be parsed or contains invalid data
             */
            Particle       readASCIIParticle(const std::string & line) override;

            /**
             * @brief Get the maximum length of ASCII particle lines
             * @return Maximum line length (1024 characters for TOPAS format)
             */
            std::size_t    getMaximumASCIILineLength() const override;

        private:
            /**
             * @brief Private constructor for format-specific initialization
             * 
             * @param filename Path to the phase space file
             * @param options User options for reading
             * @param formatAndHeader Pair containing format type and parsed header
             */
            Reader(const std::string &filename, const UserOptions &options, std::pair<FormatType,Header> formatAndHeader);

            /**
             * @brief Read a standard BINARY format particle
             * 
             * Reads the full TOPAS BINARY format including all extended columns
             * and handles pseudo-particle detection for empty history tracking.
             * 
             * @param buffer Binary buffer containing particle data
             * @return Decoded Particle object
             */
            Particle       readBinaryStandardParticle(ByteBuffer & buffer);

            /**
             * @brief Read a LIMITED format particle
             * 
             * Reads the compact TOPAS LIMITED format with fixed 29-byte records
             * containing only essential particle information.
             * 
             * @param buffer Binary buffer containing particle data
             * @return Decoded Particle object with core properties
             */
            Particle       readBinaryLimitedParticle(ByteBuffer & buffer);

            /**
             * @brief Round float to 32-bit integer with "round half away from zero"
             * 
             * @param x Float value to round
             * @return Rounded 32-bit integer value
             * @throws std::overflow_error if value is outside int32 range
             */
            std::int32_t   roundToInt32(float x);

            const Header header_;                    ///< TOPAS header with metadata and column definitions
            const TOPASFormat formatType_;           ///< Format type (ASCII, BINARY, or LIMITED)
            const std::size_t particleRecordLength_; ///< Length of each particle record in bytes
            bool  readFullDetails_;                  ///< Flag for detailed vs. core-only reading
            std::int32_t emptyHistoriesCount_;       ///< Accumulator for empty history tracking
    };

    // Inline implementations for the Reader class
    inline std::uint64_t Reader::getNumberOfParticles() const { return header_.getNumberOfParticles(); }
    inline std::uint64_t Reader::getNumberOfOriginalHistories() const { return header_.getNumberOfOriginalHistories(); }
    inline TOPASFormat Reader::getTOPASFormat() const { return formatType_; }
    inline const Header & Reader::getHeader() const { return header_; }
    inline void Reader::setDetailedReading(bool enable) { readFullDetails_ = enable; }
    inline std::size_t Reader::getParticleRecordLength() const { return particleRecordLength_; }
    inline std::size_t Reader::getMaximumASCIILineLength() const { return TOPASmaxASCIILineLength; }

    inline std::int32_t Reader::roundToInt32(float x) {
        // upper and lower bounds for int32_t in float representation
        constexpr float maxBound = static_cast<float>(std::numeric_limits<std::int32_t>::max()) - 0.5f;
        constexpr float minBound = static_cast<float>(std::numeric_limits<std::int32_t>::min()) + 0.5f;

        // cold path if out of range
        if (x > maxBound || x < minBound) {
            throw std::overflow_error("The TOPAS binary file being read contains an empty-history pseudoparticle mid-file with a weight that is outside the range of signed 32 bit integers. This is only supported if the pseudoparticle is at the end of the file.");
        }

        // add +0.5 if x>=0, or –0.5 if x<0, then truncate
        std::int32_t result = static_cast<std::int32_t>(std::lroundf(x));
        return result;
    }

    inline Particle Reader::readBinaryParticle(ByteBuffer & buffer) {
        if (formatType_ == TOPASFormat::BINARY) {
            Particle particle = readBinaryStandardParticle(buffer);
            if (particle.getWeight() < 0 && particle.getType() == ParticleType::PseudoParticle) {
                // Special particle representing a sequence of empty histories
                emptyHistoriesCount_ += roundToInt32(-particle.getWeight());
                // need to read the next particle with the countParticleInStatistics flag set to false to avoid double counting
                return getNextParticle(false);
            } else if (emptyHistoriesCount_ > 0) {
                particle.setNewHistory(true); // make sure this flag is set, although it should be already
                std::int32_t incrementalHistoryNumber = particle.hasIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER) ? particle.getIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER) : 1;
                emptyHistoriesCount_ += incrementalHistoryNumber > 0 ? incrementalHistoryNumber : 1;
                particle.setIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER, emptyHistoriesCount_);
                emptyHistoriesCount_ = 0; // Reset after using
            }
            return particle;
        } else if (formatType_ == TOPASFormat::LIMITED) {
            return readBinaryLimitedParticle(buffer);
        } else {
            throw std::runtime_error("Unsupported format type for binary reading.");
        }
    };
    

    /**
     * @brief Writer for TOPAS phase space files
     * 
     * Provides functionality to write phase space data in TOPAS format files,
     * supporting ASCII, BINARY, and LIMITED formats. Handles TOPAS-specific
     * features including pseudo-particle generation for efficient empty history
     * tracking and configurable column layouts.
     */
    class Writer : public PhaseSpaceFileWriter
    {
        public:
            /**
             * @brief Construct writer for TOPAS phase space file
             * 
             * Creates a new TOPAS format writer with format determined by command-line options.
             * Defaults to BINARY format if no format is specified.
             * 
             * @param filename Path for the output TOPAS phase space file (.phsp)
             * @param options User-specified options including format selection
             * @throws std::runtime_error if file cannot be created or format is invalid
             */
            Writer(const std::string &filename, const UserOptions &options = UserOptions{});

            /**
             * @brief Get the maximum number of particles this format can store
             * @return Maximum particle count
             */
            std::uint64_t getMaximumSupportedParticles() const override;

            /**
             * @brief Get the TOPAS format type being written
             * @return TOPASFormat enum indicating ASCII, BINARY, or LIMITED
             */
            TOPASFormat   getTOPASFormat() const;

            /**
             * @brief Get access to the TOPAS header for configuration
             * @return Reference to the header for modification and column management
             */
            Header &      getHeader();

            /**
             * @brief Get format-specific command-line options
             * @return Vector of CLI commands supported by TOPAS writer
             */
            static std::vector<CLICommand> getFormatSpecificCLICommands();

        protected:
            /**
             * @brief Get the length of each particle record in bytes
             * @return Record length as defined by the header and format type
             */
            std::size_t       getParticleRecordLength() const override;

            /**
             * @brief Get the maximum length of ASCII particle lines
             * @return Maximum line length (1024 characters for TOPAS format)
             */
            std::size_t       getMaximumASCIILineLength() const override;

            /**
             * @brief Write header data to the output buffer
             * 
             * Updates header statistics and writes the complete header file.
             * The header is written to a separate .header file.
             * 
             * @param buffer Binary buffer (unused for TOPAS as header is separate)
             */
            void              writeHeaderData(ByteBuffer & buffer) override;

            /**
             * @brief Encode and write a single particle to binary data
             * 
             * Handles format-specific binary encoding including pseudo-particle
             * generation for empty history accounting in BINARY format.
             * 
             * @param buffer Binary buffer to write particle data to
             * @param particle Particle object to encode and store
             * @throws std::runtime_error if format is unsupported
             */
            void              writeBinaryParticle(ByteBuffer & buffer, Particle & particle) override;

            /**
             * @brief Convert a particle to ASCII representation
             * 
             * Formats a particle according to TOPAS ASCII specification with
             * configurable columns.
             * 
             * @param particle Particle object to convert to ASCII
             * @return ASCII string representation
             * @throws std::runtime_error if particle type is unsupported
             */
            const std::string writeASCIIParticle(Particle & particle) override;

            /**
             * @brief Override base class method to handle additional histories
             * 
             * For BINARY format, creates a pseudo-particle to represent multiple empty
             * histories in a single record. For other formats, delegates to base class.
             * 
             * @param additionalHistories Number of additional empty histories to account for
             * @return false for BINARY (handled internally), true for others (use base class)
             */
            bool              accountForAdditionalHistories(std::uint64_t additionalHistories) override;

            /**
             * @brief Check if pseudo-particles can be written explicitly
             * @return true for BINARY format, false for ASCII and LIMITED
             */
            bool              canWritePseudoParticlesExplicitly() const override;

        private:
            /**
             * @brief Private constructor for format-specific initialization
             * 
             * @param filename Path for the output file
             * @param options User options
             * @param formatType Specific TOPAS format to write
             */
            Writer(const std::string &filename, const UserOptions &options, TOPASFormat formatType);

            /**
             * @brief Write a particle in BINARY standard format
             * 
             * Encodes a particle with all configured columns for TOPAS BINARY format,
             * including extended properties and pseudo-particle handling.
             * 
             * @param buffer Binary buffer to write to
             * @param particle Particle to encode
             */
            void              writeBinaryStandardParticle(ByteBuffer & buffer, Particle & particle);

            /**
             * @brief Write a particle in LIMITED binary format
             * 
             * Encodes a particle in the compact TOPAS LIMITED format with
             * fixed 29-byte records containing only essential information.
             * 
             * @param buffer Binary buffer to write to
             * @param particle Particle to encode
             * @throws std::runtime_error if particle type is incompatible with LIMITED format
             */
            void              writeBinaryLimitedParticle(ByteBuffer & buffer, Particle & particle);
            
            const TOPASFormat formatType_; ///< Format type being written
            Header header_;                ///< Header configuration and statistics
    };

    // Inline implementations for the Writer class

    inline std::uint64_t Writer::getMaximumSupportedParticles() const { return std::numeric_limits<std::uint64_t>::max(); }
    inline TOPASFormat Writer::getTOPASFormat() const { return formatType_; }
    inline Header & Writer::getHeader() { return header_; }
    inline std::size_t Writer::getParticleRecordLength() const { return header_.getRecordLength(); }
    inline std::size_t Writer::getMaximumASCIILineLength() const { return TOPASmaxASCIILineLength; }

    inline void Writer::writeBinaryParticle(ByteBuffer & buffer, Particle & particle) {
        if (formatType_ == TOPASFormat::BINARY) {
            ParticleType particleType = particle.getType();
            if (particleType != ParticleType::Unsupported && particleType != ParticleType::PseudoParticle && particle.hasIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER)) {
                // If the particle has an incremental history number, we need to handle it
                std::int32_t incrementalHistoryNumber = particle.getIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER);
                if (incrementalHistoryNumber > 1) {
                    accountForAdditionalHistories(incrementalHistoryNumber - 1);
                    particle.setIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER, 1); // Reset to 1 after accounting for additional histories
                }
            }
            writeBinaryStandardParticle(buffer, particle);
            header_.countParticleStats(particle);
        } else if (formatType_ == TOPASFormat::LIMITED) {
            writeBinaryLimitedParticle(buffer, particle);
            header_.countParticleStats(particle);
        } else {
            throw std::runtime_error("Unsupported format type for binary writing.");
        }
    };

    inline bool Writer::accountForAdditionalHistories(std::uint64_t additionalHistories)
    {
        if (formatType_ == TOPASFormat::BINARY) {
            float pseudoWeight = -static_cast<float>(additionalHistories);
            Particle emptyHistoryPseudoParticle(
                ParticleType::PseudoParticle, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, true, pseudoWeight
            );
            emptyHistoryPseudoParticle.setIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER, static_cast<std::int32_t>(additionalHistories));
            writeParticle(emptyHistoryPseudoParticle);
            return false; // do not let the base class increment the histories counter automatically, it is done by instead by the call to writeParticle() here
        } else {
            // do not use pseudoparticles with the ASCII or LIMITED formats
            return true;
        }
    }

    inline bool Writer::canWritePseudoParticlesExplicitly() const {
        return formatType_ == TOPASFormat::BINARY; // only binary files can write pseudo particles explicitly
    }

} // namespace ParticleZoo::TOPASphsp