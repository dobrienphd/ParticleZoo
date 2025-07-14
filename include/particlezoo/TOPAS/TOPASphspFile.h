
#include "particlezoo/PhaseSpaceFileReader.h"
#include "particlezoo/PhaseSpaceFileWriter.h"
#include "particlezoo/TOPAS/TOPASHeader.h"

#include <cmath>

namespace ParticleZoo::TOPASphspFile
{
    constexpr std::size_t TOPASmaxASCIILineLength = 1024;

    // TOPASphspReader class definition

    class Reader : public PhaseSpaceFileReader
    {
        public:
            Reader(const std::string &filename, const UserOptions &options = UserOptions{});

            // Override methods from PhaseSpaceFileReader
            std::uint64_t  getNumberOfParticles() const override;
            std::uint64_t  getNumberOfOriginalHistories() const override;

            // New methods specific to TOPASphspReader
            TOPASFormat    getTOPASFormat() const;
            const Header & getHeader() const;

            void setDetailedReading(bool enable);

        protected:
            // Override methods from PhaseSpaceFileReader
            std::size_t    getParticleRecordLength() const override;
            Particle       readBinaryParticle(ByteBuffer & buffer) override;
            Particle       readASCIIParticle(const std::string & line) override;
            std::size_t    getMaximumASCIILineLength() const override;

        private:
            // Private constructor needed to enable reading from a file with a specific format
            Reader(const std::string &filename, const UserOptions &options, std::pair<FormatType,Header> formatAndHeader);

            Particle       readBinaryStandardParticle(ByteBuffer & buffer);
            Particle       readBinaryLimitedParticle(ByteBuffer & buffer);

            // helper to round float -> int32_t, "round half away from zero"
            std::int32_t   roundToInt32(float x);

            const Header header_;
            const TOPASFormat formatType_;
            const std::size_t particleRecordLength_;
            bool  readFullDetails_;
            std::int32_t emptyHistoriesCount_;
    };

    // Inline implementations for TOPASphspReader
    inline std::uint64_t Reader::getNumberOfParticles() const { return header_.getNumberOfParticles(); }
    inline std::uint64_t Reader::getNumberOfOriginalHistories() const { return header_.getNumberOfOriginalHistories(); }
    inline TOPASFormat Reader::getTOPASFormat() const { return formatType_; }
    inline const Header & Reader::getHeader() const { return header_; }
    inline void Reader::setDetailedReading(bool enable) { readFullDetails_ = enable; }
    inline std::size_t Reader::getParticleRecordLength() const { return particleRecordLength_; }
    inline std::size_t Reader::getMaximumASCIILineLength() const { return TOPASmaxASCIILineLength; }

    inline std::int32_t Reader::roundToInt32(float x) {
        // upper and lower bounds for int32_t in float representation
        constexpr float maxBound = static_cast<float>(std::numeric_limits<std::int32_t>::max()) - 0.5f;
        constexpr float minBound = static_cast<float>(std::numeric_limits<std::int32_t>::min()) + 0.5f;

        // cold path if out of range
        if (x > maxBound || x < minBound) {
            throw std::overflow_error("The TOPAS binary file being read contains an empty-history pseudoparticle mid-file with a weight that is outside the range of signed 32 bit integers. This is only supported if the pseudoparticle is at the end of the file.");
        }

        // add +0.5 if x>=0, or –0.5 if x<0, then truncate
        return static_cast<std::int32_t>(x + std::copysignf(0.5f, x));
    }

