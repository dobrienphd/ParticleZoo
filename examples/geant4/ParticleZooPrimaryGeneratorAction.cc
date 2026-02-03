
#include "ParticleZooPrimaryGeneratorAction.hh"

#include "G4GlobalConfig.hh"
#include "G4Threading.hh"
#include "G4Event.hh"
#include "G4RunManager.hh"
#ifdef G4MULTITHREADED
    #include "G4MTRunManager.hh"
#endif

#include "particlezoo/utilities/formats.h"

namespace ParticleZoo
{

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    G4PrimaryGeneratorAction::G4PrimaryGeneratorAction(const std::string & phaseSpaceFilePath, const std::uint32_t partitionId, const std::uint32_t numberOfPartitions)
    : globalTranslation(G4ThreeVector(0,0,0)), partitionId(partitionId), numberOfPartitions(numberOfPartitions)
    {
        // Initialize the phase space reader
        UserOptions userOptions;
        phaseSpaceReader = FormatRegistry::CreateReader(phaseSpaceFilePath, userOptions);

        // Determine total number of particles and partitioning
        std::uint64_t totalNumberOfParticles = phaseSpaceReader->getNumberOfParticles();
        std::uint64_t particlesPerPartition = totalNumberOfParticles / numberOfPartitions;

        // Determine full range for this partition
        std::uint64_t fullRange_startIndex = partitionId * particlesPerPartition;
        std::uint64_t fullRange_endIndex = (partitionId == numberOfPartitions - 1) ? totalNumberOfParticles : fullRange_startIndex + particlesPerPartition;
        std::uint64_t particlesInThisPartition = fullRange_endIndex - fullRange_startIndex;

        // Default to full range
        startIndex = fullRange_startIndex;
        endIndex = fullRange_endIndex;

        // Recycling parameters
        recycleNumber = 0;
        recycleWeightFactor = 1.0;

        // Incremental histories handling
        historiesToWait = 0;
        historiesWaited = 0;

        // Get thread ID
        G4int threadId = G4Threading::G4GetThreadId();

    #ifdef G4MULTITHREADED
        if (threadId >= 0) {
            // Configure for multithreading
            const G4RunManager* runManager = G4RunManager::GetRunManager();
            const G4MTRunManager* mtRunManager = dynamic_cast<const G4MTRunManager*>(runManager);

            if (mtRunManager) {
                // Divide phase space particles among threads
                G4int nThreads = mtRunManager->GetNumberOfThreads();
                std::uint64_t particlesPerThread = particlesInThisPartition / nThreads;
                startIndex = fullRange_startIndex + threadId * particlesPerThread;
                endIndex = (threadId == nThreads - 1) ? fullRange_endIndex : startIndex + particlesPerThread;

                if (threadId > 0) { // No need to move for thread 0
                    // Move the reader to the start index for this thread
                    phaseSpaceReader->moveToParticle(startIndex);
                    // Make sure we are at the start of a new history
                    while (phaseSpaceReader->hasMoreParticles() && !phaseSpaceReader->peekNextParticle().isNewHistory()) {
                        phaseSpaceReader->getNextParticle();
                    }
                }
            }

            G4cout << "ParticleZoo::G4PrimaryGeneratorAction: Configured for multithreading. "
                   << "Thread ID: " << threadId
                   << ", Particle range: [" << startIndex << ", " << endIndex << ")" << G4endl;
        } else {
    #endif
            // Single-threaded configuration
            if (startIndex > 0) { // no need to move if starting at 0
                // Move to start index
                phaseSpaceReader->moveToParticle(startIndex);
                // Make sure we are at the start of a new history
                while (phaseSpaceReader->hasMoreParticles() && !phaseSpaceReader->peekNextParticle().isNewHistory()) {
                    phaseSpaceReader->getNextParticle();
                }
            }
            G4cout << "ParticleZoo::G4PrimaryGeneratorAction: Running in single-threaded mode." << G4endl;
    #ifdef G4MULTITHREADED
        }
    #endif
    }

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    G4PrimaryGeneratorAction::~G4PrimaryGeneratorAction()
    {
        if (phaseSpaceReader) phaseSpaceReader->close();
    }

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    void G4PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent)
    {
        // Ensure the reader is valid
        if (!phaseSpaceReader) return;

        // Handle incremental histories
        if (historiesToWait > 0 && historiesWaited < historiesToWait) {
            historiesWaited++;
            return;
        }

        // Check if there are more particles available
        bool hasMoreParticles = phaseSpaceReader->hasMoreParticles();

        // Peek at the next particle to determine incremental histories
        if (hasMoreParticles) {
            ParticleZoo::Particle particle = phaseSpaceReader->peekNextParticle();
            std::int32_t incrementalHistories = particle.getIncrementalHistories();
            if (incrementalHistories > historiesToWait + 1) {
                historiesToWait = incrementalHistories - 1;
                historiesWaited = 0;
                return;
            } else {
                historiesToWait = 0;
                historiesWaited = 0;
            }
        }

        // Current global particle index
        std::uint64_t currentIndex = startIndex + phaseSpaceReader->getParticlesRead();

        // Determine if we can accept the next particle
        bool acceptNextParticle = hasMoreParticles && currentIndex < endIndex;

        // Generate primary vertices if accepted
        if (acceptNextParticle) {

            // Lambda to read the next particle and create a G4PrimaryVertex
            auto ReadNextVertex = [this](const ParticleZoo::Particle & particle, float weightFactor = 1.0) -> G4PrimaryVertex* {
                // Define some unit conversions
                constexpr G4double energyUnit = CLHEP::MeV / ParticleZoo::MeV;
                constexpr G4double lengthUnit = CLHEP::cm / ParticleZoo::cm;

                // Get particle properties
                G4int pdgCode = static_cast<G4int>(particle.getPDGCode());
                G4double kineticEnergy = static_cast<G4double>(particle.getKineticEnergy()) * energyUnit;
                G4double weight = static_cast<G4double>(particle.getWeight()) * weightFactor;

                G4double particleX = static_cast<G4double>(particle.getX()) * lengthUnit;
                G4double particleY = static_cast<G4double>(particle.getY()) * lengthUnit;
                G4double particleZ = static_cast<G4double>(particle.getZ()) * lengthUnit;

                G4double directionX = static_cast<G4double>(particle.getDirectionalCosineX());
                G4double directionY = static_cast<G4double>(particle.getDirectionalCosineY());
                G4double directionZ = static_cast<G4double>(particle.getDirectionalCosineZ());

                // Apply global translation
                G4ThreeVector position(particleX, particleY, particleZ);
                position += this->globalTranslation;

                // Create primary vertex and particle
                G4PrimaryVertex* vertex = new G4PrimaryVertex(position, 0.0);
                G4PrimaryParticle* primary = new G4PrimaryParticle(pdgCode,
                                                                    directionX,
                                                                    directionY,
                                                                    directionZ);

                // Set kinetic energy and weight
                primary->SetKineticEnergy(kineticEnergy);
                primary->SetWeight(weight);

                // Set the primary particle to the vertex
                vertex->SetPrimary(primary);

                // Return the created vertex
                return vertex;
            };

           do {
                // Read the next particle from the phase space file
                Particle particle = phaseSpaceReader->getNextParticle();
                // If recycling is requested, create multiple vertices
                for (std::uint32_t r = 0; r <= recycleNumber; r++) {
                    // Create primary vertex and particle
                    G4PrimaryVertex* vertex = ReadNextVertex(particle, recycleWeightFactor);
                    // Add the primary vertex to the event
                    anEvent->AddPrimaryVertex(vertex);
                }
            } while (phaseSpaceReader->hasMoreParticles() && !phaseSpaceReader->peekNextParticle().isNewHistory());

        } else {

            // No more particles available - warn only once
            static bool warned = false;
            if (!warned) {
                G4cout << "No more particles available in phase space file for thread " << G4Threading::G4GetThreadId();
                if (numberOfPartitions > 1) {
                    G4cout << ", partition " << partitionId;
                }
                G4cout << G4endl;
                warned = true;
            }

        }
    }

} // namespace ParticleZoo