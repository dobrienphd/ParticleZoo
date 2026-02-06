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
     * @class G4PrimaryGeneratorAction
     * @brief Primary generator action for ParticleZoo phase space files.
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
     * 2. Pass the shared reader to each worker thread's G4PrimaryGeneratorAction along with the thread index
    */
    class G4PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
    {
        public:
            /**
             * @brief Constructor.
             * @param parallelReader Shared pointer to a ParticleBalancedParallelReader created in the master thread.
             *                       The reader must be configured with the appropriate number of threads.
             * @param threadIndex Index of the worker thread (0-based).
             */
            G4PrimaryGeneratorAction(std::shared_ptr<ParticleZoo::ParticleBalancedParallelReader> parallelReader, std::size_t threadIndex = 0);
            ~G4PrimaryGeneratorAction() override;

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

    inline void G4PrimaryGeneratorAction::SetTranslation(const G4ThreeVector & translation)
    {
        globalTranslation = translation;
        G4cout << "ParticleZoo::G4PrimaryGeneratorAction: Set global translation to "
               << globalTranslation << G4endl;
    }

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    inline void G4PrimaryGeneratorAction::SetRecycleNumber(std::uint32_t n)
    {
        recycleNumber = n;
        recycleWeightFactor = 1.0 / static_cast<G4double>(recycleNumber + 1);
        G4cout << "ParticleZoo::G4PrimaryGeneratorAction: Set recycle number to "
               << recycleNumber << ", weight factor: " << recycleWeightFactor << G4endl;
    }

};  // namespace ParticleZoo