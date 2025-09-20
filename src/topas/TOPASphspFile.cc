
#include "particlezoo/TOPAS/TOPASphspFile.h"
#include "particlezoo/TOPAS/TOPASHeader.h"
#include "particlezoo/Particle.h"
#include <sstream>
#include <iomanip>

namespace ParticleZoo::TOPASphspFile
{
    CLICommand TOPASFormatCommand { WRITER, "", "TOPAS-format", "Specify the TOPAS phase space file format to write (ASCII, BINARY or LIMITED)", { CLI_STRING }, { "BINARY" } };

    // read the header, decide ASCII vs BINARY, then hand back the header and it's format type
    inline std::pair<FormatType,Header> readHeader(const std::string &filename)
    {
        Header header(filename);
        if (header.getRecordLength() == 0) {
            throw std::runtime_error("Failed to read TOPAS header from file: " + filename);
        }
        TOPASFormat topasFormat = header.getTOPASFormat();
        FormatType format = (topasFormat == TOPASFormat::ASCII)
                        ? FormatType::ASCII
                        : FormatType::BINARY;
        return { format, std::move(header) };
    }

    Reader::Reader(const std::string &filename, const UserOptions &options)
        : Reader(filename, options, readHeader(filename))
    {}

    Reader::Reader(const std::string &filename, const UserOptions &options, std::pair<FormatType,Header> formatAndHeader)
    : PhaseSpaceFileReader(formatAndHeader.second.getTOPASFormatName(), filename, options, formatAndHeader.first),
      header_(std::move(formatAndHeader.second)),
      formatType_(header_.getTOPASFormat()),
      particleRecordLength_(header_.getRecordLength()),
      readFullDetails_(true),
      emptyHistoriesCount_(0)
    {}
    
    std::vector<CLICommand> Reader::getFormatSpecificCLICommands() { return {}; }

    Particle Reader::readASCIIParticle(const std::string & line)
    {
        std::istringstream iss(line);
        float x, y, z, u, v, energy, weight;
        std::int32_t typeCode;
        bool wIsNegative, isNewHistory;
        iss >> x >> y >> z >> u >> v >> energy >> weight >> typeCode >> wIsNegative >> isNewHistory;

        float uuvv = std::min(1.f, u*u+v*v);
        float w = std::sqrt(1.f - uuvv);
        if (wIsNegative) {
            w = -w; // restore w directional component sign
        }

        ParticleType particleType = getParticleTypeFromPDGID(typeCode);
        if (particleType == ParticleType::Unsupported) {
            throw std::runtime_error("Invalid particle type code in TOPAS ASCII phase space file: " + std::to_string(typeCode));
        }

        Particle particle(particleType, energy * MeV, x * cm, y * cm, z * cm, u, v, w, isNewHistory, weight);

        if (readFullDetails_) {
            const std::vector<Header::DataColumn> & columnTypes = header_.getColumnTypes();
            // skip the first 10 columns that we've already consumed
            for (size_t idx = 10; idx < columnTypes.size(); ++idx) {
                const Header::DataColumn & column = columnTypes[idx];
                switch (column.valueType_) {
                    case Header::DataType::BOOLEAN:
                        bool boolValue;
                        iss >> boolValue;
                        particle.setBoolProperty(BoolPropertyType::CUSTOM, boolValue);
                        break;
                    case Header::DataType::FLOAT32:
                        float floatValue;
                        iss >> floatValue;
                        particle.setFloatProperty(FloatPropertyType::CUSTOM, floatValue);
                        break;
                    case Header::DataType::FLOAT64:
                        double doubleValue;
                        iss >> doubleValue;
                        particle.setFloatProperty(FloatPropertyType::CUSTOM, static_cast<float>(doubleValue));
                        break;
                    case Header::DataType::INT8:
                        std::int8_t int8Value;
                        iss >> int8Value;
                        particle.setIntProperty(IntPropertyType::CUSTOM, static_cast<std::int32_t>(int8Value));
                        break;
                    case Header::DataType::INT32:
                        std::int32_t int32Value;
                        iss >> int32Value;
                        particle.setIntProperty(IntPropertyType::CUSTOM, int32Value);
                        break;
                    case Header::DataType::STRING:
                        {
                            std::string stringValue;
                            // read the string value, which is expected to be 22 characters long
                            stringValue.resize(22);
                            iss.read(stringValue.data(), 22);
                            particle.setStringProperty(stringValue);
                        }
                        break;
                    default:
                        throw std::runtime_error("Unknown column data type in TOPAS ASCII phase space file: " + std::to_string(static_cast<int>(column.columnType_)));
                }
            }
        }

        return particle;
    }

