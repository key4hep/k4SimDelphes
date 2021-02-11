#include "k4SimDelphesAlg.h"
#include "k4SimDelphes/DelphesEDM4HepOutputConfiguration.h"
#include "k4SimDelphes/k4GenParticlesDelphesConverter.h"
#include "ExRootTreeWriter.h" // use local copy, todo: fix / push upstream
#include "edm4hep/ReconstructedParticleCollection.h"

DECLARE_COMPONENT(k4SimDelphesAlg)

// todo: remove
using namespace k4SimDelphes;

k4SimDelphesAlg::k4SimDelphesAlg(const std::string& name, ISvcLocator* svcLoc) : GaudiAlgorithm(name, svcLoc) {
  declareProperty("GenParticles", 
                  m_InputMCParticles,
                  "(Input) Collection of generated particles");
  declareProperty("RecParticlesDelphes", 
                  m_OutputRecParticles,
                  "(Output) Collection of reconstructed particles as outputed by Delphes");
}

StatusCode k4SimDelphesAlg::initialize() {
  ///-- setup input collections --/////////////////////////////////////////////
  m_Delphes = std::make_unique<Delphes>("Delphes");
  //auto confReader = std::make_unique<ExRootConfReader>();
  //confReader->ReadFile(m_DelphesCard.value().c_str());
  //m_Delphes->SetConfReader(confReader.get());

  //m_treeWriter = new ExRootTreeWriter(nullptr, "Delphes");
  //m_converterTree = std::make_unique<TTree>("ConverterTree", "Analysis");
  //// avoid having any connection with a TFile that might be opened later
  //m_converterTree->SetDirectory(nullptr);
  //m_treeWriter->SetTree(m_converterTree.get());
  //m_Delphes->SetTreeWriter(m_treeWriter);

  //const auto branches = getBranchSettings(confReader->GetParam("TreeWriter::Branch"));
  //const auto edm4hepOutputSettings = getEDM4hepOutputSettings(m_DelphesOutputSettings.value().c_str());
  //m_edm4hepConverter = std::make_unique<DelphesEDM4HepConverter>(branches,
  //                                         edm4hepOutputSettings,
  //                                         confReader->GetDouble("ParticlePropagator::Bz", 0));
  //// has to happen before InitTask
  //m_allParticleOutputArray = m_Delphes->ExportArray("allParticles");
  //m_stableParticleOutputArray = m_Delphes->ExportArray("stableParticles");
  //m_partonOutputArray = m_Delphes->ExportArray("partons");

 //std::cout << "InitTask" << std::endl;
  //m_Delphes->InitTask();
  //m_Delphes->Clear();


  m_confReader = new ExRootConfReader();
  m_confReader->ReadFile(m_DelphesCard.value().c_str());
  //m_Delphes->SetConfReader(confReader.get());
  m_Delphes->SetConfReader(m_confReader);
  m_treeWriter = new ExRootTreeWriter(nullptr, "Delphes");
  m_converterTree = new TTree ("ConverterTree", "Analysis");
  // avoid having any connection with a TFile that might be opened later
  m_converterTree->SetDirectory(nullptr);
  m_treeWriter->SetTree(m_converterTree);
  m_Delphes->SetTreeWriter(m_treeWriter);
  //auto confReader = std::make_unique<ExRootConfReader>();
  // debug init things
  // has to happen before InitTask
  m_allParticleOutputArray = m_Delphes->ExportArray("allParticles");
  m_stableParticleOutputArray = m_Delphes->ExportArray("stableParticles");
  m_partonOutputArray = m_Delphes->ExportArray("partons");

  m_Delphes->InitTask();
  m_Delphes->Clear();
  return StatusCode::SUCCESS;
}

StatusCode k4SimDelphesAlg::execute() {
  info() << "debug k4simdelphesalg... " << endmsg;
  const auto branches = getBranchSettings(m_confReader->GetParam("TreeWriter::Branch"));
  const auto edm4hepOutputSettings = getEDM4hepOutputSettings(m_DelphesOutputSettings.value().c_str());
  m_edm4hepConverter = new DelphesEDM4HepConverter(branches,
                                           edm4hepOutputSettings,
                                           m_confReader->GetDouble("ParticlePropagator::Bz", 0));
  ///-- setup input collections --/////////////////////////////////////////////
  auto genparticles = m_InputMCParticles.get();
  // input
  auto conv = k4GenParticlesDelphesConverter(); 
  conv.convertToDelphesArrays(
      genparticles,
      *m_Delphes->GetFactory(),
      *m_allParticleOutputArray,
      *m_stableParticleOutputArray,
      *m_partonOutputArray);
  ///-- actual simulation --////////////////////////////////////////////////////
  m_Delphes->ProcessTask();
  ///-- conversion of the output --/////////////////////////////////////////////
  m_edm4hepConverter->process(m_converterTree);
  // setup output collections
  edm4hep::ReconstructedParticleCollection* recparticles = new edm4hep::ReconstructedParticleCollection();

  auto collections = m_edm4hepConverter->getCollections();
  for (auto& c: collections) {
    if (c.first == "ReconstructedParticles") {
      m_OutputRecParticles.put(static_cast<edm4hep::ReconstructedParticleCollection*>(c.second));
    }
  }
  m_Delphes->Clear();
  delete m_edm4hepConverter;
  return StatusCode::SUCCESS;
}

StatusCode k4SimDelphesAlg::finalize() {
  m_Delphes->Finish();
  delete m_confReader;
  return StatusCode::SUCCESS;
}
