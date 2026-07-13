
#include "particlezoo/MCPL/MCPLphspFile.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

#include "particlezoo/ByteBuffer.h"
#include "particlezoo/utilities/version.h"

namespace ParticleZoo::MCPLphspFile
{

    CLICommand MCPLPolarisationCommand{ WRITER, "", "MCPL-polarisation", "Store the particle polarisation vectors in the MCPL file (taken from the POLARIZATION_X/Y/Z particle properties)", { CLI_VALUELESS } };
    CLICommand MCPLUserFlagsCommand{ WRITER, "", "MCPL-userflags", "Store the particle user flags values in the MCPL file (taken from the MCPL_USERFLAGS particle property)", { CLI_VALUELESS } };
    CLICommand MCPLUniversalWeightCommand{ WRITER, "", "MCPL-universal-weight", "Store a single common weight in the MCPL header instead of per-particle weights (all particles must have this weight)", { CLI_FLOAT } };
    CLICommand MCPLUniversalPDGCommand{ WRITER, "", "MCPL-universal-pdgcode", "Store a single common PDG code in the MCPL header instead of per-particle codes (all particles must have this type)", { CLI_INT } };
    CLICommand MCPLCommentCommand{ WRITER, "", "MCPL-comment", "Add a comment to the MCPL file header", { CLI_STRING } };


    // Reader class implementation

    Reader::Reader(const std::string & fileName, const UserOptions & options)
    : PhaseSpaceFileReader("MCPL", fileName, options)
    {
        readHeader();
    }

    std::vector<CLICommand> Reader::getFormatSpecificCLICommands() {
        return {};
    }

