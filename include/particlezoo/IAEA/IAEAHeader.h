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
    class IAEAHeader
    {
        struct ParticleStats
        {
            std::uint64_t count_;
            double weightSum_;
            float minWeight_;
            float maxWeight_;
            double energySum_;
            float minEnergy_;
            float maxEnergy_;
            ParticleStats();
        };

        using SectionTable = std::unordered_map<std::string, std::string>;
        using ParticleCountTable = std::unordered_map<ParticleType, std::uint64_t>;
        using ParticleStatsTable = std::unordered_map<ParticleType, ParticleStats>;

        public:

            enum class FileType {
                PHSP_FILE = 0,
                PHSP_GENERATOR = 1
            };

            enum class SECTION {
                IAEA_INDEX,
                TITLE,
                FILE_TYPE,
                CHECKSUM,
                RECORD_CONTENTS,
                RECORD_CONSTANT,
                RECORD_LENGTH,
                BYTE_ORDER,
                ORIGINAL_HISTORIES,
                PARTICLES,
                PHOTONS,
                ELECTRONS,
                POSITRONS,
                NEUTRONS,
                PROTONS,
                TRANSPORT_PARAMETERS,
                MACHINE_TYPE,
                MONTE_CARLO_CODE_VERSION,
                GLOBAL_PHOTON_ENERGY_CUTOFF,
                GLOBAL_PARTICLE_ENERGY_CUTOFF,
                COORDINATE_SYSTEM_DESCRIPTION,
                BEAM_NAME,
                FIELD_SIZE,
                NOMINAL_SSD,
                MC_INPUT_FILENAME,
                VARIANCE_REDUCTION_TECHNIQUES,
                INITIAL_SOURCE_DESCRIPTION,
                PUBLISHED_REFERENCE,
                AUTHORS,
                INSTITUTION,
                LINK_VALIDATION,
                ADDITIONAL_NOTES,
                STATISTICAL_INFORMATION_PARTICLES,
                STATISTICAL_INFORMATION_GEOMETRY,
                CUSTOM_SECTION
            };

            enum class EXTRA_LONG_TYPE {
                USER_DEFINED_GENERIC_TYPE = 0,
                INCREMENTAL_HISTORY_NUMBER = 1,
                EGS_LATCH = 2,
                PENELOPE_ILB5 = 3,
                PENELOPE_ILB4 = 4,
                PENELOPE_ILB3 = 5,
                PENELOPE_ILB2 = 6,
                PENELOPE_ILB1 = 7
            };

            enum class EXTRA_FLOAT_TYPE {
                USER_DEFINED_GENERIC_TYPE = 0,
                XLAST = 1,
                YLAST = 2,
                ZLAST = 3
            };

            IAEAHeader(const std::string &filePath, bool newFile = false);
            IAEAHeader(const IAEAHeader & other);

            void writeHeader();

            // Getters for IAEAHeader attributes
            std::string         getHeaderFilePath() const;
            std::string         getDataFilePath() const;
            std::string         getIAEAIndex() const; // sometimes represented with leading zeros, use string to preserve format
            const std::string & getTitle() const;
            FileType            getFileType() const;
            std::uint64_t       getChecksum() const;
            bool                xIsStored() const;
            bool                yIsStored() const;
            bool                zIsStored() const;
            bool                uIsStored() const;
            bool                vIsStored() const;
            bool                wIsStored() const;
            bool                weightIsStored() const;
            unsigned int        getNumberOfExtraFloats() const;
            unsigned int        getNumberOfExtraLongs() const;
            float               getConstantX() const;
            float               getConstantY() const;
            float               getConstantZ() const;
            float               getConstantU() const;
            float               getConstantV() const;
            float               getConstantW() const;
            float               getConstantWeight() const;
            EXTRA_FLOAT_TYPE    getExtraFloatType(unsigned int index) const;
            EXTRA_LONG_TYPE     getExtraLongType(unsigned int index) const;
            std::size_t         getRecordLength() const;
            ByteOrder           getByteOrder() const;
            std::uint64_t       getOriginalHistories() const;
            std::uint64_t       getNumberOfParticles() const;
            std::uint64_t       getNumberOfParticles(ParticleType particleType) const;
            const std::string   getSection(const std::string &sectionName) const;
            const std::string   getSection(SECTION section) const;

            // Getters for particle statistics
            float               getMinX() const;
            float               getMaxX() const;
            float               getMinY() const;
            float               getMaxY() const;
            float               getMinZ() const;
            float               getMaxZ() const;
            float               getMinWeight(ParticleType particleType) const;
            float               getMaxWeight(ParticleType particleType) const;
            float               getMinEnergy(ParticleType particleType) const;
            float               getMaxEnergy(ParticleType particleType) const;
            float               getMeanWeight(ParticleType particleType) const;
            float               getMeanEnergy(ParticleType particleType) const;
            float               getTotalWeight(ParticleType particleType) const;

            // Setters for IAEAHeader attributes
            void                setFilePath(const std::string &filePath);
            void                setIAEAIndex(const std::string &index);
            void                setTitle(const std::string &title);
            void                setFileType(FileType fileType);
            void                setChecksum(std::uint64_t checksum);
            void                setConstantX(float x);
            void                setConstantY(float y);
            void                setConstantZ(float z);
            void                setConstantU(float u);
            void                setConstantV(float v);
            void                setConstantW(float w);
            void                setConstantWeight(float weight);
            void                addExtraFloat(EXTRA_FLOAT_TYPE type);
            void                addExtraLong(EXTRA_LONG_TYPE type);
            bool                hasExtraFloat(EXTRA_FLOAT_TYPE type) const;
            bool                hasExtraLong(EXTRA_LONG_TYPE type) const;
            void                setRecordLength(std::size_t length);
            void                setOriginalHistories(std::uint64_t originalHistories);
            void                countParticleStats(const Particle & particle);
            void                setSection(const std::string &sectionName, const std::string &sectionValue);
            void                setSection(SECTION section, const std::string &sectionValue);

            // Setters for particle statistics
            void                setMinX(float minX);
            void                setMaxX(float maxX);
            void                setMinY(float minY);
            void                setMaxY(float maxY);
            void                setMinZ(float minZ);
            void                setMaxZ(float maxZ);
            void                setMinWeight(ParticleType particleType, float minWeight);
            void                setMaxWeight(ParticleType particleType, float maxWeight);
            void                setMinEnergy(ParticleType particleType, float minEnergy);
            void                setMaxEnergy(ParticleType particleType, float maxEnergy);
            void                setMeanEnergy(ParticleType particleType, float meanEnergy);
            void                setTotalWeight(ParticleType particleType, float totalWeight);

            bool                checksumIsValid() const;

            const static std::string DeterminePathToHeaderFile(const std::string &filename);
            constexpr static FloatPropertyType translateExtraFloatType(IAEAHeader::EXTRA_FLOAT_TYPE type);
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