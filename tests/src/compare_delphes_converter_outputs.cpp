#include "delphesHelpers.h"

#include "edm4hep/MCParticleCollection.h"
#include "edm4hep/ReconstructedParticleCollection.h"
#include "edm4hep/RecoParticleRefCollection.h"
#include "edm4hep/MCRecoParticleAssociationCollection.h"

#include "podio/ROOTReader.h"
#include "podio/EventStore.h"

#include "classes/DelphesClasses.h"
#include "ExRootAnalysis/ExRootTreeReader.h"
#include "ExRootAnalysis/ExRootTreeBranch.h"

#include "Math/Vector4D.h"
#include "TChain.h"
#include "TClonesArray.h"
#include "TLorentzVector.h"

#include <iostream>
#include <memory>
#include <cstdlib>
#include <vector>

/**
 * Compare the 4-vectors of the delphes and the edm4hep candidate
 */
template<typename DelphesT, typename EDM4HepT>
bool compareKinematics(const DelphesT* delphesCand, const EDM4HepT& edm4hepCand) {
  using namespace k4SimDelphes;
  // Use the same matching criteria as in the converter: First try with all
  // components, if that doesn't work try again without the energy
  return equalP4(delphesCand->P4(), getP4(edm4hepCand)) ||  \
    equalP4(delphesCand->P4(), getP4(edm4hepCand), 1e-5, false);
}

/**
 * Get all MCParticles related to a given ReconstructedParticle
 *
 */
std::vector<edm4hep::ConstMCParticle>
getAssociatedMCParticles(edm4hep::ConstReconstructedParticle reco,
                         const edm4hep::MCRecoParticleAssociationCollection& associations) {
  std::vector<edm4hep::ConstMCParticle> sims;
  // NOTE: looping over the whole collection of associations here is super
  // inefficient, but as long as there is no utility for this, implementing the
  // necessary caching is just too much work for this test case here
  for (const auto assoc : associations) {
    if (assoc.getRec() == reco) {
      sims.emplace_back(assoc.getSim());
    }
  }

  return sims;
}

template<typename DelphesT>
std::vector<GenParticle*> getAssociatedMCParticles(const DelphesT* delphesCand) {
  if constexpr(std::is_same_v<DelphesT, Muon> ||
               std::is_same_v<DelphesT, Electron> ||
               std::is_same_v<DelphesT, Track>) {
    return {static_cast<GenParticle*>(delphesCand->Particle.GetObject())};
  } else {
    const auto& refArray = delphesCand->Particles;
    std::vector<GenParticle*> genParts;
    genParts.reserve(refArray.GetEntries());
    for (int i = 0; i < refArray.GetEntries(); ++i) {
      genParts.push_back(static_cast<GenParticle*>(refArray.At(i)));
    }
    return genParts;
  }
}


/**
 * Check if the delphes and the edm4hep candidate point to the same respective
 * MCParticle(s)
 */
template<typename DelphesT>
bool compareMCRelations(const DelphesT* delphesCand,
                        edm4hep::ConstReconstructedParticle edm4hepCand,
                        const edm4hep::MCRecoParticleAssociationCollection& associations) {

  const auto delphesGenParticles = getAssociatedMCParticles(delphesCand);
  const auto edm4hepMCParticles = getAssociatedMCParticles(edm4hepCand, associations);

  if (delphesGenParticles.size() != edm4hepMCParticles.size()) {
    return false;
  }

  for (size_t i = 0; i < delphesGenParticles.size(); ++i) {
    if (!compareKinematics(delphesGenParticles[i], edm4hepMCParticles[i])) {
      return false;
    }
  }

  return true;
}


/**
 * Compare some basic properties of delphes and edm4hep collections that do not
 * depend on any information about what is stored in these collections
 */
void compareCollectionsBasic(const TClonesArray* delphesColl, const podio::CollectionBase& edm4hepColl, const std::string collName) {
  if (delphesColl->GetEntries() != edm4hepColl.size()) {
    std::cerr << "Sizes of collection \'" << collName << "\' disagree: "
              << delphesColl->GetEntries() << " vs " <<  edm4hepColl.size() << std::endl;
    std::exit(1);
  }
}