    Particle Reader::readBinaryStandardParticle(ByteBuffer & buffer)
    {
        float x = buffer.read<float>() * cm;
        float y = buffer.read<float>() * cm;
        float z = buffer.read<float>() * cm;
        float u = buffer.read<float>();
        float v = buffer.read<float>();
        float energy = buffer.read<float>() * MeV;
        float weight = buffer.read<float>();
        std::int32_t typeCode = buffer.read<std::int32_t>();

        if (typeCode == 0) {
            // special particle representing a sequence of empty histories
            if (weight >= 0) throw std::runtime_error("Invalid weight for pseudo particle in TOPAS binary file");
            Particle pseudoparticle(ParticleType::PseudoParticle, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, true, weight);
            int32_t extraHistories = roundToInt32(-pseudoparticle.getWeight());
            pseudoparticle.setIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER, extraHistories);
            return pseudoparticle;
        }

        ParticleType particleType = getParticleTypeFromPDGID(typeCode);
        if (particleType == ParticleType::Unsupported) {    
            throw std::runtime_error("Invalid particle type code in TOPAS phase space file: " + std::to_string(typeCode));
        }
        
        bool wIsNegative = buffer.read<bool>();
        bool isNewHistory = buffer.read<bool>();

        float uuvv = std::min(1.f, u*u+v*v);
        float w = std::sqrt(1.f - uuvv);
        if (wIsNegative) {
            w = -w; // restore w directional component sign
        }
        
        Particle particle(particleType, energy, x, y, z, u, v, w, isNewHistory, weight);

        if (readFullDetails_) {
            const std::vector<Header::DataColumn> & columnTypes = header_.getColumnTypes();
            // skip the first 10 columns that we've already consumed
            for (size_t idx = 10; idx < columnTypes.size(); ++idx) {
                const Header::DataColumn & column = columnTypes[idx];
                switch (column.valueType_) {
                    case Header::DataType::BOOLEAN:
                        particle.setBoolProperty(BoolPropertyType::CUSTOM, buffer.read<bool>());
                        break;
                    case Header::DataType::FLOAT32:
                        particle.setFloatProperty(FloatPropertyType::CUSTOM, buffer.read<float>());
                        break;
                    case Header::DataType::FLOAT64:
                        particle.setFloatProperty(FloatPropertyType::CUSTOM, (float)buffer.read<double>());
                        break;
                    case Header::DataType::INT8:
                        particle.setIntProperty(IntPropertyType::CUSTOM, (std::int32_t)buffer.read<std::int8_t>());
                        break;
                    case Header::DataType::INT32:
                        particle.setIntProperty(IntPropertyType::CUSTOM, buffer.read<std::int32_t>());
                        break;
                    default:
                        throw std::runtime_error("Unknown column data type in TOPAS phase space file: " + std::to_string(static_cast<int>(column.columnType_)));
                }
            }
        }
        
