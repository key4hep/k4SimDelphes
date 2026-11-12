#ifndef K4GENPARTICLESDELPHESCONVERTER_H
#define K4GENPARTICLESDELPHESCONVERTER_H

#include <cmath>
#include <cstdlib>
#include <unordered_map>

#include "edm4hep/MCParticleCollection.h"

#include "classes/DelphesClasses.h"
#include "classes/DelphesFactory.h"

#include "TObjArray.h"

namespace k4SimDelphes {

/// Speed of light in mm/ns. EDM4hep MCParticle times are in ns, while Delphes expects the
/// time component of Candidate::Position in mm/c (as produced e.g. by Pythia's tProd()).
constexpr double c_light_mm_per_ns = 299.792458;

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
 * No mother / daughter information is set on the Delphes candidates, as it is
 * not needed for running Delphes and is still present in the MCParticles.
 */
inline GenParticleIdMap convertToDelphesArrays(const edm4hep::MCParticleCollection& edmColl, DelphesFactory& factory,
                                               TObjArray& allParticleOutputArray, TObjArray& stableParticleOutputArray,
                                               TObjArray& partonOutputArray) {
  GenParticleIdMap genParticleIds;
  genParticleIds.reserve(edmColl.size());

  for (const auto edmPart : edmColl) {
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

  return genParticleIds;
}

} // namespace k4SimDelphes

#endif
