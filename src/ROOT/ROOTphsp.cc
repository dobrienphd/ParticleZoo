#ifdef USE_ROOT

#include "particlezoo/ROOT/ROOTphsp.h"

#include <iostream>
#include <TKey.h>
#include <TClass.h>
#include <TSystem.h>

namespace ParticleZoo::ROOT {

    CLICommand ROOTFormatCommand{ BOTH, "", "ROOT-format", "Specify the predefined template for the ROOT branches, options are 'TOPAS' or 'OpenGATE'.", { CLI_STRING }, { "TOPAS" } };
    CLICommand ROOTTreeNameCommand{ BOTH, "", "ROOT-tree-name", "Name of the ROOT TTree to read from or write to.", { CLI_STRING } };
    CLICommand ROOTEnergyCommand{ BOTH, "", "ROOT-energy", "Name of the ROOT branch containing the particle energy.", { CLI_STRING } };
    CLICommand ROOTWeightCommand{ BOTH, "", "ROOT-weight", "Name of the ROOT branch containing the particle weight.", { CLI_STRING } };
    CLICommand ROOTPositionXCommand{ BOTH, "", "ROOT-position-x", "Name of the ROOT branch containing the particle X position.", { CLI_STRING } };
    CLICommand ROOTPositionYCommand{ BOTH, "", "ROOT-position-y", "Name of the ROOT branch containing the particle Y position.", { CLI_STRING } };
    CLICommand ROOTPositionZCommand{ BOTH, "", "ROOT-position-z", "Name of the ROOT branch containing the particle Z position.", { CLI_STRING } };
    CLICommand ROOTDirectionalCosineXCommand{ BOTH, "", "ROOT-cosine-x", "Name of the ROOT branch containing the particle X directional cosine.", { CLI_STRING } };
    CLICommand ROOTDirectionalCosineYCommand{ BOTH, "", "ROOT-cosine-y", "Name of the ROOT branch containing the particle Y directional cosine.", { CLI_STRING } };
    CLICommand ROOTDirectionalCosineZCommand{ BOTH, "", "ROOT-cosine-z", "Name of the ROOT branch containing the particle Z directional cosine.", { CLI_STRING } };
    CLICommand ROOTDirectionalCosineZIsNegativeCommand{ BOTH, "", "ROOT-cosine-z-sign", "Name of the ROOT branch containing the flag to indicate if the particle Z directional cosine is negative.", { CLI_STRING } };
    CLICommand ROOTPDGCodeCommand{ BOTH, "", "ROOT-pdg-code", "Name of the ROOT branch containing the particle PDG code.", { CLI_STRING } };
    CLICommand ROOTHistoryNumberCommand{ BOTH, "", "ROOT-history-number", "Name of the ROOT branch containing the particle history number.", { CLI_STRING } };

    inline std::string extractStringFromMap(const std::map<std::string, BranchInfo> & branchNames, const std::string & key) {
        auto it = branchNames.find(key);
        if (it != branchNames.end()) {
            return it->second.branchName;
        } else {
            return "";
        }
    }

