#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cstdint>

#include "particlezoo/Particle.h"
#include "particlezoo/ByteBuffer.h"

namespace ParticleZoo::IAEAphspFile
{
    /**
     * @brief Header manager for IAEA phase space files
     * 
     * This class handles reading, writing, and manipulating the header information
     * for IAEA format phase space files. It manages file metadata, particle statistics,
     * data layout specifications, and validation checksums.
     */
    class IAEAHeader
    {
        /**
         * @brief Statistics tracking for individual particle types
         * 
         * Records count, weight, and energy statistics for particles
         * of a specific type for inclusion in the header.
         */
        struct ParticleStats
        {
            std::uint64_t count_;      ///< Number of particles of this type
            double weightSum_;         ///< Sum of all particle weights
            float minWeight_;          ///< Minimum weight encountered
            float maxWeight_;          ///< Maximum weight encountered
            double energySum_;         ///< Sum of all particle energies
            float minEnergy_;          ///< Minimum energy encountered
            float maxEnergy_;          ///< Maximum energy encountered
            ParticleStats();
        };

        /**
         * @brief Type definition for header sections including their title and content
         */
        using SectionTable = std::unordered_map<std::string, std::string>;

        /**
         * @brief Type definition for mapping particle types to their counts
         */
        using ParticleCountTable = std::unordered_map<ParticleType, std::uint64_t>;

        /**
         * @brief Type definition for mapping particle types to their statistics
         */
        using ParticleStatsTable = std::unordered_map<ParticleType, ParticleStats>;

        public:

            /**
             * @brief File type classification for IAEA phase space files
             */
            enum class FileType {
                PHSP_FILE = 0,        ///< Standard phase space file
                PHSP_GENERATOR = 1    ///< Phase space generator file (as far as I know this is not used anywhere, but it exists in the original implementation)
            };

            /**
             * @brief Header section identifiers for IAEA format
             * 
             * Defines all standard sections that can appear in an IAEA header file,
             * used for parsing and generating header content. Includes a CUSTOM_SECTION
             * for user-defined entries.
             */
            enum class SECTION {
                IAEA_INDEX,                           ///< IAEA index code
                TITLE,                                ///< File title/description
                FILE_TYPE,                            ///< Either PHSP_FILE or PHSP_GENERATOR
                CHECKSUM,                             ///< Data integrity checksum
                RECORD_CONTENTS,                      ///< Description of record structure
                RECORD_CONSTANT,                      ///< Constant values in records
                RECORD_LENGTH,                        ///< Length of each particle record
                BYTE_ORDER,                           ///< Byte ordering specification (endianness)
                ORIGINAL_HISTORIES,                   ///< Number of original simulation histories
                PARTICLES,                            ///< Total particle count
                PHOTONS,                              ///< Photon count and statistics
                ELECTRONS,                            ///< Electron count and statistics
                POSITRONS,                            ///< Positron count and statistics
                NEUTRONS,                             ///< Neutron count and statistics
                PROTONS,                              ///< Proton count and statistics
                TRANSPORT_PARAMETERS,                 ///< Monte Carlo transport settings
                MACHINE_TYPE,                         ///< Linear accelerator type
                MONTE_CARLO_CODE_VERSION,             ///< Monte Carlo code version information
                GLOBAL_PHOTON_ENERGY_CUTOFF,          ///< Global photon cutoff energy
                GLOBAL_PARTICLE_ENERGY_CUTOFF,        ///< Global particle cutoff energy
                COORDINATE_SYSTEM_DESCRIPTION,        ///< Coordinate system definition
                BEAM_NAME,                            ///< Treatment beam name
                FIELD_SIZE,                           ///< Radiation field dimensions
                NOMINAL_SSD,                          ///< Source-to-surface distance
                MC_INPUT_FILENAME,                    ///< Monte Carlo input file name
                VARIANCE_REDUCTION_TECHNIQUES,        ///< Variance reduction methods used
                INITIAL_SOURCE_DESCRIPTION,           ///< Primary source description
                PUBLISHED_REFERENCE,                  ///< Publication reference
                AUTHORS,                              ///< File authors
                INSTITUTION,                          ///< Institution name
                LINK_VALIDATION,                      ///< Validation link information
                ADDITIONAL_NOTES,                     ///< Additional notes
                STATISTICAL_INFORMATION_PARTICLES,    ///< Particle statistics summary
                STATISTICAL_INFORMATION_GEOMETRY,     ///< Geometric statistics summary
                CUSTOM_SECTION                        ///< User-defined section
            };

            /**
             * @brief Extra integer data types for IAEA format
             * 
             * Defines the types of additional integer data that can be stored
             * with each particle record beyond the standard IAEA format.
             * Refered to as "long" in the original IAEA documentation, however it
             * is always a 32-bit integer on both 32-bit and 64-bit systems.
             */
            enum class EXTRA_LONG_TYPE {
                USER_DEFINED_GENERIC_TYPE = 0,       ///< Generic user-defined integer
                INCREMENTAL_HISTORY_NUMBER = 1,      ///< Sequential history number for tracking, tracks the number of new histories since the last particle was recorded
                EGS_LATCH = 2,                       ///< EGS-specific latch variable (see BEAMnrc User Manual for details)
                PENELOPE_ILB5 = 3,                   ///< PENELOPE ILB array value 1, corresponds to the generation of the particle (1 for primary, 2 for secondary, etc.)
                PENELOPE_ILB4 = 4,                   ///< PENELOPE ILB array value 2, corresponds to the particle type of the particle's parent (applies only if ILB1 > 1)
                PENELOPE_ILB3 = 5,                   ///< PENELOPE ILB array value 3, corresponds to the interaction type that created the particle (applies only if ILB1 > 1)
                PENELOPE_ILB2 = 6,                   ///< PENELOPE ILB array value 4, is non-zero if the particle is created by atomic relaxation and corresponds to the atomic transistion that created the particle
                PENELOPE_ILB1 = 7                    ///< PENELOPE ILB array value 5, a user-defined value which is passed on to all descendant particles created by this particle
            };

