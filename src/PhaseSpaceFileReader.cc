
#include "particlezoo/PhaseSpaceFileReader.h"

namespace ParticleZoo
{
    PhaseSpaceFileReader::PhaseSpaceFileReader(const std::string & phspFormat, const std::string & fileName, const UserOptions & userOptions, FormatType formatType, unsigned int bufferSize)
    :   phspFormat_(phspFormat),
        fileName_(fileName),
        userOptions_(userOptions),
        formatType_(formatType),
        BUFFER_SIZE(bufferSize),
        file_([&]() {
                if (formatType_ == FormatType::NONE)
                    return std::ifstream{};
                else
                    return std::ifstream(fileName_, std::ios::binary);
            }()),
        asciiCommentMarkers_({"#", "//"}),
        bytesInFile_([this]() -> std::uint64_t {
                if (formatType_ == FormatType::NONE) {
                    return 0; // For NONE format, we assume no data to read
                }
                if (!file_.is_open())
                {
                    throw std::runtime_error("Failed to open file: " + fileName_);
                }
                file_.seekg(0, std::ios::end);
                auto size = file_.tellg();
                if (size < 0)
                {
                    throw std::runtime_error("Failed to determine file size for: " + fileName_);
                }
                return static_cast<std::uint64_t>(size);
            }()),
        bytesRead_(0),
        particlesRead_(0),
        historiesRead_(0),
        numberOfParticlesToRead_(0),
        particleRecordLength_(0),
        buffer_(BUFFER_SIZE)
    {
        if (formatType != FormatType::NONE) {
            file_.seekg(0);
        }
    }

    PhaseSpaceFileReader::~PhaseSpaceFileReader() {
        close();
    }

    void PhaseSpaceFileReader::close() {
        if (file_.is_open()) {
            file_.close();
        }
    }

    const ByteBuffer PhaseSpaceFileReader::getHeaderData() {
        std::size_t headerSize = getParticleRecordStartOffset();
        if (headerSize == 0 || formatType_ == FormatType::NONE) {
            return ByteBuffer(0, buffer_.getByteOrder());
        }

        // Ensure the file is open before trying to read
        if (!file_.is_open()) {
            throw std::runtime_error("File is not open when attempting to read header data.");
        }

        std::streampos currentPos = file_.tellg();

        file_.seekg(0);
        ByteBuffer firstBlock(headerSize, buffer_.getByteOrder());
        
        // Attempt to read header data and capture actual bytes read.
        std::size_t bytesReadHeader = firstBlock.appendData(file_);
        if (bytesReadHeader < headerSize) {
            throw std::runtime_error("Insufficient header data: expected " +
                                     std::to_string(headerSize) + " bytes, got " +
                                     std::to_string(bytesReadHeader) + " bytes.");
        }

        // Reset file position to where it was before reading header data.
        file_.seekg(currentPos);
        return firstBlock;
    }

    void PhaseSpaceFileReader::readNextBlock() {
        if (formatType_ == FormatType::NONE) return;
        if (bytesRead_ >= bytesInFile_) {
            throw std::runtime_error("No more data to read.");
        }

        std::size_t particleRecordStartOffset = getParticleRecordStartOffset();

        if (bytesRead_ < particleRecordStartOffset) {
            // Skip to the start of the next particle record
            file_.seekg(particleRecordStartOffset);
            if (file_.eof() || file_.fail()) {
                throw std::runtime_error("Failed to seek to particle record start offset in file: " + fileName_);
            }
            bytesRead_ = particleRecordStartOffset;
            buffer_.clear();
        }

        // shift any unread data to the front of the buffer
        buffer_.compact();

        // Read blockSize bytes (or less at EOF) directly into buffer_
        std::size_t bytesThisRead = buffer_.appendData(file_);
        if (bytesThisRead == 0) {
            throw std::runtime_error("Failed to read any data from file: " + fileName_);
        }
        bytesRead_ += bytesThisRead;
    }

    bool PhaseSpaceFileReader::hasMoreParticles() {
        if (numberOfParticlesToRead_ == 0) numberOfParticlesToRead_ = getNumberOfParticles();
        if (particlesRead_ >= numberOfParticlesToRead_) {
            return false; // No more particles to read
        }
        switch (formatType_) {
            case FormatType::BINARY:
                return bytesInFile_ - bytesRead_ + buffer_.remainingToRead() >= getParticleRecordLength();
            case FormatType::ASCII:
                if (asciiLineBuffer_.empty()) {
                    bufferNextASCIILine();
                }
                return !asciiLineBuffer_.empty();
            default:
                return true; // For NONE format, previous check of particlesRead_ >= numberOfParticlesToRead_ is sufficient
        }
    }

