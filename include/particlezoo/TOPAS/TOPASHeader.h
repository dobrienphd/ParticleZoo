#pragma once

#include <string>

#include "particlezoo/Particle.h"

namespace ParticleZoo::TOPASphspFile
{
    /**
     * @brief TOPAS phase space file format types
     * 
     * Defines the three supported TOPAS formats:
     * - ASCII: Human-readable text format with configurable columns
     * - BINARY: Efficient binary format with configurable particle details
     * - LIMITED: Binary format limited to certain particle details
     */
    enum class TOPASFormat { ASCII, BINARY, LIMITED };

    /**
     * @brief Header for TOPAS phase space files
     * 
     * This class handles reading, writing, and managing header information
     * for TOPAS format phase space files. It manages file metadata, particle
     * statistics, column definitions, and format-specific configurations.
     * TOPAS files use separate .header and .phsp files.
     */
    class Header
    {
        /// @brief Fixed record length for LIMITED format in bytes
        constexpr static int LIMITED_RECORD_LENGTH = 29;

        public:
            
            /**
             * @brief Data types supported in TOPAS columns
             * 
             * Defines the fundamental data types that can be stored
             * in TOPAS phase space file columns.
             */
            enum class DataType {
                STRING,    ///< Variable-length string data
                BOOLEAN,   ///< Boolean true/false values
                INT8,      ///< 8-bit signed integer
                INT32,     ///< 32-bit signed integer
                FLOAT32,   ///< 32-bit floating-point
                FLOAT64    ///< 64-bit floating-point
            };

            /**
             * @brief Column types supported in TOPAS phase space files
             * 
             * Defines all possible column types that can appear in TOPAS files,
             * from basic particle properties to extended tracking information.
             */
            enum class ColumnType {
                POSITION_X,                       ///< X coordinate position
                POSITION_Y,                       ///< Y coordinate position
                POSITION_Z,                       ///< Z coordinate position
                DIRECTION_COSINE_X,               ///< X direction cosine
                DIRECTION_COSINE_Y,               ///< Y direction cosine
                ENERGY,                           ///< Kinetic energy
                WEIGHT,                           ///< Particle statistical weight
                PARTICLE_TYPE,                    ///< PDG particle type code
                DIRECTION_COSINE_Z_SIGN,          ///< Sign of Z direction cosine
                NEW_HISTORY_FLAG,                 ///< First particle from history flag
                TOPAS_TIME,                       ///< TOPAS simulation time
                TIME_OF_FLIGHT,                   ///< Particle time of flight
                RUN_ID,                           ///< Simulation run identifier
                EVENT_ID,                         ///< Event identifier within run
                TRACK_ID,                         ///< Track identifier within event
                PARENT_ID,                        ///< Parent track identifier
                CHARGE,                           ///< Particle charge
                CREATOR_PROCESS,                  ///< Physics process that created particle
                INITIAL_KINETIC_ENERGY,           ///< Initial kinetic energy at creation
                VERTEX_POSITION_X,                ///< X coordinate of creation vertex
                VERTEX_POSITION_Y,                ///< Y coordinate of creation vertex
                VERTEX_POSITION_Z,                ///< Z coordinate of creation vertex
                INITIAL_DIRECTION_COSINE_X,       ///< Initial X direction cosine
                INITIAL_DIRECTION_COSINE_Y,       ///< Initial Y direction cosine
                INITIAL_DIRECTION_COSINE_Z,       ///< Initial Z direction cosine
                SEED_PART_1,                      ///< Random seed component 1
                SEED_PART_2,                      ///< Random seed component 2
                SEED_PART_3,                      ///< Random seed component 3
                SEED_PART_4                       ///< Random seed component 4
            };

            /**
             * @brief Column definition for TOPAS phase space files
             * 
             * Describes a single column in the phase space file including
             * its type, data format, and display name.
             */
            struct DataColumn
            {
                ColumnType  columnType_;  ///< Semantic type of the column
                DataType    valueType_;   ///< Data storage type
                std::string name_;        ///< Human-readable column name

                /**
                 * @brief Construct column from name string
                 * @param name Column name (determines type automatically)
                 */
                DataColumn(std::string_view name)
                    : columnType_(getColumnType(name)), valueType_(getDataType(columnType_)), name_(name) {}

