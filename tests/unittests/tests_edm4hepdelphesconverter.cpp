#include <catch2/catch_all.hpp>

#include <algorithm>
#include <array>
#include <climits>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "test_utils.h"

#include "edm4hep/MCParticleCollection.h"
#include "k4SimDelphes/DelphesEDM4HepConverter.h"
#include "k4SimDelphes/k4GenParticlesDelphesConverter.h"
using namespace k4SimDelphes;

TEST_CASE("k4SimDelphes EDM4hep-Delphes Converter tests", "[converter]") {
  SECTION("DelphesEDM4HepConverter ctor") {}
  // DelphesEDM4HepConverter conv = DelphesEDM4HepConverter(getDelphesCard());

  SECTION("DelphesEDM4HepConverter getCollections") {}
  // auto coll = conv.getCollections();
  REQUIRE(0 == 0);
}

TEST_CASE("convertToDelphesArrays returns the UniqueID -> MCParticle map", "[converter]") {
  edm4hep::MCParticleCollection mcParticles;
  auto makeParticle = [&mcParticles](int pdg, int status, float px) {
    auto p = mcParticles.create();
    p.setPDG(pdg);
    p.setGeneratorStatus(status);
    p.setMomentum({px, 0.f, 0.f});
    return p;
  };
  makeParticle(23, 22, 0.f);  // intermediate Z: neither stable nor parton
  makeParticle(5, 23, 10.f);  // b quark: parton
  makeParticle(211, 1, 20.f); // stable pi+
  makeParticle(22, 1, 30.f);  // stable photon

  DelphesFactory factory;
  TObjArray allParticles, stableParticles, partons;
  const auto idMap = convertToDelphesArrays(mcParticles, factory, allParticles, stableParticles, partons);

  REQUIRE(idMap.size() == mcParticles.size());
  REQUIRE(allParticles.GetEntries() == 4);
  REQUIRE(stableParticles.GetEntries() == 2);
  REQUIRE(partons.GetEntries() == 1);

  // Every Delphes candidate maps back to the MCParticle it was created from
  for (int i = 0; i < allParticles.GetEntries(); ++i) {
    const auto* cand = static_cast<const Candidate*>(allParticles.At(i));
    const auto it = idMap.find(cand->GetUniqueID());
    REQUIRE(it != idMap.end());
    REQUIRE(it->second == mcParticles[i]);
    REQUIRE(cand->PID == mcParticles[i].getPDG());
    REQUIRE(cand->Momentum.Px() == Catch::Approx(mcParticles[i].getMomentum().x));
  }
  REQUIRE(idMap.at(static_cast<const Candidate*>(partons.At(0))->GetUniqueID()).getPDG() == 5);
}

TEST_CASE("convertToDelphesArrays converts the production time from ns to mm/c", "[converter]") {
  edm4hep::MCParticleCollection mcParticles;
  auto p = mcParticles.create();
  p.setPDG(211);
  p.setGeneratorStatus(1);
  p.setMomentum({1.f, 0.f, 0.f});
  p.setVertex({1.0, 2.0, 3.0});
  p.setTime(0.5f); // ns

  DelphesFactory factory;
  TObjArray allParticles, stableParticles, partons;
  convertToDelphesArrays(mcParticles, factory, allParticles, stableParticles, partons);

  REQUIRE(allParticles.GetEntries() == 1);
  const auto* cand = static_cast<const Candidate*>(allParticles.At(0));
  REQUIRE(cand->Position.X() == Catch::Approx(1.0));
  REQUIRE(cand->Position.Y() == Catch::Approx(2.0));
  REQUIRE(cand->Position.Z() == Catch::Approx(3.0));
  // Delphes uses mm/c for the time component: t [mm/c] = t [ns] * c [mm/ns]
  REQUIRE(cand->Position.T() == Catch::Approx(0.5 * c_light_mm_per_ns));
}

