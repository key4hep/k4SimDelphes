#include "delphesHelpers.h" // getAllParticleIds

#include "k4SimDelphes/DelphesEDM4HepConverter.h"
#include "k4SimDelphes/DelphesEDM4HepOutputConfiguration.h"

#include "edm4hep/CalorimeterHitCollection.h"
#include "edm4hep/ClusterCollection.h"
#include "edm4hep/Constants.h"
#include "edm4hep/EventHeaderCollection.h"
#include "edm4hep/MCParticleCollection.h"
#include "edm4hep/ParticleIDCollection.h"
#include "edm4hep/RecDqdxCollection.h"
#include "edm4hep/RecoMCParticleLinkCollection.h"
#include "edm4hep/ReconstructedParticleCollection.h"
#include "edm4hep/TrackCollection.h"
#include "edm4hep/TrackerHit3DCollection.h"
#include "edm4hep/Vector3d.h"

#include "podio/UserDataCollection.h"

#include <TMatrixDSym.h>

#include <algorithm>
#include <cmath>
#include <iostream>

namespace k4SimDelphes {

// TODO: Take these from HepPDT / HepMC?
constexpr double M_PIPLUS = 0.13957039;        // GeV (PDG 2020)
constexpr double M_MU = 0.1056583745;          // GeV (PDG 2020)
constexpr double M_ELECTRON = 0.5109989461e-3; // GeV (PDG 2020)

constexpr int PDG_PIPLUS = 211;
constexpr int PDG_MU = 13;
constexpr int PDG_ELECTRON = 11;

// TODO: Make configurable?
constexpr double trackMass = M_PIPLUS;

/**
 * Order in which the different delphes output classes will be processed.
 * Everything not defined here will not be processed.
 *
 * NOTE: not a configuration parameter. this has to be done in this order to
 * ensure that products required by later stages are producd early enough
 */
constexpr std::array<std::string_view, 10> PROCESSING_ORDER = {
    "GenParticle", "Track",     "Tower",   "ParticleFlowCandidate", "Muon", "Electron", "Photon",
    "Jet",         "MissingET", "ScalarHT"};

template <size_t N>
void sortBranchesProcessingOrder(std::vector<BranchSettings>& branches,
                                 std::array<std::string_view, N> const& processingOrder);

edm4hep::MutableTrack convertTrack(Track const* cand);

/**
 * Simple helper function to make it easier to refactor later
 */
template <typename Container>
inline bool contains(Container const& container, typename Container::value_type const& value) {
  return std::find(container.cbegin(), container.cend(), value) != container.cend();
}

// position covariance of a calo impact point calculated from the angular resolutions of the direction
edm4hep::CovMatrix3f positionCovFromAngles(const edm4hep::Vector3f& pos, double sTheta, double sPhi) {
  using edm4hep::Cartesian;
  const double r = std::sqrt(double(pos.x) * pos.x + double(pos.y) * pos.y + double(pos.z) * pos.z);
  if (r == 0.) {
    // no impact point (e.g. Delphes towers that only have track hits): the error is undefined
    return edm4hep::CovMatrix3f{};
  }
  const double rt = std::hypot(double(pos.x), double(pos.y));
  const double ct = pos.z / r, st = rt / r;
  const double cp = rt > 0. ? pos.x / rt : 1., sp = rt > 0. ? pos.y / rt : 0.;
  // local unit vectors at the impact point: uT = (tx, ty, tz), uP = (px, py, 0)
  const double tx = ct * cp, ty = ct * sp, tz = -st;
  const double px = -sp, py = cp;
  const double a2 = r * sTheta * r * sTheta, b2 = r * st * sPhi * r * st * sPhi;
  edm4hep::CovMatrix3f cov{};
  cov.setValue(a2 * tx * tx + b2 * px * px, Cartesian::x, Cartesian::x);
  cov.setValue(a2 * tx * ty + b2 * px * py, Cartesian::x, Cartesian::y);
  cov.setValue(a2 * ty * ty + b2 * py * py, Cartesian::y, Cartesian::y);
  cov.setValue(a2 * tx * tz, Cartesian::x, Cartesian::z);
  cov.setValue(a2 * ty * tz, Cartesian::y, Cartesian::z);
  cov.setValue(a2 * tz * tz, Cartesian::z, Cartesian::z);
  return cov;
}

DelphesEDM4HepConverter::DelphesEDM4HepConverter(std::string filename_delphescard) {
  auto confReader = std::make_unique<ExRootConfReader>();
  confReader->ReadFile(filename_delphescard.c_str());
  const auto branches = getBranchSettings(confReader->GetParam("TreeWriter::Branch"));
  const auto edm4hepOutputSettings = OutputSettings();
  DelphesEDM4HepConverter edm4hepConverter(branches, edm4hepOutputSettings,
                                           confReader->GetDouble("ParticlePropagator::Bz", 0));
};

DelphesEDM4HepConverter::DelphesEDM4HepConverter(const std::vector<BranchSettings>& branches,
                                                 OutputSettings const& outputSettings, double magFieldBz)
    : m_magneticFieldBz(magFieldBz), m_recoCollName(outputSettings.RecoParticleCollectionName),
      m_recoMCLinkCollName(outputSettings.RecoMCParticleLinkCollectionName) {
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
      m_processFunctions.emplace(branch.name, &DelphesEDM4HepConverter::processParticles);
    }