    void Reader::readHeader()
    {
        const std::uint64_t fileSize = getFileSize();
        if (fileSize < MCPL_FIXED_HEADER_LENGTH) {
            throw std::runtime_error("Invalid MCPL file (file is too small): " + getFileName());
        }

        // Magic, format version (three ASCII digits) and endianness marker
        ByteBuffer header = getHeaderData(MCPL_NPARTICLES_OFFSET);
        std::string magic = header.readString(MCPL_MAGIC.size());
        if (magic != MCPL_MAGIC) {
            throw std::runtime_error("Invalid MCPL file (wrong magic bytes; note that gzip-compressed .mcpl.gz files must be decompressed first): " + getFileName());
        }
        std::string versionDigits = header.readString(3);
        if (versionDigits.find_first_not_of("0123456789") != std::string::npos) {
            throw std::runtime_error("Invalid MCPL file (malformed format version field): " + getFileName());
        }
        formatVersion_ = static_cast<unsigned int>(std::stoul(versionDigits));
        if (formatVersion_ != MCPL_FORMAT_VERSION_CURRENT && formatVersion_ != MCPL_FORMAT_VERSION_LEGACY) {
            throw std::runtime_error("Unsupported MCPL file (format version " + std::to_string(formatVersion_) + "; only versions 2 and 3 are supported): " + getFileName());
        }
        byte endianMarker = header.read<byte>();
        if (endianMarker == 'L') {
            setByteOrder(ByteOrder::LittleEndian);
        } else if (endianMarker == 'B') {
            setByteOrder(ByteOrder::BigEndian);
        } else {
            throw std::runtime_error("Invalid MCPL file (unrecognized endianness marker): " + getFileName());
        }

        // Particle count and the eight option values, in the file's byte order
        std::size_t bufferedSize = static_cast<std::size_t>(std::min<std::uint64_t>(fileSize, 4096));
        header = getHeaderData(bufferedSize);
        header.moveTo(MCPL_NPARTICLES_OFFSET);
        numberOfParticles_ = header.read<std::uint64_t>();

        std::array<std::uint32_t, 8> optionValues;
        for (auto & value : optionValues) value = header.read<std::uint32_t>();
        std::uint32_t numberOfComments = optionValues[OPT_NCOMMENTS];
        std::uint32_t numberOfBlobs    = optionValues[OPT_NBLOBS];
        hasUserFlags_       = optionValues[OPT_HAS_USERFLAGS] != 0;
        hasPolarisation_    = optionValues[OPT_HAS_POLARISATION] != 0;
        singlePrecision_    = optionValues[OPT_SINGLE_PRECISION] != 0;
        universalPDGCode_   = static_cast<std::int32_t>(optionValues[OPT_UNIVERSAL_PDGCODE]);
        particleSize_       = optionValues[OPT_PARTICLE_SIZE];
        hasUniversalWeight_ = optionValues[OPT_HAS_UNIVERSAL_WEIGHT] != 0;

        // Each comment, blob key and blob occupies at least its 4-byte length
        // prefix, which bounds any legitimate count on a corrupt file
        if (numberOfComments > fileSize / sizeof(std::uint32_t) || numberOfBlobs > fileSize / (2 * sizeof(std::uint32_t))) {
            throw std::runtime_error("Invalid MCPL file (comment or blob count exceeds file size): " + getFileName());
        }

        std::size_t offset = MCPL_FIXED_HEADER_LENGTH;
        if (hasUniversalWeight_) {
            universalWeight_ = header.read<double>();
            offset += sizeof(double);
        }

        std::size_t expectedParticleSize = MCPLParticleRecordLength(singlePrecision_, hasPolarisation_,
                                                                    hasUniversalWeight_, universalPDGCode_ != 0,
                                                                    hasUserFlags_);
        if (particleSize_ != expectedParticleSize) {
            throw std::runtime_error("Invalid MCPL file (the stored particle record length " + std::to_string(particleSize_)
                                     + " does not match the " + std::to_string(expectedParticleSize)
                                     + " bytes implied by the header options): " + getFileName());
        }

        // Makes sure that at least bytesNeeded more bytes of header data are buffered,
        // re-reading a larger portion of the file if necessary.
        auto ensureBuffered = [&](std::size_t bytesNeeded) {
            if (offset + bytesNeeded > bufferedSize) {
                if (offset + bytesNeeded > fileSize) {
                    throw std::runtime_error("Invalid MCPL file (unexpected end of file while reading the header): " + getFileName());
                }
                bufferedSize = offset + bytesNeeded;
                header = getHeaderData(bufferedSize);
            }
            header.moveTo(offset);
        };

        // Reads one length-prefixed item (a uint32 byte count followed by the raw
        // bytes), returning its content or skipping over it.
        auto readLengthPrefixed = [&](bool skipContent) -> std::string {
            ensureBuffered(sizeof(std::uint32_t));
            std::uint32_t itemLength = header.read<std::uint32_t>();
            offset += sizeof(std::uint32_t);
            ensureBuffered(itemLength);
            std::string content;
            if (!skipContent) {
                content = header.readString(itemLength);
            }
            offset += itemLength;
            return content;
        };

        // Source program name, comments, blob keys and blobs; the blobs are not
        // retained but must be walked past to find where particle records begin
        srcProgName_ = readLengthPrefixed(false);
        comments_.reserve(numberOfComments);
        for (std::uint32_t i = 0; i < numberOfComments; ++i) {
            comments_.push_back(readLengthPrefixed(false));
        }
        for (std::uint32_t i = 0; i < numberOfBlobs; ++i) {
            readLengthPrefixed(true);   // blob key
        }
        for (std::uint32_t i = 0; i < numberOfBlobs; ++i) {
            readLengthPrefixed(true);   // blob content
        }
        headerSize_ = offset;

        // A zero particle count marks a file that was never properly closed; the
        // count is then recovered from the file size. A count beyond what the file
        // can hold marks a truncated file.
        std::uint64_t maximumParticlesInFile = (fileSize - headerSize_) / particleSize_;
        if (numberOfParticles_ == 0) {
            numberOfParticles_ = maximumParticlesInFile;
        } else if (numberOfParticles_ > maximumParticlesInFile) {
            throw std::runtime_error("Invalid MCPL file (the header declares " + std::to_string(numberOfParticles_)
                                     + " particles but the file can hold at most " + std::to_string(maximumParticlesInFile)
                                     + "): " + getFileName());
        }
    }

    Particle Reader::readBinaryParticle(ByteBuffer & buffer)
    {
        auto readFP = [&]() -> double {
            return singlePrecision_ ? static_cast<double>(buffer.read<float>()) : buffer.read<double>();
        };

        float polX = 0.f, polY = 0.f, polZ = 0.f;
        if (hasPolarisation_) {
            polX = static_cast<float>(readFP());
            polY = static_cast<float>(readFP());
            polZ = static_cast<float>(readFP());
        }

        double x = readFP() * cm;
        double y = readFP() * cm;
        double z = readFP() * cm;

        std::array<double, 3> packed;
        packed[0] = readFP();
        packed[1] = readFP();
        packed[2] = readFP() * MeV;

        double time = readFP() * ms;
        double weight = hasUniversalWeight_ ? universalWeight_ : readFP();
        std::int32_t pdgCode = (universalPDGCode_ != 0) ? universalPDGCode_ : buffer.read<std::int32_t>();
        std::uint32_t userFlags = hasUserFlags_ ? buffer.read<std::uint32_t>() : 0;

        double u, v, w, kineticEnergy;
        if (formatVersion_ == MCPL_FORMAT_VERSION_LEGACY) {
            MCPLUnpackDirectionAndEnergyLegacy(packed, u, v, w, kineticEnergy);
        } else {
            MCPLUnpackDirectionAndEnergy(packed, u, v, w, kineticEnergy);
        }

        ParticleType type = getParticleTypeFromPDGID(pdgCode);
        if (type == ParticleType::Unsupported) {
            throw std::runtime_error("MCPL PDG code " + std::to_string(pdgCode) + " could not be decoded to a supported particle type.");
        }

        // MCPL files carry no history structure, so every particle is its own history
        Particle particle(type, kineticEnergy, x, y, z, u, v, w, true, weight);

        particle.setFloatProperty(FloatPropertyType::TIME, static_cast<float>(time));

        if (hasPolarisation_) {
            particle.setFloatProperty(FloatPropertyType::POLARIZATION_X, polX);
            particle.setFloatProperty(FloatPropertyType::POLARIZATION_Y, polY);
            particle.setFloatProperty(FloatPropertyType::POLARIZATION_Z, polZ);
        }

        if (hasUserFlags_) {
            particle.setIntProperty(IntPropertyType::MCPL_USERFLAGS, static_cast<std::int32_t>(userFlags));
        }

        return particle;
    }