        return particle;
    }

    Particle Reader::readBinaryLimitedParticle(ByteBuffer & buffer)
    {
        std::int8_t particleType = buffer.read<std::int8_t>();
        float energy = buffer.read<float>() * MeV;
        float x = buffer.read<float>() * cm;
        float y = buffer.read<float>() * cm;
        float z = buffer.read<float>() * cm;
        float u = buffer.read<float>();
        float v = buffer.read<float>();
        float weight = buffer.read<float>();

        bool isNewHistory = energy < 0 ? true : false;
        if (isNewHistory) {
            energy = -energy; // restore energy
        }

        float uuvv = std::min(1.f, u*u+v*v);
        float w = std::sqrt(1.f - uuvv);
        if (particleType < 0) {
            w = -w; // restore w directional component sign
            particleType = -particleType; // restore particle type
        }

        ParticleType type;
        switch (particleType) {
            case 1: type = ParticleType::Photon; break;
            case 2: type = ParticleType::Electron; break;
            case 3: type = ParticleType::Positron; break;
            case 4: type = ParticleType::Neutron; break;
            case 5: type = ParticleType::Proton; break;
            default: throw std::runtime_error("Invalid particle type ("+std::to_string(particleType)+") in TOPAS Limited phase space file.");
        }

        return Particle(type, energy, x, y, z, u, v, w, isNewHistory, weight);
    }

    // Implementations for Writer

    inline FormatType getFormatTypeFromTOPASFormat(TOPASFormat format)
    {
        return (format == TOPASFormat::ASCII) ? FormatType::ASCII : FormatType::BINARY;
    }

    Writer::Writer(const std::string &filename, const UserOptions &options)
        : Writer(filename,
            options,
            [&]() {
                TOPASFormat format = TOPASFormat::BINARY; // default to BINARY
                if (options.contains(TOPASFormatCommand)) {
                    std::string formatStr = std::get<std::string>(options.at(TOPASFormatCommand).front());
                    if (formatStr == "ASCII") {
                        format = TOPASFormat::ASCII;
                    } else if (formatStr == "LIMITED") {
                        format = TOPASFormat::LIMITED;
                    } else if (formatStr == "BINARY") {
                        format = TOPASFormat::BINARY;
                    } else {
                        throw std::runtime_error("Invalid TOPAS format specified: " + formatStr);
                    }
                }
                return format;
            }())
    {}

    Writer::Writer(const std::string &filename, const UserOptions &options, TOPASFormat formatType)
        : PhaseSpaceFileWriter(Header::getTOPASFormatName(formatType), filename, options, getFormatTypeFromTOPASFormat(formatType)), formatType_(formatType), header_(filename, formatType)
    {}

    std::vector<CLICommand> Writer::getFormatSpecificCLICommands() { return { TOPASFormatCommand }; }

    void Writer::writeHeaderData(ByteBuffer & buffer)
    {
        (void)buffer; // unused in this implementation

        std::uint64_t historiesRecorded = getHistoriesWritten();
        std::uint64_t historiesInHeader = header_.getNumberOfOriginalHistories();
        if (historiesRecorded > historiesInHeader) {
            header_.setNumberOfOriginalHistories(historiesRecorded);
        }

        header_.writeHeader();
    }

    const std::string Writer::writeASCIIParticle(Particle & particle)
    {
        if (particle.getType() == ParticleType::Unsupported) {
            throw std::runtime_error("Attempting to write particle with unsupported type to TOPAS ASCII phase space file.");
        }

        if (particle.getType() == ParticleType::PseudoParticle) {
            header_.countParticleStats(particle);
            return ""; // write nothing for pseudoparticles
        }

        std::ostringstream oss;
        oss << std::setw(12) << particle.getX() / cm << " "
            << std::setw(12) << particle.getY() / cm << " "
            << std::setw(12) << particle.getZ() / cm << " "
            << std::setw(12) << particle.getDirectionalCosineX() << " "
            << std::setw(12) << particle.getDirectionalCosineY() << " "
            << std::setw(12) << particle.getKineticEnergy() / MeV << " "
            << std::setw(12) << particle.getWeight() << " "
            << std::setw(12) << getPDGIDFromParticleType(particle.getType()) << " "
            << std::setw(2) << (particle.getDirectionalCosineZ() < 0 ? 1 : 0) << " "
            << std::setw(2) << (particle.isNewHistory() ? 1 : 0);

        // Write any additional properties
        const std::vector<Header::DataColumn> & columnTypes = header_.getColumnTypes();
        if (columnTypes.size() > 10) {
            const std::vector<bool> & customBoolProperties = particle.getCustomBoolProperties();
            const std::vector<float> & customFloatProperties = particle.getCustomFloatProperties();
            const std::vector<std::int32_t> & customIntProperties = particle.getCustomIntProperties();
            const std::vector<std::string> & customStringProperties = particle.getCustomStringProperties();

            std::size_t customBoolIndex = 0;
            std::size_t customFloatIndex = 0;
            std::size_t customIntIndex = 0;
            std::size_t customStringIndex = 0;

            oss << " ";
            // skip the first 10 columns that we've already consumed
            for (std::size_t idx = 10; idx < columnTypes.size(); ++idx) {
                const Header::DataColumn & column = columnTypes[idx];
                switch (column.valueType_) {
                    case Header::DataType::STRING:
                        oss << std::setw(22) << customStringProperties[customStringIndex++].substr(0,22) << " ";
                        break;
                    case Header::DataType::BOOLEAN:
                        oss << std::setw(2) << (customBoolProperties[customBoolIndex++] ? 1 : 0) << " ";
                        break;
                    case Header::DataType::INT8:
                        {
                            std::int8_t customIntValue = static_cast<std::int8_t>(customIntProperties[customIntIndex++]);
                            oss << std::setw(12) << static_cast<int>(customIntValue) << " ";
                        }
                        break;
                    case Header::DataType::INT32:
                        oss << std::setw(12) << customIntProperties[customIntIndex++] << " ";
                        break;
                    case Header::DataType::FLOAT32:
                        oss << std::setw(12) << customFloatProperties[customFloatIndex++] << " ";
                        break;
                    case Header::DataType::FLOAT64:
                        oss << std::setw(12) << customFloatProperties[customFloatIndex++] << " ";
                        break;
                }
            }
        }
        oss << std::endl;

        header_.countParticleStats(particle);

        return oss.str();
    }

    void Writer::writeBinaryStandardParticle(ByteBuffer & buffer, Particle & particle)
    {
        if (particle.getType() == ParticleType::PseudoParticle) {
            // special particle representing a sequence of empty histories
            float weight = particle.getWeight();
            if (weight >= 0) throw std::runtime_error("Attempted to write invalid weight for pseudo particle in TOPAS binary file");
            buffer.write<float>(0.f);
            buffer.write<float>(0.f);
            buffer.write<float>(0.f);
            buffer.write<float>(0.f);
            buffer.write<float>(0.f);
            buffer.write<float>(0.f);
            buffer.write<float>(weight);
            buffer.write<std::int32_t>(0);
            buffer.write<bool>(false);
            buffer.write<bool>(true);
        } else {
            buffer.write(particle.getX() / cm);
            buffer.write(particle.getY() / cm);
            buffer.write(particle.getZ() / cm);
            buffer.write(particle.getDirectionalCosineX());
            buffer.write(particle.getDirectionalCosineY());
            buffer.write(particle.getKineticEnergy() / MeV);
            buffer.write(particle.getWeight());
            buffer.write(getPDGIDFromParticleType(particle.getType()));
            buffer.write(particle.getDirectionalCosineZ() < 0 ? true : false);
            buffer.write(particle.isNewHistory());
        }

        // Write any additional properties
        const std::vector<Header::DataColumn> & columnTypes = header_.getColumnTypes();
        if (columnTypes.size() > 10) {
            const std::vector<bool> & customBoolProperties = particle.getCustomBoolProperties();
            const std::vector<float> & customFloatProperties = particle.getCustomFloatProperties();
            const std::vector<std::int32_t> & customIntProperties = particle.getCustomIntProperties();
            const std::vector<std::string> & customStringProperties = particle.getCustomStringProperties();

            std::size_t customBoolIndex = 0;
            std::size_t customFloatIndex = 0;
            std::size_t customIntIndex = 0;
            std::size_t customStringIndex = 0;

            // skip the first 10 columns that we've already consumed
            for (std::size_t idx = 10; idx < columnTypes.size(); ++idx) {
                const Header::DataColumn & column = columnTypes[idx];
                switch (column.valueType_) {
                    case Header::DataType::STRING:
                    {
                        const std::string & customString = customStringIndex < customStringProperties.size() ? customStringProperties[customStringIndex++] : "";
                        buffer.writeString(customString);
                        break;
                    }
                    case Header::DataType::BOOLEAN:
                    {
                        const bool customBool = customBoolIndex < customBoolProperties.size() ? customBoolProperties[customBoolIndex++] : false;
                        buffer.write(static_cast<bool>(customBool));
                        break;
                    }
                    case Header::DataType::INT8:
                    {
                        const std::int8_t customInt8 = customIntIndex < customIntProperties.size() ? static_cast<std::int8_t>(customIntProperties[customIntIndex++]) : 0;
                        buffer.write(customInt8);
                        break;
                    }
                    case Header::DataType::INT32:
                    {
                        const std::int32_t customInt32 = customIntIndex < customIntProperties.size() ? static_cast<std::int32_t>(customIntProperties[customIntIndex++]) : 0;
                        buffer.write(customInt32);
                        break;
                    }
                    case Header::DataType::FLOAT32:
                    {
                        const float customFloat32 = customFloatIndex < customFloatProperties.size() ? customFloatProperties[customFloatIndex++] : 0.0f;
                        buffer.write(customFloat32);
                        break;
                    }
                    case Header::DataType::FLOAT64:
                    {
                        const double customFloat64 = customFloatIndex < customFloatProperties.size() ? static_cast<double>(customFloatProperties[customFloatIndex++]) : 0.0;
                        buffer.write(customFloat64);
                        break;
                    }
                }
            }
        }
    }

    void Writer::writeBinaryLimitedParticle(ByteBuffer & buffer, Particle & particle)
    {
        float energy = particle.getKineticEnergy() / MeV;
        if (particle.isNewHistory()) {
            energy = -energy;
        }

        std::int32_t pdgID = getPDGIDFromParticleType(particle.getType());
        std::int8_t particleTypeCode;
        switch (pdgID) {
            case 22: particleTypeCode = 1; break; // Photon
            case 11: particleTypeCode = 2; break; // Electron
            case -11: particleTypeCode = 3; break; // Positron
            case 2112: particleTypeCode = 4; break; // Neutron
            case 2212: particleTypeCode = 5; break; // Proton
            default:
                std::string particleTypeName = std::string(getParticleTypeName(particle.getType()));
                throw std::runtime_error("Attempted to write particle type '" + particleTypeName + "' (" + std::to_string(pdgID) + ") which is not compatible with a TOPAS Limited phase space file.");
        }
        if (particle.getDirectionalCosineZ() < 0) {
            particleTypeCode = -particleTypeCode;
        }

        buffer.write(particleTypeCode);
        buffer.write(energy);
        buffer.write(particle.getX() / cm);
        buffer.write(particle.getY() / cm);
        buffer.write(particle.getZ() / cm);
        buffer.write(particle.getDirectionalCosineX());
        buffer.write(particle.getDirectionalCosineY());
        buffer.write(particle.getWeight());
    }

} // namespace ParticleZoo::TOPASphsp