TEST_CASE("encodeIndexRange follows the Pythia mother / daughter conventions", "[converter]") {
  using V = std::vector<int>;
  const auto enc = [](V v, bool daughters) { return encodeIndexRange(v, daughters); };
  REQUIRE(enc({}, true) == std::pair{-1, -1});
  REQUIRE(enc({}, false) == std::pair{-1, -1});
  REQUIRE(enc({4}, true) == std::pair{4, 4});        // one daughter: D1 == D2
  REQUIRE(enc({4}, false) == std::pair{4, -1});      // one mother: M2 == -1
  REQUIRE(enc({5, 3, 4}, true) == std::pair{3, 5});  // contiguous range, any order
  REQUIRE(enc({3, 4}, false) == std::pair{3, 4});    // two adjacent: range
  REQUIRE(enc({7, 3}, true) == std::pair{7, 3});     // two separate: second < first
  REQUIRE(enc({2, 9, 5}, false) == std::pair{2, 9}); // not representable: enclosing range
  REQUIRE(enc({3, 3, 4}, true) == std::pair{3, 4});  // duplicates are ignored
}

namespace {
using IndexSet = std::set<int>;

template <typename RelatedT>
IndexSet indexSet(const RelatedT& related) {
  IndexSet indices;
  for (const auto& p : related) {
    indices.insert(p.getObjectID().index);
  }
  return indices;
}

} // namespace

namespace {
/// Build the output relations directly from (M1, M2, D1, D2) per particle
edm4hep::MCParticleCollection fromIndices(const std::vector<std::array<int, 4>>& indices) {
  std::vector<GenParticle> genParticles(indices.size());
  std::vector<const GenParticle*> ptrs;
  edm4hep::MCParticleCollection coll;
  for (size_t i = 0; i < indices.size(); ++i) {
    std::tie(genParticles[i].M1, genParticles[i].M2, genParticles[i].D1, genParticles[i].D2) =
        std::tuple{indices[i][0], indices[i][1], indices[i][2], indices[i][3]};
    ptrs.push_back(&genParticles[i]);
    coll.create();
  }
  setMotherDaughterRelations(ptrs, coll);
  return coll;
}

void requireConsistent(const edm4hep::MCParticleCollection& coll) {
  for (size_t i = 0; i < coll.size(); ++i) {
    INFO("particle " << i);
    for (const auto& d : coll[i].getDaughters()) {
      REQUIRE(indexSet(d.getParents()).count(static_cast<int>(i)));
    }
    for (const auto& m : coll[i].getParents()) {
      REQUIRE(indexSet(m.getDaughters()).count(static_cast<int>(i)));
    }
  }
}
} // namespace

// The following cases are taken from Whizard + Pythia6 e+e- -> nu nu H events
// (see key4hep/k4SimDelphes#164)
TEST_CASE("HEPEVT: a daughter that only references its mother is kept (ISR photon)", "[converter]") {
  // 0: e- with daughters (1, 1), but the ISR photon 2 has mother 0 as well
  const auto coll = fromIndices({{-1, -1, 1, 1}, {0, -1, -1, -1}, {0, -1, -1, -1}});
  REQUIRE(indexSet(coll[0].getDaughters()) == IndexSet{1, 2});
  REQUIRE(indexSet(coll[2].getParents()) == IndexSet{0});
  requireConsistent(coll);
}

TEST_CASE("HEPEVT: daughter ranges of interleaved showers are not taken literally", "[converter]") {
  // 0: b -> 2 5, 1: bbar -> 3 4, but stored as the ranges (2, 5) and (3, 4)
  const auto coll =
      fromIndices({{-1, -1, 2, 5}, {-1, -1, 3, 4}, {0, -1, -1, -1}, {1, -1, -1, -1}, {1, -1, -1, -1}, {0, -1, -1, -1}});
  REQUIRE(indexSet(coll[0].getDaughters()) == IndexSet{2, 5});
  REQUIRE(indexSet(coll[1].getDaughters()) == IndexSet{3, 4});
  REQUIRE(indexSet(coll[3].getParents()) == IndexSet{1});
  requireConsistent(coll);
}

