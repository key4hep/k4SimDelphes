#include <catch2/catch_all.hpp>

#include <array>
#include <set>
#include <string>
#include <tuple>
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

TEST_CASE("A daughter without mothers is taken from the mother", "[converter]") {
  const auto coll = fromIndices({{-1, -1, 1, 2}, {-1, -1, -1, -1}, {0, -1, -1, -1}});
  REQUIRE(indexSet(coll[0].getDaughters()) == IndexSet{1, 2});
  REQUIRE(indexSet(coll[1].getParents()) == IndexSet{0});
  requireConsistent(coll);
}