    // Writer class implementation

    namespace {
        // Writes a string as a uint32 byte count followed by the raw bytes
        // (no null terminator, no padding).
        void writeLengthPrefixedString(ByteBuffer & buffer, const std::string & value) {
            buffer.write<std::uint32_t>(static_cast<std::uint32_t>(value.size()));
            buffer.writeString(value);
        }
    }

    Writer::Writer(const std::string & fileName, const UserOptions & options)
    : PhaseSpaceFileWriter("MCPL", fileName, options),
      srcProgName_(Version::GetVersionString())
    {
        polarisation_ = options.contains(MCPLPolarisationCommand);
        userFlags_ = options.contains(MCPLUserFlagsCommand);

        if (options.contains(MCPLUniversalWeightCommand)) {
            hasUniversalWeight_ = true;
            universalWeight_ = static_cast<double>(options.extractFloatOption(MCPLUniversalWeightCommand));
            if (!(universalWeight_ > 0.0)) {
                throw std::runtime_error("The MCPL universal weight must be a positive number.");
            }
        }

        if (options.contains(MCPLUniversalPDGCommand)) {
            universalPDGCode_ = options.extractIntOption(MCPLUniversalPDGCommand);
            if (universalPDGCode_ == 0) {
                throw std::runtime_error("The MCPL universal PDG code must not be zero.");
            }
        }

        for (const CLIValue & value : options.extractValues(MCPLCommentCommand)) {
            if (auto * comment = std::get_if<std::string>(&value)) {
                comments_.push_back(*comment);
            }
        }

        // The header and record sizes must stay fixed from here on: the base class
        // seeks past the header before the first particle is written, and the
        // header itself (written when the file is closed) has no padding.
        particleSize_ = MCPLParticleRecordLength(true, polarisation_, hasUniversalWeight_, universalPDGCode_ != 0, userFlags_);
        headerSize_ = MCPL_FIXED_HEADER_LENGTH
                    + (hasUniversalWeight_ ? sizeof(double) : 0)
                    + sizeof(std::uint32_t) + srcProgName_.size();
        for (const std::string & comment : comments_) {
            headerSize_ += sizeof(std::uint32_t) + comment.size();
        }
    }

    std::vector<CLICommand> Writer::getFormatSpecificCLICommands() {
        return { MCPLPolarisationCommand, MCPLUserFlagsCommand, MCPLUniversalWeightCommand, MCPLUniversalPDGCommand, MCPLCommentCommand };
    }