            /**
             * @brief Extra float data types for IAEA format
             * 
             * Defines the types of additional floating-point data that can be stored
             * with each particle record beyond the standard IAEA format.
             */
            enum class EXTRA_FLOAT_TYPE {
                USER_DEFINED_GENERIC_TYPE = 0,       ///< Generic user-defined float
                XLAST = 1,                           ///< Last X position
                YLAST = 2,                           ///< Last Y position
                ZLAST = 3                            ///< Last Z position
            };

            // Constructors and destructor

            /**
             * @brief Construct header from existing IAEA header file
             * 
             * @param filePath Path to the IAEA header file (.IAEAheader)
             * @param newFile If true, creates a new header; if false, reads existing file
             * @throws std::runtime_error if file cannot be read or is invalid
             */
            IAEAHeader(const std::string &filePath, bool newFile = false);

            /**
             * @brief Copy constructor with new file path
             * 
             * Creates a new header based on an existing one but with a different file path.
             * Resets particle counts and statistics to zero.
             * 
             * @param other Source header to copy from
             * @param newFilePath Path for the new header file
             */
            IAEAHeader(const IAEAHeader &other, const std::string & newFilePath);

            // Header file operations

            /**
             * @brief Write header information to file
             * 
             * Writes the complete header information to the associated .IAEAheader file,
             * including all sections, particle statistics, and metadata.
             * 
             * @throws std::runtime_error if file cannot be written
             */
            void writeHeader();

            // Getters for IAEAHeader attributes

            /**
             * @brief Get the path to the header file
             * @return Path to the .IAEAheader file
             */
            std::string         getHeaderFilePath() const;

            /**
             * @brief Get the path to the associated data file
             * @return Path to the .IAEAphsp data file
             */
            std::string         getDataFilePath() const;

            /**
             * @brief Get the IAEA index string
             * @return IAEA index (preserved with leading zeros if present)
             */
            std::string         getIAEAIndex() const;

            /**
             * @brief Get the phase space file title
             * @return Title string describing the phase space file
             */
            const std::string & getTitle() const;

            /**
             * @brief Get the file type classification
             * @return FileType indicating PHSP_FILE or PHSP_GENERATOR
             */
            FileType            getFileType() const;

            /**
             * @brief Get the data integrity checksum
             * @return Checksum value for data validation
             */
            std::uint64_t       getChecksum() const;

            /**
             * @brief Check if X coordinates are stored in records
             * @return true if X values are stored, false if constant
             */
            bool                xIsStored() const;

            /**
             * @brief Check if Y coordinates are stored in records
             * @return true if Y values are stored, false if constant
             */
            bool                yIsStored() const;

            /**
             * @brief Check if Z coordinates are stored in records
             * @return true if Z values are stored, false if constant
             */
            bool                zIsStored() const;

            /**
             * @brief Check if U direction cosines are stored in records
             * @return true if U values are stored, false if constant
             */
            bool                uIsStored() const;

            /**
             * @brief Check if V direction cosines are stored in records
             * @return true if V values are stored, false if constant
             */
            bool                vIsStored() const;

            /**
             * @brief Check if W direction cosines are stored in records
             * @note W being 'stored' means that it is not a constant value recorded in the header.
             * The 'stored' value is only implicitly stored and is actually calculated as needed from U and V.
             * @return true if W values are stored, false if constant
             */
            bool                wIsStored() const;

            /**
             * @brief Check if particle weights are stored in records
             * @return true if weights are stored, false if constant
             */
            bool                weightIsStored() const;

            /**
             * @brief Get the number of extra float values per record
             * @return Count of additional floating-point values
             */
            unsigned int        getNumberOfExtraFloats() const;

            /**
             * @brief Get the number of extra integer values per record
             * @return Count of additional integer values
             */
            unsigned int        getNumberOfExtraLongs() const;

            /**
             * @brief Get the constant X coordinate value (when not stored per particle)
             * @return X coordinate when not stored per particle
             */
            float               getConstantX() const;

            /**
             * @brief Get the constant Y coordinate value (when not stored per particle)
             * @return Y coordinate when not stored per particle
             */
            float               getConstantY() const;

            /**
             * @brief Get the constant Z coordinate value (when not stored per particle)
             * @return Z coordinate when not stored per particle
             */
            float               getConstantZ() const;

            /**
             * @brief Get the constant U direction cosine value (when not stored per particle)
             * @return U direction cosine when not stored per particle
             */
            float               getConstantU() const;

            /**
             * @brief Get the constant V direction cosine value (when not stored per particle)
             * @return V direction cosine when not stored per particle
             */
            float               getConstantV() const;

            /**
             * @brief Get the constant W direction cosine value (when not implicitly stored per particle)
             * @return W direction cosine when not stored per particle
             */
            float               getConstantW() const;

            /**
             * @brief Get the constant particle weight value (when not stored per particle)
             * @return Weight when not stored per particle
             */
            float               getConstantWeight() const;

            /**
             * @brief Get the type of the extra float value at the specified index
             * @param index Index of the extra float (0-based)
             * @return EXTRA_FLOAT_TYPE describing the data type
             * @throws std::out_of_range if index is invalid
             */
            EXTRA_FLOAT_TYPE    getExtraFloatType(unsigned int index) const;

            /**
             * @brief Get the type of the extra integer value at the specified index
             * @param index Index of the extra integer (0-based)
             * @return EXTRA_LONG_TYPE describing the data type
             * @throws std::out_of_range if index is invalid
             */
            EXTRA_LONG_TYPE     getExtraLongType(unsigned int index) const;

            /**
             * @brief Get the length of each particle record in bytes
             * @return Record length in bytes
             */
            std::size_t         getRecordLength() const;

            /**
             * @brief Get the byte order for binary data (endianness)
             * @return ByteOrder specification for data interpretation
             */
            ByteOrder           getByteOrder() const;

