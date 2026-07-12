#pragma once

#include "particlezoo/PhaseSpaceFileReader.h"
#include "particlezoo/PhaseSpaceFileWriter.h"

#include <string>
#include <limits>
#include <cstdint>

#include "particlezoo/Particle.h"
#include "particlezoo/MCNP/MCNPConstants.h"

namespace ParticleZoo
{
    namespace MCNPphspFile {

        extern CLICommand MCNPTitleCommand;         ///< Command to specify the title card (aids) written to the MCNP surface source file header
        extern CLICommand MCNPSurfaceCommand;       ///< Command to specify the surface number recorded on particle records that do not carry one
        extern CLICommand MCNPFormatCommand;        ///< Command to select the MCNP surface source format to write (MCNP6, MCNP5 or MCNPX)
        extern CLICommand MCNPLegacyHeaderCommand;  ///< Command to write the MCNP 6.1.1-style header instead of the MCNP 6.2/6.3 layout


        /**
         * @brief Reader class for MCNP surface source files (wssa/rssa).
         *
         * Reads MCNP6 (SF_00001), MCNP5 and MCNPX surface source files. Files with
         * 32-bit or 64-bit Fortran record markers and either byte order are handled,
         * as are both the MCNP 6.1.1 and MCNP 6.2/6.3 header layouts, fission-site
         * (KCODE cell source) files, antiparticles, heavy ions, and files with
         * unexpected trailing header records.
         *
         * Not supported: the shortened spherically-symmetric variant (SYM != 0), which
         * fails with a clear error.
         */
        class Reader : public PhaseSpaceFileReader
        {
            public:
                /**
                 * @brief Construct a new MCNP surface source file reader.
                 *
                 * @param fileName Path to the MCNP surface source file to read
                 * @param options User options including user-specific configuration
                 * @throws std::runtime_error if the file is not a valid SF_00001 surface source file
                 */
                Reader(const std::string & fileName, const UserOptions & options = UserOptions{});

                /**
                 * @brief Get the total number of particle tracks in the file (nrss).
                 *
                 * @return std::uint64_t Number of particle tracks
                 */
                std::uint64_t getNumberOfParticles() const override { return numberOfParticles_; }

                /**
                 * @brief Get the number of histories in the surface source write run (|np1|).
                 *
                 * @return std::uint64_t Number of original histories
                 */
                std::uint64_t getNumberOfOriginalHistories() const override { return numberOfOriginalHistories_; }

                /**
                 * @brief Get the number of histories represented on the file (niss).
                 *
                 * @return std::uint64_t Number of represented histories
                 */
                std::uint64_t getNumberOfRepresentedHistories() const override { return numberOfRepresentedHistories_; }

                /**
                 * @brief The represented history count is stored in the header (niss).
                 */
                bool hasNativeRepresentedHistoryCount() const override { return true; }

                /**
                 * @brief History numbers are stored per-particle (the 'a' value of each record).
                 */
                bool hasNativeIncrementalHistoryCounters() const override { return true; }

                /**
                 * @brief Get the MCNP code family that wrote this file.
                 *
                 * @return MCNPVariant MCNP6, MCNPX or MCNP5
                 */
                MCNPVariant getMCNPVariant() const { return variant_; }

                /**
                 * @brief Get the list of MCNP-specific command line interface commands.
                 *
                 * @return std::vector<CLICommand> Vector of MCNP-specific CLI commands
                 */
                static std::vector<CLICommand> getFormatSpecificCLICommands();

                /**
                 * @brief Check if there are more particles to read, resynchronizing the
                 *        history bookkeeping after a seek if necessary.
                 *
                 * New-history detection compares each record's history number against the
                 * preceding record's. This override (called at the start of every read or
                 * peek) detects when the read position has jumped (moveToParticle()) and
                 * re-primes that comparison by reading the record before the seek target.
                 * Without this, the first particle after a seek would always be flagged as
                 * a new history, breaking history-boundary alignment in the parallel readers.
                 *
                 * @return true if there are more particles available to read
                 */
                bool hasMoreParticles() override;

