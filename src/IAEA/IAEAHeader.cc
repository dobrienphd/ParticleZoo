#include "particlezoo/IAEA/IAEAHeader.h"

#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iomanip>

namespace ParticleZoo::IAEAphspFile
{

    IAEAHeader::ParticleStats::ParticleStats()
    : count_(0), weightSum_(0.0f), minWeight_(std::numeric_limits<float>::max()), maxWeight_(0),
    energySum_(0.0f), minEnergy_(std::numeric_limits<float>::max()), maxEnergy_(0) {}

    IAEAHeader::IAEAHeader(const std::string &filePath, bool newFile)
        : filePath_(filePath), IAEAIndex_("1000"), title_("PHASESPACE in IAEA format"), fileType_(FileType::PHSP_FILE), checksum_(0),
          xIsStored_(true), yIsStored_(true), zIsStored_(true),
          uIsStored_(true), vIsStored_(true), wIsStored_(true),
          weightIsStored_(true), extraFloatData_(), extraLongData_(),
          recordLength_(29), byteOrder_(HOST_BYTE_ORDER), originalHistories_(0),
          numberOfParticles_(0), minX_(std::numeric_limits<float>::max()),
          maxX_(std::numeric_limits<float>::lowest()), minY_(std::numeric_limits<float>::max()),
          maxY_(std::numeric_limits<float>::lowest()), minZ_(std::numeric_limits<float>::max()),
          maxZ_(std::numeric_limits<float>::lowest()), particleStatsTable_(), sectionTable_()
    {
        // Initialize the particle statistics table with default values.
        particleStatsTable_[ParticleType::Photon] = ParticleStats();
        particleStatsTable_[ParticleType::Electron] = ParticleStats();
        particleStatsTable_[ParticleType::Positron] = ParticleStats();
        particleStatsTable_[ParticleType::Neutron] = ParticleStats();
        particleStatsTable_[ParticleType::Proton] = ParticleStats();
        
        if (!newFile) {
            std::ifstream file(filePath);
            if (file.is_open())
            {
                readHeader(file);
            }
        }
        generateSectionTable();
    }

    IAEAHeader::IAEAHeader(const IAEAHeader &other, const std::string & newFilePath)
    : filePath_(newFilePath),
      IAEAIndex_(other.IAEAIndex_),
      title_(other.title_),
      fileType_(other.fileType_),
      checksum_(0),
      xIsStored_(other.xIsStored_),
      yIsStored_(other.yIsStored_),
      zIsStored_(other.zIsStored_),
      uIsStored_(other.uIsStored_),
      vIsStored_(other.vIsStored_),
      wIsStored_(other.wIsStored_),
      weightIsStored_(other.weightIsStored_),
      constantX_(other.constantX_),
      constantY_(other.constantY_),
      constantZ_(other.constantZ_),
      constantU_(other.constantU_),
      constantV_(other.constantV_),
      constantW_(other.constantW_),
      constantWeight_(other.constantWeight_),
      extraFloatData_(other.extraFloatData_),
      extraLongData_(other.extraLongData_),
      recordLength_(other.recordLength_),
      byteOrder_(other.byteOrder_),
      originalHistories_(0),
      numberOfParticles_(0),
      minX_(std::numeric_limits<float>::max()),
      maxX_(std::numeric_limits<float>::lowest()),
      minY_(std::numeric_limits<float>::max()),
      maxY_(std::numeric_limits<float>::lowest()),
      minZ_(std::numeric_limits<float>::max()),
      maxZ_(std::numeric_limits<float>::lowest()),
      particleStatsTable_(),
      sectionTable_(other.sectionTable_)
    {
        // Initialize the particle statistics table with default values.
        particleStatsTable_[ParticleType::Photon] = ParticleStats();
        particleStatsTable_[ParticleType::Electron] = ParticleStats();
        particleStatsTable_[ParticleType::Positron] = ParticleStats();
        particleStatsTable_[ParticleType::Neutron] = ParticleStats();
        particleStatsTable_[ParticleType::Proton] = ParticleStats();
        generateSectionTable();
    }

    const std::string IAEAHeader::DeterminePathToHeaderFile(const std::string & filename)
    {
        // The header file is the same as the data file, but with the extension changed to .IAEAheader
        std::string headerFileName = filename.substr(0, filename.find_last_of('.')) + ".IAEAheader";
        return headerFileName;
    }

