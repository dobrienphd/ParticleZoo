#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>

namespace ParticleZoo
{
    namespace MCPLphspFile {

        /*
         * Constants defining the MCPL (Monte Carlo Particle Lists) file format:
         * the fixed and variable header layout, the option-dependent particle
         * record layout, and the packing of the kinetic energy and direction
         * unit vector into three stored values.
         *
         * Reference: T. Kittelmann et al., "Monte Carlo Particle Lists: MCPL",
         * Computer Physics Communications 218 (2017) 17-42.
         */

        constexpr std::string_view MCPL_MAGIC = "MCPL";                    ///< File magic at offset 0

        constexpr unsigned int MCPL_FORMAT_VERSION_CURRENT = 3;            ///< Format version written (adaptive projection packing)
        constexpr unsigned int MCPL_FORMAT_VERSION_LEGACY  = 2;            ///< Oldest format version readable (octahedral packing)

        // The fixed portion of the header is:
        //   offset 0:  "MCPL" magic (4 bytes)
        //   offset 4:  format version as three ASCII digits (e.g. "003")
        //   offset 7:  endianness marker, 'L' (little) or 'B' (big)
        //   offset 8:  number of particles (uint64, rewritten when the file is closed)
        //   offset 16: eight uint32 option values (see the OPT_ indices below)
        //   offset 48: the universal weight (double), present only when OPT_HAS_UNIVERSAL_WEIGHT is set
        // followed by the variable portion: the source program name, the comments,
        // the blob keys and the blobs, each stored as a uint32 byte count followed
        // by the raw bytes (no null terminator, no padding).
        constexpr std::size_t MCPL_NPARTICLES_OFFSET   = 8;                ///< Byte offset of the particle count
        constexpr std::size_t MCPL_OPTIONS_OFFSET      = 16;               ///< Byte offset of the eight option values
        constexpr std::size_t MCPL_FIXED_HEADER_LENGTH = 48;               ///< Length of the fixed portion of the header

        // Indices into the eight uint32 option values at MCPL_OPTIONS_OFFSET
        constexpr std::size_t OPT_NCOMMENTS            = 0;                ///< Number of comment strings in the header
        constexpr std::size_t OPT_NBLOBS               = 1;                ///< Number of binary data blobs in the header
        constexpr std::size_t OPT_HAS_USERFLAGS        = 2;                ///< 1 if each particle record ends with a uint32 user flags value
        constexpr std::size_t OPT_HAS_POLARISATION     = 3;                ///< 1 if each particle record begins with a polarisation vector
        constexpr std::size_t OPT_SINGLE_PRECISION     = 4;                ///< 1 if floating point fields are 4 bytes wide instead of 8
        constexpr std::size_t OPT_UNIVERSAL_PDGCODE    = 5;                ///< Common PDG code for all particles (an int32; 0 means per-particle codes)
        constexpr std::size_t OPT_PARTICLE_SIZE        = 6;                ///< Length of each particle record in bytes
        constexpr std::size_t OPT_HAS_UNIVERSAL_WEIGHT = 7;                ///< 1 if a common weight for all particles follows the option values

        /**
         * @brief Compute the particle record length implied by a set of header options.
         *
         * Each record holds, in order: the polarisation vector (3 values, only when
         * enabled), the position (3 values), the packed kinetic energy and direction
         * (3 values), the time (1 value), the weight (1 value, omitted when a
         * universal weight is set), the PDG code (int32, omitted when a universal
         * code is set) and the user flags (uint32, only when enabled).
         *
         * @param singlePrecision True if floating point fields are 4 bytes wide
         * @param hasPolarisation True if records carry a polarisation vector
         * @param hasUniversalWeight True if the weight is stored once in the header
         * @param hasUniversalPDGCode True if the PDG code is stored once in the header
         * @param hasUserFlags True if records carry a user flags value
         * @return std::size_t The particle record length in bytes
         */
        constexpr std::size_t MCPLParticleRecordLength(bool singlePrecision, bool hasPolarisation,
                                                       bool hasUniversalWeight, bool hasUniversalPDGCode,
                                                       bool hasUserFlags)
        {
            std::size_t fpSize = singlePrecision ? sizeof(float) : sizeof(double);
            std::size_t fpCount = 7 + (hasPolarisation ? 3 : 0) + (hasUniversalWeight ? 0 : 1);
            return fpCount * fpSize
                 + (hasUniversalPDGCode ? 0 : sizeof(std::int32_t))
                 + (hasUserFlags ? sizeof(std::uint32_t) : 0);
        }

