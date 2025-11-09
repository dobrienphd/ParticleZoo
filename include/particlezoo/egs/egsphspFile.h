#pragma once

#include "particlezoo/PhaseSpaceFileReader.h"
#include "particlezoo/PhaseSpaceFileWriter.h"

#include <string>
#include <limits>

#include "particlezoo/Particle.h"
#include "particlezoo/egs/EGSLATCH.h"

namespace ParticleZoo
{
    namespace EGSphspFile {

        extern CLICommand EGSIgnoreHeaderCountCommand;  ///< Command to ignore particle count in header and calculate it from the file size instead
        extern CLICommand EGSParticleZValueCommand;     ///< Command to specify Z coordinate for all particles when reading from an egsphsp file (needed since Z is not stored in the file)
        extern CLICommand EGSModeCommand;               ///< Command to specify EGS file mode (MODE0/MODE2)

        constexpr std::size_t MINIMUM_HEADER_DATA_LENGTH = 25;  ///< Minimum header size in bytes
        
        /**
         * @brief Enumeration of supported EGS phase space file modes.
         * 
         * The values correspond to the record length in bytes for each mode.
         */
        enum class EGSMODE { 
            MODE0 = 28,     ///< Standard mode with 28-byte records
            MODE2 = 32      ///< Extended mode with 32-byte records (includes ZLAST)
        };

        /**
         * @brief Reader class for EGS phase space files.
         * 
         * Provides functionality to read EGS phase space files created by EGSnrc Monte
         * Carlo simulations (or it's variants BEAMnrc, DOSXYZnrc, etc.). Supports both
         * MODE0 and MODE2 formats.
         */
        class Reader : public PhaseSpaceFileReader
        {
            public:
                /**
                 * @brief Construct a new EGS phase space file reader.
                 * 
                 * @param fileName Path to the EGS phase space file to read
                 * @param options User options including user-specific configuration
                 * @throws std::runtime_error if file format is invalid or unsupported mode
                 */
                Reader(const std::string & fileName, const UserOptions & options = UserOptions{});

                /**
                 * @brief Get the total number of particles in the phase space file.
                 * 
                 * @return std::uint64_t Number of particles (from header or calculated from file size)
                 */
                std::uint64_t getNumberOfParticles() const override { return static_cast<std::uint64_t>(numberOfParticles_); };
                
                /**
                 * @brief Get the number of original Monte Carlo histories.
                 * 
                 * @return std::uint64_t Number of original histories that generated this phase space
                 */
                std::uint64_t getNumberOfOriginalHistories() const override { return static_cast<std::uint64_t>(numberOfOriginalHistories_); };

                /**
                 * @brief Get the EGS file mode (MODE0 or MODE2).
                 * 
                 * @return EGSMODE The detected file mode
                 */
                EGSMODE getMode() const { return mode_; };

                /**
                 * @brief Get the LATCH interpretation option.
                 * 
                 * @return EGSLATCHOPTION The LATCH option used for reading particles
                 */
                EGSLATCHOPTION getLATCHOption() const { return latchOption_; };

                /**
                 * @brief Get the number of photons in the phase space.
                 * 
                 * @return unsigned int Number of photon particles
                 */
                unsigned int getNumberOfPhotons() const { return numberOfPhotons_; };
                
                /**
                 * @brief Get the maximum kinetic energy of particles in the file.
                 * 
                 * @return float Maximum kinetic energy
                 */
                float getMaxKineticEnergy() const { return maxKineticEnergy_; };
                
                /**
                 * @brief Get the minimum electron energy threshold used in the simulation.
                 * 
                 * @return float Minimum electron energy
                 */
                float getMinElectronEnergy() const { return minElectronEnergy_; };

                /**
                 * @brief Get the list of EGS-specific command line interface commands.
                 * 
                 * @return std::vector<CLICommand> Vector of EGS-specific CLI commands
                 */
                static std::vector<CLICommand> getFormatSpecificCLICommands();

            protected:
                /**
                 * @brief Get the length of each particle record in bytes.
                 * 
                 * @return std::size_t Record length (28 for MODE0, 32 for MODE2)
                 */
                std::size_t getParticleRecordLength() const override { return std::max(static_cast<std::size_t>(mode_), MINIMUM_HEADER_DATA_LENGTH); };
                
                /**
                 * @brief Get the byte offset where particle records start.
                 * 
                 * @return std::size_t Offset from the beginning of the file (same as record length for EGS files)
                 */
                std::size_t getParticleRecordStartOffset() const override { return getParticleRecordLength(); };

                /**
                 * @brief Read a single particle in EGS binary format.
                 * 
                 * Parses EGS binary format including LATCH bits and ZLAST if present.
                 * 
                 * @param buffer The byte buffer containing particle data
                 * @return Particle The parsed particle object with EGS-specific properties
                 * @throws std::runtime_error if particle data is invalid
                 */
                Particle readBinaryParticle(ByteBuffer & buffer) override;

