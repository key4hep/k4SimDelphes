#include <catch2/catch_all.hpp>

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

/// Run the MCParticles through convertToDelphesArrays, copy the mother /
/// daughter indices to Delphes GenParticles (as the Delphes TreeWriter does)
/// and rebuild the relations with setMotherDaughterRelations, as the
/// DelphesEDM4HepConverter does for the output MCParticles.
edm4hep::MCParticleCollection roundtrip(const edm4hep::MCParticleCollection& mcParticles) {
  DelphesFactory factory;
  TObjArray allParticles, stableParticles, partons;
  convertToDelphesArrays(mcParticles, factory, allParticles, stableParticles, partons);
  REQUIRE(allParticles.GetEntries() == static_cast<int>(mcParticles.size()));

  edm4hep::MCParticleCollection output;
  std::vector<edm4hep::MutableMCParticle> outParticles;
  for (size_t i = 0; i < mcParticles.size(); ++i) {
    outParticles.push_back(output.create());
  }
  for (size_t i = 0; i < mcParticles.size(); ++i) {
    const auto* cand = static_cast<const Candidate*>(allParticles.At(static_cast<int>(i)));
    GenParticle genParticle;
    genParticle.M1 = cand->M1;
    genParticle.M2 = cand->M2;
    genParticle.D1 = cand->D1;
    genParticle.D2 = cand->D2;
    setMotherDaughterRelations(&genParticle, outParticles[i], output);
  }
  return output;
}
} // namespace

TEST_CASE("Mother / daughter relations survive the roundtrip through the Delphes indices", "[converter]") {
  edm4hep::MCParticleCollection mcParticles;
  auto make = [&mcParticles](int pdg, int status) {
    auto p = mcParticles.create();
    p.setPDG(pdg);
    p.setGeneratorStatus(status);
    return p;
  };
  const auto link = [](auto& mother, auto& daughter) {
    mother.addToDaughters(daughter);
    daughter.addToParents(mother);
  };

  auto tau = make(15, 2);    // 0
  auto nu = make(16, 1);     // 1
  auto rho = make(213, 2);   // 2
  auto gl = make(21, 51);    // 3: shower gluon with two non-adjacent daughters
  auto pip = make(211, 1);   // 4
  auto pi0 = make(111, 1);   // 5
  auto g1 = make(21, 51);    // 6
  auto other = make(22, 1);  // 7: unrelated, sits between the gluon daughters
  auto g2 = make(21, 51);    // 8
  auto q = make(1, 71);      // 9: string q g qbar, stored contiguously
  auto g = make(21, 71);     // 10
  auto qbar = make(-1, 71);  // 11
  auto had1 = make(211, 1);  // 12: hadrons from the string
  auto had2 = make(-211, 1); // 13

  link(tau, nu);
  link(tau, rho);
  link(rho, pi0); // added out of order
  link(rho, pip);
  link(gl, g2);
  link(gl, g1);
  // two non-adjacent mothers: encoded as M1 > M2
  link(g1, had1);
  link(g2, had1);
  for (auto* parton : {&q, &g, &qbar}) {
    link(*parton, had2);
  }
  // a relation to a particle that is not in the input collection is dropped
  edm4hep::MCParticleCollection otherColl;
  other.addToParents(otherColl.create());

  DelphesFactory factory;
  TObjArray allParticles, stableParticles, partons;
  convertToDelphesArrays(mcParticles, factory, allParticles, stableParticles, partons);
  const auto cand = [&allParticles](int i) { return static_cast<const Candidate*>(allParticles.At(i)); };

  SECTION("Delphes indices") {
    // tau -> nu rho: contiguous daughters, as walked by Delphes' TauTagging
    REQUIRE(std::pair{cand(0)->M1, cand(0)->M2} == std::pair{-1, -1});
    REQUIRE(std::pair{cand(0)->D1, cand(0)->D2} == std::pair{1, 2});
    REQUIRE(std::pair{cand(1)->M1, cand(1)->M2} == std::pair{0, -1});
    REQUIRE(std::pair{cand(1)->D1, cand(1)->D2} == std::pair{-1, -1});
    REQUIRE(std::pair{cand(2)->D1, cand(2)->D2} == std::pair{4, 5});
    // g -> g g with an unrelated particle in between: D1 > D2
    REQUIRE(std::pair{cand(3)->D1, cand(3)->D2} == std::pair{8, 6});
    REQUIRE(std::pair{cand(12)->M1, cand(12)->M2} == std::pair{8, 6});
    REQUIRE(std::pair{cand(13)->M1, cand(13)->M2} == std::pair{9, 11});
    REQUIRE(std::pair{cand(7)->M1, cand(7)->M2} == std::pair{-1, -1});
  }

  SECTION("Relations are reproduced exactly") {
    const auto output = roundtrip(mcParticles);
    REQUIRE(output.size() == mcParticles.size());
    for (size_t i = 0; i < mcParticles.size(); ++i) {
      INFO("particle " << i);
      if (i == 7) {
        // the relation to the particle outside of the collection is dropped
        REQUIRE(output[i].getParents().empty());
      } else {
        REQUIRE(indexSet(output[i].getParents()) == indexSet(mcParticles[i].getParents()));
      }
      REQUIRE(indexSet(output[i].getDaughters()) == indexSet(mcParticles[i].getDaughters()));
    }
  }
}

TEST_CASE("More than two non-contiguous relations become the enclosing range in the roundtrip", "[converter]") {
  // The (first, second) index pair cannot represent such a set; the Delphes
  // HepMC readers resolve this in the same way. In HepMC-ordered input this
  // only happens for the parents of string-fragmentation hadrons.
  edm4hep::MCParticleCollection mcParticles;
  std::vector<edm4hep::MutableMCParticle> particles;
  for (int i = 0; i < 6; ++i) {
    particles.push_back(mcParticles.create());
  }
  auto& hadron = particles[5];
  for (int i : {0, 2, 4}) {
    particles[i].addToDaughters(hadron);
    hadron.addToParents(particles[i]);
  }

  const auto output = roundtrip(mcParticles);
  REQUIRE(indexSet(output[5].getParents()) == IndexSet{0, 1, 2, 3, 4});
  for (int i : {0, 2, 4}) {
    REQUIRE(indexSet(output[i].getDaughters()) == IndexSet{5});
  }
  for (int i : {1, 3}) {
    REQUIRE(output[i].getDaughters().empty());
  }
}