            /**
             * @brief Get the number of original simulation histories
             * @return The number of primary histories used to generate the phase space
             */
            std::uint64_t       getOriginalHistories() const;

            /**
             * @brief Get the total number of particles in the phase space
             * @return Total particle count across all types
             */
            std::uint64_t       getNumberOfParticles() const;

            /**
             * @brief Get the number of particles of a specific type
             * @param particleType Type of particle to count
             * @return Number of particles of the specified type
             */
            std::uint64_t       getNumberOfParticles(ParticleType particleType) const;

            /**
             * @brief Get a header section value by name
             * @param sectionName Name of the section to retrieve
             * @return Section content as string, "UNKNOWN" if not found
             */
            const std::string   getSection(const std::string &sectionName) const;

            /**
             * @brief Get a header section value by enum
             * @param section Section identifier to retrieve
             * @return Section content as string, empty if not found
             */
            const std::string   getSection(SECTION section) const;

            // Getters for particle statistics

            /**
             * @brief Get the minimum X coordinate across all particles
             * @return Minimum X value in the phase space
             */
            float               getMinX() const;

            /**
             * @brief Get the maximum X coordinate across all particles
             * @return Maximum X value in the phase space
             */
            float               getMaxX() const;

            /**
             * @brief Get the minimum Y coordinate across all particles
             * @return Minimum Y value in the phase space
             */
            float               getMinY() const;

            /**
             * @brief Get the maximum Y coordinate across all particles
             * @return Maximum Y value in the phase space
             */
            float               getMaxY() const;

            /**
             * @brief Get the minimum Z coordinate across all particles
             * @return Minimum Z value in the phase space
             */
            float               getMinZ() const;

            /**
             * @brief Get the maximum Z coordinate across all particles
             * @return Maximum Z value in the phase space
             */
            float               getMaxZ() const;

            /**
             * @brief Get the minimum weight for particles of a specific type
             * @param particleType Type of particle to query
             * @return Minimum weight value for the particle type
             */
            float               getMinWeight(ParticleType particleType) const;

            /**
             * @brief Get the maximum weight for particles of a specific type
             * @param particleType Type of particle to query
             * @return Maximum weight value for the particle type
             */
            float               getMaxWeight(ParticleType particleType) const;

            /**
             * @brief Get the minimum energy for particles of a specific type
             * @param particleType Type of particle to query
             * @return Minimum kinetic energy for the particle type
             */
            float               getMinEnergy(ParticleType particleType) const;

            /**
             * @brief Get the maximum energy for particles of a specific type
             * @param particleType Type of particle to query
             * @return Maximum kinetic energy for the particle type
             */
            float               getMaxEnergy(ParticleType particleType) const;

            /**
             * @brief Get the mean weight for particles of a specific type
             * @param particleType Type of particle to query
             * @return Average weight value for the particle type
             */
            float               getMeanWeight(ParticleType particleType) const;

            /**
             * @brief Get the mean energy for particles of a specific type
             * @param particleType Type of particle to query
             * @return Average kinetic energy for the particle type
             */
            float               getMeanEnergy(ParticleType particleType) const;

            /**
             * @brief Get the total weight for particles of a specific type
             * @param particleType Type of particle to query
             * @return Sum of all weights for the particle type
             */
            float               getTotalWeight(ParticleType particleType) const;

            // Setters for IAEAHeader attributes

            /**
             * @brief Set the file path for the header
             * @param filePath New path to the .IAEAheader file
             */
            void                setFilePath(const std::string &filePath);

            /**
             * @brief Set the IAEA index string
             * @param index New IAEA index identifier
             */
            void                setIAEAIndex(const std::string &index);

            /**
             * @brief Set the phase space file title
             * @param title New title for the file
             */
            void                setTitle(const std::string &title);

            /**
             * @brief Set the file type classification
             * @param fileType Type specification (PHSP_FILE or PHSP_GENERATOR)
             */
            void                setFileType(FileType fileType);

            /**
             * @brief Set the data integrity checksum
             * @param checksum New checksum value
             */
            void                setChecksum(std::uint64_t checksum);

            /**
             * @brief Set the constant X coordinate value
             * @param x X coordinate for all particles
             */
            void                setConstantX(float x);

            /**
             * @brief Set the constant Y coordinate value
             * @param y Y coordinate for all particles
             */
            void                setConstantY(float y);

            /**
             * @brief Set the constant Z coordinate value
             * @param z Z coordinate for all particles
             */
            void                setConstantZ(float z);

            /**
             * @brief Set the constant U direction cosine value
             * @param u U direction cosine for all particles
             */
            void                setConstantU(float u);

            /**
             * @brief Set the constant V direction cosine value
             * @param v V direction cosine for all particles
             */
            void                setConstantV(float v);

            /**
             * @brief Set the constant W direction cosine value
             * @param w W direction cosine for all particles
             */
            void                setConstantW(float w);

            /**
             * @brief Set the constant particle weight value
             * @param weight Weight for all particles
             */
            void                setConstantWeight(float weight);

            /**
             * @brief Add an extra float data type to the record format
             * @param type Type of additional floating-point data to include
             */
            void                addExtraFloat(EXTRA_FLOAT_TYPE type);

            /**
             * @brief Add an extra integer data type to the record format
             * @param type Type of additional integer data to include
             */
            void                addExtraLong(EXTRA_LONG_TYPE type);

            /**
             * @brief Check if a specific extra float type is included
             * @param type Extra float type to check for
             * @return true if the type is included in the record format
             */
            bool                hasExtraFloat(EXTRA_FLOAT_TYPE type) const;

            /**
             * @brief Check if a specific extra integer type is included
             * @param type Extra integer type to check for
             * @return true if the type is included in the record format
             */
            bool                hasExtraLong(EXTRA_LONG_TYPE type) const;

            /**
             * @brief Set the particle record length in bytes
             * @param length New record length for each particle
             */
            void                setRecordLength(std::size_t length);