            private:
                EGSMODE mode_;                         ///< File mode (MODE0 or MODE2)
                EGSLATCHOPTION latchOption_;           ///< LATCH interpretation option
                unsigned int numberOfParticles_{};     ///< Total number of particles in file
                unsigned int numberOfPhotons_{};       ///< Number of photon particles
                float maxKineticEnergy_{};             ///< Maximum kinetic energy in file
                float minElectronEnergy_{};            ///< Minimum electron energy threshold
                float numberOfOriginalHistories_{};    ///< Number of original Monte Carlo histories
                float particleZValue_{};               ///< Z coordinate assigned to all particles, since EGS format does not store Z

                /**
                 * @brief Read and parse the EGS file header.
                 * 
                 * Reads the header to determine file mode, particle counts, energy ranges,
                 * and the number of original histories.
                 * 
                 * Can optionally override the header particle count by calculating it from the file size instead.
                 * 
                 * @param ignoreHeaderParticleCount If true, calculate particle count from file size
                 * @throws std::runtime_error if header format is invalid
                 */
                void readHeader(bool ignoreHeaderParticleCount);
        };
    

        /**
         * @brief Writer class for EGS phase space files.
         * 
         * Provides functionality to write EGS phase space files compatible with EGSnrc
         * Monte Carlo simulations (or it's variants BEAMnrc, DOSXYZnrc, etc.). Supports both
         * MODE0 and MODE2 formats.
         */
        class Writer : public PhaseSpaceFileWriter
        {
            public:
                /**
                 * @brief Construct a new EGS phase space file writer.
                 * 
                 * @param fileName Path where the EGS phase space file will be written
                 * @param options User options including EGS-specific configuration (e.g., mode selection)
                 * @throws std::runtime_error if specified mode is unsupported
                 */
                Writer(const std::string & fileName, const UserOptions & options = UserOptions{});

                /**
                 * @brief Get the maximum number of particles this format can support.
                 * 
                 * @return std::uint64_t Maximum particle count (limited by 32-bit unsigned integer)
                 */
                std::uint64_t getMaximumSupportedParticles() const override { return std::numeric_limits<std::uint32_t>::max(); };

                /**
                 * @brief Get the EGS file mode (MODE0 or MODE2).
                 * 
                 * @return EGSMODE The file mode
                 */
                EGSMODE getMode() const { return mode_; };

                /**
                 * @brief Get the LATCH interpretation option.
                 * 
                 * @return EGSLATCHOPTION The LATCH option used for writing particles
                 */
                EGSLATCHOPTION getLATCHOption() const { return latchOption_; };

                /**
                 * @brief Manually set the number of original Monte Carlo histories.
                 * 
                 * Allows explicit specification of the history count instead of automatic tracking.
                 * 
                 * @param numberOfOriginalHistories The number of original histories to record in the header
                 */
                void setNumberOfOriginalHistories(unsigned int numberOfOriginalHistories) {
                    numberOfOriginalHistories_ = (float)numberOfOriginalHistories;
                    historyCountManualSet_ = true;
                }

                /**
                 * @brief Get the list of EGS-specific command line interface commands.
                 * 
                 * @return std::vector<CLICommand> Vector of EGS-specific CLI commands for writers
                 */
                static std::vector<CLICommand> getFormatSpecificCLICommands();

            protected:
                /**
                 * @brief Get the length of each particle record in bytes.
                 * 
                 * @return std::size_t Record length (28 for MODE0, 32 for MODE2)
                 */
                std::size_t getParticleRecordLength() const override { return static_cast<std::size_t>(mode_); };
                
                /**
                 * @brief Get the byte offset where particle records start.
                 * 
                 * @return std::size_t Offset from the beginning of the file (same as record length for EGS files)
                 */
                std::size_t getParticleRecordStartOffset() const override { return getParticleRecordLength(); };
                
                /**
                 * @brief Write the EGS file header with current statistics.
                 * 
                 * Writes the header containing mode string, particle counts, energy ranges,
                 * and history information based on accumulated statistics (or specified histories).
                 * 
                 * @param buffer The byte buffer to write header data into
                 * @throws std::runtime_error if mode is unsupported
                 */
                virtual void writeHeaderData(ByteBuffer & buffer) override;
                
                /**
                 * @brief Write a single particle in EGS binary format.
                 * 
                 * Converts particle data to EGS format including LATCH encoding
                 * and ZLAST if in MODE2.
                 * 
                 * @param buffer The byte buffer to write particle data into
                 * @param particle The particle to write
                 * @throws std::runtime_error if particle type is unsupported or required properties are missing
                 */
                virtual void writeBinaryParticle(ByteBuffer & buffer, Particle & particle) override;

            private:
                EGSMODE mode_;                                                      ///< File mode (MODE0 or MODE2)
                EGSLATCHOPTION latchOption_;                                        ///< LATCH interpretation option
                unsigned int numberOfParticles_{};                                  ///< Total number of particles written
                unsigned int numberOfPhotons_{};                                    ///< Number of photon particles written
                float maxKineticEnergy_{};                                          ///< Maximum kinetic energy encountered
                float minElectronEnergy_{std::numeric_limits<float>::infinity()};   ///< Minimum electron energy encountered
                float numberOfOriginalHistories_{};                                 ///< Number of original Monte Carlo histories
                bool historyCountManualSet_{false};                                 ///< Flag indicating if history count was manually set
        };

    } // namespace EGSphspFile

} // namespace ParticleZoo