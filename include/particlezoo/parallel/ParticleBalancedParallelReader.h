#pragma once

#include "particlezoo/PhaseSpaceFileReader.h"

#include <vector>
#include <memory>
#include <string>
#include <cstdint>
#include <limits>

namespace ParticleZoo {

    /**
     * @brief Multi-threaded phase space file reader for parallel particle processing.
     * 
     * ParticleBalancedParallelReader provides thread-safe parallel reading of phase space files by
     * creating multiple independent PhaseSpaceFileReader instances and partitioning the
     * particles across threads. Each thread maintains its own reader positioned at a
     * specific section of the phase space file.
     * 
     * The important property of this class is that it distributes particles evenly across
     * threads, rather than distributing histories evenly. This is for use-cases where
     * individual particles are the fundamental unit of work and optimal load balancing
     * by particle count is desired. This approach ensures that each thread processes
     * approximately the same number of particles, regardless of the distribution of
     * particles across histories. Borders between histories are still respected, but
     * some threads may read a different number of histories than others.
     * 
     * The reader automatically handles:
     * - Particle partitioning across threads with proper history boundary alignment
     * - Thread-safe particle retrieval without locking (each thread has its own reader)
     * - Incremental history number tracking across partition boundaries
     * 
     * @note Each thread must use its assigned thread index when calling methods.
     */
    class ParticleBalancedParallelReader {

        public:
            /**
             * @brief Constructs a multi-threaded phase space reader.
             * 
             * Creates multiple PhaseSpaceFileReader instances (one per thread) and positions
             * each at its designated starting particle. The constructor partitions the total
             * particles evenly across threads, with remainder particles distributed
             * to the first N threads.
             * 
             * @param filename Path to the phase space file to read
             * @param options User options for configuring the reader (format-specific settings)
             * @param numThreads Number of parallel threads that will read from this file
             * 
             * @throws std::runtime_error If the file cannot be opened or contains zero particles
             * @throws std::runtime_error If a PhaseSpaceFileReader cannot be created
             */
            ParticleBalancedParallelReader(const std::string& filename, const UserOptions& options = {}, size_t numThreads = 1);
            
            /**
             * @brief Destructor that closes all underlying readers.
             */
            ~ParticleBalancedParallelReader();

            /**
             * @brief Peeks at the next particle for a specific thread without consuming it.
             * 
             * Allows inspection of the next particle that would be returned by getNextParticle()
             * without advancing the reader's position. Useful for checking history boundaries.
             * 
             * @param threadIndex The index of the calling thread (0 to numThreads-1)
             * @return The next particle for this thread
             * 
             * @throws std::out_of_range If threadIndex is invalid
             * @throws std::runtime_error If no more particles are available for this thread
             * 
             * @note Must call hasMoreParticles() first to verify particles are available
             */
            Particle peekNextParticle(size_t threadIndex);

            /**
             * @brief Retrieves the next particle for a specific thread.
             * 
             * Reads and returns the next particle from the phase space file for the given
             * thread. Updates internal counters for history and particle tracking.
             * 
             * @param threadIndex The index of the calling thread (0 to numThreads-1)
             * @return The next particle for this thread
             * 
             * @throws std::out_of_range If threadIndex is invalid
             * @throws std::runtime_error If no more particles are available for this thread
             * 
             * @note Must call hasMoreParticles() first to verify particles are available
             * @note Sets the INCREMENTAL_HISTORY_NUMBER property on particles
             */
            Particle getNextParticle(size_t threadIndex);
            
            /**
             * @brief Checks if more particles are available for a specific thread.
             * 
             * Determines whether the thread has reached its partition boundary or the end
             * of the file.
             * 
             * @param threadIndex The index of the calling thread (0 to numThreads-1)
             * @return true if more particles are available, false otherwise
             * 
             * @throws std::out_of_range If threadIndex is invalid
             * 
             * @note Always call this before getNextParticle() to avoid exceptions
             */
            bool     hasMoreParticles(size_t threadIndex);
            
            /**
             * @brief Gets the total number of particles processed by a specific thread.
             * 
             * Returns the cumulative count of particles that have been processed by this thread.
             * This represents the contribution to the total particle count.
             * 
             * @param threadIndex The index of the thread (0 to numThreads-1)
             * @return Total number of particles processed
             * 
             * @throws std::out_of_range If threadIndex is invalid
             */
            std::uint64_t getParticlesRead(size_t threadIndex) const;

            /**
             * @brief Gets the total number of particles in the phase space file.
             * 
             * @return Total particle count in the file
             */
            std::uint64_t getNumberOfParticles() const { return numberOfParticlesInPhsp_; }
            
            /**
             * @brief Gets the number of original histories in the phase space file.
             * 
             * Returns the total number of histories that are in the phase space file, including
             * those that have no particles (empty histories).
             * 
             * @return Number of original histories (including empty ones)
             */
            std::uint64_t getNumberOfOriginalHistories() const { return numberOfOriginalHistories_; }

            /**
             * @brief Gets the number of represented histories in the file.
             * 
             * Returns the count of histories that actually produced particles and are
             * represented in the phase space file. This is always less than or equal to
             * the number of original histories.
             * 
             * @note If the phase space format does not support direct retrieval of this value,
             *       it will be computed on the first call by scanning the file which may be time-consuming.
             * @return Number of histories with at least one particle
             */
            std::uint64_t getNumberOfRepresentedHistories();

            /**
             * @brief Gets the number of threads used by this reader.
             * 
             * @return Number of parallel threads
             */
            std::size_t getNumberOfThreads() const { return numThreads_; }

            /**
             * @brief Closes all underlying phase space file readers.
             * 
             * Should be called when done reading to free file handles and resources.
             * Automatically called by the destructor.
             */
            void     close();

        private:
            std::string filename_;
            UserOptions options_;
            std::size_t numThreads_;

            std::vector<std::shared_ptr<PhaseSpaceFileReader>> readers_;
            std::vector<std::uint64_t> startingParticleIndex_;
            std::vector<std::uint64_t> particlesRead_;

            std::uint64_t numberOfOriginalHistories_;
            std::uint64_t numberOfParticlesInPhsp_;
            std::uint64_t numberOfRepresentedHistories_;
    };

}