#ifndef DELPHESEDM4HEP_DELPHESHELPERS
#define DELPHESEDM4HEP_DELPHESHELPERS

#include "classes/DelphesClasses.h"

#include "Math/Vector4D.h"
#include "TRefArray.h"

#include <algorithm>
#include <set>
#include <vector>

namespace k4SimDelphes {

  using LorentzVectorT = ROOT::Math::PxPyPzEVector;

  /**
   * Get the 4-momentum from an edm4hep type, using the momentum and the energy
   */
  template <typename T> inline LorentzVectorT getP4(const T& particle) {
    return LorentzVectorT{particle.getMomentum()[0], particle.getMomentum()[1], particle.getMomentum()[2],
                          particle.getEnergy()};
  }

  namespace {
    /**
     * Combination of a relative and absolute comparison. The absolute comparison
     * should take precedence if the compared values are small, where relative
     * differences can become very large. The definition of "small" depends on the
     * ratio of relEps / absEps. E.g. for equal values everything for |a|, |b| < 1
     * is considered small. In general max(|a|, |b|) < absEps / relEps defines where
     * "small" begins for inputs a and b
     */
    bool cmp_float(float a, float b, float absEps = 1e-6, float relEps = 1e-6) {
      return std::abs(a - b) <= std::max(absEps, relEps * std::max(std::abs(a), std::abs(b)));
    }
  }  // namespace

  /**
   * Compare two 4-momentum vectors to be "equal" in all their components. The
   * energy requirement can be dropped.
   *
   * The components are compared to be either within a configurable absolute
   * tolerance (default 1e-5) or to have a relative difference smaller than
   * approximately 6e-7
   */
  template <typename LVectorT, typename LVectorU>
  inline bool equalP4(const LVectorT& p1, const LVectorU& p2, double tol = 1e-5, bool checkEnergy = true) {
    const float relEpsilon = 5 * std::numeric_limits<float>::epsilon();
    if (checkEnergy && !cmp_float(p1.E(), p2.E(), tol, relEpsilon))
      return false;
    if (!cmp_float(p1.Px(), p2.Px(), tol, relEpsilon))
      return false;
    if (!cmp_float(p1.Py(), p2.Py(), tol, relEpsilon))
      return false;
    if (!cmp_float(p1.Pz(), p2.Pz(), tol, relEpsilon))
      return false;

    return true;
  }

  /**
   * Get all UniqueIDs of the GenParticles associated with this delphes candidate,
   * where DelphesT is a Delphes output class type. For the internally used
   * Candidate class there is a dedicated overload with a slightly different
   * algorithm
   */
  template <typename DelphesT> std::vector<UInt_t> getAllParticleIDs(DelphesT* delphesCand) {
    std::vector<UInt_t> relatedParticles;

    if constexpr (std::is_same_v<DelphesT, Muon> || std::is_same_v<DelphesT, Electron>) {
      relatedParticles.push_back(delphesCand->Particle.GetUniqueID());
    } else {
      const auto& refArray = delphesCand->Particles;
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
    TIter            it1(candidate->GetCandidates());
    it1.Reset();

    while ((candidate = static_cast<Candidate*>(it1.Next()))) {
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
      while ((candidate = static_cast<Candidate*>(it2.Next()))) {
        relatedParticles.insert(candidate->GetCandidates()->At(0)->GetUniqueID());
      }
    }

    return relatedParticles;
  }

}  // namespace k4SimDelphes

#endif
