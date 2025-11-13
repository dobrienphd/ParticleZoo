#pragma once

#include <fstream>
#include <string>
#include <cstdint>
#include <list>
#include <vector>

#include "particlezoo/utilities/argParse.h"
#include "particlezoo/Particle.h"
#include "particlezoo/ByteBuffer.h"

namespace ParticleZoo
{

    /**
     * @brief Base class for reading phase space files
     * 
     * This abstract class provides a unified interface for reading particle phase space files
     * from different simulation formats (EGS, IAEA, TOPAS, etc.). It handles both binary and
     * ASCII file formats and provides functionality for particle iteration, statistics tracking,
     * and format-specific optimizations. In cases where I/O must be handled by a third-party
     * library (e.g., ROOT), this class also provides a framework for manually reading particles.
     */
    class PhaseSpaceFileReader
    {
        public:
            /**
             * @brief Construct a new Phase Space File Reader object.
             * 
             * @param phspFormat The format identifier of the phase space file (e.g., "IAEA", "EGS", "TOPAS")
             * @param fileName The path to the phase space file to read
             * @param userOptions User-defined options for reading behavior
             * @param formatType The format type (BINARY, ASCII, or NONE), defaults to BINARY
             * @param fixedValues Pre-defined constant values for certain particle properties
             * @param bufferSize Size of the internal buffer for reading, defaults to DEFAULT_BUFFER_SIZE
             */
            PhaseSpaceFileReader(const std::string & phspFormat, const std::string & fileName, const UserOptions & userOptions, FormatType formatType = FormatType::BINARY, const FixedValues fixedValues = FixedValues(), unsigned int bufferSize = DEFAULT_BUFFER_SIZE);
            
            /**
             * @brief Destroy the Phase Space File Reader object.
             * 
             * Ensures proper cleanup of file handles and allocated resources.
             */
            virtual ~PhaseSpaceFileReader();

            /**
             * @brief Get the next particle from the phase space file.
             * 
             * Reads and returns the next particle in the file. This method automatically
             * handles buffering and format-specific parsing. The particle is counted in
             * the read statistics.
             * 
             * @return Particle The next particle object containing position, momentum, energy, etc.
             */
            Particle              getNextParticle();
            
            /**
             * @brief Peek at the next particle without advancing the file position.
             * 
             * Reads the next particle but does not move the internal file pointer forward.
             * This allows for inspecting the upcoming particle without consuming it.
             * 
             * @return Particle The next particle object containing position, momentum, energy, etc.
             */
            Particle              peekNextParticle();

            /**
             * @brief Check if there are more particles to read in the file.
             * 
             * @return true if there are more particles available to read
             * @return false if the end of file has been reached
             */
            virtual bool          hasMoreParticles();

            /**
             * @brief Get the phase space file format identifier.
             * 
             * @return const std::string The format identifier (e.g., "IAEA", "EGS", "TOPAS")
             */
            const std::string     getPHSPFormat() const;

            /**
             * @brief Get the total number of particles in the phase space file.
             * 
             * This is a pure virtual method that must be implemented by derived classes
             * as the method for determining particle count varies by format.
             * 
             * @return std::uint64_t The total number of particles in the file
             */
            virtual std::uint64_t getNumberOfParticles() const = 0;
            
            /**
             * @brief Get the number of original Monte Carlo histories that generated this phase space.
             * 
             * This is a pure virtual method that must be implemented by derived classes
             * as the method for determining history count varies by format.
             * 
             * @return std::uint64_t The number of original histories
             */
            virtual std::uint64_t getNumberOfOriginalHistories() const = 0;

            /**
             * @brief Get the number of Monte Carlo histories that have been read so far.
             * If the end of the file has been reached, this will return the total number of original histories
             * unless more histories than expected have already been read - in which case it returns the actual count.
             * 
             * @return std::uint64_t The number of histories read
             */
            virtual std::uint64_t getHistoriesRead();
            
            /**
             * @brief Get the number of particles that have been read so far.
             * 
             * This excludes metadata particles and skipped particles.
             * 
             * @return std::uint64_t The number of particles read
             */
            virtual std::uint64_t getParticlesRead();