    inline const std::map<std::string,BranchInfo> getBranchDetailsFromUserOptions(const UserOptions & options) {
        std::map<std::string,BranchInfo> branchNames;

        if (options.contains(ROOTFormatCommand)) {
            CLIValue formatValue = options.at(ROOTFormatCommand).front();
            std::string format = std::get<std::string>(formatValue);
            if (format == "TOPAS") {
                branchNames = TOPASBranches;
            } else if (format == "OpenGATE") {
                branchNames = OPENGateBranches;
            } else {
                throw std::runtime_error("Unsupported ROOT format: " + format);
            }
        } else {
            branchNames = defaultBranchNames;
        }

        // Override branch names and based on user options
        for (const auto & [key, value] : options) {
            if (key == ROOTTreeNameCommand) {
                branchNames["treeName"].branchName = std::get<std::string>(value[0]);
            } else if (key == ROOTEnergyCommand) {
                branchNames["energy"].branchName = std::get<std::string>(value[0]);
            } else if (key == ROOTWeightCommand) {
                branchNames["weight"].branchName = std::get<std::string>(value[0]);
            } else if (key == ROOTPositionXCommand) {
                branchNames["positionX"].branchName = std::get<std::string>(value[0]);
            } else if (key == ROOTPositionYCommand) {
                branchNames["positionY"].branchName = std::get<std::string>(value[0]);
            } else if (key == ROOTPositionZCommand) {
                branchNames["positionZ"].branchName = std::get<std::string>(value[0]);
            } else if (key == ROOTDirectionalCosineXCommand) {
                branchNames["directionalCosineX"].branchName = std::get<std::string>(value[0]);
            } else if (key == ROOTDirectionalCosineYCommand) {
                branchNames["directionalCosineY"].branchName = std::get<std::string>(value[0]);
            } else if (key == ROOTDirectionalCosineZCommand) {
                branchNames["directionalCosineZ"].branchName = std::get<std::string>(value[0]);
            } else if (key == ROOTDirectionalCosineZIsNegativeCommand) {
                branchNames["directionalCosineZIsNegative"].branchName = std::get<std::string>(value[0]);
            } else if (key == ROOTPDGCodeCommand) {
                branchNames["pdgCode"].branchName = std::get<std::string>(value[0]);
            } else if (key == ROOTHistoryNumberCommand) {
                branchNames["historyNumber"].branchName = std::get<std::string>(value[0]);
            }
        }

        return branchNames;
    }

    Reader::Reader(const std::string & fileName, const UserOptions & options)
    : Reader(fileName, getBranchDetailsFromUserOptions(options), options)
    {
    }

