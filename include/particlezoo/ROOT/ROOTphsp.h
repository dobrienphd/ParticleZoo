#pragma once

#ifdef USE_ROOT

#include "particlezoo/PhaseSpaceFileReader.h"
#include "particlezoo/PhaseSpaceFileWriter.h"

#include <TFile.h>
#include <TTree.h>

namespace ParticleZoo::ROOT {

    /**
     * @struct BranchInfo
     * @brief Configuration for ROOT TTree branch mapping.
     * 
     * Contains the branch name and unit conversion factor for mapping
     * physical quantities to ROOT TTree branches.
     */
    struct BranchInfo {
        std::string branchName;  ///< Name of the ROOT TTree branch
        double       unitFactor; ///< Unit conversion factor to internal units
    };

    /**
     * @brief ROOT branch mapping configuration for TOPAS generated ROOT files.
     * 
     * Predefined branch names and unit factors for TOPAS ROOT output format.
     * 
     * Branch mappings:
     * - treeName: "ROOTOutput" (TTree name)
     * - isNewHistory: "Flag_to_tell_if_this_is_the_First_Scored_Particle_from_this_History__1_means_true_"
     * - energy: "Energy__MeV_" (in MeV units)
     * - weight: "Weight" (unitless)
     * - positionX: "Position_X__cm_" (in cm units)
     * - positionY: "Position_Y__cm_" (in cm units)
     * - positionZ: "Position_Z__cm_" (in cm units)
     * - directionalCosineX: "Direction_Cosine_X" (unitless)
     * - directionalCosineY: "Direction_Cosine_Y" (unitless)
     * - directionalCosineZIsNegative: "Flag_to_tell_if_Third_Direction_Cosine_is_Negative__1_means_true_"
     * - pdgCode: "Particle_Type__in_PDG_Format_" (unitless)
     * - historyNumber: "Event_ID" (unitless)
     */
    inline const std::map<std::string, BranchInfo> TOPASBranches = {
        {"treeName",     {"ROOTOutput", 1.0}},
        {"isNewHistory", {"Flag_to_tell_if_this_is_the_First_Scored_Particle_from_this_History__1_means_true_", 1.0}},
        {"energy",       {"Energy__MeV_", MeV}},
        {"weight",       {"Weight", 1.f}},
        {"positionX",    {"Position_X__cm_", cm}},
        {"positionY",    {"Position_Y__cm_", cm}},
        {"positionZ",    {"Position_Z__cm_", cm}},
        {"directionalCosineX",    {"Direction_Cosine_X", 1.0}},
        {"directionalCosineY",    {"Direction_Cosine_Y", 1.0}},
        {"directionalCosineZIsNegative",    {"Flag_to_tell_if_Third_Direction_Cosine_is_Negative__1_means_true_", 1.0}},
        {"pdgCode",      {"Particle_Type__in_PDG_Format_", 1.0}},
        {"historyNumber",{"Event_ID", 1.0}}
    };

    /**
     * @brief ROOT branch mapping configuration for OpenGATE generated ROOT files.
     * 
     * Predefined branch names and unit factors for OpenGATE ROOT output format.
     * 
     * Branch mappings:
     * - energy: "KineticEnergy" (in MeV units)
     * - weight: "Weight" (unitless)
     * - positionX: "PrePositionLocal_X" (in mm units)
     * - positionY: "PrePositionLocal_Y" (in mm units)
     * - positionZ: "PrePositionLocal_Z" (in mm units)
     * - directionalCosineX: "PreDirectionLocal_X" (unitless)
     * - directionalCosineY: "PreDirectionLocal_Y" (unitless)
     * - directionalCosineZ: "PreDirectionLocal_Z" (unitless)
     * - pdgCode: "PDGCode" (unitless)
     */
    inline const std::map<std::string, BranchInfo> OPENGateBranches = {
        {"energy",       {"KineticEnergy", MeV}},
        {"weight",       {"Weight", 1.0}},
        {"positionX",    {"PrePositionLocal_X", mm}},
        {"positionY",    {"PrePositionLocal_Y", mm}},
        {"positionZ",    {"PrePositionLocal_Z", mm}},
        {"directionalCosineX",    {"PreDirectionLocal_X", 1.0}},
        {"directionalCosineY",    {"PreDirectionLocal_Y", 1.0}},
        {"directionalCosineZ",    {"PreDirectionLocal_Z", 1.0}},
        {"pdgCode",      {"PDGCode", 1.0}}
    };