            /**
             * @brief Get the size of the phase space file in bytes.
             * 
             * @return std::uint64_t The file size in bytes
             */
            std::uint64_t         getFileSize() const;
            
            /**
             * @brief Get the filename of the phase space file being read.
             * 
             * @return const std::string The filename/path of the file
             */
            const std::string     getFileName() const;

            /**
             * @brief Set comment markers for ASCII format files.
             * 
             * Defines the strings that mark comment lines in ASCII format files.
             * Lines beginning with these markers will be ignored during parsing.
             * 
             * @param commentMarkers Vector of strings that indicate comment lines
             */
            void                  setCommentMarkers(const std::vector<std::string> & commentMarkers);

            /**
             * @brief Check if the X coordinate is constant for all particles.
             * 
             * @return true if X coordinate is constant across all particles
             * @return false if X coordinate varies between particles
             */
            bool                  isXConstant() const;
            
            /**
             * @brief Check if the Y coordinate is constant for all particles.
             * 
             * @return true if Y coordinate is constant across all particles
             * @return false if Y coordinate varies between particles
             */
            bool                  isYConstant() const;
            
            /**
             * @brief Check if the Z coordinate is constant for all particles.
             * 
             * @return true if Z coordinate is constant across all particles
             * @return false if Z coordinate varies between particles
             */
            bool                  isZConstant() const;
            
            /**
             * @brief Check if the X-component of momentum is constant for all particles.
             * 
             * @return true if Px is constant across all particles
             * @return false if Px varies between particles
             */
            bool                  isPxConstant() const;
            
            /**
             * @brief Check if the Y-component of momentum is constant for all particles.
             * 
             * @return true if Py is constant across all particles
             * @return false if Py varies between particles
             */
            bool                  isPyConstant() const;
            
            /**
             * @brief Check if the Z-component of momentum is constant for all particles.
             * 
             * @return true if Pz is constant across all particles
             * @return false if Pz varies between particles
             */
            bool                  isPzConstant() const;
            
            /**
             * @brief Check if the statistical weight is constant for all particles.
             * 
             * @return true if weight is constant across all particles
             * @return false if weight varies between particles
             */
            bool                  isWeightConstant() const;

            /**
             * @brief Get the constant X coordinate value (if constant).
             * 
             * @return float The constant X coordinate value
             * @throws std::runtime_error if X is not constant
             */
            float                 getConstantX() const;
            
            /**
             * @brief Get the constant Y coordinate value (if constant).
             * 
             * @return float The constant Y coordinate value
             * @throws std::runtime_error if Y is not constant
             */
            float                 getConstantY() const;
            
            /**
             * @brief Get the constant Z coordinate value (if constant).
             * 
             * @return float The constant Z coordinate value
             * @throws std::runtime_error if Z is not constant
             */
            float                 getConstantZ() const;
            
            /**
             * @brief Get the constant X-component of the direction unit vector (if constant).
             * 
             * @return float The constant Px value
             * @throws std::runtime_error if Px is not constant
             */
            float                 getConstantPx() const;
            
            /**
             * @brief Get the constant Y-component of the direction unit vector (if constant).
             * 
             * @return float The constant Py value
             * @throws std::runtime_error if Py is not constant
             */
            float                 getConstantPy() const;
            
            /**
             * @brief Get the constant Z-component of the direction unit vector (if constant).
             * 
             * @return float The constant Pz value
             * @throws std::runtime_error if Pz is not constant
             */
            float                 getConstantPz() const;
            
            /**
             * @brief Get the constant statistical weight value (if constant).
             * 
             * @return float The constant weight value
             * @throws std::runtime_error if weight is not constant
             */
            float                 getConstantWeight() const;

            /**
             * @brief Get the fixed values configuration.
             * 
             * @return const FixedValues The complete fixed values structure
             */
            const FixedValues     getFixedValues() const;

            /**
             * @brief Get command line interface commands supported by this reader.
             * 
             * Returns a vector of CLI commands that can be used with this reader type.
             * 
             * @return std::vector<CLICommand> Vector of supported CLI commands
             */
            static std::vector<CLICommand> getCLICommands();

            /**
             * @brief Move the file position to a specific particle index.
             * 
             * Allows random access to particles within the file. The next call to
             * getNextParticle() will return the particle at the specified index.
             * 
             * @param particleIndex Zero-based index of the particle to move to
             */
            void                  moveToParticle(std::uint64_t particleIndex);

