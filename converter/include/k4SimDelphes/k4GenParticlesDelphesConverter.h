#ifndef K4GENPARTICLESDELPHESCONVERTER_H
#define K4GENPARTICLESDELPHESCONVERTER_H

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "edm4hep/MCParticleCollection.h"

#include "classes/DelphesClasses.h"
#include "classes/DelphesFactory.h"

#include "TObjArray.h"

namespace k4SimDelphes {

/// Speed of light in mm/ns. EDM4hep MCParticle times are in ns, while Delphes expects the
/// time component of Candidate::Position in mm/c (as produced e.g. by Pythia's tProd()).
constexpr double c_light_mm_per_ns = 299.792458;

/**
 * Encode a set of indices into the input array as the (first, second) pair of
 * Delphes / Pythia mother (M1, M2) or daughter (D1, D2) indices, following the
 * Pythia conventions that the Delphes modules (e.g. TauTagging, which loops over
 * D1..D2, or JetFlavorAssociation, which reads M1, M2, D1 and D2 individually)
 * and the DelphesEDM4HepConverter expect:
 *  - no entries: (-1, -1)
 *  - one entry i: (i, i) for daughters (Pythia daughter1 = daughter2) and
 *    (i, -1) for mothers (Pythia mother2 = 0)
 *  - a contiguous range: (first, last)
 *  - two non-adjacent entries i < j: (j, i), i.e. second < first, which is the
 *    Pythia convention for two separately stored mothers / daughters
 *  - more than two entries that are not contiguous: (first, last), which
 *    includes the particles in between. This is what the Delphes HepMC readers
 *    do as well, since the index pair cannot represent such a set exactly.
 */
inline std::pair<int, int> encodeIndexRange(std::vector<int>& indices, bool isDaughters) {
  if (indices.empty()) {
    return {-1, -1};
  }
  std::sort(indices.begin(), indices.end());
  indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
  const int first = indices.front();
  const int last = indices.back();
  if (indices.size() == 1) {
    return {first, isDaughters ? first : -1};
  }
  if (indices.size() == 2 && last - first > 1) {
    return {last, first};
  }
  return {first, last};
}

/**
 * Order in which the MCParticles of edmColl are put into the Delphes input
 * array, such that the daughters of each particle are stored contiguously as
 * far as possible. This is what Pythia (and the HEPEVT record) does, and it is
 * necessary because the Delphes index pairs (M1, M2) and (D1, D2) can only
 * represent a contiguous range or two separate entries. The MCParticle order
 * itself does not guarantee this, e.g. HepMC3 inputs converted by k4Gen have
 * all final state particles at the end.
 *
 * The particles without parents in edmColl come first, in their original
 * order. The record is then built breadth first: when a particle is reached,
 * all its daughters that are not yet placed are appended together. Daughters
 * that are shared by several mothers are placed by the first of them, so a
 * daughter set is only split if it partially overlaps with the daughters of
 * another particle. Parent sets are contiguous whenever the parents are
 * daughters of the same particle (e.g. the partons forming a string).
 *
 * Returns the indices into edmColl in the order in which they should be
 * stored.
 */
inline std::vector<size_t> daughterContiguousOrder(const edm4hep::MCParticleCollection& edmColl) {
  const auto n = edmColl.size();
  std::unordered_map<edm4hep::MCParticle, size_t> edmIndex;
  edmIndex.reserve(n);
  for (size_t i = 0; i < n; ++i) {
    edmIndex.emplace(edmColl[i], i);
  }

  std::vector<size_t> order;
  order.reserve(n);
  std::vector<bool> placed(n, false);
  const auto place = [&order, &placed](size_t i) {
    if (!placed[i]) {
      placed[i] = true;
      order.push_back(i);
    }
  };

  for (size_t i = 0; i < n; ++i) {
    const auto parents = edmColl[i].getParents();
    if (std::none_of(parents.begin(), parents.end(), [&edmIndex](const auto& p) { return edmIndex.count(p); })) {
      place(i);
    }
  }
  for (size_t k = 0; k < order.size(); ++k) {
    for (const auto& daughter : edmColl[order[k]].getDaughters()) {
      if (const auto it = edmIndex.find(daughter); it != edmIndex.end()) {
        place(it->second);
      }
    }
  }
  // Anything not reachable from a particle without parents (only possible for
  // inconsistent relations, e.g. cycles) keeps its relative order at the end
  for (size_t i = 0; i < n; ++i) {
    place(i);
  }
  return order;
}

/**
 * Set the mother / daughter indices (M1, M2, D1, D2) of the Delphes candidates
 * created from edmColl from the MCParticle parent / daughter relations.
 * arrayIndex[i] is the index in allParticleOutputArray of the candidate created
 * from edmColl[i]. Only relations to particles in edmColl are taken into
 * account.
 *
 * Some Delphes modules need these indices, e.g. TauTagging walks the daughters
 * of the taus to identify hadronic tau decays (without them no tau is ever
 * matched and only the mistag rate remains), and JetFlavorAssociation uses the
 * mothers and daughters of the partons.
 */
inline void setMotherDaughterIndices(const edm4hep::MCParticleCollection& edmColl, const std::vector<int>& arrayIndex,
                                     const TObjArray& allParticleOutputArray) {
  std::unordered_map<edm4hep::MCParticle, int> particleIndex;
  particleIndex.reserve(edmColl.size());
  for (size_t i = 0; i < edmColl.size(); ++i) {
    particleIndex.emplace(edmColl[i], arrayIndex[i]);
  }

  std::vector<int> indices;
  const auto encode = [&particleIndex, &indices](const auto& related, bool isDaughters) {
    indices.clear();
    for (const auto& rel : related) {
      if (const auto it = particleIndex.find(rel); it != particleIndex.end()) {
        indices.push_back(it->second);
      }
    }
    return encodeIndexRange(indices, isDaughters);
  };

  for (size_t i = 0; i < edmColl.size(); ++i) {
    auto* candidate = static_cast<Candidate*>(allParticleOutputArray.At(arrayIndex[i]));
    const auto edmPart = edmColl[i];
    std::tie(candidate->M1, candidate->M2) = encode(edmPart.getParents(), false);
    std::tie(candidate->D1, candidate->D2) = encode(edmPart.getDaughters(), true);
  }
}

/// Map from the UniqueID of the Delphes input candidates to the MCParticles
/// they were created from
using GenParticleIdMap = std::unordered_map<UInt_t, edm4hep::MCParticle>;

/**
 * Convert the passed MCParticles into Delphes input candidates and add them to
 * the Delphes input arrays. Returns the map from the UniqueID of each created
 * candidate to the MCParticle it was created from, which can be passed to
 * DelphesEDM4HepConverter::process() to link the reconstructed particles to
 * the input MCParticles.
 *
 * The candidates are added in the order given by daughterContiguousOrder,
 * which in general differs from the order of edmColl, and their mother /
 * daughter indices are set from the MCParticle relations, see
 * setMotherDaughterIndices.
 */
inline GenParticleIdMap convertToDelphesArrays(const edm4hep::MCParticleCollection& edmColl, DelphesFactory& factory,
                                               TObjArray& allParticleOutputArray, TObjArray& stableParticleOutputArray,
                                               TObjArray& partonOutputArray) {
  GenParticleIdMap genParticleIds;
  genParticleIds.reserve(edmColl.size());

  const int offset = allParticleOutputArray.GetEntriesFast();
  std::vector<int> arrayIndex(edmColl.size());
  int nAdded = 0;
  for (const auto iEdm : daughterContiguousOrder(edmColl)) {
    const auto edmPart = edmColl[iEdm];
    arrayIndex[iEdm] = offset + nAdded++;
    auto* candidate = factory.NewCandidate();

    const auto mass = edmPart.getMass();
    candidate->Mass = mass;
    const auto mom = edmPart.getMomentum();
    const auto p2 = mom.x * mom.x + mom.y * mom.y + mom.z * mom.z;
    const auto m2 = (mass >= 0) ? mass * mass : -mass * mass;
    candidate->Momentum.SetPxPyPzE(mom.x, mom.y, mom.z, std::sqrt(p2 + m2));

    const auto pos = edmPart.getVertex();
    candidate->Position.SetXYZT(pos.x, pos.y, pos.z, edmPart.getTime() * c_light_mm_per_ns);

    candidate->Charge = edmPart.getCharge();
    candidate->PID = edmPart.getPDG();
    candidate->Status = edmPart.getGeneratorStatus();

    genParticleIds.emplace(candidate->GetUniqueID(), edmPart);

    allParticleOutputArray.Add(candidate);
    const int pdgCode = std::abs(candidate->PID);
    if (candidate->Status == 1) {
      stableParticleOutputArray.Add(candidate);
    } else if (pdgCode <= 5 || pdgCode == 21 || pdgCode == 15) {
      partonOutputArray.Add(candidate);
    }
  }

  setMotherDaughterIndices(edmColl, arrayIndex, allParticleOutputArray);

  return genParticleIds;
}

} // namespace k4SimDelphes

#endif