                /**
                 * @brief Construct column from type (uses default name)
                 * @param columnType Column type
                 */
                DataColumn(ColumnType columnType)
                    : columnType_(columnType), valueType_(getDataType(columnType)), name_(getColumnName(columnType)) {}

                /**
                 * @brief Construct column with specific data type
                 * @param columnType Column type
                 * @param valueType Data storage type (overrides default)
                 */
                DataColumn(ColumnType columnType, DataType valueType)
                    : columnType_(columnType), valueType_(valueType), name_(getColumnName(columnType)) {}

                /**
                 * @brief Construct column with all parameters specified
                 * @param columnType Column type
                 * @param valueType Data storage type
                 * @param name Custom column name
                 */
                DataColumn(ColumnType columnType, DataType valueType, std::string_view name)
                    : columnType_(columnType), valueType_(valueType), name_(name) {}

                /**
                 * @brief Get the storage size of this column's data type
                 * @return Size in bytes (0 for variable-length strings)
                 */
                inline std::size_t sizeOf() const {
                    switch (valueType_) {
                        case DataType::STRING:   return 0; // Variable length
                        case DataType::BOOLEAN:  return sizeof(bool);
                        case DataType::INT8:    return sizeof(std::int8_t);
                        case DataType::INT32:    return sizeof(std::int32_t);
                        case DataType::FLOAT32:  return sizeof(float);
                        case DataType::FLOAT64:  return sizeof(double);
                        default: throw std::runtime_error("Unsupported value type.");
                    }
                }
                
                /**
                 * @brief Get the default data type for a column type
                 * @param columnType Column type to query
                 * @return Default DataType for storage
                 * @throws std::runtime_error if column type is unknown
                 */
                static constexpr DataType getDataType(ColumnType columnType) {
                    switch (columnType) {
                        // Floating-point columns
                        case ColumnType::POSITION_X:
                        case ColumnType::POSITION_Y:
                        case ColumnType::POSITION_Z:
                        case ColumnType::DIRECTION_COSINE_X:
                        case ColumnType::DIRECTION_COSINE_Y:
                        case ColumnType::ENERGY:
                        case ColumnType::WEIGHT:
                        case ColumnType::TOPAS_TIME:
                        case ColumnType::TIME_OF_FLIGHT:
                        case ColumnType::CHARGE:
                        case ColumnType::INITIAL_KINETIC_ENERGY:
                        case ColumnType::VERTEX_POSITION_X:
                        case ColumnType::VERTEX_POSITION_Y:
                        case ColumnType::VERTEX_POSITION_Z:
                        case ColumnType::INITIAL_DIRECTION_COSINE_X:
                        case ColumnType::INITIAL_DIRECTION_COSINE_Y:
                        case ColumnType::INITIAL_DIRECTION_COSINE_Z:
                            return DataType::FLOAT32;
                        // Boolean columns
                        case ColumnType::DIRECTION_COSINE_Z_SIGN:
                        case ColumnType::NEW_HISTORY_FLAG:
                            return DataType::BOOLEAN;
                        // String columns
                        case ColumnType::CREATOR_PROCESS:
                            return DataType::STRING;
                        // Integer columns
                        case ColumnType::PARTICLE_TYPE:
                        case ColumnType::RUN_ID:
                        case ColumnType::EVENT_ID:
                        case ColumnType::TRACK_ID:
                        case ColumnType::PARENT_ID:
                        case ColumnType::SEED_PART_1:
                        case ColumnType::SEED_PART_2:
                        case ColumnType::SEED_PART_3:
                        case ColumnType::SEED_PART_4:
                            return DataType::INT32;
                    }
                    throw std::runtime_error("Unknown column type.");
                }