            /**
             * @brief Close the phase space file and clean up resources.
             * 
             * Explicitly closes the file handle and frees associated resources.
             * The reader cannot be used after calling this method.
             */
            void                  close();

        protected:

            /**
             * @brief Set a constant X coordinate value for all particles.
             * 
             * @param X The constant X coordinate value to set
             */
            void                  setConstantX(float X);
            
            /**
             * @brief Set a constant Y coordinate value for all particles.
             * 
             * @param Y The constant Y coordinate value to set
             */
            void                  setConstantY(float Y);
            
            /**
             * @brief Set a constant Z coordinate value for all particles.
             * 
             * @param Z The constant Z coordinate value to set
             */
            void                  setConstantZ(float Z);
            
            /**
             * @brief Set a constant X-component of the direction unit vector for all particles.
             * 
             * @param Px The constant Px value to set
             */
            void                  setConstantPx(float Px);
            
            /**
             * @brief Set a constant Y-component of the direction unit vector for all particles.
             * 
             * @param Py The constant Py value to set
             */
            void                  setConstantPy(float Py);
            
            /**
             * @brief Set a constant Z-component of the direction unit vector for all particles.
             * 
             * @param Pz The constant Pz value to set
             */
            void                  setConstantPz(float Pz);
            
            /**
             * @brief Set a constant statistical weight for all particles.
             * 
             * @param weight The constant weight value to set
             */
            void                  setConstantWeight(float weight);

            /**
             * @brief Get the next particle with optional statistics counting control.
             * 
             * This protected version allows derived classes to control whether the
             * particle should be counted in the read statistics.
             * 
             * @param countParticleInStatistics Whether to count this particle in statistics
             * @return Particle The next particle object
             */
            Particle              getNextParticle(bool countParticleInStatistics);

            /**
             * @brief Get the number of particles read with optional inclusion of skipped particles (including pseudo-particles).
             * 
             * @param includeSkippedParticles Whether to include pseudo-particles and particles skipped by moveToParticle()
             * @return std::uint64_t The number of particles read
             */
            virtual std::uint64_t getParticlesRead(bool includeSkippedParticles);
            
            /**
             * @brief Get the byte offset where particle records start in the file.
             * 
             * This is typically after any file header. Default implementation returns 0.
             * 
             * @return std::size_t The byte offset of the first particle record
             */
            virtual std::size_t   getParticleRecordStartOffset() const;
            
            /**
             * @brief Get the length in bytes of each particle record.
             * 
             * Must be implemented by derived classes for binary formatted files.
             * 
             * @return std::size_t The length of each particle record in bytes
             * @throws std::runtime_error if not implemented for binary format
             */
            virtual std::size_t   getParticleRecordLength() const;
            
            /**
             * @brief Get the number of particle records that fit in the file.
             * 
             * For binary files, calculates how many complete records fit in the file.
             * For other formats, returns getNumberOfParticles().
             * 
             * @return std::size_t The number of particle entries in the file
             */
                    std::size_t   getNumberOfEntriesInFile() const;

            /**
             * @brief Read a particle from binary data.
             * 
             * Must be implemented by derived classes that support binary format.
             * The default implementation throws an exception.
             * 
             * @param buffer The byte buffer containing the particle data
             * @return Particle The particle object parsed from binary data
             * @throws std::runtime_error if not implemented for binary format
             */
            virtual Particle      readBinaryParticle(ByteBuffer & buffer);
            
            /**
             * @brief Read a particle from ASCII data.
             * 
             * Must be implemented by derived classes that support ASCII format.
             * The default implementation throws an exception.
             * 
             * @param line The ASCII line containing the particle data
             * @return Particle The particle object parsed from ASCII data
             * @throws std::runtime_error if not implemented for ASCII format
             */
            virtual Particle      readASCIIParticle(const std::string & line); // not pure virtual to allow for binary format
            
            /**
             * @brief Read a particle manually (for formats requiring third-party I/O).
             * 
             * Can be implemented by derived classes to support manual file I/O,
             * circumventing the internal file stream and buffer.
             * 
             * Must be implemented by derived classes that specify FormatType::NONE.
             * The default implementation throws an exception.
             * 
             * @return Particle The manually entered particle object
             * @throws std::runtime_error if not implemented
             */
            virtual Particle      readParticleManually();

