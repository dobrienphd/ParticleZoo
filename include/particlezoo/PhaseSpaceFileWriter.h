#pragma once

#include <fstream>
#include <string>
#include <cstdint>
#include <memory>

#include "particlezoo/utilities/argParse.h"
#include "particlezoo/Particle.h"
#include "particlezoo/ByteBuffer.h"

namespace ParticleZoo
{
    extern CLICommand ConstantXCommand;
    extern CLICommand ConstantYCommand;
    extern CLICommand ConstantZCommand;
    extern CLICommand ConstantPxCommand;
    extern CLICommand ConstantPyCommand;
    extern CLICommand ConstantPzCommand;
    extern CLICommand ConstantWeightCommand;
    extern CLICommand FlipXDirectionCommand;
    extern CLICommand FlipYDirectionCommand;
    extern CLICommand FlipZDirectionCommand;

    /**
     * @brief Base class for writing phase space files
     * 
     * This abstract class provides a unified interface for writing particle phase space files
     * to different simulation formats (EGS, IAEA, TOPAS, etc.). It handles both binary and
     * ASCII file formats, provides buffering for efficient I/O, and supports statistics tracking
     * and format-specific optimizations. In cases where I/O must be handled by a third-party
     * library (e.g., ROOT), this class also provides a framework for manually writing particles.
     */
    class PhaseSpaceFileWriter
    {
        public:
            /**
             * @brief Construct a new Phase Space File Writer object.
             * 
             * @param phspFormat The format identifier of the phase space file (e.g., "IAEA", "EGS", "TOPAS")
             * @param fileName The path where the phase space file will be written
             * @param userOptions User-defined options for writing behavior
             * @param formatType The format type (BINARY or ASCII), defaults to BINARY
             * @param fixedValues Pre-defined constant values for certain particle properties
             * @param bufferSize Size of the internal buffer for writing, defaults to DEFAULT_BUFFER_SIZE
             */
            PhaseSpaceFileWriter(const std::string & phspFormat, const std::string & fileName, const UserOptions & userOptions, FormatType formatType = FormatType::BINARY, const FixedValues fixedValues = FixedValues(), unsigned int bufferSize = DEFAULT_BUFFER_SIZE);
            
            /**
             * @brief Destroy the Phase Space File Writer object.
             * 
             * Ensures proper cleanup by closing the file and flushing any remaining buffered data.
             */
            virtual ~PhaseSpaceFileWriter();

            /**
             * @brief Get the phase space file format identifier.
             * 
             * @return const std::string The format identifier (e.g., "IAEA", "EGS", "TOPAS")
             */
            const std::string           getPHSPFormat() const;

            /**
             * @brief Write a particle to the phase space file.
             * 
             * Writes the given particle to the file using the appropriate format.
             * For binary or ASCII files, the particle is automatically buffered
             * and written when the buffer is full. Applies any constant values
             * that have been set before writing.
             * 
             * @param particle The particle object to write to the file
             */
            virtual void                writeParticle(Particle particle);
            
            /**
             * @brief Get the maximum number of particles this writer can support.
             * 
             * This is a pure virtual method that must be implemented by derived classes
             * as the maximum can vary by format.
             * 
             * @return std::uint64_t The maximum number of particles supported
             */
            virtual std::uint64_t       getMaximumSupportedParticles() const = 0;
            
            /**
             * @brief Get the number of Monte Carlo histories that have been written.
             * 
             * @return std::uint64_t The number of histories written to the file
             */
            virtual std::uint64_t       getHistoriesWritten() const;
            
            /**
             * @brief Get the number of particles that have been written to the file.
             * 
             * @return std::uint64_t The number of particles written (excludes pseudo-particles)
             */
                    std::uint64_t       getParticlesWritten() const;

            /**
             * @brief Add additional Monte Carlo histories to the count.
             * 
             * Used to account for simulation histories that produced no particles to write.
             * Some formats may need special handling for empty histories.
             * 
             * @param additionalHistories The number of additional (empty) histories to account for
             */
            void                        addAdditionalHistories(std::uint64_t additionalHistories);

