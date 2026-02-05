
#pragma once

#include "particlezoo/Particle.h"
#include "particlezoo/utilities/argParse.h"

namespace ParticleZoo::EGSphspFile
{
    inline CLICommand EGSLATCHOptionCommand { BOTH, "", "EGS-latch-option", "Specify the LATCH interpretation option when reading/writing EGS phase space files (1, 2, or 3)", { CLI_INT } };
    inline CLICommand EGSLATCHFilterCommand { NONE, "", "EGS-latch-filter", "Specify a bitmask filter to apply to the LATCH value when reading/writing EGS phase space files", { CLI_UINT } };

    /**
     * @brief Enumeration of supported LATCH interpretation options.
     * 
     * Defines how the LATCH bits in EGS phase space files are to be interpreted
     * when reading or writing particle data.
     * 
     * See the EGS documentation for details on each LATCH option.
     */
    enum class EGSLATCHOPTION : unsigned short {
        LATCH_OPTION_1 = 1, ///< Non-Inherited LATCH Setting : bits 1-23 store where the particle has been, secondaries do not inherit any bits from parent particles
        LATCH_OPTION_2 = 2, ///< Comprehensive LATCH Setting - default : bits 1-23 store where the particle has been, bit settings are inherited from parent particles
        LATCH_OPTION_3 = 3  ///< Comprehensive LATCH Setting 2 : bits 1-23 store where the particle has interacted instead of where it has been, bit settings are inherited from parent particles
    };

    inline void ApplyLATCHToParticle(Particle & particle, const unsigned int LATCH, const EGSLATCHOPTION latchOption)
    {
        particle.setIntProperty(IntPropertyType::EGS_LATCH, LATCH);

        bool isMultiPasser = ((LATCH >> 31) & 1) == 1;
        particle.setBoolProperty(BoolPropertyType::IS_MULTIPLE_CROSSER, isMultiPasser);

        // Attempt to get primary/secondary status from LATCH bits
        switch (latchOption) {
            case EGSLATCHOPTION::LATCH_OPTION_1:
                // Non-Inherited LATCH Setting
                // No information on secondary status is stored in this option
                break;
            case EGSLATCHOPTION::LATCH_OPTION_2: // same as LATCH_OPTION_3
            case EGSLATCHOPTION::LATCH_OPTION_3:
                // Comprehensive LATCH Settings
                // Particle is secondary if bits 24-28 are non-zero, otherwise primary
                {
                    // Proceed only if the particle generation hasn't been set by another property (since this is an imperfect method)
                    if (!particle.hasIntProperty(IntPropertyType::GENERATION)) {
                        // Extract the contents of bits 24-28 to determine if the particle is secondary. These bits indicate the region secondary particles are created, and are set to zero for primary particles
                        int contentsOfBits24to28 = (LATCH >> 24) & 0x1F;
                        // we don't know if the secondary is a first-generation secondary or higher, so we just set the generation to 2 if any secondary bits are set since it is definitely not a primary particle
                        const int generation = contentsOfBits24to28 != 0 ? 2 : 1;
                        particle.setIntProperty(IntPropertyType::GENERATION, generation);
                        particle.setBoolProperty(BoolPropertyType::IS_SECONDARY_PARTICLE, generation > 1);
                    }
                }
                break;
            default:
                break;
        }
    }

    inline unsigned int ExtractLATCHFromParticle(const Particle & particle, const EGSLATCHOPTION latchOption)
    {
        // Return existing LATCH value if present
        if (particle.hasIntProperty(IntPropertyType::EGS_LATCH)) {
            return (unsigned int) particle.getIntProperty(IntPropertyType::EGS_LATCH);
        }

        // Construct LATCH value from particle properties
        unsigned int LATCH = 0;

        // Bits 29-30: Particle charge
        // 00 = neutral (photon), 01 = negative (electron), 10 = positive (positron)
        switch (particle.getType()) {
            case ParticleType::Photon:
                LATCH |= (0 << 29);
                break;
            case ParticleType::Electron:
                LATCH |= (1 << 29);
                break;
            case ParticleType::Positron:
                LATCH |= (2 << 29);
                break;
            default:
                break;
        }

        if (particle.hasBoolProperty(BoolPropertyType::IS_MULTIPLE_CROSSER) && particle.getBoolProperty(BoolPropertyType::IS_MULTIPLE_CROSSER)) {
            LATCH |= (1 << 31); // Set bit 32
        } else {
            LATCH &= ~(1 << 31); // Clear bit 32
        }

        switch (latchOption) {
            case EGSLATCHOPTION::LATCH_OPTION_1:
                // Non-Inherited LATCH Setting
                // No information on secondary status is stored in this option
                break;
            case EGSLATCHOPTION::LATCH_OPTION_2:
            case EGSLATCHOPTION::LATCH_OPTION_3:
                // Comprehensive LATCH Settings
                // Set bits 24-28 based on secondary status
                // Normally these bits would encode the location where the particle has been or interacted
                // but since we don't have that information here we just set this 5-bit field to 0 for primary and 1 for secondary
                {
                    const bool isPrimary = particle.hasIntProperty(IntPropertyType::GENERATION) && particle.getIntProperty(IntPropertyType::GENERATION) == 1;
                    if (isPrimary) {
                        LATCH &= ~(0x1F << 24); // Clear bits 24-28 to indicate primary
                    } else {
                        LATCH |= (1 << 24); // Set bit 24 to indicate secondary
                    }
                }
                break;
            default:
                break;
        }

        return LATCH;
    }

    inline bool DoesParticlePassLATCHFilter(const Particle & particle, const unsigned int latchFilter)
    {
        if (!particle.hasIntProperty(IntPropertyType::EGS_LATCH)) {
            return false; // No LATCH property means it cannot pass the filter
        }

        unsigned int particleLATCH = (unsigned int) particle.getIntProperty(IntPropertyType::EGS_LATCH);
        return (particleLATCH & latchFilter) == latchFilter;
    }
}