    /// @brief Default branch mapping (uses TOPAS format)
    inline const std::map<std::string, BranchInfo> defaultBranchNames = TOPASBranches;

    // CLI command declarations for ROOT format configuration
    extern CLICommand ROOTFormatCommand;                        ///< Format preset selection (TOPAS/OpenGATE)
    extern CLICommand ROOTTreeNameCommand;                      ///< TTree name specification
    extern CLICommand ROOTEnergyCommand;                        ///< Energy branch name
    extern CLICommand ROOTWeightCommand;                        ///< Weight branch name
    extern CLICommand ROOTPositionXCommand;                     ///< X position branch name
    extern CLICommand ROOTPositionYCommand;                     ///< Y position branch name
    extern CLICommand ROOTPositionZCommand;                     ///< Z position branch name
    extern CLICommand ROOTDirectionalCosineXCommand;            ///< X directional cosine branch name
    extern CLICommand ROOTDirectionalCosineYCommand;            ///< Y directional cosine branch name
    extern CLICommand ROOTDirectionalCosineZCommand;            ///< Z directional cosine branch name
    extern CLICommand ROOTDirectionalCosineZIsNegativeCommand;  ///< Z directional cosine sign flag
    extern CLICommand ROOTPDGCodeCommand;                       ///< PDG particle code branch name
    extern CLICommand ROOTHistoryNumberCommand;                 ///< History number branch name

    /**
     * @class Reader
     * @brief ROOT format phase space file reader.
     * 
     * Reads particle data from ROOT TTree structures with configurable branch mappings.
     * Supports multiple format presets (TOPAS, OpenGATE) and custom branch configurations.
     */

    class Reader : public ParticleZoo::PhaseSpaceFileReader
    {
        public:
            /**
             * @brief Construct a ROOT file reader with user options.
             * @param fileName Path to the ROOT file
             * @param options User configuration options including branch mappings
             * @throws std::runtime_error If file cannot be opened or required branches missing
             */
            Reader(const std::string & fileName, const UserOptions & options = UserOptions{});

            /**
             * @brief Destructor - closes ROOT file and cleans up resources.
             */
            ~Reader() override;

            /**
             * @brief Get total number of particles in the ROOT file.
             * @return Number of particles (TTree entries)
             */
            std::uint64_t getNumberOfParticles() const override;
            
            /**
             * @brief Get number of original Monte Carlo histories.
             * @return Number of original histories
             */
            std::uint64_t getNumberOfOriginalHistories() const override;

            /**
             * @brief Get format-specific CLI commands for ROOT configuration.
             * @return Vector of ROOT-specific CLI commands
             */
            static std::vector<CLICommand> getFormatSpecificCLICommands();

        protected:
            /**
             * @brief Read next particle from ROOT TTree.
             * @return Particle object with data from current TTree entry
             * @throws std::runtime_error If end of file reached or read error
             */
            Particle      readParticleManually() override;

        private:
            /**
             * @brief Internal constructor with explicit branch mapping.
             * @param fileName Path to the ROOT file
             * @param branchNames Branch name to property mapping
             * @param options Additional user options
             */
            Reader(const std::string & fileName,
                const std::map<std::string,BranchInfo> & branchNames,
                const UserOptions & options
            );

            // TTree branch data storage
            double energy_{};          ///< Particle kinetic energy from TTree
            double x_{};               ///< X position from TTree
            double y_{};               ///< Y position from TTree
            double z_{};               ///< Z position from TTree
            double px_{};              ///< X directional cosine from TTree
            double py_{};              ///< Y directional cosine from TTree
            double pz_{};              ///< Z directional cosine from TTree
            double weight_{1.0};       ///< Particle weight from TTree
            int pdgCode_{};            ///< PDG particle code from TTree
            bool isNewHistory_{true};  ///< New history flag from TTree
            
            bool pzIsNegative_{false}; ///< Z directional cosine sign flag
            bool pzIsStored_{false};   ///< Whether pz is explicitly stored in TTree

            // Unit conversion factors
            double xUnits_{1.0};       ///< X position unit conversion factor
            double yUnits_{1.0};       ///< Y position unit conversion factor
            double zUnits_{1.0};       ///< Z position unit conversion factor
            double energyUnits_{1.0};  ///< Energy unit conversion factor

            int historyNumber_{-1};    ///< Current history number from TTree

