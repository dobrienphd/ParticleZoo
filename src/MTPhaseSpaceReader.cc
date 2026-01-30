
#include "particlezoo/MTPhaseSpaceReader.h"

#include "particlezoo/utilities/formats.h"

#include <thread>

namespace ParticleZoo {

    MTPhaseSpaceReader::MTPhaseSpaceReader(const std::string& filename, const UserOptions& options, size_t numThreads, std::uint64_t totalHistoriesToRead)
    : hasGapsBetweenHistories_(false), emptyHistoriesBetweenEachHistory_(0), perHistoryErrorContribution_(0), totalHistoriesToRead_(totalHistoriesToRead)
    {
        // Create PhaseSpaceFileReader instances for each thread
        readers_.reserve(numThreads);
        for (size_t i = 0; i < numThreads; ++i) {
            auto reader = FormatRegistry::CreateReader(filename, options);
            if (!reader) {
                throw std::runtime_error("Failed to create PhaseSpaceFileReader for file: " + filename);
            }
            readers_.emplace_back(std::move(reader));
        }

        // Determine the number of represented histories
        try {
            // Attempt to get the number of represented histories directly
            numberOfRepresentedHistories_ = readers_[0]->getNumberOfRepresentedHistories();
        } catch (const std::runtime_error&) {
            // Manually count the number of represented histories if not supported
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
        if (numberOfOriginalHistories_ < totalHistoriesToRead_) {
            totalHistoriesToRead_ = numberOfOriginalHistories_;
        }

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
            emptyHistoryErrors_.resize(numThreads);
        }

        // Calculate starting history for each thread
        startingHistorys_.reserve(numThreads);
        historiesRead_.reserve(numThreads);
        totalHistoriesRead_.resize(numThreads, 0);
        hasMoreParticlesCache_.resize(numThreads, NEEDS_CHECKING);
        std::uint64_t historiesPerThread = numberOfRepresentedHistories_ / numThreads;
        std::uint64_t remainderHistories = numberOfRepresentedHistories_ % numThreads;
        std::uint64_t currentStartingHistory = 0;
        for (size_t i = 0; i < numThreads; ++i) {
            startingHistorys_.push_back(currentStartingHistory);
            historiesRead_.push_back(currentStartingHistory);
            // Calculate the correct initial error for this thread based on how many
            // represented histories have been "processed" before this thread's starting point
            if (hasGapsBetweenHistories_) {
                std::uint64_t initialError = numberOfRepresentedHistories_ / 2;
                emptyHistoryErrors_[i] = (initialError + currentStartingHistory * perHistoryErrorContribution_) % numberOfRepresentedHistories_;
            }
            currentStartingHistory += historiesPerThread;
            if (i < remainderHistories) {
                currentStartingHistory += 1; // Distribute remainder histories
            }
        }

        // In separate threads, move each reader to its starting history
        std::vector<std::thread> threads;
        for (size_t i = 0; i < numThreads; ++i) {
            threads.emplace_back([this, i]() {
                std::uint64_t targetHistory = startingHistorys_[i];
                std::uint64_t currentHistory = 0;
                while (readers_[i]->hasMoreParticles() && currentHistory < targetHistory) {
                    const Particle particle = readers_[i]->peekNextParticle();
                    if (particle.isNewHistory()) {
                        currentHistory++;
                    }
                    if (currentHistory < targetHistory) {
                        readers_[i]->getNextParticle(); // Advance only if we haven't reached the target history
                    }
                }
            });
        }

        // Join threads
        for (auto& thread : threads) {
            thread.join();
        }
    }

    bool MTPhaseSpaceReader::hasMoreParticles(size_t threadIndex) {
        // Validate thread index
        if (threadIndex >= readers_.size()) {
            throw std::out_of_range("Thread index out of range in hasMoreParticles()");
        }

        // check the cache first
        if (hasMoreParticlesCache_[threadIndex] != NEEDS_CHECKING) {
            return hasMoreParticlesCache_[threadIndex] == HAS_MORE_PARTICLES;
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
            if (historiesRead_[threadIndex] == targetHistories) {
                // Check if the next particle would start a new history
                const Particle nextParticle = readers_[threadIndex]->peekNextParticle();
                // If the next particle starts a new history, we have no more particles for this thread
                hasMoreParticles = !nextParticle.isNewHistory();
            }
        }

        // Update the cache
        hasMoreParticlesCache_[threadIndex] = hasMoreParticles ? HAS_MORE_PARTICLES : NO_MORE_PARTICLES;

        return hasMoreParticles;
    }

    Particle MTPhaseSpaceReader::getNextParticle(size_t threadIndex) {
        // Validate thread index
        if (threadIndex >= readers_.size()) {
            throw std::out_of_range("Thread index out of range in getNextParticle()");
        }
        if (!hasMoreParticles(threadIndex)) {
            throw std::runtime_error("No more particles to read in getNextParticle() for thread index " + std::to_string(threadIndex));
        }

        // reset the cache since we are consuming a particle
        hasMoreParticlesCache_[threadIndex] = NEEDS_CHECKING;

        // Get the next particle from the appropriate reader
        Particle particle = readers_[threadIndex]->getNextParticle();

        // Update history count if this particle starts a new history
        int32_t incrementalHistoryNumber = 0;
        if (particle.isNewHistory()) {
            if (hasGapsBetweenHistories_) {
                emptyHistoryErrors_[threadIndex] += perHistoryErrorContribution_;
                if (emptyHistoryErrors_[threadIndex] >= numberOfRepresentedHistories_) {
                    incrementalHistoryNumber = 2 + emptyHistoriesBetweenEachHistory_;
                    emptyHistoryErrors_[threadIndex] -= numberOfRepresentedHistories_;
                } else {
                    incrementalHistoryNumber = 1 + emptyHistoriesBetweenEachHistory_;
                }
            } else {
                incrementalHistoryNumber = 1;
            }
            historiesRead_[threadIndex]++;
        } else {
            incrementalHistoryNumber = 0;
        }

        particle.setIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER, incrementalHistoryNumber);

        totalHistoriesRead_[threadIndex] += incrementalHistoryNumber;

        return particle;
    }

    std::uint64_t MTPhaseSpaceReader::getHistoriesRead(size_t threadIndex) const {
        // Validate thread index
        if (threadIndex >= readers_.size()) {
            throw std::out_of_range("Thread index out of range in getHistoriesRead()");
        }
        return totalHistoriesRead_[threadIndex];
    }

    void MTPhaseSpaceReader::close() {
        for (auto& reader : readers_) {
            reader->close();
        }
    }

    MTPhaseSpaceReader::~MTPhaseSpaceReader() {
        close();
    }

}  // namespace ParticleZoo