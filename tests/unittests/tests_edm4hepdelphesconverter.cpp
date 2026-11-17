#include <catch2/catch_all.hpp>

#include <algorithm>
#include <set>
#include <string>
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
  std::vector<edm4hep::MutableMCParticle> outParticles;
  for (int j = 0; j < allParticles.GetEntries(); ++j) {
    outParticles.push_back(result.output.create());
    const auto* cand = static_cast<const Candidate*>(allParticles.At(j));
    const auto i = idMap.at(cand->GetUniqueID()).getObjectID().index;
    result.arrayIndex[i] = j;
    result.mothers[i] = {cand->M1, cand->M2};
    result.daughters[i] = {cand->D1, cand->D2};
  }
  for (int j = 0; j < allParticles.GetEntries(); ++j) {
    const auto* cand = static_cast<const Candidate*>(allParticles.At(j));
    GenParticle genParticle;
    genParticle.M1 = cand->M1;
    genParticle.M2 = cand->M2;
    genParticle.D1 = cand->D1;
    genParticle.D2 = cand->D2;
    setMotherDaughterRelations(&genParticle, outParticles[j], result.output);
  }
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

  REQUIRE(daughterContiguousOrder(mcParticles) == std::vector<size_t>{0, 1, 2, 3, 5, 4, 6, 7, 8, 9, 10, 11, 12});
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

TEST_CASE("Hadron parents that are not daughters of the same particle become the enclosing range", "[converter]") {
  // The (first, second) index pair cannot represent such a set; the Delphes
  // HepMC readers resolve this in the same way
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
  REQUIRE(rt.parents(5) == IndexSet{0, 1, 2, 3, 4});
  for (int i : {0, 2, 4}) {
    REQUIRE(rt.daughters_(i) == IndexSet{5});
  }
  for (int i : {1, 3}) {
    REQUIRE(rt.daughters_(i).empty());
  }
}
