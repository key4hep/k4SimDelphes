#ifndef DELPHESEDM4HEP_DELPHESEDM4HEP_CONVERTER_H__
#define DELPHESEDM4HEP_DELPHESEDM4HEP_CONVERTER_H__

// edm4hep
#include "edm4hep/MCParticle.h"
#include "edm4hep/MutableReconstructedParticle.h"
#include "edm4hep/RecoMCParticleLinkCollection.h"

// podio
#include "podio/CollectionBase.h"

// ROOT
#include "TClonesArray.h"
#include "TTree.h"

//Delphes
#include "classes/DelphesClasses.h"
#include "modules/Delphes.h"

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

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

  /**
   * Classes that will be stored as particle flow candidates
   */
  constexpr std::array<std::string_view, 1> RECO_CANDIDATES_OUTPUT = {"ParticleFlowCandidate"};

  /**
   * Classes that will be stored as TrackerHits
   */
  constexpr auto TRACKERHIT_OUTPUT_NAME = "TrackerHits";

  /**
   * * Classes that will be stored as TrackerHits
   */
  constexpr auto CALORIMETERHIT_OUTPUT_NAME = "CalorimeterHits";

  /**
   * * Eventheader class will be stored only once
   */
  constexpr auto EVENTHEADER_NAME = "EventHeader";

  struct BranchSettings {
    std::string input;
    std::string name;
    std::string className;
  };

  inline std::vector<BranchSettings> getBranchSettings(ExRootConfParam /*const&*/ treeConf) {
    std::vector<k4SimDelphes::BranchSettings> branches;
    for (int b = 0; b < treeConf.GetSize(); b += 3) {
      k4SimDelphes::BranchSettings branch{treeConf[b].GetString(), treeConf[b + 1].GetString(),
                                          treeConf[b + 2].GetString()};
      branches.push_back(branch);
    }
    return branches;
  }

  class OutputSettings;

  class DelphesEDM4HepConverter {
  public:
    using CollectionMapT = std::unordered_map<std::string, std::unique_ptr<podio::CollectionBase>>;

    DelphesEDM4HepConverter(std::string filename_delphescard);

    DelphesEDM4HepConverter(const std::vector<BranchSettings>& branches, OutputSettings const& outputSettings,
                            double magFieldBz);

    /**
     * Process the passed delphesTree and convert the particles contained in it.
     */
    void process(TTree* delphesTree);

    /** Get the converted collections and their ownership.
     *
     * NOTE: Since this moves the ownership of the internal collections, this
     * can only be called once to actually get a filled map, after a call to
     * process.
     */
    CollectionMapT getCollections() { return std::move(m_collections); }

    edm4hep::RecoMCParticleLinkCollection* createExternalRecoMCLinks(
        const std::unordered_map<UInt_t, edm4hep::MCParticle>& mc_map);

  private:
    void createEventHeader(const HepMCEvent* delphesEvent);

    void processParticles(const TClonesArray* delphesCollection, std::string const& branch);
    void processTracks(const TClonesArray* delphesCollection, std::string const& branch);
    void processPFlowCandidates(const TClonesArray* delphesCollection, std::string const& branch);
    void processClusters(const TClonesArray* delphesCollection, std::string const& branch);
    void processJets(const TClonesArray* delphesCollection, std::string const& branch);
    void processPhotons(const TClonesArray* delphesCollection, std::string const& branch) {
      fillReferenceCollection<Photon>(delphesCollection, branch, "photon");
    }

    void processMissingET(const TClonesArray* delphesCollection, std::string const& branch);
    void processScalarHT(const TClonesArray* delphesCollection, std::string const& branch);

    void processMuons(const TClonesArray* delphesCollection, std::string const& branch) {
      fillReferenceCollection<Muon>(delphesCollection, branch, "muon");
    }
    void processElectrons(const TClonesArray* delphesCollection, std::string const& branch) {
      fillReferenceCollection<Electron>(delphesCollection, branch, "electron");
    }

  private:
    template <typename DelphesT>
    void fillReferenceCollection(const TClonesArray* delphesCollection, std::string const& branch,
                                 const std::string_view type);

    void registerGlobalCollections();

    /** Create a collection in the internal map
     */
    template <typename CollectionT> CollectionT* createCollection(std::string const& name, bool makeRefColl = false);

    /** Get a collection that has already been registered in the internal map
     */
    template <typename CollectionT> CollectionT* getCollection(const std::string& name) {
      return static_cast<CollectionT*>(m_collections[name].get());
    }

    // cannot mark DelphesT as const, because for Candidate* the GetCandidates()
    // method is not marked as const.
    template <typename DelphesT>
    std::optional<edm4hep::MutableReconstructedParticle> getMatchingReco(/*const*/ DelphesT* delphesCand) const;

    using ProcessFunction = void (DelphesEDM4HepConverter::*)(const TClonesArray*, std::string const&);

    std::vector<BranchSettings>                           m_branches;
    CollectionMapT                                        m_collections;
    std::unordered_map<std::string_view, ProcessFunction> m_processFunctions;

    double m_magneticFieldBz;  // necessary for determining track parameters

    std::string m_recoCollName;
    std::string m_recoMCLinkCollName;

    // map from UniqueIDs (delphes generated particles) to MCParticles
    std::unordered_map<UInt_t, edm4hep::MCParticle> m_genParticleIds;
    // map from UniqueIDs (delphes generated particles) to (possibly multiple)
    // ReconstructedParticles. This map is necessary because Muon, Electron and
    // Photon can only be reliably matched via their gen particles
    std::unordered_multimap<UInt_t, edm4hep::MutableReconstructedParticle> m_recoParticleGenIds;
    // Map of UniqueIDs (delphes candidates) to ReconstrucedParticles. Used in
    // putting together the list of jet constituents. This is necessary because
    // the particle flow implementation of Delphes creates candidates without a
    // gen particle.
    std::unordered_map<UInt_t, edm4hep::MutableReconstructedParticle> m_recoParticleIds;
  };

  template <typename CollectionT>
  CollectionT* DelphesEDM4HepConverter::createCollection(std::string const& name, bool makeRefColl) {
    auto col = std::make_unique<CollectionT>();
    col->setSubsetCollection(makeRefColl);
    auto [it, inserted] = m_collections.emplace(name, std::move(col));
    if (!inserted) {
      std::cerr << "K4SIMDELPHES ERROR: Collection with name " << name
                << " already created. Cannot have a second collection with this name!" << std::endl;
      // TODO: Something more drastic?
    }

    return static_cast<CollectionT*>(it->second.get());
  }

}  //namespace k4SimDelphes

#endif
