#include "k4SimDelphesAlg.h"
#include "k4SimDelphes/DelphesEDM4HepOutputConfiguration.h"
#include "k4SimDelphes/k4GenParticlesDelphesConverter.h"
#include "ExRootTreeWriter.h" // use local copy, todo: fix / push upstream
#include "edm4hep/ReconstructedParticleCollection.h"
#include "edm4hep/MCRecoParticleAssociationCollection.h"

DECLARE_COMPONENT(k4SimDelphesAlg)

// todo: remove
using namespace k4SimDelphes;

k4SimDelphesAlg::k4SimDelphesAlg(const std::string& name, ISvcLocator* svcLoc) : GaudiAlgorithm(name, svcLoc), m_eventDataSvc("EventDataSvc", "k4SimDelphesAlg") {
  declareProperty("GenParticles", 
                  m_InputMCParticles,
                  "(Input) Collection of generated particles");
}

StatusCode k4SimDelphesAlg::initialize() {
  ///-- setup Configuration and input arrays //////////////////////////////////
  m_Delphes = std::make_unique<Delphes>("Delphes");
  m_confReader = new ExRootConfReader();
  m_confReader->ReadFile(m_DelphesCard.value().c_str());
  m_Delphes->SetConfReader(m_confReader);
  m_treeWriter = new ExRootTreeWriter(nullptr, "Delphes");
  m_converterTree = new TTree ("ConverterTree", "Analysis");
  // avoid having any connection with a TFile that might be opened later
  m_converterTree->SetDirectory(nullptr);
  m_treeWriter->SetTree(m_converterTree);
  m_Delphes->SetTreeWriter(m_treeWriter);
  // ExportArray: has to happen before InitTask
  m_allParticleOutputArray = m_Delphes->ExportArray("allParticles");
  m_stableParticleOutputArray = m_Delphes->ExportArray("stableParticles");
  m_partonOutputArray = m_Delphes->ExportArray("partons");
  m_Delphes->InitTask();
  m_Delphes->Clear();

  // data service
  m_eventDataSvc.retrieve();
  m_podioDataSvc = dynamic_cast<PodioDataSvc*>( m_eventDataSvc.get());

  return StatusCode::SUCCESS;
}

StatusCode k4SimDelphesAlg::execute() {
  verbose() << "Execute k4SimDelphesAlg... " << endmsg;
  const auto branches = getBranchSettings(m_confReader->GetParam("TreeWriter::Branch"));
  const auto edm4hepOutputSettings = getEDM4hepOutputSettings(m_DelphesOutputSettings.value().c_str());
  m_edm4hepConverter = new DelphesEDM4HepConverter(branches,
                                           edm4hepOutputSettings,
                                           m_confReader->GetDouble("ParticlePropagator::Bz", 0));
  verbose() << " ... Setup Input Collections " << endmsg;
  auto genparticles = m_InputMCParticles.get();
  // input
  auto conv = k4GenParticlesDelphesConverter(); 
  conv.convertToDelphesArrays(
      genparticles,
      *m_Delphes->GetFactory(),
      *m_allParticleOutputArray,
      *m_stableParticleOutputArray,
      *m_partonOutputArray);
  auto mapSimDelphes = conv.getGenParticleIdMap();
  ///-- actual simulation --////////////////////////////////////////////////////
  m_Delphes->ProcessTask();
  ///-- conversion of the output --/////////////////////////////////////////////
  m_edm4hepConverter->process(m_converterTree);
  // setup output collections

  auto collections = m_edm4hepConverter->getCollections();
  for (auto& c: collections) {
    if (c.first == "MCRecoAssociations") {
    auto new_c = m_edm4hepConverter->createExternalRecoAssociations(mapSimDelphes);
       DataWrapper<podio::CollectionBase>* wrapper = new DataWrapper<podio::CollectionBase>();
       wrapper->setData(new_c);
       m_podioDataSvc->registerObject("/Event", "/" + std::string(c.first), wrapper);
       continue;
     }

    DataWrapper<podio::CollectionBase>* wrapper = new DataWrapper<podio::CollectionBase>();
    wrapper->setData(c.second);
    m_podioDataSvc->registerObject("/Event", "/" + std::string(c.first), wrapper);
  }
  m_Delphes->Clear();
  //delete m_edm4hepConverter;
  return StatusCode::SUCCESS;
}

StatusCode k4SimDelphesAlg::finalize() {
  m_Delphes->Finish();
  delete m_confReader;
  return StatusCode::SUCCESS;
}