    Reader::Reader(const std::string & fileName,
                    const std::map<std::string,BranchInfo> & branchNames,
                    const UserOptions & options)
    : ParticleZoo::PhaseSpaceFileReader("ROOT", fileName, options, FormatType::NONE)
    {
        file = TFile::Open(fileName.c_str(), "READ");
        if (!file || !file->IsOpen() || file->IsZombie()) {
            int err = gSystem->GetErrno();
            throw std::runtime_error("Failed to open ROOT file: " + fileName + " with error number " + std::to_string(err) + " (" + strerror(err) + ")");
        }

        std::string treeName = extractStringFromMap(branchNames, "treeName");

        if (treeName.empty()) {
            // If no tree name is provided, look for the first TTree in the file
            TIter nextKey(file->GetListOfKeys());
            TKey* key;
            while ((key = static_cast<TKey*>(nextKey()))) {
                if (std::string(key->GetClassName()) == "TTree") {
                    tree = dynamic_cast<TTree*>(key->ReadObj());
                    break;
                }
            }
        } else {
            // If a tree name is provided, check if it exists
            if (!file->GetListOfKeys()->Contains(treeName.c_str())) {
                throw std::runtime_error("TTree with name '" + treeName + "' not found in ROOT file: " + fileName);
            }
            tree = dynamic_cast<TTree*>(file->Get(treeName.c_str()));
        }

        if (!tree) {
            throw std::runtime_error("No TTree found in ROOT file: " + fileName);
        }

        numberOfParticles_ = tree->GetEntries();

        std::string isNewHistoryBranchName = extractStringFromMap(branchNames, "isNewHistory");
        std::string energyBranchName       = extractStringFromMap(branchNames, "energy");
        std::string weightBranchName       = extractStringFromMap(branchNames, "weight");
        std::string positionXBranchName    = extractStringFromMap(branchNames, "positionX");
        std::string positionYBranchName    = extractStringFromMap(branchNames, "positionY");
        std::string positionZBranchName    = extractStringFromMap(branchNames, "positionZ");
        std::string directionalCosineXBranchName    = extractStringFromMap(branchNames, "directionalCosineX");
        std::string directionalCosineYBranchName    = extractStringFromMap(branchNames, "directionalCosineY");
        std::string directionalCosineZBranchName    = extractStringFromMap(branchNames, "directionalCosineZ");
        std::string directionalCosineZIsNegativeBranchName    = extractStringFromMap(branchNames, "directionalCosineZIsNegative");
        std::string pdgCodeBranchName      = extractStringFromMap(branchNames, "pdgCode");
        std::string historyNumberBranchName = extractStringFromMap(branchNames, "historyNumber");

        // mandatory branches
        if (energyBranchName.length()>0 && tree->GetBranch(energyBranchName.c_str())) {
            tree->SetBranchAddress(energyBranchName.c_str(),    &energy_);
            energyUnits_ = branchNames.at("energy").unitFactor; // Set the units for energy
        } else 
            throw std::runtime_error("Branch '" + energyBranchName + "' not found in TTree: " + treeName);

        if (positionXBranchName.length()>0 && tree->GetBranch(positionXBranchName.c_str())) {
            tree->SetBranchAddress(positionXBranchName.c_str(), &x_);
            xUnits_ = branchNames.at("positionX").unitFactor; // Set the units for x
        } else
            throw std::runtime_error("Branch '" + positionXBranchName + "' not found in TTree: " + treeName);

        if (positionYBranchName.length()>0 && tree->GetBranch(positionYBranchName.c_str())) {
            tree->SetBranchAddress(positionYBranchName.c_str(), &y_);
            yUnits_ = branchNames.at("positionY").unitFactor; // Set the units for y
        } else
            throw std::runtime_error("Branch '" + positionYBranchName + "' not found in TTree: " + treeName);

        if (positionZBranchName.length()>0 && tree->GetBranch(positionZBranchName.c_str())) {
            tree->SetBranchAddress(positionZBranchName.c_str(), &z_);
            zUnits_ = branchNames.at("positionZ").unitFactor; // Set the units for z
        } else
            throw std::runtime_error("Branch '" + positionZBranchName + "' not found in TTree: " + treeName);

        if (directionalCosineXBranchName.length()>0 && tree->GetBranch(directionalCosineXBranchName.c_str()))
            tree->SetBranchAddress(directionalCosineXBranchName.c_str(), &px_);
        else
            throw std::runtime_error("Branch '" + directionalCosineXBranchName + "' not found in TTree: " + treeName);

        if (directionalCosineYBranchName.length()>0 && tree->GetBranch(directionalCosineYBranchName.c_str()))
            tree->SetBranchAddress(directionalCosineYBranchName.c_str(), &py_);
        else
            throw std::runtime_error("Branch '" + directionalCosineYBranchName + "' not found in TTree: " + treeName);

        if (pdgCodeBranchName.length()>0 && tree->GetBranch(pdgCodeBranchName.c_str()))
            tree->SetBranchAddress(pdgCodeBranchName.c_str(),   &pdgCode_);
        else
            throw std::runtime_error("Branch '" + pdgCodeBranchName + "' not found in TTree: " + treeName);

        // optional branches
        if (isNewHistoryBranchName.length()>0 && tree->GetBranch(isNewHistoryBranchName.c_str())) {
            treeHasNewHistoryMarker_ = true;
            tree->SetBranchAddress(isNewHistoryBranchName.c_str(), &isNewHistory_);
        } else if (isNewHistoryBranchName.length()>0) {
            treeHasNewHistoryMarker_ = false;
            std::cerr << "Warning: Branch '" << isNewHistoryBranchName << "' not found in TTree: " << treeName << ". All particles will be assumed to be new histories." << std::endl;
        }

        if (weightBranchName.length()>0 && tree->GetBranch(weightBranchName.c_str()))
            tree->SetBranchAddress(weightBranchName.c_str(),    &weight_);
        else if (weightBranchName.length()>0)
            std::cerr << "Warning: Branch '" << weightBranchName << "' not found in TTree: " << treeName << ". All particles will be assumed to have a weight of 1." << std::endl;

        if (directionalCosineZBranchName.length()>0 && tree->GetBranch(directionalCosineZBranchName.c_str())) {
            tree->SetBranchAddress(directionalCosineZBranchName.c_str(), &pz_);
            pzIsStored_ = true; // Indicate that pz is stored in the tree
        } else if (directionalCosineZBranchName.length()>0) {
            if (directionalCosineZIsNegativeBranchName.length()>0 && tree->GetBranch(directionalCosineZIsNegativeBranchName.c_str())) {
                tree->SetBranchAddress(directionalCosineZIsNegativeBranchName.c_str(), &pzIsNegative_);                
                std::cerr << "Warning: Branch '" << directionalCosineZBranchName << "' not found in TTree: " << treeName << ". Z directional cosine value will be calculated from the X and Y directional cosines." << std::endl;
            } else {
                pzIsNegative_ = false; // Default to positive if not specified
                std::cerr << "Warning: Branch '" << directionalCosineZBranchName << "' not found in TTree: " << treeName << ". Z directional cosine value will be calculated from the X and Y directional cosines. The sign will be assumed to be positive." << std::endl;
            }
        } else {
            if (directionalCosineZIsNegativeBranchName.length()>0 && tree->GetBranch(directionalCosineZIsNegativeBranchName.c_str())) {
                tree->SetBranchAddress(directionalCosineZIsNegativeBranchName.c_str(), &pzIsNegative_);
            } else {
                pzIsNegative_ = false; // Default to positive if not specified
                std::cerr << "Warning: Branch '" << directionalCosineZIsNegativeBranchName << "' not found in TTree: " << treeName << ". The sign of the Z directional cosine will be assumed to be positive." << std::endl;
            }
        }

        if (historyNumberBranchName.length()>0 && tree->GetBranch(historyNumberBranchName.c_str())) {
            treeHasHistoryNumber_ = true;
            tree->SetBranchAddress(historyNumberBranchName.c_str(), &historyNumber_);
        } else if (historyNumberBranchName.length()>0) {
            std::cerr << "Warning: Branch '" << historyNumberBranchName << "' not found in TTree: " << treeName << ". Empty histories will not be accounted for." << std::endl;
        }

        tree->SetCacheSize(256 * 1024 * 1024);       // 256 MB cache
        tree->SetCacheLearnEntries(500);            // learn basket layout on first 500 entries
        if (energyBranchName.length()>0) tree->AddBranchToCache(energyBranchName.c_str(), true);
        if (positionXBranchName.length()>0) tree->AddBranchToCache(positionXBranchName.c_str(), true);
        if (positionYBranchName.length()>0) tree->AddBranchToCache(positionYBranchName.c_str(), true);
        if (positionZBranchName.length()>0) tree->AddBranchToCache(positionZBranchName.c_str(), true);
        if (directionalCosineXBranchName.length()>0) tree->AddBranchToCache(directionalCosineXBranchName.c_str(), true);
        if (directionalCosineYBranchName.length()>0) tree->AddBranchToCache(directionalCosineYBranchName.c_str(), true);
        if (directionalCosineZBranchName.length()>0) tree->AddBranchToCache(directionalCosineZBranchName.c_str(), true);
        if (directionalCosineZIsNegativeBranchName.length()>0) tree->AddBranchToCache(directionalCosineZIsNegativeBranchName.c_str(), true);
        if (weightBranchName.length()>0) tree->AddBranchToCache(weightBranchName.c_str(), true);
        if (pdgCodeBranchName.length()>0) tree->AddBranchToCache(pdgCodeBranchName.c_str(), true);
        if (isNewHistoryBranchName.length()>0) tree->AddBranchToCache(isNewHistoryBranchName.c_str(), true);
        if (historyNumberBranchName.length()>0) tree->AddBranchToCache(historyNumberBranchName.c_str(), true);

        if (numberOfParticles_ > 0) {
            // determine the number of original histories in the file, if possible
            if (treeHasHistoryNumber_) { // if we have a history number branch, we can determine the number of original histories directly
                int firstHistoryNumber, lastHistoryNumber;
                tree->GetEntry(0);
                firstHistoryNumber = historyNumber_;
                tree->GetEntry(numberOfParticles_-1);
                lastHistoryNumber = historyNumber_;
                int originalHistories = lastHistoryNumber - firstHistoryNumber + 1;
                if (originalHistories <= 0) {
                    std::cerr << "Warning: The history numbers in branch '" << historyNumberBranchName << "' do not appear to be valid. Empty histories will not be accounted for." << std::endl;
                    treeHasHistoryNumber_ = false;
                    numberOfOriginalHistories_ = 0;
                } else {
                    numberOfOriginalHistories_ = static_cast<std::uint64_t>(originalHistories);
                }
                historyNumber_ = firstHistoryNumber - 1; // Adjust initial history number for incremental histories
            } else if (treeHasNewHistoryMarker_) { // if we have a new history marker, we need to loop over all particles to count the number of histories
                numberOfOriginalHistories_ = 0;
                for (std::uint64_t i=0; i<numberOfParticles_; ++i) {
                    tree->GetEntry(i);
                    if (isNewHistory_) numberOfOriginalHistories_++;
                }
                historyNumber_ = -1; // Reset initial history number for incremental histories
            } else { // if no history information is available, assume each particle is a new history
                numberOfOriginalHistories_ = numberOfParticles_;
                historyNumber_ = -1; // Reset initial history number for incremental histories
            }
        } else { // no particles in the file
            numberOfOriginalHistories_ = 0;
            historyNumber_ = -1;
        }
    }

