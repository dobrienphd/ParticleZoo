#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "particlezoo/PDGParticleCodes.h"
#include "particlezoo/utilities/units.h"

namespace ParticleZoo
{
    namespace MCNPphspFile {

        /*
         * Constants defining the MCNP surface source (SSW/RSSA) file formats:
         * the Fortran record layouts of the MCNP6 ("SF_00001"), MCNP5 and MCNPX
         * header variants, and the packing of particle types (and, for the older
         * formats, surface numbers) into the b value of each particle record.
         */

        constexpr std::string_view MCNP_FORMAT_ID = "SF_00001";            ///< The MCNP6 surface source format identifier

        /**
         * @brief The MCNP code family that wrote a surface source file.
         *
         * The three families differ in their header layouts and in how the particle
         * type and surface are packed into each record's b value.
         */
        enum class MCNPVariant {
            MCNP6,  ///< MCNP6 "SF_00001" files (format record + first record of 143 or 191 bytes)
            MCNPX,  ///< MCNPX files (first record of 163 or 167 bytes, 32-bit header counts)
            MCNP5   ///< MCNP5 files (first record of 143 bytes, no format record)
        };

        constexpr std::size_t PARTICLE_RECORD_DATA_LENGTH = 88;            ///< 11 doubles per particle record (SSB array)
        constexpr std::size_t PARTICLE_RECORD_LENGTH = PARTICLE_RECORD_DATA_LENGTH + 8; ///< Particle record length including the Fortran record length markers

        // The first and third header records grew between MCNP versions: MCNP 6.1.1
        // writes an 80-character title card and a 3-integer third record, while
        // MCNP 6.2/6.3 write a 128-character title card and pad the third record
        // to 20 integers. The Reader accepts both; the Writer emits the modern
        // layout unless the legacy header option is given.
        constexpr std::size_t FORMAT_RECORD_DATA_LENGTH        = 8;        ///< id (char*8)
        constexpr std::size_t FIRST_RECORD_FIXED_FIELDS_LENGTH = 63;       ///< kods (8) + vers (5) + lods (8) + idtms (19) + probs (19) + knods (int)
        constexpr std::size_t FIRST_RECORD_DATA_LENGTH_LEGACY  = 143;      ///< First record with an 80-character title card (MCNP 6.1.1)
        constexpr std::size_t FIRST_RECORD_DATA_LENGTH_MODERN  = 191;      ///< First record with a 128-character title card (MCNP 6.2/6.3)
        constexpr std::size_t SECOND_RECORD_DATA_LENGTH        = 32;       ///< np1, nrss, nrcd, njsw, niss
        constexpr std::size_t THIRD_RECORD_DATA_LENGTH_LEGACY  = 12;       ///< niwr, mipts, kjaq (MCNP 6.1.1)
        constexpr std::size_t THIRD_RECORD_DATA_LENGTH_MODERN  = 80;       ///< niwr, mipts, kjaq + 17 zero-padding integers (MCNP 6.2/6.3)
        constexpr std::size_t FOURTH_RECORD_DATA_LENGTH        = 20;       ///< jss, kst, n, scf(1) as written by the Writer (SO surface, one coefficient)
        constexpr std::size_t SUMMARY_RECORD_DATA_LENGTH       = 32;       ///< zero (double) + 6 integers as written by the Writer (njss=1, mipts=1)

        /// Total header length written by the Writer (each record is wrapped in two 4-byte length markers)
        constexpr std::size_t WRITER_HEADER_LENGTH_MODERN = (FORMAT_RECORD_DATA_LENGTH + 8)
                                                          + (FIRST_RECORD_DATA_LENGTH_MODERN + 8)
                                                          + (SECOND_RECORD_DATA_LENGTH + 8)
                                                          + (THIRD_RECORD_DATA_LENGTH_MODERN + 8)
                                                          + (FOURTH_RECORD_DATA_LENGTH + 8)
                                                          + (SUMMARY_RECORD_DATA_LENGTH + 8);

        /// Total header length written by the Writer when the legacy (MCNP 6.1.1) header layout is selected
        constexpr std::size_t WRITER_HEADER_LENGTH_LEGACY = (FORMAT_RECORD_DATA_LENGTH + 8)
                                                          + (FIRST_RECORD_DATA_LENGTH_LEGACY + 8)
                                                          + (SECOND_RECORD_DATA_LENGTH + 8)
                                                          + (THIRD_RECORD_DATA_LENGTH_LEGACY + 8)
                                                          + (FOURTH_RECORD_DATA_LENGTH + 8)
                                                          + (SUMMARY_RECORD_DATA_LENGTH + 8);

