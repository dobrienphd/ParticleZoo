
#pragma once

#include "particlezoo/PhaseSpaceFileReader.h"

#include <vector>
#include <memory>
#include <string>
#include <cstdint>
#include <limits>
#include <mutex>
#include <atomic>

namespace ParticleZoo {

    /**
     * @brief Multi-threaded phase space file reader for parallel particle processing.
     * 
     * HistoryBalancedParallelReader provides thread-safe parallel reading of phase space files by
     * creating multiple independent PhaseSpaceFileReader instances and partitioning the
     * histories across threads. Each thread maintains its own reader positioned at a
     * specific section of the phase space file.
     * 
     * The important property of this class is that it distributes histories evenly across
     * threads, it does not necessarily distribute particles evenly. This is for use-cases
     * where histories are the fundamental unit of work. If load balancing by particle count
     * is desired, consider using a different approach. Although on average load balancing
     * by history should effectively balance particle counts as well, there is significant
     * overhead associated with dynamically distributing histories to threads so a dedicated
     * particle-based load balancing mechanism would be preferable in that scenario.
     * 
     * The reader automatically handles:
     * - History partitioning across threads with proper boundary alignment
     * - Empty history gap distribution for files with empty histories
     * - Thread-safe particle retrieval without locking (each thread has its own reader)
     * 
     * @note Each thread must use its assigned thread index when calling methods.
     * @note Incremental history counts are derived from represented histories only and do not reflect empty histories counts directly recorded in the phase space file if present.
     * @note Particle order within a history is preserved, but histories are processed
     *       in parallel across threads.
     */
    class HistoryBalancedParallelReader {

        enum HasMoreParticlesResult {
            HAS_MORE_PARTICLES,
            NO_MORE_PARTICLES,
            NEEDS_CHECKING
        };

        public:
            /**
             * @brief Constructs a multi-threaded phase space reader.
             * 
             * Creates multiple PhaseSpaceFileReader instances (one per thread) and positions
             * each at its designated starting history. The constructor partitions the total
             * represented histories evenly across threads, with remainder histories distributed
             * to the first N threads.
             * 
             * @param filename Path to the phase space file to read
             * @param options User options for configuring the reader (format-specific settings)
             * @param numThreads Number of parallel threads that will read from this file
             * 
             * @throws std::runtime_error If the file cannot be opened or contains zero histories
             * @throws std::runtime_error If a PhaseSpaceFileReader cannot be created
             */
            HistoryBalancedParallelReader(const std::string& filename, const UserOptions& options = {}, size_t numThreads = 1);
            
            /**
             * @brief Destructor that closes all underlying readers.
             */
            ~HistoryBalancedParallelReader();

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
             * thread. Updates internal counters for history tracking and automatically
             * distributes empty history gaps when present.
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
             * of the file. Uses caching to avoid repeated checks on consecutive calls.
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
             * @brief Gets the total number of histories processed by a specific thread.
             * 
             * Returns the cumulative count of histories (including empty histories)
             * that have been processed by this thread. This represents the contribution
             * to the total original history count.
             * 
             * @param threadIndex The index of the thread (0 to numThreads-1)
             * @return Total number of original histories processed (including empty ones)
             * 
             * @throws std::out_of_range If threadIndex is invalid
             */
            std::uint64_t getHistoriesRead(size_t threadIndex) const;

            /**
             * @brief Gets the total number of particles processed by a specific thread.
             *
             * Returns the cumulative count of particles that have been read by this thread.
             *
             * @param threadIndex The index of the thread (0 to numThreads-1)
             * @return Total number of particles processed
             *
             * @throws std::out_of_range If threadIndex is invalid
             */
            std::uint64_t getParticlesRead(size_t threadIndex) const;

            /**
             * @brief Gets the total number of particles read across all threads.
             * 
             * @return Total number of particles read
             */
            std::uint64_t getTotalParticlesRead() const;

            /**
             * @brief Gets the total number of original histories read across all threads.
             *
             * @return Total number of original histories read
             */
            std::uint64_t getTotalHistoriesRead() const;

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
             * @return Number of histories with at least one particle
             */
            std::uint64_t getNumberOfRepresentedHistories() const { return numberOfRepresentedHistories_; }

            /**
             * @brief Gets the number of threads used by this reader.
             *
             * @return Number of parallel threads
             */
            std::size_t getNumberOfThreads() const { return readers_.size(); }

            /**
             * @brief Checks if the underlying phase space format provides native represented history count.
             * 
             * @return True if native represented history count is available, false otherwise
             */
            bool hasNativeRepresentedHistoryCount() const;

            /**
             * @brief Checks if the underlying phase space format provides native incremental history counters.
             * 
             * @return True if native incremental history counters are available, false otherwise
             */
            bool hasNativeIncrementalHistoryCounters() const;

            /**
             * @brief Closes all underlying phase space file readers.
             * 
             * Should be called when done reading to free file handles and resources.
             * Automatically called by the destructor.
             */
            void     close();

        private:
            struct ThreadStatistics {
                mutable std::mutex mutex; // For thread-safe updates
                std::atomic<std::uint64_t> particlesRead = 0;
                std::atomic<std::uint64_t> totalHistoriesRead = 0;
                // Per-thread-only fields (no cross-thread access needed)
                std::uint64_t historiesRead = 0;
                std::uint64_t emptyHistoryError = 0;
                HasMoreParticlesResult hasMoreParticlesCache = NEEDS_CHECKING;
            };

            std::vector<std::shared_ptr<PhaseSpaceFileReader>> readers_;
            std::vector<ThreadStatistics> threadStats_;
            std::vector<std::uint64_t> startingHistorys_;

            bool hasNativeRepresentedHistoryCount_;
            bool hasNativeIncrementalHistoryCounters_;
            bool hasGapsBetweenHistories_;
            std::uint64_t numberOfOriginalHistories_;
            std::uint64_t numberOfParticlesInPhsp_;
            std::uint64_t numberOfRepresentedHistories_;
            std::uint64_t emptyHistoriesBetweenEachHistory_;
            std::uint64_t perHistoryErrorContribution_;
    };

}