    Reader::~Reader()
    {
        if (file) {
            file->Close();
            delete file;
        }
    }

    std::vector<CLICommand> Reader::getFormatSpecificCLICommands() {
        return {
            ROOTFormatCommand,
            ROOTTreeNameCommand,
            ROOTEnergyCommand,
            ROOTWeightCommand,
            ROOTPositionXCommand,
            ROOTPositionYCommand,
            ROOTPositionZCommand,
            ROOTDirectionalCosineXCommand,
            ROOTDirectionalCosineYCommand,
            ROOTDirectionalCosineZCommand,
            ROOTDirectionalCosineZIsNegativeCommand,
            ROOTPDGCodeCommand,
            ROOTHistoryNumberCommand
        };
    }

    Particle Reader::readParticleManually()
    {
        std::uint64_t particlesRead = getParticlesRead(true);
        if (particlesRead >= numberOfParticles_) throw std::runtime_error("Attempted to read more particles than available in the ROOT file.");

        if (!treeHasHistoryNumber_) {
            historyNumber_ = static_cast<int>(getHistoriesRead());
        }
        
        int lastHistoryNumber = historyNumber_;

        tree->GetEntry(particlesRead++);

        if (!treeHasHistoryNumber_ && !treeHasNewHistoryMarker_) isNewHistory_ = true; // If no history info is available, assume each particle is a new history

        ParticleType type = getParticleTypeFromPDGID(pdgCode_);

        if (!pzIsStored_) {
            pz_ = calcThirdUnitComponent(px_, py_);
            if (pzIsNegative_) { pz_ = -pz_; }
        }

        int historyIncrement = historyNumber_ - lastHistoryNumber;
        if (getHistoriesRead() == 0 || (historyIncrement == 0 && isNewHistory_)) historyIncrement = 1;
        isNewHistory_ = historyIncrement > 0;

        Particle particle(type,
                        energy_ * energyUnits_,
                        x_ * xUnits_,
                        y_ * yUnits_,
                        z_ * zUnits_,
                        px_,
                        py_,
                        pz_,
                        isNewHistory_,
                        weight_);

        if (treeHasHistoryNumber_) {
            particle.setIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER, historyIncrement);
        }

