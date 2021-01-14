#ifndef _K4SIMDELPHESALG_H
#define _K4SIMDELPHESALG_H

#include "k4FWCore/DataHandle.h"
#include "GaudiAlg/GaudiAlgorithm.h"

#include "edm4hep/MCParticleCollection.h"
#include "edm4hep/ReconstructedParticleCollection.h"

namespace edm4hep {
class MCParticleCollection;
}

/**

**/

class k4SimDelphesAlg : public GaudiAlgorithm {

public:
  k4SimDelphesAlg(const std::string& name, ISvcLocator* svcLoc);

  virtual StatusCode initialize();
  virtual StatusCode execute();
  virtual StatusCode finalize();

private:
  DataHandle<edm4hep::MCParticleCollection> m_InputMCParticles{"GenParticles", Gaudi::DataHandle::Reader, this};

  DataHandle<edm4hep::ReconstructedParticleCollection> m_OutputRecParticles{"RecParticlesDelphes", Gaudi::DataHandle::Writer, this};

};

#endif // _K4SIMDELPHESALG_H