TEST_CASE("HepMC: two mothers stored as a range do not include the particles in between", "[converter]") {
  // 0: e- -> 1 2 (e-, ISR photon), 1 and 3: e-, e+ -> 4 (stored with mothers (1, 3))
  const auto coll = fromIndices({{-1, -1, 1, 2}, {0, -1, 4, 4}, {0, -1, -1, -1}, {-1, -1, 4, 4}, {1, 3, -1, -1}});
  REQUIRE(indexSet(coll[4].getParents()) == IndexSet{1, 3});
  REQUIRE(coll[2].getDaughters().empty());
  requireConsistent(coll);
}

TEST_CASE("Two separately stored mothers keep the (M1, M2) order", "[converter]") {
  // 3 has the mothers (2, 0), i.e. M1 > M2
  const auto coll = fromIndices({{-1, -1, 3, 3}, {-1, -1, -1, -1}, {-1, -1, 3, 3}, {2, 0, -1, -1}});
  REQUIRE(coll[3].getParents().size() == 2);
  REQUIRE(coll[3].getParents(0).getObjectID().index == 2);
  REQUIRE(coll[3].getParents(1).getObjectID().index == 0);
  requireConsistent(coll);
}

TEST_CASE("Out of bounds and self-referencing indices are ignored", "[converter]") {
  // 0 -> (1, INT_MAX) is clamped to the existing particles, 2 references
  // itself, 3 has the mother 1 and two out of bounds daughters
  const auto coll = fromIndices({{-1, -1, 1, INT_MAX}, {0, -1, -1, -1}, {0, 2, 2, 2}, {1, -1, 100, 50}});
  REQUIRE(indexSet(coll[0].getDaughters()) == IndexSet{1, 2});
  REQUIRE(indexSet(coll[1].getDaughters()) == IndexSet{3});
  REQUIRE(indexSet(coll[2].getParents()) == IndexSet{0});
  REQUIRE(coll[2].getDaughters().empty());
  REQUIRE(coll[3].getDaughters().empty());
  requireConsistent(coll);
}

TEST_CASE("A daughter without mothers is taken from the mother", "[converter]") {
  const auto coll = fromIndices({{-1, -1, 1, 2}, {-1, -1, -1, -1}, {0, -1, -1, -1}});
  REQUIRE(indexSet(coll[0].getDaughters()) == IndexSet{1, 2});
  REQUIRE(indexSet(coll[1].getParents()) == IndexSet{0});
  requireConsistent(coll);
}

namespace {
/// The result of running MCParticles through the Delphes input conversion and
/// back through the output conversion of the mother / daughter relations
struct Roundtrip {
  edm4hep::MCParticleCollection output{};                  ///< in the order of the Delphes input array
  std::vector<int> arrayIndex{};                           ///< input MCParticle index -> Delphes array index
  std::vector<std::pair<int, int>> mothers{}, daughters{}; ///< (M1, M2) / (D1, D2) per input MCParticle

