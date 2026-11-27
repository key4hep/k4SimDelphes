#ifndef K4GENPARTICLESDELPHESCONVERTER_H
#define K4GENPARTICLESDELPHESCONVERTER_H

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
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
 * Pythia conventions that the Delphes modules and the DelphesEDM4HepConverter
 * expect:
 *  - no entries: (-1, -1)
 *  - one entry i: (i, i) for daughters (Pythia daughter1 = daughter2) and
 *    (i, -1) for mothers (Pythia mother2 = 0)
 *  - a contiguous range: (first, last)
 *  - two non-adjacent entries i < j: (j, i), i.e. second < first, which is the
 *    Pythia convention for two separately stored mothers / daughters
 *  - more than two entries that are not contiguous: (first, last), which
 *    includes the particles in between. This is what the Delphes HepMC readers
 *    do as well, since the index pair cannot represent such a set exactly.
 *
 * Note that Delphes modules that walk the daughters (e.g. TauTagging, which
 * loops over D1..D2 and rejects D2 < D1) only work as intended for contiguous
 * daughter ranges. daughterContiguousOrder ensures that the daughters of a
 * particle are contiguous whenever they have no other mothers, which is always
 * the case for decays, e.g. of taus.
 */
inline std::pair<int, int> encodeIndexRange(std::vector<int> indices, bool isDaughters) {
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
 * The parent / daughter relations of the particles in an MCParticle
 * collection, as indices into the collection. Relations to particles that are
 * not in the collection are dropped.
 */
struct CollectionRelations {
  std::vector<std::vector<size_t>> parents{};
  std::vector<std::vector<size_t>> daughters{};

  size_t size() const { return parents.size(); }
};

/**
 * Remove the self-references and the relations closing a cycle from rels. The
 * Delphes modules follow the mother and daughter indices without any cycle
 * protection (e.g. LLPFilter walks the M1 chain, UnstablePropagator recurses
 * over D1..D2), so such relations could make them loop forever. A relation is
 * taken into account if it is present on either side (as a parent or a
 * daughter), and it is removed from both.
 */
inline void removeCycles(CollectionRelations& rels) {
  const auto n = rels.size();
  // all mother -> daughter edges, from both sides of the relations
  std::vector<std::vector<size_t>> edges(rels.daughters);
  for (size_t i = 0; i < n; ++i) {
    for (const auto m : rels.parents[i]) {
      edges[m].push_back(i);
    }
  }

  // iterative depth first search, marking the edges to a particle that is
  // currently on the stack (back edges)
  enum class State : unsigned char { New, OnStack, Done };
  std::vector<State> state(n, State::New);
  std::vector<std::pair<size_t, size_t>> backEdges;
  std::vector<std::pair<size_t, size_t>> stack; // (particle, next edge)
  for (size_t root = 0; root < n; ++root) {
    if (state[root] != State::New) {
      continue;
    }
    stack.emplace_back(root, 0);
    state[root] = State::OnStack;
    while (!stack.empty()) {
      auto& [i, next] = stack.back();
      if (next == edges[i].size()) {
        state[i] = State::Done;
        stack.pop_back();
        continue;
      }
      const auto d = edges[i][next++];
      if (state[d] == State::OnStack) {
        backEdges.emplace_back(i, d);
      } else if (state[d] == State::New) {
        state[d] = State::OnStack;
        stack.emplace_back(d, 0); // invalidates i and next
      }
    }
  }

  for (const auto& [m, d] : backEdges) {
    auto& daughters = rels.daughters[m];
    daughters.erase(std::remove(daughters.begin(), daughters.end(), d), daughters.end());
    auto& parents = rels.parents[d];
    parents.erase(std::remove(parents.begin(), parents.end(), m), parents.end());
  }
}

/**
 * Get the parent / daughter relations within edmColl, without self-references
 * and cycles (see removeCycles).
 */
inline CollectionRelations collectionRelations(const edm4hep::MCParticleCollection& edmColl) {
  const auto n = edmColl.size();
  std::unordered_map<edm4hep::MCParticle, size_t> edmIndex;
  edmIndex.reserve(n);
  for (size_t i = 0; i < n; ++i) {
    edmIndex.emplace(edmColl[i], i);
  }

  CollectionRelations rels;
  rels.parents.resize(n);
  rels.daughters.resize(n);
  const auto fill = [&edmIndex](const auto& related, std::vector<size_t>& indices) {
    for (const auto& rel : related) {
      if (const auto it = edmIndex.find(rel); it != edmIndex.end()) {
        indices.push_back(it->second);
      }
    }
  };
  for (size_t i = 0; i < n; ++i) {
    const auto edmPart = edmColl[i];
    fill(edmPart.getParents(), rels.parents[i]);
    fill(edmPart.getDaughters(), rels.daughters[i]);
  }

  removeCycles(rels);
  return rels;
}

/**
 * Check whether storing the particles at the positions pos (pos[i] is the
 * position of particle i) allows to encode all relations exactly and in a way
 * that the Delphes modules can follow: the daughters of each particle are
 * contiguous and the mothers are contiguous or two separate entries.
 */
inline bool isExactlyEncodable(const CollectionRelations& rels, const std::vector<size_t>& pos) {
  std::vector<size_t> positions;
  const auto contiguous = [&positions, &pos](const std::vector<size_t>& related, bool allowTwo) {
    positions.clear();
    for (const auto i : related) {
      positions.push_back(pos[i]);
    }
    std::sort(positions.begin(), positions.end());
    positions.erase(std::unique(positions.begin(), positions.end()), positions.end());
    return positions.size() <= 1 || (allowTwo && positions.size() == 2) ||
           positions.back() - positions.front() + 1 == positions.size();
  };
  for (size_t i = 0; i < rels.size(); ++i) {
    if (!contiguous(rels.daughters[i], false) || !contiguous(rels.parents[i], true)) {
      return false;
    }
  }
  return true;
}

/**
 * Order in which the MCParticles are put into the Delphes input array, such
 * that the daughters of each particle are stored contiguously as far as
 * possible. This is what Pythia (and the HEPEVT record) does, and it is
 * necessary because the Delphes index pairs (M1, M2) and (D1, D2) can only
 * represent a contiguous range or two separate entries.
 *
 * If the original order already allows this (see isExactlyEncodable), e.g. for
 * inputs from Pythia or HEPEVT, it is kept, so that the Delphes arrays (and
 * hence the order in which Delphes draws random numbers) follow the input. This
 * is not the case e.g. for HepMC3 inputs converted by k4Gen, which have all
 * final state particles at the end. Then the particles without parents come
 * first, in their original order, and the record is built breadth first: when
 * a particle is reached, all its daughters that are not yet placed are
 * appended together. Daughters that are shared by several mothers are placed by
 * the first of them, so a daughter set is only split if it partially overlaps
 * with the daughters of another particle. In particular the daughters of a
 * particle are always contiguous if they have no other mothers. Parent sets are
 * contiguous whenever the parents are daughters of the same particle (e.g. the
 * partons forming a string).
 *
 * Returns the indices into the collection in the order in which they should be
 * stored.
 */
inline std::vector<size_t> daughterContiguousOrder(const CollectionRelations& rels) {
  const auto n = rels.size();
  std::vector<size_t> order(n);
  for (size_t i = 0; i < n; ++i) {
    order[i] = i;
  }
  if (isExactlyEncodable(rels, order)) {
    return order;
  }

  order.clear();
  std::vector<bool> placed(n, false);
  const auto place = [&order, &placed](size_t i) {
    if (!placed[i]) {
      placed[i] = true;
      order.push_back(i);
    }
  };

  for (size_t i = 0; i < n; ++i) {
    if (rels.parents[i].empty()) {
      place(i);
    }
  }
  for (size_t k = 0; k < order.size(); ++k) {
    for (const auto d : rels.daughters[order[k]]) {
      place(d);
    }
  }
  // Anything not reachable from a particle without parents (only possible for
  // inconsistent relations) keeps its relative order at the end
  for (size_t i = 0; i < n; ++i) {
    place(i);
  }
  return order;
}

/**
 * Set the mother / daughter indices (M1, M2, D1, D2) of the Delphes candidates
 * from the relations rels (see collectionRelations). arrayIndex[i] is the
 * index in allParticleOutputArray of the candidate created from particle i.
 *
 * Some Delphes modules need these indices, e.g. TauTagging walks the daughters
 * of the taus to identify hadronic tau decays (without them no tau is ever
 * matched and only the mistag rate remains), and JetFlavorAssociation uses the
 * mothers and daughters of the partons.
 */
inline void setMotherDaughterIndices(const CollectionRelations& rels, const std::vector<int>& arrayIndex,
                                     const TObjArray& allParticleOutputArray) {
  std::vector<int> indices;
  const auto encode = [&arrayIndex, &indices](const std::vector<size_t>& related, bool isDaughters) {
    indices.clear();
    for (const auto i : related) {
      indices.push_back(arrayIndex[i]);
    }
    return encodeIndexRange(indices, isDaughters);
  };

  for (size_t i = 0; i < rels.size(); ++i) {
    auto* candidate = static_cast<Candidate*>(allParticleOutputArray.At(arrayIndex[i]));
    std::tie(candidate->M1, candidate->M2) = encode(rels.parents[i], false);
    std::tie(candidate->D1, candidate->D2) = encode(rels.daughters[i], true);
  }
}

/// The MCParticle a Delphes input candidate was created from
struct GenParticleInput {
  edm4hep::MCParticle particle;
  /// Index of the particle in the collection passed to convertToDelphesArrays
  size_t index;
};

/// Map from the UniqueID of the Delphes input candidates to the MCParticles
/// they were created from
using GenParticleIdMap = std::unordered_map<UInt_t, GenParticleInput>;

/**
 * Convert the passed MCParticles into Delphes input candidates and add them to
 * the Delphes input arrays. Returns the map from the UniqueID of each created
 * candidate to the MCParticle it was created from (and its index in edmColl),
 * which can be passed to
 * DelphesEDM4HepConverter::process() to link the reconstructed particles to
 * the input MCParticles.
 *
 * The candidates are added in the order given by daughterContiguousOrder,
 * which can differ from the order of edmColl, and their mother / daughter
 * indices are set from the MCParticle relations, see setMotherDaughterIndices.
 * Since these indices refer to positions in allParticleOutputArray, it has to
 * be empty when this is called.
 */
inline GenParticleIdMap convertToDelphesArrays(const edm4hep::MCParticleCollection& edmColl, DelphesFactory& factory,
                                               TObjArray& allParticleOutputArray, TObjArray& stableParticleOutputArray,
                                               TObjArray& partonOutputArray) {
  if (allParticleOutputArray.GetEntriesFast() != 0) {
    throw std::invalid_argument("convertToDelphesArrays: allParticleOutputArray has to be empty");
  }

  GenParticleIdMap genParticleIds;
  genParticleIds.reserve(edmColl.size());

  const auto rels = collectionRelations(edmColl);
  std::vector<int> arrayIndex(edmColl.size());
  int nAdded = 0;
  for (const auto iEdm : daughterContiguousOrder(rels)) {
    const auto edmPart = edmColl[iEdm];
    arrayIndex[iEdm] = nAdded++;
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

    genParticleIds.emplace(candidate->GetUniqueID(), GenParticleInput{edmPart, iEdm});

    allParticleOutputArray.Add(candidate);
    const int pdgCode = std::abs(candidate->PID);
    if (candidate->Status == 1) {
      stableParticleOutputArray.Add(candidate);
    } else if (pdgCode <= 5 || pdgCode == 21 || pdgCode == 15) {
      partonOutputArray.Add(candidate);
    }
  }

  setMotherDaughterIndices(rels, arrayIndex, allParticleOutputArray);

  return genParticleIds;
}

} // namespace k4SimDelphes

#endif
