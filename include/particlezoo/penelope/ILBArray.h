#pragma once

#include <array>

#include "particlezoo/Particle.h"

namespace ParticleZoo::Penelope
{

    /**
     * @brief Applies ILB1 value to a particle and sets secondary particle flag
     * 
     * ILB1 represents the generation of the particle in the PENELOPE simulation:
     * - 1: Primary particle (from original source)
     * - 2 or higher: Secondary particle (created during simulation)
     * 
     * This function also sets the IS_SECONDARY_PARTICLE boolean property accordingly.
     * 
     * @param particle The particle to modify
     * @param ilb1 The ILB1 value indicating particle generation (must be >= 1)
     * @throws std::runtime_error if ilb1 value is invalid (<1)
     */
    inline void ApplyILB1ToParticle(Particle & particle, const int ilb1)
    {
        if (ilb1 == 1) {
            particle.setBoolProperty(BoolPropertyType::IS_SECONDARY_PARTICLE, false);
        } else if (ilb1 > 1) {
            particle.setBoolProperty(BoolPropertyType::IS_SECONDARY_PARTICLE, true);
        } else {
            throw std::runtime_error("Invalid ILB1 value: " + std::to_string(ilb1));
        }
        particle.setIntProperty(IntPropertyType::PENELOPE_ILB1, ilb1);
    }

    /**
     * @brief Applies ILB2 value to a particle
     * 
     * ILB2 stores the particle type of the parent particle that created this particle.
     * Only meaningful if ILB1 > 1 (i.e., for secondary particles).
     * 
     * @param particle The particle to modify
     * @param ilb2 The parent particle type code
     */
    inline void ApplyILB2ToParticle(Particle & particle, const int ilb2) {
        particle.setIntProperty(IntPropertyType::PENELOPE_ILB2, ilb2);
    }

    /**
     * @brief Applies ILB3 value to a particle
     * 
     * ILB3 stores the interaction type that created this particle.
     * Only meaningful if ILB1 > 1 (i.e., for secondary particles).
     * 
     * @param particle The particle to modify
     * @param ilb3 The interaction type code that created this particle
     */
    inline void ApplyILB3ToParticle(Particle & particle, const int ilb3) {
        particle.setIntProperty(IntPropertyType::PENELOPE_ILB3, ilb3);
    }

    /**
     * @brief Applies ILB4 value to a particle
     * 
     * ILB4 indicates whether the particle was created by atomic relaxation.
     * If non-zero, stores the atomic transition that created the particle.
     * 
     * @param particle The particle to modify
     * @param ilb4 The atomic transition (0 if not created by atomic relaxation)
     */
    inline void ApplyILB4ToParticle(Particle & particle, const int ilb4) {
        particle.setIntProperty(IntPropertyType::PENELOPE_ILB4, ilb4);
    }

    /**
     * @brief Applies ILB5 value to a particle
     * 
     * ILB5 is a user-defined value that can be set for tracking purposes.
     * 
     * @param particle The particle to modify
     * @param ilb5 The user-defined tracking value
     */
    inline void ApplyILB5ToParticle(Particle & particle, const int ilb5) {
        particle.setIntProperty(IntPropertyType::PENELOPE_ILB5, ilb5);
    }

    /**
     * @brief Applies all five ILB values to a particle from an array
     * 
     * PENELOPE uses a five-value ILB array to track particle interaction history:
     * - ILB[0] (ILB1): Particle generation (1=primary, >1=secondary)
     * - ILB[1] (ILB2): Parent particle type (for secondaries)
     * - ILB[2] (ILB3): Interaction type that created the particle (for secondaries)
     * - ILB[3] (ILB4): Atomic transition (if created by atomic relaxation)
     * - ILB[4] (ILB5): User-defined value
     * 
     * @param particle The particle to modify
     * @param ilbArray Array containing the five ILB values in order [ILB1, ILB2, ILB3, ILB4, ILB5]
     */
    inline void ApplyILBArrayToParticle(Particle & particle, const std::array<int,5> & ilbArray)
    {
        ApplyILB1ToParticle(particle, ilbArray[0]);
        ApplyILB2ToParticle(particle, ilbArray[1]);
        ApplyILB3ToParticle(particle, ilbArray[2]);
        ApplyILB4ToParticle(particle, ilbArray[3]);
        ApplyILB5ToParticle(particle, ilbArray[4]);
    }