    std::string IAEAHeader::getDataFilePath() const
    {
        const std::string & headerFileName = filePath_;
        // the data file name is the same as the header file name, but with the extention changed to .IAEAphsp
        std::string dataFileName = headerFileName.substr(0, headerFileName.find_last_of('.')) + ".IAEAphsp";
        return dataFileName;
    }

    std::uint64_t IAEAHeader::getNumberOfParticles(ParticleType particleType) const
    {
        auto it = particleStatsTable_.find(particleType);
        if (it != particleStatsTable_.end())
        {
            return it->second.count_;
        }
        return 0;
    }

    unsigned int IAEAHeader::calculateMinimumRecordLength() const
    {
        unsigned int minimumRecordLength = 1 + 4 + 4 * ((xIsStored_ ? 1 : 0) + (yIsStored_ ? 1 : 0) + (zIsStored_ ? 1 : 0) +
        (uIsStored_ ? 1 : 0) + (vIsStored_ ? 1 : 0) +
        (weightIsStored_ ? 1 : 0) + extraFloatData_.size() + extraLongData_.size());
        return minimumRecordLength;
    }

    bool IAEAHeader::checksumIsValid() const
    {
        unsigned int minimumRecordLength = calculateMinimumRecordLength();

        std::size_t recordLength = getRecordLength();
        std::uint64_t numberOfParticles = getNumberOfParticles();

        std::uint64_t expectedChecksum = recordLength * numberOfParticles;
        std::uint64_t checksum = getChecksum();

        // get the file size in bytes
        std::string dataFilePath = getDataFilePath();
        std::ifstream check_file(dataFilePath, std::ios::binary | std::ios::ate);
        std::uint64_t fileSize = 0;
        if (check_file.is_open())
        {
            fileSize = static_cast<std::uint64_t>(check_file.tellg());
            check_file.close();
        }
        else
        {
            throw std::runtime_error("Failed to open file for checksum validation: " + dataFilePath);
        }

        bool checksumEqualsFileSize = (checksum == fileSize);
        bool recordLengthValid = (recordLength >= minimumRecordLength);
        bool expectedChecksumValue = (expectedChecksum == checksum);

        return (checksumEqualsFileSize) && (recordLengthValid) && (expectedChecksumValue);
    }

    const std::string IAEAHeader::getSection(const std::string &sectionName) const
    {
        auto it = sectionTable_.find(sectionName);
        if (it != sectionTable_.end())
        {
            return it->second;
        }
        return "UNKNOWN";
    }

    const std::string IAEAHeader::getSection(SECTION section) const
    {
        auto key = sectionToString(section);
        auto it = sectionTable_.find(std::string{key});
        return (it != sectionTable_.end()) ? it->second : "";
    }

    float IAEAHeader::getMinWeight(ParticleType particleType) const { 
        auto it = particleStatsTable_.find(particleType);
        if (it == particleStatsTable_.end() || it->second.count_ == 0)
            return 0;
        return it->second.minWeight_;
    }
    
    float IAEAHeader::getMaxWeight(ParticleType particleType) const {
        auto it = particleStatsTable_.find(particleType);
        if (it == particleStatsTable_.end() || it->second.count_ == 0)
            return 0;
        return it->second.maxWeight_;
    }
    
    float IAEAHeader::getMinEnergy(ParticleType particleType) const {
        auto it = particleStatsTable_.find(particleType);
        if (it == particleStatsTable_.end() || it->second.count_ == 0)
            return 0;
        return it->second.minEnergy_;
    }
    
    float IAEAHeader::getMaxEnergy(ParticleType particleType) const {
        auto it = particleStatsTable_.find(particleType);
        if (it == particleStatsTable_.end() || it->second.count_ == 0)
            return 0;
        return it->second.maxEnergy_;
    }
    
    float IAEAHeader::getMeanWeight(ParticleType particleType) const {
        auto it = particleStatsTable_.find(particleType);
        if (it == particleStatsTable_.end() || it->second.count_ == 0)
            return 0.0f;
        return (float) (it->second.weightSum_ / it->second.count_);
    }
    
    float IAEAHeader::getMeanEnergy(ParticleType particleType) const {
        auto it = particleStatsTable_.find(particleType);
        if (it == particleStatsTable_.end() || it->second.count_ == 0)
            return 0.0f;
        return (float)(it->second.energySum_ / it->second.count_);
    }    
    