            /**
             * @brief Get the filename where the phase space file is being written.
             * 
             * @return const std::string The filename/path of the output file
             */
            const std::string           getFileName() const;
            
            /**
             * @brief Get the byte order used for binary data writing.
             * 
             * @return ByteOrder The current byte order (little-endian, big-endian, or PDP-endian)
             */
            ByteOrder                   getByteOrder() const;

            /**
             * @brief Check if the X coordinate is set to a constant value for all particles.
             * 
             * @return true if X coordinate is constant across all particles
             * @return false if X coordinate varies between particles
             */
            bool                        isXConstant() const;
            
            /**
             * @brief Check if the Y coordinate is set to a constant value for all particles.
             * 
             * @return true if Y coordinate is constant across all particles
             * @return false if Y coordinate varies between particles
             */
            bool                        isYConstant() const;
            
            /**
             * @brief Check if the Z coordinate is set to a constant value for all particles.
             * 
             * @return true if Z coordinate is constant across all particles
             * @return false if Z coordinate varies between particles
             */
            bool                        isZConstant() const;
            
            /**
             * @brief Check if the X-component of the direction unit vector is set to a constant value for all particles.
             * 
             * @return true if Px is constant across all particles
             * @return false if Px varies between particles
             */
            bool                        isPxConstant() const;
            
            /**
             * @brief Check if the Y-component of the direction unit vector is set to a constant value for all particles.
             * 
             * @return true if Py is constant across all particles
             * @return false if Py varies between particles
             */
            bool                        isPyConstant() const;
            
            /**
             * @brief Check if the Z-component of the direction unit vector is set to a constant value for all particles.
             * 
             * @return true if Pz is constant across all particles
             * @return false if Pz varies between particles
             */
            bool                        isPzConstant() const;
            
            /**
             * @brief Check if the statistical weight is set to a constant value for all particles.
             * 
             * @return true if weight is constant across all particles
             * @return false if weight varies between particles
             */
            bool                        isWeightConstant() const;

            /**
             * @brief Get the constant X coordinate value (if constant).
             * 
             * @return float The constant X coordinate value
             * @throws std::runtime_error if X is not set to constant
             */
            float                       getConstantX() const;
            
            /**
             * @brief Get the constant Y coordinate value (if constant).
             * 
             * @return float The constant Y coordinate value
             * @throws std::runtime_error if Y is not set to constant
             */
            float                       getConstantY() const;
            
            /**
             * @brief Get the constant Z coordinate value (if constant).
             * 
             * @return float The constant Z coordinate value
             * @throws std::runtime_error if Z is not set to constant
             */
            float                       getConstantZ() const;
            
            /**
             * @brief Get the constant X-component of the direction unit vector (if constant).
             * 
             * @return float The constant Px value
             * @throws std::runtime_error if Px is not set to constant
             */
            float                       getConstantPx() const;
            
            /**
             * @brief Get the constant Y-component of the direction unit vector (if constant).
             * 
             * @return float The constant Py value
             * @throws std::runtime_error if Py is not set to constant
             */
            float                       getConstantPy() const;
            
            /**
             * @brief Get the constant Z-component of the direction unit vector (if constant).
             * 
             * @return float The constant Pz value
             * @throws std::runtime_error if Pz is not set to constant
             */
            float                       getConstantPz() const;
            
            /**
             * @brief Get the constant statistical weight value (if constant).
             * 
             * @return float The constant weight value
             * @throws std::runtime_error if weight is not set to constant
             */
            float                       getConstantWeight() const;

            /**
             * @brief Get the fixed values configuration.
             * 
             * @return const FixedValues The complete fixed values structure
             */
            const FixedValues           getFixedValues() const;

            /**
             * @brief Get command line interface commands supported by this writer.
             * 
             * Returns a vector of CLI commands that can be used with this writer type.
             * 
             * @return std::vector<CLICommand> Vector of supported CLI commands
             */
            static std::vector<CLICommand> getCLICommands();

            /**
             * @brief Close the phase space file and finalize writing.
             * 
             * Flushes any remaining buffered data, writes the file header, and closes
             * the file handle. The writer cannot be used after calling this method.
             */
            void                        close();

        protected:

            /**
             * @brief Check if this format supports constant X coordinates.
             * 
             * Derived classes can override this to indicate format-specific capabilities.
             * Default implementation returns false.
             * 
             * @return true if constant X coordinates are supported by this format
             * @return false if constant X coordinates are not supported
             */
            virtual bool                canHaveConstantX() const;
            
            /**
             * @brief Check if this format supports constant Y coordinates.
             * 
             * Derived classes can override this to indicate format-specific capabilities.
             * Default implementation returns false.
             * 
             * @return true if constant Y coordinates are supported by this format
             * @return false if constant Y coordinates are not supported
             */
            virtual bool                canHaveConstantY() const;
            
            /**
             * @brief Check if this format supports constant Z coordinates.
             * 
             * Derived classes can override this to indicate format-specific capabilities.
             * Default implementation returns false.
             * 
             * @return true if constant Z coordinates are supported by this format
             * @return false if constant Z coordinates are not supported
             */
            virtual bool                canHaveConstantZ() const;
            
            /**
             * @brief Check if this format supports constant X-component of the direction unit vector.
             * 
             * Derived classes can override this to indicate format-specific capabilities.
             * Default implementation returns false.
             * 
             * @return true if constant Px is supported by this format
             * @return false if constant Px is not supported
             */
            virtual bool                canHaveConstantPx() const;
            
            /**
             * @brief Check if this format supports constant Y-component of the direction unit vector.
             * 
             * Derived classes can override this to indicate format-specific capabilities.
             * Default implementation returns false.
             * 
             * @return true if constant Py is supported by this format
             * @return false if constant Py is not supported
             */
            virtual bool                canHaveConstantPy() const;
            
            /**
             * @brief Check if this format supports constant Z-component of the direction unit vector.
             * 
             * Derived classes can override this to indicate format-specific capabilities.
             * Default implementation returns false.
             * 
             * @return true if constant Pz is supported by this format
             * @return false if constant Pz is not supported
             */
            virtual bool                canHaveConstantPz() const;
            
            /**
             * @brief Check if this format supports constant statistical weights.
             * 
             * Derived classes can override this to indicate format-specific capabilities.
             * Default implementation returns false.
             * 
             * @return true if constant weights are supported by this format
             * @return false if constant weights are not supported
             */
            virtual bool                canHaveConstantWeight() const;

            /**
             * @brief Set a constant X coordinate value for all particles.
             * 
             * @param X The constant X coordinate value to set
             */
            void                        setConstantX(float X);
            
            /**
             * @brief Set a constant Y coordinate value for all particles.
             * 
             * @param Y The constant Y coordinate value to set
             */
            void                        setConstantY(float Y);
            
            /**
             * @brief Set a constant Z coordinate value for all particles.
             * 
             * @param Z The constant Z coordinate value to set
             */
            void                        setConstantZ(float Z);
            
            /**
             * @brief Set a constant X-component of the direction unit vector for all particles.
             * 
             * @param Px The constant Px value to set
             */
            void                        setConstantPx(float Px);
            
            /**
             * @brief Set a constant Y-component of the direction unit vector for all particles.
             * 
             * @param Py The constant Py value to set
             */
            void                        setConstantPy(float Py);
            
            /**
             * @brief Set a constant Z-component of the direction unit vector for all particles.
             * 
             * @param Pz The constant Pz value to set
             */
            void                        setConstantPz(float Pz);
            
            /**
             * @brief Set a constant statistical weight for all particles.
             * 
             * @param weight The constant weight value to set
             */
            void                        setConstantWeight(float weight);

            /**
             * @brief Called when fixed values have been changed.
             * 
             * Derived classes can override this to perform any necessary updates
             * when constant values are modified. Default implementation does nothing.
             */
            virtual void                fixedValuesHaveChanged(){};

            /**
             * @brief Get the byte offset where particle records start in the file.
             * 
             * This is typically after any file header. Default implementation returns 0.
             * 
             * @return std::size_t The byte offset of the first particle record
             */
            virtual std::size_t         getParticleRecordStartOffset() const;
            