            /**
             * @brief Set the number of original simulation histories
             * @param originalHistories Count of primary histories
             */
            void                setOriginalHistories(std::uint64_t originalHistories);

            /**
             * @brief Set the total number of particles
             * @param numberOfParticles Total particle count across all types
             */
            void                setNumberOfParticles(std::uint64_t numberOfParticles);

            /**
             * @brief Set the number of particles for a specific type
             * @param particleType Type of particle to set count for
             * @param numberOfParticles Number of particles of this type
             */
            void                setNumberOfParticles(ParticleType particleType, std::uint64_t numberOfParticles);

            /**
             * @brief Update particle statistics with a new particle
             * @param particle Particle to include in statistics calculations
             */
            void                countParticleStats(const Particle & particle);

            /**
             * @brief Set a header section value by name
             * @param sectionName Name of the section to set
             * @param sectionValue Content to store in the section
             */
            void                setSection(const std::string &sectionName, const std::string &sectionValue);

            /**
             * @brief Set a header section value using the explicit enum type
             * @param section Section identifier to set
             * @param sectionValue Content to store in the section
             */
            void                setSection(SECTION section, const std::string &sectionValue);

            // Setters for particle statistics

            /**
             * @brief Set the minimum X coordinate boundary
             * @param minX Minimum X value in the phase space
             */
            void                setMinX(float minX);

            /**
             * @brief Set the maximum X coordinate boundary
             * @param maxX Maximum X value in the phase space
             */
            void                setMaxX(float maxX);

            /**
             * @brief Set the minimum Y coordinate boundary
             * @param minY Minimum Y value in the phase space
             */
            void                setMinY(float minY);

            /**
             * @brief Set the maximum Y coordinate boundary
             * @param maxY Maximum Y value in the phase space
             */
            void                setMaxY(float maxY);

            /**
             * @brief Set the minimum Z coordinate boundary
             * @param minZ Minimum Z value in the phase space
             */
            void                setMinZ(float minZ);

            /**
             * @brief Set the maximum Z coordinate boundary
             * @param maxZ Maximum Z value in the phase space
             */
            void                setMaxZ(float maxZ);

            /**
             * @brief Set the minimum weight for particles of a specific type
             * @param particleType Type of particle to set statistics for
             * @param minWeight Minimum weight value for this particle type
             */
            void                setMinWeight(ParticleType particleType, float minWeight);

            /**
             * @brief Set the maximum weight for particles of a specific type
             * @param particleType Type of particle to set statistics for
             * @param maxWeight Maximum weight value for this particle type
             */
            void                setMaxWeight(ParticleType particleType, float maxWeight);

            /**
             * @brief Set the minimum energy for particles of a specific type
             * @param particleType Type of particle to set statistics for
             * @param minEnergy Minimum kinetic energy for this particle type
             */
            void                setMinEnergy(ParticleType particleType, float minEnergy);

            /**
             * @brief Set the maximum energy for particles of a specific type
             * @param particleType Type of particle to set statistics for
             * @param maxEnergy Maximum kinetic energy for this particle type
             */
            void                setMaxEnergy(ParticleType particleType, float maxEnergy);

            /**
             * @brief Set the mean energy for particles of a specific type
             * @param particleType Type of particle to set statistics for
             * @param meanEnergy Average kinetic energy for this particle type
             */
            void                setMeanEnergy(ParticleType particleType, float meanEnergy);

            /**
             * @brief Set the total weight for particles of a specific type
             * @param particleType Type of particle to set statistics for
             * @param totalWeight Sum of all weights for this particle type
             */
            void                setTotalWeight(ParticleType particleType, float totalWeight);

            // Validation and utility methods

            /**
             * @brief Validate the data integrity checksum
             * This check is strict. Not only does it verify that the checksum matches the file size,
             * but it also checks that it equals the record length multiplied by the number of particles.
             * @return true if checksum matches expected value based on file size and record length
             */
            bool                checksumIsValid() const;

            /**
             * @brief Determine the header file path from a data file name
             * @param filename Path to the data file (.IAEAphsp)
             * @return Path to the corresponding header file (.IAEAheader)
             */
            const static std::string DeterminePathToHeaderFile(const std::string &filename);

            /**
             * @brief Convert IAEA extra float type to ParticleZoo property type
             * @param type IAEA-specific extra float type
             * @return Corresponding ParticleZoo FloatPropertyType
             */
            constexpr static FloatPropertyType translateExtraFloatType(IAEAHeader::EXTRA_FLOAT_TYPE type);

            /**
             * @brief Convert IAEA extra 'long' type to ParticleZoo integer property type
             * @param type IAEA-specific extra 'long' type
             * @return Corresponding ParticleZoo IntPropertyType
             */
            constexpr static IntPropertyType translateExtraLongType(IAEAHeader::EXTRA_LONG_TYPE type);

        private:
            std::string         filePath_;
            std::string         IAEAIndex_;
            std::string         title_;
            FileType            fileType_;
            std::uint64_t       checksum_;

            bool                xIsStored_;
            bool                yIsStored_;
            bool                zIsStored_;
            bool                uIsStored_;
            bool                vIsStored_;
            bool                wIsStored_;
            bool                weightIsStored_;

            float               constantX_;
            float               constantY_;
            float               constantZ_;
            float               constantU_;
            float               constantV_;
            float               constantW_;
            float               constantWeight_;

            std::vector<EXTRA_FLOAT_TYPE> extraFloatData_;
            std::vector<EXTRA_LONG_TYPE> extraLongData_;

            std::size_t         recordLength_;
            ByteOrder           byteOrder_;
            std::uint64_t       originalHistories_;
            std::uint64_t       numberOfParticles_;

            float minX_, maxX_;
            float minY_, maxY_;
            float minZ_, maxZ_;
            ParticleStatsTable  particleStatsTable_;

            SectionTable        sectionTable_;

            void                readHeader(std::ifstream & file);
            void                generateSectionTable();
            unsigned int        calculateMinimumRecordLength() const;

