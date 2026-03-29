
#include "particlezoo/parallel/HistoryBalancedParallelReader.h"

#include "particlezoo/utilities/formats.h"

namespace ParticleZoo {

    HistoryBalancedParallelReader::HistoryBalancedParallelReader(const std::string& filename, const UserOptions& options, size_t numThreads)
    : hasGapsBetweenHistories_(false), emptyHistoriesBetweenEachHistory_(0), perHistoryErrorContribution_(0)
    {
        if (numThreads < 1) {
            throw std::invalid_argument("Number of threads must be at least 1 in HistoryBalancedParallelReader");
        }

        // Create PhaseSpaceFileReader instances for each thread
        readers_.reserve(numThreads);
        for (size_t i = 0; i < numThreads; ++i) {
            auto reader = FormatRegistry::CreateReader(filename, options);
            if (!reader) {
                throw std::runtime_error("Failed to create PhaseSpaceFileReader for file: " + filename);
            }
            readers_.emplace_back(std::move(reader));
        }

        // Detect if the format provides native represented history count or incremental history counters
        hasNativeRepresentedHistoryCount_ = readers_[0]->hasNativeRepresentedHistoryCount();
        hasNativeIncrementalHistoryCounters_ = readers_[0]->hasNativeIncrementalHistoryCounters();

        // Determine the number of represented histories
        if (hasNativeRepresentedHistoryCount_) {
            numberOfRepresentedHistories_ = readers_[0]->getNumberOfRepresentedHistories();
        } else {
            // Manually count the number of represented histories by scanning the file
            numberOfRepresentedHistories_ = 0;
            auto reader = FormatRegistry::CreateReader(filename, options);
            while (reader->hasMoreParticles()) {
                const Particle particle = reader->getNextParticle();
                if (particle.isNewHistory()) {
                    numberOfRepresentedHistories_++;
                }
            }
            reader->close();
        }

        if (numberOfRepresentedHistories_ == 0) {
            throw std::runtime_error("Phase space file contains zero represented histories: " + filename);
        }

        // Get the number of original histories and adjust totalHistoriesToRead_ if necessary
        numberOfOriginalHistories_ = readers_[0]->getNumberOfOriginalHistories();

        // Get the number of particles in the phase space
        numberOfParticlesInPhsp_ = readers_[0]->getNumberOfParticles();

        // Determine if there are gaps between histories
        std::uint64_t numberOfEmptyHistories = (numberOfRepresentedHistories_ < numberOfOriginalHistories_)
                                            ? (numberOfOriginalHistories_ - numberOfRepresentedHistories_)
                                            : 0;
        hasGapsBetweenHistories_ = numberOfEmptyHistories > 0;

        // Calculate gap distribution parameters if there are gaps
        if (hasGapsBetweenHistories_) {
            emptyHistoriesBetweenEachHistory_ = numberOfEmptyHistories / numberOfRepresentedHistories_;
            perHistoryErrorContribution_ = numberOfEmptyHistories % numberOfRepresentedHistories_;
        }

        // Calculate starting history for each thread
        startingHistorys_.reserve(numThreads);
        threadStats_.reserve(numThreads);
        for (size_t i = 0; i < numThreads; ++i) {
            threadStats_.emplace_back(std::make_unique<ThreadStatistics>());
        }
        std::uint64_t historiesPerThread = numberOfRepresentedHistories_ / numThreads;
        std::uint64_t remainderHistories = numberOfRepresentedHistories_ % numThreads;
        std::uint64_t currentStartingHistory = 0;
        for (size_t i = 0; i < numThreads; ++i) {
            startingHistorys_.push_back(currentStartingHistory);
            threadStats_[i]->historiesRead = currentStartingHistory;
            // Calculate the correct initial error for this thread based on how many
            // represented histories have been "processed" before this thread's starting point
            if (hasGapsBetweenHistories_) {
                std::uint64_t initialError = numberOfRepresentedHistories_ / 2;
                threadStats_[i]->emptyHistoryError = (initialError + currentStartingHistory * perHistoryErrorContribution_) % numberOfRepresentedHistories_;
            }
            currentStartingHistory += historiesPerThread;
            if (i < remainderHistories) {
                currentStartingHistory += 1; // Distribute remainder histories
            }
        }

        // Single-pass scan to find the particle index at each thread's starting history.
        if (numThreads > 1) {
            std::vector<std::uint64_t> startingParticleIndices(numThreads, 0);
            {
                auto scanner = FormatRegistry::CreateReader(filename, options);
                std::uint64_t historyCount = 0;
                std::uint64_t particleIndex = 0;
                size_t nextThreadToFind = 1; // Thread 0 always starts at particle 0

                while (nextThreadToFind < numThreads && scanner->hasMoreParticles()) {
                    const Particle particle = scanner->getNextParticle();
                    if (particle.isNewHistory()) {
                        if (historyCount == startingHistorys_[nextThreadToFind]) {
                            startingParticleIndices[nextThreadToFind] = particleIndex;
                            nextThreadToFind++;
                        }
                        historyCount++;
                    }
                    particleIndex++;
                }
                scanner->close();
            }

            // Position each reader at its starting particle using direct seek
            for (size_t i = 1; i < numThreads; ++i) {
                readers_[i]->moveToParticle(startingParticleIndices[i]);
            }
        }
    }

