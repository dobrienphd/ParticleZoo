
#include "particlezoo/PhaseSpaceFileReader.h"

namespace ParticleZoo
{

    std::vector<CLICommand> PhaseSpaceFileReader::getCLICommands() {
        return {};
    }

    PhaseSpaceFileReader::PhaseSpaceFileReader(const std::string & phspFormat, const std::string & fileName, const UserOptions & userOptions, FormatType formatType, const FixedValues fixedValues, unsigned int bufferSize)
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
        metaparticlesRead_(0),
        particlesSkipped_(0),
        historiesRead_(0),
        numberOfParticlesToRead_(0),
        particleRecordLength_(0),
        isFirstParticle_(true),
        buffer_(BUFFER_SIZE),
        readParticleDepth_(0),
        fixedValues_(fixedValues)
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

    void PhaseSpaceFileReader::moveToParticle(std::uint64_t particleIndex) {
        // Validate input
        if (formatType_ == FormatType::NONE) {
            throw std::runtime_error("moveToParticle is not supported for NONE format.");
        }
        if (particleIndex >= getNumberOfEntriesInFile()) {
            throw std::out_of_range("Particle index out of range.");
        }

        // Reset reading state
        file_.clear(); // Clear any EOF or fail flags
        buffer_.clear();
        asciiLineBuffer_.clear();
        numberOfParticlesToRead_ = 0;

        if (formatType_ == FormatType::BINARY) {
            // Calculate byte offset to seek to
            std::size_t particleRecordStartOffset = getParticleRecordStartOffset();
            std::size_t particleRecordLength = getParticleRecordLength();
            std::size_t bytesToSkip = particleRecordStartOffset + static_cast<std::size_t>(particleIndex) * particleRecordLength;

            // Seek to the calculated position
            if (bytesToSkip + particleRecordLength > bytesInFile_) {
                throw std::out_of_range("Attempted to seek beyond end of file.");
            }
            file_.seekg(bytesToSkip, std::ios::beg);
            if (file_.fail()) {
                throw std::runtime_error("Failed to seek to particle index " + std::to_string(particleIndex) + " in file: " + fileName_);
            }

            bytesRead_ = bytesToSkip;
        } else if (formatType_ == FormatType::ASCII) {
            // For ASCII, we need to read lines until we reach the desired particle index
            file_.seekg(0, std::ios::beg);
            if (file_.fail()) {
                throw std::runtime_error("Failed to seek to beginning of file: " + fileName_);
            }
            // reset counters
            bytesRead_  = 0;
            particlesRead_ = 0;
            historiesRead_ = 0;
            for (std::size_t p = 0 ; p < particleIndex ; p++) {
                getNextParticle(false); // Read without counting in statistics
            }
        } else {
            throw std::runtime_error("Unsupported format type for moveToParticle."); // Just for safety
        }

        particlesRead_ = particleIndex;
        particlesSkipped_ = particleIndex;
        metaparticlesRead_ = 0;
        // historiesRead_ restarts from the seek point; a mid-file seek target is not
        // "the first particle of the file" so only re-arm the flag when seeking to the start
        historiesRead_ = 0;
        isFirstParticle_ = (particleIndex == 0);
    }

    const ByteBuffer PhaseSpaceFileReader::getHeaderData() {
        std::size_t headerSize = getParticleRecordStartOffset();
        return getHeaderData(headerSize);
    }

