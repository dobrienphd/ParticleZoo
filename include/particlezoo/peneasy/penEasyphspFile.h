#pragma once

#include <string>
#include <limits>
#include <list>

#include "particlezoo/PhaseSpaceFileWriter.h"
#include "particlezoo/PhaseSpaceFileReader.h"
#include "particlezoo/Particle.h"
#include "particlezoo/ByteBuffer.h"

namespace ParticleZoo
{
    namespace penEasyphspFile
    {

        /// @brief Length of the file header in bytes
        static constexpr std::size_t HEADER_LENGTH = 112;
        
        /// @brief Maximum length of each ASCII particle record line
        static constexpr std::size_t MAX_ASCII_LINE_LENGTH = 205;
        
        /// @brief Standard header text for penEasy phase space files
        static constexpr std::string_view FILE_HEADER = "# [PHASE SPACE FILE FORMAT penEasy v.2008-05-15]\n# KPAR : E : X : Y : Z : U : V : W : WGHT : DeltaN : ILB(1..5)\n";

        /**
         * @brief Mapping of PENELOPE ILB array indices to ParticleZoo property types
         * 
         * PENELOPE uses a five value array called ILB (ILB1-ILB5) to track
         * particle interaction history. This array maps array indices to the
         * corresponding IntPropertyType enumeration values.
         */
        constexpr IntPropertyType PROPERTY_PENELOPE_ILB[5] = {
            IntPropertyType::PENELOPE_ILB1,    ///< PENELOPE ILB array value 1, corresponds to the generation of the particle (1 for primary, 2 for secondary, etc.)
            IntPropertyType::PENELOPE_ILB2,    ///< PENELOPE ILB array value 2, corresponds to the particle type of the particle's parent (applies only if ILB1 > 1)
            IntPropertyType::PENELOPE_ILB3,    ///< PENELOPE ILB array value 3, corresponds to the interaction type that created the particle (applies only if ILB1 > 1)
            IntPropertyType::PENELOPE_ILB4,    ///< PENELOPE ILB array value 4, is non-zero if the particle is created by atomic relaxation and corresponds to the atomic transistion that created the particle
            IntPropertyType::PENELOPE_ILB5     ///< PENELOPE ILB array value 5, a user-defined value which is passed on to all descendant particles created by this particle
        };

        /**
         * @brief Writer for penEasy format phase space files
         * 
         * Provides functionality to write phase space data in the penEasy ASCII format,
         * which is compatible with the PENELOPE Monte Carlo transport code. The format
         * includes particle type, energy, position, direction, weight, and PENELOPE-specific
         * values (ILB1-ILB5) and delta-N values.
         */
        class Writer : public PhaseSpaceFileWriter
        {
            public:
                /**
                 * @brief Construct writer for penEasy phase space file
                 * 
                 * @param fileName Path to the output penEasy phase space file
                 * @param options User-specified options for writing behavior
                 */
                Writer(const std::string & fileName, const UserOptions & options = UserOptions{});

                /**
                 * @brief Get the maximum number of particles this format can store
                 * @return Maximum particle count
                 */
                std::uint64_t getMaximumSupportedParticles() const override;

            protected:
                /**
                 * @brief Get the byte offset where particle records start
                 * @return Header length (112 bytes for penEasy format)
                 */
                std::size_t getParticleRecordStartOffset() const override;

                /**
                 * @brief Write the file header to the output buffer
                 * @param buffer Byte buffer to write header data to
                 */
                void writeHeaderData(ByteBuffer & buffer) override;

                /**
                 * @brief Convert a particle to ASCII representation
                 * 
                 * Formats a particle according to the penEasy specification:
                 * KPAR E X Y Z U V W WGHT DeltaN ILB(1..5)
                 * 
                 * @param particle Particle object to convert to ASCII
                 * @return ASCII string representation of the particle
                 * @throws std::runtime_error if particle type is unsupported or data is too long
                 */
                const std::string writeASCIIParticle(Particle & particle) override;

                /**
                 * @brief Get the maximum length of ASCII particle lines, required for buffer sizing
                 * @return Maximum line length
                 */
                size_t getMaximumASCIILineLength() const override;
        };

        // Inline implementations for the Writer class
        inline std::uint64_t Writer::getMaximumSupportedParticles() const { return std::numeric_limits<std::uint64_t>::max(); }
        inline std::size_t Writer::getParticleRecordStartOffset() const { return HEADER_LENGTH; }
        inline size_t Writer::getMaximumASCIILineLength() const { return MAX_ASCII_LINE_LENGTH; }

        /**
         * @brief Reader for penEasy format phase space files
         * 
         * Provides functionality to read phase space data from penEasy ASCII format files.
         * Automatically counts total particles and total histories during construction
         * by scanning the entire file (this may be slow for very large files).
         */
        class Reader : public PhaseSpaceFileReader
        {
            public:
                /**
                 * @brief Construct reader for penEasy phase space file
                 * 
                 * Scans the file during construction to count particles and sum delta-N values
                 * for determining the total number of original histories.
                 * 
                 * @param fileName Path to the penEasy phase space file to read
                 * @param options User-specified options for reading behavior
                 * @throws std::runtime_error if file cannot be opened or parsed
                 */
                Reader(const std::string & fileName, const UserOptions & options = UserOptions{});
    
                /**
                 * @brief Get the total number of particles in the phase space
                 * @return Total particle count determined by file scanning
                 */
                std::uint64_t getNumberOfParticles() const override;

                /**
                 * @brief Get the number of original simulation histories
                 * @return Sum of all delta-N values from particle records
                 */
                std::uint64_t getNumberOfOriginalHistories() const override;
    
            protected:
                /**
                 * @brief Parse a single ASCII line into a Particle object
                 * 
                 * Parses penEasy format: KPAR E X Y Z U V W WGHT DeltaN ILB(1..5)
                 * - KPAR: particle type code (1=electron, 2=photon, 3=positron, 4=proton)
                 * - E: kinetic energy in eV
                 * - X,Y,Z: position coordinates
                 * - U,V,W: direction cosines
                 * - WGHT: particle weight
                 * - DeltaN: incremental history number
                 * - ILB(1..5): PENELOPE ILB array values
                 * 
                 * @param line ASCII line containing particle data
                 * @return Parsed Particle object with all properties set
                 * @throws std::runtime_error if line cannot be parsed or contains invalid data
                 */
                Particle readASCIIParticle(const std::string & line) override;

                /**
                 * @brief Get the maximum length of ASCII particle lines, required for buffer sizing
                 * @return Maximum line length
                 */
                size_t getMaximumASCIILineLength() const override;

            private:
                std::uint64_t numberOfParticles_;         ///< Total number of particles in the file
                std::uint64_t numberOfOriginalHistories_; ///< Sum of delta-N values (original histories)
        };

        // Inline implementations for the Reader class
        inline std::uint64_t Reader::getNumberOfParticles() const { return numberOfParticles_; }
        inline std::uint64_t Reader::getNumberOfOriginalHistories() const { return numberOfOriginalHistories_; }
        inline size_t Reader::getMaximumASCIILineLength() const { return MAX_ASCII_LINE_LENGTH; }

    } // namespace penEasyphspFile

} // namespace ParticleZoo