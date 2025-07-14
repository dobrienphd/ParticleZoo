#pragma once

#include <cstdint>
#include <string_view>

namespace ParticleZoo {

    // PDG (Particle Data Group) codes are used to uniquely identify particles in high-energy physics.
    // The PDG codes are standardized and used widely in particle physics to represent particles, antiparticles, and resonances.
    // The codes are integers, with positive values typically representing particles and negative values representing antiparticles.
    // The PDG codes are defined in the Particle Data Group's Particle Listings, which can be found at https://pdg.lbl.gov/

    // ——— 1) List every (name, PDG-code) once ———
    #define PARTICLE_LIST \
        /* Quarks */ \
        X(DownQuark,                       1) \
        X(UpQuark,                         2) \
        X(StrangeQuark,                    3) \
        X(CharmQuark,                      4) \
        X(BottomQuark,                     5) \
        X(TopQuark,                        6) \
        X(AntiDownQuark,                  -1) \
        X(AntiUpQuark,                    -2) \
        X(AntiStrangeQuark,               -3) \
        X(AntiCharmQuark,                 -4) \
        X(AntiBottomQuark,                -5) \
        X(AntiTopQuark,                   -6) \
        X(BPrimeQuark,                     7) \
        X(AntiBPrimeQuark,                -7) \
        X(TPrimeQuark,                     8) \
        X(AntiTPrimeQuark,                -8) \
        \
        /* Leptons */ \
        X(Electron,                       11) \
        X(Positron,                      -11) \
        X(ElectronNeutrino,              12) \
        X(AntiElectronNeutrino,         -12) \
        X(Muon,                          13) \
        X(AntiMuon,                     -13) \
        X(MuonNeutrino,                 14) \
        X(AntiMuonNeutrino,            -14) \
        X(Tau,                          15) \
        X(AntiTau,                     -15) \
        X(TauNeutrino,                 16) \
        X(AntiTauNeutrino,            -16) \
        X(TauPrime,                     17) \
        X(AntiTauPrime,                -17) \
        X(TauPrimeNeutrino,            18) \
        X(AntiTauPrimeNeutrino,       -18) \
        \
        /* Bosons */ \
        X(Gluon,                         21) \
        X(Photon,                        22) \
        X(ZBoson,                        23) \
        X(WBoson,                        24) \
        X(AntiWBoson,                   -24) \
        X(HiggsBoson,                    25) \
        X(ZPrimeBoson,                   32) \
        X(ZDoublePrimeBoson,             33) \
        X(WPrimeBoson,                   34) \
        X(AntiWPrimeBoson,              -34) \
        X(NeutralHiggsBoson,             35) \
        X(PseudoscalarHiggsBoson,        36) \
        X(ChargedHiggsBoson,             37) \
        X(AntiChargedHiggsBoson,        -37) \
        \
        /* Diquarks */ \
        X(Diquark_dd_1,                   1103) \
        X(AntiDiquark_dd_1,              -1103) \
        X(Diquark_ud_0,                   2101) \
        X(AntiDiquark_ud_0,              -2101) \
        X(Diquark_ud_1,                   2103) \
        X(AntiDiquark_ud_1,              -2103) \
        X(Diquark_uu_1,                   2203) \
        X(AntiDiquark_uu_1,              -2203) \
        X(Diquark_sd_0,                   3101) \
        X(AntiDiquark_sd_0,              -3101) \
        X(Diquark_sd_1,                   3103) \
        X(AntiDiquark_sd_1,              -3103) \
        X(Diquark_su_0,                   3201) \
        X(AntiDiquark_su_0,              -3201) \
        X(Diquark_su_1,                   3203) \
        X(AntiDiquark_su_1,              -3203) \
        X(Diquark_ss_1,                   3303) \
        X(AntiDiquark_ss_1,              -3303) \
        X(Diquark_cd_0,                   4101) \
        X(AntiDiquark_cd_0,              -4101) \
        X(Diquark_cd_1,                   4103) \
        X(AntiDiquark_cd_1,              -4103) \
        X(Diquark_cu_0,                   4201) \
        X(AntiDiquark_cu_0,              -4201) \
        X(Diquark_cu_1,                   4203) \
        X(AntiDiquark_cu_1,              -4203) \
        X(Diquark_cs_0,                   4301) \
        X(AntiDiquark_cs_0,              -4301) \
        X(Diquark_cs_1,                   4303) \
        X(AntiDiquark_cs_1,              -4303) \
        X(Diquark_cc_1,                   4403) \
        X(AntiDiquark_cc_1,              -4403) \
        X(Diquark_bd_0,                   5101) \
        X(AntiDiquark_bd_0,              -5101) \
        X(Diquark_bd_1,                   5103) \
        X(AntiDiquark_bd_1,              -5103) \
        X(Diquark_bu_0,                   5201) \
        X(AntiDiquark_bu_0,              -5201) \
        X(Diquark_bu_1,                   5203) \
        X(AntiDiquark_bu_1,              -5203) \
        X(Diquark_bs_0,                   5301) \
        X(AntiDiquark_bs_0,              -5301) \
        X(Diquark_bs_1,                   5303) \
        X(AntiDiquark_bs_1,              -5303) \
        X(Diquark_bc_0,                   5401) \
        X(AntiDiquark_bc_0,              -5401) \
        X(Diquark_bc_1,                   5403) \
        X(AntiDiquark_bc_1,              -5403) \
        X(Diquark_bb_1,                   5503) \
        X(AntiDiquark_bb_1,              -5503) \
        \
        /* Light Mesons (I=1) */ \
        X(PionZero,                      111) \
        X(PionPlus,                      211) \
        X(AntiPionPlus,                 -211) \
        X(a0_980_Zero,                9000111) \
        X(a0_980_Plus,               9000211) \
        X(Anti_a0_980_Plus,          -9000211) \
        X(Pi_1300_Zero,                100111) \
        X(Pi_1300_Plus,                100211) \
        X(AntiPi_1300_Plus,           -100211) \
        X(a0_1450_Zero,                 10111) \
        X(a0_1450_Plus,                 10211) \
        X(Anti_a0_1450_Plus,           -10211) \
        X(Pi_1800_Zero,               9010111) \
        X(Pi_1800_Plus,               9010211) \
        X(AntiPi_1800_Plus,          -9010211) \
        X(Rho_770_Zero,                  113) \
        X(Rho_770_Plus,                  213) \
        X(AntiRho_770_Plus,             -213) \
        X(b1_1235_Zero,                10113) \
        X(b1_1235_Plus,                10213) \
        X(Anti_b1_1235_Plus,          -10213) \
        X(a1_1260_Zero,                20113) \
        X(a1_1260_Plus,                20213) \
        X(Anti_a1_1260_Plus,          -20213) \
        X(Pi1_1400_Zero,              9000113) \
        X(Pi1_1400_Plus,              9000213) \
        X(Anti_Pi1_1400_Plus,        -9000213) \
        X(Rho_1450_Zero,               100113) \
        X(Rho_1450_Plus,               100213) \
        X(AntiRho_1450_Plus,          -100213) \
        X(Pi1_1600_Zero,              9010113) \
        X(Pi1_1600_Plus,              9010213) \
        X(AntiPi1_1600_Plus,         -9010213) \
        X(a1_1640_Zero,               9020113) \
        X(a1_1640_Plus,               9020213) \
        X(Anti_a1_1640_Plus,         -9020213) \
        X(Rho_1700_Zero,                30113) \
        X(Rho_1700_Plus,                30213) \
        X(AntiRho_1700_Plus,           -30213) \
        X(Rho_1900_Zero,             9030113) \
        X(Rho_1900_Plus,             9030213) \
        X(AntiRho_1900_Plus,        -9030213) \
        X(Rho_2150_Zero,             9040113) \
        X(Rho_2150_Plus,             9040213) \
        X(AntiRho_2150_Plus,        -9040213) \
        X(a2_1320_Zero,                  115) \
        X(a2_1320_Plus,                  215) \
        X(Anti_a2_1320_Plus,            -215) \
        X(Pi2_1670_Zero,               10115) \
        X(Pi2_1670_Plus,               10215) \
        X(AntiPi2_1670_Plus,          -10215) \
        X(a2_1700_Zero,             9000115) \
        X(a2_1700_Plus,             9000215) \
        X(Anti_a2_1700_Plus,        -9000215) \
        X(Pi2_2100_Zero,            9010115) \
        X(Pi2_2100_Plus,            9010215) \
        X(AntiPi2_2100_Plus,       -9010215) \
        X(Rho3_1690_Zero,                117) \
        X(Rho3_1690_Plus,                217) \
        X(AntiRho3_1690_Plus,           -217) \
        X(Rho3_1990_Zero,            9000117) \
        X(Rho3_1990_Plus,            9000217) \
        X(AntiRho3_1990_Plus,       -9000217) \
        X(Rho3_2250_Zero,            9010117) \
        X(Rho3_2250_Plus,            9010217) \
        X(AntiRho3_2250_Plus,       -9010217) \
        X(a4_2040_Zero,                 119) \
        X(a4_2040_Plus,                 219) \
        X(Anti_a4_2040_Plus,           -219) \
        \
        /* Light Mesons (I=0) */ \
        X(Eta,                            221) \
        X(EtaPrime_958,                   331) \
        X(f0_600,                     9000221) \
        X(f0_980,                     9010221) \
        X(Eta_1295,                     100221) \
        X(f0_1370,                       10221) \
        X(Eta_1405,                   9020221) \
        X(Eta_1475,                     100331) \
        X(f0_1500,                    9030221) \
        X(f0_1710,                       10331) \
        X(Eta_1760,                   9040221) \
        X(f0_2020,                    9050221) \
        X(f0_2100,                    9060221) \
        X(f0_2200,                    9070221) \
        X(Eta_2225,                   9080221) \
        X(Omega_782,                      223) \
        X(Phi_1020,                       333) \
        X(h1_1170,                       10223) \
        X(f1_1285,                       20223) \
        X(h1_1380,                       10333) \
        X(f1_1420,                       20333) \
        X(Omega_1420,                   100223) \
        X(f1_1510,                    9000223) \
        X(h1_1595,                    9010223) \
        X(Omega_1650,                    30223) \
        X(Phi_1680,                     100333) \
        X(f2_1270,                        225) \
        X(f2_1430,                    9000225) \
        X(f2_1525,                        335) \
        X(f2_1565,                    9010225) \
        X(f2_1640,                    9020225) \
        X(Eta2_1645,                     10225) \
        X(f2_1810,                    9030225) \
        X(Eta2_1870,                     10335) \
        X(f2_1910,                    9040225) \
        X(f2_1950,                    9050225) \
        X(f2_2010,                    9060225) \
        X(f2_2150,                    9070225) \
        X(f2_2300,                    9080225) \
        X(f2_2340,                    9090225) \
        X(Omega3_1670,                    227) \
        X(Phi3_1850,                      337) \
        X(f4_2050,                        229) \
        X(f4_2220,                    9000229) \
        X(f4_2300,                    9010229) \
        \
        /* Light Baryons */ \
        X(Proton,                       2212) \
        X(AntiProton,                  -2212) \
        X(Neutron,                      2112) \
        X(AntiNeutron,                 -2112) \
        X(DeltaPlusPlus,                2224) \
        X(AntiDeltaPlusPlus,           -2224) \
        X(DeltaPlus,                    2214) \
        X(AntiDeltaPlus,               -2214) \
        X(DeltaZero,                    2114) \
        X(AntiDeltaZero,               -2114) \
        X(DeltaMinus,                   1114) \
        X(AntiDeltaMinus,              -1114) \
        \
        /* Strange Baryons */ \
        X(Lambda,                       3122) \
        X(AntiLambda,                  -3122) \
        X(SigmaPlus,                    3222) \
        X(AntiSigmaPlus,               -3222) \
        X(SigmaZero,                    3212) \
        X(AntiSigmaZero,               -3212) \
        X(SigmaMinus,                   3112) \
        X(AntiSigmaMinus,              -3112) \
        X(SigmaStarPlus,                3224) \
        X(AntiSigmaStarPlus,           -3224) \
        X(SigmaStarZero,                3214) \
        X(AntiSigmaStarZero,           -3214) \
        X(SigmaStarMinus,               3114) \
        X(AntiSigmaStarMinus,          -3114) \
        X(XiZero,                       3322) \
        X(AntiXiZero,                  -3322) \
        X(XiMinus,                      3312) \
        X(AntiXiMinus,                 -3312) \
        X(XiStarZero,                   3324) \
        X(AntiXiStarZero,              -3324) \
        X(XiStarMinus,                  3314) \
        X(AntiXiStarMinus,             -3314) \
        X(OmegaMinus,                   3334) \
        X(AntiOmegaMinus,              -3334) \
        \
        /* Charmed Baryons */ \
        X(Lambda_c_Plus,                4122) \
        X(AntiLambda_c_Plus,           -4122) \
        X(Sigma_c_PlusPlus,             4222) \
        X(AntiSigma_c_PlusPlus,        -4222) \
        X(Sigma_c_Plus,                 4212) \
        X(AntiSigma_c_Plus,            -4212) \
        X(Sigma_c_Zero,                 4112) \
        X(AntiSigma_c_Zero,            -4112) \
        X(Sigma_c_Star_PlusPlus,        4224) \
        X(AntiSigma_c_Star_PlusPlus,   -4224) \
        X(Sigma_c_Star_Plus,            4214) \
        X(AntiSigma_c_Star_Plus,       -4214) \
        X(Sigma_c_Star_Zero,            4114) \
        X(AntiSigma_c_Star_Zero,       -4114) \
        X(Xi_c_Plus,                    4232) \
        X(AntiXi_c_Plus,               -4232) \
        X(Xi_c_Zero,                    4132) \
        X(AntiXi_c_Zero,               -4132) \
        X(Xi_c_Prime_Plus,              4322) \
        X(AntiXi_c_Prime_Plus,         -4322) \
        X(Xi_c_Prime_Zero,              4312) \
        X(AntiXi_c_Prime_Zero,         -4312) \
        X(Xi_c_Star_Plus,               4324) \
        X(AntiXi_c_Star_Plus,          -4324) \
        X(Xi_c_Star_Zero,               4314) \
        X(AntiXi_c_Star_Zero,          -4314) \
        X(Omega_c_Zero,                 4332) \
        X(AntiOmega_c_Zero,            -4332) \
        X(Omega_c_Star_Zero,            4334) \
        X(AntiOmega_c_Star_Zero,       -4334) \
        X(Xi_cc_PlusPlus,               4412) \
        X(AntiXi_cc_PlusPlus,          -4412) \
        X(Xi_cc_Plus,                   4422) \
        X(AntiXi_cc_Plus,              -4422) \
        X(Xi_cc_Star_Plus,              4414) \
        X(AntiXi_cc_Star_Plus,         -4414) \
        X(Xi_cc_Star_PlusPlus,          4424) \
        X(AntiXi_cc_Star_PlusPlus,     -4424) \
        X(Omega_cc_Plus,                4432) \
        X(AntiOmega_cc_Plus,           -4432) \
        X(Omega_cc_Star_Plus,           4434) \
        X(AntiOmega_cc_Star_Plus,      -4434) \
        X(Omega_ccc_PlusPlus,           4444) \
        X(AntiOmega_ccc_PlusPlus,      -4444) \
        \
        /* Bottom Baryons */ \
        X(Lambda_b_Zero,                5122) \
        X(AntiLambda_b_Zero,           -5122) \
        X(Sigma_b_Zero,                 5212) \
        X(AntiSigma_b_Zero,            -5212) \
        X(Sigma_b_Plus,                 5222) \
        X(AntiSigma_b_Plus,            -5222) \
        X(Sigma_b_Minus,                5112) \
        X(AntiSigma_b_Minus,           -5112) \
        X(Sigma_b_Star_Zero,            5214) \
        X(AntiSigma_b_Star_Zero,       -5214) \
        X(Sigma_b_Star_Plus,            5224) \
        X(AntiSigma_b_Star_Plus,       -5224) \
        X(Xi_b_Zero,                    5132) \
        X(AntiXi_b_Zero,               -5132) \
        X(Xi_b_Minus,                   5232) \
        X(AntiXi_b_Minus,              -5232) \
        X(Xi_b_Prime_Zero,              5312) \
        X(AntiXi_b_Prime_Zero,         -5312) \
        X(Xi_b_Prime_Minus,             5322) \
        X(AntiXi_b_Prime_Minus,        -5322) \
        X(Xi_b_Star_Zero,               5314) \
        X(AntiXi_b_Star_Zero,          -5314) \
        X(Xi_b_Star_Minus,              5324) \
        X(AntiXi_b_Star_Minus,         -5324) \
        X(Omega_b_Minus,                5332) \
        X(AntiOmega_b_Minus,           -5332) \
        X(Omega_b_Star_Minus,           5334) \
        X(AntiOmega_b_Star_Minus,      -5334) \
        X(Xi_bc_Zero,                   5142) \
        X(AntiXi_bc_Zero,              -5142) \
        X(Xi_bc_Plus,                   5242) \
        X(AntiXi_bc_Plus,              -5242) \
        X(Xi_bc_Prime_Zero,             5412) \
        X(AntiXi_bc_Prime_Zero,        -5412) \
        X(Xi_bc_Prime_Plus,             5422) \
        X(AntiXi_bc_Prime_Plus,        -5422) \
        X(Xi_bc_Star_Zero,              5414) \
        X(AntiXi_bc_Star_Zero,         -5414) \
        X(Xi_bc_Star_Plus,              5424) \
        X(AntiXi_bc_Star_Plus,         -5424) \
        X(Omega_bc_Zero,                5342) \
        X(AntiOmega_bc_Zero,           -5342) \
        X(Omega_bc_Prime_Zero,          5432) \
        X(AntiOmega_bc_Prime_Zero,     -5432) \
        X(Omega_bc_Star_Zero,           5434) \
        X(AntiOmega_bc_Star_Zero,      -5434) \
        X(Omega_bcc_Plus,               5442) \
        X(AntiOmega_bcc_Plus,          -5442) \
        X(Omega_bcc_Star_Plus,          5444) \
        X(AntiOmega_bcc_Star_Plus,     -5444) \
        X(Xi_bb_Zero,                   5512) \
        X(AntiXi_bb_Zero,              -5512) \
        X(Xi_bb_Minus,                  5522) \
        X(AntiXi_bb_Minus,             -5522) \
        X(Xi_bb_Star_Zero,              5514) \
        X(AntiXi_bb_Star_Zero,         -5514) \
        X(Xi_bb_Star_Minus,             5524) \
        X(AntiXi_bb_Star_Minus,        -5524) \
        X(Omega_bb_Minus,               5532) \
        X(AntiOmega_bb_Minus,          -5532) \
        X(Omega_bb_Star_Minus,          5534) \
        X(AntiOmega_bb_Star_Minus,     -5534) \
        X(Omega_bbc_Zero,               5542) \
        X(AntiOmega_bbc_Zero,          -5542) \
        X(Omega_bbc_Star_Zero,          5544) \
        X(AntiOmega_bbc_Star_Zero,     -5544) \
        X(Omega_bbb_Minus,              5554) \
        X(AntiOmega_bbb_Minus,         -5554) \
        \
        /* Supersymmetric */ \
        /* Squarks (Left-handed) */ \
        X(Squark_d_L,                  1000001) \
        X(AntiSquark_d_L,             -1000001) \
        X(Squark_u_L,                  1000002) \
        X(AntiSquark_u_L,             -1000002) \
        X(Squark_s_L,                  1000003) \
        X(AntiSquark_s_L,             -1000003) \
        X(Squark_c_L,                  1000004) \
        X(AntiSquark_c_L,             -1000004) \
        X(Squark_b_1,                  1000005) \
        X(AntiSquark_b_1,             -1000005) \
        X(Squark_t_1,                  1000006) \
        X(AntiSquark_t_1,             -1000006) \
        /* Sleptons (Left-handed) */ \
        X(Selectron_L,                 1000011) \
        X(AntiSelectron_L,            -1000011) \
        X(Sneutrino_e_L,               1000012) \
        X(AntiSneutrino_e_L,          -1000012) \
        X(Smuon_L,                     1000013) \
        X(AntiSmuon_L,                -1000013) \
        X(Sneutrino_mu_L,              1000014) \
        X(AntiSneutrino_mu_L,         -1000014) \
        X(Stau_1,                      1000015) \
        X(AntiStau_1,                 -1000015) \
        X(Sneutrino_tau_L,             1000016) \
        X(AntiSneutrino_tau_L,        -1000016) \
        /* Squarks (Right-handed) */ \
        X(Squark_d_R,                  2000001) \
        X(AntiSquark_d_R,             -2000001) \
        X(Squark_u_R,                  2000002) \
        X(AntiSquark_u_R,             -2000002) \
        X(Squark_s_R,                  2000003) \
        X(AntiSquark_s_R,             -2000003) \
        X(Squark_c_R,                  2000004) \
        X(AntiSquark_c_R,             -2000004) \
        X(Squark_b_2,                  2000005) \
        X(AntiSquark_b_2,             -2000005) \
        X(Squark_t_2,                  2000006) \
        X(AntiSquark_t_2,             -2000006) \
        /* Sleptons (Right-handed) */ \
        X(Selectron_R,                 2000011) \
        X(AntiSelectron_R,            -2000011) \
        X(Smuon_R,                     2000013) \
        X(AntiSmuon_R,                -2000013) \
        X(Stau_2,                      2000015) \
        X(AntiStau_2,                 -2000015) \
        /* Gauginos, Higgsinos, and Gravitino */ \
        X(Gluino,                      1000021) \
        X(Neutralino1,                 1000022) \
        X(Neutralino2,                 1000023) \
        X(Chargino1Plus,               1000024) \
        X(AntiChargino1Plus,          -1000024) \
        X(Neutralino3,                 1000025) \
        X(Neutralino4,                 1000035) \
        X(Chargino2Plus,               1000037) \
        X(AntiChargino2Plus,          -1000037) \
        X(Gravitino,                   1000039) \
        \
        /* Technicolor */ \
        X(TechniPiZero,                3000111) \
        X(TechniPiPlus,                3000211) \
        X(AntiTechniPiPlus,           -3000211) \
        X(TechniPiPrimeZero,           3000221) \
        X(TechniEtaZero,               3100221) \
        X(TechniRhoZero,               3000113) \
        X(TechniRhoPlus,               3000213) \
        X(AntiTechniRhoPlus,          -3000213) \
        X(TechniOmegaZero,             3000223) \
        X(TechniV8,                    3100021) \
        X(TechniPi22_1,                3060111) \
        X(TechniPi22_8,                3160111) \
        X(TechniRho11,                 3130113) \
        X(TechniRho12,                 3140113) \
        X(TechniRho21,                 3150113) \
        X(TechniRho22,                 3160113) \
        \
        /* R-Hadrons */ \
        /* Gluino R-Hadrons */ \
        X(RHadron_g_g,                 1000993) \
        X(RHadron_g_dd_bar,            1009113) \
        X(RHadron_g_ud_bar_Plus,       1009213) \
        X(AntiRHadron_g_ud_bar_Plus,  -1009213) \
        X(RHadron_g_uu_bar,            1009223) \
        X(RHadron_g_ds_bar,            1009313) \
        X(AntiRHadron_g_ds_bar,       -1009313) \
        X(RHadron_g_us_bar_Plus,       1009323) \
        X(AntiRHadron_g_us_bar_Plus,  -1009323) \
        X(RHadron_g_ss_bar,            1009333) \
        X(RHadron_g_ddd,               1091114) \
        X(AntiRHadron_g_ddd,          -1091114) \
        X(RHadron_g_udd_Plus,          1092114) \
        X(AntiRHadron_g_udd_Plus,     -1092114) \
        X(RHadron_g_uud_PlusPlus,      1092214) \
        X(AntiRHadron_g_uud_PlusPlus, -1092214) \
        X(RHadron_g_uuu_PlusPlus,      1092224) \
        X(AntiRHadron_g_uuu_PlusPlus, -1092224) \
        X(RHadron_g_sdd,               1093114) \
        X(AntiRHadron_g_sdd,          -1093114) \
        X(RHadron_g_sud_Plus,          1093214) \
        X(AntiRHadron_g_sud_Plus,     -1093214) \
        X(RHadron_g_suu_PlusPlus,      1093314) \
        X(AntiRHadron_g_suu_PlusPlus, -1093314) \
        X(RHadron_g_ssd_Plus,          1093324) \
        X(AntiRHadron_g_ssd_Plus,     -1093324) \
        X(RHadron_g_sss,               1093334) \
        X(AntiRHadron_g_sss,          -1093334) \
        /* Stop R-Hadrons */ \
        X(RHadron_t1_t1_bar,           1000612) \
        X(RHadron_t1_d_bar,            1000622) \
        X(AntiRHadron_t1_d_bar,       -1000622) \
        X(RHadron_t1_s_bar,            1000632) \
        X(AntiRHadron_t1_s_bar,       -1000632) \
        X(RHadron_t1_b_bar,            1000642) \
        X(AntiRHadron_t1_b_bar,       -1000642) \
        X(RHadron_t1_u_bar_Plus,       1000652) \
        X(AntiRHadron_t1_u_bar_Plus,  -1000652) \
        X(RHadron_t1_dd1,              1006113) \
        X(AntiRHadron_t1_dd1,         -1006113) \
        X(RHadron_t1_ud0_Plus,         1006211) \
        X(AntiRHadron_t1_ud0_Plus,    -1006211) \
        X(RHadron_t1_ud1_Plus,         1006213) \
        X(AntiRHadron_t1_ud1_Plus,    -1006213) \
        X(RHadron_t1_uu1_PlusPlus,     1006223) \
        X(AntiRHadron_t1_uu1_PlusPlus,-1006223) \
        X(RHadron_t1_sd0,              1006311) \
        X(AntiRHadron_t1_sd0,         -1006311) \
        X(RHadron_t1_sd1,              1006313) \
        X(AntiRHadron_t1_sd1,         -1006313) \
        X(RHadron_t1_su0_Plus,         1006321) \
        X(AntiRHadron_t1_su0_Plus,    -1006321) \
        X(RHadron_t1_su1_Plus,         1006323) \
        X(AntiRHadron_t1_su1_Plus,    -1006323) \
        X(RHadron_t1_ss1,              1006333) \
        X(AntiRHadron_t1_ss1,         -1006333) \
        \
        /* Strange Mesons */ \
        X(KaonLong,                         130) \
        X(KaonShort,                        310) \
        X(KaonZero,                         311) \
        X(AntiKaonZero,                    -311) \
        X(KaonPlus,                         321) \
        X(AntiKaonPlus,                    -321) \
        X(K0Star_800_Zero,            9000311) \
        X(AntiK0Star_800_Zero,       -9000311) \
        X(K0Star_800_Plus,            9000321) \
        X(AntiK0Star_800_Plus,       -9000321) \
        X(K0Star_1430_Zero,             10311) \
        X(AntiK0Star_1430_Zero,        -10311) \
        X(K0Star_1430_Plus,             10321) \
        X(AntiK0Star_1430_Plus,        -10321) \
        X(K_1460_Zero,                  100311) \
        X(AntiK_1460_Zero,             -100311) \
        X(K_1460_Plus,                  100321) \
        X(AntiK_1460_Plus,             -100321) \
        X(K_1830_Zero,                9010311) \
        X(AntiK_1830_Zero,           -9010311) \
        X(K_1830_Plus,                9010321) \
        X(AntiK_1830_Plus,           -9010321) \
        X(K0Star_1950_Zero,           9020311) \
        X(AntiK0Star_1950_Zero,      -9020311) \
        X(K0Star_1950_Plus,           9020321) \
        X(AntiK0Star_1950_Plus,      -9020321) \
        X(KStar_892_Zero,                 313) \
        X(AntiKStar_892_Zero,            -313) \
        X(KStar_892_Plus,                 323) \
        X(AntiKStar_892_Plus,            -323) \
        X(K1_1270_Zero,                 10313) \
        X(AntiK1_1270_Zero,            -10313) \
        X(K1_1270_Plus,                 10323) \
        X(AntiK1_1270_Plus,            -10323) \
        X(K1_1400_Zero,                 20313) \
        X(AntiK1_1400_Zero,            -20313) \
        X(K1_1400_Plus,                 20323) \
        X(AntiK1_1400_Plus,            -20323) \
        X(KStar_1410_Zero,             100313) \
        X(AntiKStar_1410_Zero,        -100313) \
        X(KStar_1410_Plus,             100323) \
        X(AntiKStar_1410_Plus,        -100323) \
        X(K1_1650_Zero,               9000313) \
        X(AntiK1_1650_Zero,          -9000313) \
        X(K1_1650_Plus,               9000323) \
        X(AntiK1_1650_Plus,          -9000323) \
        X(KStar_1680_Zero,              30313) \
        X(AntiKStar_1680_Zero,         -30313) \
        X(KStar_1680_Plus,              30323) \
        X(AntiKStar_1680_Plus,         -30323) \
        X(K2Star_1430_Zero,               315) \
        X(AntiK2Star_1430_Zero,          -315) \
        X(K2Star_1430_Plus,               325) \
        X(AntiK2Star_1430_Plus,          -325) \
        X(K2_1580_Zero,               9000315) \
        X(AntiK2_1580_Zero,          -9000315) \
        X(K2_1580_Plus,               9000325) \
        X(AntiK2_1580_Plus,          -9000325) \
        X(K2_1770_Zero,                 10315) \
        X(AntiK2_1770_Zero,            -10315) \
        X(K2_1770_Plus,                 10325) \
        X(AntiK2_1770_Plus,            -10325) \
        X(K2_1820_Zero,                 20315) \
        X(AntiK2_1820_Zero,            -20315) \
        X(K2_1820_Plus,                 20325) \
        X(AntiK2_1820_Plus,            -20325) \
        X(K2_1980_Zero,               9010315) \
        X(AntiK2_1980_Zero,          -9010315) \
        X(K2_1980_Plus,               9010325) \
        X(AntiK2_1980_Plus,          -9010325) \
        X(K2_2250_Zero,               9020315) \
        X(AntiK2_2250_Zero,          -9020315) \
        X(K2_2250_Plus,               9020325) \
        X(AntiK2_2250_Plus,          -9020325) \
        X(K3Star_1780_Zero,               317) \
        X(AntiK3Star_1780_Zero,          -317) \
        X(K3Star_1780_Plus,               327) \
        X(AntiK3Star_1780_Plus,          -327) \
        X(K3_2320_Zero,               9010317) \
        X(AntiK3_2320_Zero,          -9010317) \
        X(K3_2320_Plus,               9010327) \
        X(AntiK3_2320_Plus,          -9010327) \
        X(K4Star_2045_Zero,               319) \
        X(AntiK4Star_2045_Zero,          -319) \
        X(K4Star_2045_Plus,               329) \
        X(AntiK4Star_2045_Plus,          -329) \
        X(K4_2500_Zero,               9000319) \
        X(AntiK4_2500_Zero,          -9000319) \
        X(K4_2500_Plus,               9000329) \
        X(AntiK4_2500_Plus,          -9000329) \
        \
        /* Charmed Mesons */ \
        X(DPlus,                         411) \
        X(AntiDPlus,                    -411) \
        X(DZero,                         421) \
        X(AntiDZero,                    -421) \
        X(D0Star_2400_Plus,            10411) \
        X(AntiD0Star_2400_Plus,       -10411) \
        X(D0Star_2400_Zero,            10421) \
        X(AntiD0Star_2400_Zero,       -10421) \
        X(DStar_2010_Plus,               413) \
        X(AntiDStar_2010_Plus,          -413) \
        X(DStar_2007_Zero,               423) \
        X(AntiDStar_2007_Zero,          -423) \
        X(D1_2420_Plus,                10413) \
        X(AntiD1_2420_Plus,           -10413) \
        X(D1_2420_Zero,                10423) \
        X(AntiD1_2420_Zero,           -10423) \
        X(D1_H_Plus,                   20413) \
        X(AntiD1_H_Plus,              -20413) \
        X(D1_2430_Zero,                20423) \
        X(AntiD1_2430_Zero,           -20423) \
        X(D2Star_2460_Plus,              415) \
        X(AntiD2Star_2460_Plus,         -415) \
        X(D2Star_2460_Zero,              425) \
        X(AntiD2Star_2460_Zero,         -425) \
        X(DsPlus,                        431) \
        X(AntiDsPlus,                   -431) \
        X(Ds0Star_2317_Plus,           10431) \
        X(AntiDs0Star_2317_Plus,      -10431) \
        X(DsStarPlus,                    433) \
        X(AntiDsStarPlus,               -433) \
        X(Ds1_2536_Plus,               10433) \
        X(AntiDs1_2536_Plus,          -10433) \
        X(Ds1_2460_Plus,               20433) \
        X(AntiDs1_2460_Plus,          -20433) \
        X(Ds2_2573_Plus,                 435) \
        X(AntiDs2_2573_Plus,            -435) \
        \
        /* Bottom Mesons */ \
        X(BZero,                         511) \
        X(AntiBZero,                    -511) \
        X(BPlus,                         521) \
        X(AntiBPlus,                    -521) \
        X(B0Star_Zero,                 10511) \
        X(AntiB0Star_Zero,            -10511) \
        X(B0Star_Plus,                 10521) \
        X(AntiB0Star_Plus,            -10521) \
        X(BStar_Zero,                    513) \
        X(AntiBStar_Zero,               -513) \
        X(BStar_Plus,                    523) \
        X(AntiBStar_Plus,               -523) \
        X(B1_L_Zero,                   10513) \
        X(AntiB1_L_Zero,              -10513) \
        X(B1_L_Plus,                   10523) \
        X(AntiB1_L_Plus,              -10523) \
        X(B1_H_Zero,                   20513) \
        X(AntiB1_H_Zero,              -20513) \
        X(B1_H_Plus,                   20523) \
        X(AntiB1_H_Plus,              -20523) \
        X(B2Star_Zero,                   515) \
        X(AntiB2Star_Zero,              -515) \
        X(B2Star_Plus,                   525) \
        X(AntiB2Star_Plus,              -525) \
        X(Bs_Zero,                       531) \
        X(AntiBs_Zero,                  -531) \
        X(Bs0Star_Zero,                10531) \
        X(AntiBs0Star_Zero,           -10531) \
        X(BsStar_Zero,                   533) \
        X(AntiBsStar_Zero,              -533) \
        X(Bs1_L_Zero,                  10533) \
        X(AntiBs1_L_Zero,             -10533) \
        X(Bs1_H_Zero,                  20533) \
        X(AntiBs1_H_Zero,             -20533) \
        X(Bs2Star_Zero,                  535) \
        X(AntiBs2Star_Zero,             -535) \
        X(Bc_Plus,                       541) \
        X(AntiBc_Plus,                  -541) \
        X(Bc0Star_Plus,                10541) \
        X(AntiBc0Star_Plus,           -10541) \
        X(BcStar_Plus,                   543) \
        X(AntiBcStar_Plus,              -543) \
        X(Bc1_L_Plus,                  10543) \
        X(AntiBc1_L_Plus,             -10543) \
        X(Bc1_H_Plus,                  20543) \
        X(AntiBc1_H_Plus,             -20543) \
        X(Bc2Star_Plus,                  545) \
        X(AntiBc2Star_Plus,             -545) \
        \
        /* Charmonium Mesons */ \
        X(Eta_c_1S,                       441) \
        X(Chi_c0_1P,                    10441) \
        X(Eta_c_2S,                     100441) \
        X(J_psi_1S,                       443) \
        X(h_c_1P,                       10443) \
        X(Chi_c1_1P,                    20443) \
        X(psi_2S,                       100443) \
        X(psi_3770,                     30443) \
        X(psi_4040,                   9000443) \
        X(psi_4160,                   9010443) \
        X(psi_4415,                   9020443) \
        X(Chi_c2_1P,                      445) \
        X(Chi_c2_2P,                    100445) \
        \
        /* Bottomonium Mesons */ \
        X(Eta_b_1S,                        551) \
        X(Chi_b0_1P,                     10551) \
        X(Eta_b_2S,                      100551) \
        X(Chi_b0_2P,                     110551) \
        X(Eta_b_3S,                      200551) \
        X(Chi_b0_3P,                     210551) \
        X(Upsilon_1S,                      553) \
        X(h_b_1P,                        10553) \
        X(Chi_b1_1P,                     20553) \
        X(Upsilon1_1D,                   30553) \
        X(Upsilon_2S,                    100553) \
        X(h_b_2P,                        110553) \
        X(Chi_b1_2P,                     120553) \
        X(Upsilon1_2D,                   130553) \
        X(Upsilon_3S,                    200553) \
        X(h_b_3P,                        210553) \
        X(Chi_b1_3P,                     220553) \
        X(Upsilon_4S,                    300553) \
        X(Upsilon_10860,               9000553) \
        X(Upsilon_11020,               9010553) \
        X(Chi_b2_1P,                       555) \
        X(Eta_b2_1D,                     10555) \
        X(Upsilon2_1D,                   20555) \
        X(Chi_b2_2P,                     100555) \
        X(Eta_b2_2D,                     110555) \
        X(Upsilon2_2D,                   120555) \
        X(Chi_b2_3P,                     200555) \
        X(Upsilon3_1D,                     557) \
        X(Upsilon3_2D,                   100557) \
        \
        /* Pentaquarks */ \
        X(ThetaPlus,                  9221132) \
        X(AntiThetaPlus,             -9221132) \
        X(PhiMinusMinus,              9331122) \
        X(AntiPhiMinusMinus,         -9331122) \
        \
        /* Excited */ \
        X(ExcitedDownQuark,     4000001) \
        X(ExcitedUpQuark,       4000002) \
        X(ExcitedElectron,      4000011) \
        X(ExcitedElectronNeutrino,4000012) \
        X(ExcitedAntiDownQuark,-4000001) \
        X(ExcitedAntiUpQuark,  -4000002) \
        X(ExcitedPositron,     -4000011) \
        X(ExcitedAntiElectronNeutrino,-4000012) \
        \
        /* Special */ \
        X(Graviton,                   39) \
        X(RHadron,                    41) \
        X(Leptoquark,                 42) \
        X(Reggeon,                   110) \
        X(Pomeron,                   990) \
        X(Odderon,                  9990) \
        \
        /* Nuclei */ \
        X(Deuteron,            1000010020) \
        X(AntiDeuteron,       -1000010020) \
        X(Triton,              1000010030) \
        X(AntiTriton,         -1000010030) \
        X(Helium3Nucleus,      1000020030) \
        X(AntiHelium3Nucleus, -1000020030) \
        X(HeliumNucleus,       1000020040) \
        X(AntiHeliumNucleus,  -1000020040) \
        X(Lithium6Nucleus,     1000030060) \
        X(AntiLithium6Nucleus,-1000030060) \
        X(Lithium7Nucleus,     1000030070) \
        X(AntiLithium7Nucleus,-1000030070) \
        X(Beryllium7Nucleus,   1000040070) \
        X(AntiBeryllium7Nucleus,-1000040070) \
        X(Beryllium9Nucleus,   1000040090) \
        X(AntiBeryllium9Nucleus,-1000040090) \
        X(Boron10Nucleus,      1000050100) \
        X(AntiBoron10Nucleus, -1000050100) \
        X(Boron11Nucleus,      1000050110) \
        X(AntiBoron11Nucleus, -1000050110) \
        X(Carbon11Nucleus,     1000060110) \
        X(AntiCarbon11Nucleus,-1000060110) \
        X(CarbonNucleus,       1000060120) \
        X(AntiCarbonNucleus,  -1000060120) \
        X(Nitrogen14Nucleus,   1000070140) \
        X(AntiNitrogen14Nucleus,-1000070140) \
        X(Oxygen15Nucleus,     1000080150) \
        X(AntiOxygen15Nucleus,-1000080150) \
        X(OxygenNucleus,       1000080160) \
        X(AntiOxygenNucleus,  -1000080160)

    // ——— 2) Generate the enum from that list ———
    enum class ParticleType : std::int32_t {
    #define X(name, code) name = code,
        PARTICLE_LIST
    #undef X
        Unsupported =  99
    };

    // ——— 3) Generate your “safe” from-PDGID lookup ———
    inline ParticleType getParticleTypeFromPDGID(std::int32_t pdg) noexcept {
        switch (pdg) {
    #define X(name, code) case code: return ParticleType::name;
            PARTICLE_LIST
    #undef X
            default: return ParticleType::Unsupported;
        }
    }

    // ——— 4) Generate your name lookup, too ———
    constexpr std::string_view getParticleTypeName(ParticleType t) {
        switch (t) {
    #define X(name, code) case ParticleType::name: return #name;
            PARTICLE_LIST
    #undef X
            default: return "Unsupported";
        }
    }

    // Returns the numeric code corresponding to the ParticleType.
    inline std::int32_t getPDGIDFromParticleType(ParticleType type) noexcept {
        return static_cast<std::int32_t>(type);
    }

    #undef PARTICLE_LIST

} // namespace ParticleZoo