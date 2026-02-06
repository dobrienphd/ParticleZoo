
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

#include "G4PHSPSourceAction.hh"

#include "G4GlobalConfig.hh"
#include "G4Event.hh"

namespace ParticleZoo
{

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    G4PHSPSourceAction::G4PHSPSourceAction(std::shared_ptr<ParticleZoo::ParticleBalancedParallelReader> parallelReader, std::size_t threadIndex)
    : parallelReader(parallelReader),
      threadIndex(threadIndex),
      globalTranslation(G4ThreeVector(0,0,0)),
      recycleNumber(0),
      recycleWeightFactor(1.0),
      historiesToWait(0),
      historiesWaited(0)
    {
        G4cout << "ParticleZoo::G4PHSPSourceAction: Initialized for thread index " << threadIndex << G4endl;
    }

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    G4PHSPSourceAction::~G4PHSPSourceAction()
    {
        // Note: parallelReader is shared and will be closed when the last reference is released
    }

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    void G4PHSPSourceAction::GeneratePrimaries(G4Event* anEvent)
    {
        // Ensure the reader is valid
        if (!parallelReader) return;

        // Handle incremental histories
        if (historiesToWait > 0 && historiesWaited < historiesToWait) {
            historiesWaited++;
            return;
        }

        // Check if there are more particles available for this thread
        bool hasMoreParticles = parallelReader->hasMoreParticles(threadIndex);

        // Peek at the next particle to determine incremental histories
        if (hasMoreParticles) {
            ParticleZoo::Particle particle = parallelReader->peekNextParticle(threadIndex);
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

        // Generate primary vertices if particles are available
        if (hasMoreParticles) {

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
                Particle particle = parallelReader->getNextParticle(threadIndex);
                // If recycling is requested, create multiple vertices
                for (std::uint32_t r = 0; r <= recycleNumber; r++) {
                    // Create primary vertex and particle
                    G4PrimaryVertex* vertex = ReadNextVertex(particle, recycleWeightFactor);
                    // Add the primary vertex to the event
                    anEvent->AddPrimaryVertex(vertex);
                }
            } while (parallelReader->hasMoreParticles(threadIndex) && !parallelReader->peekNextParticle(threadIndex).isNewHistory());

        } else {

            // No more particles available - warn only once
            thread_local static bool warned = false;
            if (!warned) {
                G4cout << "No more particles available in phase space file for thread index " << threadIndex << G4endl;
                warned = true;
            }

        }
    }

} // namespace ParticleZoo