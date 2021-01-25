#ifndef K4GENPARTICLESDELPHESCONVERTER_H
#define K4GENPARTICLESDELPHESCONVERTER_H

#include "edm4hep/MCParticleCollection.h"
#include "classes/DelphesFactory.h"
#include "classes/DelphesClasses.h"
#include "TObjArray.h"



class k4GenParticlesDelphesConverter {
public:
  void convertToDelphesArrays(const edm4hep::MCParticleCollection*,
    DelphesFactory& factory,
    TObjArray& allParticleOutputArray,
    TObjArray& stableParticleOutputArray,
    TObjArray& partonOutputArray)
  {
    auto candidate = factory.NewCandidate();

    // do conversion
        //candidate->PID = hepMCPart.pdg_id();
        //candidate->Status = hepMCPart.status();
        //candidate->Mass = hepMCPart.generatedMass();
        //candidate->Momentum.SetPxPyPzE(
        //candidate->Charge = pdgParticle ? int(pdgParticle->Charge() / 3.0) : -999;

    allParticleOutputArray.Add(candidate);
    int pdgCode = TMath::Abs(candidate->PID);
    if (candidate->Status == 1) {
      stableParticleOutputArray.Add(candidate);
    } else if (pdgCode <= 5 || pdgCode == 21 || pdgCode == 15) {
      partonOutputArray.Add(candidate);
    }
  }
};

#endif
