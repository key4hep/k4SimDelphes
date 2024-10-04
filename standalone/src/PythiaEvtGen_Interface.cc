// Include files

#include "EvtGenBase/EvtAbsRadCorr.hh"
#include "EvtGenBase/EvtDecayBase.hh"
#include "EvtGenBase/EvtMTRandomEngine.hh"
#include "EvtGenBase/EvtPDL.hh"
#include "EvtGenBase/EvtParticle.hh"
#include "EvtGenBase/EvtParticleFactory.hh"
#include "EvtGenBase/EvtRandom.hh"
#include "EvtGenBase/EvtSimpleRandomEngine.hh"
#include "EvtGenBase/EvtVector4R.hh"

#include "EvtGen/EvtGen.hh"
#include "EvtGenBase/EvtDecayBase.hh"
#include "EvtGenBase/EvtDecayTable.hh"
#include "EvtGenBase/EvtPDL.hh"
#include "EvtGenBase/EvtParticle.hh"
#include "EvtGenBase/EvtParticleDecayList.hh"
#include "EvtGenBase/EvtParticleFactory.hh"
#include "EvtGenBase/EvtRandomEngine.hh"
#include "EvtGenExternal/EvtExternalGenList.hh"

// local
#include "PythiaEvtGen_Interface.h"

//-----------------------------------------------------------------------------
// Implementation file for class : PythiaEvtGen_Interface
//
// 2020-12-07 : Jihyun Bhom, Marcin Chrzaszcz
//-----------------------------------------------------------------------------

//=============================================================================
// Standard constructor, initializes variables
//=============================================================================
PythiaEvtGen_Interface::PythiaEvtGen_Interface(Pythia8::Pythia* p, std::string pdf, std::string evt_pdl, int seed) {
  // this is a standard constructor used for default confuguration
  pythia           = p;  // adding pythia
  particleDataFile = pdf;
  init_seed        = seed;
  pdl              = evt_pdl;
  debug            = false;

  regenerate = false;
  verbose    = false;

  std::cout << "Adding Inclusive File:" << std::endl;
  add_inclusive(pdf);  // adding inclusive decays
  //  int arr[] = {511,521,531,541,5122,5132,5142,5232,5242,5332,5342,5412,5414,5422,5424,5432,5434,5442,5444,5512,5514,5522,5524,5532,5534,5542,5544,5544};
  B_ids = std::vector<int>{511,  521,  531,  541,  5122, 5132, 5142, 5232, 5242, 5332, 5342, 5412, 5414, 5422,
                           5424, 5432, 5434, 5442, 5444, 5512, 5514, 5522, 5524, 5532, 5534, 5542, 5544, 5544};
  std::cout << "Finished Initialization" << std::endl;
  n_rehadronize = 1e4;
}
//=============================================================================
// Destructor
//=============================================================================
PythiaEvtGen_Interface::~PythiaEvtGen_Interface() { delete evtgen; }
void PythiaEvtGen_Interface::add_decays(std::string decayfile, int motherID, std::string name) {
  evtgen->readUDecay(decayfile.c_str());
  motherIDs.push_back(motherID);
  sig_names.push_back(name);
}
void PythiaEvtGen_Interface::add_inclusive(std::string decayfile) {
  add_inclusive(decayfile, init_seed);
  init_seed++;
}

void PythiaEvtGen_Interface::add_inclusive(std::string decayfile, int seed) {
  // Specify if we want to use Pythia 6 physics codes for decays
  bool convertPythiaCodes = false;
  // Specify the pythia xml data directory to use the default PYTHIA8DATA location
  std::string pythiaDir = "";
  // Specify the photon type for Photos
  std::string photonType = "gamma";
  // Specify if we want to use the EvtGen random number engine for these generators
  bool useEvtGenRandom = true;
  // Set up the default external generator list: Photos, Pythia and/or Tauola
  EvtExternalGenList genList(convertPythiaCodes, pythiaDir, photonType, useEvtGenRandom);
  // Get the interface to the to the radiative correction engine
  EvtAbsRadCorr* radCorrEngine = genList.getPhotosModel();
  // Get the interface to the other external generators (Pythia and Tauola)
  std::list<EvtDecayBase*> extraModels = genList.getListOfModels();
  // Random engine:

  EvtRandomEngine* eng = new EvtMTRandomEngine(seed);
  EvtRandom::setRandomEngine(eng);

  if (debug)
    std::cout << "Creating EvtGen instance" << std::endl;

  // Create the EvtGen generator object, passing the external generators

  evtgen = new EvtGen(decayfile.c_str(), pdl.c_str(), eng, radCorrEngine, &extraModels);
}
///////////////////////////////////////////////////////////////////////////////
// Function that checks if we have all the b hadrons need for signal to decay
// returns true if yes or false if not
///////////////////////////////////////////////////////////////////////////////