    void Writer::writeHeaderData(ByteBuffer & buffer)
    {
        char endianMarker;
        switch (buffer.getByteOrder()) {
            case ByteOrder::LittleEndian: endianMarker = 'L'; break;
            case ByteOrder::BigEndian:    endianMarker = 'B'; break;
            default: throw std::runtime_error("The MCPL format does not support the host byte order.");
        }

        std::string versionDigits;
        versionDigits.push_back(static_cast<char>('0' + (MCPL_FORMAT_VERSION_CURRENT / 100) % 10));
        versionDigits.push_back(static_cast<char>('0' + (MCPL_FORMAT_VERSION_CURRENT / 10) % 10));
        versionDigits.push_back(static_cast<char>('0' + MCPL_FORMAT_VERSION_CURRENT % 10));

        buffer.writeString(std::string(MCPL_MAGIC));
        buffer.writeString(versionDigits);
        buffer.write<byte>(static_cast<byte>(endianMarker));
        buffer.write<std::uint64_t>(getParticlesWritten());
        buffer.write<std::uint32_t>(static_cast<std::uint32_t>(comments_.size()));    // number of comments
        buffer.write<std::uint32_t>(0);                                               // number of blobs
        buffer.write<std::uint32_t>(userFlags_ ? 1 : 0);
        buffer.write<std::uint32_t>(polarisation_ ? 1 : 0);
        buffer.write<std::uint32_t>(1);                                               // single precision
        buffer.write<std::uint32_t>(static_cast<std::uint32_t>(universalPDGCode_));
        buffer.write<std::uint32_t>(static_cast<std::uint32_t>(particleSize_));
        buffer.write<std::uint32_t>(hasUniversalWeight_ ? 1 : 0);
        if (hasUniversalWeight_) {
            buffer.write<double>(universalWeight_);
        }
        writeLengthPrefixedString(buffer, srcProgName_);
        for (const std::string & comment : comments_) {
            writeLengthPrefixedString(buffer, comment);
        }

        // The MCPL header has no padding, so any shortfall against the particle
        // record start offset (which the base class would pad with zeros) means
        // the file would be corrupt
        if (buffer.length() != headerSize_) {
            throw std::runtime_error("The MCPL header does not have the expected size (" + std::to_string(buffer.length())
                                     + " bytes written, expected " + std::to_string(headerSize_) + ").");
        }
    }

    void Writer::writeBinaryParticle(ByteBuffer & buffer, Particle & particle)
    {
        constexpr float inv_cm = 1.0f / cm;
        constexpr float inv_MeV = 1.0f / MeV;
        constexpr float inv_ms = 1.0f / ms;

        double kineticEnergy = static_cast<double>(particle.getKineticEnergy());
        if (!(kineticEnergy >= 0.0)) {
            throw std::runtime_error("The MCPL format cannot store a particle with a negative kinetic energy.");
        }

        auto packed = MCPLPackDirectionAndEnergy(particle.getDirectionalCosineX(),
                                                 particle.getDirectionalCosineY(),
                                                 particle.getDirectionalCosineZ(),
                                                 kineticEnergy);

        if (polarisation_) {
            buffer.write<float>(particle.hasFloatProperty(FloatPropertyType::POLARIZATION_X) ? particle.getFloatProperty(FloatPropertyType::POLARIZATION_X) : 0.f);
            buffer.write<float>(particle.hasFloatProperty(FloatPropertyType::POLARIZATION_Y) ? particle.getFloatProperty(FloatPropertyType::POLARIZATION_Y) : 0.f);
            buffer.write<float>(particle.hasFloatProperty(FloatPropertyType::POLARIZATION_Z) ? particle.getFloatProperty(FloatPropertyType::POLARIZATION_Z) : 0.f);
        }

        buffer.write<float>(particle.getX() * inv_cm);
        buffer.write<float>(particle.getY() * inv_cm);
        buffer.write<float>(particle.getZ() * inv_cm);

        buffer.write<float>(static_cast<float>(packed[0]));
        buffer.write<float>(static_cast<float>(packed[1]));
        buffer.write<float>(static_cast<float>(packed[2] * inv_MeV));

        float time = particle.hasFloatProperty(FloatPropertyType::TIME)
                   ? particle.getFloatProperty(FloatPropertyType::TIME)
                   : 0.f;
        buffer.write<float>(time * inv_ms);

        if (hasUniversalWeight_) {
            if (static_cast<double>(particle.getWeight()) != universalWeight_) {
                throw std::runtime_error("A particle with weight " + std::to_string(particle.getWeight())
                                         + " cannot be stored in an MCPL file with a universal weight of " + std::to_string(universalWeight_) + ".");
            }
        } else {
            buffer.write<float>(particle.getWeight());
        }

        std::int32_t pdgCode = particle.getPDGCode();
        if (universalPDGCode_ != 0) {
            if (pdgCode != universalPDGCode_) {
                throw std::runtime_error("A particle with PDG code " + std::to_string(pdgCode)
                                         + " cannot be stored in an MCPL file with a universal PDG code of " + std::to_string(universalPDGCode_) + ".");
            }
        } else {
            if (pdgCode == 0) {
                throw std::runtime_error("Particle type " + std::string(getParticleTypeName(particle.getType())) + " has no PDG code and cannot be stored in an MCPL file.");
            }
            buffer.write<std::int32_t>(pdgCode);
        }

        if (userFlags_) {
            std::uint32_t userFlagsValue = particle.hasIntProperty(IntPropertyType::MCPL_USERFLAGS)
                                         ? static_cast<std::uint32_t>(particle.getIntProperty(IntPropertyType::MCPL_USERFLAGS))
                                         : 0u;
            buffer.write<std::uint32_t>(userFlagsValue);
        }
    }

}