    void PhaseSpaceFileReader::bufferNextASCIILine() {
        std::size_t remainingToRead = buffer_.remainingToRead();
        std::size_t maxASCIILength = getMaximumASCIILineLength();

        if (remainingToRead < maxASCIILength && bytesRead_ < bytesInFile_) {
            readNextBlock();
        }

        std::string line;
        size_t pos;
        bool isComment = false;
        while (buffer_.remainingToRead() > 0 && (line.empty() || isComment)) {
            if (buffer_.remainingToRead() < maxASCIILength && bytesRead_ < bytesInFile_) {
                readNextBlock();
            }
            line = buffer_.readLine();
            pos = line.find_first_not_of(" \t");
            if (pos == std::string::npos) {
                line.clear(); // Empty line, continue to next iteration
                continue;
            }
            isComment = false;
            for (auto& marker : asciiCommentMarkers_) {
                auto mlen = marker.size();
                if (line.size() >= pos + mlen &&
                    std::memcmp(line.data() + pos, marker.data(), mlen) == 0)
                {
                    isComment = true;
                    break;
                }
            }
        }

        if (!line.empty()) {
            asciiLineBuffer_.push_back(line);
        }
    }

    Particle PhaseSpaceFileReader::getNextParticle(bool countParticleInStatistics) {
        switch (formatType_) {
            
        case (FormatType::BINARY): // Binary format
            {
                if (!hasMoreParticles()) {
                    throw std::runtime_error("No more particles to read.");
                }
                
                if (particleRecordLength_ == 0) particleRecordLength_ = getParticleRecordLength();
                // Read the next block of data if necessary
                if (buffer_.length() == 0 || buffer_.remainingToRead() < particleRecordLength_) {
                    readNextBlock();
                }

                // Get a view of the next particle record in the buffer
                std::span<const ParticleZoo::byte> recordView = buffer_.readBytes(particleRecordLength_);
                ByteBuffer particleData(recordView, buffer_.getByteOrder());
                
                // Read the next particle from the buffer
                Particle particle = readBinaryParticle(particleData);
                if (countParticleInStatistics) {
                    particlesRead_++;
                    if (particle.isNewHistory()) {
                        if (particle.hasIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER)) {
                            int deltaN = (int) particle.getIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER);
                            if (deltaN > 0) {
                                historiesRead_ += static_cast<std::uint64_t>(deltaN);
                            } else {
                                historiesRead_++;
                            }
                        } else {
                            historiesRead_++;
                        }
                    }
                }
                return particle;
            }
            break;
        case (FormatType::ASCII): // ASCII format
            {
                if (!hasMoreParticles()) {
                    throw std::runtime_error("No more particles to read.");
                }
                try {
                    bufferNextASCIILine();
                    std::string line = std::move(asciiLineBuffer_.front());
                    asciiLineBuffer_.pop_front();
                    Particle particle = readASCIIParticle(line);
                    if (countParticleInStatistics) {
                        particlesRead_++;
                        if (particle.isNewHistory()) {
                            if (particle.hasIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER)) {
                                int deltaN = (int) particle.getIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER);
                                if (deltaN > 0) {
                                    historiesRead_ += static_cast<std::uint64_t>(deltaN);
                                } else {
                                    historiesRead_++;
                                }
                            } else {
                                historiesRead_++;
                            }
                        }
                    }
                    return particle;
                } catch (const std::runtime_error &e) {
                    throw std::runtime_error("Error reading line from file: " + std::string(e.what()));
                }
            }
            break;
        default: // NONE format
            {
                // For NONE format, all I/O needs to be implemented manually by the subclass.
                Particle particle = readParticleManually();
                if (countParticleInStatistics) {
                    particlesRead_++;
                    if (particle.isNewHistory()) {
                        if (particle.hasIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER)) {
                            int deltaN = (int) particle.getIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER);
                            if (deltaN > 0) {
                                historiesRead_ += static_cast<std::uint64_t>(deltaN);
                            } else {
                                historiesRead_++;
                            }
                        } else {
                            historiesRead_++;
                        }
                    }
                }
                return particle;
            }
            break;

        }
    }

}