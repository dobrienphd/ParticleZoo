
#include "particlezoo/TOPAS/TOPASHeader.h"

#include <fstream>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <algorithm>

#include "particlezoo/geant4/Geant4Particles.h"

namespace ParticleZoo::TOPASphspFile
{
    Header::Header(const std::string & fileName)
        : numberOfOriginalHistories_(0), numberOfRepresentedHistories_(0), numberOfParticles_(0)
    {
        setFileNames(fileName);
        readHeader();
    }

    Header::Header(const std::string & fileName, TOPASFormat formatType)
        : formatType_(formatType), numberOfOriginalHistories_(0), numberOfRepresentedHistories_(0), numberOfParticles_(0)
    {
        setFileNames(fileName);

        columnTypes_.emplace_back(ColumnType::POSITION_X);
        columnTypes_.emplace_back(ColumnType::POSITION_Y);
        columnTypes_.emplace_back(ColumnType::POSITION_Z);
        columnTypes_.emplace_back(ColumnType::DIRECTION_COSINE_X);
        columnTypes_.emplace_back(ColumnType::DIRECTION_COSINE_Y);
        columnTypes_.emplace_back(ColumnType::ENERGY);
        columnTypes_.emplace_back(ColumnType::WEIGHT);
        columnTypes_.emplace_back(ColumnType::PARTICLE_TYPE);
        columnTypes_.emplace_back(ColumnType::DIRECTION_COSINE_Z_SIGN);
        columnTypes_.emplace_back(ColumnType::NEW_HISTORY_FLAG);
    }

    inline void Header::setFileNames(const std::string & fileName)
    {
        // TOPAS phase space files are split between a .header and a .phsp file.
        // fileName may point to either, so we need to assign it to either headerPath or phspPath and then derive the other path.
        if (fileName.find(".header") != std::string::npos) {
            headerFileName_ = fileName;
            phspFileName_ = fileName.substr(0, fileName.find(".header")) + ".phsp";
        } else if (fileName.find(".phsp") != std::string::npos) {
            phspFileName_ = fileName;
            headerFileName_ = fileName.substr(0, fileName.find(".phsp")) + ".header";
        } else {
            throw std::runtime_error("Invalid file name: " + fileName);
        }
    }

    std::size_t Header::getRecordLength() const
    {
        if (formatType_ == TOPASFormat::LIMITED) {
            return LIMITED_RECORD_LENGTH; // Fixed length for limited format
        } else {
            std::size_t length = 0;
            for (const auto &column : columnTypes_) {
                length += column.sizeOf();
            }
            return length;
        }
    }

    void Header::writeHeader()
    {
        std::ofstream file(headerFileName_, std::ios::out | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + headerFileName_);
        }

        switch(formatType_) {
            case TOPASFormat::ASCII:
                writeHeader_ASCII(file);
                break;
            case TOPASFormat::BINARY:
                writeHeader_Binary(file);
                break;
            default:
                writeHeader_Limited(file);
                break;
        }
        