    float IAEAHeader::getTotalWeight(ParticleType particleType) const {
        auto it = particleStatsTable_.find(particleType);
        if (it == particleStatsTable_.end() || it->second.count_ == 0)
            return 0.0f;
        return (float)it->second.weightSum_;
    }

    template<typename T>
    bool readToken(std::istream& stream, T& value) {
        std::string token;
        while (stream >> token) {
            // Remove any inline comment from the token.
            size_t commentPos = token.find("//");
            if (commentPos != std::string::npos)
                token = token.substr(0, commentPos);
            if (!token.empty()) {
                std::istringstream iss(token);
                iss >> value;
                return true;
            }
        }
        return false;
    }

    ParticleType convertParticleTypeFromString(const std::string &typeStr) {
        if (typeStr == "PHOTONS") return ParticleType::Photon;
        if (typeStr == "ELECTRONS") return ParticleType::Electron;
        if (typeStr == "POSITRONS") return ParticleType::Positron;
        if (typeStr == "NEUTRONS") return ParticleType::Neutron;
        if (typeStr == "PROTONS") return ParticleType::Proton;
        throw std::runtime_error("Unknown particle type: " + typeStr);
    }

    void IAEAHeader::readHeader(std::ifstream & file)
    {
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open IAEA header file: " + filePath_);
        }