            /**
             * @brief Get the length in bytes of each particle record.
             * 
             * Must be implemented by derived classes for binary formatted files.
             * 
             * @return std::size_t The length of each particle record in bytes
             * @throws std::runtime_error if not implemented for binary format
             */
            virtual std::size_t         getParticleRecordLength() const;
            
            /**
             * @brief Get the maximum line length for ASCII format files.
             * 
             * Must be implemented by derived classes that support ASCII format.
             * Used for buffer allocation and writing optimization.
             * 
             * @return std::size_t The maximum length of ASCII lines in characters
             * @throws std::runtime_error if not implemented for ASCII format
             */
            virtual size_t              getMaximumASCIILineLength() const;

            /**
             * @brief Write header data to a byte buffer.
             * 
             * This is a pure virtual method that must be implemented by derived classes
             * to write format-specific header information.
             * 
             * @param buffer The byte buffer to write header data into
             */
            virtual void                writeHeaderData(ByteBuffer & buffer) = 0;
            
            /**
             * @brief Write a particle in binary format to a byte buffer.
             * 
             * Must be implemented by derived classes that support binary format.
             * The default implementation throws an exception.
             * 
             * @param buffer The byte buffer to write particle data into
             * @param particle The particle object to write
             * @throws std::runtime_error if not implemented for binary format
             */
            virtual void                writeBinaryParticle(ByteBuffer & buffer, Particle & particle);
            
            /**
             * @brief Write a particle in ASCII format as a string.
             * 
             * Must be implemented by derived classes that support ASCII format.
             * The default implementation throws an exception.
             * 
             * @param particle The particle object to write
             * @return const std::string The ASCII representation of the particle
             * @throws std::runtime_error if not implemented for ASCII format
             */
            virtual const std::string   writeASCIIParticle(Particle & particle);
            
            /**
             * @brief Write a particle manually (for formats requiring third-party I/O).
             * 
             * Can be implemented by derived classes to support manual file I/O,
             * circumventing the internal file stream and buffer.
             * 
             * Must be implemented by derived classes that specify FormatType::NONE.
             * The default implementation throws an exception.
             * 
             * @param particle The particle object to write manually
             * @throws std::runtime_error if not implemented
             */
            virtual void                writeParticleManually(Particle & particle);

            /**
             * @brief Handle accounting for simulation histories that produced no particles.
             * 
             * Called by addAdditionalHistories() to handle format-specific requirements
             * for empty histories. Some formats need special handling such as writing
             * pseudo-particles or updating header counters.
             * 
             * The default implementation returns true, indicating that the base class
             * should automatically increment the history counter. Derived classes can override
             * this to handle it manually (e.g., by writing additional pseudo-particles)
             * 
             * @param additionalHistories The number of additional (empty) histories
             * @return true if the base class should automatically increment the history counter
             * @return false if the derived class handles it manually (e.g., by writing additional pseudo-particles)
             */
            virtual bool                accountForAdditionalHistories(std::uint64_t additionalHistories);

            /**
             * @brief Get the number of pending histories to account for.
             * 
             * Used internally to track histories that have not yet been written
             * to the file (e.g., empty histories).
             * 
             * Override in derived classes if special handling is needed.
             * 
             * @return std::uint64_t The number of pending histories
             */
            virtual std::uint64_t       getPendingHistories() const;

            /**
             * @brief Check if this format can write pseudo-particles explicitly.
             * 
             * Derived classes can override this to indicate if they support writing
             * pseudo-particles (metadata particles) explicitly to the file.
             * Default implementation returns false.
             * 
             * @return true if pseudo-particles can be written explicitly
             * @return false if explicit pseudo-particle writing is not supported
             */
            virtual bool                canWritePseudoParticlesExplicitly() const;

            /**
             * @brief Set the byte order for binary data writing.
             * 
             * @param byteOrder The byte order to use (little-endian, big-endian, or PDP-endian)
             */
            void                        setByteOrder(ByteOrder byteOrder);

            /**
             * @brief Get the user options that were passed to the constructor.
             * 
             * @return const UserOptions& Reference to the user options
             */
            const UserOptions&          getUserOptions() const;

