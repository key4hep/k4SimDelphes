#ifndef _K4SIMDELPHESALG_H
#define _K4SIMDELPHESALG_H

#include "k4FWCore/DataHandle.h"
#include "GaudiAlg/GaudiAlgorithm.h"

#include "edm4hep/MCParticleCollection.h"
#include "edm4hep/ReconstructedParticleCollection.h"

#include "modules/Delphes.h"

namespace edm4hep {
class MCParticleCollection;
}

/** @class k4SimDelphesAlg
 *
 *  Main Algorithm to run Delphes, getting MCParticle input, producing 
 *  ReconstructedParticle output.
**/
class k4SimDelphesAlg : public GaudiAlgorithm {

public:
  k4SimDelphesAlg(const std::string& name, ISvcLocator* svcLoc);

  virtual StatusCode initialize();
  virtual StatusCode execute();
  virtual StatusCode finalize();

private:
  /// Input from Generator
  DataHandle<edm4hep::MCParticleCollection> m_InputMCParticles{"GenParticles", Gaudi::DataHandle::Reader, this};
  /// Output from Delphes
  DataHandle<edm4hep::ReconstructedParticleCollection> m_OutputRecParticles{"RecParticlesDelphes", Gaudi::DataHandle::Writer, this};

   // Delphes detector card to be read in
  /// Name of Delphes tcl config file with detector and simulation parameters
  Gaudi::Property<std::string> m_DelphesCard{this, "DelphesCard", "",
                                             "Name of Delphes tcl config file with detector and simulation parameters"};
  Gaudi::Property<std::string> m_DelphesOutputSettings{this, "DelphesOutputSettings", "",
                                             "Name of config file with k4simdelphes specific output settings"};


  std::unique_ptr<Delphes> m_Delphes{nullptr};
  TObjArray* m_allParticleOutputArray;
  TObjArray* m_stableParticleOutputArray;
  TObjArray* m_partonOutputArray;


};

#endif // _K4SIMDELPHESALG_H