        auto processSection = [&](const std::string &sectionTitle, const std::stringstream &sectionContent) {
            SECTION sectionType = getSectionFromString(sectionTitle);
            std::string content = stripWhiteSpace(sectionContent.str());
            
            switch(sectionType)
            {
                case SECTION::IAEA_INDEX:
                    IAEAIndex_ = content;
                    break;

                case SECTION::TITLE:
                    title_ = content;
                    break;

                case SECTION::FILE_TYPE:
                    {
                        int fileTypeValue = (int)getIntValue(content);
                        switch (fileTypeValue) {
                            case 0: fileType_ = FileType::PHSP_FILE; break;
                            case 1: fileType_ = FileType::PHSP_GENERATOR; break;
                            default: throw std::runtime_error("Unknown file type code: " + std::to_string(fileTypeValue));
                        }
                        if (fileType_ == FileType::PHSP_GENERATOR) {
                            throw std::runtime_error("IAEA Header represents an IAEA phase space generator not an IAEA phase space file. This format is not supported.");
                        }
                    }
                    break;

                case SECTION::CHECKSUM:
                    checksum_ = getIntValue(content);
                    break;

                case SECTION::RECORD_CONTENTS:
                    {
                        std::vector<std::uint64_t> recordContents = getIntArray(content);
                        if (recordContents.size() < 9) {
                            throw std::runtime_error("Invalid RECORD_CONTENTS section: it should have at least 9 values.");
                        }
                        xIsStored_ = recordContents[0] == 1;
                        yIsStored_ = recordContents[1] == 1;
                        zIsStored_ = recordContents[2] == 1;
                        uIsStored_ = recordContents[3] == 1;
                        vIsStored_ = recordContents[4] == 1;
                        wIsStored_ = recordContents[5] == 1;
                        weightIsStored_ = recordContents[6] == 1;

                        // Sometimes "W is stored ?" is marked as 0 when both U and V are stored.
                        // It should be marked as 1 in that case as it's value is derived from U
                        // and V and so it is not recorded explicitly in the header. It is "stored"
                        // implicitly in the binary data.
                        if (!wIsStored_ && uIsStored_ && vIsStored_) {
                            wIsStored_ = true;
                        }

                        int numberOfExtraFloats = (int)recordContents[7];
                        int numberOfExtraLongs = (int)recordContents[8];
                        extraFloatData_.resize(numberOfExtraFloats);
                        extraLongData_.resize(numberOfExtraLongs);
                        for (int i = 0; i < numberOfExtraFloats; ++i) {
                            extraFloatData_[i] = static_cast<EXTRA_FLOAT_TYPE>(recordContents[9 + i]);
                        }
                        for (int i = 0; i < numberOfExtraLongs; ++i) {
                            extraLongData_[i] = static_cast<EXTRA_LONG_TYPE>(recordContents[9 + numberOfExtraFloats + i]);
                        }
                    }
                    break;

                case SECTION::RECORD_CONSTANT:
                    {
                        std::size_t expectedConstants = 0;
                        if (!xIsStored_) expectedConstants++;
                        if (!yIsStored_) expectedConstants++;
                        if (!zIsStored_) expectedConstants++;
                        if (!uIsStored_) expectedConstants++;
                        if (!vIsStored_) expectedConstants++;
                        if (!wIsStored_) expectedConstants++;
                        std::vector<float> recordConstants = getFloatArray(content);
                        if (recordConstants.size() < expectedConstants) {
                            throw std::runtime_error("Invalid RECORD_CONSTANT section: it should have at least " + std::to_string(expectedConstants) + " values.");
                        }
                        std::size_t index = 0;
                        if (!xIsStored_) constantX_ = recordConstants[index++];
                        if (!yIsStored_) constantY_ = recordConstants[index++];
                        if (!zIsStored_) constantZ_ = recordConstants[index++];
                        if (!uIsStored_) constantU_ = recordConstants[index++];
                        if (!vIsStored_) constantV_ = recordConstants[index++];
                        if (!wIsStored_) constantW_ = recordConstants[index++];
                        if (!weightIsStored_) { // allow for weight to be missing
                            if (index >= recordConstants.size()) {
                                constantWeight_ = 1.f; // Default value if not provided
                            } else {
                                constantWeight_ = recordConstants[index];
                            }
                        }
                    }
                    break;

                case SECTION::RECORD_LENGTH:
                    {
                        unsigned int minimumRecordLength = calculateMinimumRecordLength();
                        recordLength_ = (std::size_t) getIntValue(content);
                        if (recordLength_ < minimumRecordLength) {
                            throw std::runtime_error("Invalid RECORD_LENGTH section: it should be at least " + std::to_string(minimumRecordLength) + " bytes.");
                        }
                        break;
                    }

                case SECTION::BYTE_ORDER:
                    {
                        int byteOrderCode = (int)getIntValue(content);
                        switch (byteOrderCode) {
                            case 1234: byteOrder_ = ByteOrder::LittleEndian; break;
                            case 4321: byteOrder_ = ByteOrder::BigEndian; break;
                            case 3412: byteOrder_ = ByteOrder::PDPEndian; break;
                            default: throw std::runtime_error("Unknown byte order code: " + std::to_string(byteOrderCode));
                        }
                        break;
                    }

                case SECTION::ORIGINAL_HISTORIES:
                    originalHistories_ = getIntValue(content);
                    break;

                case SECTION::PARTICLES:
                    numberOfParticles_ = getIntValue(content);
                    break;

                case SECTION::PHOTONS:
                    {
                        std::uint64_t numberOfPhotons = getIntValue(content);
                        particleStatsTable_[ParticleType::Photon].count_ = numberOfPhotons;
                        break;
                    }

                case SECTION::ELECTRONS:
                    {
                        std::uint64_t numberOfElectrons = getIntValue(content);
                        particleStatsTable_[ParticleType::Electron].count_ = numberOfElectrons;
                        break;
                    }

                case SECTION::POSITRONS:
                    {
                        std::uint64_t numberOfPositrons = getIntValue(content);
                        particleStatsTable_[ParticleType::Positron].count_ = numberOfPositrons;
                        break;
                    }

                case SECTION::NEUTRONS:
                    {
                        std::uint64_t numberOfNeutrons = getIntValue(content);
                        particleStatsTable_[ParticleType::Neutron].count_ = numberOfNeutrons;
                        break;
                    }
                    
                case SECTION::PROTONS:
                    {
                        std::uint64_t numberOfProtons = getIntValue(content);
                        particleStatsTable_[ParticleType::Proton].count_ = numberOfProtons;
                        break;
                    }

                case SECTION::STATISTICAL_INFORMATION_PARTICLES:
                {
                    std::istringstream iss(content);
                    std::string statsLine;
                    while (std::getline(iss, statsLine))
                    {
                        // Remove inline comments and trim whitespace.
                        statsLine = removeInlineComments(statsLine);
                        statsLine = stripWhiteSpace(statsLine);
                        if (statsLine.empty())
                            continue;
                        // Skip header lines (those not starting with a digit or '-' or '.')
                        if (!std::isdigit(statsLine.front()) && statsLine.front() != '-' && statsLine.front() != '.')
                            continue;
                        
                        std::istringstream lineStream(statsLine);
                        // Variables to hold parsed tokens.
                        float totalWeight, minWeight, maxWeight, meanEnergy, minEnergy, maxEnergy;
                        std::string particleName;
                        if (!(lineStream >> totalWeight >> minWeight >> maxWeight >> meanEnergy >> minEnergy >> maxEnergy >> particleName))
                        {
                            // If parsing fails for this line, go to the next.
                            continue;
                        }
                        
                        ParticleType particleType;
                        particleType = convertParticleTypeFromString(particleName);
                        
                        // Update the statistics for this particle type.
                        auto &stats = particleStatsTable_[particleType];
                        
                        // Use the parsed min and max weight and weight sum.
                        stats.minWeight_ = minWeight;
                        stats.maxWeight_ = maxWeight;
                        stats.weightSum_ = totalWeight;
                        
                        // Set the energy values.
                        stats.energySum_ = stats.count_ * static_cast<float>(meanEnergy);
                        stats.minEnergy_ = minEnergy;
                        stats.maxEnergy_ = maxEnergy;
                    }
                    break;
                }

                case SECTION::STATISTICAL_INFORMATION_GEOMETRY:
                {
                    // Extract all tokens convertible to float.
                    std::istringstream geometryStream(content);
                    std::vector<float> nums;
                    std::string token;
                    while (geometryStream >> token)
                    {
                        try
                        {
                            float value = std::stof(token);
                            nums.push_back(value);
                        }
                        catch (const std::exception&)
                        {
                            // Skip non-numeric tokens.
                            continue;
                        }
                    }
                    
                    size_t idx = 0;
                    // For axes that are stored, assign the next available pair.
                    if (xIsStored_ && idx + 1 < nums.size())
                    {
                        minX_ = nums[idx++];
                        maxX_ = nums[idx++];
                    } else {
                        minX_ = constantX_;
                        maxX_ = constantX_;
                    }

                    if (yIsStored_ && idx + 1 < nums.size())
                    {
                        minY_ = nums[idx++];
                        maxY_ = nums[idx++];
                    } else {
                        minY_ = constantY_;
                        maxY_ = constantY_;
                    }

                    if (zIsStored_ && idx + 1 < nums.size())
                    {
                        minZ_ = nums[idx++];
                        maxZ_ = nums[idx++];
                    } else {
                        minZ_ = constantZ_;
                        maxZ_ = constantZ_;
                    }
                    
                    break;
                }
                default:
                    break;
            }

            sectionTable_.emplace(sectionTitle, content);
        };


