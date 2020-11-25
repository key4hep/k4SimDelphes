#ifndef DELPHESEDM4HEP_DELPHESHELPERS
#define DELPHESEDM4HEP_DELPHESHELPERS

#include "classes/DelphesClasses.h"

#include "TRefArray.h"
#include "Math/Vector4D.h"

#include <vector>
#include <set>

namespace k4simdelphes {
// TODO: If CLHEP ever gets part of edm4hep, take this from there.
static constexpr double c_light = 2.99792458e+8;

using LorentzVectorT = ROOT::Math::PxPyPzEVector;

/**
 * Get the 4-momentum from an edm4hep type, using the momentum and the energy
 */
template<typename T>
inline LorentzVectorT getP4(const T& particle) {
  return LorentzVectorT{
    particle.getMomentum()[0],
    particle.getMomentum()[1],
    particle.getMomentum()[2],
    particle.getEnergy()
  };
}

/**
 * Compare two 4-momentum vectors to be within a relative tolerance in the
 * energy and all momentum components
 */
template<typename LVectorT, typename LVectorU>
inline bool equalP4(const LVectorT& p1, const LVectorU& p2, double tol=1e-3) {
  if (std::abs(p1.E() - p2.E()) / p1.E() > tol) return false;
  if (std::abs((p1.Px() - p2.Px()) / p1.Px()) > tol) return false;
  if (std::abs((p1.Py() - p2.Py()) / p1.Py()) > tol) return false;
  if (std::abs((p1.Pz() - p2.Pz()) / p1.Pz()) > tol) return false;

  return true;
}


/**
 * Get all UniqueIDs of the GenParticles associated with this delphes candidate,
 * where DelphesT is a Delphes output class type. For the internally used
 * Candidate class there is a dedicated overload with a slightly different
 * algorithm
 */
template<typename DelphesT>
std::vector<UInt_t> getAllParticleIDs(DelphesT* delphesCand) {
  std::vector<UInt_t> relatedParticles;

  if constexpr (std::is_same_v<DelphesT, Muon> ||
                std::is_same_v<DelphesT, Electron>) {
    relatedParticles.push_back(delphesCand->Particle.GetUniqueID());
  } else {
    const auto &refArray = delphesCand->Particles;
    relatedParticles.reserve(refArray.GetEntries());
    for (int i = 0; i < refArray.GetEntries(); ++i) {
      relatedParticles.push_back(refArray.At(i)->GetUniqueID());
    }
  }

  return relatedParticles;
}

/**
 * Delphes algorithm implemented in TreeWriter::FillParticles.
 * Returning a set here, because that makes deduplicating the Ids easier
 */
std::set<UInt_t> getAllParticleIDs(Candidate* candidate) {
  std::set<UInt_t> relatedParticles;
  TIter it1(candidate->GetCandidates());
  it1.Reset();

  while((candidate = static_cast<Candidate*>(it1.Next()))) {
    TIter it2(candidate->GetCandidates());

    // particle
    if (candidate->GetCandidates()->GetEntriesFast() == 0) {
      relatedParticles.insert(candidate->GetUniqueID());
      continue;
    }

    // track
    candidate = static_cast<Candidate*>(candidate->GetCandidates()->At(0));
    if (candidate->GetCandidates()->GetEntriesFast() == 0) {
      relatedParticles.insert(candidate->GetUniqueID());
      continue;
    }

    // tower
    it2.Reset();
    while((candidate = static_cast<Candidate*>(it2.Next()))) {
      relatedParticles.insert(candidate->GetCandidates()->At(0)->GetUniqueID());
    }
  }

  return relatedParticles;
}


} // namespace k4simdelphes

#endif
