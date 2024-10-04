#include "delphesHelpers.h"

#include "edm4hep/MCParticleCollection.h"
#include "edm4hep/RecoMCParticleLinkCollection.h"
#include "edm4hep/ReconstructedParticleCollection.h"
#include "edm4hep/utils/kinematics.h"

#include "podio/Frame.h"
#include "podio/ROOTReader.h"

#include "ExRootAnalysis/ExRootTreeBranch.h"
#include "ExRootAnalysis/ExRootTreeReader.h"
#include "classes/DelphesClasses.h"

#include "Math/Vector4D.h"
#include "TChain.h"
#include "TClonesArray.h"
#include "TLorentzVector.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <numeric>
#include <vector>

/**
 * Compare the 4-vectors of the delphes and the edm4hep candidate
 */
template <typename DelphesT, typename EDM4HepT>
bool compareKinematics(const DelphesT* delphesCand, const EDM4HepT& edm4hepCand) {
  using namespace k4SimDelphes;
  // Use the same matching criteria as in the converter: First try with all
  // components, if that doesn't work try again without the energy
  return equalP4(delphesCand->P4(), getP4(edm4hepCand)) || equalP4(delphesCand->P4(), getP4(edm4hepCand), 1e-5, false);
}

/**
 * The default error message for mismatched kinematics
 */
std::string stdErrorMessage(const std::string& collName, const int index) {
  return std::string("Delphes and edm4hep candidate ") + std::to_string(index) + std::string(" in collection \'") +
         collName + std::string("\' have different kinematics");
}

/**
 * Assert that the delphes and edm4hep candidate have the same kinematics
 * (approximately) and terminate if they do not. Print the message that is
 * returned by the message func when passed the msgArgs
 */
template <typename DelphesT, typename EDM4HepT, typename MsgF, typename... MsgArgs>
void assertSameKinematics(const DelphesT* delphesCand, const EDM4HepT& edm4hepCand, MsgF msgF, MsgArgs&&... msgArgs) {
  if (!compareKinematics(delphesCand, edm4hepCand)) {
    const auto& p4 = delphesCand->P4();
    std::cerr << msgF(std::forward<MsgArgs>(msgArgs)...) << ": "
              << "(" << p4.Px() << ", " << p4.Py() << ", " << p4.Pz() << ", " << p4.E() << ")"
              << " vs " << k4SimDelphes::getP4(edm4hepCand) << std::endl;
    std::exit(1);
  }
}

/**
 * Get all MCParticles related to a given ReconstructedParticle
 *
 */
std::vector<edm4hep::MCParticle> getAssociatedMCParticles(edm4hep::ReconstructedParticle               reco,
                                                          const edm4hep::RecoMCParticleLinkCollection& associations) {
  std::vector<edm4hep::MCParticle> sims;
  // NOTE: looping over the whole collection of associations here is super
  // inefficient, but as long as there is no utility for this, implementing the
  // necessary caching is just too much work for this test case here
  for (const auto assoc : associations) {
    if (assoc.getFrom() == reco) {
      sims.emplace_back(assoc.getTo());
    }
  }

  return sims;
}