    if (contains(outputSettings.ReconstructedParticleCollections, branch.name.c_str()) &&
        contains(RECO_TRACK_OUTPUT, branch.className.c_str())) {
      m_processFunctions.emplace(branch.name, &DelphesEDM4HepConverter::processTracks);
    }

    if (contains(outputSettings.ReconstructedParticleCollections, branch.name.c_str()) &&
        contains(RECO_CANDIDATES_OUTPUT, branch.className.c_str())) {
      m_processFunctions.emplace(branch.name, &DelphesEDM4HepConverter::processPFlowCandidates);
    }

    if (contains(outputSettings.ReconstructedParticleCollections, branch.name.c_str()) &&
        contains(RECO_CLUSTER_OUTPUT, branch.className.c_str())) {
      m_processFunctions.emplace(branch.name, &DelphesEDM4HepConverter::processClusters);
    }

    if (contains(outputSettings.JetCollections, branch.name.c_str())) {
      m_processFunctions.emplace(branch.name, &DelphesEDM4HepConverter::processJets);
    }

    if (contains(outputSettings.MuonCollections, branch.name.c_str()) ||
        contains(outputSettings.ElectronCollections, branch.name.c_str()) ||
        contains(outputSettings.PhotonCollections, branch.name.c_str())) {
      m_processFunctions.emplace(branch.name, refProcessFunctions.at(branch.className));
    }

    if (contains(outputSettings.MissingETCollections, branch.name.c_str())) {
      m_processFunctions.emplace(branch.name, &DelphesEDM4HepConverter::processMissingET);
    }

    if (contains(outputSettings.ScalarHTCollections, branch.name.c_str())) {
      m_processFunctions.emplace(branch.name, &DelphesEDM4HepConverter::processScalarHT);
    }
  }
}

void DelphesEDM4HepConverter::process(TTree* delphesTree,
                                      const std::unordered_map<UInt_t, edm4hep::MCParticle>* externalMCParticles) {
  // beginning of processing: clear previous event from containers
  m_collections.clear();

  // Make sure the shared collections are present
  registerGlobalCollections();

  // filling the event header
  auto* eventBranch = delphesTree->GetBranch("Event");

  if (eventBranch) {
    auto* delphesEvents = *(TClonesArray**)eventBranch->GetAddress();
    auto* delphesEvent = static_cast<HepMCEvent*>(delphesEvents->At(0));
    createEventHeader(delphesEvent);
  }

  std::vector<std::string> jetCollNames;
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
      auto* delphesCollection = *(TClonesArray**)rootBranch->GetAddress();
      (this->*processFuncIt->second)(delphesCollection, branch.name);
      if (processFuncIt->second == &DelphesEDM4HepConverter::processJets) {
        jetCollNames.push_back(branch.name);
      }
    }
  }

  // Update jet energy and mass from final constituent energies (constituents may
  // have had their energies updated by muon/electron processing after jets were
  // first filled)
  finalizeJets(jetCollNames);

  // Links to external MCParticles (e.g. the Delphes input in the framework
  // case) have to be created before the internal ID maps are cleared below
  if (externalMCParticles) {
    m_collections[m_recoMCLinkCollName] = createExternalRecoMCLinks(*externalMCParticles);
  }

  // Clear the internal maps that hold references to entites that have been put
  // into maps here for internal use only (see #89)
  m_genParticleIds.clear();
  m_recoParticleGenIds.clear();
  m_recoParticleIds.clear();
}