  /// parents / daughters of the output particle for input particle i, as input indices
  template <typename GetterT>
  IndexSet related(size_t i, GetterT getter) const {
    std::vector<int> inputIndex(arrayIndex.size());
    for (size_t j = 0; j < arrayIndex.size(); ++j) {
      inputIndex[arrayIndex[j]] = static_cast<int>(j);
    }
    IndexSet result;
    for (const auto idx : indexSet(getter(output[arrayIndex[i]]))) {
      result.insert(inputIndex[idx]);
    }
    return result;
  }
  IndexSet parents(size_t i) const {
    return related(i, [](const auto& p) { return p.getParents(); });
  }
  IndexSet daughters_(size_t i) const {
    return related(i, [](const auto& p) { return p.getDaughters(); });
  }
};

/// Run the MCParticles through convertToDelphesArrays, copy the mother /
/// daughter indices to Delphes GenParticles (as the Delphes TreeWriter does)
/// and rebuild the relations with setMotherDaughterRelations, as the
/// DelphesEDM4HepConverter does for the output MCParticles.
Roundtrip roundtrip(const edm4hep::MCParticleCollection& mcParticles) {
  DelphesFactory factory;
  TObjArray allParticles, stableParticles, partons;
  const auto idMap = convertToDelphesArrays(mcParticles, factory, allParticles, stableParticles, partons);
  REQUIRE(allParticles.GetEntries() == static_cast<int>(mcParticles.size()));

  Roundtrip result;
  result.arrayIndex.resize(mcParticles.size());
  result.mothers.resize(mcParticles.size());
  result.daughters.resize(mcParticles.size());
  std::vector<GenParticle> genParticles(allParticles.GetEntries());
  std::vector<const GenParticle*> genParticlePtrs;
  for (int j = 0; j < allParticles.GetEntries(); ++j) {
    result.output.create();
    const auto* cand = static_cast<const Candidate*>(allParticles.At(j));
    const auto i = idMap.at(cand->GetUniqueID()).getObjectID().index;
    result.arrayIndex[i] = j;
    result.mothers[i] = {cand->M1, cand->M2};
    result.daughters[i] = {cand->D1, cand->D2};
    genParticles[j].M1 = cand->M1;
    genParticles[j].M2 = cand->M2;
    genParticles[j].D1 = cand->D1;
    genParticles[j].D2 = cand->D2;
    genParticlePtrs.push_back(&genParticles[j]);
  }
  setMotherDaughterRelations(genParticlePtrs, result.output);
  return result;
}

/// Check that all relations to particles in mcParticles survive the roundtrip exactly
void requireExactRoundtrip(const edm4hep::MCParticleCollection& mcParticles, const Roundtrip& rt) {
  REQUIRE(rt.output.size() == mcParticles.size());
  for (size_t i = 0; i < mcParticles.size(); ++i) {
    INFO("particle " << i);
    IndexSet parents, daughters;
    for (const auto& p : mcParticles[i].getParents()) {
      if (std::find(mcParticles.begin(), mcParticles.end(), p) != mcParticles.end()) {
        parents.insert(p.getObjectID().index);
      }
    }
    for (const auto& d : mcParticles[i].getDaughters()) {
      daughters.insert(d.getObjectID().index);
    }
    REQUIRE(rt.parents(i) == parents);
    REQUIRE(rt.daughters_(i) == daughters);
  }
}

edm4hep::MutableMCParticle make(edm4hep::MCParticleCollection& coll, int pdg, int status) {
  auto p = coll.create();
  p.setPDG(pdg);
  p.setGeneratorStatus(status);
  return p;
}

void link(edm4hep::MutableMCParticle& mother, edm4hep::MutableMCParticle& daughter) {
  mother.addToDaughters(daughter);
  daughter.addToParents(mother);
}
} // namespace

TEST_CASE("daughterContiguousOrder stores the daughters of each particle contiguously", "[converter]") {
  // HepMC3-like order with the final state particles at the end:
  // 0 -> 1 2, 1 -> 3 5, 2 -> 4 6, 3 -> 7 8 9 (omega -> pi0 pi+ pi-), 4 -> 10 11 12
  edm4hep::MCParticleCollection mcParticles;
  std::vector<edm4hep::MutableMCParticle> p;
  for (int i = 0; i < 13; ++i) {
    p.push_back(make(mcParticles, 100 + i, i < 5 ? 2 : 1));
  }
  link(p[0], p[1]);
  link(p[0], p[2]);
  link(p[1], p[3]);
  link(p[1], p[5]);
  link(p[2], p[4]);
  link(p[2], p[6]);
  for (int d : {7, 8, 9}) {
    link(p[3], p[d]);
  }
  for (int d : {10, 11, 12}) {
    link(p[4], p[d]);
  }

  REQUIRE(daughterContiguousOrder(collectionRelations(mcParticles)) ==
          std::vector<size_t>{0, 1, 2, 3, 5, 4, 6, 7, 8, 9, 10, 11, 12});
}

