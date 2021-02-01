#include <catch2/catch_all.hpp>

#include <string>

#include "k4SimDelphes/k4GenParticlesDelphesConverter.h"
#include "edm4hep/MCParticleCollection.h"
using namespace k4SimDelphes;

TEST_CASE( "k4SimDelphes EDM4hep-Delphes Converter tests", "[converter]" ) {

    SECTION("DelphesEDM4HepConverter ctor");
    //DelphesEDM4HepConverter conv = DelphesEDM4HepConverter("data/delphes_card_IDEA.tcl");

    SECTION("DelphesEDM4HepConverter getCollections");
    //auto coll = conv.getCollections();
    REQUIRE(0 == 0);


}