        file.flush();
        file.close();
    }

    void Header::writeHeader_Limited(std::ofstream & file) const
    {
        file << "$TITLE:" << std::endl;
        file << "TOPAS Phase Space in \"limited\" format. Should only be used when it is necessary to read or write from restrictive older codes." << std::endl;
        file << "$RECORD_CONTENTS:" << std::endl;
        file << "    1     // X is stored ?" << std::endl;
        file << "    1     // Y is stored ?" << std::endl;
        file << "    1     // Z is stored ?" << std::endl;
        file << "    1     // U is stored ?" << std::endl;
        file << "    1     // V is stored ?" << std::endl;
        file << "    1     // W is stored ?" << std::endl;
        file << "    1     // Weight is stored ?" << std::endl;
        file << "    0     // Extra floats stored ?" << std::endl;
        file << "    0     // Extra longs stored ?" << std::endl;
        file << "$RECORD_LENGTH:" << std::endl;
        file << LIMITED_RECORD_LENGTH << std::endl; // Fixed length for limited format
        file << "$ORIG_HISTORIES:" << std::endl;
        file << numberOfOriginalHistories_ << std::endl;
        file << "$PARTICLES:" << std::endl;
        file << numberOfParticles_ << std::endl;
        file << "$EXTRA_FLOATS:" << std::endl;
        file << "0" << std::endl;
        file << "$EXTRA_INTS:" << std::endl;
        file << "0" << std::endl;
    }

    void Header::writeHeader_Binary(std::ofstream & file) const
    {
        // prefix
        file << "TOPAS Binary Phase Space" << std::endl << std::endl;
        file << "Number of Original Histories: " << numberOfOriginalHistories_ << std::endl;
        file << "Number of Original Histories that Reached Phase Space: " << numberOfRepresentedHistories_ << std::endl;
        file << "Number of Scored Particles: " << numberOfParticles_ << std::endl;

        // column descriptions
        file << "Number of Bytes per Particle: " << getRecordLength() << std::endl;
        file << std::endl << "Byte order of each record is as follows:" << std::endl;
        
        for (const auto &column : columnTypes_) {
            if (column.valueType_ == DataType::STRING) continue;
            std::size_t size = column.sizeOf();
            switch (column.valueType_) {
                case DataType::INT8:
                case DataType::INT32:
                    file << "i";
                    break;
                case DataType::FLOAT32:
                case DataType::FLOAT64:
                    file << "f";
                    break;
                case DataType::BOOLEAN:
                    file << "b";
                    break;
                default:
                    throw std::runtime_error("Unsupported value type in header.");
            }
            file << size << ": " << column.name_ << std::endl;
        }
        file << std::endl;

        // suffix
        writeSuffix(file);
    }

    void Header::writeHeader_ASCII(std::ofstream & file) const
    {
        // prefix
        file << "TOPAS ASCII Phase Space" << std::endl << std::endl;
        file << "Number of Original Histories: " << numberOfOriginalHistories_ << std::endl;
        file << "Number of Original Histories that Reached Phase Space: " << numberOfRepresentedHistories_ << std::endl;
        file << "Number of Scored Particles: " << numberOfParticles_ << std::endl;

        // column descriptions
        int nDigits = (columnTypes_.size() > 0) ? static_cast<int>(std::log10(columnTypes_.size())) + 1 : 1;
        file << std::endl << "Columns of data are as follows:" << std::endl;
        for (std::size_t i = 0 ; i < columnTypes_.size() ; i++) {
            const auto &column = columnTypes_[i];
            file << std::setw(nDigits) << i + 1 << ": " << column.name_ << std::endl;
        }
        file << std::endl;
        
        // suffix
        writeSuffix(file);
    }

    void Header::writeSuffix(std::ofstream & file) const
    {
        ParticleStats pdgZeroStats;

        for (const auto &entry : particleStatsTable_) {
            ParticleType type = entry.first;
            const ParticleStats &stats = entry.second;
            int pdgCode = getPDGIDFromParticleType(type);
            if (!PDGtoGeant4NameLookupTable.contains(pdgCode)) type = ParticleType::Unsupported;
            if (type != ParticleType::Unsupported)
            {
                file << "Number of " << PDGtoGeant4NameLookupTable.at(pdgCode) << ": " << stats.count_ << std::endl;
            } else {
                pdgZeroStats.count_ += stats.count_;
            }
        }
        if (pdgZeroStats.count_ > 0) {
            file << "Number of particles with PDG code zero: " << pdgZeroStats.count_ << std::endl;
        }

        file << std::endl;

        for (const auto &entry : particleStatsTable_) {
            ParticleType type = entry.first;
            const ParticleStats &stats = entry.second;
            int pdgCode = getPDGIDFromParticleType(type);
            if (!PDGtoGeant4NameLookupTable.contains(pdgCode)) type = ParticleType::Unsupported;
            if (type != ParticleType::Unsupported)
            {
                file << "Minimum Kinetic Energy of " << PDGtoGeant4NameLookupTable.at(pdgCode) << ": " << stats.minKineticEnergy_ / MeV << " MeV" << std::endl;
            } else {
                pdgZeroStats.minKineticEnergy_ = std::min(pdgZeroStats.minKineticEnergy_, stats.minKineticEnergy_);
            }
        }
        if (pdgZeroStats.count_ > 0) {
            file << "Minimum Kinetic Energy of particles with PDG code zero: " << pdgZeroStats.minKineticEnergy_ / MeV << " MeV" << std::endl;
        }

        file << std::endl;

        for (const auto &entry : particleStatsTable_) {
            ParticleType type = entry.first;
            const ParticleStats &stats = entry.second;
            int pdgCode = getPDGIDFromParticleType(type);
            if (!PDGtoGeant4NameLookupTable.contains(pdgCode)) type = ParticleType::Unsupported;
            if (type != ParticleType::Unsupported)
            {
                file << "Maximum Kinetic Energy of " << PDGtoGeant4NameLookupTable.at(pdgCode) << ": " << stats.maxKineticEnergy_ / MeV << " MeV" << std::endl;
            } else {
                pdgZeroStats.maxKineticEnergy_ = std::max(pdgZeroStats.maxKineticEnergy_, stats.maxKineticEnergy_);
            }
        }
        if (pdgZeroStats.count_ > 0) {
            file << "Maximum Kinetic Energy of particles with PDG code zero: " << pdgZeroStats.maxKineticEnergy_ / MeV << " MeV" << std::endl;
        }
    }

    void Header::readHeader()
    {
        std::ifstream headerFile(headerFileName_);
        if (!headerFile.is_open() || headerFile.fail() || headerFile.bad()) {
            throw std::runtime_error("Failed to open header file: " + headerFileName_);
        }
        std::string firstLine;
        std::getline(headerFile, firstLine);

        particleStatsTable_.clear();

        if (firstLine.find("$TITLE:") != std::string::npos) {
            formatType_ = TOPASphspFile::TOPASFormat::LIMITED;
            readHeader_Limited(headerFile);
        } else if (firstLine.find("TOPAS ASCII") != std::string::npos) {
            formatType_ = TOPASphspFile::TOPASFormat::ASCII;
            readHeader_Standard(headerFile);
        } else if (firstLine.find("TOPAS Binary") != std::string::npos) {
            formatType_ = TOPASphspFile::TOPASFormat::BINARY;
            readHeader_Standard(headerFile);
        } else {
            throw std::runtime_error("Unsupported TOPAS phsp format in file: " + phspFileName_);
        }

        headerFile.close();
    }

    void Header::readHeader_Limited(std::ifstream & headerFile)
    {
        std::string line;
        while (std::getline(headerFile, line)) {
            if (line.find("$ORIG_HISTORIES:") != std::string::npos) {
                std::getline(headerFile, line);
                numberOfOriginalHistories_ = std::stoul(line);
            } else if (line.find("$PARTICLES:") != std::string::npos) {
                std::getline(headerFile, line);
                numberOfParticles_ = std::stoul(line);
            }
        }
        numberOfRepresentedHistories_ = 0;
    }

    inline auto trim(std::string &stringToTrim) {
        if (stringToTrim.empty()) return; // No need to trim empty strings
        auto ws = " \t\r\n";
        stringToTrim.erase(0, stringToTrim.find_first_not_of(ws));
        stringToTrim.erase(stringToTrim.find_last_not_of(ws) + 1);
    };

    inline std::string readNextNonEmptyLine(std::ifstream & file)
    {
        std::string line;
        while (std::getline(file, line) && file.good()) {
            trim(line);
            if (!line.empty()) {
                return line;
            }
        }
        throw std::runtime_error("Unexpected end of file while reading header.");
    }

    inline std::string getFirstTokenLower(const std::string &line) {
        std::istringstream iss(line);
        std::string tok;
        if (!(iss >> tok)) return "";
        // strip off any trailing non‐alphanumeric chars (e.g. ':')
        while (!tok.empty() && !std::isalnum(tok.back())) {
            tok.pop_back();
        }
        std::transform(tok.begin(), tok.end(), tok.begin(),
                   [](unsigned char c) -> char {
                       return static_cast<char>(std::tolower(c));
                   });
        return tok;
    }

    void Header::readHeader_Standard(std::ifstream & headerFile)
    {
        auto extractNumber = [](std::string & line) -> std::uint64_t {
            size_t colonPos = line.find_last_of(':');
            if (colonPos == std::string::npos) {
                throw std::runtime_error("Invalid TOPAS header file.");
            }
            std::string numberStr = line.substr(colonPos + 1);
            numberStr.erase(0, numberStr.find_first_not_of(" \t\n\r")); // Trim leading whitespace
            numberStr.erase(numberStr.find_last_not_of(" \t\n\r") + 1); // Trim trailing whitespace
            return std::stoull(numberStr);
        };

        auto extractEnergy = [](const std::string &line) -> double {
            auto colonPos = line.find_last_of(':');
            if (colonPos == std::string::npos)
                throw std::runtime_error("Invalid TOPAS header file.");

            std::string energyStr = line.substr(colonPos + 1);

            // 1) robust trim on both ends
            auto start = energyStr.find_first_not_of(" \t\r\n");
            if (start == std::string::npos)
                throw std::runtime_error("Empty energy string in header.");
            energyStr.erase(0, start);
            auto end = energyStr.find_last_not_of(" \t\r\n");
            if (end != std::string::npos)
                energyStr.erase(end + 1);

            // 2) strip trailing " MeV"
            constexpr std::string_view suf{ " MeV" };
            if (energyStr.size() >= suf.size() &&
                energyStr.compare(energyStr.size() - suf.size(), suf.size(), suf) == 0)
            {
                energyStr.resize(energyStr.size() - suf.size());
                // re-trim trailing whitespace
                auto e2 = energyStr.find_last_not_of(" \t\r\n");
                if (e2 != std::string::npos)
                    energyStr.erase(e2 + 1);
            }

            return std::stod(energyStr) * MeV;
        };

        std::string line;
        line = readNextNonEmptyLine(headerFile);
        numberOfOriginalHistories_ = extractNumber(line);
        line = readNextNonEmptyLine(headerFile);
        numberOfRepresentedHistories_ = extractNumber(line);
        line = readNextNonEmptyLine(headerFile);
        numberOfParticles_ = extractNumber(line);

        if (formatType_ == TOPASFormat::BINARY) {
            readColumns_Binary(headerFile);
        } else if (formatType_ == TOPASFormat::ASCII) {
            readColumns_ASCII(headerFile);
        } else {
            throw std::runtime_error("Unsupported format type for reading columns.");
        }

        // Read particle counts
        while (std::getline(headerFile, line)) {
            if (line.empty()) {
                break; // End of section
            }
            auto particleCount = extractNumber(line);
            if (line.find("Number of ") != std::string::npos) {
                std::string particleName = line.substr(10, line.find(':') - 10);
                ParticleType particleType = Geant4NameToParticleTypeLookupTable.contains(particleName) ? Geant4NameToParticleTypeLookupTable.at(particleName) : ParticleType::Unsupported;
                auto & stats = particleStatsTable_[particleType];
                stats.count_ += particleCount;
            } else throw std::runtime_error("Invalid TOPAS header file. Invalid particle count line: " + line);
        }

        // Read minimum kinetic energies
        while (std::getline(headerFile, line)) {
            if (line.empty()) {
                break; // End of section
            }
            if (line.find("Minimum Kinetic Energy of ") != std::string::npos) {
                std::string particleName = line.substr(26, line.find(':') - 26);
                ParticleType particleType = Geant4NameToParticleTypeLookupTable.contains(particleName) ? Geant4NameToParticleTypeLookupTable.at(particleName) : ParticleType::Unsupported;
                auto & stats = particleStatsTable_[particleType];
                double energy = extractEnergy(line);
                stats.minKineticEnergy_ = std::min(energy, stats.minKineticEnergy_);
            } else throw std::runtime_error("Invalid TOPAS header file. Invalid minimum kinetic energy line: " + line);
        }

        // Read maximum kinetic energies
        while (std::getline(headerFile, line)) {
            if (line.empty()) {
                break; // End of section
            }
            if (line.find("Maximum Kinetic Energy of ") != std::string::npos) {
                std::string particleName = line.substr(26, line.find(':') - 26);
                ParticleType particleType = Geant4NameToParticleTypeLookupTable.contains(particleName) ? Geant4NameToParticleTypeLookupTable.at(particleName) : ParticleType::Unsupported;
                auto & stats = particleStatsTable_[particleType];
                double energy = extractEnergy(line);
                stats.maxKineticEnergy_ = std::max(energy, stats.maxKineticEnergy_);
            } else throw std::runtime_error("Invalid TOPAS header file. Invalid maximum kinetic energy line: " + line);
        }
    }


    void Header::readColumns_Binary(std::ifstream & headerFile)
    {
        std::string line;
        columnTypes_.clear();
        bool foundColumnsSection = false;
        while (std::getline(headerFile, line)) {
            trim(line);

            auto key = getFirstTokenLower(line);
            // skip both "Number of Bytes per Particle…" and "Byte order…" lines
            if (key == "number" || key == "byte") {
                continue;
            } else if (line.empty()) {
                if (foundColumnsSection) break; // End of section
                else continue; // Skip empty lines before columns section
            }
            foundColumnsSection = true;

            // Parse column definition
            size_t colonPos = line.find(':');
            if (colonPos == std::string::npos)
                throw std::runtime_error("Invalid column definition in binary header.");
            std::string typeField = line.substr(0, colonPos);
            std::string nameField = line.substr(colonPos + 1);
            trim(typeField);
            trim(nameField);

            std::string columnName = nameField;
            DataType valueType;
            char code = typeField[0];
            int size = std::stoi(typeField.substr(1));
            switch (code) {
                case 'i':
                    switch (size) {
                        case sizeof(std::int8_t):
                            valueType = DataType::INT8;
                            break;
                        case sizeof(std::int32_t):
                            valueType = DataType::INT32;
                            break;
                        default:
                            throw std::runtime_error("Unsupported integer size in binary header.");
                    }
                    break;
                case 'f':
                    switch (size) {
                        case sizeof(float):
                            valueType = DataType::FLOAT32;
                            break;
                        case sizeof(double):
                            valueType = DataType::FLOAT64;
                            break;
                        default:
                            throw std::runtime_error("Unsupported float size in binary header.");
                    }
                    break;
                case 'b':
                    valueType = DataType::BOOLEAN;
                    break;
                default:
                    throw std::runtime_error("Unsupported value type in binary header.");
            }
            // recover the ColumnType from the name
            ColumnType columnType = DataColumn::getColumnType(columnName);
            columnTypes_.emplace_back(columnType, valueType, columnName);
        }
    }

    void Header::readColumns_ASCII(std::ifstream & headerFile)
    {
        std::string line;
        columnTypes_.clear();
        bool foundColumnsSection = false;
        while (std::getline(headerFile, line)) {
            trim(line);
            auto key = getFirstTokenLower(line);
            if (key == "columns") {
                // Skip this line
                continue;
            } else if (line.empty()) {
                if (foundColumnsSection) break; // End of section
                else continue; // Skip empty lines before columns section
            }
            foundColumnsSection = true;

            // Parse column definition
            size_t colonPos = line.find(':');
            if (colonPos == std::string::npos) {
                throw std::runtime_error("Invalid column definition in ASCII header.");
            }
            std::string columnName = line.substr(colonPos + 1);
            trim(columnName);
            columnTypes_.emplace_back(columnName);
        }
    }

} // namespace ParticleZoo::TOPASphsp