            /**
             * @brief Peek at a particle manually (for formats requiring third-party I/O).
             * 
             * Can be implemented by derived classes to support manual file I/O,
             * circumventing the internal file stream and buffer.
             * 
             * Must be implemented by derived classes that specify FormatType::NONE.
             * The default implementation throws an exception.
             * 
             * @return Particle The manually entered particle object
             * @throws std::runtime_error if not implemented
             */
            virtual Particle      peekParticleManually();
            
            /**
             * @brief Get the maximum line length for ASCII format files.
             * 
             * Must be implemented by derived classes that support ASCII format.
             * Used for buffer allocation and parsing optimization.
             * 
             * @return std::size_t The maximum length of ASCII lines in number of characters
             * @throws std::runtime_error if not implemented for ASCII format
             */
            virtual std::size_t   getMaximumASCIILineLength() const; // must be implemented for ASCII formatted files


            /**
             * @brief Get the file header data as a byte buffer.
             * 
             * Reads the entire header portion of the file into a ByteBuffer.
             * The header size is determined by getParticleRecordStartOffset().
             * 
             * @return const ByteBuffer The header data
             */
            const ByteBuffer      getHeaderData();
            
            /**
             * @brief Get a specific amount of header data as a byte buffer.
             * 
             * @param headerSize The number of bytes to read from the header
             * @return const ByteBuffer The header data of specified size
             */
            const ByteBuffer      getHeaderData(std::size_t headerSize);
            
            /**
             * @brief Set the byte order for binary data interpretation.
             * 
             * @param byteOrder The byte order to use (little-endian, big-endian, or PDP-endian)
             */
            void                  setByteOrder(ByteOrder byteOrder);

            /**
             * @brief Calculate the third component of a unit vector from two components (float precision).
             * 
             * Given two components of a unit vector, calculates the third component.
             * Handles normalization if the input components are not properly normalized.
             * 
             * @param u First component (may be modified for normalization)
             * @param v Second component (may be modified for normalization)
             * @return float The calculated third component
             */
            float                 calcThirdUnitComponent(float & u, float & v) const;
            
            /**
             * @brief Calculate the third component of a unit vector from two components (double precision).
             * 
             * Given two components of a unit vector, calculates the third component.
             * Handles normalization if the input components are not properly normalized.
             * 
             * @param u First component (may be modified for normalization)
             * @param v Second component (may be modified for normalization)
             * @return double The calculated third component
             */
            double                calcThirdUnitComponent(double & u, double & v) const;

            /**
             * @brief Get the user options that were passed to the constructor.
             * 
             * @return const UserOptions& Reference to the user options
             */
            const UserOptions&    getUserOptions() const;

        private:
            void                  readNextBlock();
            void                  bufferNextASCIILine();

            const std::string phspFormat_;
            const std::string fileName_;
            const UserOptions userOptions_;
            const FormatType formatType_;
            const int BUFFER_SIZE;
            std::ifstream file_;

            std::list<std::string> asciiLineBuffer_;
            std::vector<std::string> asciiCommentMarkers_;

            const std::uint64_t bytesInFile_;
            std::uint64_t bytesRead_;
            std::uint64_t particlesRead_;     /// counts all particle records even if they are skipped or are only meta-data particles
            std::uint64_t metaparticlesRead_; /// counts all metadata-only particles read which are not counted towards the reported number of particles in the file
            std::uint64_t particlesSkipped_;  /// counts all particles skipped by moveToParticle
            std::uint64_t historiesRead_;
            std::uint64_t numberOfParticlesToRead_;
            std::size_t particleRecordLength_;
            bool isFirstParticle_;
            ByteBuffer buffer_;

            FixedValues fixedValues_;
    };

    /**
     * @brief Special runtime exception class to catch EOF for ASCII formatted files.
     * 
     * This exception is thrown when attempting to read beyond the end of an ASCII
     * format phase space file. It allows for graceful handling of end-of-file
     * conditions during parsing.
     */
    class EndOfFileException : public std::runtime_error {
        public:
            /**
             * @brief Construct a new End Of File Exception object.
             * 
             * @param message Descriptive message about the EOF condition
             */
            explicit EndOfFileException(const std::string & message) : std::runtime_error(message) {}
    };

