#pragma once

#include <string>

#include "particlezoo/Particle.h"

namespace ParticleZoo::TOPASphspFile
{
    enum class TOPASFormat { ASCII, BINARY, LIMITED };

    class Header
    {
        constexpr static int LIMITED_RECORD_LENGTH = 29; // Fixed length for limited format

        public:
            
            enum class DataType {
                STRING,
                BOOLEAN,
                INT8,
                INT32,
                FLOAT32,
                FLOAT64
            };

            enum class ColumnType {
                POSITION_X,
                POSITION_Y,
                POSITION_Z,
                DIRECTION_COSINE_X,
                DIRECTION_COSINE_Y,
                ENERGY,
                WEIGHT,
                PARTICLE_TYPE,
                DIRECTION_COSINE_Z_SIGN,
                NEW_HISTORY_FLAG,
                TOPAS_TIME,
                TIME_OF_FLIGHT,
                RUN_ID,
                EVENT_ID,
                TRACK_ID,
                PARENT_ID,
                CHARGE,
                CREATOR_PROCESS,
                INITIAL_KINETIC_ENERGY,
                VERTEX_POSITION_X,
                VERTEX_POSITION_Y,
                VERTEX_POSITION_Z,
                INITIAL_DIRECTION_COSINE_X,
                INITIAL_DIRECTION_COSINE_Y,
                INITIAL_DIRECTION_COSINE_Z,
                SEED_PART_1,
                SEED_PART_2,
                SEED_PART_3,
                SEED_PART_4
            };

            struct DataColumn
            {
                ColumnType columnType_;
                DataType valueType_;
                std::string name_;

                DataColumn(std::string_view name)
                    : columnType_(getColumnType(name)), valueType_(getDataType(columnType_)), name_(name) {}

                DataColumn(ColumnType columnType)
                    : columnType_(columnType), valueType_(getDataType(columnType)), name_(getColumnName(columnType)) {}

                DataColumn(ColumnType columnType, DataType valueType)
                    : columnType_(columnType), valueType_(valueType), name_(getColumnName(columnType)) {}

                DataColumn(ColumnType columnType, DataType valueType, std::string_view name)
                    : columnType_(columnType), valueType_(valueType), name_(name) {}

                inline std::size_t sizeOf() const {
                    switch (valueType_) {
                        case DataType::STRING:   return 0;
                        case DataType::BOOLEAN:  return sizeof(bool);
                        case DataType::INT8:    return sizeof(std::int8_t);
                        case DataType::INT32:    return sizeof(std::int32_t);
                        case DataType::FLOAT32:  return sizeof(float);
                        case DataType::FLOAT64:  return sizeof(double);
                        default: throw std::runtime_error("Unsupported value type.");
                    }
                }
                
                static constexpr DataType getDataType(ColumnType columnType) {
                    switch (columnType) {
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
                        case ColumnType::DIRECTION_COSINE_Z_SIGN:
                        case ColumnType::NEW_HISTORY_FLAG:
                            return DataType::BOOLEAN;
                        case ColumnType::CREATOR_PROCESS:
                            return DataType::STRING;
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

            struct ParticleStats
            {
                std::uint64_t count_{};
                double minKineticEnergy_ = std::numeric_limits<double>::max();
                double maxKineticEnergy_ = 0;
            };

            typedef std::unordered_map<ParticleType, ParticleStats> ParticleStatsTable;

            Header(const std::string & fileName); // reading from existing file
            Header(const std::string & fileName, TOPASFormat formatType); // writing to new file

            TOPASFormat    getTOPASFormat() const;
            std::size_t    getRecordLength() const;
            std::uint64_t  getNumberOfOriginalHistories() const;
            std::uint64_t  getNumberOfRepresentedHistories() const;
            std::uint64_t  getNumberOfParticles() const;
            std::uint64_t  getNumberOfParticlesOfType(ParticleType type) const;
            double getMinKineticEnergyOfType(ParticleType type) const;
            double getMaxKineticEnergyOfType(ParticleType type) const;
            const std::vector<DataColumn> & getColumnTypes() const;

            void countParticleStats(const Particle & particle);
            void addColumnType(ColumnType columnType);
            void setNumberOfOriginalHistories(std::uint64_t originalHistories);

            void writeHeader();

        private:
            void writeHeader_ASCII(std::ofstream & file) const;
            void writeHeader_Binary(std::ofstream & file) const;
            void writeHeader_Limited(std::ofstream & file) const;

            void writeSuffix(std::ofstream & file) const;

            void readHeader();
            void readHeader_Limited(std::ifstream & headerFile);
            void readHeader_Standard(std::ifstream & headerFile);
            void readColumns_Binary(std::ifstream & headerFile);
            void readColumns_ASCII(std::ifstream & headerFile);

            void setFileNames(const std::string & fileName);

            TOPASFormat             formatType_;
            std::string             headerFileName_;
            std::string             phspFileName_;
            std::uint64_t           numberOfOriginalHistories_;
            std::uint64_t           numberOfRepresentedHistories_;
            std::uint64_t           numberOfParticles_;
            ParticleStatsTable      particleStatsTable_;
            std::vector<DataColumn> columnTypes_;
    };

    inline std::uint64_t Header::getNumberOfOriginalHistories() const { return numberOfOriginalHistories_; }
    inline std::uint64_t Header::getNumberOfRepresentedHistories() const { return numberOfRepresentedHistories_; }
    inline std::uint64_t Header::getNumberOfParticles() const { return numberOfParticles_; }
    inline TOPASFormat    Header::getTOPASFormat() const { return formatType_; }
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
        // Capture original history details even for pseudo particles
        if (particle.isNewHistory()) {
            if (particle.hasIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER)) {
                numberOfOriginalHistories_ += particle.getIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER);
            } else {
                numberOfOriginalHistories_++;
            }
        }

        // Don't count other statistics for pseudo particles
        if (particle.getType() == ParticleType::Unsupported || particle.getWeight() <= 0) return;

        if (particle.isNewHistory()) numberOfRepresentedHistories_++;
        ParticleType type = particle.getType();
        auto & stats = particleStatsTable_[type];
        stats.count_++;
        double energy = particle.getKineticEnergy();
        stats.minKineticEnergy_ = std::min(energy, stats.minKineticEnergy_);
        stats.maxKineticEnergy_ = std::max(energy, stats.maxKineticEnergy_);

        numberOfParticles_++;
    }

} // namespace ParticleZoo::TOPASphsp