                /**
                 * @brief Get the string represented name for a column type
                 * @param columnType Column type to query
                 * @return Human-readable column name with units
                 * @throws std::runtime_error if column type is unknown
                 */
                static constexpr std::string_view getColumnName(ColumnType columnType) {
                    switch (columnType) {
                        case ColumnType::POSITION_X: return "Position X [cm]";
                        case ColumnType::POSITION_Y: return "Position Y [cm]";
                        case ColumnType::POSITION_Z: return "Position Z [cm]";
                        case ColumnType::DIRECTION_COSINE_X: return "Direction Cosine X";
                        case ColumnType::DIRECTION_COSINE_Y: return "Direction Cosine Y";
                        case ColumnType::ENERGY: return "Energy [MeV]";
                        case ColumnType::WEIGHT: return "Weight";
                        case ColumnType::PARTICLE_TYPE: return "Particle Type (in PDG Format)";
                        case ColumnType::DIRECTION_COSINE_Z_SIGN: return "Flag to tell if Third Direction Cosine is Negative (1 means true)";
                        case ColumnType::NEW_HISTORY_FLAG: return "Flag to tell if this is the First Scored Particle from this History (1 means true)";
                        case ColumnType::TOPAS_TIME: return "TOPAS Time [s]";
                        case ColumnType::TIME_OF_FLIGHT: return "Time of Flight [ns]";
                        case ColumnType::RUN_ID: return "Run ID";
                        case ColumnType::EVENT_ID: return "Event ID";
                        case ColumnType::TRACK_ID: return "Track ID";
                        case ColumnType::PARENT_ID: return "Parent ID";
                        case ColumnType::CHARGE: return "Charge";
                        case ColumnType::CREATOR_PROCESS: return "Creator Process Name";
                        case ColumnType::INITIAL_KINETIC_ENERGY: return "Initial Kinetic Energy [MeV]";
                        case ColumnType::VERTEX_POSITION_X: return "Vertex Position X [cm]";
                        case ColumnType::VERTEX_POSITION_Y: return "Vertex Position Y [cm]";
                        case ColumnType::VERTEX_POSITION_Z: return "Vertex Position Z [cm]";
                        case ColumnType::INITIAL_DIRECTION_COSINE_X: return "Initial Direction Cosine X";
                        case ColumnType::INITIAL_DIRECTION_COSINE_Y: return "Initial Direction Cosine Y";
                        case ColumnType::INITIAL_DIRECTION_COSINE_Z: return "Initial Direction Cosine Z";
                        case ColumnType::SEED_PART_1: return "Seed Part 1";
                        case ColumnType::SEED_PART_2: return "Seed Part 2";
                        case ColumnType::SEED_PART_3: return "Seed Part 3";
                        case ColumnType::SEED_PART_4: return "Seed Part 4";
                    }
                    throw std::runtime_error("Unknown column type.");
                }

                /**
                 * @brief Parse column type from name
                 * @param name Column name to parse
                 * @return Corresponding ColumnType
                 * @throws std::runtime_error if name is not recognized
                 */
                static constexpr ColumnType getColumnType(std::string_view name) {
                    if (name == "Position X [cm]") return ColumnType::POSITION_X;
                    if (name == "Position Y [cm]") return ColumnType::POSITION_Y;
                    if (name == "Position Z [cm]") return ColumnType::POSITION_Z;
                    if (name == "Direction Cosine X") return ColumnType::DIRECTION_COSINE_X;
                    if (name == "Direction Cosine Y") return ColumnType::DIRECTION_COSINE_Y;
                    if (name == "Energy [MeV]") return ColumnType::ENERGY;
                    if (name == "Weight") return ColumnType::WEIGHT;
                    if (name == "Particle Type (in PDG Format)") return ColumnType::PARTICLE_TYPE;
                    if (name == "Flag to tell if Third Direction Cosine is Negative (1 means true)") return ColumnType::DIRECTION_COSINE_Z_SIGN;
                    if (name == "Flag to tell if this is the First Scored Particle from this History (1 means true)") return ColumnType::NEW_HISTORY_FLAG;
                    if (name == "TOPAS Time [s]") return ColumnType::TOPAS_TIME;
                    if (name == "Time of Flight [ns]") return ColumnType::TIME_OF_FLIGHT;
                    if (name == "Run ID") return ColumnType::RUN_ID;
                    if (name == "Event ID") return ColumnType::EVENT_ID;
                    if (name == "Track ID") return ColumnType::TRACK_ID;
                    if (name == "Parent ID") return ColumnType::PARENT_ID;
                    if (name == "Charge") return ColumnType::CHARGE;
                    if (name == "Creator Process Name") return ColumnType::CREATOR_PROCESS;
                    if (name == "Initial Kinetic Energy [MeV]") return ColumnType::INITIAL_KINETIC_ENERGY;
                    if (name == "Vertex Position X [cm]") return ColumnType::VERTEX_POSITION_X;
                    if (name == "Vertex Position Y [cm]") return ColumnType::VERTEX_POSITION_Y;
                    if (name == "Vertex Position Z [cm]") return ColumnType::VERTEX_POSITION_Z;
                    if (name == "Initial Direction Cosine X") return ColumnType::INITIAL_DIRECTION_COSINE_X;
                    if (name == "Initial Direction Cosine Y") return ColumnType::INITIAL_DIRECTION_COSINE_Y;
                    if (name == "Initial Direction Cosine Z") return ColumnType::INITIAL_DIRECTION_COSINE_Z;
                    if (name == "Seed Part 1") return ColumnType::SEED_PART_1;
                    if (name == "Seed Part 2") return ColumnType::SEED_PART_2;
                    if (name == "Seed Part 3") return ColumnType::SEED_PART_3;
                    if (name == "Seed Part 4") return ColumnType::SEED_PART_4;
                    throw std::runtime_error("Unknown column name: "+ std::string(name));
                }
            };

