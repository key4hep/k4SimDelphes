#include "k4SimDelphes/DelphesEDM4HepConverter.h"
#include "k4SimDelphes/DelphesEDM4HepOutputConfiguration.h"
#include "delphesHelpers.h" // getAllParticleIds

#include "edm4hep/ReconstructedParticleCollection.h"
#include "edm4hep/MCParticleCollection.h"
#include "edm4hep/TrackCollection.h"
#include "edm4hep/ClusterCollection.h"
#include "edm4hep/MCRecoParticleAssociationCollection.h"
#include "edm4hep/RecoParticleRefCollection.h"
#include "edm4hep/ParticleIDCollection.h"

#include "classes/DelphesClasses.h"

#include <iostream>
#include <iterator>
#include <set>
#include <TMatrixDSym.h>

namespace k4SimDelphes {

// TODO: Take these from HepPDT / HepMC?
constexpr double M_PIPLUS = 0.13957039; // GeV (PDG 2020)
constexpr double M_MU = 0.1056583745; // GeV (PDG 2020)
constexpr double M_ELECTRON = 0.5109989461e-3; // GeV (PDG 2020)

// TODO: Make configurable?
constexpr double trackMass = M_PIPLUS;

/**
 * Order in which the different delphes output classes will be processed.
 * Everything not defined here will not be processed.
 *
 * NOTE: not a configuration parameter. this has to be done in this order to
 * ensure that products required by later stages are producd early enough
 */
constexpr std::array<std::string_view, 9> PROCESSING_ORDER = {
  "GenParticle",
  "Track",
  "Tower",
  "Muon",
  "Electron",
  "Photon",
  "Jet",
  "MissingET",
  "SclalarHT"
};

template<size_t N>
void sortBranchesProcessingOrder(std::vector<BranchSettings>& branches,
                                 std::array<std::string_view, N> const& processingOrder);

edm4hep::Track convertTrack(Track const* cand, const double magFieldBz);

void setMotherDaughterRelations(GenParticle const* delphesCand,
                                edm4hep::MCParticle particle,
                                edm4hep::MCParticleCollection& mcParticles);

/**
 * Simple helper function to make it easier to refactor later
 */
template<typename Container>
inline bool contains(Container const& container, typename Container::value_type const& value)
{
  return std::find(container.cbegin(), container.cend(), value) != container.cend();
}


DelphesEDM4HepConverter::DelphesEDM4HepConverter(std::string filename_delphescard) {
  auto confReader = std::make_unique<ExRootConfReader>();
  confReader->ReadFile(filename_delphescard.c_str());
  const auto branches = getBranchSettings(confReader->GetParam("TreeWriter::Branch"));
  const auto edm4hepOutputSettings = OutputSettings();
  DelphesEDM4HepConverter edm4hepConverter(branches,
                                           edm4hepOutputSettings,
                                           confReader->GetDouble("ParticlePropagator::Bz", 0));
};

DelphesEDM4HepConverter::DelphesEDM4HepConverter(const std::vector<BranchSettings>& branches,
                                                 OutputSettings const& outputSettings, double magFieldBz) :
  m_magneticFieldBz(magFieldBz),
  m_recoCollName(outputSettings.RecoParticleCollectionName),
  m_particleIDName(outputSettings.ParticleIDCollectionName),
  m_mcRecoAssocCollName(outputSettings.MCRecoAssociationCollectionName)
{
  for (const auto& branch : branches) {
    if (contains(PROCESSING_ORDER, branch.className)) {
      m_branches.push_back(branch);
    }

  }

  sortBranchesProcessingOrder(m_branches, PROCESSING_ORDER);


  const std::unordered_map<std::string, ProcessFunction> refProcessFunctions = {
    {"Photon", &DelphesEDM4HepConverter::processPhotons},
    {"Muon", &DelphesEDM4HepConverter::processMuons},
    {"Electron", &DelphesEDM4HepConverter::processElectrons}};

  for (const auto& branch : m_branches) {
    if (contains(outputSettings.GenParticleCollections, branch.name.c_str())) {
      createCollection<edm4hep::MCParticleCollection>(branch.name);
      m_processFunctions.emplace(branch.name, &DelphesEDM4HepConverter::processParticles);
    }

    if (contains(outputSettings.ReconstructedParticleCollections, branch.name.c_str()) &&
        contains(RECO_TRACK_OUTPUT, branch.className.c_str())) {
      registerGlobalCollections();
      createCollection<edm4hep::TrackCollection>(branch.name);
      m_processFunctions.emplace(branch.name, &DelphesEDM4HepConverter::processTracks);
    }

    if (contains(outputSettings.ReconstructedParticleCollections, branch.name.c_str()) &&
        contains(RECO_CLUSTER_OUTPUT, branch.className.c_str())) {
      registerGlobalCollections();
      createCollection<edm4hep::ClusterCollection>(branch.name);
      m_processFunctions.emplace(branch.name, &DelphesEDM4HepConverter::processClusters);
    }

    if (contains(outputSettings.JetCollections, branch.name.c_str())) {
      createCollection<edm4hep::ReconstructedParticleCollection>(branch.name);
      m_processFunctions.emplace(branch.name, &DelphesEDM4HepConverter::processJets);
    }

    if (contains(outputSettings.MuonCollections, branch.name.c_str()) ||
        contains(outputSettings.ElectronCollections, branch.name.c_str()) ||
        contains(outputSettings.PhotonCollections, branch.name.c_str())) {
      createCollection<edm4hep::RecoParticleRefCollection>(branch.name);
      m_processFunctions.emplace(branch.name, refProcessFunctions.at(branch.className));
    }

    if (contains(outputSettings.MissingETCollections, branch.name.c_str())) {
      createCollection<edm4hep::ReconstructedParticleCollection>(branch.name);
      m_processFunctions.emplace(branch.name, &DelphesEDM4HepConverter::processMissingET);
    }

    if (contains(outputSettings.ScalarHTCollections, branch.name.c_str())) {
      createCollection<edm4hep::ParticleIDCollection>(branch.name);
      m_processFunctions.emplace(branch.name, &DelphesEDM4HepConverter::processScalarHT);
    }
  }
}

void DelphesEDM4HepConverter::process(TTree* delphesTree) {
  // beginning of processing: clear previous event from containers
  for (auto& coll : m_collections) {
   coll.second->clear();
  }

  for (const auto& branch : m_branches) {
    // at this point it is not guaranteed that all entries in branch (which follow
    // the input from the delphes card) are also present in the processing
    // functions. Checking this here, basically allows us to skip these checks
    // in all called processing functions, since whatever is accessed there will
    // also be in the collection map, since that is filled with the same keys as
    // the processing function map
    const auto processFuncIt = m_processFunctions.find(branch.name);
    // Also check whether the desired input is actually in the branch
    auto* rootBranch = delphesTree->GetBranch(branch.name.c_str());
    if (processFuncIt != m_processFunctions.end() && rootBranch) {
      auto* delphesCollection = *(TClonesArray**) rootBranch->GetAddress();
      (this->*processFuncIt->second)(delphesCollection, branch.name);
    }
  }

  // Clear the internal maps that hold references to entites that have been put
  // into maps here for internal use only (see #89)
  m_genParticleIds.clear();
  m_recoParticleGenIds.clear();
}

void DelphesEDM4HepConverter::processParticles(const TClonesArray* delphesCollection, std::string_view const branch)
{
  auto* collection = static_cast<edm4hep::MCParticleCollection*>(m_collections[branch]);
  for (int iCand = 0; iCand < delphesCollection->GetEntries(); ++iCand) {
    auto* delphesCand = static_cast<GenParticle*>(delphesCollection->At(iCand));

    auto cand = collection->create();
    cand.setCharge(delphesCand->Charge);
    cand.setMass(delphesCand->Mass);
    cand.setMomentum({delphesCand->Px, delphesCand->Py, delphesCand->Pz});
    cand.setVertex({delphesCand->X, delphesCand->Y, delphesCand->Z});
    cand.setPDG(delphesCand->PID); // delphes uses whatever hepevt.idhep provides
    cand.setGeneratorStatus(delphesCand->Status);

    if (const auto [it, inserted] = m_genParticleIds.emplace(delphesCand->GetUniqueID(), cand); !inserted) {
      std::cerr << "**** WARNING: UniqueID " << delphesCand->GetUniqueID() << " is already used by MCParticle with id: " << it->second.id() << std::endl;
    }
  }

  // mother-daughter relations
  const auto nElements = collection->size();
  for (int iCand = 0; iCand < delphesCollection->GetEntries(); ++iCand) {
    const auto* delphesCand = static_cast<GenParticle*>(delphesCollection->At(iCand));
    auto cand = collection->at(iCand);

    setMotherDaughterRelations(delphesCand, cand, *collection);
  }

}


void DelphesEDM4HepConverter::processTracks(const TClonesArray* delphesCollection, std::string_view const branch)
{
  auto* particleCollection = static_cast<edm4hep::ReconstructedParticleCollection*>(m_collections[m_recoCollName]);
  auto* trackCollection = static_cast<edm4hep::TrackCollection*>(m_collections[branch]);
  auto* mcRecoRelations = static_cast<edm4hep::MCRecoParticleAssociationCollection*>(m_collections[m_mcRecoAssocCollName]);
  auto* idCollection = static_cast<edm4hep::ParticleIDCollection*>(m_collections[m_particleIDName]);

  for (auto iCand = 0; iCand < delphesCollection->GetEntries(); ++iCand) {
    auto* delphesCand = static_cast<Track*>(delphesCollection->At(iCand));

    auto track = convertTrack(delphesCand, m_magneticFieldBz);
    trackCollection->push_back(track);

    auto id = idCollection->create();

    auto cand = particleCollection->create();
    cand.setCharge(delphesCand->Charge);
    const auto momentum = delphesCand->P4();
    cand.setEnergy(momentum.E());
    cand.setMomentum({(float) momentum.Px(), (float) momentum.Py(), (float) momentum.Pz()});
    // At this point indiscriminantly set the mass for each track. If this is a
    // muon or an electron, the mass will be set to the appropriate value later.
    cand.setMass(trackMass);

    cand.addToTracks(track);

    // id.addToParameters(delphesCand->IsolationVar);
    cand.addToParticleIDs(id);

    UInt_t genId = delphesCand->Particle.GetUniqueID();
    if (const auto genIt = m_genParticleIds.find(genId); genIt != m_genParticleIds.end()) {
      auto relation = mcRecoRelations->create();
      relation.setSim(genIt->second);
      relation.setRec(cand);
    }

    m_recoParticleGenIds.emplace(genId, cand);
  }
}

void DelphesEDM4HepConverter::processClusters(const TClonesArray* delphesCollection, std::string_view const branch)
{
  auto* particleCollection = static_cast<edm4hep::ReconstructedParticleCollection*>(m_collections[m_recoCollName]);
  auto* clusterCollection = static_cast<edm4hep::ClusterCollection*>(m_collections[branch]);
  auto* mcRecoRelations = static_cast<edm4hep::MCRecoParticleAssociationCollection*>(m_collections[m_mcRecoAssocCollName]);

  for (auto iCand = 0; iCand < delphesCollection->GetEntries(); ++iCand) {
    auto* delphesCand = static_cast<Tower*>(delphesCollection->At(iCand));

    auto cluster = clusterCollection->create();
    cluster.setEnergy(delphesCand->E);
    // TODO: how to determine position from a Tower instead of a Candidate? Does
    // it make sense to define this for a cluster? Can we get enough info from
    // Delphes?
    // cluster.setPosition({(float) delphesCand->Position.X(),
    //                      (float) delphesCand->Position.Y(),
    //                      (float) delphesCand->Position.Z()});
    // TODO: time? (could be stored in a CalorimeterHit)
    // TODO: mc relations? would definitely need a CalorimeterHit for that
    //
    // TODO: Potentially every delphes tower could be split into two
    // edm4hep::clusters, with energies split according to Eem and Ehad. But
    // that would probably make the matching that is done below much harder

    auto cand = particleCollection->create();
    const auto momentum = delphesCand->P4(); // NOTE: assuming massless here!
    // TODO: fill this when it is available later, when when we link the references?
    // cand.setCharge(delphesCand->Charge);
    cand.setMomentum({(float) momentum.Px(), (float) momentum.Py(), (float) momentum.Pz()});
    cand.setEnergy(delphesCand->E);

    cand.addToClusters(cluster);

    for (const auto genId : getAllParticleIDs(delphesCand)) {
      if (const auto genIt = m_genParticleIds.find(genId); genIt != m_genParticleIds.end()) {
        auto relation = mcRecoRelations->create();
        relation.setSim(genIt->second);
        relation.setRec(cand);
      }

      m_recoParticleGenIds.emplace(genId, cand);
    }

  }
}

void DelphesEDM4HepConverter::processJets(const TClonesArray* delphesCollection, std::string_view const branch)
{
  auto* jetCollection = static_cast<edm4hep::ReconstructedParticleCollection*>(m_collections[branch]);
  auto* idCollection = static_cast<edm4hep::ParticleIDCollection*>(m_collections[m_particleIDName]);

  for (auto iCand = 0; iCand < delphesCollection->GetEntries(); ++iCand) {
    auto* delphesCand = static_cast<Jet*>(delphesCollection->At(iCand));
    auto jet = jetCollection->create();
    auto id = idCollection->create();

    // NOTE: Filling the jet with the information delievered by Delphes, which
    // is not necessarily the same as the sum of its constituents (filled below)
    jet.setCharge(delphesCand->Charge);
    jet.setMass(delphesCand->Mass);
    const auto momentum = delphesCand->P4();
    jet.setEnergy(momentum.E());
    jet.setMomentum({(float) momentum.Px(), (float) momentum.Py(), (float) momentum.Pz()});

    // id.addToParameters(delphesCand->IsolationVar);
    id.addToParameters(delphesCand->BTag);
    id.addToParameters(delphesCand->TauTag);
    jet.addToParticleIDs(id);

    const auto& constituents = delphesCand->Constituents;
    for (auto iConst = 0; iConst < constituents.GetEntries(); ++iConst) {
      // TODO: Can we do better than Candidate here?
      auto* constituent = static_cast<Candidate*>(constituents.At(iConst));
      if (auto matchedReco = getMatchingReco(constituent)) {
        jet.addToParticles(*matchedReco);
      } else {
        std::cerr << "**** WARNING: No matching ReconstructedParticle was found for a Jet constituent" << std::endl;
      }
    }
  }
}

template<typename DelphesT>
void DelphesEDM4HepConverter::fillReferenceCollection(const TClonesArray* delphesCollection, std::string_view const branch, std::string_view const type)
{
  auto* collection = static_cast<edm4hep::RecoParticleRefCollection*>(m_collections[branch]);

  for (auto iCand = 0; iCand < delphesCollection->GetEntries(); ++iCand) {
    auto* delphesCand = static_cast<DelphesT*>(delphesCollection->At(iCand));

    if (auto matchedReco = getMatchingReco(delphesCand)) {
      auto recoRef = collection->create();
      recoRef.setParticle(*matchedReco);
      // if we have an electron or muon we update the mass as well here
      if constexpr (std::is_same_v<DelphesT, Muon>) {
        matchedReco->setMass(M_MU);
      } else if constexpr (std::is_same_v<DelphesT, Electron>) {
        matchedReco->setMass(M_ELECTRON);
      }

      // If we have a charge available, also set it
      if constexpr (!std::is_same_v<DelphesT, Photon>) {
        matchedReco->setCharge(delphesCand->Charge);
      }

    } else {
      std::cerr << "**** WARNING: No matching ReconstructedParticle was found for a Delphes " << type << std::endl;

    }
  }

}


void DelphesEDM4HepConverter::processMissingET(const TClonesArray* delphesCollection, std::string_view const branch)
{
  auto* collection = static_cast<edm4hep::ReconstructedParticleCollection*>(m_collections[branch]);
  auto* delphesCand = static_cast<MissingET*>(delphesCollection->At(0));

  auto cand = collection->create();
  const auto momentum = delphesCand->P4(); // NOTE: assuming massless here!
  cand.setMomentum({(float) momentum.Px(), (float) momentum.Py(), (float) momentum.Pz()});
  cand.setEnergy(momentum.E());
  // cand.setMass(delphesCand->Mass);
}

void DelphesEDM4HepConverter::processScalarHT(const TClonesArray* delphesCollection, std::string_view const branch)
{
  auto* collection = static_cast<edm4hep::ParticleIDCollection*>(m_collections[branch]);
  auto* delphesCand = static_cast<ScalarHT*>(delphesCollection->At(0));

  auto cand = collection->create();
  cand.addToParameters(delphesCand->HT);
}


template<typename DelphesT>
std::optional<edm4hep::ReconstructedParticle> DelphesEDM4HepConverter::getMatchingReco(DelphesT* delphesCand) const
{
  // Here we have to do some work to actually match the Delphes candidate to
  // the correct edm4hep::ReconstructedParticle because the possibility exists
  // that more than one ReconstructedParticle point to the same UniqueID. Here
  // we are NOT interested in the physics interpretation of such a case, but
  // only want to identify the correct ReconstructedParticle to which we
  // should point. To do the matching we compare the 4-momenta of the stored
  // edm4hep::ReconstructedParticle associated to the GenParticle UniqueID and
  // take the FIRST good match. Since the delphes candidate originates from
  // either a Track or a Tower, there should always be exactly one such good
  // match.

  // Handling slightly different member names for delphes depending on
  // whether we are still working with Candidates or the actual output
  // classes already
  const auto delphes4Mom = [delphesCand]() {
    if constexpr(std::is_same_v<DelphesT, Candidate>) {
      return delphesCand->Momentum;
    } else {
      return delphesCand->P4();
    }
  }();

  for (const auto genId : getAllParticleIDs(delphesCand)) {
    const auto [recoBegin, recoEnd] = m_recoParticleGenIds.equal_range(genId);
    for (auto it = recoBegin; it != recoEnd; ++it) {
      const auto edm4Mom = getP4(it->second);
      if (equalP4(edm4Mom, delphes4Mom)) {
        return it->second;
      } else if (equalP4(edm4Mom, delphes4Mom, 1e-5, false)) {
        // std::cout << "**** DEBUG: Kinematic matching successful after dropping energy matching" << std::endl;
        return it->second;
      }
    }
  }

  return {};
}

edm4hep::MCRecoParticleAssociationCollection* DelphesEDM4HepConverter::createExternalRecoAssociations(const std::unordered_map<UInt_t, edm4hep::ConstMCParticle>& mc_map) {

  auto mcRecoRelations = new edm4hep::MCRecoParticleAssociationCollection();
    for (const auto& particleID: mc_map) {
    const auto [recoBegin, recoEnd] = m_recoParticleGenIds.equal_range(particleID.first);
    for (auto it = recoBegin; it != recoEnd; ++it) {
      auto relation = mcRecoRelations->create();
      relation.setSim(particleID.second);
      relation.setRec(it-> second);
    }
    }
  return mcRecoRelations;
}


void DelphesEDM4HepConverter::registerGlobalCollections()
{
  // Make sure that these are only registered once
  if (m_collections.find(m_recoCollName) == m_collections.end()) {
    createCollection<edm4hep::ReconstructedParticleCollection>(m_recoCollName);
  }
  if (m_collections.find(m_mcRecoAssocCollName) == m_collections.end()) {
    createCollection<edm4hep::MCRecoParticleAssociationCollection>(m_mcRecoAssocCollName);
  }
  if (m_collections.find(m_particleIDName) == m_collections.end()) {
    createCollection<edm4hep::ParticleIDCollection>(m_particleIDName);
  }
}


template<size_t N>
void sortBranchesProcessingOrder(std::vector<BranchSettings>& branches,
                                 std::array<std::string_view, N> const& processingOrder)
{
  std::unordered_map<std::string_view, size_t> classSortIndices;
  for (size_t i = 0; i < processingOrder.size(); ++i) {
    classSortIndices.emplace(processingOrder[i], i);
  }

  const auto endIt = classSortIndices.end();
  std::sort(branches.begin(), branches.end(),
            [&classSortIndices, endIt] (const auto& left, const auto& right) {
              const auto leftIt = classSortIndices.find(left.className);
              const auto rightIt = classSortIndices.find(right.className);

              // if we have the class defined in the processing order use the
              // corresponding index, otherwise use one definitely not inside
              // the processing order
              return (leftIt != endIt ? leftIt->second : N + 1) < (rightIt != endIt ? rightIt->second : N + 1);
            });
}


edm4hep::Track convertTrack(Track const* cand, const double magFieldBz)
{
  edm4hep::Track track;
  // Delphes does not really provide any information that would go into the
  // track itself. But some information can be used to at least partially
  // populate a TrackState
  edm4hep::TrackState trackState{};
  trackState.D0 = cand->D0;
  trackState.Z0 = cand->DZ;

  // Delphes calculates this from the momentum 4-vector at the track
  // perigee so this should be what we want. Note that this value
  // only undergoes smearing in the TrackSmearing module but e.g.
  // not in the MomentumSmearing module
  trackState.phi = cand->Phi;
  // Same thing under different name in Delphes
  trackState.tanLambda = cand->CtgTheta;
  // Only do omega when there is actually a magnetic field.
  double varOmega = 0;
  if (magFieldBz) {
    // conversion to have omega in [1/mm]
    constexpr double a = c_light * 1e3 * 1e-15;

    trackState.omega = a * magFieldBz / cand->PT * std::copysign(1.0, cand->Charge);
    // calculate variation using simple error propagation, assuming
    // constant B-field -> relative error on pT is relative error on omega
    varOmega = cand->ErrorPT * cand->ErrorPT / cand->PT / cand->PT * trackState.omega * trackState.omega;
  }

  // fill the covariance matrix. There is a conversion of units
  // because the covariance matrix in delphes is with the original units 
  // from Franco Bedeschi's code so meter and GeV
  auto& covMatrix = trackState.covMatrix;
  
  TMatrixDSym covaFB = cand->CovarianceMatrix();

  //This is needed as Delphes stores the units in the orignal format, so GeV and meters
  double scale0 = 1.e3;
  double scale1 = 1.;
  //double scale2 = 1.e-3;
  double scale2 = -2.* 1.e-3;    // CAREFUL: DELPHES USES THE HALF-CURVATURE
  double scale3 = 1.e3;
  double scale4 = 1.;

  covMatrix[0]  = covaFB(0,0) *scale0 * scale0;
  
  covMatrix[1]  = covaFB(1,0) *scale1 * scale0;
  covMatrix[2]  = covaFB(1,1) *scale1 * scale1;

  covMatrix[3]  = covaFB(2,0) *scale2 * scale0;
  covMatrix[4]  = covaFB(2,1) *scale2 * scale1;
  covMatrix[5]  = covaFB(2,2) *scale2 * scale2;
  
  covMatrix[6]  = covaFB(3,0) *scale3 * scale0;
  covMatrix[7]  = covaFB(3,1) *scale3 * scale1;
  covMatrix[8]  = covaFB(3,2) *scale3 * scale2;
  covMatrix[9]  = covaFB(3,3) *scale3 * scale3;
  
  covMatrix[10] = covaFB(4,0) *scale4 * scale0;
  covMatrix[11] = covaFB(4,1) *scale4 * scale1;
  covMatrix[12] = covaFB(4,2) *scale4 * scale2;
  covMatrix[13] = covaFB(4,3) *scale4 * scale3;
  covMatrix[14] = covaFB(4,4) *scale4 * scale4;

  track.addToTrackStates(trackState);

  return track;
}

void setMotherDaughterRelations(GenParticle const* delphesCand,
                                edm4hep::MCParticle particle,
                                edm4hep::MCParticleCollection& mcParticles)
{
  // NOTE: it is in general probably not possible to handle all the different
  // possibilities that are present in the different readers. So, for now we are
  // going to follow the pythia documentation (with some additional sanity
  // checks, to avoid some crashs, in case the input is buggy) in the hope that
  // it will work for most inputs
  // Pythia documentation: http://home.thep.lu.se/~torbjorn/pythia81html/ParticleProperties.html

  // Mothers
  // Only set parents if not accessing out of bounds
  const auto safeSetParent = [&mcParticles, &particle] (int index) {
    if (index < mcParticles.size()) {
      particle.addToParents(mcParticles[index]);
    }
  };

  // If M1 == -1, then this particle has no mother, so we only handle cases
  // where there is at least one
  if (delphesCand->M1 > -1) {
    // case 3, only one mother
    if (delphesCand->M2 == -1) {
      safeSetParent(delphesCand->M1);
    }
    if (delphesCand->M2 > -1){
      // case 6, two mothers
      if (delphesCand->M2 < delphesCand->M1) {
        safeSetParent(delphesCand->M1);
        safeSetParent(delphesCand->M2);

      } else {
        //  cases 2, 5 (and 4 without checking the status), mothers in a range
        for (auto iMother = delphesCand->M1; iMother <= delphesCand->M2; ++iMother) {
          safeSetParent(iMother);
        }
      }
    }
  }

  // Daughters
  const auto safeSetDaughter = [&mcParticles, &particle] (int index) {
    if (index < mcParticles.size()) {
      particle.addToDaughters(mcParticles[index]);
    }
  };
  
  // again handle only the cases where there is at least one daughter
  if (delphesCand->D1 > -1) {
    // case 3
    if (delphesCand->D2 == -1) {
      safeSetDaughter(delphesCand->D1);
    }
    if (delphesCand->D2 > -1) {
      // case 5
      if (delphesCand->D2 < delphesCand->D1) {
        safeSetDaughter(delphesCand->D1);
        safeSetDaughter(delphesCand->D2);
      } else {
        // cases 2 and 4
        for (auto iDaughter = delphesCand->D1; iDaughter <= delphesCand->D2; ++iDaughter) {
          safeSetDaughter(iDaughter);
        }
      }
    }
  }
}


} // namespace k4SimDelphes