/**
 * Get the expected parent / daughter indices
 * See http://home.thep.lu.se/~torbjorn/pythia81html/ParticleProperties.html
 * for a few more details about the different cases we check here
 */
std::vector<int> expectedDaughtersParents(int index1, int index2) {
  // if the first index is -1, then we don't even look at the second one
  if (index1 == -1) {
    return {};
  }
  // if the first is not -1, but the second is, then we only have one parent/daughter
  if (index2 == -1) {
    return {index1};
  }
  // if the second one is > -1, we have to identify cases, where we have two parents/daughters
  // or an arbitrary number > 1
  if (index2 > -1) {
    if (index2 < index1) {
      return {index1, index2};
    } else {
      std::vector<int> indices;
      for (int i = index1; i <= index2; ++i) indices.push_back(i);
      return indices;
    }
  }
  return {};
}

/**
 * Compare the elements of the MCParticle collections element-wise
 */
void compareCollectionElements(const TClonesArray* delphesColl,
                               const edm4hep::MCParticleCollection& edm4hepColl,
                               const std::string collName) {
  for (int i = 0; i < delphesColl->GetEntries(); ++i) {
    const auto* delphesCand = static_cast<GenParticle*>(delphesColl->At(i));
    const auto edm4hepCand = edm4hepColl[i];
    if (!compareKinematics(delphesCand, edm4hepCand)) {
      std::cerr << "Delphes candidate " << i << " has different kinematics than edm4hep candidate in collection \'" << collName << "\'" << std::endl;
      std::exit(1);
    }

    const auto expParents = expectedDaughtersParents(delphesCand->M1, delphesCand->M2);

    if (expParents.size() != edm4hepCand.getParents().size()) {
      std::cerr << "Number of parents differs for particle " << i << " in collection \'"
                << collName << "\': " << expParents.size() << " vs " << edm4hepCand.getParents().size() << std::endl;
      std::exit(1);
    }

    const auto expDaughters = expectedDaughtersParents(delphesCand->D1, delphesCand->D2);

    if (expDaughters.size() != edm4hepCand.getDaughters().size()) {
      std::cerr << "Number of daughters differs for particle " << i << " in collection \'"
                << collName << "\': " << expDaughters.size() << " vs " << edm4hepCand.getDaughters().size() << std::endl;
      std::exit(1);
    }

    // compare the parents
    int iParent = 0;
    for (const auto iM : expParents) {
      if (!compareKinematics(static_cast<GenParticle*>(delphesColl->At(iM)),
                             edm4hepCand.getParents(iParent))) {
        std::cerr << "Parent " << iParent << " of particle " << i << " differs between delphes and edm4hep output" << std::endl;
        std::exit(1);
      }
      iParent++;
    }
   
    // comapre the daughters
    int iDaughter = 0;
    for (const auto iD: expDaughters) {
      if (!compareKinematics(static_cast<GenParticle*>(delphesColl->At(iD)),
                             edm4hepCand.getDaughters(iDaughter))) {
        std::cerr << "Daughter " << iDaughter << " of particle " << i << " differs between delphes and edm4hep output" << std::endl;
        std::exit(1);
      }
      iDaughter++;
    }
  }
}

/**
 * Compare the collections that are stored as RecoParticleRef in edm4hep (Muon,
 * Electron, Photon) element by element
 */
template<typename DelphesT>
void compareCollectionElements(const TClonesArray* delphesColl,
                               const edm4hep::RecoParticleRefCollection& edm4hepColl,
                               const std::string collName) {
  for (int i = 0; i < delphesColl->GetEntries(); ++i) {
    const auto* delphesCand = static_cast<DelphesT*>(delphesColl->At(i));
    const auto edm4hepCand = edm4hepColl[i].getParticle();
    if (!compareKinematics(delphesCand, edm4hepCand)) {
      std::cerr << "Delphes candidate " << i << " has different kinematics than edm4hep candidate in collection \'" << collName << "\'" << std::endl;
      std::exit(1);
    }

    // Photons have no charge, so nothing to compare here
    if constexpr (!std::is_same_v<DelphesT, Photon>) {
      if (delphesCand->Charge != edm4hepCand.getCharge()) {
        std::cerr << "Delphes candidate " << i << " has different charge than edm4hep candidate in collection \'" << collName << "\'" << std::endl;
        std::exit(1);
      }
    }
  }
}

