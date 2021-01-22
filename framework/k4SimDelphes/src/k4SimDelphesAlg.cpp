#include "k4SimDelphesAlg.h"
#include "k4SimDelphes/DelphesEDM4HepOutputConfiguration.h"
#include "k4SimDelphes/DelphesEDM4HepConverter.h"

DECLARE_COMPONENT(k4SimDelphesAlg)

std::vector<k4SimDelphes::BranchSettings> getBranchSettings(ExRootConfParam /*const&*/treeConf) {
  std::vector<k4SimDelphes::BranchSettings> branches;
  for (int b = 0; b < treeConf.GetSize(); b += 3) {
    k4SimDelphes::BranchSettings branch{treeConf[b].GetString(),
                                        treeConf[b + 1].GetString(),
                                        treeConf[b + 2].GetString()};
    branches.push_back(branch);
  }
  return branches;
}


// todo: remove
using namespace k4SimDelphes;

k4SimDelphesAlg::k4SimDelphesAlg(const std::string& name, ISvcLocator* svcLoc) : GaudiAlgorithm(name, svcLoc) {
  declareProperty("GenParticles", m_InputMCParticles);
  declareProperty("RecParticlesDelphes", m_OutputRecParticles);
}

StatusCode k4SimDelphesAlg::initialize() {
  m_Delphes = std::make_unique<Delphes>("Delphes");

  auto confReader = std::make_unique<ExRootConfReader>();
  confReader->ReadFile(m_DelphesCard.value().c_str());
  m_Delphes->SetConfReader(confReader.get());
  const auto branches = getBranchSettings(confReader->GetParam("TreeWriter::Branch"));
  const auto edm4hepOutputSettings = getEDM4hepOutputSettings(m_DelphesOutputSettings.value().c_str());
  DelphesEDM4HepConverter edm4hepConverter(branches,
                                           edm4hepOutputSettings,
                                           confReader->GetDouble("ParticlePropagator::Bz", 0));

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


  // setup output collections

  // setup input collections

  // convert to delphes
      if (!inputReader.readEvent(inputReader,
                                 allParticleOutputArray,
                                 stableParticleOutputArray,
                                 partonOutputArray)) {
        break;
      }

      m_Delphes->ProcessTask();
      edm4hepConverter.process(inputReader.converterTree());

      m_Delphes->Clear();
    }

    modularDelphes->Finish();
  return StatusCode::SUCCESS;
}

StatusCode k4SimDelphesAlg::finalize() {
  return StatusCode::SUCCESS;
}