            // helper functions for string parsing
            static bool isSectionHeader(const std::string &str);
            static std::uint64_t getIntValue(const std::string &str);
            static float getFloatValue(const std::string &str);
            static std::vector<float> getFloatArray(const std::string &str);
            static std::vector<std::uint64_t> getIntArray(const std::string &str);
            static std::string removeInlineComments(const std::string &str);
            static std::string stripWhiteSpace(const std::string &str);
            static std::string cleanLine(const std::string &line);

            constexpr static std::string_view sectionToString(SECTION section);
            constexpr static IAEAHeader::SECTION getSectionFromString(const std::string & sectionTitle);

            constexpr static std::string_view EXTRA_FLOAT_TYPE_LABELS[4] = {
                "Generic float variable stored in the extrafloat array",
                "XLAST variable stored in the extrafloat array",
                "YLAST variable stored in the extrafloat array",
                "ZLAST variable stored in the extrafloat array"
            };

            constexpr static std::string_view EXTRA_LONG_TYPE_LABELS[8] = {
                "Generic integer variable stored in the extralong array",
                "Incremental history number stored in the extralong array",
                "LATCH EGS variable stored in the extralong array",
                "ILB5 PENELOPE variable stored in the extralong array",
                "ILB4 PENELOPE variable stored in the extralong array",
                "ILB3 PENELOPE variable stored in the extralong array",
                "ILB2 PENELOPE variable stored in the extralong array",
                "ILB1 PENELOPE variable stored in the extralong array"
            };
    };

    // Inline implementation of the IAEAHeader class methods

    inline std::string IAEAHeader::getHeaderFilePath()  const { return filePath_; }
    inline std::string IAEAHeader::getIAEAIndex() const { return IAEAIndex_; }
    inline const std::string & IAEAHeader::getTitle() const { return title_; }
    inline IAEAHeader::FileType IAEAHeader::getFileType() const { return fileType_; }
    inline std::uint64_t IAEAHeader::getChecksum() const { return checksum_; }
    inline bool IAEAHeader::xIsStored() const { return xIsStored_; }
    inline bool IAEAHeader::yIsStored() const { return yIsStored_; }
    inline bool IAEAHeader::zIsStored() const { return zIsStored_; }
    inline bool IAEAHeader::uIsStored() const { return uIsStored_; }
    inline bool IAEAHeader::vIsStored() const { return vIsStored_; }
    inline bool IAEAHeader::wIsStored() const { return wIsStored_; }
    inline bool IAEAHeader::weightIsStored() const { return weightIsStored_; }
    inline float IAEAHeader::getConstantX() const { return constantX_; }
    inline float IAEAHeader::getConstantY() const { return constantY_; }
    inline float IAEAHeader::getConstantZ() const { return constantZ_; }
    inline float IAEAHeader::getConstantU() const { return constantU_; }
    inline float IAEAHeader::getConstantV() const { return constantV_; }
    inline float IAEAHeader::getConstantW() const { return constantW_; }
    inline float IAEAHeader::getConstantWeight() const { return constantWeight_; }

    inline IAEAHeader::EXTRA_FLOAT_TYPE IAEAHeader::getExtraFloatType(unsigned int index) const {
        if (index >= extraFloatData_.size()) {
            throw std::out_of_range("Index out of range for extra float data.");
        }
        return extraFloatData_[index];
    }
    
    inline IAEAHeader::EXTRA_LONG_TYPE IAEAHeader::getExtraLongType(unsigned int index) const {
        if (index >= extraLongData_.size()) {
            throw std::out_of_range("Index out of range for extra long data.");
        }
        return extraLongData_[index];
    }
    
    inline std::size_t IAEAHeader::getRecordLength() const { return recordLength_; }
    inline ByteOrder IAEAHeader::getByteOrder() const { return byteOrder_; }
    inline unsigned int IAEAHeader::getNumberOfExtraFloats() const { return extraFloatData_.size(); }
    inline unsigned int IAEAHeader::getNumberOfExtraLongs() const { return extraLongData_.size(); }
    inline std::uint64_t IAEAHeader::getOriginalHistories() const { return originalHistories_; }
    inline std::uint64_t IAEAHeader::getNumberOfParticles() const { return numberOfParticles_; }
    inline float IAEAHeader::getMinX() const { return minX_; }
    inline float IAEAHeader::getMaxX() const { return maxX_; }
    inline float IAEAHeader::getMinY() const { return minY_; }
    inline float IAEAHeader::getMaxY() const { return maxY_; }
    inline float IAEAHeader::getMinZ() const { return minZ_; }
    inline float IAEAHeader::getMaxZ() const { return maxZ_; }

    inline void IAEAHeader::setFilePath(const std::string &filePath) { filePath_ = filePath; }
    inline void IAEAHeader::setIAEAIndex(const std::string &index) { IAEAIndex_ = index; }
    inline void IAEAHeader::setTitle(const std::string &title) { title_ = title; }
    inline void IAEAHeader::setFileType(IAEAHeader::FileType fileType) { fileType_ = fileType; }
    inline void IAEAHeader::setChecksum(std::uint64_t checksum) { checksum_ = checksum; }
    inline void IAEAHeader::setConstantX(float x) { constantX_ = x; if (xIsStored_) { xIsStored_ = false; recordLength_ -= sizeof(float); } }
    inline void IAEAHeader::setConstantY(float y) { constantY_ = y; if (yIsStored_) { yIsStored_ = false; recordLength_ -= sizeof(float); } }
    inline void IAEAHeader::setConstantZ(float z) { constantZ_ = z; if (zIsStored_) { zIsStored_ = false; recordLength_ -= sizeof(float); } }
    inline void IAEAHeader::setConstantU(float u) { constantU_ = u; if (uIsStored_) { uIsStored_ = false; recordLength_ -= sizeof(float); } }
    inline void IAEAHeader::setConstantV(float v) { constantV_ = v; if (vIsStored_) { vIsStored_ = false; recordLength_ -= sizeof(float); } }
    inline void IAEAHeader::setConstantW(float w) { constantW_ = w; if (wIsStored_) { wIsStored_ = false; recordLength_ -= sizeof(float); } }
    inline void IAEAHeader::setConstantWeight(float weight) { constantWeight_ = weight; if (weightIsStored_) { weightIsStored_ = false; recordLength_ -= sizeof(float); } }
    inline void IAEAHeader::setRecordLength(std::size_t length) { recordLength_ = length; }
    inline void IAEAHeader::setOriginalHistories(std::uint64_t originalHistories) { originalHistories_ = originalHistories; }
    inline void IAEAHeader::setNumberOfParticles(std::uint64_t numberOfParticles) { numberOfParticles_ = numberOfParticles; }
    inline void IAEAHeader::setNumberOfParticles(ParticleType particleType, std::uint64_t numberOfParticles) {
        if (particleStatsTable_.find(particleType) == particleStatsTable_.end()) {
            particleStatsTable_[particleType] = ParticleStats();
        }
        particleStatsTable_[particleType].count_ = numberOfParticles;
    }