    inline Particle Reader::readBinaryParticle(ByteBuffer & buffer) {
        if (formatType_ == TOPASFormat::BINARY) {
            Particle particle = readBinaryStandardParticle(buffer);
            if (particle.getWeight() < 0 && particle.getType() == ParticleType::Unsupported) {
                // Special particle representing a sequence of empty histories
                emptyHistoriesCount_ += roundToInt32(-particle.getWeight());
                // need to read the next particle with the countParticleInStatistics flag set to false to avoid double counting
                return getNextParticle(false);
            } else if (emptyHistoriesCount_ > 0) {
                particle.setNewHistory(true); // make sure this flag is set, although it should be already
                std::int32_t incrementalHistoryNumber = particle.hasIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER) ? particle.getIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER) : 1;
                emptyHistoriesCount_ += incrementalHistoryNumber > 0 ? incrementalHistoryNumber : 1;
                particle.setIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER, emptyHistoriesCount_);
                emptyHistoriesCount_ = 0; // Reset after using
            }
            return particle;
        } else if (formatType_ == TOPASFormat::LIMITED) {
            return readBinaryLimitedParticle(buffer);
        } else {
            throw std::runtime_error("Unsupported format type for binary reading.");
        }
    };
    

    // TOPASphspWriter class definition


    class Writer : public PhaseSpaceFileWriter
    {
        public:
            Writer(const std::string &filename, const UserOptions &options = UserOptions{});

            // Override methods from PhaseSpaceFileWriter
            std::uint64_t getMaximumSupportedParticles() const override;

            // New methods specific to TOPASphspWriter
            TOPASFormat   getTOPASFormat() const;
            Header &      getHeader();

        protected:
            // Override methods from PhaseSpaceFileWriter
            std::size_t       getParticleRecordLength() const override;
            std::size_t       getMaximumASCIILineLength() const override;
            void              writeHeaderData(ByteBuffer & buffer) override;
            void              writeBinaryParticle(ByteBuffer & buffer, Particle & particle) override;
            const std::string writeASCIIParticle(Particle & particle) override;
            bool              accountForAdditionalHistories(std::uint64_t additionalHistories) override;

        private:
            Writer(const std::string &filename, const UserOptions &options, TOPASFormat formatType);

            // Specific methods for writing binary particles
            void              writeBinaryStandardParticle(ByteBuffer & buffer, Particle & particle);
            void              writeBinaryLimitedParticle(ByteBuffer & buffer, Particle & particle);
            
            const TOPASFormat formatType_;
            Header header_;
    };

    // Inline implementations for TOPASphspWriter

    inline std::uint64_t Writer::getMaximumSupportedParticles() const { return std::numeric_limits<std::uint64_t>::max(); }
    inline TOPASFormat Writer::getTOPASFormat() const { return formatType_; }
    inline Header & Writer::getHeader() { return header_; }
    inline std::size_t Writer::getParticleRecordLength() const { return header_.getRecordLength(); }
    inline std::size_t Writer::getMaximumASCIILineLength() const { return TOPASmaxASCIILineLength; }

    inline void Writer::writeBinaryParticle(ByteBuffer & buffer, Particle & particle) {
        if (formatType_ == TOPASFormat::BINARY) {
            if (particle.getType() != ParticleType::Unsupported && particle.hasIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER)) {
                // If the particle has an incremental history number, we need to handle it
                std::int32_t incrementalHistoryNumber = particle.getIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER);
                if (incrementalHistoryNumber > 1) {
                    accountForAdditionalHistories(incrementalHistoryNumber - 1);
                    particle.setIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER, 1); // Reset to 1 after accounting for additional histories
                }
            }
            writeBinaryStandardParticle(buffer, particle);
            header_.countParticleStats(particle);
        } else if (formatType_ == TOPASFormat::LIMITED) {
            writeBinaryLimitedParticle(buffer, particle);
            header_.countParticleStats(particle);
        } else {
            throw std::runtime_error("Unsupported format type for binary writing.");
        }
    };

    inline bool Writer::accountForAdditionalHistories(std::uint64_t additionalHistories)
    {
        float pseudoWeight = -static_cast<float>(additionalHistories);
        Particle emptyHistoryPseudoParticle(
            ParticleType::Unsupported, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, true, pseudoWeight
        );
        emptyHistoryPseudoParticle.setIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER, static_cast<std::int32_t>(additionalHistories));
        writeParticle(emptyHistoryPseudoParticle);
        return false; // do not increment the histories counter manually, it is done by the writeParticle() method
    }

} // namespace ParticleZoo::TOPASphsp