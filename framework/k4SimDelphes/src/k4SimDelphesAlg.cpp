#include "k4SimDelphesAlg.h"

DECLARE_COMPONENT(k4SimDelphesAlg)

k4SimDelphesAlg::k4SimDelphesAlg(const std::string& name, ISvcLocator* svcLoc) : GaudiAlgorithm(name, svcLoc) {
  declareProperty("GenParticles", m_InputMCParticles);
  declareProperty("RecParticlesDelphes", m_OutputRecParticles);
}

StatusCode k4SimDelphesAlg::initialize() {
  return StatusCode::SUCCESS;
}

StatusCode k4SimDelphesAlg::execute() {
  info() << "debug k4simdelphesalg... " << endmsg;
  return StatusCode::SUCCESS;
}

StatusCode k4SimDelphesAlg::finalize() {
  return StatusCode::SUCCESS;
}