TEST_CASE("Mother / daughter relations survive the roundtrip through the Delphes indices", "[converter]") {
  edm4hep::MCParticleCollection mcParticles;
  auto tau = make(mcParticles, 15, 2);    // 0
  auto nu = make(mcParticles, 16, 1);     // 1
  auto rho = make(mcParticles, 213, 2);   // 2
  auto gl = make(mcParticles, 21, 51);    // 3: shower gluon, daughters not adjacent in the input
  auto pip = make(mcParticles, 211, 1);   // 4
  auto pi0 = make(mcParticles, 111, 1);   // 5
  auto g1 = make(mcParticles, 21, 51);    // 6
  auto other = make(mcParticles, 22, 1);  // 7: unrelated, sits between the gluon daughters
  auto g2 = make(mcParticles, 21, 51);    // 8
  auto q = make(mcParticles, 1, 71);      // 9: string q g qbar
  auto g = make(mcParticles, 21, 71);     // 10
  auto qbar = make(mcParticles, -1, 71);  // 11
  auto had1 = make(mcParticles, 211, 1);  // 12: hadron from the two shower gluons
  auto had2 = make(mcParticles, -211, 1); // 13: hadron from the string
  auto omega = make(mcParticles, 223, 2); // 14: omega -> pi0 pi+ pi-, with the pions spread out
  auto pi0b = make(mcParticles, 111, 1);  // 15
  auto filler = make(mcParticles, 22, 1); // 16
  auto pipb = make(mcParticles, 211, 1);  // 17
  auto pimb = make(mcParticles, -211, 1); // 18

  link(tau, nu);
  link(tau, rho);
  link(rho, pi0); // added out of order
  link(rho, pip);
  link(gl, g2);
  link(gl, g1);
  link(g1, had1);
  link(g2, had1);
  for (auto* parton : {&q, &g, &qbar}) {
    link(*parton, had2);
  }
  link(omega, pi0b);
  link(omega, pipb);
  link(omega, pimb);
  // a relation to a particle that is not in the input collection is dropped
  edm4hep::MCParticleCollection otherColl;
  other.addToParents(otherColl.create());

  const auto rt = roundtrip(mcParticles);
  const auto a = [&rt](int i) { return rt.arrayIndex[i]; };

  SECTION("Delphes indices are contiguous ranges") {
    REQUIRE(rt.mothers[0] == std::pair{-1, -1});
    REQUIRE(rt.daughters[0] == std::pair{a(1), a(2)}); // walked by Delphes' TauTagging
    REQUIRE(rt.mothers[1] == std::pair{a(0), -1});
    REQUIRE(rt.daughters[1] == std::pair{-1, -1});
    REQUIRE(rt.daughters[2].second - rt.daughters[2].first == 1);
    REQUIRE(rt.daughters[3].second - rt.daughters[3].first == 1);
    REQUIRE(rt.mothers[12].second - rt.mothers[12].first == 1);
    REQUIRE(rt.daughters[14].second - rt.daughters[14].first == 2);
    REQUIRE(rt.mothers[7] == std::pair{-1, -1});
  }

  SECTION("Relations are reproduced exactly") { requireExactRoundtrip(mcParticles, rt); }
}

TEST_CASE("Hadron parents that are not daughters of the same particle are recovered from the enclosing range",
          "[converter]") {
  // The (first, second) index pair cannot represent such a set, so the input is
  // encoded as the enclosing range, as the Delphes HepMC readers do. Since the
  // particles in between do not list the hadron as a daughter,
  // setMotherDaughterRelations drops them again.
  edm4hep::MCParticleCollection mcParticles;
  std::vector<edm4hep::MutableMCParticle> particles;
  for (int i = 0; i < 6; ++i) {
    particles.push_back(make(mcParticles, 1, 71));
  }
  auto& hadron = particles[5];
  for (int i : {0, 2, 4}) {
    link(particles[i], hadron);
  }

  const auto rt = roundtrip(mcParticles);
  REQUIRE(rt.mothers[5] == std::pair{rt.arrayIndex[0], rt.arrayIndex[4]});
  REQUIRE(rt.parents(5) == IndexSet{0, 2, 4});
  for (int i : {0, 2, 4}) {
    REQUIRE(rt.daughters_(i) == IndexSet{5});
  }
  for (int i : {1, 3}) {
    REQUIRE(rt.daughters_(i).empty());
  }
}