// convert the eventHeader with metaData
void DelphesEDM4HepConverter::createEventHeader(const HepMCEvent* delphesEvent) {
  auto* collection = createCollection<edm4hep::EventHeaderCollection>(EVENTHEADER_NAME);
  auto cand = collection->create();

  cand.setWeight(delphesEvent->Weight);
  cand.setEventNumber(delphesEvent->Number);
}

void DelphesEDM4HepConverter::setEventWeights(const std::vector<double>& weights) {
  auto* collection = getCollection<edm4hep::EventHeaderCollection>(EVENTHEADER_NAME);
  if (!collection || collection->size() == 0) {
    std::cerr << "K4SIMDELPHES ERROR: setEventWeights called but no EventHeader exists for this event" << std::endl;
    return;
  }
  auto header = collection->at(0);
  for (const auto w : weights) {
    header.addToWeights(w);
  }
}

void DelphesEDM4HepConverter::processParticles(const TClonesArray* delphesCollection, std::string const& branch) {
  auto* collection = createCollection<edm4hep::MCParticleCollection>(branch);

  for (int iCand = 0; iCand < delphesCollection->GetEntries(); ++iCand) {
    auto* delphesCand = static_cast<GenParticle*>(delphesCollection->At(iCand));

    auto cand = collection->create();
    cand.setCharge(delphesCand->Charge);
    cand.setMass(delphesCand->Mass);
    cand.setMomentum({delphesCand->Px, delphesCand->Py, delphesCand->Pz});
    cand.setVertex({delphesCand->X, delphesCand->Y, delphesCand->Z});
    cand.setTime(delphesCand->T);  // in seconds
    cand.setPDG(delphesCand->PID); // delphes uses whatever hepevt.idhep provides
    cand.setGeneratorStatus(delphesCand->Status);
    if (const auto [it, inserted] = m_genParticleIds.emplace(delphesCand->GetUniqueID(), cand); !inserted) {
      std::cerr << "**** WARNING: UniqueID " << delphesCand->GetUniqueID()
                << " is already used by MCParticle with id: " << it->second.id() << std::endl;
    }
  }

  // mother-daughter relations
  std::vector<const GenParticle*> delphesCands;
  delphesCands.reserve(delphesCollection->GetEntries());
  for (int iCand = 0; iCand < delphesCollection->GetEntries(); ++iCand) {
    delphesCands.push_back(static_cast<const GenParticle*>(delphesCollection->At(iCand)));
  }
  setMotherDaughterRelations(delphesCands, *collection);
}