    // Inline implementations for the PhaseSpaceFileReader class

    inline const std::string PhaseSpaceFileReader::getPHSPFormat() const { return phspFormat_; }

    inline Particle PhaseSpaceFileReader::getNextParticle() {
        return getNextParticle(true); // count particle in statistics by default
    }
    inline std::uint64_t PhaseSpaceFileReader::getFileSize() const { return bytesInFile_; }
    inline const std::string PhaseSpaceFileReader::getFileName() const { return fileName_; }
    inline std::size_t PhaseSpaceFileReader::getParticleRecordStartOffset() const { return 0; }
    inline void PhaseSpaceFileReader::setByteOrder(ByteOrder byteOrder) { buffer_.setByteOrder(byteOrder); }
    inline const UserOptions& PhaseSpaceFileReader::getUserOptions() const { return userOptions_; }

    inline bool PhaseSpaceFileReader::isXConstant() const { return fixedValues_.xIsConstant; }
    inline bool PhaseSpaceFileReader::isYConstant() const { return fixedValues_.yIsConstant; }
    inline bool PhaseSpaceFileReader::isZConstant() const { return fixedValues_.zIsConstant; }
    inline bool PhaseSpaceFileReader::isPxConstant() const { return fixedValues_.pxIsConstant; }
    inline bool PhaseSpaceFileReader::isPyConstant() const { return fixedValues_.pyIsConstant; }
    inline bool PhaseSpaceFileReader::isPzConstant() const { return fixedValues_.pzIsConstant; }
    inline bool PhaseSpaceFileReader::isWeightConstant() const { return fixedValues_.weightIsConstant; }

    inline float PhaseSpaceFileReader::getConstantX() const { if (!fixedValues_.xIsConstant) throw std::runtime_error("X is not a constant"); return fixedValues_.constantX; }
    inline float PhaseSpaceFileReader::getConstantY() const { if (!fixedValues_.yIsConstant) throw std::runtime_error("Y is not a constant"); return fixedValues_.constantY; }
    inline float PhaseSpaceFileReader::getConstantZ() const { if (!fixedValues_.zIsConstant) throw std::runtime_error("Z is not a constant"); return fixedValues_.constantZ; }
    inline float PhaseSpaceFileReader::getConstantPx() const { if (!fixedValues_.pxIsConstant) throw std::runtime_error("Px is not a constant"); return fixedValues_.constantPx; }
    inline float PhaseSpaceFileReader::getConstantPy() const { if (!fixedValues_.pyIsConstant) throw std::runtime_error("Py is not a constant"); return fixedValues_.constantPy; }
    inline float PhaseSpaceFileReader::getConstantPz() const { if (!fixedValues_.pzIsConstant) throw std::runtime_error("Pz is not a constant"); return fixedValues_.constantPz; }
    inline float PhaseSpaceFileReader::getConstantWeight() const { if (!fixedValues_.weightIsConstant) throw std::runtime_error("Weight is not a constant"); return fixedValues_.constantWeight; }

    inline void PhaseSpaceFileReader::setConstantX(float X) { fixedValues_.xIsConstant = true; fixedValues_.constantX = X; }
    inline void PhaseSpaceFileReader::setConstantY(float Y) { fixedValues_.yIsConstant = true; fixedValues_.constantY = Y; }
    inline void PhaseSpaceFileReader::setConstantZ(float Z) { fixedValues_.zIsConstant = true; fixedValues_.constantZ = Z; }
    inline void PhaseSpaceFileReader::setConstantPx(float Px) { fixedValues_.pxIsConstant = true; fixedValues_.constantPx = Px; }
    inline void PhaseSpaceFileReader::setConstantPy(float Py) { fixedValues_.pyIsConstant = true; fixedValues_.constantPy = Py; }
    inline void PhaseSpaceFileReader::setConstantPz(float Pz) { fixedValues_.pzIsConstant = true; fixedValues_.constantPz = Pz; }
    inline void PhaseSpaceFileReader::setConstantWeight(float weight) { fixedValues_.weightIsConstant = true; fixedValues_.constantWeight = weight; }

