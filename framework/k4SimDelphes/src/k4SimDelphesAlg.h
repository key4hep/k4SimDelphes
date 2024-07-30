#ifndef _K4SIMDELPHESALG_H
#define _K4SIMDELPHESALG_H

#include "k4SimDelphes/DelphesEDM4HepConverter.h"
#include "k4SimDelphes/DelphesEDM4HepOutputConfiguration.h"

#include "edm4hep/MCParticleCollection.h"
#include "edm4hep/ReconstructedParticleCollection.h"

#include "k4FWCore/DataHandle.h"
#include "k4FWCore/DataWrapper.h"
#include "k4FWCore/PodioDataSvc.h"

#include "Gaudi/Algorithm.h"

#include "modules/Delphes.h"

#include <memory>

namespace edm4hep {
  class MCParticleCollection;
}

/** @class k4SimDelphesAlg
 *
 *  Main Algorithm to run Delphes, getting MCParticle input, producing
 *  ReconstructedParticle output.
 */
class k4SimDelphesAlg : public Gaudi::Algorithm {
public:
  k4SimDelphesAlg(const std::string& name, ISvcLocator* svcLoc);

  virtual StatusCode initialize();
  virtual StatusCode execute(const EventContext&) const;
  virtual StatusCode finalize();

private:
  /// Input from Generator
  mutable DataHandle<edm4hep::MCParticleCollection> m_InputMCParticles{"GenParticles", Gaudi::DataHandle::Reader, this};
  /// Output from Delphes
  mutable DataHandle<edm4hep::ReconstructedParticleCollection> m_OutputRecParticles{"RecParticlesDelphes",
                                                                                    Gaudi::DataHandle::Writer, this};

  // Delphes detector card to be read in
  /// Name of Delphes tcl config file with detector and simulation parameters
  Gaudi::Property<std::string> m_DelphesCard{this, "DelphesCard", "",
                                             "Name of Delphes tcl config file with detector and simulation parameters"};
  Gaudi::Property<std::string> m_DelphesOutputSettings{
      this, "DelphesOutputSettings", "", "Name of config file with k4simdelphes specific output settings"};

  std::unique_ptr<Delphes>                               m_Delphes{nullptr};
  std::unique_ptr<ExRootConfReader>                      m_confReader{nullptr};
  std::unique_ptr<k4SimDelphes::DelphesEDM4HepConverter> m_edm4hepConverter{nullptr};
  mutable TObjArray*                                     m_allParticleOutputArray{nullptr};
  mutable TObjArray*                                     m_stableParticleOutputArray{nullptr};
  mutable TObjArray*                                     m_partonOutputArray{nullptr};

  mutable ExRootTreeWriter* m_treeWriter{nullptr};
  TTree*                    m_converterTree{nullptr};

  // since branch names are taken from delphes config
  // and not declared as data handles,
  // need podiodatasvc directly
  mutable PodioDataSvc*           m_podioDataSvc;
  ServiceHandle<IDataProviderSvc> m_eventDataSvc;
};

#endif  // _K4SIMDELPHESALG_H