            protected:
                /**
                 * @brief Get the length of each particle record in bytes (including record markers).
                 *
                 * @return std::size_t 8 bytes per SSB value plus the two Fortran record markers
                 */
                std::size_t getParticleRecordLength() const override { return particleRecordDataLength_ + 2 * markerWidth_; }

                /**
                 * @brief Get the byte offset where particle records start.
                 *
                 * @return std::size_t The header size determined while parsing the header records
                 */
                std::size_t getParticleRecordStartOffset() const override { return headerSize_; }

                /**
                 * @brief Read a single particle record (a, b, wgt, erg, tme, x, y, z, u, v, c).
                 *
                 * @param buffer The byte buffer containing the particle record
                 * @return Particle The parsed particle
                 * @throws std::runtime_error if the record markers or particle type are invalid
                 */
                Particle readBinaryParticle(ByteBuffer & buffer) override;

            private:
                MCNPVariant variant_{MCNPVariant::MCNP6};           ///< The MCNP code family that wrote the file
                std::size_t markerWidth_{4};                        ///< Width of the Fortran record length markers in bytes (4 or 8)
                std::size_t particleRecordDataLength_{PARTICLE_RECORD_DATA_LENGTH}; ///< Data bytes per particle record (8 per SSB value; 80 or 88)
                std::size_t headerSize_{};                          ///< Total size of the header records in bytes
                std::uint64_t numberOfParticles_{};                 ///< Number of tracks on the file (nrss)
                std::uint64_t numberOfOriginalHistories_{};         ///< Number of histories in the SSW run (|np1|)
                std::uint64_t numberOfRepresentedHistories_{};      ///< Number of histories on the file (niss)

                std::string kods_{};                                ///< Name of the code that wrote the file
                std::string vers_{};                                ///< Version of the code that wrote the file
                std::string aids_{};                                ///< Title card of the surface source write run

                std::uint64_t lastParsedRecordIndex_{std::numeric_limits<std::uint64_t>::max()}; ///< Index of the last particle record parsed (to make peeking idempotent)
                std::int64_t previousHistoryNumber_{};              ///< History number of the record before the one last parsed
                std::int64_t currentHistoryNumber_{};               ///< History number of the record last parsed
                bool resyncingHistoryState_{false};                 ///< Guards against re-entry while the history bookkeeping is being re-primed after a seek

                /**
                 * @brief Detect the MCNP variant, record marker width and byte order.
                 *
                 * Probes the first bytes of the file for the known MCNP6/MCNPX/MCNP5
                 * header signatures, in native and then swapped byte order.
                 *
                 * @throws std::runtime_error if no known layout is recognized
                 */
                void detectLayout();

                /**
                 * @brief Read and validate the header records and determine the header size.
                 *
                 * Detects the file layout, then parses the format (MCNP6 only), first,
                 * second, third, fourth and summary records.
                 *
                 * @throws std::runtime_error if the header is invalid or the format variant is unsupported
                 */
                void readHeader();

                /**
                 * @brief Re-prime the history bookkeeping after a seek.
                 *
                 * Detects a discontinuous read position (one that is neither the record last
                 * parsed nor the one after it) and, for a non-zero position, reads the
                 * preceding record once so the next particle's new-history comparison uses
                 * the correct baseline; the position and read statistics are then restored
                 * with a second seek. A jump to index 0 simply resets the bookkeeping.
                 */
                void resyncHistoryStateAfterSeek();
        };