void DelphesEDM4HepConverter::processTracks(const TClonesArray* delphesCollection, std::string const& branch) {
  auto* particleCollection = getCollection<edm4hep::ReconstructedParticleCollection>(m_recoCollName);
  auto* trackCollection = createCollection<edm4hep::TrackCollection>(branch);
  auto* dqdxCollection = createCollection<edm4hep::RecDqdxCollection>(branch + "_dNdx");
  // UserData for overflowing information
  auto* pathLengthCollection = createCollection<podio::UserDataCollection<float>>(branch + "_L");

  // user data to define magnetic field (fill once per event)
  auto* magFieldCollection = createCollection<podio::UserDataCollection<float>>("magFieldBz");
  magFieldCollection->push_back(m_magneticFieldBz);

  auto* mcRecoRelations = getCollection<edm4hep::RecoMCParticleLinkCollection>(m_recoMCLinkCollName);
  auto* idCollection = createCollection<edm4hep::ParticleIDCollection>(branch + "_PID");
  auto* trackerHitColl = getCollection<edm4hep::TrackerHit3DCollection>(TRACKERHIT_OUTPUT_NAME);

  for (auto iCand = 0; iCand < delphesCollection->GetEntries(); ++iCand) {
    auto* delphesCand = static_cast<Track*>(delphesCollection->At(iCand));

    auto track = convertTrack(delphesCand);
    trackCollection->push_back(track);

    // this is the position/time at the IP
    auto trackerHit0 = trackerHitColl->create();
    trackerHit0.setTime(delphesCand->T); // in seconds
    edm4hep::Vector3d position0(delphesCand->X, delphesCand->Y, delphesCand->Z);
    trackerHit0.setPosition(position0);
    track.addToTrackerHits(trackerHit0);

    // this is the position of the first hit (NB: time not available in Delphes here)
    auto trackerHit1 = trackerHitColl->create();
    edm4hep::Vector3d position1(delphesCand->XFirstHit, delphesCand->YFirstHit, delphesCand->ZFirstHit);
    trackerHit1.setPosition(position1);
    track.addToTrackerHits(trackerHit1);

    // this is the position/time at the calorimeter
    auto trackerHit2 = trackerHitColl->create();
    trackerHit2.setTime(delphesCand->TOuter); // in seconds
    edm4hep::Vector3d position2(delphesCand->XOuter, delphesCand->YOuter, delphesCand->ZOuter);
    trackerHit2.setPosition(position2);
    track.addToTrackerHits(trackerHit2);

    pathLengthCollection->push_back(delphesCand->L);

    auto dndx = dqdxCollection->create();
    dndx.getDQdx().value = delphesCand->dNdx;
    dndx.setTrack(track);

    auto id = idCollection->create();

    auto cand = particleCollection->create();
    cand.setCharge(delphesCand->Charge);
    const auto momentum = delphesCand->P4();
    cand.setEnergy(std::hypot(std::hypot(momentum.Px(), momentum.Py(), momentum.Pz()), trackMass));
    cand.setMomentum({(float)momentum.Px(), (float)momentum.Py(), (float)momentum.Pz()});
    // At this point indiscriminantly set the mass for each track. If this is a
    // muon or an electron, the mass will be set to the appropriate value later.
    cand.setMass(trackMass);
    cand.setPDG(delphesCand->Charge > 0 ? PDG_PIPLUS : -PDG_PIPLUS);

    cand.addToTracks(track);

    // id.addToParameters(delphesCand->IsolationVar);
    id.setParticle(cand);

    UInt_t genId = delphesCand->Particle.GetUniqueID();
    if (const auto genIt = m_genParticleIds.find(genId); genIt != m_genParticleIds.end()) {
      auto relation = mcRecoRelations->create();
      relation.setTo(genIt->second);
      relation.setFrom(cand);
    }

    m_recoParticleGenIds.emplace(genId, cand);
    m_recoParticleIds.emplace(delphesCand->GetUniqueID(), cand);
  }
}

void DelphesEDM4HepConverter::processClusters(const TClonesArray* delphesCollection, std::string const& branch) {
  auto* particleCollection = getCollection<edm4hep::ReconstructedParticleCollection>(m_recoCollName);
  auto* clusterCollection = createCollection<edm4hep::ClusterCollection>(branch);
  auto* mcRecoRelations = getCollection<edm4hep::RecoMCParticleLinkCollection>(m_recoMCLinkCollName);
  auto* calorimeterHitColl = getCollection<edm4hep::CalorimeterHitCollection>(CALORIMETERHIT_OUTPUT_NAME);

  for (auto iCand = 0; iCand < delphesCollection->GetEntries(); ++iCand) {
    auto* delphesCand = static_cast<Tower*>(delphesCollection->At(iCand));

    auto cluster = clusterCollection->create();
    cluster.setEnergy(delphesCand->E);
    const edm4hep::Vector3f position(delphesCand->X, delphesCand->Y, delphesCand->Z);
    cluster.setPosition(position);
    // TODO: Potentially every delphes tower could be split into two
    // edm4hep::clusters, with energies split according to Eem and Ehad. But
    // that would probably make the matching that is done below much harder

    auto cand = particleCollection->create();
    // NOTE: Delphes assumes m=0 for photons and m=0.497611 for neutral hadrons (KL)
    const auto momentum = delphesCand->P4();
    // TODO: fill this when it is available later, when when we link the references?
    // cand.setCharge(delphesCand->Charge);
    cand.setMomentum({(float)momentum.Px(), (float)momentum.Py(), (float)momentum.Pz()});
    cand.setEnergy(delphesCand->E);
    cand.setMass(delphesCand->P4().M());
    // NOTE: Particle-Flow Neutral are either photons or K_L in Delphes
    auto pid = (delphesCand->Ehad > 0.) ? 130 : 22;
    cand.setPDG(pid); // NOTE: set PID of cluster consistent with mass

    if (pid == 22) {
      cluster.setEnergyError(delphesCand->ErrorE);
      cluster.setITheta(delphesCand->ThetaP);
      cluster.setIPhi(delphesCand->PhiP);
      // assume no covariance between theta and phi for now
      cluster.setDirectionError(
          {delphesCand->ErrorPhiP * delphesCand->ErrorPhiP, 0.f, delphesCand->ErrorThetaP * delphesCand->ErrorThetaP});
      cluster.setPositionError(positionCovFromAngles(position, delphesCand->ErrorTheta, delphesCand->ErrorPhi));
      cand.setReferencePoint(position);
    }

    // store position and time of neutral candidate in a CalorimeterHit
    auto calorimeterHit = calorimeterHitColl->create();
    calorimeterHit.setTime(delphesCand->T); // in seconds
    calorimeterHit.setPosition(position);
    cluster.addToHits(calorimeterHit);
    cand.addToClusters(cluster);

    for (const auto genId : getAllParticleIDs(delphesCand)) {
      if (const auto genIt = m_genParticleIds.find(genId); genIt != m_genParticleIds.end()) {
        auto relation = mcRecoRelations->create();
        relation.setTo(genIt->second);
        relation.setFrom(cand);
      }

      m_recoParticleGenIds.emplace(genId, cand);
    }
    m_recoParticleIds.emplace(delphesCand->GetUniqueID(), cand);
  }
}