            bool treeHasNewHistoryMarker_{false}; ///< Whether TTree has new history branch
            bool treeHasHistoryNumber_{false};    ///< Whether TTree has history number branch
            std::uint64_t numberOfParticles_{};   ///< Total number of particles in file
            std::uint64_t numberOfOriginalHistories_{}; ///< Number of original Monte Carlo histories
            TFile * file{};          ///< ROOT file pointer
            TTree * tree{};          ///< ROOT TTree pointer
    };

    // Inline method definitions for Reader class

    inline std::uint64_t Reader::getNumberOfParticles() const { return numberOfParticles_; }
    inline std::uint64_t Reader::getNumberOfOriginalHistories() const { return numberOfOriginalHistories_; }


    /**
     * @class Writer
     * @brief ROOT format phase space file writer.
     * 
     * Writes particle data to ROOT TTree structures with configurable branch mappings.
     * Supports multiple format presets (TOPAS, OpenGATE) and custom branch configurations.
     */

    class Writer : public ParticleZoo::PhaseSpaceFileWriter
    {
        static constexpr std::string_view DEFAULT_TREE_NAME = "PhaseSpaceData"; ///< Default TTree name

        public:
            /**
             * @brief Construct a ROOT file writer with user options.
             * @param fileName Path for the output ROOT file
             * @param options User configuration options including branch mappings
             * @throws std::runtime_error If file cannot be created or TTree setup fails
             */
            Writer(const std::string & fileName, const UserOptions & options = UserOptions{});

            /**
             * @brief Destructor - writes TTree and closes ROOT file.
             */
            ~Writer() override;

            /**
             * @brief Get maximum number of particles that can be stored.
             * @return Maximum supported particles
             */
            std::uint64_t getMaximumSupportedParticles() const override;

            /**
             * @brief Get format-specific CLI commands for ROOT configuration.
             * @return Vector of ROOT-specific CLI commands
             */
            static std::vector<CLICommand> getFormatSpecificCLICommands();

        protected:
            /**
             * @brief Write particle data to ROOT TTree.
             * @param particle Particle object to write to current TTree entry
             */
            void          writeParticleManually(Particle & particle) override;
            
            /**
             * @brief Write header data (not used for ROOT format).
             * @param buffer Unused buffer parameter
             */
            void          writeHeaderData(ByteBuffer & buffer) override;

        private:
            /**
             * @brief Internal constructor with explicit branch mapping.
             * @param fileName Path for the output ROOT file
             * @param branchNames Branch name to property mapping
             * @param options Additional user options
             */
            Writer(const std::string & fileName,
                const std::map<std::string,BranchInfo> & branchNames,
                const UserOptions & options
            );

            // TTree branch data storage
            double energy_{};          ///< Particle kinetic energy for TTree
            double x_{};               ///< X position for TTree
            double y_{};               ///< Y position for TTree
            double z_{};               ///< Z position for TTree
            double px_{};              ///< X directional cosine for TTree
            double py_{};              ///< Y directional cosine for TTree
            double pz_{};              ///< Z directional cosine for TTree
            double weight_{1.0};       ///< Particle weight for TTree
            int pdgCode_{};            ///< PDG particle code for TTree
            bool isNewHistory_{true};  ///< New history flag for TTree

            bool pzIsNegative_{false}; ///< Z directional cosine sign flag
            bool pzIsStored_{false};   ///< Whether pz is explicitly stored in TTree

            // Inverse unit conversion factors
            double inverseXUnits_{1.0};      ///< X position inverse unit conversion
            double inverseYUnits_{1.0};      ///< Y position inverse unit conversion
            double inverseZUnits_{1.0};      ///< Z position inverse unit conversion
            double inverseEnergyUnits_{1.0}; ///< Energy inverse unit conversion

            int historyNumber_{0};    ///< Current history number for TTree

            bool storeIncrementalHistories_{false}; ///< Whether to store incremental history numbers
            TFile * file_{};         ///< ROOT file pointer
            TTree * tree_{};         ///< ROOT TTree pointer
            std::map<std::string, BranchInfo> branchNames_; ///< Branch mapping configuration
    };

    // Inline method definitions for Writer class

    inline std::uint64_t Writer::getMaximumSupportedParticles() const {
        return std::numeric_limits<std::uint64_t>::max();
    }

    inline void Writer::writeHeaderData(ByteBuffer & buffer) {}

} // namespace ParticleZoo::ROOT

#endif // USE_ROOT