    /**
     * @brief Extracts ILB1 value from a particle
     * 
     * Retrieves the particle generation value. If PENELOPE_ILB1 property is set,
     * returns that value directly. Otherwise, attempts to infer from IS_SECONDARY_PARTICLE
     * flag (returns 2 if secondary, 1 if primary).
     * 
     * @param particle The particle to extract from
     * @return The ILB1 value (particle generation), 2 if secondary flag is set, 1 if primary flag is set, or 0 if neither is available
     */
    inline int ExtractILB1FromParticle(const Particle & particle)
    {
        if (particle.hasIntProperty(IntPropertyType::PENELOPE_ILB1)) {
            return particle.getIntProperty(IntPropertyType::PENELOPE_ILB1);
        } else if (particle.hasBoolProperty(BoolPropertyType::IS_SECONDARY_PARTICLE)) {
            return particle.getBoolProperty(BoolPropertyType::IS_SECONDARY_PARTICLE) ? 2 : 1;
        } else {
            return 0;
        }
    }

    /**
     * @brief Extracts ILB2 value from a particle
     * 
     * Retrieves the parent particle PENELOPE type code for secondary particles.
     * 
     * @param particle The particle to extract from
     * @return The ILB2 value (parent particle PENELOPE type), or 0 if not set
     */
    inline int ExtractILB2FromParticle(const Particle & particle)
    {
        if (particle.hasIntProperty(IntPropertyType::PENELOPE_ILB2)) {
            return particle.getIntProperty(IntPropertyType::PENELOPE_ILB2);
        } else {
            return 0;
        }
    }

    /**
     * @brief Extracts ILB3 value from a particle
     * 
     * Retrieves the interaction type PENELOPE code that created this secondary particle.
     * 
     * @param particle The particle to extract from
     * @return The ILB3 value (interaction type), or 0 if not set
     */
    inline int ExtractILB3FromParticle(const Particle & particle)
    {
        if (particle.hasIntProperty(IntPropertyType::PENELOPE_ILB3)) {
            return particle.getIntProperty(IntPropertyType::PENELOPE_ILB3);
        } else {
            return 0;
        }
    }

    /**
     * @brief Extracts ILB4 value from a particle
     * 
     * Retrieves the atomic transition if the particle was created by atomic relaxation.
     * 
     * @param particle The particle to extract from
     * @return The ILB4 value (atomic transition), or 0 if not created by atomic relaxation or not set
     */
    inline int ExtractILB4FromParticle(const Particle & particle)
    {
        if (particle.hasIntProperty(IntPropertyType::PENELOPE_ILB4)) {
            return particle.getIntProperty(IntPropertyType::PENELOPE_ILB4);
        } else {
            return 0;
        }
    }

    /**
     * @brief Extracts ILB5 value from a particle
     * 
     * Retrieves the user-defined tracking value
     * 
     * @param particle The particle to extract from
     * @return The ILB5 value (user-defined tracking value), or 0 if not set
     */
    inline int ExtractILB5FromParticle(const Particle & particle)
    {
        if (particle.hasIntProperty(IntPropertyType::PENELOPE_ILB5)) {
            return particle.getIntProperty(IntPropertyType::PENELOPE_ILB5);
        } else {
            return 0;
        }
    }

    /**
     * @brief Extracts all five ILB values from a particle into an array
     * 
     * Retrieves the complete PENELOPE ILB array from a particle's properties:
     * - ILB[0] (ILB1): Particle generation
     * - ILB[1] (ILB2): Parent particle type
     * - ILB[2] (ILB3): Interaction type that created the particle
     * - ILB[3] (ILB4): Atomic transition code
     * - ILB[4] (ILB5): User-defined value
     * 
     * @param particle The particle to extract from
     * @return Array containing the five ILB values in order [ILB1, ILB2, ILB3, ILB4, ILB5]
     */
    inline std::array<int,5> ExtractILBArrayFromParticle(const Particle & particle)
    {
        std::array<int,5> ilbArray = {
            ExtractILB1FromParticle(particle),
            ExtractILB2FromParticle(particle),
            ExtractILB3FromParticle(particle),
            ExtractILB4FromParticle(particle),
            ExtractILB5FromParticle(particle)
        };
        return ilbArray;
    }

} // namespace ParticleZoo::Penelope