template <typename DelphesT> std::vector<GenParticle*> getAssociatedMCParticles(const DelphesT* delphesCand) {
  if constexpr (std::is_same_v<DelphesT, Muon> || std::is_same_v<DelphesT, Electron> ||
                std::is_same_v<DelphesT, Track>) {
    return {static_cast<GenParticle*>(delphesCand->Particle.GetObject())};
  } else {
    const auto&               refArray = delphesCand->Particles;
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
template <typename DelphesT>
bool compareMCRelations(const DelphesT* delphesCand, edm4hep::ReconstructedParticle edm4hepCand,
                        const edm4hep::RecoMCParticleLinkCollection& associations) {
  auto delphesGenParticles = getAssociatedMCParticles(delphesCand);
  auto edm4hepMCParticles  = getAssociatedMCParticles(edm4hepCand, associations);

  if (delphesGenParticles.size() != edm4hepMCParticles.size()) {
    return false;
  }

  // In some cases the order of the vectors is not exactly the same, because
  // Delphes(!) seems to write them in a different order from time to time even
  // with equal random seeds. Here we sort them simply by the x coordinate of
  // the momentum since that is easiest to do without having to wade into the
  // subtleties of potentially small differences in energy in a four vector
  // depending on whether energy is stored directly or whether we compute it via
  // the momentum and the mass.
  std::sort(delphesGenParticles.begin(), delphesGenParticles.end(),
            [](const auto* a, const auto* b) { return a->P4().X() < b->P4().X(); });
  std::sort(edm4hepMCParticles.begin(), edm4hepMCParticles.end(),
            [](const auto& a, const auto& b) { return a.getMomentum().x < b.getMomentum().x; });

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
void compareCollectionsBasic(const TClonesArray* delphesColl, const podio::CollectionBase& edm4hepColl,
                             const std::string collName) {
  if (delphesColl->GetEntries() != static_cast<int>(edm4hepColl.size())) {
    std::cerr << "Sizes of collection \'" << collName << "\' disagree: " << delphesColl->GetEntries() << " vs "
              << edm4hepColl.size() << std::endl;
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
      for (int i = index1; i <= index2; ++i)
        indices.push_back(i);
      return indices;
    }
  }
  return {};
}

/**
 * Compare the elements of the MCParticle collections element-wise
 */
void compareCollectionElements(const TClonesArray* delphesColl, const edm4hep::MCParticleCollection& edm4hepColl,
                               const std::string& collName) {
  for (int i = 0; i < delphesColl->GetEntries(); ++i) {
    const auto* delphesCand = static_cast<GenParticle*>(delphesColl->At(i));
    const auto  edm4hepCand = edm4hepColl[i];
    assertSameKinematics(delphesCand, edm4hepCand, stdErrorMessage, collName, i);

    const auto expParents = expectedDaughtersParents(delphesCand->M1, delphesCand->M2);

    if (expParents.size() != edm4hepCand.getParents().size()) {
      std::cerr << "Number of parents differs for particle " << i << " in collection \'" << collName
                << "\': " << expParents.size() << " vs " << edm4hepCand.getParents().size() << std::endl;
      std::exit(1);
    }

    const auto expDaughters = expectedDaughtersParents(delphesCand->D1, delphesCand->D2);

    if (expDaughters.size() != edm4hepCand.getDaughters().size()) {
      std::cerr << "Number of daughters differs for particle " << i << " in collection \'" << collName
                << "\': " << expDaughters.size() << " vs " << edm4hepCand.getDaughters().size() << std::endl;
      std::exit(1);
    }

    const auto assertMsg = [](const std::string& name, const int index, const int relIndex,
                              const std::string& relation) {
      return relation + std::to_string(relIndex) + " of particle " + std::to_string(index) +
             " differs between delphes and edm4hep output in collection \'" + name + "\'";
    };

    // compare the parents
    int iParent = 0;
    for (const auto iM : expParents) {
      assertSameKinematics(static_cast<GenParticle*>(delphesColl->At(iM)), edm4hepCand.getParents(iParent), assertMsg,
                           collName, i, iParent, "Parent ");
      iParent++;
    }

    // comapre the daughters
    int iDaughter = 0;
    for (const auto iD : expDaughters) {
      assertSameKinematics(static_cast<GenParticle*>(delphesColl->At(iD)), edm4hepCand.getDaughters(iDaughter),
                           assertMsg, collName, i, iDaughter, "Daughter ");
      iDaughter++;
    }
  }
}

/**
 * Compare the collections that are stored as RecoParticleRef in edm4hep (Muon,
 * Electron, Photon) element by element
 */
template <typename DelphesT>
void compareCollectionElements(const TClonesArray*                             delphesColl,
                               const edm4hep::ReconstructedParticleCollection& edm4hepColl,
                               const std::string                               collName) {
  for (int i = 0; i < delphesColl->GetEntries(); ++i) {
    const auto* delphesCand = static_cast<DelphesT*>(delphesColl->At(i));
    const auto  edm4hepCand = edm4hepColl[i];
    assertSameKinematics(delphesCand, edm4hepCand, stdErrorMessage, collName, i);

    // Photons have no charge, so nothing to compare here
    if constexpr (!std::is_same_v<DelphesT, Photon>) {
      if (delphesCand->Charge != edm4hepCand.getCharge()) {
        std::cerr << "Delphes candidate " << i << " has different charge than edm4hep candidate in collection \'"
                  << collName << "\'" << std::endl;
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
template <typename DelphesT>
void compareCollectionElements(const TClonesArray*                             delphesColl,
                               const edm4hep::ReconstructedParticleCollection& edm4hepColl, const std::string collName,
                               const int startIndex, const edm4hep::RecoMCParticleLinkCollection& associations) {
  for (int i = 0; i < delphesColl->GetEntries(); ++i) {
    const auto* delphesCand = static_cast<DelphesT*>(delphesColl->At(i));
    const auto  edm4hepCand = edm4hepColl[i + startIndex];
    assertSameKinematics(delphesCand, edm4hepCand, stdErrorMessage, collName, i);

    if (!compareMCRelations(delphesCand, edm4hepCand, associations)) {
      std::cerr << "MC relations of candidate " << i << " are different between delphes and edm4hep output"
                << std::endl;
      std::exit(1);
    }

    // Towers / clusters have no charge, so nothing to compare here
    if constexpr (!std::is_same_v<DelphesT, Tower>) {
      if (delphesCand->Charge != edm4hepCand.getCharge()) {
        std::cerr << "Delphes candidate " << i << " has different charge than edm4hep candidate in collection \'"
                  << collName << "\'" << std::endl;
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
void compareJets(const TClonesArray* delphesColl, const edm4hep::ReconstructedParticleCollection& edm4hepColl,
                 const std::string collName) {
  for (int i = 0; i < delphesColl->GetEntries(); ++i) {
    const auto* delphesCand = static_cast<Jet*>(delphesColl->At(i));
    const auto  edm4hepCand = edm4hepColl[i];
    assertSameKinematics(delphesCand, edm4hepCand, stdErrorMessage, collName, i);

    if (delphesCand->Constituents.GetEntries() != static_cast<int>(edm4hepCand.getParticles().size())) {
      std::cerr << "Number of Jet constitutents in Jet " << i
                << " differs between delphes and edm4hep output: " << delphesCand->Constituents.GetEntries() << " vs "
                << edm4hepCand.getParticles().size() << std::endl;
      std::exit(1);
    }

    const auto assertMsg = [](const std::string& name, const int index, const int iConst) {
      return std::string("Jet constituent ") + std::to_string(iConst) +
             " has different kinematics in delphes and in edm4hep in Jet " + std::to_string(index) +
             " in collection \'" + name + "\'";
    };

    for (int j = 0; j < delphesCand->Constituents.GetEntries(); ++j) {
      // Just to be sure we handle Tracks and Towers correctly, we explicitly
      // cast them here before comparing them to the edm4hep output
      if (delphesCand->Constituents.At(j)->ClassName() == std::string("Track")) {
        assertSameKinematics(static_cast<Track*>(delphesCand->Constituents.At(j)), edm4hepCand.getParticles(j),
                             assertMsg, collName, i, j);
      } else {
        assertSameKinematics(static_cast<Tower*>(delphesCand->Constituents.At(j)), edm4hepCand.getParticles(j),
                             assertMsg, collName, i, j);
      }
    }

    // Use the energy for comparisons here, since we set the mass for the tracks
    // (potentially differently to what delphes is doing)
    const auto edm4hepJetP4   = edm4hep::utils::p4(edm4hepCand, edm4hep::utils::UseEnergy);
    const auto edm4hepConstP4 = std::transform_reduce(
        edm4hepCand.getParticles().begin(), edm4hepCand.getParticles().end(), edm4hep::LorentzVectorE{}, std::plus{},
        [](const auto& cand) { return edm4hep::utils::p4(cand, edm4hep::utils::UseEnergy); });

    // Compare only the full 4 momenta here as this should be exact
    if (!k4SimDelphes::equalP4(edm4hepJetP4, edm4hepConstP4)) {
      std::cerr << "Sum of EDM4hep jet constituents 4-momenta is not Jet momentum for Jet " << i
                << " (sum(constituents): " << edm4hepConstP4 << ", jet: " << edm4hepJetP4 << ")" << std::endl;
      std::exit(1);
    }

    // TODO: Check the same for delphes?
  }
}

void compareMET(const TClonesArray* delphesColl, const edm4hep::ReconstructedParticleCollection& edm4hepColl) {
  const auto delphesMET = static_cast<MissingET*>(delphesColl->At(0));
  const auto edm4hepMET = edm4hepColl[0];
  assertSameKinematics(delphesMET, edm4hepMET, []() { return "MET differs between delphes and edm4hep output"; });
}

int main(int, char* argv[]) {
  // do the necessary setup work for podio and delphes first
  podio::ROOTReader reader{};
  reader.openFile(argv[1]);

  auto chain = std::make_unique<TChain>("Delphes");
  chain->Add(argv[2]);
  auto treeReader = std::make_unique<ExRootTreeReader>(chain.get());

  auto* genParticleCollDelphes = treeReader->UseBranch("Particle");
  auto* electronCollDelphes    = treeReader->UseBranch("Electron");
  auto* muonCollDelphes        = treeReader->UseBranch("Muon");
  auto* photonCollDelphes      = treeReader->UseBranch("Photon");
  auto* tracks                 = treeReader->UseBranch("EFlowTrack");
  auto* ecalClusters           = treeReader->UseBranch("EFlowPhoton");
  auto* hcalClusters           = treeReader->UseBranch("EFlowNeutralHadron");
  auto* delphesJetColl         = treeReader->UseBranch("Jet");
  auto* delphesMET             = treeReader->UseBranch("MissingET");

  // now start comparing the contents of the files
  const auto nEntries = treeReader->GetEntries();
  if (nEntries != reader.getEntries("events")) {
    std::cerr << "Number of events in delphes and edm4hep outputfile do not agree: " << nEntries << " vs "
              << reader.getEntries("events") << std::endl;
    return 1;
  }

  for (int entry = 0; entry < nEntries; ++entry) {
    treeReader->ReadEntry(entry);
    podio::Frame frame(reader.readNextEntry("events"));

    std::cout << "EVENT: " << entry << std::endl;

    auto& genParticleColl = frame.get<edm4hep::MCParticleCollection>("Particle");
    compareCollectionsBasic(genParticleCollDelphes, genParticleColl, "Particle");
    compareCollectionElements(genParticleCollDelphes, genParticleColl, "Particle");

    auto& electronColl = frame.get<edm4hep::ReconstructedParticleCollection>("Electron");
    compareCollectionsBasic(electronCollDelphes, electronColl, "Electron");
    compareCollectionElements<Electron>(electronCollDelphes, electronColl, "Electron");

    auto& muonColl = frame.get<edm4hep::ReconstructedParticleCollection>("Muon");
    compareCollectionsBasic(muonCollDelphes, muonColl, "Muon");
    compareCollectionElements<Muon>(muonCollDelphes, muonColl, "Muon");

    auto& photonColl = frame.get<edm4hep::ReconstructedParticleCollection>("Photon");
    compareCollectionsBasic(photonCollDelphes, photonColl, "Photon");
    compareCollectionElements<Photon>(photonCollDelphes, photonColl, "Photon");

    auto&          recoColl = frame.get<edm4hep::ReconstructedParticleCollection>("ReconstructedParticles");
    const unsigned nRecos   = tracks->GetEntries() + ecalClusters->GetEntries() + hcalClusters->GetEntries();

    if (nRecos != recoColl.size()) {
      std::cerr << "The global ReconstructedParticle collection and the original delphes collections differ in size: "
                << recoColl.size() << " vs " << nRecos << std::endl;
      return 1;
    }

    auto& mcRecoAssocColl = frame.get<edm4hep::RecoMCParticleLinkCollection>("MCRecoAssociations");

    compareCollectionElements<Track>(tracks, recoColl, "EFlowTrack", 0, mcRecoAssocColl);
    compareCollectionElements<Tower>(ecalClusters, recoColl, "EFlowPhoton", tracks->GetEntries(), mcRecoAssocColl);
    compareCollectionElements<Tower>(hcalClusters, recoColl, "EFlowNeutralHadron",
                                     tracks->GetEntries() + ecalClusters->GetEntries(), mcRecoAssocColl);

    auto& jetColl = frame.get<edm4hep::ReconstructedParticleCollection>("Jet");
    compareCollectionsBasic(delphesJetColl, jetColl, "Jet");
    compareJets(delphesJetColl, jetColl, "Jet");

    auto& metColl = frame.get<edm4hep::ReconstructedParticleCollection>("MissingET");
    compareCollectionsBasic(delphesMET, metColl, "MissingET");
    compareMET(delphesMET, metColl);
  }

  return 0;
}