        constexpr std::size_t FIRST_RECORD_DATA_LENGTH_MCNPX  = 163;       ///< MCNPX first record (28-character lods field, 80-character title card)
        constexpr std::size_t SECOND_RECORD_DATA_LENGTH_MCNPX = 20;        ///< MCNPX second record (all counts are 32-bit integers)

        /// Total header length written by the Writer when the MCNP5 format is selected (no format record)
        constexpr std::size_t WRITER_HEADER_LENGTH_MCNP5 = (FIRST_RECORD_DATA_LENGTH_LEGACY + 8)
                                                         + (SECOND_RECORD_DATA_LENGTH + 8)
                                                         + (THIRD_RECORD_DATA_LENGTH_LEGACY + 8)
                                                         + (FOURTH_RECORD_DATA_LENGTH + 8)
                                                         + (SUMMARY_RECORD_DATA_LENGTH + 8);

        /// Total header length written by the Writer when the MCNPX format is selected (no format record)
        constexpr std::size_t WRITER_HEADER_LENGTH_MCNPX = (FIRST_RECORD_DATA_LENGTH_MCNPX + 8)
                                                         + (SECOND_RECORD_DATA_LENGTH_MCNPX + 8)
                                                         + (THIRD_RECORD_DATA_LENGTH_LEGACY + 8)
                                                         + (FOURTH_RECORD_DATA_LENGTH + 8)
                                                         + (SUMMARY_RECORD_DATA_LENGTH + 8);

        constexpr float shake = 1e-8f * s;                                 ///< MCNP time unit (one shake = 10 ns)

        /**
         * @brief MCNP6 particle type table: SSW type index to particle type.
         *
         * The particle type is packed into the |b| value of each record as
         * 8 * typeIndex, with bit 2 acting as an antiparticle flag, bits 0-1 as
         * auxiliary flags (bit 0 marks cell-source particles), and type index 37
         * marking a heavy ion whose A/Z/S are packed into the higher bits. The
         * antiparticle and heavy-ion encodings operate on the PDG codes that the
         * ParticleType enumerator values represent.
         */
        constexpr std::array<ParticleType, 37> MCNP6_SSW_TYPE_TABLE = {
            ParticleType::Unsupported,          // 0
            ParticleType::Neutron,              // 1
            ParticleType::Photon,               // 2
            ParticleType::Electron,             // 3
            ParticleType::Muon,                 // 4
            ParticleType::AntiNeutron,          // 5
            ParticleType::ElectronNeutrino,     // 6
            ParticleType::MuonNeutrino,         // 7
            ParticleType::Positron,             // 8
            ParticleType::Proton,               // 9
            ParticleType::Lambda,               // 10
            ParticleType::SigmaPlus,            // 11
            ParticleType::SigmaMinus,           // 12
            ParticleType::XiZero,               // 13
            ParticleType::XiMinus,              // 14
            ParticleType::OmegaMinus,           // 15
            ParticleType::AntiMuon,             // 16
            ParticleType::AntiElectronNeutrino, // 17
            ParticleType::AntiMuonNeutrino,     // 18
            ParticleType::AntiProton,           // 19
            ParticleType::PionPlus,             // 20
            ParticleType::PionZero,             // 21
            ParticleType::KaonPlus,             // 22
            ParticleType::KaonShort,            // 23
            ParticleType::KaonLong,             // 24
            ParticleType::AntiLambda,           // 25
            ParticleType::AntiSigmaPlus,        // 26
            ParticleType::AntiSigmaMinus,       // 27
            ParticleType::AntiXiZero,           // 28
            ParticleType::AntiXiMinus,          // 29
            ParticleType::AntiOmegaMinus,       // 30
            ParticleType::Deuteron,             // 31
            ParticleType::Triton,               // 32
            ParticleType::Helium3Nucleus,       // 33
            ParticleType::HeliumNucleus,        // 34
            ParticleType::AntiPionPlus,         // 35
            ParticleType::AntiKaonPlus };       // 36