        private:
            void                        writeNextBlock();
            void                        writeHeaderToFile();
            ByteBuffer *                getParticleBuffer();

            const std::string phspFormat_;
            const std::string fileName_;
            const UserOptions userOptions_;
            const unsigned int BUFFER_SIZE;
            FormatType formatType_;
            std::ofstream file_;
            std::uint64_t historiesWritten_;
            std::uint64_t particlesWritten_;
            std::size_t particleRecordLength_;

            std::uint64_t historiesToAccountFor_;

            ByteBuffer buffer_;
            std::unique_ptr<ByteBuffer> particleBuffer_;
            int writeParticleDepth_; // to track nested calls to writeParticle

            FixedValues fixedValues_;
            bool flipXDirection_;
            bool flipYDirection_;
            bool flipZDirection_;
    };


    // Inline implementations for the PhaseSpaceFileWriter class


    inline const std::string PhaseSpaceFileWriter::getPHSPFormat() const { return phspFormat_; }
    inline std::uint64_t PhaseSpaceFileWriter::getHistoriesWritten() const { return historiesWritten_ + getPendingHistories(); }
    inline std::uint64_t PhaseSpaceFileWriter::getParticlesWritten() const { return particlesWritten_; }
    inline const std::string PhaseSpaceFileWriter::getFileName() const { return fileName_; }
    inline ByteOrder PhaseSpaceFileWriter::getByteOrder() const { return buffer_.getByteOrder(); }
    inline std::uint64_t PhaseSpaceFileWriter::getPendingHistories() const { return historiesToAccountFor_; }

    inline bool PhaseSpaceFileWriter::isXConstant() const { return fixedValues_.xIsConstant; }
    inline bool PhaseSpaceFileWriter::isYConstant() const { return fixedValues_.yIsConstant; }
    inline bool PhaseSpaceFileWriter::isZConstant() const { return fixedValues_.zIsConstant; }
    inline bool PhaseSpaceFileWriter::isPxConstant() const { return fixedValues_.pxIsConstant; }
    inline bool PhaseSpaceFileWriter::isPyConstant() const { return fixedValues_.pyIsConstant; }
    inline bool PhaseSpaceFileWriter::isPzConstant() const { return fixedValues_.pzIsConstant; }
    inline bool PhaseSpaceFileWriter::isWeightConstant() const { return fixedValues_.weightIsConstant; }

    inline float PhaseSpaceFileWriter::getConstantX() const { if (!fixedValues_.xIsConstant) throw std::runtime_error("X is not a constant"); return fixedValues_.constantX; }
    inline float PhaseSpaceFileWriter::getConstantY() const { if (!fixedValues_.yIsConstant) throw std::runtime_error("Y is not a constant"); return fixedValues_.constantY; }
    inline float PhaseSpaceFileWriter::getConstantZ() const { if (!fixedValues_.zIsConstant) throw std::runtime_error("Z is not a constant"); return fixedValues_.constantZ; }
    inline float PhaseSpaceFileWriter::getConstantPx() const { if (!fixedValues_.pxIsConstant) throw std::runtime_error("Px is not a constant"); return fixedValues_.constantPx; }
    inline float PhaseSpaceFileWriter::getConstantPy() const { if (!fixedValues_.pyIsConstant) throw std::runtime_error("Py is not a constant"); return fixedValues_.constantPy; }
    inline float PhaseSpaceFileWriter::getConstantPz() const { if (!fixedValues_.pzIsConstant) throw std::runtime_error("Pz is not a constant"); return fixedValues_.constantPz; }
    inline float PhaseSpaceFileWriter::getConstantWeight() const { if (!fixedValues_.weightIsConstant) throw std::runtime_error("Weight is not a constant"); return fixedValues_.constantWeight; }

    inline bool PhaseSpaceFileWriter::canHaveConstantX() const { return false; }
    inline bool PhaseSpaceFileWriter::canHaveConstantY() const { return false; }
    inline bool PhaseSpaceFileWriter::canHaveConstantZ() const { return false; }
    inline bool PhaseSpaceFileWriter::canHaveConstantPx() const { return false; }
    inline bool PhaseSpaceFileWriter::canHaveConstantPy() const { return false; }
    inline bool PhaseSpaceFileWriter::canHaveConstantPz() const { return false; }
    inline bool PhaseSpaceFileWriter::canHaveConstantWeight() const { return false; }

