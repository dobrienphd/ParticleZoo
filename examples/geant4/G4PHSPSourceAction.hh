/*
 * Example Geant4 Primary Generator Action using ParticleZoo phase space files.
 * Distributed as part of ParticleZoo. https://www.github.com/dobrienphd/ParticleZoo
 * 
 * MIT License
 * 
 * Copyright (c) 2025 Daniel O'Brien
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4Event.hh"
#include "G4ThreeVector.hh"

#include <memory>
#include <string>
#include <particlezoo/parallel/ParticleBalancedParallelReader.h>

namespace ParticleZoo
{

    /**
     * @class G4PHSPSourceAction
     * @brief Primary generator action for phase space files using ParticleZoo.
     * 
     * This class reads particles from a ParticleZoo phase space file and
     * generates primary vertices for Geant4 events.
     * 
     * Features:
     * - Handles incremental histories
     * - Supports multithreading via shared ParticleBalancedParallelReader
     * - Particles can be recycled multiple times with adjusted weights
     * 
     * Usage:
     * 1. Create a shared ParticleBalancedParallelReader in the master thread
     * 2. Pass the shared reader to each worker thread's G4PHSPSourceAction along with the thread index
    */
    class G4PHSPSourceAction : public G4VUserPrimaryGeneratorAction
    {
        public:
            /**
             * @brief Constructor.
             * @param parallelReader Shared pointer to a ParticleBalancedParallelReader created in the master thread.
             *                       The reader must be configured with the appropriate number of threads.
             * @param threadIndex Index of the worker thread (0-based).
             */
            G4PHSPSourceAction(std::shared_ptr<ParticleZoo::ParticleBalancedParallelReader> parallelReader, std::size_t threadIndex = 0);
            ~G4PHSPSourceAction() override;

            /**
             * @brief Generate primary vertices for the given event.
             * @param anEvent Pointer to the Geant4 event.
             */
            void GeneratePrimaries(G4Event* anEvent) override;

            /**
             * @brief Set the global translation to apply to all particle positions.
             * @param translation The translation vector.
             */
            void SetTranslation(const G4ThreeVector & translation);

            /**
             * @brief Set the number of times to recycle each particle.
             * @param n Number of times to recycle each particle.
             */
            void SetRecycleNumber(std::uint32_t n);

        private:
            // Shared parallel phase space file reader
            std::shared_ptr<ParticleZoo::ParticleBalancedParallelReader> parallelReader;

            // Thread index for this worker (0-based, aligned with Geant4 worker thread IDs)
            std::size_t threadIndex;

            // Global translation to apply to all particle positions
            G4ThreeVector globalTranslation;

            // Recycling parameters
            std::uint32_t recycleNumber;
            G4double recycleWeightFactor;

            // Incremental histories handling
            std::int32_t historiesToWait;
            std::int32_t historiesWaited;
    };

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    inline void G4PHSPSourceAction::SetTranslation(const G4ThreeVector & translation)
    {
        globalTranslation = translation;
        G4cout << "ParticleZoo::G4PHSPSourceAction: Set global translation to "
               << globalTranslation << G4endl;
    }

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    inline void G4PHSPSourceAction::SetRecycleNumber(std::uint32_t n)
    {
        recycleNumber = n;
        recycleWeightFactor = 1.0 / static_cast<G4double>(recycleNumber + 1);
        G4cout << "ParticleZoo::G4PHSPSourceAction: Set recycle number to "
               << recycleNumber << ", weight factor: " << recycleWeightFactor << G4endl;
    }

};  // namespace ParticleZoo