        /**
         * @brief Pack a direction unit vector and kinetic energy into three values
         *        (the "adaptive projection packing" of MCPL format version 3).
         *
         * The two smallest-magnitude direction components are stored as-is and the
         * largest is replaced by its reciprocal, whose magnitude of at least 1
         * tells the unpacker which component was dropped. The dropped component
         * only needs its sign preserved, so the third slot stores the (non-negative)
         * kinetic energy carrying that sign.
         *
         * @param u X component of the direction unit vector
         * @param v Y component of the direction unit vector
         * @param w Z component of the direction unit vector
         * @param kineticEnergy The kinetic energy (must not be negative)
         * @return std::array<double, 3> The three values to store
         */
        inline std::array<double, 3> MCPLPackDirectionAndEnergy(double u, double v, double w, double kineticEnergy)
        {
            double first, second, dropped;
            if (std::fabs(w) < std::max(std::fabs(u), std::fabs(v))) {
                double invW = (w != 0.0) ? 1.0 / w : std::numeric_limits<double>::infinity();
                if (std::fabs(u) >= std::fabs(v)) {
                    first = invW; second = v; dropped = u;
                } else {
                    first = u; second = invW; dropped = v;
                }
            } else {
                first = u; second = v; dropped = w;
            }
            return { first, second, std::copysign(kineticEnergy, dropped) };
        }

        /**
         * @brief Unpack the three stored values of an MCPL format version 3 record
         *        into a direction unit vector and kinetic energy.
         *
         * The reciprocal stored in place of the dropped direction component is
         * recognized by its magnitude exceeding 1; the dropped component itself is
         * reconstructed from unit normalization with its sign taken from the third
         * value, whose magnitude is the kinetic energy.
         *
         * @param packed The three stored values
         * @param u Receives the X component of the direction unit vector
         * @param v Receives the Y component of the direction unit vector
         * @param w Receives the Z component of the direction unit vector
         * @param kineticEnergy Receives the kinetic energy
         */
        inline void MCPLUnpackDirectionAndEnergy(const std::array<double, 3> & packed,
                                                 double & u, double & v, double & w, double & kineticEnergy)
        {
            kineticEnergy = std::fabs(packed[2]);
            double droppedSign = std::signbit(packed[2]) ? -1.0 : 1.0;
            auto reconstruct = [droppedSign](double a, double b) {
                return droppedSign * std::sqrt(std::max(0.0, 1.0 - (a * a + b * b)));
            };
            if (std::fabs(packed[0]) > 1.0) {
                w = 1.0 / packed[0];
                v = packed[1];
                u = reconstruct(v, w);
            } else if (std::fabs(packed[1]) > 1.0) {
                u = packed[0];
                w = 1.0 / packed[1];
                v = reconstruct(u, w);
            } else {
                u = packed[0];
                v = packed[1];
                w = reconstruct(u, v);
            }
        }

        /**
         * @brief Unpack the three stored values of an MCPL format version 2 record
         *        into a direction unit vector and kinetic energy.
         *
         * Format version 2 stored the direction with octahedral packing: the unit
         * sphere is mapped onto a unit octahedron whose lower half is folded
         * outward into the corners of the [-1,1] square, so two coordinates
         * determine the full vector. The kinetic energy is stored in the third
         * value; a negative sign on it marks a direction lying exactly in the
         * z = 0 plane (where the reconstructed z would otherwise carry noise).
         *
         * @param packed The three stored values
         * @param u Receives the X component of the direction unit vector
         * @param v Receives the Y component of the direction unit vector
         * @param w Receives the Z component of the direction unit vector
         * @param kineticEnergy Receives the kinetic energy
         */
        inline void MCPLUnpackDirectionAndEnergyLegacy(const std::array<double, 3> & packed,
                                                       double & u, double & v, double & w, double & kineticEnergy)
        {
            w = 1.0 - std::fabs(packed[0]) - std::fabs(packed[1]);
            if (w < 0.0) {
                // Unfold the lower hemisphere from the corners of the square
                u = (1.0 - std::fabs(packed[1])) * (packed[0] >= 0.0 ? 1.0 : -1.0);
                v = (1.0 - std::fabs(packed[0])) * (packed[1] >= 0.0 ? 1.0 : -1.0);
            } else {
                u = packed[0];
                v = packed[1];
            }
            double norm = 1.0 / std::sqrt(u * u + v * v + w * w);
            u *= norm;
            v *= norm;
            w *= norm;

            kineticEnergy = packed[2];
            if (std::signbit(packed[2])) {
                kineticEnergy = -kineticEnergy;
                w = 0.0;
            }
        }

    } // namespace MCPLphspFile

} // namespace ParticleZoo
