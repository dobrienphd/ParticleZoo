#pragma once

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4Event.hh"
#include "G4ThreeVector.hh"

#include <memory>
#include <string>
#include <particlezoo/PhaseSpaceFileReader.h>

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
     * - Supports multithreading by dividing the particle set among threads (i.e. can be used in multithreaded Geant4 applications)
     * - Supports partitioning for applications that run multiple instances of Geant4 to split the workload
     * - Particles can be recycled multiple times with adjusted weights
    */
    class G4PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
    {
        public:
            /**
             * @brief Constructor.
             * @param phaseSpaceFilePath Path to the ParticleZoo phase space file.
             * @param partitionId The partition ID for this instance (used for splitting workload across multiple application instances, default is 0).
             * @param numberOfPartitions The total number of partitions (used for splitting workload across multiple application instances, default is 1).
             */
            G4PrimaryGeneratorAction(const std::string & phaseSpaceFilePath, const std::uint32_t partitionId = 0, const std::uint32_t numberOfPartitions = 1);
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
            // Phase space file reader
            std::unique_ptr<ParticleZoo::PhaseSpaceFileReader> phaseSpaceReader;

            // Global translation to apply to all particle positions
            G4ThreeVector globalTranslation;

            // Particle range for this generator action
            std::uint64_t startIndex;
            std::uint64_t endIndex;

            // Phase-space partitioning information for multiple instances
            // for applications that don't use Geant4 multithreading but
            // want to split the workload across multiple G4 runs.
            std::uint32_t partitionId;
            std::uint32_t numberOfPartitions;

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