/**
 * Compare the reconstructed particles that go into the global collection in
 * edm4hep, but are stored in several collections in delphes. startIndex is the
 * first element in the global collection that should be used for comparison.
 * Here we also check that we have the same MCParticles associated to the
 * ReconstructedParticles as delphes has. Since we go over all the
 * ReconstructedParticles there are in here, it should be enough to do this here
 * and not for other collections (e.g. Jet and RecoParticlRefs) again.
 */
template<typename DelphesT>
void compareCollectionElements(const TClonesArray* delphesColl,
                               const edm4hep::ReconstructedParticleCollection& edm4hepColl,
                               const std::string collName,
                               const int startIndex,
                               const edm4hep::MCRecoParticleAssociationCollection& associations) {
  for (int i = 0; i < delphesColl->GetEntries(); ++i) {
    const auto* delphesCand = static_cast<DelphesT*>(delphesColl->At(i));
    const auto edm4hepCand = edm4hepColl[i + startIndex];
    if (!compareKinematics(delphesCand, edm4hepCand)) {
      std::cerr << "Delphes candidate " << i << " has different kinematics than edm4hep candidate " << i + startIndex << " in collection \'" << collName << "\'" << std::endl;
      std::exit(1);
    }

    if (!compareMCRelations(delphesCand, edm4hepCand, associations)) {
      std::cerr << "MC relations of candidate " << i << " are different between delphes and edm4hep output" << std::endl;
      std::exit(1);
    }

    // Towers / clusters have no charge, so nothing to compare here
    if constexpr (!std::is_same_v<DelphesT, Tower>) {
      if (delphesCand->Charge != edm4hepCand.getCharge()) {
        std::cerr << "Delphes candidate " << i << " has different charge than edm4hep candidate in collection \'" << collName << "\'" << std::endl;
        std::exit(1);
      }
    }
  }
}

/**
 * Compare the jet collections by first checking the jet kinematics and then
 * also verifying that the jet constituents are the same in delphes and in
 * edm4hep
 */
void compareJets(const TClonesArray* delphesColl,
                 const edm4hep::ReconstructedParticleCollection& edm4hepColl,
                 const std::string collName) {
  for (int i = 0; i < delphesColl->GetEntries(); ++i) {
    const auto* delphesCand = static_cast<Jet*>(delphesColl->At(i));
    const auto edm4hepCand = edm4hepColl[i];
    if (!compareKinematics(delphesCand, edm4hepCand)) {
      std::cerr << "Delphes candidate " << i << " has different kinematics than edm4hep candidate in collection \'" << collName << "\'" << std::endl;
      std::exit(1);
    }

    if (delphesCand->Constituents.GetEntries() != edm4hepCand.getParticles().size()) {
      std::cerr << "Number of Jet constitutents in Jet " << i << " differs between delphes and edm4hep output: "
                << delphesCand->Constituents.GetEntries() << " vs " <<  edm4hepCand.getParticles().size() << std::endl;
      std::exit(1);
    }

    for (int j = 0; j < delphesCand->Constituents.GetEntries(); ++j) {
      bool OK = false;
      // Just to be sure we handle Tracks and Towers correctly, we explicitly
      // cast them here before comparing them to the edm4hep output
      if (delphesCand->Constituents.At(j)->ClassName() == std::string("Track")) {
        OK = compareKinematics(static_cast<Track*>(delphesCand->Constituents.At(j)), edm4hepCand.getParticles(j));
      } else {
        OK = compareKinematics(static_cast<Tower*>(delphesCand->Constituents.At(j)), edm4hepCand.getParticles(j));
      }

      if (!OK) {
        std::cerr << "Jet constituent " << j << " has different kinematics in delphes and in edm4hep "
          << " in Jet " << i << " in collection \'" << collName << "\'" << std::endl;
        std::exit(1);
      }
    }
  }
}

void compareMET(const TClonesArray* delphesColl,
                const edm4hep::ReconstructedParticleCollection& edm4hepColl) {
  const auto delphesMET = static_cast<MissingET*>(delphesColl->At(0));
  const auto edm4hepMET = edm4hepColl[0];
  if (!compareKinematics(delphesMET, edm4hepMET)) {
    std::cerr << "MET differs between delphes and edm4hep output" << std::endl;
    std::exit(1);
  }
}



