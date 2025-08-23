#include "particlezoo/PhaseSpaceFileWriter.h"

namespace ParticleZoo
{
    PhaseSpaceFileWriter::PhaseSpaceFileWriter(const std::string & phspFormat, const std::string & fileName, const UserOptions & userOptions, FormatType formatType, unsigned int bufferSize)
    : phspFormat_(phspFormat),
      fileName_(fileName),
      userOptions_(userOptions),
      BUFFER_SIZE(bufferSize),
      formatType_(formatType),
//      file_(fileName, std::ios::binary),
      file_([&]() {
            if (formatType_ == FormatType::NONE) {
                return std::ofstream{};
            } else {
                return std::ofstream(fileName, std::ios::binary);
            }
        }()),
      historiesWritten_(0),
      particleRecordLength_(0),
      buffer_(BUFFER_SIZE),
      writeParticleDepth_(0)
    {
        if (formatType != FormatType::NONE && !file_.is_open())
        {
            throw std::runtime_error("Failed to open file: " + fileName_);
        }
    }

    PhaseSpaceFileWriter::~PhaseSpaceFileWriter() {
        close();
    }

    void PhaseSpaceFileWriter::close() {
        if (file_.is_open()) {
            writeNextBlock();
            writeHeaderToFile();
            file_.flush();
            file_.close();
        }
    }

    void PhaseSpaceFileWriter::writeHeaderToFile() {
        if (formatType_ == FormatType::NONE) {
            return; // No header to write for NONE format
        }
        std::size_t headerSize = getParticleRecordStartOffset();

        std::size_t bufferSize = (std::size_t) std::fmax(1, headerSize);
        ByteBuffer headerBuffer(bufferSize, buffer_.getByteOrder());
        writeHeaderData(headerBuffer);

        if (headerSize == 0) {
            file_.flush();
            return;
        }

        if (headerBuffer.length() == 0) return;

        if (!file_.is_open()) {
            throw std::runtime_error("File is not open when attempting to write header data.");
        }

        if (headerBuffer.length() > headerSize) {
            throw std::runtime_error("Header data exceeds particle record start offset.");
        }

        if (headerBuffer.getByteOrder() != buffer_.getByteOrder()) {
            throw std::runtime_error("Header data byte order does not match particle record byte order.");
        }

        std::streampos currentPos = file_.tellp();

        file_.seekp(0);
        file_.write(reinterpret_cast<const char*>(headerBuffer.data()), headerBuffer.length());

        // Pad the header data with zeros to the start of the particle record
        std::size_t padding = headerSize - headerBuffer.length();
        if (padding > 0) {
            std::vector<byte> zeros(padding, 0);
            file_.write(reinterpret_cast<const char*>(zeros.data()), padding);
        }

        
        file_.flush();

        // Reset file position to where it was before writing header data.
        file_.seekp(currentPos);
    }

    void PhaseSpaceFileWriter::writeParticle(Particle particle) {        
        ParticleType type = particle.getType();

        if (type == ParticleType::Unsupported) {
            throw std::runtime_error("Attempting to write particle with unsupported type to phase space file.");
        }

        // do not attempt to write pseudoparticles to the file unless the writer explicitly supports that
        if (type != ParticleType::PseudoParticle || canWritePseudoParticlesExplicitly()) {

            switch (formatType_) {

            case (FormatType::BINARY): // Binary format
                {
                    if (particleRecordLength_ == 0) particleRecordLength_ = getParticleRecordLength();
                    writeParticleDepth_++;
                    
                    ByteBuffer * particleBuffer;
                    std::unique_ptr<ByteBuffer> temporaryParticleBuffer;
                    if (writeParticleDepth_ == 1) {
                        // avoid creating a new particle buffer if one is available
                        particleBuffer = getParticleBuffer();
                        particleBuffer->clear();
                    } else {
                        // If the particle buffer is not available, create a new one
                        temporaryParticleBuffer = std::make_unique<ByteBuffer>(particleRecordLength_, buffer_.getByteOrder());
                        particleBuffer = temporaryParticleBuffer.get();
                    }

                    writeBinaryParticle(*particleBuffer, particle);

                    if (particleBuffer->length() < particleRecordLength_) {
                        particleBuffer->expand();
                    }

                    if (buffer_.length() + getParticleRecordLength() > buffer_.capacity()) {
                        writeNextBlock();
                    }

                    buffer_.appendData(*particleBuffer, true);

                    writeParticleDepth_--;
                }
                break;
            case (FormatType::ASCII): // ASCII format
                {
                    if (buffer_.length() + getMaximumASCIILineLength() > buffer_.capacity()) {
                        writeNextBlock();
                    }

                    std::string asciiLine = writeASCIIParticle(particle);

                    if (asciiLine.length() > getMaximumASCIILineLength()) {
                        throw std::runtime_error("ASCII line length exceeds maximum allowed length.");
                    }

                    buffer_.writeString(asciiLine);
                }
                break;
            default: // NONE format
                {
                    writeParticleManually(particle);
                }
                break;
            }

        }

        // Update the number of histories written based on the particle's history status (even for pseudoparticles)
        if (particle.isNewHistory()) {
            std::uint64_t historiesWritten = 1; // Default to 1 for a new history
            if (particle.hasIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER)) {
                std::uint64_t incrementalHistoryNumber = particle.getIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER);
                if (incrementalHistoryNumber > 0) historiesWritten = incrementalHistoryNumber;
            }
            historiesWritten_ += historiesWritten;
        }
    }

    void PhaseSpaceFileWriter::writeNextBlock() {
        if (formatType_ == FormatType::NONE) return;
        if (!file_.is_open()) {
            throw std::runtime_error("File is not open when attempting to write data.");
        }
        if (buffer_.length() == 0) {
            return;
        }
        std::size_t particleRecordStartOffset = getParticleRecordStartOffset();
        std::size_t currentPos = (std::size_t) file_.tellp();
        if (currentPos < particleRecordStartOffset) {
            file_.seekp(particleRecordStartOffset);
        }

        file_.write(reinterpret_cast<const char*>(buffer_.data()), buffer_.length());

        buffer_.clear();
    }
}