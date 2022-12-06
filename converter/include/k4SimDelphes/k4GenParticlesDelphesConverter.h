#ifndef K4GENPARTICLESDELPHESCONVERTER_H
#define K4GENPARTICLESDELPHESCONVERTER_H

#include <unordered_map>

#include "edm4hep/MCParticleCollection.h"

#include "classes/DelphesClasses.h"
#include "classes/DelphesFactory.h"

#include "TObjArray.h"

// todo: move to converter and test

namespace k4SimDelphes {

  class k4GenParticlesDelphesConverter {
  public:
    void convertToDelphesArrays(const edm4hep::MCParticleCollection* edm_coll, DelphesFactory& factory,
                                TObjArray& allParticleOutputArray, TObjArray& stableParticleOutputArray,
                                TObjArray& partonOutputArray) {
      m_genParticleIds.clear();
      // loop over all input edm4hep particles
      for (auto&& edm_part : *edm_coll) {
        // create the delphes particle to be filled from edm4hep
        auto candidate = factory.NewCandidate();
        // Setting Momentum with the help of some auxiliary variables
        auto _M         = edm_part.getMass();
        candidate->Mass = _M;
        auto edm_mom    = edm_part.getMomentum();
        auto _P2        = edm_mom.x * edm_mom.x + edm_mom.y * edm_mom.y + edm_mom.z * edm_mom.z;
        auto _M2        = (_M >= 0) ? _M * _M : -_M * _M;
        candidate->Momentum.SetPxPyPzE(edm_mom.x, edm_mom.y, edm_mom.z, std::sqrt(_P2 + _M2));
        // Setting Position
        auto edm_pos = edm_part.getVertex();
        candidate->Position.SetXYZT(edm_pos.x, edm_pos.y, edm_pos.z, edm_part.getTime());
        // Setting other members with a one-to-one correspondence
        candidate->Charge = edm_part.getCharge();
        candidate->PID    = edm_part.getPDG();
        candidate->Status = edm_part.getGeneratorStatus();
        m_genParticleIds.emplace(candidate->GetUniqueID(), edm_part);

        // candidate has all necessary infos, add it to delphes arrays
        allParticleOutputArray.Add(candidate);
        int pdgCode = TMath::Abs(candidate->PID);
        if (candidate->Status == 1) {
          stableParticleOutputArray.Add(candidate);
        } else if (pdgCode <= 5 || pdgCode == 21 || pdgCode == 15) {
          partonOutputArray.Add(candidate);
        }
      }
      // no mother / daughter information is set in the Delphes GenParticles
      // as it is not needed for running delphes, and still present in edm4hep.
      //
    }

    std::unordered_map<UInt_t, edm4hep::MCParticle> getGenParticleIdMap() { return m_genParticleIds; }

  private:
    std::unordered_map<UInt_t, edm4hep::MCParticle> m_genParticleIds;
  };

}  // namespace k4SimDelphes

#endif