void DelphesEDM4HepConverter::processJets(const TClonesArray* delphesCollection, std::string const& branch) {
  auto* jetCollection = createCollection<edm4hep::ReconstructedParticleCollection>(branch);
  auto* idCollection_HF_tags = createCollection<edm4hep::ParticleIDCollection>(branch + "_HF_tags");
  auto* idCollection_tau_tags = createCollection<edm4hep::ParticleIDCollection>(branch + "_tau_tags");

  for (auto iCand = 0; iCand < delphesCollection->GetEntries(); ++iCand) {
    auto* delphesCand = static_cast<Jet*>(delphesCollection->At(iCand));
    auto jet = jetCollection->create();
    auto id_HF_tag = idCollection_HF_tags->create();
    auto id_tau_tag = idCollection_tau_tags->create();

    jet.setCharge(delphesCand->Charge);
    const auto momentum = delphesCand->P4();
    jet.setMomentum({(float)momentum.Px(), (float)momentum.Py(), (float)momentum.Pz()});
    // Set energy and mass from Delphes as a fallback; finalizeJets() will
    // override these from constituent sums when constituents are available.
    jet.setEnergy(momentum.E());
    jet.setMass(delphesCand->Mass);

    // id.addToParameters(delphesCand->IsolationVar);
    id_HF_tag.addToParameters(delphesCand->BTag);
    id_tau_tag.addToParameters(delphesCand->TauTag);
    id_HF_tag.setParticle(jet);
    id_tau_tag.setParticle(jet);

    const auto& constituents = delphesCand->Constituents;
    for (auto iConst = 0; iConst < constituents.GetEntries(); ++iConst) {
      auto* constituent = static_cast<Candidate*>(constituents.At(iConst));
      if (const auto it = m_recoParticleIds.find(constituent->GetUniqueID()); it != m_recoParticleIds.end()) {
        jet.addToParticles(it->second);
      } else {
        std::cerr << "**** WARNING: No matching ReconstructedParticle was found for a Jet constituent" << std::endl;
      }
    }
  }
}

void DelphesEDM4HepConverter::finalizeJets(const std::vector<std::string>& jetCollNames) {
  for (const auto& collName : jetCollNames) {
    auto* jetColl = getCollection<edm4hep::ReconstructedParticleCollection>(collName);
    for (auto jet : *jetColl) {
      if (jet.getParticles().empty()) {
        continue; // keep the Delphes energy/mass set in processJets
      }
      double jetE = 0;
      for (const auto& part : jet.getParticles()) {
        jetE += part.getEnergy();
      }
      const auto mom = jet.getMomentum();
      const double p2 = mom.x * mom.x + mom.y * mom.y + mom.z * mom.z;
      const double mass2 = jetE * jetE - p2;
      jet.setEnergy(jetE);
      jet.setMass(mass2 > 0 ? std::sqrt(mass2) : 0);
    }
  }
}