    const ByteBuffer PhaseSpaceFileReader::getHeaderData(std::size_t headerSize) {
        if (headerSize == 0 || formatType_ == FormatType::NONE) {
            return ByteBuffer(std::span<const byte>{}, buffer_.getByteOrder());
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
        if (numberOfParticlesToRead_ == 0) numberOfParticlesToRead_ = getNumberOfEntriesInFile();

        std::uint64_t legitParticlesRead = particlesRead_ - metaparticlesRead_;
        std::uint64_t nominalTotalParticles = getNumberOfParticles();

        if (legitParticlesRead >= nominalTotalParticles || particlesRead_ >= numberOfParticlesToRead_) {
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
            // At end-of-file, allow a final line without a trailing newline
            line = buffer_.readLine(bytesRead_ >= bytesInFile_);
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

    Particle PhaseSpaceFileReader::readBinaryRecord(std::span<const byte> record) {
        // Decrement the depth however readBinaryParticle exits
        struct DepthGuard { int & depth; ~DepthGuard() { depth--; } } depthGuard{++readParticleDepth_};

        ByteBuffer * particleBuffer;
        std::unique_ptr<ByteBuffer> temporaryParticleBuffer;
        if (readParticleDepth_ == 1) {
            // avoid creating a new particle buffer for every record
            if (!particleBuffer_ || particleBuffer_->capacity() < record.size()) {
                particleBuffer_ = std::make_unique<ByteBuffer>(record.size(), buffer_.getByteOrder());
            }
            particleBuffer = particleBuffer_.get();
        } else {
            // a nested read (e.g. readBinaryParticle reading ahead) must not overwrite the outer record
            temporaryParticleBuffer = std::make_unique<ByteBuffer>(record.size(), buffer_.getByteOrder());
            particleBuffer = temporaryParticleBuffer.get();
        }

        particleBuffer->setData(record);
        return readBinaryParticle(*particleBuffer);
    }

    std::span<const byte> PhaseSpaceFileReader::nextBinaryRecord(std::size_t recordsAhead, bool consume) {
        // Make sure the buffer holds the requested record, reading the next block if necessary
        std::size_t bytesNeeded = (recordsAhead + 1) * particleRecordLength_;
        if (buffer_.remainingToRead() < bytesNeeded) {
            if (bytesInFile_ - bytesRead_ + buffer_.remainingToRead() < bytesNeeded) {
                throw std::runtime_error("No more particles to read.");
            }
            readNextBlock();
        }
        if (consume) return buffer_.readBytes(particleRecordLength_);
        return buffer_.peekBytes(bytesNeeded).subspan(recordsAhead * particleRecordLength_);
    }

    void PhaseSpaceFileReader::foldEmptyHistories(Particle & particle, bool consume) {
        // A pseudoparticle record only carries a count of empty histories (e.g. in TOPAS files),
        // so fold those histories into the next real particle
        std::size_t recordsAhead = 0;
        std::uint64_t emptyHistories = 0;
        while (particle.getType() == ParticleType::PseudoParticle) {
            emptyHistories += particle.getIncrementalHistories();
            if (consume) {
                particlesRead_++;
                metaparticlesRead_++;
            } else {
                recordsAhead++;
            }
            particle = readBinaryRecord(nextBinaryRecord(recordsAhead, consume));
        }

        if (emptyHistories > 0) {
            std::uint64_t ownHistories = std::max<std::uint32_t>(1, particle.getIncrementalHistories());
            particle.setIncrementalHistories(static_cast<std::uint32_t>(emptyHistories + ownHistories));
        }
    }

    Particle PhaseSpaceFileReader::getNextParticle(bool countParticleInStatistics) {
        if (!hasMoreParticles()) {
            throw std::runtime_error("No more particles to read.");
        }
        Particle particle = [&]() {
            switch (formatType_) {
                case (FormatType::BINARY): // Binary format
                    {                        
                        if (particleRecordLength_ == 0) particleRecordLength_ = getParticleRecordLength();
                        // Read the next block of data if necessary
                        if (buffer_.length() == 0 || buffer_.remainingToRead() < particleRecordLength_) {
                            readNextBlock();
                        }

                        // Read the next particle record in the buffer
                        Particle nextParticle = readBinaryRecord(buffer_.readBytes(particleRecordLength_));
                        if (nextParticle.getType() == ParticleType::PseudoParticle) [[unlikely]] {
                            foldEmptyHistories(nextParticle, true);
                        }
                        return nextParticle;
                    }
                    break;
                case (FormatType::ASCII): // ASCII format
                    {
                        try {
                            if (asciiLineBuffer_.empty()) bufferNextASCIILine();
                            std::string line = std::move(asciiLineBuffer_.front());
                            asciiLineBuffer_.pop_front();
                            return readASCIIParticle(line);
                        } catch (const std::runtime_error &e) {
                            throw std::runtime_error("Error reading line from file: " + std::string(e.what()));
                        }
                    }
                    break;
                default: // NONE format
                    {
                        // For NONE format, all I/O needs to be implemented manually by the subclass.
                        return readParticleManually();
                    }
                    break;
            }
        }();

        if (countParticleInStatistics) {
            if (particle.getType() == ParticleType::PseudoParticle) metaparticlesRead_++;
            else if (isFirstParticle_) {
                particle.setNewHistory(true); // First real particle read always starts a new history
                isFirstParticle_ = false;
            }

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
        } else {
            metaparticlesRead_++;
        }
        particlesRead_++;

        return particle;
    }

    Particle PhaseSpaceFileReader::peekNextParticle() {
        if (!hasMoreParticles()) {
            throw std::runtime_error("No more particles to read.");
        }
        Particle particle = [&]() {
            switch (formatType_) {
                case (FormatType::BINARY): // Binary format
                    {                        
                        if (particleRecordLength_ == 0) particleRecordLength_ = getParticleRecordLength();
                        // Read the next block of data if necessary
                        if (buffer_.length() == 0 || buffer_.remainingToRead() < particleRecordLength_) {
                            readNextBlock();
                        }

                        // Read the next particle record in the buffer without advancing the read position
                        Particle nextParticle = readBinaryRecord(buffer_.peekBytes(particleRecordLength_));
                        if (nextParticle.getType() == ParticleType::PseudoParticle) [[unlikely]] {
                            foldEmptyHistories(nextParticle, false);
                        }
                        return nextParticle;
                    }
                    break;
                case (FormatType::ASCII): // ASCII format
                    {
                        try {
                            if (asciiLineBuffer_.empty()) bufferNextASCIILine();
                            std::string line = asciiLineBuffer_.front(); // Peek without popping
                            return readASCIIParticle(line);
                        } catch (const std::runtime_error &e) {
                            throw std::runtime_error("Error reading line from file: " + std::string(e.what()));
                        }
                    }
                    break;
                default: // NONE format
                    {
                        // For NONE format, all I/O needs to be implemented manually by the subclass.
                        return peekParticleManually();
                    }
                    break;
            }
        }();
        return particle;
    }

}