            /**
             * @brief Statistics tracking for individual particle types for TOPAS phase space files
             */
            struct ParticleStats
            {
                std::uint64_t count_{};                                           ///< Number of particles of this type
                double minKineticEnergy_ = std::numeric_limits<double>::max();    ///< Minimum kinetic energy encountered
                double maxKineticEnergy_ = 0;                                     ///< Maximum kinetic energy encountered
            };

            /// @brief Type alias for particle statistics table
            using ParticleStatsTable = std::unordered_map<ParticleType, ParticleStats>;

            // Constructors

            /**
             * @brief Construct header by reading from existing TOPAS file
             * 
             * @param fileName Path to TOPAS phase space file (.phsp or .header)
             * @throws std::runtime_error if file cannot be read or is invalid
             */
            Header(const std::string & fileName);

            /**
             * @brief Construct header for writing new TOPAS file
             * 
             * @param fileName Path for new TOPAS phase space file
             * @param formatType Format to write (ASCII, BINARY, or LIMITED)
             */
            Header(const std::string & fileName, TOPASFormat formatType);

            // Getters

            /**
             * @brief Get the TOPAS format type
             * @return TOPASFormat enum value
             */
            TOPASFormat    getTOPASFormat() const;

            /**
             * @brief Get the human-readable format name
             * @return Format name as string (e.g., "TOPAS BINARY")
             */
            std::string    getTOPASFormatName() const;

            /**
             * @brief Get the length of each particle record in bytes
             * @return Record length based on format and column configuration
             */
            std::size_t    getRecordLength() const;

            /**
             * @brief Get the number of original simulation histories
             * @return Count of primary histories used in the simulation
             */
            std::uint64_t  getNumberOfOriginalHistories() const;

            /**
             * @brief Get the number of histories explicitly represented by particles in the phase space
             * @return Count of histories that produced particles that reached the phase space
             */
            std::uint64_t  getNumberOfRepresentedHistories() const;

            /**
             * @brief Get the total number of particles in the phase space
             * @return Total particle count across all types
             */
            std::uint64_t  getNumberOfParticles() const;

            /**
             * @brief Get the number of particles of a specific type
             * @param type Particle type to query
             * @return Number of particles of the specified type
             */
            std::uint64_t  getNumberOfParticlesOfType(ParticleType type) const;

            /**
             * @brief Get the minimum kinetic energy for particles of a specific type
             * @param type Particle type to query
             * @return Minimum kinetic energy for the particle type
             */
            double getMinKineticEnergyOfType(ParticleType type) const;

            /**
             * @brief Get the maximum kinetic energy for particles of a specific type
             * @param type Particle type to query
             * @return Maximum kinetic energy for the particle type
             */
            double getMaxKineticEnergyOfType(ParticleType type) const;

            /**
             * @brief Get the column definitions for this phase space
             * @return Vector of DataColumn objects describing the file structure
             */
            const std::vector<DataColumn> & getColumnTypes() const;

            // Setters and modification methods

            /**
             * @brief Update particle statistics with a new particle
             * @param particle Particle to include in statistics calculations
             */
            void countParticleStats(const Particle & particle);

            /**
             * @brief Add a new column type to the phase space format
             * @param columnType Type of column to add
             */
            void addColumnType(ColumnType columnType);

            /**
             * @brief Set the number of original simulation histories
             * @param originalHistories Count of primary histories
             */
            void setNumberOfOriginalHistories(std::uint64_t originalHistories);

            /**
             * @brief Write the complete header file
             * 
             * Writes the header information to the .header file with
             * format-specific structure and particle statistics.
             * 
             * @throws std::runtime_error if file cannot be written
             */
            void writeHeader();