void DelphesEDM4HepConverter::processPFlowCandidates(const TClonesArray* delphesCollection, std::string const& branch) {
  auto* candidateCollection = createCollection<edm4hep::ReconstructedParticleCollection>(branch);

  for (auto iCand = 0; iCand < delphesCollection->GetEntries(); ++iCand) {
    auto* delphesCand = static_cast<ParticleFlowCandidate*>(delphesCollection->At(iCand));
    auto candidate = candidateCollection->create();

    candidate.setCharge(delphesCand->Charge);
    candidate.setMass(delphesCand->Mass);
    const auto momentum = delphesCand->P4();
    candidate.setEnergy(momentum.E());
    candidate.setMomentum({(float)momentum.Px(), (float)momentum.Py(), (float)momentum.Pz()});
  }
}

template <typename DelphesT>
void DelphesEDM4HepConverter::fillReferenceCollection(const TClonesArray* delphesCollection, std::string const& branch,
                                                      std::string_view const type) {
  auto* collection = createCollection<edm4hep::ReconstructedParticleCollection>(branch, true);

  // add collection for the isolation variable calculated by delphes:
  auto* isoIDColl = createCollection<podio::UserDataCollection<float>>(std::string(branch) + "_IsolationVar");

  for (auto iCand = 0; iCand < delphesCollection->GetEntries(); ++iCand) {
    auto* delphesCand = static_cast<DelphesT*>(delphesCollection->At(iCand));

    if (auto matchedReco = getMatchingReco(delphesCand)) {
      collection->push_back(*matchedReco);
      // if we have an electron or muon we update the mass as well here
      if constexpr (std::is_same_v<DelphesT, Muon>) {
        const auto& p = matchedReco->getMomentum();
        matchedReco->setMass(M_MU);
        matchedReco->setEnergy(std::hypot(std::hypot(p.x, p.y, p.z), M_MU));
        matchedReco->setPDG(delphesCand->Charge < 0 ? PDG_MU : -PDG_MU);
      } else if constexpr (std::is_same_v<DelphesT, Electron>) {
        const auto& p = matchedReco->getMomentum();
        matchedReco->setMass(M_ELECTRON);
        matchedReco->setEnergy(std::hypot(std::hypot(p.x, p.y, p.z), M_ELECTRON));
        matchedReco->setPDG(delphesCand->Charge < 0 ? PDG_ELECTRON : -PDG_ELECTRON);
      }

      // If we have a charge available, also set it
      if constexpr (!std::is_same_v<DelphesT, Photon>) {
        matchedReco->setCharge(delphesCand->Charge);
      }

      // fill the isolation var
      isoIDColl->push_back(delphesCand->IsolationVar);

    } else {
      std::cerr << "**** WARNING: No matching ReconstructedParticle was found for a Delphes " << type << std::endl;
    }
  }
}

void DelphesEDM4HepConverter::processMissingET(const TClonesArray* delphesCollection, std::string const& branch) {
  auto* collection = createCollection<edm4hep::ReconstructedParticleCollection>(branch);
  auto* delphesCand = static_cast<MissingET*>(delphesCollection->At(0));

  auto cand = collection->create();
  const auto momentum = delphesCand->P4(); // NOTE: assuming massless here!
  cand.setMomentum({(float)momentum.Px(), (float)momentum.Py(), (float)momentum.Pz()});
  cand.setEnergy(momentum.E());
  // cand.setMass(delphesCand->Mass);
}

void DelphesEDM4HepConverter::processScalarHT(const TClonesArray* delphesCollection, std::string const& branch) {
  auto* collection = createCollection<podio::UserDataCollection<float>>(branch);
  auto* delphesCand = static_cast<ScalarHT*>(delphesCollection->At(0));
  collection->push_back(delphesCand->HT);
}

