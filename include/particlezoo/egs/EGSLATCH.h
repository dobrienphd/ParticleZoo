
#pragma once

#include "particlezoo/Particle.h"
#include "particlezoo/utilities/argParse.h"

namespace ParticleZoo::EGSphspFile
{
    inline CLICommand EGSLATCHOptionCommand { BOTH, "", "EGS-latch-option", "Specify the LATCH interpretation option when reading/writing EGS phase space files (1, 2, or 3)", { CLI_INT } };
    inline CLICommand EGSLATCHFilterCommand { NONE, "", "EGS-latch-filter", "Specify a bitmask filter to apply to the LATCH value when reading/writing EGS phase space files", { CLI_UINT } };

    constexpr unsigned int LATCH_NEGATIVE_CHARGE_BIT = 1u << 30;   ///< Set for negatively charged particles (electrons), 0-based bit 30
    constexpr unsigned int LATCH_POSITIVE_CHARGE_BIT = 1u << 29;   ///< Set for positively charged particles (positrons), 0-based bit 29
    constexpr unsigned int LATCH_CHARGE_BITS = LATCH_NEGATIVE_CHARGE_BIT | LATCH_POSITIVE_CHARGE_BIT;

    /**
     * @brief Set the charge bits of a LATCH value to match the given particle type.
     *
     * Clears bits 29-30 and re-sets them from the particle type; all other bits are preserved.
     * Mirrors EGSnrc's egs_phsp_scoring, which always re-sets the charge bits from the particle charge.
     */
    inline unsigned int SetLATCHChargeBits(unsigned int LATCH, const ParticleType type)
    {
        LATCH &= ~LATCH_CHARGE_BITS;
        if (type == ParticleType::Electron) {
            LATCH |= LATCH_NEGATIVE_CHARGE_BIT;
        } else if (type == ParticleType::Positron) {
            LATCH |= LATCH_POSITIVE_CHARGE_BIT;
        }
        return LATCH;
    }

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
                        // IS_SECONDARY_PARTICLE is deprecated but still maintained for backward
                        // compatibility until its removal; suppress the self-inflicted warning
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996)
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
                        particle.setBoolProperty(BoolPropertyType::IS_SECONDARY_PARTICLE, generation > 1);
#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
                    }
                }
                break;
            default:
                break;
        }
    }

    inline unsigned int ExtractLATCHFromParticle(const Particle & particle, const EGSLATCHOPTION latchOption)
    {
        // Use existing LATCH value if present, but always make the charge bits consistent with the particle type
        if (particle.hasIntProperty(IntPropertyType::EGS_LATCH)) {
            unsigned int LATCH = (unsigned int) particle.getIntProperty(IntPropertyType::EGS_LATCH);
            return SetLATCHChargeBits(LATCH, particle.getType());
        }

        // Construct LATCH value from particle properties
        // Bits 29-30: Particle charge (bit 30 = negative/electron, bit 29 = positive/positron, neither = neutral/photon)
        unsigned int LATCH = SetLATCHChargeBits(0, particle.getType());

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
                    const bool isPrimary = !particle.hasIntProperty(IntPropertyType::GENERATION) || particle.getIntProperty(IntPropertyType::GENERATION) == 1;
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