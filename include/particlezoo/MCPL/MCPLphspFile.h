#pragma once

#include "particlezoo/PhaseSpaceFileReader.h"
#include "particlezoo/PhaseSpaceFileWriter.h"

#include <string>
#include <limits>
#include <cstdint>
#include <vector>

#include "particlezoo/Particle.h"
#include "particlezoo/MCPL/MCPLConstants.h"

namespace ParticleZoo
{
    namespace MCPLphspFile {

        extern CLICommand MCPLPolarisationCommand;      ///< Command to store the particle polarisation vectors in the file
        extern CLICommand MCPLUserFlagsCommand;         ///< Command to store the particle user flags values in the file
        extern CLICommand MCPLUniversalWeightCommand;   ///< Command to store a single common weight in the header instead of per-particle weights
        extern CLICommand MCPLUniversalPDGCommand;      ///< Command to store a single common PDG code in the header instead of per-particle codes
        extern CLICommand MCPLCommentCommand;           ///< Command to add a comment to the file header

        /**
         * @brief Reader class for MCPL (Monte Carlo Particle Lists) files.
         *
         * Reads MCPL format version 3 files as well as the legacy format version 2
         * (which used octahedral direction packing). Single and double precision
         * files, both byte orders, polarisation vectors, user flags and the
         * universal weight and PDG code header options are all handled. Header
         * blobs are skipped. Gzip-compressed files (.mcpl.gz) are not supported
         * and must be decompressed externally first.
         *
         * MCPL files carry no history structure, so every particle is treated as
         * its own history. Double precision values are narrowed to the single
         * precision used internally.
         */
        class Reader : public PhaseSpaceFileReader
        {
            public:
                /**
                 * @brief Construct a new MCPL file reader.
                 *
                 * @param fileName Path to the MCPL file to read
                 * @param options User options including user-specific configuration
                 * @throws std::runtime_error if the file is not a valid MCPL format version 2 or 3 file
                 */
                Reader(const std::string & fileName, const UserOptions & options = UserOptions{});

                /**
                 * @brief Get the total number of particles in the file.
                 *
                 * For files that were not properly closed (a zero particle count in
                 * the header) the count is computed from the file size instead.
                 *
                 * @return std::uint64_t Number of particles
                 */
                std::uint64_t getNumberOfParticles() const override { return numberOfParticles_; }

                /**
                 * @brief Get the number of original histories.
                 *
                 * MCPL files carry no history structure, so every particle counts
                 * as one history.
                 *
                 * @return std::uint64_t Number of original histories
                 */
                std::uint64_t getNumberOfOriginalHistories() const override { return numberOfParticles_; }

                /**
                 * @brief Get the number of histories represented on the file.
                 *
                 * Equal to the number of particles, since every particle counts as
                 * one history.
                 *
                 * @return std::uint64_t Number of represented histories
                 */
                std::uint64_t getNumberOfRepresentedHistories() const override { return numberOfParticles_; }

                /**
                 * @brief The represented history count follows directly from the particle count.
                 */
                bool hasNativeRepresentedHistoryCount() const override { return true; }

                /**
                 * @brief Get the source program name recorded in the header.
                 *
                 * @return const std::string & The name of the program that wrote the file
                 */
                const std::string & getSourceProgramName() const { return srcProgName_; }

                /**
                 * @brief Get the comments recorded in the header.
                 *
                 * @return const std::vector<std::string> & The header comments
                 */
                const std::vector<std::string> & getComments() const { return comments_; }

                /**
                 * @brief Get the list of MCPL-specific command line interface commands.
                 *
                 * @return std::vector<CLICommand> Vector of MCPL-specific CLI commands
                 */
                static std::vector<CLICommand> getFormatSpecificCLICommands();

            protected:
                /**
                 * @brief Get the length of each particle record in bytes.
                 *
                 * @return std::size_t The record length stored in the header
                 */
                std::size_t getParticleRecordLength() const override { return particleSize_; }

                /**
                 * @brief Get the byte offset where particle records start.
                 *
                 * @return std::size_t The header size determined while parsing the header
                 */
                std::size_t getParticleRecordStartOffset() const override { return headerSize_; }

                /**
                 * @brief Read a single particle record.
                 *
                 * @param buffer The byte buffer containing the particle record
                 * @return Particle The parsed particle
                 * @throws std::runtime_error if the particle type is not supported
                 */
                Particle readBinaryParticle(ByteBuffer & buffer) override;