int main(int argc, char* argv[]) {
  // do the necessary setup work for podio and delphes first
  podio::ROOTReader reader{};
  reader.openFile(argv[1]);
  podio::EventStore store{};
  store.setReader(&reader);

  auto chain = std::make_unique<TChain>("Delphes");
  chain->Add(argv[2]);
  auto treeReader = std::make_unique<ExRootTreeReader>(chain.get());

  auto* genParticleCollDelphes = treeReader->UseBranch("Particle");
  auto* electronCollDelphes = treeReader->UseBranch("Electron");
  auto* muonCollDelphes = treeReader->UseBranch("Muon");
  auto* photonCollDelphes = treeReader->UseBranch("Photon");
  auto* tracks = treeReader->UseBranch("EFlowTrack");
  auto* ecalClusters = treeReader->UseBranch("EFlowPhoton");
  auto* hcalClusters = treeReader->UseBranch("EFlowNeutralHadron");
  auto* delphesJetColl = treeReader->UseBranch("Jet");
  auto* delphesMET = treeReader->UseBranch("MissingET");

  // now start comparing the contents of the files
  const auto nEntries = treeReader->GetEntries();
  if (nEntries != reader.getEntries()) {
    std::cerr << "Number of events in delphes and edm4hep outputfile do not agree: "
              << nEntries << " vs " << reader.getEntries() << std::endl;
    return 1;
  }

  for (int entry = 0; entry < nEntries; ++entry) {
    treeReader->ReadEntry(entry);

    std::cout << "EVENT: " << entry << std::endl;

    auto& genParticleColl = store.get<edm4hep::MCParticleCollection>("Particle");
    compareCollectionsBasic(genParticleCollDelphes, genParticleColl, "Particle");
    compareCollectionElements(genParticleCollDelphes, genParticleColl, "Particle");

    auto& electronColl = store.get<edm4hep::RecoParticleRefCollection>("Electron");
    compareCollectionsBasic(electronCollDelphes, electronColl, "Electron");
    compareCollectionElements<Electron>(electronCollDelphes, electronColl, "Electron");

    auto& muonColl = store.get<edm4hep::RecoParticleRefCollection>("Muon");
    compareCollectionsBasic(muonCollDelphes, muonColl, "Muon");
    compareCollectionElements<Muon>(muonCollDelphes, muonColl, "Muon");

    auto& photonColl = store.get<edm4hep::RecoParticleRefCollection>("Photon");
    compareCollectionsBasic(photonCollDelphes, photonColl, "Photon");
    compareCollectionElements<Photon>(photonCollDelphes, photonColl, "Photon");

    auto& recoColl = store.get<edm4hep::ReconstructedParticleCollection>("ReconstructedParticles");
    const int nRecos = tracks->GetEntries() + ecalClusters->GetEntries() + hcalClusters->GetEntries();

    if (nRecos != recoColl.size()) {
      std::cerr << "The global ReconstructedParticle collection and the original delphes collections differ in size: "
                << recoColl.size() << " vs " << nRecos << std::endl;
      return 1;
    }

    auto& mcRecoAssocColl = store.get<edm4hep::MCRecoParticleAssociationCollection>("MCRecoAssociations");

    compareCollectionElements<Track>(tracks, recoColl, "EFlowTrack", 0, mcRecoAssocColl);
    compareCollectionElements<Tower>(ecalClusters, recoColl, "EFlowPhoton", tracks->GetEntries(), mcRecoAssocColl);
    compareCollectionElements<Tower>(hcalClusters, recoColl, "EFlowNeutralHadron", tracks->GetEntries() + ecalClusters->GetEntries(), mcRecoAssocColl);

    auto& jetColl = store.get<edm4hep::ReconstructedParticleCollection>("Jet");
    compareCollectionsBasic(delphesJetColl, jetColl, "Jet");
    compareJets(delphesJetColl, jetColl, "Jet");

    auto& metColl = store.get<edm4hep::ReconstructedParticleCollection>("MissingET");
    compareCollectionsBasic(delphesMET, metColl, "MissingET");
    compareMET(delphesMET, metColl);

    store.clear();
    reader.endOfEvent();
  }

  return 0;
}