        /**
         * @brief Decode the particle type packed into an SSW record's |b| value.
         *
         * |b| packs, from the least significant bit: two auxiliary flag bits
         * (bit 0 marks cell-source particles), an antiparticle bit, a 6-bit
         * particle type index, and - for heavy ions (type index 37) - the mass
         * number A (9 bits), charge number Z (7 bits) and excited state S.
         *
         * @param packedType The |b| value of the particle record, rounded to an integer
         * @return ParticleType The particle type, or ParticleType::Unsupported if it cannot be decoded
         */
        inline ParticleType getParticleTypeFromSSWType(long long packedType) {
            long long antibit = (packedType >> 2) & 1;
            long long typeIndex = (packedType >> 3) & 63;
            if (typeIndex <= 36) {
                ParticleType type = MCNP6_SSW_TYPE_TABLE[static_cast<std::size_t>(typeIndex)];
                return (antibit && type != ParticleType::Photon) ? getAntiParticleType(type) : type;
            }
            if (typeIndex == 37) {
                // Heavy ion: A/Z/S packed above the type index; assemble the nucleus PDG code
                long long A = (packedType >> 9) & 511;
                long long Z = (packedType >> 18) & 127;
                long long S = packedType >> 25;
                if (A < 1 || Z < 1 || A < Z || S > 9) return ParticleType::Unsupported;
                std::int32_t pdg = static_cast<std::int32_t>(1000000000 + 10000 * Z + 10 * A + S);
                return getParticleTypeFromPDGID(antibit ? -pdg : pdg);
            }
            return ParticleType::Unsupported;
        }

        /**
         * @brief Encode a particle type into an SSW |b| type value.
         *
         * The inverse of getParticleTypeFromSSWType(). Particle types present in the
         * MCNP6 type table are encoded directly as 8 * typeIndex; their charge
         * conjugates use the antiparticle bit; nuclei are packed as heavy ions.
         *
         * @param type The particle type
         * @return long long The |b| value to write, or 0 if the type has no MCNP equivalent
         */
        inline long long getSSWTypeFromParticleType(ParticleType type) {
            if (type == ParticleType::Unsupported || type == ParticleType::PseudoParticle) return 0;
            for (std::size_t i = 1; i <= 36; ++i) {
                if (MCNP6_SSW_TYPE_TABLE[i] == type) return 8LL * static_cast<long long>(i);
            }
            ParticleType conjugate = getAntiParticleType(type);
            if (conjugate != ParticleType::Unsupported) {
                for (std::size_t i = 1; i <= 36; ++i) {
                    if (MCNP6_SSW_TYPE_TABLE[i] == conjugate) return 8LL * static_cast<long long>(i) + 4; // antiparticle bit
                }
            }
            // Nuclei: the PDG code encoded by the ParticleType value is 10LZZZAAAI;
            // only L=0 nuclei have an MCNP encoding
            std::int32_t code = static_cast<std::int32_t>(type);
            std::int32_t absCode = code < 0 ? -code : code;
            if (absCode > 1000000000 && absCode <= 1009999990) {
                long long S = absCode % 10;
                long long rest = absCode / 10;
                long long A = rest % 1000;
                rest /= 1000;
                long long Z = rest % 1000;
                if (rest / 1000 != 100 || A < 1 || Z < 1 || A < Z) return 0;
                return (code < 0 ? 4LL : 0LL) + (37LL << 3) + (A << 9) + (Z << 18) + (S << 25);
            }
            return 0;
        }

        /**
         * @brief MCNPX particle type table: MCNPX type index to particle type.
         */
        constexpr std::array<ParticleType, 35> MCNPX_SSW_TYPE_TABLE = {
            ParticleType::Unsupported,          // 0
            ParticleType::Neutron,              // 1
            ParticleType::Photon,               // 2
            ParticleType::Electron,             // 3
            ParticleType::Muon,                 // 4
            ParticleType::Tau,                  // 5
            ParticleType::ElectronNeutrino,     // 6
            ParticleType::MuonNeutrino,         // 7
            ParticleType::TauNeutrino,          // 8
            ParticleType::Proton,               // 9
            ParticleType::Lambda,               // 10
            ParticleType::SigmaPlus,            // 11
            ParticleType::SigmaMinus,           // 12
            ParticleType::XiZero,               // 13
            ParticleType::XiMinus,              // 14
            ParticleType::OmegaMinus,           // 15
            ParticleType::Lambda_c_Plus,        // 16
            ParticleType::Xi_c_Plus,            // 17
            ParticleType::Xi_c_Zero,            // 18
            ParticleType::Lambda_b_Zero,        // 19
            ParticleType::PionPlus,             // 20
            ParticleType::PionZero,             // 21
            ParticleType::KaonPlus,             // 22
            ParticleType::KaonShort,            // 23
            ParticleType::KaonLong,             // 24
            ParticleType::DPlus,                // 25
            ParticleType::DZero,                // 26
            ParticleType::DsPlus,               // 27
            ParticleType::BPlus,                // 28
            ParticleType::BZero,                // 29
            ParticleType::Bs_Zero,              // 30
            ParticleType::Deuteron,             // 31
            ParticleType::Triton,               // 32
            ParticleType::Helium3Nucleus,       // 33
            ParticleType::HeliumNucleus };      // 34