        return particle;
    }


    // Implementation of Writer class

    Writer::Writer(const std::string & fileName, const UserOptions & options)
    : Writer(fileName, getBranchDetailsFromUserOptions(options), options)
    {
    }

    Writer::Writer(const std::string & fileName, const std::map<std::string,BranchInfo> & branchNames, const UserOptions & options)
    : ParticleZoo::PhaseSpaceFileWriter("ROOT", fileName, options, FormatType::NONE), branchNames_(branchNames)
    {
        file_ = TFile::Open(fileName.c_str(), "RECREATE");
        if (!file_ || !file_->IsOpen() || file_->IsZombie()) {
            int err = gSystem->GetErrno();
            throw std::runtime_error("Failed to open ROOT file for writing: " + fileName + " with error number " + std::to_string(err) + " (" + strerror(err) + ")");
        }

        file_->cd();

        std::string treeName = extractStringFromMap(branchNames_, "treeName");
        if (treeName.empty()) {
            treeName = DEFAULT_TREE_NAME;
        }

        tree_ = new TTree(treeName.c_str(), treeName.c_str());
        tree_->SetAutoSave(0); // Disable autosave cycles to avoid unnecessary writes

        for (const auto & [key, info] : branchNames_) {
            if (info.branchName.empty()) continue; // Skip empty branch names

            std::string branchKey = key;
            std::string branchName = info.branchName;

            if (branchKey == "treeName") {
                continue; // Skip treeName as it is not a branch
            } else if (branchKey == "energy") {
                tree_->Branch(branchName.c_str(), &energy_, (branchName + "/D").c_str());
                inverseEnergyUnits_ = 1.0 / info.unitFactor; // Set the units for energy
            } else if (branchKey == "positionX") {
                tree_->Branch(branchName.c_str(), &x_, (branchName + "/D").c_str());
                inverseXUnits_ = 1.0 / info.unitFactor; // Set the units for positionX
            } else if (branchKey == "positionY") {
                tree_->Branch(branchName.c_str(), &y_, (branchName + "/D").c_str());
                inverseYUnits_ = 1.0 / info.unitFactor; // Set the units for positionY
            } else if (branchKey == "positionZ") {
                tree_->Branch(branchName.c_str(), &z_, (branchName + "/D").c_str());
                inverseZUnits_ = 1.0 / info.unitFactor; // Set the units for positionZ
            } else if (branchKey == "directionalCosineX") {
                tree_->Branch(branchName.c_str(), &px_, (branchName + "/D").c_str());
            } else if (branchKey == "directionalCosineY") {
                tree_->Branch(branchName.c_str(), &py_, (branchName + "/D").c_str());
            } else if (branchKey == "directionalCosineZ") {
                tree_->Branch(branchName.c_str(), &pz_, (branchName + "/D").c_str());
                pzIsStored_ = true; // Indicate that pz is stored in the tree
            } else if (branchKey == "directionalCosineZIsNegative") {
                tree_->Branch(branchName.c_str(), &pzIsNegative_, (branchName + "/O").c_str());
            } else if (branchKey == "weight") {
                tree_->Branch(branchName.c_str(), &weight_, (branchName + "/D").c_str());
            } else if (branchKey == "pdgCode") {
                tree_->Branch(branchName.c_str(), &pdgCode_, (branchName + "/I").c_str());
            } else if (branchKey == "isNewHistory") {
                tree_->Branch(branchName.c_str(), &isNewHistory_, (branchName + "/O").c_str());
            } else if (branchKey == "historyNumber") {
                tree_->Branch(branchName.c_str(), &historyNumber_, (branchName + "/I").c_str());
                storeIncrementalHistories_ = true;
            }
        }

        file_->SetCompressionLevel(0); // no compression results in faster I/O, performance is more important than file size
    }

    Writer::~Writer()
    {
        if (file_) {
            tree_->Write();
            file_->Close();
            delete file_;
        }
    }

    std::vector<CLICommand> Writer::getFormatSpecificCLICommands() {
        return { 
            ROOTFormatCommand,
            ROOTTreeNameCommand,
            ROOTEnergyCommand,
            ROOTWeightCommand,
            ROOTPositionXCommand,
            ROOTPositionYCommand,
            ROOTPositionZCommand,
            ROOTDirectionalCosineXCommand,
            ROOTDirectionalCosineYCommand,
            ROOTDirectionalCosineZCommand,
            ROOTDirectionalCosineZIsNegativeCommand,
            ROOTPDGCodeCommand,
            ROOTHistoryNumberCommand
        };
    }

    void Writer::writeParticleManually(Particle & particle)
    {
        energy_ = particle.getKineticEnergy() * inverseEnergyUnits_;
        x_ = particle.getX() * inverseXUnits_;
        y_ = particle.getY() * inverseYUnits_;
        z_ = particle.getZ() * inverseZUnits_;
        px_ = particle.getDirectionalCosineX();
        py_ = particle.getDirectionalCosineY();
        pz_ = particle.getDirectionalCosineZ();
        pzIsNegative_ = (pz_ < 0.0); // Store the sign of the Z directional cosine
        weight_ = particle.getWeight();
        pdgCode_ = getPDGIDFromParticleType(particle.getType());
        isNewHistory_ = particle.isNewHistory();
        if (storeIncrementalHistories_) {
            int historyIncrement = isNewHistory_ ? 1 : 0;
            if (particle.hasIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER)) {
                historyIncrement = particle.getIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER);
            }
            historyNumber_ += historyIncrement;
        }

        tree_->Fill();
    }

} // namespace ParticleZoo::ROOT

#endif // USE_ROOT