    inline void IAEAHeader::addExtraFloat(EXTRA_FLOAT_TYPE type) {
        if (!hasExtraFloat(type)) {
            extraFloatData_.push_back(type);
            recordLength_ += sizeof(float);
        }
    }
    
    inline void IAEAHeader::addExtraLong(EXTRA_LONG_TYPE type) {
        if (!hasExtraLong(type)) {
            extraLongData_.push_back(type);
            recordLength_ += sizeof(std::int32_t);
        }
    }

    inline bool IAEAHeader::hasExtraFloat(EXTRA_FLOAT_TYPE type) const {
        return std::find(extraFloatData_.begin(), extraFloatData_.end(), type) != extraFloatData_.end();
    }

    inline bool IAEAHeader::hasExtraLong(EXTRA_LONG_TYPE type) const {
        return std::find(extraLongData_.begin(), extraLongData_.end(), type) != extraLongData_.end();
    }

    inline void IAEAHeader::setMinX(float minX) { minX_ = minX; }
    inline void IAEAHeader::setMaxX(float maxX) { maxX_ = maxX; }
    inline void IAEAHeader::setMinY(float minY) { minY_ = minY; }
    inline void IAEAHeader::setMaxY(float maxY) { maxY_ = maxY; }
    inline void IAEAHeader::setMinZ(float minZ) { minZ_ = minZ; }
    inline void IAEAHeader::setMaxZ(float maxZ) { maxZ_ = maxZ; }
    inline void IAEAHeader::setMinWeight(ParticleType particleType, float minWeight) { particleStatsTable_[particleType].minWeight_ = minWeight; }
    inline void IAEAHeader::setMaxWeight(ParticleType particleType, float maxWeight) { particleStatsTable_[particleType].maxWeight_ = maxWeight; }
    inline void IAEAHeader::setMinEnergy(ParticleType particleType, float minEnergy) { particleStatsTable_[particleType].minEnergy_ = minEnergy; }
    inline void IAEAHeader::setMaxEnergy(ParticleType particleType, float maxEnergy) { particleStatsTable_[particleType].maxEnergy_ = maxEnergy; }
    inline void IAEAHeader::setMeanEnergy(ParticleType particleType, float meanEnergy) { particleStatsTable_[particleType].energySum_ = (double)meanEnergy * particleStatsTable_[particleType].count_; }
    inline void IAEAHeader::setTotalWeight(ParticleType particleType, float totalWeight) { particleStatsTable_[particleType].weightSum_ = (double)totalWeight; }