bool PythiaEvtGen_Interface::check_Signal_Appereance() {
  unsigned NOfSignal = motherIDs.size();
  NOfSignal_list     = std::vector(NOfSignal, 0);
  //signal_map= new int[NOfSignal][10];// here we assume we won't have more then 10 signal candidates
  signal_map = std::vector<std::vector<int>>(NOfSignal);  // resetting this vector//;(NOfSignal);
  //signal_map= new int*[NOfSignal];
  for (auto i = 0u; i < NOfSignal; ++i) {
    //    signal_map[i] = new int[10]; // here we assume we won't have more then 10 signal candidates
    signal_map[i] = std::vector<int>(10);
  }

  if (debug)
    std::cout << "Looking for " << NOfSignal << " signal candidates" << std::endl;

  if (debug)
    pythia->event.list();
  Pythia8::Event& event = pythia->event;

  //loop over particles to find candidates that mach what we need from the decay
  for (int iPro = 0; iPro < event.size(); ++iPro) {
    Pythia8::Particle* part = &event[iPro];
    if (part->isFinal() == false)
      continue;  // skipping the particles that already decayed
    //if (incIDs.find(part->id()) == incIDs.end()) continue; // particle not in the inclusive ID, continue
    if (part->status() == 93 || part->status() == 94)
      continue;  // particles handed by external program

    if (std::find(motherIDs.begin(), motherIDs.end(), abs(part->id())) !=
        motherIDs.end())  // the ID is part of the signal to be decayed
    {
      std::vector<int>::iterator it;
      it                                   = (std::find(motherIDs.begin(), motherIDs.end(), abs(part->id())));
      int tmp                              = std::distance(motherIDs.begin(), it);
      signal_map[tmp][NOfSignal_list[tmp]] = iPro;  // here we are storing where the signal particle is on the list
      NOfSignal_list[tmp] += 1;                     // ticking off that we have the particle requested
    }
  }
  // now checking if we have all B candidates
  size_t sum = 0;
  for (unsigned i = 0; i < NOfSignal_list.size(); i++) {
    if (NOfSignal_list[i] > 0)
      sum += 1;
  }
  if (debug)
    std::cout << "Found " << sum << " signal candidates" << std::endl;

  if (sum == NOfSignal) {
    return true;
  }

  return false;
};

void PythiaEvtGen_Interface::decay_signals() {
  Pythia8::Event& event = pythia->event;

  if (debug)
    std::cout << "In decay_signals" << std::endl;
  unsigned NOfSignal = motherIDs.size();
  for (auto i_sig = 0u; i_sig < NOfSignal; i_sig++)  // loop over signal B's
  {
    int randomsignal = (rand() % NOfSignal_list[i_sig]);
    if (debug)
      std::cout << "Rand: " << randomsignal << std::endl;
    int                part_index = signal_map[i_sig][randomsignal];
    Pythia8::Particle* part       = &event[part_index];
    //Particle preparation for EvtGen.
    EvtId B_i = EvtPDL::getId(sig_names[i_sig]);
    if (debug)
      std::cout << "EvtID " << sig_names[i_sig] << "  " << part_index << std::endl;
    EvtVector4R pInit(part->e(), part->px(), part->py(), part->pz());
    if (part->id() != EvtPDL::getStdHep(B_i)) {
      if (debug)
        std::cout << "We have wrong charge conjugate, changing" << std::endl;
      if (debug)
        std::cout << "Changing the name: " << EvtPDL::name(B_i) << std::endl;
      B_i = EvtPDL::chargeConj(B_i);
    }
    EvtParticle* Evtpart = EvtParticleFactory::particleFactory(B_i, pInit);
    if (debug)
      std::cout << "Evtparticle " << std::endl;
    Evtpart->setDiagonalSpinDensity();
    //EvtDecayTable::getInstance()->printSummary();
    evtgen->generateDecay(Evtpart);
    if (debug)
      std::cout << "Decayed" << std::endl;
    part->tau(Evtpart->getLifetime());
    UpdatePythiaEvent(part, Evtpart);
  }
}

