//#define CATCH_CONFIG_FAST_COMPILE 
#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

#include <string>

#include "k4SimDelphes/DelphesEDM4HepConverter.h"
#include "k4SimDelphes/DelphesEDM4HepOutputConfiguration.h"
using namespace k4SimDelphes;

TEST_CASE( "k4SimDelphes Converter Tests", "[converter]" ) {

    SECTION("DelphesEDM4HepConverter ctor");
    DelphesEDM4HepConverter conv = DelphesEDM4HepConverter("data/delphes_card_IDEA.tcl");

    SECTION("DelphesEDM4HepConverter getCollections");
    auto coll = conv.getCollections();
    REQUIRE(coll.size() == 0);


}
