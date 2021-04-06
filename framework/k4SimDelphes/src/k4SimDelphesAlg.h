#ifndef _K4SIMDELPHESALG_H
#define _K4SIMDELPHESALG_H

#include "k4FWCore/DataHandle.h"
#include "GaudiAlg/GaudiAlgorithm.h"
#include "k4FWCore/DataWrapper.h"
#include "k4FWCore/PodioDataSvc.h"

#include "edm4hep/MCParticleCollection.h"
#include "edm4hep/ReconstructedParticleCollection.h"

 #include "k4SimDelphes/DelphesEDM4HepConverter.h"


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
  //std::unique_ptr<k4SimDelphes::DelphesEDM4HepConverter> m_edm4hepConverter{nullptr};
  k4SimDelphes::DelphesEDM4HepConverter* m_edm4hepConverter{nullptr};
  TObjArray* m_allParticleOutputArray{nullptr};
  TObjArray* m_stableParticleOutputArray{nullptr};
  TObjArray* m_partonOutputArray{nullptr};

  ExRootTreeWriter* m_treeWriter{nullptr};
  TTree* m_converterTree{nullptr};
  ExRootConfReader* m_confReader{nullptr};

  // since branch names are taken from delphes config
  // and not declared as data handles,
  // need podiodatasvc directly
  PodioDataSvc* m_podioDataSvc;
  ServiceHandle<IDataProviderSvc> m_eventDataSvc;


};

#endif // _K4SIMDELPHESALG_H
