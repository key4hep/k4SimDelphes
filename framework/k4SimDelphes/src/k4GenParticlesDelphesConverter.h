#ifndef K4GENPARTICLESDELPHESCONVERTER_H
#define K4GENPARTICLESDELPHESCONVERTER_H

#include "edm4hep/MCParticleCollection.h"
#include "classes/DelphesFactory.h"
#include "classes/DelphesClasses.h"
#include "TObjArray.h"




class k4GenParticlesDelphesConverter {
public:

  /// Map to store vertex to particle relations
  typedef std::map<int, std::pair<int, int>> VertexParticleMap;

  void convertToDelphesArrays(
    const edm4hep::MCParticleCollection* edm_coll,
    DelphesFactory& factory,
    TObjArray& allParticleOutputArray,
    TObjArray& stableParticleOutputArray,
    TObjArray& partonOutputArray)
  {

    VertexParticleMap motherMap;
    // Vertex ID -> first daughter particle, last daughter particle
    VertexParticleMap daughterMap;

    for (auto&& edm_part: *edm_coll) {

      auto candidate = factory.NewCandidate();

      candidate->PID = edm_part.getPDG();
      candidate->Status = edm_part.getGeneratorStatus();
      auto _M = edm_part.getMass();
      candidate->Mass = _M;
      auto edm_mom = edm_part.getMomentum();
      auto _P2 = edm_mom.x*edm_mom.x +
                 edm_mom.y*edm_mom.y +
                 edm_mom.z*edm_mom.z;
      auto _M2 = ( _M  >= 0 ) ?  _M*_M :  -_M*_M;
      candidate->Momentum.SetPxPyPzE(edm_mom.x,
                                     edm_mom.y,
                                     edm_mom.z,
                                     std::sqrt(_P2 + _M2));
      candidate->Charge = edm_part.getCharge();

      // add candidate to delphes arrays
      allParticleOutputArray.Add(candidate);
      int pdgCode = TMath::Abs(candidate->PID);
      if (candidate->Status == 1) {
        stableParticleOutputArray.Add(candidate);
      } else if (pdgCode <= 5 || pdgCode == 21 || pdgCode == 15) {
        partonOutputArray.Add(candidate);
      }
    }
  setRelationIndices(allParticleOutputArray, daughterMap, motherMap);
  };


  /// Sets relations of particles
  void setRelationIndices(TObjArray& allParticleOutputArray,
                          const VertexParticleMap& daughterMap,
                          const VertexParticleMap& motherMap) const {

                          };

};

#endif
