#include "k4SimDelphesAlg.h"
#include "ExRootAnalysis/ExRootTreeWriter.h"
#include "edm4hep/RecoMCParticleLinkCollection.h"
#include "edm4hep/ReconstructedParticleCollection.h"
#include "k4SimDelphes/DelphesEDM4HepOutputConfiguration.h"
#include "k4SimDelphes/k4GenParticlesDelphesConverter.h"

DECLARE_COMPONENT(k4SimDelphesAlg)

// todo: remove
using namespace k4SimDelphes;

k4SimDelphesAlg::k4SimDelphesAlg(const std::string& name, ISvcLocator* svcLoc)
    : Gaudi::Algorithm(name, svcLoc), m_eventDataSvc("EventDataSvc", "k4SimDelphesAlg") {
  declareProperty("GenParticles", m_InputMCParticles, "(Input) Collection of generated particles");
}

StatusCode k4SimDelphesAlg::initialize() {
  ///-- setup Configuration and input arrays //////////////////////////////////
  m_Delphes    = std::make_unique<Delphes>("Delphes");
  m_confReader = std::make_unique<ExRootConfReader>();
  m_confReader->ReadFile(m_DelphesCard.value().c_str());
  m_Delphes->SetConfReader(m_confReader.get());
  m_treeWriter    = new ExRootTreeWriter(nullptr, "Delphes");
  m_converterTree = new TTree("ConverterTree", "Analysis");
  // avoid having any connection with a TFile that might be opened later
  m_converterTree->SetDirectory(nullptr);
  m_treeWriter->SetTree(m_converterTree);
  m_Delphes->SetTreeWriter(m_treeWriter);
  // ExportArray: has to happen before InitTask
  m_allParticleOutputArray    = m_Delphes->ExportArray("allParticles");
  m_stableParticleOutputArray = m_Delphes->ExportArray("stableParticles");
  m_partonOutputArray         = m_Delphes->ExportArray("partons");
  m_Delphes->InitTask();
  m_Delphes->Clear();

  // data service
  m_eventDataSvc.retrieve().ignore();
  m_podioDataSvc = dynamic_cast<PodioDataSvc*>(m_eventDataSvc.get());

  const auto branches = getBranchSettings(m_confReader->GetParam("TreeWriter::Branch"));
  m_outputConfig      = getEDM4hepOutputSettings(m_DelphesOutputSettings.value().c_str());
  m_edm4hepConverter  = std::make_unique<k4SimDelphes::DelphesEDM4HepConverter>(
      branches, m_outputConfig, m_confReader->GetDouble("ParticlePropagator::Bz", 0));

  return StatusCode::SUCCESS;
}

StatusCode k4SimDelphesAlg::execute(const EventContext&) const {
  verbose() << "Execute k4SimDelphesAlg... " << endmsg;
  m_allParticleOutputArray->Clear();
  m_stableParticleOutputArray->Clear();
  m_partonOutputArray->Clear();
  m_treeWriter->Clear();

  verbose() << " ... Setup Input Collections " << endmsg;
  auto genparticles = m_InputMCParticles.get();
  // input
  auto conv = k4GenParticlesDelphesConverter();
  conv.convertToDelphesArrays(genparticles, *m_Delphes->GetFactory(), *m_allParticleOutputArray,
                              *m_stableParticleOutputArray, *m_partonOutputArray);
  auto mapSimDelphes = conv.getGenParticleIdMap();
  ///-- actual simulation --////////////////////////////////////////////////////
  verbose() << "Running delphes simulation" << endmsg;
  m_Delphes->ProcessTask();
  ///-- conversion of the output --/////////////////////////////////////////////
  verbose() << "Running converter" << endmsg;
  m_edm4hepConverter->process(m_converterTree);
  // setup output collections

  auto collections = m_edm4hepConverter->getCollections();
  for (auto& c : collections) {
    if (c.first == m_outputConfig.RecoMCParticleLinkCollectionName) {
      auto                                new_c   = m_edm4hepConverter->createExternalRecoMCLinks(mapSimDelphes);
      DataWrapper<podio::CollectionBase>* wrapper = new DataWrapper<podio::CollectionBase>();
      wrapper->setData(new_c);
      m_podioDataSvc->registerObject("/Event", "/" + std::string(c.first), wrapper).ignore();
      continue;
    }

    DataWrapper<podio::CollectionBase>* wrapper = new DataWrapper<podio::CollectionBase>();
    wrapper->setData(c.second.release());  // DataWrapper takes ownership
    m_podioDataSvc->registerObject("/Event", "/" + std::string(c.first), wrapper).ignore();
  }
  m_Delphes->Clear();

  return StatusCode::SUCCESS;
}

StatusCode k4SimDelphesAlg::finalize() {
  m_Delphes->Finish();

  return StatusCode::SUCCESS;
}
