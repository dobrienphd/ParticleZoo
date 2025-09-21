#pragma once

#ifdef USE_ROOT

#include "particlezoo/PhaseSpaceFileReader.h"
#include "particlezoo/PhaseSpaceFileWriter.h"

#include <TFile.h>
#include <TTree.h>

namespace ParticleZoo::ROOT {

    struct BranchInfo {
        std::string branchName;
        double       unitFactor;
    };

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

    inline const std::map<std::string, BranchInfo> defaultBranchNames = TOPASBranches;

    extern CLICommand ROOTFormatCommand;
    extern CLICommand ROOTTreeNameCommand;
    extern CLICommand ROOTEnergyCommand;
    extern CLICommand ROOTWeightCommand;
    extern CLICommand ROOTPositionXCommand;
    extern CLICommand ROOTPositionYCommand;
    extern CLICommand ROOTPositionZCommand;
    extern CLICommand ROOTDirectionalCosineXCommand;
    extern CLICommand ROOTDirectionalCosineYCommand;
    extern CLICommand ROOTDirectionalCosineZCommand;
    extern CLICommand ROOTDirectionalCosineZIsNegativeCommand;
    extern CLICommand ROOTPDGCodeCommand;
    extern CLICommand ROOTHistoryNumberCommand;

    // Reader class

    class Reader : public ParticleZoo::PhaseSpaceFileReader
    {
        public:
            Reader(const std::string & fileName, const UserOptions & options = UserOptions{});

            ~Reader() override;

            std::uint64_t getNumberOfParticles() const override;
            std::uint64_t getNumberOfOriginalHistories() const override;

            static std::vector<CLICommand> getFormatSpecificCLICommands();

        protected:
            Particle      readParticleManually() override;

        private:
            Reader(const std::string & fileName,
                const std::map<std::string,BranchInfo> & branchNames,
                const UserOptions & options
            );

            double energy_{};
            double x_{};
            double y_{};
            double z_{};
            double px_{};
            double py_{};
            double pz_{};
            double weight_{1.f};
            int pdgCode_{};
            bool isNewHistory_{true};
            
            bool pzIsNegative_{false};
            bool pzIsStored_{false};

            double xUnits_{1.0};
            double yUnits_{1.0};
            double zUnits_{1.0};
            double energyUnits_{1.0};

            int historyNumber_{-1};

            bool readIncrementalHistories_{false};
            std::uint64_t numberOfParticles_{};
            TFile * file{};
            TTree * tree{};
    };

    inline std::uint64_t Reader::getNumberOfParticles() const { return numberOfParticles_; }
    inline std::uint64_t Reader::getNumberOfOriginalHistories() const { return historyNumber_+1; } // currently no implemented way to know how many histories are in the file until they are all read


    // Writer class

    class Writer : public ParticleZoo::PhaseSpaceFileWriter
    {
        static constexpr std::string_view DEFAULT_TREE_NAME = "PhaseSpaceData";

        public:
            Writer(const std::string & fileName, const UserOptions & options = UserOptions{});

            ~Writer() override;

            std::uint64_t getMaximumSupportedParticles() const override;

            static std::vector<CLICommand> getFormatSpecificCLICommands();

        protected:
            void          writeParticleManually(Particle & particle) override;
            void          writeHeaderData(ByteBuffer & buffer) override;

        private:
            Writer(const std::string & fileName,
                const std::map<std::string,BranchInfo> & branchNames,
                const UserOptions & options
            );

            double energy_{};
            double x_{};
            double y_{};
            double z_{};
            double px_{};
            double py_{};
            double pz_{};
            double weight_{1.f};
            int pdgCode_{};
            bool isNewHistory_{true};

            bool pzIsNegative_{false};
            bool pzIsStored_{false};

            double inverseXUnits_{1.f};
            double inverseYUnits_{1.f};
            double inverseZUnits_{1.f};
            double inverseEnergyUnits_{1.f};

            int historyNumber_{0};

            bool storeIncrementalHistories_{false};
            TFile * file_{};
            TTree * tree_{};
            std::map<std::string, BranchInfo> branchNames_;
    };

    inline std::uint64_t Writer::getMaximumSupportedParticles() const {
        return std::numeric_limits<std::uint64_t>::max();
    }

    inline void Writer::writeHeaderData(ByteBuffer & buffer) {}

} // namespace ParticleZoo::ROOT

#endif // USE_ROOT