            private:
                unsigned int formatVersion_{MCPL_FORMAT_VERSION_CURRENT};   ///< Format version of the file (2 or 3)
                std::uint64_t numberOfParticles_{};     ///< Number of particles in the file
                std::size_t headerSize_{};              ///< Total size of the header in bytes
                std::size_t particleSize_{};            ///< Length of each particle record in bytes
                bool singlePrecision_{true};            ///< True if floating point fields are 4 bytes wide
                bool hasPolarisation_{false};           ///< True if records carry a polarisation vector
                bool hasUserFlags_{false};              ///< True if records carry a user flags value
                bool hasUniversalWeight_{false};        ///< True if all particles share the weight stored in the header
                double universalWeight_{1.0};           ///< The common particle weight (when hasUniversalWeight_ is set)
                std::int32_t universalPDGCode_{0};      ///< The common PDG code (0 means per-particle codes)
                std::string srcProgName_;               ///< Name of the program that wrote the file
                std::vector<std::string> comments_;     ///< Comments recorded in the header

                /**
                 * @brief Read and validate the header and determine where particle records start.
                 *
                 * Parses the fixed portion of the header (magic, format version,
                 * endianness, particle count and option values) and then walks the
                 * variable-length strings and blobs to find the header size.
                 *
                 * @throws std::runtime_error if the header is invalid or the format version is unsupported
                 */
                void readHeader();
        };


        /**
         * @brief Writer class for MCPL (Monte Carlo Particle Lists) files.
         *
         * Writes MCPL format version 3 files in the host byte order using single
         * precision (the values stored internally are single precision, so double
         * precision output would add no information). Polarisation vectors and
         * user flags are written when enabled with the --MCPL-polarisation and
         * --MCPL-userflags options, taking their values from the POLARIZATION_X/Y/Z
         * float properties and the MCPL_USERFLAGS integer property. The
         * --MCPL-universal-weight and --MCPL-universal-pdgcode options store a
         * single common weight or PDG code in the header instead of storing them
         * on every particle record; particles that disagree with the common value
         * are rejected with an error. Particle times are taken from the
         * FloatPropertyType::TIME property. Gzip compression is not supported.
         */
        class Writer : public PhaseSpaceFileWriter
        {
            public:
                /**
                 * @brief Construct a new MCPL file writer.
                 *
                 * @param fileName Path where the MCPL file will be written
                 * @param options User options including MCPL-specific configuration
                 * @throws std::runtime_error if an MCPL-specific option value is invalid
                 */
                Writer(const std::string & fileName, const UserOptions & options = UserOptions{});

                /**
                 * @brief Get the maximum number of particles this format can support.
                 *
                 * @return std::uint64_t Maximum particle count (the count is stored as an unsigned 64-bit integer)
                 */
                std::uint64_t getMaximumSupportedParticles() const override { return std::numeric_limits<std::uint64_t>::max(); }

                /**
                 * @brief Get the list of MCPL-specific command line interface commands.
                 *
                 * @return std::vector<CLICommand> Vector of MCPL-specific CLI commands for writers
                 */
                static std::vector<CLICommand> getFormatSpecificCLICommands();

            protected:
                /**
                 * @brief Get the length of each particle record in bytes.
                 *
                 * @return std::size_t The record length implied by the enabled options
                 */
                std::size_t getParticleRecordLength() const override { return particleSize_; }

                /**
                 * @brief Get the byte offset where particle records start.
                 *
                 * @return std::size_t The header size computed at construction
                 */
                std::size_t getParticleRecordStartOffset() const override { return headerSize_; }

                /**
                 * @brief Write the MCPL header with the final particle count.
                 *
                 * The header must fill the particle record start offset exactly,
                 * since the MCPL header has no padding; a mismatch throws.
                 *
                 * @param buffer The byte buffer to write header data into
                 * @throws std::runtime_error if the header does not have the expected size
                 */
                void writeHeaderData(ByteBuffer & buffer) override;

                /**
                 * @brief Write a single particle record.
                 *
                 * @param buffer The byte buffer to write particle data into
                 * @param particle The particle to write
                 * @throws std::runtime_error if the particle cannot be represented with the enabled options
                 */
                void writeBinaryParticle(ByteBuffer & buffer, Particle & particle) override;

            private:
                bool polarisation_{false};              ///< True to store polarisation vectors on the particle records
                bool userFlags_{false};                 ///< True to store user flags values on the particle records
                bool hasUniversalWeight_{false};        ///< True to store a single common weight in the header
                double universalWeight_{1.0};           ///< The common particle weight (when hasUniversalWeight_ is set)
                std::int32_t universalPDGCode_{0};      ///< The common PDG code (0 means per-particle codes)
                std::string srcProgName_;               ///< Source program name written to the header
                std::vector<std::string> comments_;     ///< Comments written to the header
                std::size_t particleSize_{};            ///< Length of each particle record in bytes
                std::size_t headerSize_{};              ///< Total size of the header in bytes
        };

    } // namespace MCPLphspFile

} // namespace ParticleZoo