template <typename DelphesT>
std::optional<edm4hep::MutableReconstructedParticle>
DelphesEDM4HepConverter::getMatchingReco(DelphesT* delphesCand) const {
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
    if constexpr (std::is_same_v<DelphesT, Candidate>) {
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

std::unique_ptr<edm4hep::RecoMCParticleLinkCollection> DelphesEDM4HepConverter::createExternalRecoMCLinks(
    const std::unordered_map<UInt_t, edm4hep::MCParticle>& mc_map) const {
  auto mcRecoRelations = std::make_unique<edm4hep::RecoMCParticleLinkCollection>();
  for (const auto& particleID : mc_map) {
    const auto [recoBegin, recoEnd] = m_recoParticleGenIds.equal_range(particleID.first);
    for (auto it = recoBegin; it != recoEnd; ++it) {
      auto relation = mcRecoRelations->create();
      relation.setTo(particleID.second);
      relation.setFrom(it->second);
    }
  }
  return mcRecoRelations;
}

void DelphesEDM4HepConverter::registerGlobalCollections() {
  // Make sure that these are only registered once
  if (m_collections.find(m_recoCollName) == m_collections.end()) {
    createCollection<edm4hep::ReconstructedParticleCollection>(m_recoCollName);
  }
  if (m_collections.find(m_recoMCLinkCollName) == m_collections.end()) {
    createCollection<edm4hep::RecoMCParticleLinkCollection>(m_recoMCLinkCollName);
  }
  if (m_collections.find(TRACKERHIT_OUTPUT_NAME) == m_collections.end()) {
    createCollection<edm4hep::TrackerHit3DCollection>(TRACKERHIT_OUTPUT_NAME);
  }
  if (m_collections.find(CALORIMETERHIT_OUTPUT_NAME) == m_collections.end()) {
    createCollection<edm4hep::CalorimeterHitCollection>(CALORIMETERHIT_OUTPUT_NAME);
  }
}

template <size_t N>
void sortBranchesProcessingOrder(std::vector<BranchSettings>& branches,
                                 std::array<std::string_view, N> const& processingOrder) {
  std::unordered_map<std::string_view, size_t> classSortIndices;
  for (size_t i = 0; i < processingOrder.size(); ++i) {
    classSortIndices.emplace(processingOrder[i], i);
  }

  const auto endIt = classSortIndices.end();
  std::sort(branches.begin(), branches.end(), [&classSortIndices, endIt](const auto& left, const auto& right) {
    const auto leftIt = classSortIndices.find(left.className);
    const auto rightIt = classSortIndices.find(right.className);

    // if we have the class defined in the processing order use the
    // corresponding index, otherwise use one definitely not inside
    // the processing order
    return (leftIt != endIt ? leftIt->second : N + 1) < (rightIt != endIt ? rightIt->second : N + 1);
  });
}

edm4hep::MutableTrack convertTrack(Track const* cand) {
  edm4hep::MutableTrack track;
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

  // fill the covariance matrix. There is a conversion of units
  // because the covariance matrix in delphes is with the original units
  // from Franco Bedeschi's code so meter and GeV
  auto& covMatrix = trackState.covMatrix;

  TMatrixDSym covaFB = cand->CovarianceMatrix();

  // needs to be applied to any covariance matrix element
  // relating to curvature (index 2)
  double scale2 = -2.; // CAREFUL: DELPHES USES THE HALF-CURVATURE

  trackState.omega = cand->C * scale2;

  covMatrix[0] = covaFB(0, 0);

  covMatrix[1] = covaFB(1, 0);
  covMatrix[2] = covaFB(1, 1);

  covMatrix[3] = covaFB(2, 0) * scale2;
  covMatrix[4] = covaFB(2, 1) * scale2;
  covMatrix[5] = covaFB(2, 2) * scale2 * scale2;

  covMatrix[6] = covaFB(3, 0);
  covMatrix[7] = covaFB(3, 1);
  covMatrix[8] = covaFB(3, 2) * scale2;
  covMatrix[9] = covaFB(3, 3);

  covMatrix[10] = covaFB(4, 0);
  covMatrix[11] = covaFB(4, 1);
  covMatrix[12] = covaFB(4, 2) * scale2;
  covMatrix[13] = covaFB(4, 3);
  covMatrix[14] = covaFB(4, 4);

  track.addToTrackStates(trackState);

  return track;
}

namespace {
  /// The particles referenced by a (first, second) pair of mother or daughter
  /// indices, following the Pythia conventions. explicitRef marks the entries
  /// that are referenced directly (a single entry, two separate entries, or the
  /// ends of a range), as opposed to the inside of a range.
  struct IndexRefs {
    std::vector<int> indices{};
    std::vector<bool> explicitRef{};

    bool contains(int i) const { return std::find(indices.begin(), indices.end(), i) != indices.end(); }
    bool containsExplicit(int i) const {
      for (size_t k = 0; k < indices.size(); ++k) {
        if (indices[k] == i) {
          return explicitRef[k];
        }
      }
      return false;
    }
  };

  IndexRefs decodeIndexPair(int first, int second, int size) {
    IndexRefs refs;
    const auto add = [&refs, size](int i, bool isExplicit) {
      if (i >= 0 && i < size) {
        refs.indices.push_back(i);
        refs.explicitRef.push_back(isExplicit);
      }
    };
    if (first < 0) {
      return refs;
    }
    if (second < 0) {
      add(first, true); // only one
    } else if (second < first) {
      add(first, true); // two separate ones
      add(second, true);
    } else {
      for (int i = first; i <= second; ++i) { // a range
        add(i, i == first || i == second);
      }
    }
    return refs;
  }
} // namespace

void setMotherDaughterRelations(const std::vector<const GenParticle*>& delphesCands,
                                edm4hep::MCParticleCollection& mcParticles) {
  // NOTE: The mother and daughter indices are interpreted following the Pythia
  // conventions, see
  // http://home.thep.lu.se/~torbjorn/pythia81html/ParticleProperties.html
  // Since a pair of indices can only represent a contiguous range or two
  // separate entries, many inputs cannot be stored exactly, and the readers
  // resolve this differently. For example the HEPEVT record (STDHEP reader)
  // only has daughter ranges, which include unrelated particles when the
  // showers of two partons are interleaved, or which miss e.g. ISR photons
  // stored elsewhere, while the mother indices are correct. The Delphes HepMC
  // readers on the other hand store two separate mothers as a range, which
  // then includes the particles in between. Hence, a relation is only set if
  //  - mother and daughter both reference each other, or
  //  - the daughter references the mother explicitly, i.e. not only as part of
  //    the inside of a range, or
  //  - the mother references the daughter and the daughter has no mothers
  // This ensures that parents and daughters are always consistent.
  const auto size = std::min(static_cast<int>(delphesCands.size()), static_cast<int>(mcParticles.size()));
  std::vector<IndexRefs> mothers(size), daughters(size);
  for (int i = 0; i < size; ++i) {
    mothers[i] = decodeIndexPair(delphesCands[i]->M1, delphesCands[i]->M2, size);
    daughters[i] = decodeIndexPair(delphesCands[i]->D1, delphesCands[i]->D2, size);
  }

  const auto isRelated = [&mothers, &daughters](int iMother, int iDaughter) {
    const auto& motherRefs = mothers[iDaughter];
    if (motherRefs.containsExplicit(iMother)) {
      return true;
    }
    const bool daughterRefd = daughters[iMother].contains(iDaughter);
    return daughterRefd && (motherRefs.contains(iMother) || motherRefs.indices.empty());
  };

  std::vector<std::vector<int>> parentsOf(size), daughtersOf(size);
  const auto addRelation = [&parentsOf, &daughtersOf](int iMother, int iDaughter) {
    auto& parents = parentsOf[iDaughter];
    if (std::find(parents.begin(), parents.end(), iMother) == parents.end()) {
      parents.push_back(iMother);
      daughtersOf[iMother].push_back(iDaughter);
    }
  };
  for (int i = 0; i < size; ++i) {
    for (const auto iMother : mothers[i].indices) {
      if (isRelated(iMother, i)) {
        addRelation(iMother, i);
      }
    }
    for (const auto iDaughter : daughters[i].indices) {
      if (isRelated(i, iDaughter)) {
        addRelation(i, iDaughter);
      }
    }
  }

  for (int i = 0; i < size; ++i) {
    auto particle = mcParticles[i];
    std::sort(parentsOf[i].begin(), parentsOf[i].end());
    std::sort(daughtersOf[i].begin(), daughtersOf[i].end());
    for (const auto iMother : parentsOf[i]) {
      particle.addToParents(mcParticles[iMother]);
    }
    for (const auto iDaughter : daughtersOf[i]) {
      particle.addToDaughters(mcParticles[iDaughter]);
    }
  }
}

} // namespace k4SimDelphes
