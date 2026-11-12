#include <catch2/catch_all.hpp>

#include <string>

#include "test_utils.h"

#include "edm4hep/MCParticleCollection.h"
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