    inline void PhaseSpaceFileWriter::setConstantX(float X) { fixedValues_.xIsConstant = true; fixedValues_.constantX = X; fixedValuesHaveChanged(); }
    inline void PhaseSpaceFileWriter::setConstantY(float Y) { fixedValues_.yIsConstant = true; fixedValues_.constantY = Y; fixedValuesHaveChanged(); }
    inline void PhaseSpaceFileWriter::setConstantZ(float Z) { fixedValues_.zIsConstant = true; fixedValues_.constantZ = Z; fixedValuesHaveChanged(); }
    inline void PhaseSpaceFileWriter::setConstantPx(float Px) { fixedValues_.pxIsConstant = true; fixedValues_.constantPx = Px; fixedValuesHaveChanged(); }
    inline void PhaseSpaceFileWriter::setConstantPy(float Py) { fixedValues_.pyIsConstant = true; fixedValues_.constantPy = Py; fixedValuesHaveChanged(); }
    inline void PhaseSpaceFileWriter::setConstantPz(float Pz) { fixedValues_.pzIsConstant = true; fixedValues_.constantPz = Pz; fixedValuesHaveChanged(); }
    inline void PhaseSpaceFileWriter::setConstantWeight(float weight) { fixedValues_.weightIsConstant = true; fixedValues_.constantWeight = weight; fixedValuesHaveChanged(); }

    inline const FixedValues PhaseSpaceFileWriter::getFixedValues() const { return fixedValues_; }

    inline std::size_t PhaseSpaceFileWriter::getParticleRecordStartOffset() const { return 0; }

    inline void PhaseSpaceFileWriter::setByteOrder(ByteOrder byteOrder) {
        buffer_.setByteOrder(byteOrder);
        getParticleBuffer()->setByteOrder(byteOrder);
    }

    inline const UserOptions& PhaseSpaceFileWriter::getUserOptions() const { return userOptions_; }

    inline ByteBuffer * PhaseSpaceFileWriter::getParticleBuffer() {
        if (!particleBuffer_) {
            std::size_t particleRecordLength = getParticleRecordLength();
            particleBuffer_ = std::make_unique<ByteBuffer>(particleRecordLength, buffer_.getByteOrder());
        }
        return particleBuffer_.get();
    }

    inline std::size_t PhaseSpaceFileWriter::getParticleRecordLength() const {
        throw std::runtime_error("getParticleRecordLength() must be implemented for binary formatted file writers.");
    }

    inline size_t PhaseSpaceFileWriter::getMaximumASCIILineLength() const {
        throw std::runtime_error("getMaximumASCILineLength() must be implemented for ASCII formatted file writers.");
    }

    inline void PhaseSpaceFileWriter::writeBinaryParticle(ByteBuffer & buffer, Particle & particle) {
        (void)buffer;
        (void)particle;
        throw std::runtime_error("writeBinaryParticle() must be implemented for binary formatted file writers.");
    }

    inline const std::string PhaseSpaceFileWriter::writeASCIIParticle(Particle & particle) {
        (void)particle;
        throw std::runtime_error("writeASCIIParticle() must be implemented for ASCII formatted file writers.");
    }

    inline void PhaseSpaceFileWriter::writeParticleManually(Particle & particle) {
        (void)particle;
        throw std::runtime_error("writeParticleManually() must be implemented for manual particle writing.");
    }

    inline bool PhaseSpaceFileWriter::accountForAdditionalHistories(std::uint64_t additionalHistories) {
        (void)additionalHistories; // unused in this implementation
        return true;
    }

    inline void PhaseSpaceFileWriter::addAdditionalHistories(std::uint64_t additionalHistories) {
        if (accountForAdditionalHistories(additionalHistories)) {
            historiesToAccountFor_ = additionalHistories;
        }
    }

    inline bool PhaseSpaceFileWriter::canWritePseudoParticlesExplicitly() const {
        return false;
    }

} // namespace ParticleZoo