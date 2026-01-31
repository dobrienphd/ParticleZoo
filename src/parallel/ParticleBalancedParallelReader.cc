
#include "particlezoo/parallel/ParticleBalancedParallelReader.h"

#include "particlezoo/utilities/formats.h"

#include <thread>

namespace ParticleZoo {

    ParticleBalancedParallelReader::ParticleBalancedParallelReader(const std::string& filename, const UserOptions& options, size_t numThreads)
    : filename_(filename), options_(options), numThreads_(numThreads)
    {
        if (numThreads < 1) {
            throw std::invalid_argument("Number of threads must be at least 1 in ParticleBalancedParallelReader");
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

        // Get the number of original histories
        numberOfOriginalHistories_ = readers_[0]->getNumberOfOriginalHistories();

        // Get the number of particles in the phase space
        numberOfParticlesInPhsp_ = readers_[0]->getNumberOfParticles();

        // Leave numberOfRepresentedHistories_ undetermined for now, lazily compute it later if needed
        numberOfRepresentedHistories_ = 0;

        std::uint64_t particlesPerThread = numberOfParticlesInPhsp_ / numThreads;
        std::uint64_t remainderParticles = numberOfParticlesInPhsp_ % numThreads;

        // Position each reader at its starting particle
        startingParticleIndex_.reserve(numThreads);
        particlesRead_.resize(numThreads, 0);
        std::uint64_t currentParticleIndex = 0;
        for (size_t i = 0; i < numThreads; ++i) {
            std::uint64_t particlesToRead = particlesPerThread + (i < remainderParticles ? 1 : 0);

            // Advance the reader to the starting particle
            std::uint64_t targetParticleIndex = currentParticleIndex;
            currentParticleIndex += particlesToRead;

            // Move the reader to the target particle, respecting history boundaries
            readers_[i]->moveToParticle(targetParticleIndex);
            while (readers_[i]->hasMoreParticles() && !readers_[i]->peekNextParticle().isNewHistory()) {
                // Skip ahead to the next history boundary
                readers_[i]->getNextParticle();
                targetParticleIndex++;
            }
            startingParticleIndex_.push_back(targetParticleIndex);
        }
    }

    bool ParticleBalancedParallelReader::hasMoreParticles(size_t threadIndex) {
        // Validate thread index
        if (threadIndex >= readers_.size()) {
            throw std::out_of_range("Thread index out of range in hasMoreParticles()");
        }

        // Check if the reader has more particles
        bool hasMore = readers_[threadIndex]->hasMoreParticles();
        if (hasMore) {
            std::uint64_t targetParticleIndex = (threadIndex < readers_.size() - 1)
                                            ? startingParticleIndex_[threadIndex + 1]
                                            : numberOfParticlesInPhsp_;
            // Check if we have completed all particles for this thread
            if (particlesRead_[threadIndex] >= (targetParticleIndex - startingParticleIndex_[threadIndex])) {
                hasMore = false;
            }
        }

        return hasMore;
    }

    Particle ParticleBalancedParallelReader::getNextParticle(size_t threadIndex) {
        // Validate thread index
        if (threadIndex >= readers_.size()) {
            throw std::out_of_range("Thread index out of range in getNextParticle()");
        }
        if (!hasMoreParticles(threadIndex)) {
            throw std::runtime_error("No more particles to read in getNextParticle() for thread index " + std::to_string(threadIndex));
        }

        // Get the next particle from the appropriate reader
        Particle particle = readers_[threadIndex]->getNextParticle();
        particlesRead_[threadIndex]++;
        return particle;
    }

    Particle ParticleBalancedParallelReader::peekNextParticle(size_t threadIndex) {
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

    std::uint64_t ParticleBalancedParallelReader::getNumberOfRepresentedHistories() {
        if (numberOfRepresentedHistories_ == 0) {
            // Lazily compute the number of represented histories
            try {
                // Attempt to get the number of represented histories directly
                numberOfRepresentedHistories_ = readers_[0]->getNumberOfRepresentedHistories();
            } catch (const std::runtime_error&) {
                // Manually count the number of represented histories if not supported
                numberOfRepresentedHistories_ = 0;
                auto reader = FormatRegistry::CreateReader(filename_, options_);
                while (reader->hasMoreParticles()) {
                    const Particle particle = reader->getNextParticle();
                    if (particle.isNewHistory()) {
                        numberOfRepresentedHistories_++;
                    }
                }
                reader->close();
            }
        }
        return numberOfRepresentedHistories_;
    }

    std::uint64_t ParticleBalancedParallelReader::getParticlesRead(size_t threadIndex) const {
        // Validate thread index
        if (threadIndex >= readers_.size()) {
            throw std::out_of_range("Thread index out of range in getHistoriesRead()");
        }
        return particlesRead_[threadIndex];
    }

    void ParticleBalancedParallelReader::close() {
        for (auto& reader : readers_) {
            reader->close();
        }
    }

    ParticleBalancedParallelReader::~ParticleBalancedParallelReader() {
        close();
    }

}