void PythiaEvtGen_Interface::decay() {
  Pythia8::Event savedEvent = pythia->event;

  if (debug)
    std::cout << "Before hadronization:" << std::endl;
  if (debug)
    pythia->event.list();
  pythia->forceHadronLevel(false);  // we are hadronizing
  if (debug)
    std::cout << "After hadronization:" << std::endl;

  if (debug)
    pythia->event.list();

  // first check if events have all the B's we need:
  bool IsSignal      = check_Signal_Appereance();
  int  i_rehadronize = 0;

  if (IsSignal == false)  // repeat hadronization
  {
    if (debug)
      std::cout << "We don't have an event:" << std::endl;

    do {
      pythia->event = savedEvent;
      if (regenerate) {
        if (debug) {
          std::cout << "We are regenerating whole decay" << std::endl;
        }
        pythia->next();
        if (debug)
          pythia->event.list();
      }
      if (i_rehadronize >= n_rehadronize)
        pythia->next();

      pythia->forceHadronLevel(false);
      i_rehadronize++;
    } while (check_Signal_Appereance() == false);

    if (debug)
      std::cout << "We have an event after regeneration:" << std::endl;

  } else {
    if (debug)
      std::cout << "We have an event:" << std::endl;
  }
  // now we have in the event all the B's that are needed to be decayed
  // first decay signal:
  decay_signals();
  if (debug)
    std::cout << "Decayed only signal:" << std::endl;
  if (debug)
    pythia->event.list();

  if (debug)
    std::cout << "Decaying rest:" << std::endl;
  // decay rest (aka rest of B's)
  Pythia8::Event& event = pythia->event;

  for (int iPro = 0; iPro < event.size(); ++iPro) {
    // Check particle is final and can be decayed by EvtGen.
    Pythia8::Particle* part = &event[iPro];
    if (!part->isFinal())
      continue;
    if (part->status() == 93 || part->status() == 94)
      continue;

    if (std::find(B_ids.begin(), B_ids.end(), abs(part->id())) != B_ids.end()) {
      /* v contains x */

      if (debug)
        std::cout << "Trying to decay ''background'' : " << part->id() << " index: " << iPro << std::endl;
      // here we have inclusive
      EvtParticle* Evtpart = EvtParticleFactory::particleFactory(
          EvtPDL::evtIdFromStdHep(part->id()), EvtVector4R(part->e(), part->px(), part->py(), part->pz()));
      Evtpart->setDiagonalSpinDensity();
      evtgen->generateDecay(Evtpart);
      part->tau(Evtpart->getLifetime());
      UpdatePythiaEvent(part, Evtpart);
    }
  }
  if (verbose)
    std::cout << "EvtGen has finished with the following record:" << std::endl;
  if (verbose)
    pythia->event.list();
};
void PythiaEvtGen_Interface::UpdatePythiaEvent(Pythia8::Particle* part, EvtParticle* Evtpart) {
  // inspired by Phil Ilten
  EvtParticle*                              egMom = Evtpart;
  Pythia8::Event&                           event = pythia->event;
  std::vector<std::pair<EvtParticle*, int>> moms(1, std::pair<EvtParticle*, int>(egMom, part->index()));
  while (moms.size() != 0) {
    // Check if particle can decay.
    egMom                    = moms.back().first;
    int                iMom  = moms.back().second;
    Pythia8::Particle* pyMom = &event[iMom];
    moms.pop_back();
    //if (!checkVertex(pyMom)) continue;
    //bool osc(checkOsc(egMom));
    // Set the children of the mother.
    pyMom->daughters(event.size(), event.size() + egMom->getNDaug() - 1);
    pyMom->statusNeg();
    Pythia8::Vec4 vProd = pyMom->vDec();
    for (int iDtr = 0; iDtr < (int)egMom->getNDaug(); ++iDtr) {
      EvtParticle* egDtr = egMom->getDaug(iDtr);
      int          id    = egDtr->getPDGId();
      EvtVector4R  p     = egDtr->getP4Lab();
      int idx = event.append(id, 93, iMom, 0, 0, 0, 0, 0, p.get(1), p.get(2), p.get(3), p.get(0), egDtr->mass());
      Pythia8::Particle* pyDtr = &event.back();
      pyDtr->vProd(vProd);
      pyDtr->tau(egDtr->getLifetime());
      if (egDtr->getNDaug() > 0)
        moms.push_back(std::pair<EvtParticle*, int>(egDtr, idx));
    }
  }
};

//=============================================================================
