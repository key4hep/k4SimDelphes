#ifndef DELPHESEDM4HEP_DELPHESEDM4HEP_CONVERTER_H__
#define DELPHESEDM4HEP_DELPHESEDM4HEP_CONVERTER_H__

// podio
#include "podio/CollectionBase.h"

// edm4hep
#include "edm4hep/MCParticleConst.h"
#include "edm4hep/ReconstructedParticle.h"
#include "edm4hep/MCRecoParticleAssociationCollection.h"

// ROOT
#include "TTree.h"
#include "TClonesArray.h"

//Delphes
#include "modules/Delphes.h"

#include <vector>
#include <string>
#include <string_view>
#include <array>
#include <unordered_map>
#include <optional>


// Delphes output classes
class Muon;
class Electron;
class Photon;

namespace k4SimDelphes {


/**
 * Classes that will be stored as reconstructed particle with an attached track
 */
constexpr std::array<std::string_view, 1> RECO_TRACK_OUTPUT = {"Track"};

/**
 * Classes that will be stored as reconstructed particle with an attached cluster
 */
constexpr std::array<std::string_view, 1> RECO_CLUSTER_OUTPUT = {"Tower"};

struct BranchSettings {
  std::string input;
  std::string name;
  std::string className;
};

std::vector<BranchSettings> getBranchSettings(ExRootConfParam /*const&*/treeConf) {
  std::vector<k4SimDelphes::BranchSettings> branches;
  for (int b = 0; b < treeConf.GetSize(); b += 3) {
    k4SimDelphes::BranchSettings branch{treeConf[b].GetString(),
                                        treeConf[b + 1].GetString(),
                                        treeConf[b + 2].GetString()};
    branches.push_back(branch);
  }
  return branches;
}

class OutputSettings;

class DelphesEDM4HepConverter {
public:

  DelphesEDM4HepConverter(std::string filename_delphescard);

  DelphesEDM4HepConverter(const std::vector<BranchSettings>& branches,
                          OutputSettings const& outputSettings, double magFieldBz);

  void process(TTree* delphesTree);

  inline std::unordered_map<std::string_view, podio::CollectionBase*> getCollections() { return m_collections; }


  void processParticles(const TClonesArray* delphesCollection, std::string_view const branch);
  void processTracks(const TClonesArray* delphesCollection, std::string_view const branch);
  void processClusters(const TClonesArray* delphesCollection, std::string_view const branch);
  void processJets(const TClonesArray* delphesCollection, std::string_view const branch);
  void processPhotons(const TClonesArray* delphesCollection, std::string_view const branch) {
    fillReferenceCollection<Photon>(delphesCollection, branch, "photon");
  }

  void processMissingET(const TClonesArray* delphesCollection, std::string_view const branch);
  void processScalarHT(const TClonesArray* delphesCollection, std::string_view const branch);

  void processMuons(const TClonesArray* delphesCollection, std::string_view const branch) {
    fillReferenceCollection<Muon>(delphesCollection, branch, "muon");
  }
  void processElectrons(const TClonesArray* delphesCollection, std::string_view const branch) {
    fillReferenceCollection<Electron>(delphesCollection, branch, "electron");
  }

edm4hep::MCRecoParticleAssociationCollection* createExternalRecoAssociations(const std::unordered_map<UInt_t, edm4hep::ConstMCParticle>& mc_map);

private:

  template<typename DelphesT>
  void fillReferenceCollection(const TClonesArray* delphesCollection, std::string_view const branch,
                               const std::string_view type);

  void registerGlobalCollections();

  template<typename CollectionT>
  void createCollection(std::string_view const name);

  // cannot mark DelphesT as const, because for Candidate* the GetCandidates()
  // method is not marked as const.
  template<typename DelphesT>
  std::optional<edm4hep::ReconstructedParticle> getMatchingReco(/*const*/ DelphesT* delphesCand) const;

  using ProcessFunction = void (DelphesEDM4HepConverter::*)(const TClonesArray*, std::string_view const);

  std::vector<BranchSettings> m_branches;
  std::unordered_map<std::string_view, podio::CollectionBase*> m_collections;
  std::unordered_map<std::string_view, ProcessFunction> m_processFunctions;

  double m_magneticFieldBz; // necessary for determining track parameters

  std::string m_recoCollName;
  std::string m_particleIDName;
  std::string m_mcRecoAssocCollName;

  // map from UniqueIDs (delphes generated particles) to MCParticles
  std::unordered_map<UInt_t, edm4hep::ConstMCParticle> m_genParticleIds;
  // map from UniqueIDs (delphes generated particles) to (possibly multiple)
  // ReconstructedParticles
  std::unordered_multimap<UInt_t, edm4hep::ReconstructedParticle> m_recoParticleGenIds;
};

template<typename CollectionT>
void DelphesEDM4HepConverter::createCollection(std::string_view name) {
  std::string nameStr(name);
  CollectionT* col = new CollectionT();
  m_collections.emplace(name, col);
}


} //namespace k4SimDelphes

#endif