    bool HistoryBalancedParallelReader::hasMoreParticles(size_t threadIndex) {
        // Validate thread index
        if (threadIndex >= readers_.size()) {
            throw std::out_of_range("Thread index out of range in hasMoreParticles()");
        }

        // check the cache first
        if (threadStats_[threadIndex]->hasMoreParticlesCache != NEEDS_CHECKING) {
            return threadStats_[threadIndex]->hasMoreParticlesCache == HAS_MORE_PARTICLES;
        }

        // Check if the reader has more particles
        bool hasMoreParticles = readers_[threadIndex]->hasMoreParticles();

        // Additionally check if we have reached the target number of histories for this thread
        if (hasMoreParticles) {
            // Determine the target number of represented histories for this thread
            std::uint64_t targetHistories = threadIndex < readers_.size() - 1
                                          ? startingHistorys_[threadIndex + 1]
                                          : numberOfRepresentedHistories_;

            // Check if we have completed all histories for this thread
            if (threadStats_[threadIndex]->historiesRead >= targetHistories) {
                // Check if the next particle would start a new history
                const Particle nextParticle = readers_[threadIndex]->peekNextParticle();
                // If the next particle starts a new history, we have no more particles for this thread
                hasMoreParticles = !nextParticle.isNewHistory();
            }
        }

        // Update the cache
        threadStats_[threadIndex]->hasMoreParticlesCache = hasMoreParticles ? HAS_MORE_PARTICLES : NO_MORE_PARTICLES;

        return hasMoreParticles;
    }

    Particle HistoryBalancedParallelReader::getNextParticle(size_t threadIndex) {
        // Validate thread index
        if (threadIndex >= readers_.size()) {
            throw std::out_of_range("Thread index out of range in getNextParticle()");
        }
        if (!hasMoreParticles(threadIndex)) {
            throw std::runtime_error("No more particles to read in getNextParticle() for thread index " + std::to_string(threadIndex));
        }

        // reset the cache since we are consuming a particle
        threadStats_[threadIndex]->hasMoreParticlesCache = NEEDS_CHECKING;

        // Get the next particle from the appropriate reader
        Particle particle = readers_[threadIndex]->getNextParticle();

        // Update history count if this particle starts a new history
        int32_t incrementalHistoryNumber = 0;
        {
            std::lock_guard<std::mutex> lock(threadStats_[threadIndex]->mutex);
            threadStats_[threadIndex]->particlesRead++;

            if (particle.isNewHistory()) {
                if (hasGapsBetweenHistories_) {
                    threadStats_[threadIndex]->emptyHistoryError += perHistoryErrorContribution_;
                    if (threadStats_[threadIndex]->emptyHistoryError >= numberOfRepresentedHistories_) {
                        incrementalHistoryNumber = 2 + emptyHistoriesBetweenEachHistory_;
                        threadStats_[threadIndex]->emptyHistoryError -= numberOfRepresentedHistories_;
                    } else {
                        incrementalHistoryNumber = 1 + emptyHistoriesBetweenEachHistory_;
                    }
                } else {
                    incrementalHistoryNumber = 1;
                }
                threadStats_[threadIndex]->historiesRead++;
            }

            threadStats_[threadIndex]->totalHistoriesRead += incrementalHistoryNumber;
        }

        particle.setIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER, incrementalHistoryNumber);

        return particle;
    }

    Particle HistoryBalancedParallelReader::peekNextParticle(size_t threadIndex) {
        // Validate thread index
        if (threadIndex >= readers_.size()) {
            throw std::out_of_range("Thread index out of range in peekNextParticle()");
        }
        if (!hasMoreParticles(threadIndex)) {
            throw std::runtime_error("No more particles to read in peekNextParticle() for thread index " + std::to_string(threadIndex));
        }

        // Peek at the next particle from the appropriate reader
        return readers_[threadIndex]->peekNextParticle();
    }

    std::uint64_t HistoryBalancedParallelReader::getHistoriesRead(size_t threadIndex) const {
        // Validate thread index
        if (threadIndex >= readers_.size()) {
            throw std::out_of_range("Thread index out of range in getHistoriesRead()");
        }
        return threadStats_[threadIndex]->totalHistoriesRead;
    }

    std::uint64_t HistoryBalancedParallelReader::getParticlesRead(size_t threadIndex) const {
        if (threadIndex >= readers_.size()) {
            throw std::out_of_range("Thread index out of range in getParticlesRead()");
        }
        return threadStats_[threadIndex]->particlesRead;
    }

    std::uint64_t HistoryBalancedParallelReader::getTotalParticlesRead() const {
        std::uint64_t total = 0;
        for (size_t t = 0; t < readers_.size(); t++)
            total += threadStats_[t]->particlesRead;
        return total;
    }

    std::uint64_t HistoryBalancedParallelReader::getTotalHistoriesRead() const {
        std::uint64_t total = 0;
        for (size_t t = 0; t < readers_.size(); t++)
            total += threadStats_[t]->totalHistoriesRead;
        return total;
    }

    void HistoryBalancedParallelReader::close() {
        for (auto& reader : readers_) {
            reader->close();
        }
    }

    HistoryBalancedParallelReader::~HistoryBalancedParallelReader() {
        close();
    }

}  // namespace ParticleZoo