        /**
         * @brief Decode an MCNPX particle type value into a particle type.
         *
         * MCNPX packs the type as an index into its 35-entry particle table, with
         * indices 401-434 marking antiparticles, an xy035 suffix marking heavy ions
         * encoded as (Z-1)*1000000 + A*1000 + 35 (or +435 for anti-ions), and a
         * couple of auxiliary hundreds digits that are stripped before retrying.
         *
         * @param mcnpxType The type value (|b| / 1000000 of the particle record)
         * @return ParticleType The particle type, or ParticleType::Unsupported if it cannot be decoded
         */
        inline ParticleType getParticleTypeFromMCNPXType(long long mcnpxType) {
            if (mcnpxType < 0) return ParticleType::Unsupported;
            if (mcnpxType <= 34) return MCNPX_SSW_TYPE_TABLE[static_cast<std::size_t>(mcnpxType)];
            if (mcnpxType >= 401 && mcnpxType <= 434)
                return mcnpxType == 402 ? ParticleType::Photon
                                        : getAntiParticleType(MCNPX_SSW_TYPE_TABLE[static_cast<std::size_t>(mcnpxType % 100)]);
            bool anti = false;
            if (mcnpxType % 1000 == 435) { anti = true; mcnpxType -= 400; }
            if (mcnpxType % 1000 == 35) {
                // Heavy ion encoded as (Z-1)*1000000 + A*1000 + 35; assemble the nucleus PDG code
                mcnpxType /= 1000;
                long long A = mcnpxType % 1000;
                if (!A) return ParticleType::Unsupported;
                mcnpxType /= 1000;
                if (mcnpxType / 1000) return ParticleType::Unsupported;
                long long ZM1 = mcnpxType % 1000;
                std::int32_t pdg = static_cast<std::int32_t>(1000000000 + (ZM1 + 1) * 10000 + A * 10);
                return getParticleTypeFromPDGID(anti ? -pdg : pdg);
            }
            // Strip auxiliary hundreds digits (2xx / 6xx) and retry
            long long j = (mcnpxType % 1000) / 100;
            if (j == 2 || j == 6) return getParticleTypeFromMCNPXType(mcnpxType - 200);
            return ParticleType::Unsupported;
        }

        /**
         * @brief Encode a particle type into an MCNPX particle type value.
         *
         * The inverse of getParticleTypeFromMCNPXType(). Particle types present in
         * the MCNPX particle table are encoded directly as their table index; their
         * charge conjugates as index + 400; ground-state nuclei as
         * (Z-1)*1000000 + A*1000 + 35 (or + 435 for anti-nuclei).
         *
         * @param type The particle type
         * @return long long The MCNPX type value, or 0 if the type has no MCNPX equivalent
         */
        inline long long getMCNPXTypeFromParticleType(ParticleType type) {
            if (type == ParticleType::Unsupported || type == ParticleType::PseudoParticle) return 0;
            for (std::size_t i = 1; i <= 34; ++i) {
                if (MCNPX_SSW_TYPE_TABLE[i] == type) return static_cast<long long>(i);
            }
            ParticleType conjugate = getAntiParticleType(type);
            if (conjugate != ParticleType::Unsupported) {
                for (std::size_t i = 1; i <= 34; ++i) {
                    if (MCNPX_SSW_TYPE_TABLE[i] == conjugate) return 400LL + static_cast<long long>(i);
                }
            }
            // Nuclei: the PDG code encoded by the ParticleType value is 10LZZZAAAI;
            // MCNPX can only encode L=0, I=0 nuclei
            std::int32_t code = static_cast<std::int32_t>(type);
            std::int32_t absCode = code < 0 ? -code : code;
            if (absCode > 1000000000 && absCode <= 1009999990) {
                long long S = absCode % 10;
                long long rest = absCode / 10;
                long long A = rest % 1000;
                rest /= 1000;
                long long Z = rest % 1000;
                if (S != 0 || rest / 1000 != 100 || A < 1 || Z < 1 || A < Z) return 0;
                return (Z - 1) * 1000000 + A * 1000 + (code < 0 ? 435 : 35);
            }
            return 0;
        }
    } // namespace MCNPphspFile

} // namespace ParticleZoo