    inline const FixedValues PhaseSpaceFileReader::getFixedValues() const { return fixedValues_; }

    inline std::uint64_t PhaseSpaceFileReader::getHistoriesRead() {
        if (!hasMoreParticles()) {
            // If we have reached the end of the file, we should set historiesRead_ to the total number of original histories
            // Unless we have already read more histories than that (which implies something is wrong - maybe bad header information, or maybe a bug - so keep the current count and let the caller handle it)
            historiesRead_ = std::max(getNumberOfOriginalHistories(), historiesRead_);
        }
        return historiesRead_;
    }

    inline std::uint64_t PhaseSpaceFileReader::getParticlesRead() { return getParticlesRead(false); }
    inline std::uint64_t PhaseSpaceFileReader::getParticlesRead(bool includeAllParticleRecords) { return includeAllParticleRecords ? particlesRead_ : particlesRead_ - metaparticlesRead_ - particlesSkipped_; }

    inline void PhaseSpaceFileReader::setCommentMarkers(const std::vector<std::string> & commentMarkers) {
        asciiCommentMarkers_ = commentMarkers;
    }

    inline std::size_t PhaseSpaceFileReader::getParticleRecordLength() const {
        throw std::runtime_error("getParticleRecordLength() must be implemented for binary formatted file writers.");
    }

    inline size_t PhaseSpaceFileReader::getMaximumASCIILineLength() const {
        throw std::runtime_error("getMaximumASCILineLength() must be implemented for ASCII formatted file readers.");
    }

    inline Particle PhaseSpaceFileReader::readBinaryParticle(ByteBuffer & buffer) {
        (void)buffer;
        throw std::runtime_error("readBinaryParticle() must be implemented for binary formatted file readers.");
    }

    inline Particle PhaseSpaceFileReader::readASCIIParticle(const std::string & line) {
        (void)line;
        throw std::runtime_error("readASCIIParticle() must be implemented for ASCII formatted file readers.");
    }

    inline Particle PhaseSpaceFileReader::readParticleManually() {
        throw std::runtime_error("readParticleManually() must be implemented for manual particle reading.");
    }

    inline Particle PhaseSpaceFileReader::peekParticleManually() {
        throw std::runtime_error("peekParticleManually() must be implemented for manual particle reading.");
    }

    inline std::size_t PhaseSpaceFileReader::getNumberOfEntriesInFile() const {
        // For binary files, return the number of times the record length fits into the file size minus the header size
        // For other formats, just return getNumberOfParticles()
        if (formatType_ != FormatType::BINARY) {
            return static_cast<std::size_t>(getNumberOfParticles());
        }
        std::size_t bytesInFile = static_cast<std::size_t>(bytesInFile_);
        std::size_t headerSize = getParticleRecordStartOffset();
        if (bytesInFile <= headerSize) {
            return 0;
        }
        std::size_t recordLength = getParticleRecordLength();
        return recordLength > 0 ? (bytesInFile - headerSize) / recordLength : 0;
    }

    inline float PhaseSpaceFileReader::calcThirdUnitComponent(float & u, float & v) const {
        const float uuvv = std::fma(u, u, v * v);
        if (uuvv > 1.f) [[unlikely]] {
            // assume w is 0 and renormalize u and v
            float normFactor = 1.f / std::sqrt(uuvv);
            u *= normFactor;
            v *= normFactor;
            return 0.f; // Exactly tangential
        }
        if (uuvv == 1.f) [[unlikely]] {
            return 0.f; // Exactly tangential
        }
        return std::sqrt(1.f - uuvv); // Standard form
    }

    inline double PhaseSpaceFileReader::calcThirdUnitComponent(double & u, double & v) const {
        const double uuvv = std::fma(u, u, v * v);
        if (uuvv > 1.0) [[unlikely]] {
            // assume w is 0 and renormalize u and v
            double normFactor = 1.0 / std::sqrt(uuvv);
            u *= normFactor;
            v *= normFactor;
            return 0.0; // Exactly tangential
        }
        if (uuvv == 1.0) [[unlikely]] {
            return 0.0; // Exactly tangential
        }
        return std::sqrt(1.0 - uuvv); // Standard form
    }    
} // namespace ParticleZoo