#include "particlezoo/PhaseSpaceFileWriter.h"

namespace ParticleZoo
{

    CLICommand ConstantXCommand{ WRITER, "X", "constantX", "Set all particles to be written with this constant value for the X position", { CLI_FLOAT } };
    CLICommand ConstantYCommand{ WRITER, "Y", "constantY", "Set all particles to be written with this constant value for the Y position", { CLI_FLOAT } };
    CLICommand ConstantZCommand{ WRITER, "Z", "constantZ", "Set all particles to be written with this constant value for the Z position", { CLI_FLOAT } };
    CLICommand ConstantPxCommand{ WRITER, "Pz", "constantPx", "Set all particles to be written with this constant value for the X directional cosine", { CLI_FLOAT } };
    CLICommand ConstantPyCommand{ WRITER, "Py", "constantPy", "Set all particles to be written with this constant value for the Y directional cosine", { CLI_FLOAT } };
    CLICommand ConstantPzCommand{ WRITER, "Pz", "constantPz", "Set all particles to be written with this constant value for the Z directional cosine", { CLI_FLOAT } };
    CLICommand ConstantWeightCommand{ WRITER, "W", "constantWeight", "Set all particles to be written with this constant value for the weight", { CLI_FLOAT } };

    std::vector<CLICommand> PhaseSpaceFileWriter::getCLICommands() {
        return { ConstantXCommand, ConstantYCommand, ConstantZCommand,
                 ConstantPxCommand, ConstantPyCommand, ConstantPzCommand,
                 ConstantWeightCommand };
    }

    PhaseSpaceFileWriter::PhaseSpaceFileWriter(const std::string & phspFormat, const std::string & fileName, const UserOptions & userOptions, FormatType formatType, const FixedValues fixedValues, unsigned int bufferSize)
    : phspFormat_(phspFormat),
      fileName_(fileName),
      userOptions_(userOptions),
      BUFFER_SIZE(bufferSize),
      formatType_(formatType),
      file_([&]() {
            if (formatType_ == FormatType::NONE) {
                return std::ofstream{};
            } else {
                return std::ofstream(fileName, std::ios::binary);
            }
        }()),
      historiesWritten_(0),
      particlesWritten_(0),
      particleRecordLength_(0),
      historiesToAccountFor_(0),
      buffer_(BUFFER_SIZE),
      writeParticleDepth_(0),
      fixedValues_(fixedValues)
    {
        if (formatType != FormatType::NONE && !file_.is_open())
        {
            throw std::runtime_error("Failed to open file: " + fileName_);
        }
        if (userOptions_.contains(ConstantXCommand)) {
            CLIValue constantXValue = userOptions.at(ConstantXCommand).front();
            setConstantX(std::get<float>(constantXValue));
        }
        if (userOptions_.contains(ConstantYCommand)) {
            CLIValue constantYValue = userOptions.at(ConstantYCommand).front();
            setConstantY(std::get<float>(constantYValue));
        }
        if (userOptions_.contains(ConstantZCommand)) {
            CLIValue constantZValue = userOptions.at(ConstantZCommand).front();
            setConstantZ(std::get<float>(constantZValue));
        }
        if (userOptions_.contains(ConstantPxCommand)) {
            CLIValue constantPxValue = userOptions.at(ConstantPxCommand).front();
            setConstantPx(std::get<float>(constantPxValue));
        }
        if (userOptions_.contains(ConstantPyCommand)) {
            CLIValue constantPyValue = userOptions.at(ConstantPyCommand).front();
            setConstantPy(std::get<float>(constantPyValue));
        }
        if (userOptions_.contains(ConstantPzCommand)) {
            CLIValue constantPzValue = userOptions.at(ConstantPzCommand).front();
            setConstantPz(std::get<float>(constantPzValue));
        }
        if (userOptions_.contains(ConstantWeightCommand)) {
            CLIValue constantWeightValue = userOptions.at(ConstantWeightCommand).front();
            setConstantWeight(std::get<float>(constantWeightValue));
        }
    }

    PhaseSpaceFileWriter::~PhaseSpaceFileWriter() {
        close();
    }

    void PhaseSpaceFileWriter::close() {
        historiesWritten_ += historiesToAccountFor_;
        historiesToAccountFor_ = 0;
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
        if (getParticlesWritten() >= getMaximumSupportedParticles()) {
            throw std::runtime_error("Maximum number of particles reached for this writer (" + std::to_string(getMaximumSupportedParticles()) + ").");
        }

        ParticleType type = particle.getType();

        if (type == ParticleType::Unsupported) {
            throw std::runtime_error("Attempting to write particle with unsupported type to phase space file.");
        }

        if (historiesToAccountFor_ > 0) {
            if (particle.isNewHistory()) {
                std::uint64_t incrementalHistories = particle.getIncrementalHistories();
                incrementalHistories += historiesToAccountFor_;
                particle.setIncrementalHistories(static_cast<std::uint32_t>(incrementalHistories));
            } else {
                particle.setIncrementalHistories(static_cast<std::uint32_t>(historiesToAccountFor_));
            }
            historiesToAccountFor_ = 0;
        }

        // do not attempt to write pseudoparticles to the file unless the writer explicitly supports that
        if (type != ParticleType::PseudoParticle || canWritePseudoParticlesExplicitly()) {
            bool recheckDirectionNormalization = false;
            if (fixedValues_.xIsConstant) particle.setX(fixedValues_.constantX);
            if (fixedValues_.yIsConstant) particle.setY(fixedValues_.constantY);
            if (fixedValues_.zIsConstant) particle.setZ(fixedValues_.constantZ);
            if (fixedValues_.pxIsConstant) { particle.setDirectionalCosineX(fixedValues_.constantPx); recheckDirectionNormalization = true; }
            if (fixedValues_.pyIsConstant) { particle.setDirectionalCosineY(fixedValues_.constantPy); recheckDirectionNormalization = true; }
            if (fixedValues_.pzIsConstant) { particle.setDirectionalCosineZ(fixedValues_.constantPz); recheckDirectionNormalization = true; }
            if (fixedValues_.weightIsConstant) particle.setWeight(fixedValues_.constantWeight);

            if (recheckDirectionNormalization) {
                float directionMagnitude = particle.getDirectionalCosineX()*particle.getDirectionalCosineX() + particle.getDirectionalCosineY()*particle.getDirectionalCosineY() + particle.getDirectionalCosineZ()*particle.getDirectionalCosineZ();
                constexpr float EPSILON = 1e-6f;
                if (directionMagnitude < 1.0f - EPSILON || directionMagnitude > 1.0f + EPSILON) {
                    throw std::runtime_error("Particle direction is not normalized.");
                }
            }

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

        if (type != ParticleType::PseudoParticle) {
            particlesWritten_++;
        }

        // Update the number of histories written based on the particle's history status (even for pseudoparticles)
        if (particle.isNewHistory()) {
            historiesWritten_ += particle.getIncrementalHistories();
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