TEST_CASE("Self-references and cycles are not passed on to the Delphes indices", "[converter]") {
  edm4hep::MCParticleCollection mcParticles;
  auto p0 = make(mcParticles, 23, 2);
  auto p1 = make(mcParticles, 5, 2);
  auto p2 = make(mcParticles, -5, 2);
  link(p0, p1);
  link(p1, p1); // self-reference
  link(p1, p2);
  p0.addToParents(p2); // 2-cycle 0 -> 1 -> 2 -> 0, only on the parent side

  DelphesFactory factory;
  TObjArray allParticles, stableParticles, partons;
  convertToDelphesArrays(mcParticles, factory, allParticles, stableParticles, partons);
  // the indices must not form a cycle, i.e. walking the M1 chain (as LLPFilter
  // does) has to terminate
  for (int j = 0; j < allParticles.GetEntriesFast(); ++j) {
    const auto* cand = static_cast<const Candidate*>(allParticles.At(j));
    REQUIRE(cand->M1 != j);
    REQUIRE(cand->D1 != j);
    int steps = 0;
    for (const auto* c = cand; c->M1 >= 0; c = static_cast<const Candidate*>(allParticles.At(c->M1))) {
      REQUIRE(++steps <= allParticles.GetEntriesFast());
    }
  }
}

TEST_CASE("convertToDelphesArrays requires an empty particle array", "[converter]") {
  edm4hep::MCParticleCollection mcParticles;
  make(mcParticles, 22, 1);
  DelphesFactory factory;
  TObjArray allParticles, stableParticles, partons;
  allParticles.Add(factory.NewCandidate());
  REQUIRE_THROWS_AS(convertToDelphesArrays(mcParticles, factory, allParticles, stableParticles, partons),
                    std::invalid_argument);
}

TEST_CASE("The input order is kept if all relations can be encoded exactly", "[converter]") {
  // Pythia-like record: 0 -> 1 2, 1 -> 3 4, 2 -> 5, the string 3 4 -> 6 7
  edm4hep::MCParticleCollection mcParticles;
  std::vector<edm4hep::MutableMCParticle> p;
  for (int i = 0; i < 8; ++i) {
    p.push_back(make(mcParticles, 1, 2));
  }
  link(p[0], p[1]);
  link(p[0], p[2]);
  link(p[1], p[3]);
  link(p[1], p[4]);
  link(p[2], p[5]);
  for (int m : {3, 4}) {
    for (int d : {6, 7}) {
      link(p[m], p[d]);
    }
  }
  REQUIRE(daughterContiguousOrder(collectionRelations(mcParticles)) == std::vector<size_t>{0, 1, 2, 3, 4, 5, 6, 7});

  const auto rt = roundtrip(mcParticles);
  for (int i = 0; i < 8; ++i) {
    REQUIRE(rt.arrayIndex[i] == i);
  }
  requireExactRoundtrip(mcParticles, rt);
}

TEST_CASE("Enclosing ranges that cover each other can add relations, copying from the input does not", "[converter]") {
  // Relations 0->3, 0->6, 1->3, 1->4, 1->5, 2->6, 5->6: the daughters of 1 and
  // the parents of 6 cannot both be stored contiguously, and their enclosing
  // ranges cover each other
  edm4hep::MCParticleCollection mcParticles;
  std::vector<edm4hep::MutableMCParticle> p;
  for (int i = 0; i < 7; ++i) {
    p.push_back(make(mcParticles, 1, 2));
  }
  for (const auto [m, d] : std::vector<std::pair<int, int>>{{0, 3}, {0, 6}, {1, 3}, {1, 4}, {1, 5}, {2, 6}, {5, 6}}) {
    link(p[m], p[d]);
  }

  // Through the Delphes indices the relations stay consistent, but are not
  // necessarily exact
  const auto rt = roundtrip(mcParticles);
  requireConsistent(rt.output);

  // In the framework the output takes the relations from the input
  edm4hep::MCParticleCollection output;
  std::vector<edm4hep::MCParticle> inputs;
  for (const auto& mcp : mcParticles) {
    output.create();
    inputs.push_back(mcp);
  }
  copyMotherDaughterRelations(inputs, output);
  for (size_t i = 0; i < mcParticles.size(); ++i) {
    REQUIRE(indexSet(output[i].getParents()) == indexSet(mcParticles[i].getParents()));
    REQUIRE(indexSet(output[i].getDaughters()) == indexSet(mcParticles[i].getDaughters()));
  }
}