            /**
             * @brief Get format name from enum value
             * @param format TOPAS format type
             * @return Human-readable format name
             */
            static std::string getTOPASFormatName(TOPASFormat format);

        private:
            // Format-specific header writing methods
            void writeHeader_ASCII(std::ofstream & file) const;
            void writeHeader_Binary(std::ofstream & file) const;
            void writeHeader_Limited(std::ofstream & file) const;

            void writeSuffix(std::ofstream & file) const;

            // Header reading methods
            void readHeader();
            void readHeader_Limited(std::ifstream & headerFile);
            void readHeader_Standard(std::ifstream & headerFile);
            void readColumns_Binary(std::ifstream & headerFile);
            void readColumns_ASCII(std::ifstream & headerFile);

            void setFileNames(const std::string & fileName);

            // Member variables
            TOPASFormat             formatType_;                   ///< Format type of this phase space file
            std::string             headerFileName_;               ///< Path to the .header file
            std::string             phspFileName_;                 ///< Path to the .phsp data file
            std::uint64_t           numberOfOriginalHistories_;    ///< Number of original simulation histories
            std::uint64_t           numberOfRepresentedHistories_; ///< Number of histories that reached phase space
            std::uint64_t           numberOfParticles_;            ///< Total number of particles
            ParticleStatsTable      particleStatsTable_;           ///< Statistics by particle type
            std::vector<DataColumn> columnTypes_;                  ///< Column definitions for the file format
    };

    // Inline implementations

    inline std::uint64_t Header::getNumberOfOriginalHistories() const { return numberOfOriginalHistories_; }
    inline std::uint64_t Header::getNumberOfRepresentedHistories() const { return numberOfRepresentedHistories_; }
    inline std::uint64_t Header::getNumberOfParticles() const { return numberOfParticles_; }
    inline TOPASFormat   Header::getTOPASFormat() const { return formatType_; }

    inline std::string Header::getTOPASFormatName() const { return getTOPASFormatName(formatType_); }
    inline std::string Header::getTOPASFormatName(TOPASFormat format) {
        switch (format) {
            case TOPASFormat::ASCII: return "TOPAS ASCII";
            case TOPASFormat::BINARY: return "TOPAS BINARY";
            case TOPASFormat::LIMITED: return "TOPAS LIMITED";
            default: throw std::runtime_error("Unknown TOPAS format.");
        }
    }

    inline const std::vector<Header::DataColumn> & Header::getColumnTypes() const { return columnTypes_; }
    inline void Header::setNumberOfOriginalHistories(std::uint64_t originalHistories) { numberOfOriginalHistories_ = originalHistories; }

    inline std::uint64_t Header::getNumberOfParticlesOfType(ParticleType type) const {
        auto it = particleStatsTable_.find(type);
        return (it != particleStatsTable_.end()) ? it->second.count_ : 0;
    }

    inline double Header::getMinKineticEnergyOfType(ParticleType type) const {
        auto it = particleStatsTable_.find(type);
        return (it != particleStatsTable_.end()) ? it->second.minKineticEnergy_ : 0.0;
    }

    inline double Header::getMaxKineticEnergyOfType(ParticleType type) const {
        auto it = particleStatsTable_.find(type);
        return (it != particleStatsTable_.end()) ? it->second.maxKineticEnergy_ : 0.0;
    }

    inline void Header::countParticleStats(const Particle & particle) {
        ParticleType particleType = particle.getType();
        if (particleType == ParticleType::Unsupported) return;

        // Capture original history details even for pseudo particles
        if (particle.isNewHistory()) {
            if (particle.hasIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER)) {
                numberOfOriginalHistories_ += particle.getIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER);
            } else {
                numberOfOriginalHistories_++;
            }
        }

        // Don't count other statistics for pseudo particles
        if (particleType == ParticleType::PseudoParticle) return;

        if (particle.isNewHistory()) numberOfRepresentedHistories_++;
        auto & stats = particleStatsTable_[particleType];
        stats.count_++;
        double energy = particle.getKineticEnergy();
        stats.minKineticEnergy_ = std::min(energy, stats.minKineticEnergy_);
        stats.maxKineticEnergy_ = std::max(energy, stats.maxKineticEnergy_);

        numberOfParticles_++;
    }

} // namespace ParticleZoo::TOPASphspFile