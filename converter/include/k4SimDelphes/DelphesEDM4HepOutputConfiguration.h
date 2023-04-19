#ifndef K4SIMDELPHES_DELPHESEDM4HEP_OUTPUTCONFIGURATION_H__
#define K4SIMDELPHES_DELPHESEDM4HEP_OUTPUTCONFIGURATION_H__

#include "ExRootAnalysis/ExRootConfReader.h"

#include <iomanip>
#include <ostream>
#include <string>
#include <vector>

namespace k4SimDelphes {
  /**
   * The settings steering the output of the DelphesEDM4HepConverter. Default
   * values correspond to the ones found in delphes example cards.
   */
  struct OutputSettings {
    /**
     * Branches that will be stored in one global ReconstructedParticle collection
     * under the name specified below. These will be associated with the generated
     * particles.
     */
    std::vector<std::string> ReconstructedParticleCollections{{"EflowTrack", "EFlowPhoton", "EFlowNeutralHadron"}};

    /**
     * Branches that will be considered and stored as MCParticle. Each Delphes
     * collection will get its own edm4hep collection, but all generated particles
     * will be considered for the associations to reconstructed particles.
     */
    std::vector<std::string> GenParticleCollections{{"GenParticle"}};

    /**
     * Branches that contain delphes jet collections that will be converted to
     * ReconstructedParticle. Each delphes collection gets its own edm4hep
     * collection. The constituents of the jets will be taken from the global
     * ReconstructedParticle collection.
     */
    std::vector<std::string> JetCollections{{"Jet"}};

    /**
     * Name of the delphes muon collections that will be converted to
     * RecoParticleRefs that point into the global reconstructed particle
     * collection. Each delphes collection gets its own edm4hep collection.
     */
    std::vector<std::string> MuonCollections{{"Muon"}};

    /**
     * Name of the delphes electron collections that will be converted to
     * RecoParticleRefs that point into the global reconstructed particle
     * collection. Each delphes collection gets its own edm4hep collection.
     */
    std::vector<std::string> ElectronCollections{{"Electron"}};

    /**
     * Name of the delphes photon collections that will be converted to
     * RecoParticleRefs that point into the global reconstructed particle
     * collection. Each delphes collection gets its own edm4hep collection.
     */
    std::vector<std::string> PhotonCollections{{"Photon"}};

    /**
     * Name of the delphes MissingET collections that are converted and stored as
     * ReconstructedParticle (one per event). Each delphes collection will get its
     * own edm4hep collection.
     */
    std::vector<std::string> MissingETCollections{{"MissingET"}};

    /**
     * Name of the delphes ScalarHT collections that are converted and stored as
     * ParticleID objects (one per event). Each delphes collection will get its
     * own edm4hep collection.
     */
    std::vector<std::string> ScalarHTCollections{{"ScalarHT"}};

    /**
     * Name of the global ReconstructedParticle collection
     */
    std::string RecoParticleCollectionName{"ReconstructedParticles"};

    /**
     * Name of the MCRecoParticleAssociationCollection holding the associations of
     * generated to reconstructed particles.
     */
    std::string MCRecoAssociationCollectionName{"MCRecoAssociations"};

    /**
     * Name of the ParticleIDCollection holding the ctags / isolation variables.
     */
    std::string ParticleIDCollectionName{"ParticleIDs"};
  };

  template <typename T> std::ostream& operator<<(std::ostream& os, std::vector<T> const& container) {
    if (container.empty()) {
      os << " -empty- ";
      return os;
    }

    for (size_t i = 0; i < container.size() - 1; ++i) {
      os << container[i] << ", ";
    }
    os << container.back();
    return os;
  }

  std::ostream& operator<<(std::ostream& os, OutputSettings const& settings) {
    os << "---------- DelphesEDM4HepConverter OutputSettings ----------\n";
    os << std::setw(40) << " ReconstructedParticleCollections: " << settings.ReconstructedParticleCollections << "\n";
    os << std::setw(40) << " GenParticleCollections: " << settings.GenParticleCollections << "\n";
    os << std::setw(40) << " JetCollections: " << settings.JetCollections << "\n";
    os << std::setw(40) << " MuonCollections: " << settings.MuonCollections << "\n";
    os << std::setw(40) << " ElectronCollections: " << settings.ElectronCollections << "\n";
    os << std::setw(40) << " PhotonCollections: " << settings.PhotonCollections << "\n";
    os << std::setw(40) << " MissingETCollections: " << settings.MissingETCollections << "\n";
    os << std::setw(40) << " ScalarHTCollections: " << settings.ScalarHTCollections << "\n";
    os << std::setw(40) << " RecoParticleCollectionName: " << settings.RecoParticleCollectionName << "\n";
    os << std::setw(40) << " MCRecoAssociationCollectionName: " << settings.MCRecoAssociationCollectionName << "\n";
    os << "------------------------------------------------------------\n";

    return os;
  }

  std::vector<std::string> toVecString(ExRootConfParam param, std::vector<std::string>&& defVals) {
    if (!param.GetSize()) {
      return std::move(defVals);
    }

    std::vector<std::string> paramVals(param.GetSize());
    for (int i = 0; i < param.GetSize(); ++i) {
      paramVals[i] = param[i].GetString();
    }
    return paramVals;
  }

  OutputSettings getEDM4hepOutputSettings(ExRootConfReader* confReader) {
    OutputSettings settings;

    settings.ReconstructedParticleCollections =
        toVecString(confReader->GetParam("EDM4HepOutput::ReconstructedParticleCollections"),
                    {"EFlowTrack", "EFlowPhoton", "EFlowNeutralHadron"});

    settings.GenParticleCollections =
        toVecString(confReader->GetParam("EDM4HepOutput::GenParticleCollections"), {"GenParticle"});

    settings.JetCollections = toVecString(confReader->GetParam("EDM4HepOutput::JetCollections"), {});

    settings.MuonCollections = toVecString(confReader->GetParam("EDM4HepOutput::MuonCollections"), {});

    settings.ElectronCollections = toVecString(confReader->GetParam("EDM4HepOutput::ElectronCollections"), {});

    settings.PhotonCollections = toVecString(confReader->GetParam("EDM4HepOutput::PhotonCollections"), {});

    settings.MissingETCollections = toVecString(confReader->GetParam("EDM4HepOutput::MissingETCollections"), {});

    settings.ScalarHTCollections = toVecString(confReader->GetParam("EDM4HepOutput::ScalarHTCollections"), {});

    settings.RecoParticleCollectionName =
        confReader->GetString("EDM4HepOutput::RecoParticleCollectionName", "ReconstructedParticles");

    settings.MCRecoAssociationCollectionName =
        confReader->GetString("EDM4HepOutput::MCRecoAssociationCollectionName", "MCRecoAssociations");

    return settings;
  }

  OutputSettings getEDM4hepOutputSettings(const char* confFile) {
    ExRootConfReader confReader{};
    confReader.ReadFile(confFile);
    return getEDM4hepOutputSettings(&confReader);
  }

}  // namespace k4SimDelphes

#endif