        std::string line;
        std::string sectionTitle;
        std::stringstream sectionContent;
        while (std::getline(file, line))
        {
            // Clean the line by removing comments and stripping whitespace
            line = cleanLine(line);

            if (isSectionHeader(line))
            {
                // remove $ from beginning and : from end of line
                line = line.substr(1, line.find(':') - 1);
                line = stripWhiteSpace(line);
                // If we have a previous section, process it
                if (!sectionTitle.empty())
                {
                    processSection(sectionTitle, sectionContent);
                }

                sectionTitle = line; // Replace with the new section title
                sectionContent = std::stringstream(); // Reset the content stream
            } else {
                // Append the line to the current section content
                sectionContent << line << "\n";
            }
        }
        // Handle the last section
        if (!sectionTitle.empty())
        {
            processSection(sectionTitle, sectionContent);
        }
        file.close();
    }

    void IAEAHeader::generateSectionTable()
    {
        sectionTable_["IAEA_INDEX"] = IAEAIndex_;
        sectionTable_["TITLE"] = title_;
        sectionTable_["FILE_TYPE"] = std::to_string(static_cast<int>(fileType_));

        checksum_ = recordLength_ * numberOfParticles_;
        sectionTable_["CHECKSUM"] = std::to_string(checksum_);

        std::ostringstream recordContents;
        recordContents  << "    " << (xIsStored_ ? 1 : 0) << "     // X is stored ?\n"
                        << "    " << (yIsStored_ ? 1 : 0) << "     // Y is stored ?\n"
                        << "    " << (zIsStored_ ? 1 : 0) << "     // Z is stored ?\n"
                        << "    " << (uIsStored_ ? 1 : 0) << "     // U is stored ?\n"
                        << "    " << (vIsStored_ ? 1 : 0) << "     // V is stored ?\n"
                        << "    " << (wIsStored_ ? 1 : 0) << "     // W is stored ?\n"
                        << "    " << (weightIsStored_ ? 1 : 0) << "     // Weight is stored ?\n"
                        << "    " << extraFloatData_.size() << "     // Extra floats stored ?\n"
                        << "    " << extraLongData_.size() << "     // Extra longs stored ?\n";

        for (size_t i = 0; i < extraFloatData_.size(); i++)
        {
            EXTRA_FLOAT_TYPE extraFloatType = extraFloatData_[i];
            int extraFloatIndex = static_cast<int>(extraFloatType);
            recordContents << "    " << extraFloatIndex << "     // " << EXTRA_FLOAT_TYPE_LABELS[extraFloatIndex] << " [ "<< i << "] \n";
        }

        for (size_t i = 0; i < extraLongData_.size(); i++)
        {
            EXTRA_LONG_TYPE extraLongType = extraLongData_[i];
            int extraLongIndex = static_cast<int>(extraLongType);
            recordContents << "    " << extraLongIndex << "     // " << EXTRA_LONG_TYPE_LABELS[extraLongIndex] << " [ "<< i << "] \n";
        }

        sectionTable_["RECORD_CONTENTS"] = recordContents.str();

        std::ostringstream recordConstants;
        if (!xIsStored_) recordConstants << "   " << std::setw(8) << std::fixed << std::setprecision(4) << constantX_ << "     // Constant X\n";
        if (!yIsStored_) recordConstants << "   " << std::setw(8) << std::fixed << std::setprecision(4) << constantY_ << "     // Constant Y\n";
        if (!zIsStored_) recordConstants << "   " << std::setw(8) << std::fixed << std::setprecision(4) << constantZ_ << "     // Constant Z\n";
        if (!uIsStored_) recordConstants << "   " << std::setw(8) << std::fixed << std::setprecision(4) << constantU_ << "     // Constant U\n";
        if (!vIsStored_) recordConstants << "   " << std::setw(8) << std::fixed << std::setprecision(4) << constantV_ << "     // Constant V\n";
        if (!wIsStored_) recordConstants << "   " << std::setw(8) << std::fixed << std::setprecision(4) << constantW_ << "     // Constant W\n";
        if (!weightIsStored_) recordConstants << "   " << std::setw(8) << std::fixed << std::setprecision(4) << constantWeight_ << "     // Constant Weight\n";
        sectionTable_["RECORD_CONSTANT"] = recordConstants.str();

        sectionTable_["RECORD_LENGTH"] = std::to_string(recordLength_);
        sectionTable_["BYTE_ORDER"] = std::to_string(static_cast<int>(byteOrder_));
        sectionTable_["ORIG_HISTORIES"] = std::to_string(originalHistories_);
        sectionTable_["PARTICLES"] = std::to_string(numberOfParticles_);
        if (particleStatsTable_[ParticleType::Photon].count_ > 0)
        {
            sectionTable_["PHOTONS"] = std::to_string(particleStatsTable_[ParticleType::Photon].count_);
        }
        if (particleStatsTable_[ParticleType::Electron].count_ > 0)
        {
            sectionTable_["ELECTRONS"] = std::to_string(particleStatsTable_[ParticleType::Electron].count_);
        }
        if (particleStatsTable_[ParticleType::Positron].count_ > 0)
        {
            sectionTable_["POSITRONS"] = std::to_string(particleStatsTable_[ParticleType::Positron].count_);
        }
        if (particleStatsTable_[ParticleType::Neutron].count_ > 0)
        {
            sectionTable_["NEUTRONS"] = std::to_string(particleStatsTable_[ParticleType::Neutron].count_);
        }
        if (particleStatsTable_[ParticleType::Proton].count_ > 0)
        {
            sectionTable_["PROTONS"] = std::to_string(particleStatsTable_[ParticleType::Proton].count_);
        }

        // Set values close to zero to exactly zero to prevent -0 from being printed.
        auto fixVal = [](float val) -> float {
            return (val > -1e-7 && std::fabs(val) < 1e-7 ? 0.0f : val);
        };

        auto printParticleStats = [&](const ParticleType particleType, std::ostringstream & statsStream) {
            const ParticleStats &stats = particleStatsTable_[particleType];
            if (stats.count_ == 0) {
                return; // Skip if no particles of this type
            }
            std::string particleTypeName = std::string(getParticleTypeName(particleType));
            std::transform(particleTypeName.begin(), particleTypeName.end(), particleTypeName.begin(), [](unsigned char c) -> char { return (char)std::toupper(c); });                
            statsStream << "  " 
                        << std::setw(15) << std::setprecision(6) << std::defaultfloat << fixVal((float)stats.weightSum_) << " "
                        << std::setw(10) << std::setprecision(4) << fixVal(stats.minWeight_) << " "
                        << std::setw(10) << std::setprecision(4) << fixVal(stats.maxWeight_) << " "
                        << std::setw(10) << std::setprecision(4) << fixVal(getMeanEnergy(particleType)) << "    "
                        << std::setw(10) << std::setprecision(4) << fixVal(stats.minEnergy_) << "  "
                        << std::setw(10) << std::setprecision(4) << fixVal(stats.maxEnergy_) << "   "
                        << particleTypeName << "S\n";
        };

        std::ostringstream statsStream;
        statsStream << "//        Weight        Wmin       Wmax       <E>         Emin         Emax    Particle\n";
        printParticleStats(ParticleType::Photon, statsStream);
        printParticleStats(ParticleType::Electron, statsStream);
        printParticleStats(ParticleType::Positron, statsStream);
        printParticleStats(ParticleType::Neutron, statsStream);
        printParticleStats(ParticleType::Proton, statsStream);
        sectionTable_["STATISTICAL_INFORMATION_PARTICLES"] = statsStream.str();

        std::ostringstream geometryStream;
        if (xIsStored_) geometryStream << minX_ << " " << maxX_ << "\n";
        if (yIsStored_) geometryStream << minY_ << " " << maxY_ << "\n";
        if (zIsStored_) geometryStream << minZ_ << " " << maxZ_ << "\n";
        sectionTable_["STATISTICAL_INFORMATION_GEOMETRY"] = geometryStream.str();

        if (sectionTable_.find("TRANSPORT_PARAMETERS") == sectionTable_.end())
        {
            sectionTable_["TRANSPORT_PARAMETERS"] = "";
        }
        if (sectionTable_.find("MACHINE_TYPE") == sectionTable_.end())
        {
            sectionTable_["MACHINE_TYPE"] = "";
        }
        if (sectionTable_.find("MONTE_CARLO_CODE_VERSION") == sectionTable_.end())
        {
            sectionTable_["MONTE_CARLO_CODE_VERSION"] = "";
        }
        if (sectionTable_.find("GLOBAL_PHOTON_ENERGY_CUTOFF") == sectionTable_.end())
        {
            sectionTable_["GLOBAL_PHOTON_ENERGY_CUTOFF"] = "";
        }
        if (sectionTable_.find("GLOBAL_PARTICLE_ENERGY_CUTOFF") == sectionTable_.end())
        {
            sectionTable_["GLOBAL_PARTICLE_ENERGY_CUTOFF"] = "";
        }
        if (sectionTable_.find("COORDINATE_SYSTEM_DESCRIPTION") == sectionTable_.end())
        {
            sectionTable_["COORDINATE_SYSTEM_DESCRIPTION"] = "";
        }
        if (sectionTable_.find("BEAM_NAME") == sectionTable_.end())
        {
            sectionTable_["BEAM_NAME"] = "";
        }
        if (sectionTable_.find("FIELD_SIZE") == sectionTable_.end())
        {
            sectionTable_["FIELD_SIZE"] = "";
        }
        if (sectionTable_.find("NOMINAL_SSD") == sectionTable_.end())
        {
            sectionTable_["NOMINAL_SSD"] = "";
        }
        if (sectionTable_.find("MC_INPUT_FILENAME") == sectionTable_.end())
        {
            sectionTable_["MC_INPUT_FILENAME"] = "";
        }
        if (sectionTable_.find("VARIANCE_REDUCTION_TECHNIQUES") == sectionTable_.end())
        {
            sectionTable_["VARIANCE_REDUCTION_TECHNIQUES"] = "";
        }
        if (sectionTable_.find("INITIAL_SOURCE_DESCRIPTION") == sectionTable_.end())
        {
            sectionTable_["INITIAL_SOURCE_DESCRIPTION"] = "";
        }
        if (sectionTable_.find("PUBLISHED_REFERENCE") == sectionTable_.end())
        {
            sectionTable_["PUBLISHED_REFERENCE"] = "";
        }
        if (sectionTable_.find("AUTHORS") == sectionTable_.end())
        {
            sectionTable_["AUTHORS"] = "";
        }
        if (sectionTable_.find("INSTITUTION") == sectionTable_.end())
        {
            sectionTable_["INSTITUTION"] = "";
        }
        if (sectionTable_.find("LINK_VALIDATION") == sectionTable_.end())
        {
            sectionTable_["LINK_VALIDATION"] = "";
        }
        if (sectionTable_.find("ADDITIONAL_NOTES") == sectionTable_.end())
        {
            sectionTable_["ADDITIONAL_NOTES"] = "This is IAEA header as defined in the technical\nreport IAEA(NDS)-0484, Vienna, 2006\n";
        }
    }

    void IAEAHeader::writeHeader()
    {
        generateSectionTable();
        std::ofstream file(filePath_);
        if (!file.is_open())
        {
            throw std::runtime_error("Unable to open header file for writing: " + filePath_);
        }
    
        auto writeSectionFromName = [&](std::string sectionTitle) {
            std::string & sectionContent = sectionTable_[sectionTitle];
            file << "$" << sectionTitle << ":\n";
            file << sectionContent;
            // Ensure the section ends with a newline.
            if (!sectionContent.empty() && sectionContent.back() != '\n')
            {
                file << "\n";
            }
            // Add an extra newline for separation.
            file << "\n";
        };

        auto writeSection = [&](SECTION section) {
            std::string sectionTitle = std::string(sectionToString(section));
            writeSectionFromName(sectionTitle);
        };

        writeSection(SECTION::IAEA_INDEX);
        writeSection(SECTION::TITLE);
        writeSection(SECTION::FILE_TYPE);
        writeSection(SECTION::CHECKSUM);
        writeSection(SECTION::RECORD_CONTENTS);
        writeSection(SECTION::RECORD_CONSTANT);
        writeSection(SECTION::RECORD_LENGTH);
        writeSection(SECTION::BYTE_ORDER);
        writeSection(SECTION::ORIGINAL_HISTORIES);
        writeSection(SECTION::PARTICLES);
        if (particleStatsTable_[ParticleType::Photon].count_ > 0)
        {
            writeSection(SECTION::PHOTONS);
        }
        if (particleStatsTable_[ParticleType::Electron].count_ > 0)
        {
            writeSection(SECTION::ELECTRONS);
        }
        if (particleStatsTable_[ParticleType::Positron].count_ > 0)
        {
            writeSection(SECTION::POSITRONS);
        }
        if (particleStatsTable_[ParticleType::Neutron].count_ > 0)
        {
            writeSection(SECTION::NEUTRONS);
        }
        if (particleStatsTable_[ParticleType::Proton].count_ > 0)
        {
            writeSection(SECTION::PROTONS);
        }
        writeSection(SECTION::TRANSPORT_PARAMETERS);
        writeSection(SECTION::MACHINE_TYPE);
        writeSection(SECTION::MONTE_CARLO_CODE_VERSION);
        writeSection(SECTION::GLOBAL_PHOTON_ENERGY_CUTOFF);
        writeSection(SECTION::GLOBAL_PARTICLE_ENERGY_CUTOFF);
        writeSection(SECTION::COORDINATE_SYSTEM_DESCRIPTION);

        file << "//  OPTIONAL INFORMATION\n\n";

        writeSection(SECTION::BEAM_NAME);
        writeSection(SECTION::FIELD_SIZE);
        writeSection(SECTION::NOMINAL_SSD);
        writeSection(SECTION::MC_INPUT_FILENAME);
        writeSection(SECTION::VARIANCE_REDUCTION_TECHNIQUES);
        writeSection(SECTION::INITIAL_SOURCE_DESCRIPTION);
        writeSection(SECTION::PUBLISHED_REFERENCE);
        writeSection(SECTION::AUTHORS);
        writeSection(SECTION::INSTITUTION);
        writeSection(SECTION::LINK_VALIDATION);
        writeSection(SECTION::ADDITIONAL_NOTES);

        for (const auto &entry : sectionTable_)
        {
            const std::string & sectionTitle = entry.first;
            SECTION sectionType = getSectionFromString(sectionTitle);
            if (sectionType == SECTION::CUSTOM_SECTION)
            {
                writeSectionFromName(sectionTitle);
            }
        }

        writeSection(SECTION::STATISTICAL_INFORMATION_PARTICLES);
        writeSection(SECTION::STATISTICAL_INFORMATION_GEOMETRY);

        file.close();
    }

}