    // keeping this function inline for performance reasons
    inline void IAEAHeader::countParticleStats(const Particle &particle)
    {
        // Retrieve the particle type once.
        ParticleType type = particle.getType();
    
        // Update global particle count and the per-type count.
        numberOfParticles_++;
    
        // Update the original histories based on particle properties.
        if (particle.hasIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER))
        {
            originalHistories_ += particle.getIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER);
        }
        else if (particle.isNewHistory())
        {
            originalHistories_++;
        }
    
        // Update particle statistics.
        auto &stats = particleStatsTable_[type];
        float weight         = particle.getWeight();
        float kineticEnergy  = particle.getKineticEnergy();
        stats.count_++;
        stats.weightSum_    += (double)weight;
        stats.minWeight_     = std::min(stats.minWeight_, weight);
        stats.maxWeight_     = std::max(stats.maxWeight_, weight);
        stats.energySum_    += (double)kineticEnergy;
        stats.minEnergy_     = std::min(stats.minEnergy_, kineticEnergy);
        stats.maxEnergy_     = std::max(stats.maxEnergy_, kineticEnergy);
    
        // Cache spatial coordinates and update global bounds.
        float x = particle.getX();
        float y = particle.getY();
        float z = particle.getZ();
        minX_ = std::min(minX_, x);
        maxX_ = std::max(maxX_, x);
        minY_ = std::min(minY_, y);
        maxY_ = std::max(maxY_, y);
        minZ_ = std::min(minZ_, z);
        maxZ_ = std::max(maxZ_, z);

        checksum_ = numberOfParticles_ * recordLength_;
    }


    // helper function to strip string of white space
    inline std::string IAEAHeader::stripWhiteSpace(const std::string &str)
    {
        size_t start = str.find_first_not_of(" \t\r\n");
        size_t end = str.find_last_not_of(" \t\r\n");
        return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
    }

    // helper function to remove inline comments
    inline std::string IAEAHeader::removeInlineComments(const std::string &str)
    {
        size_t pos = 0;
        while (true) {
            pos = str.find("//", pos);
            if (pos == std::string::npos)
                break;
    
            // Check if the "//" is preceded by at least one whitespace character.
            if (pos == 0 || std::isspace(static_cast<unsigned char>(str[pos - 1]))) {
                return str.substr(0, pos);
            }
            // Not a valid comment, search further.
            pos += 2;
        }
        return str;
    }

    // helper function to clean the line by removing comments and stripping whitespace
    inline std::string IAEAHeader::cleanLine(const std::string &line)
    {
        std::string cleanedLine = removeInlineComments(line);
        cleanedLine = stripWhiteSpace(cleanedLine);
        return cleanedLine;
    }

    // helper function to check if the line is a section header
    inline bool IAEAHeader::isSectionHeader(const std::string &str)
    {
        return str[0] == '$' && str.find(":") != std::string::npos;
    }

    // return int value of contents
    inline std::uint64_t IAEAHeader::getIntValue(const std::string &str)
    {
        std::istringstream iss(str);
        std::uint64_t intValue;
        iss >> intValue;
        return intValue;
    }

    // return float value of contents
    inline float IAEAHeader::getFloatValue(const std::string &str)
    {
        std::istringstream iss(str);
        float floatValue;
        iss >> floatValue;
        return floatValue;
    }

    // return array of floats taken line by line from the contents
    inline std::vector<float> IAEAHeader::getFloatArray(const std::string &str)
    {
        std::istringstream iss(str);
        std::vector<float> floatArray;
        std::string line;
        while (std::getline(iss, line))
        {
            std::istringstream lineStream(line);
            float value;
            while (lineStream >> value)
            {
                floatArray.push_back(value);
            }
        }
        return floatArray;
    }

    // return array of ints taken line by line from the contents
    inline std::vector<std::uint64_t> IAEAHeader::getIntArray(const std::string &str)
    {
        std::istringstream iss(str);
        std::vector<std::uint64_t> intArray;
        std::string line;
        while (std::getline(iss, line))
        {
            std::istringstream lineStream(line);
            std::uint64_t value;
            while (lineStream >> value)
            {
                intArray.push_back(value);
            }
        }
        return intArray;
    }

    constexpr std::string_view IAEAHeader::sectionToString(SECTION section)
    {
        switch (section) {
            case SECTION::IAEA_INDEX:                          return "IAEA_INDEX";
            case SECTION::TITLE:                               return "TITLE";
            case SECTION::FILE_TYPE:                           return "FILE_TYPE";
            case SECTION::CHECKSUM:                            return "CHECKSUM";
            case SECTION::RECORD_CONTENTS:                     return "RECORD_CONTENTS";
            case SECTION::RECORD_CONSTANT:                     return "RECORD_CONSTANT";
            case SECTION::RECORD_LENGTH:                       return "RECORD_LENGTH";
            case SECTION::BYTE_ORDER:                          return "BYTE_ORDER";
            case SECTION::ORIGINAL_HISTORIES:                  return "ORIG_HISTORIES";
            case SECTION::PARTICLES:                           return "PARTICLES";
            case SECTION::PHOTONS:                             return "PHOTONS";
            case SECTION::ELECTRONS:                           return "ELECTRONS";
            case SECTION::POSITRONS:                           return "POSITRONS";
            case SECTION::NEUTRONS:                            return "NEUTRONS";
            case SECTION::PROTONS:                             return "PROTONS";            
            case SECTION::TRANSPORT_PARAMETERS:                return "TRANSPORT_PARAMETERS";
            case SECTION::MACHINE_TYPE:                        return "MACHINE_TYPE";
            case SECTION::MONTE_CARLO_CODE_VERSION:            return "MONTE_CARLO_CODE_VERSION";
            case SECTION::GLOBAL_PHOTON_ENERGY_CUTOFF:         return "GLOBAL_PHOTON_ENERGY_CUTOFF";
            case SECTION::GLOBAL_PARTICLE_ENERGY_CUTOFF:       return "GLOBAL_PARTICLE_ENERGY_CUTOFF";
            case SECTION::COORDINATE_SYSTEM_DESCRIPTION:       return "COORDINATE_SYSTEM_DESCRIPTION";
            case SECTION::BEAM_NAME:                           return "BEAM_NAME";
            case SECTION::FIELD_SIZE:                          return "FIELD_SIZE";
            case SECTION::NOMINAL_SSD:                         return "NOMINAL_SSD";
            case SECTION::MC_INPUT_FILENAME:                   return "MC_INPUT_FILENAME";
            case SECTION::VARIANCE_REDUCTION_TECHNIQUES:       return "VARIANCE_REDUCTION_TECHNIQUES";
            case SECTION::INITIAL_SOURCE_DESCRIPTION:          return "INITIAL_SOURCE_DESCRIPTION";
            case SECTION::PUBLISHED_REFERENCE:                 return "PUBLISHED_REFERENCE";
            case SECTION::AUTHORS:                             return "AUTHORS";
            case SECTION::INSTITUTION:                         return "INSTITUTION";
            case SECTION::LINK_VALIDATION:                     return "LINK_VALIDATION";
            case SECTION::ADDITIONAL_NOTES:                    return "ADDITIONAL_NOTES";
            case SECTION::STATISTICAL_INFORMATION_PARTICLES:   return "STATISTICAL_INFORMATION_PARTICLES";
            case SECTION::STATISTICAL_INFORMATION_GEOMETRY:    return "STATISTICAL_INFORMATION_GEOMETRY";
            default:                                           return "UNKNOWN";
        }
    }

    constexpr IAEAHeader::SECTION IAEAHeader::getSectionFromString(const std::string & sectionTitle)
    {
        if (sectionTitle == "IAEA_INDEX") return IAEAHeader::SECTION::IAEA_INDEX;
        if (sectionTitle == "TITLE") return IAEAHeader::SECTION::TITLE;
        if (sectionTitle == "FILE_TYPE") return IAEAHeader::SECTION::FILE_TYPE;
        if (sectionTitle == "CHECKSUM") return IAEAHeader::SECTION::CHECKSUM;
        if (sectionTitle == "RECORD_CONTENTS") return IAEAHeader::SECTION::RECORD_CONTENTS;
        if (sectionTitle == "RECORD_CONSTANT") return IAEAHeader::SECTION::RECORD_CONSTANT;
        if (sectionTitle == "RECORD_LENGTH") return IAEAHeader::SECTION::RECORD_LENGTH;
        if (sectionTitle == "BYTE_ORDER") return IAEAHeader::SECTION::BYTE_ORDER;
        if (sectionTitle == "ORIG_HISTORIES") return IAEAHeader::SECTION::ORIGINAL_HISTORIES;
        if (sectionTitle == "PARTICLES") return IAEAHeader::SECTION::PARTICLES;
        if (sectionTitle == "PHOTONS") return IAEAHeader::SECTION::PHOTONS;
        if (sectionTitle == "ELECTRONS") return IAEAHeader::SECTION::ELECTRONS;
        if (sectionTitle == "POSITRONS") return IAEAHeader::SECTION::POSITRONS;
        if (sectionTitle == "NEUTRONS") return IAEAHeader::SECTION::NEUTRONS;
        if (sectionTitle == "PROTONS") return IAEAHeader::SECTION::PROTONS;
        if (sectionTitle == "TRANSPORT_PARAMETERS") return IAEAHeader::SECTION::TRANSPORT_PARAMETERS;
        if (sectionTitle == "MACHINE_TYPE") return IAEAHeader::SECTION::MACHINE_TYPE;
        if (sectionTitle == "MONTE_CARLO_CODE_VERSION") return IAEAHeader::SECTION::MONTE_CARLO_CODE_VERSION;
        if (sectionTitle == "GLOBAL_PHOTON_ENERGY_CUTOFF") return IAEAHeader::SECTION::GLOBAL_PHOTON_ENERGY_CUTOFF;
        if (sectionTitle == "GLOBAL_PARTICLE_ENERGY_CUTOFF") return IAEAHeader::SECTION::GLOBAL_PARTICLE_ENERGY_CUTOFF;
        if (sectionTitle == "COORDINATE_SYSTEM_DESCRIPTION") return IAEAHeader::SECTION::COORDINATE_SYSTEM_DESCRIPTION;
        if (sectionTitle == "BEAM_NAME") return IAEAHeader::SECTION::BEAM_NAME;
        if (sectionTitle == "FIELD_SIZE") return IAEAHeader::SECTION::FIELD_SIZE;
        if (sectionTitle == "NOMINAL_SSD") return IAEAHeader::SECTION::NOMINAL_SSD;
        if (sectionTitle == "MC_INPUT_FILENAME") return IAEAHeader::SECTION::MC_INPUT_FILENAME;
        if (sectionTitle == "VARIANCE_REDUCTION_TECHNIQUES") return IAEAHeader::SECTION::VARIANCE_REDUCTION_TECHNIQUES;
        if (sectionTitle == "INITIAL_SOURCE_DESCRIPTION") return IAEAHeader::SECTION::INITIAL_SOURCE_DESCRIPTION;
        if (sectionTitle == "PUBLISHED_REFERENCE") return IAEAHeader::SECTION::PUBLISHED_REFERENCE;
        if (sectionTitle == "AUTHORS") return IAEAHeader::SECTION::AUTHORS;
        if (sectionTitle == "INSTITUTION") return IAEAHeader::SECTION::INSTITUTION;
        if (sectionTitle == "LINK_VALIDATION") return IAEAHeader::SECTION::LINK_VALIDATION;
        if (sectionTitle == "ADDITIONAL_NOTES") return IAEAHeader::SECTION::ADDITIONAL_NOTES;
        if (sectionTitle == "STATISTICAL_INFORMATION_PARTICLES") return IAEAHeader::SECTION::STATISTICAL_INFORMATION_PARTICLES;
        if (sectionTitle == "STATISTICAL_INFORMATION_GEOMETRY") return IAEAHeader::SECTION::STATISTICAL_INFORMATION_GEOMETRY;
        return IAEAHeader::SECTION::CUSTOM_SECTION;
    }

    
    constexpr FloatPropertyType IAEAHeader::translateExtraFloatType(IAEAHeader::EXTRA_FLOAT_TYPE type)
    {
        switch (type) {
            case EXTRA_FLOAT_TYPE::USER_DEFINED_GENERIC_TYPE: return FloatPropertyType::CUSTOM;
            case EXTRA_FLOAT_TYPE::XLAST: return FloatPropertyType::XLAST;
            case EXTRA_FLOAT_TYPE::YLAST: return FloatPropertyType::YLAST;
            case EXTRA_FLOAT_TYPE::ZLAST: return FloatPropertyType::ZLAST;
            default: return FloatPropertyType::INVALID;
        }
    }    

    constexpr IntPropertyType IAEAHeader::translateExtraLongType(IAEAHeader::EXTRA_LONG_TYPE type)
    {
        switch (type) {
            case EXTRA_LONG_TYPE::USER_DEFINED_GENERIC_TYPE: return IntPropertyType::CUSTOM;
            case EXTRA_LONG_TYPE::INCREMENTAL_HISTORY_NUMBER: return IntPropertyType::INCREMENTAL_HISTORY_NUMBER;
            case EXTRA_LONG_TYPE::EGS_LATCH: return IntPropertyType::EGS_LATCH;
            case EXTRA_LONG_TYPE::PENELOPE_ILB5: return IntPropertyType::PENELOPE_ILB5;
            case EXTRA_LONG_TYPE::PENELOPE_ILB4: return IntPropertyType::PENELOPE_ILB4;
            case EXTRA_LONG_TYPE::PENELOPE_ILB3: return IntPropertyType::PENELOPE_ILB3;
            case EXTRA_LONG_TYPE::PENELOPE_ILB2: return IntPropertyType::PENELOPE_ILB2;
            case EXTRA_LONG_TYPE::PENELOPE_ILB1: return IntPropertyType::PENELOPE_ILB1;
            default: return IntPropertyType::INVALID;
        }
    }

} // namespace ParticleZoo