        /**
         * @brief Writer class for MCNP surface source files (wssa/rssa).
         *
         * Writes minimally functional MCNP surface source files. By default MCNP6
         * "SF_00001" files with the MCNP 6.2/6.3 header layout are written; the
         * --MCNP-legacy-header option selects the MCNP 6.1.1 layout instead, and the
         * --MCNP-format option selects the older MCNP5 or MCNPX file formats (which
         * support fewer particle types and pack the surface number into each record's
         * type field). A single placeholder SO surface
         * is written to the surface table; the surface number recorded on each particle
         * record can be set with the --MCNP-surface option (default 99999) and is
         * preserved for particles that carry the MCNP_SURFACE_ID (and optionally
         * MCNP_MACROBODY_FACET) integer properties (e.g. particles read from another
         * MCNP file). Particle times are taken from the FloatPropertyType::TIME property.
         */
        class Writer : public PhaseSpaceFileWriter
        {
            public:
                /**
                 * @brief Construct a new MCNP surface source file writer.
                 *
                 * @param fileName Path where the MCNP surface source file will be written
                 * @param options User options including MCNP-specific configuration
                 * @throws std::runtime_error if the requested MCNP format is invalid or
                 *         conflicts with the legacy header option
                 */
                Writer(const std::string & fileName, const UserOptions & options = UserOptions{});

                /**
                 * @brief Get the maximum number of particles this format can support.
                 *
                 * @return std::uint64_t Maximum particle count (nrss is a 64-bit signed
                 *         integer for MCNP6/MCNP5 and a 32-bit integer for MCNPX)
                 */
                std::uint64_t getMaximumSupportedParticles() const override {
                    return writeVariant_ == MCNPVariant::MCNPX
                         ? static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max())
                         : static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
                }

                /**
                 * @brief Get the list of MCNP-specific command line interface commands.
                 *
                 * @return std::vector<CLICommand> Vector of MCNP-specific CLI commands for writers
                 */
                static std::vector<CLICommand> getFormatSpecificCLICommands();

            protected:
                /**
                 * @brief Get the length of each particle record in bytes (including record markers).
                 *
                 * @return std::size_t Record length (96 bytes)
                 */
                std::size_t getParticleRecordLength() const override { return PARTICLE_RECORD_LENGTH; }

                /**
                 * @brief Get the byte offset where particle records start.
                 *
                 * @return std::size_t The fixed header size written by this writer
                 */
                std::size_t getParticleRecordStartOffset() const override {
                    switch (writeVariant_) {
                        case MCNPVariant::MCNP5: return WRITER_HEADER_LENGTH_MCNP5;
                        case MCNPVariant::MCNPX: return WRITER_HEADER_LENGTH_MCNPX;
                        default:                 return legacyHeader_ ? WRITER_HEADER_LENGTH_LEGACY : WRITER_HEADER_LENGTH_MODERN;
                    }
                }

                /**
                 * @brief Write the MCNP surface source header records.
                 *
                 * Writes the format, first, second, third, fourth and summary records with
                 * the accumulated particle and history statistics.
                 *
                 * @param buffer The byte buffer to write header data into
                 */
                void writeHeaderData(ByteBuffer & buffer) override;

                /**
                 * @brief Write a single particle record (a, b, wgt, erg, tme, x, y, z, u, v, c).
                 *
                 * @param buffer The byte buffer to write particle data into
                 * @param particle The particle to write
                 * @throws std::runtime_error if the particle type has no MCNP equivalent
                 */
                void writeBinaryParticle(ByteBuffer & buffer, Particle & particle) override;

            private:
                /**
                 * @brief Delegated constructor taking the pre-validated format to write.
                 *
                 * The write variant is resolved (and any option conflicts rejected) before
                 * the base class is constructed, so that an invalid option cannot leave a
                 * partially constructed writer behind.
                 */
                Writer(const std::string & fileName, const UserOptions & options, MCNPVariant writeVariant);

                std::string title_;                                 ///< Title card (aids) for the header
                std::uint32_t surfaceNumber_;                       ///< Default surface number for particle records and the surface table
                MCNPVariant writeVariant_{MCNPVariant::MCNP6};      ///< The MCNP file format to write
                bool legacyHeader_{false};                          ///< True to write the MCNP 6.1.1 header layout instead of the MCNP 6.2/6.3 layout (MCNP6 only)
                std::uint64_t currentHistoryNumber_{};              ///< Running history number written to the particle records
                std::uint64_t representedHistories_{};              ///< Number of histories that produced tracks on the file (niss)
        };

    } // namespace MCNPphspFile

} // namespace ParticleZoo
