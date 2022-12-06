#ifndef DELPHESEDM4HEP_DELPHESPYTHIA8EVTGENREADER
#define DELPHESEDM4HEP_DELPHESPYTHIA8EVTGENREADER

#include <iostream>

#include "DelphesInputReader.h"
#include "DelphesPythia8Common.h"

#include "TChain.h"
#include "TClonesArray.h"
#include "TDatabasePDG.h"
#include "TObjArray.h"
#include "TParticlePDG.h"
#include "TStopwatch.h"

#include "ExRootAnalysis/ExRootTreeWriter.h"
#include "classes/DelphesClasses.h"
#include "classes/DelphesLHEFReader.h"
#include "classes/DelphesStream.h"
#include "modules/Delphes.h"

#include "Pythia.h"
#include "Pythia8Plugins/CombineMatchingInput.h"
#include "Pythia8Plugins/EvtGen.h"

#include <iostream>
#include <memory>
#include <sstream>

//---------------------------------------------------------------------------

class DelphesPythia8EvtGenReader : public DelphesInputReader {
public:
  std::string init(Delphes* modularDelphes, int argc, char* argv[]) override {
    if (argc != 8) {
      return "";
    }
    std::string outputfile = argv[4];
    // Initialize Pythia
    m_pythia = std::make_unique<Pythia8::Pythia>();

    // jet matching
#if PYTHIA_VERSION_INTEGER < 8300
    matching = combined->getHook(*m_pythia);
    if (!m_matching) {
      throw std::runtime_error("can't do matching");
    }
    m_pythia->setUserHooksPtr(m_matching);
#endif

    if (!m_pythia) {
      throw std::runtime_error("can't create Pythia instance");
    }

    // Read in commands from configuration file
    std::string pythia8configname(argv[3]);
    if (!m_pythia->readFile(pythia8configname)) {
      std::stringstream message;
      message << "can't read Pythia8 configuration file " << pythia8configname << std::endl;
      throw std::runtime_error(message.str());
    }

    // Extract settings to be used in the main program
    m_numberOfEvents  = m_pythia->mode("Main:numberOfEvents");
    m_timeAllowErrors = m_pythia->mode("Main:m_timeAllowErrors");
    m_spareFlag1      = m_pythia->flag("Main:m_spareFlag1");
    m_spareMode1      = m_pythia->mode("Main:m_spareMode1");
    m_spareParm1      = m_pythia->parm("Main:m_spareParm1");
    m_spareParm2      = m_pythia->parm("Main:m_spareParm2");

    m_treeWriter    = new ExRootTreeWriter(nullptr, "Delphes");
    m_converterTree = std::make_unique<TTree>("ConverterTree", "Analysis");
    // avoid having any connection with a TFile that might be opened later
    m_converterTree->SetDirectory(nullptr);
    m_treeWriter->SetTree(m_converterTree.get());
    modularDelphes->SetTreeWriter(m_treeWriter);

    // Check if particle gun
    if (!m_spareFlag1) {
      m_inputFile = fopen(m_pythia->word("Beams:LHEF").c_str(), "r");
      if (m_inputFile) {
        m_reader = std::make_unique<DelphesLHEFReader>();
        m_reader->SetInputFile(m_inputFile);

        m_branchEventLHEF  = m_treeWriter->NewBranch("EventLHEF", LHEFEvent::Class());
        m_branchWeightLHEF = m_treeWriter->NewBranch("WeightLHEF", LHEFWeight::Class());

        m_allParticleOutputArrayLHEF    = modularDelphes->ExportArray("allParticlesLHEF");
        m_stableParticleOutputArrayLHEF = modularDelphes->ExportArray("stableParticlesLHEF");
        m_partonOutputArrayLHEF         = modularDelphes->ExportArray("partonsLHEF");
      }
    }

    // Initialize EvtGen.
    m_evtgen = std::make_unique<Pythia8::EvtGenDecays>(
        m_pythia.get(),  // a pointer to the PYTHIA generator
        argv[5],         // the EvtGen decay file name
        argv[6],         // the EvtGen particle data file name
        nullptr,  // the optional EvtExternalGenList pointer (must be be provided if the next argument is provided to avoid double initializations)
        nullptr,  // the EvtAbsRadCorr pointer to pass to EvtGen
        1,        // the mixing type to pass to EvtGen
        false,    // a flag to use XML files to pass to EvtGen
        true,     // a flag to limit decays based on the Pythia criteria (based on the particle decay vertex)
        true,     // a flag to use external models with EvtGen
        false);   // a flag if an FSR model should be passed to EvtGen (pay attention to this, default is true)

    m_evtgen->readDecayFile(argv[7]);
    m_pythia->init();

    return outputfile;
  };

  int getNumberOfEvents() const override { return m_numberOfEvents; }

  std::string getUsage() const override {
    std::stringstream sstr;
    sstr << "Usage: " << m_appName
         << "config_file output_config_file pythia_card output_file DECAY.DEC evt.pdl user.dec\n"
         << "config_file - configuration file in Tcl format,\n"
         << "output_config_file - configuration file steering the content of the edm4hep output in Tcl format,\n"
         << "pythia_card - Pythia8 configuration file,\n"
         << "output_file - output file in ROOT format,\n"
         << "DECAY.DEC - EvtGen full decay file,\n"
         << "evt.pdl - EvtGen particle list,\n"
         << "user.dec - EvtGen user decay file.\n";
    return sstr.str();
  };

  bool readEvent(Delphes* modularDelphes, TObjArray* allParticleOutputArray, TObjArray* stableParticleOutputArray,
                 TObjArray* partonOutputArray) override {
    m_treeWriter->Clear();
    auto factory = modularDelphes->GetFactory();
    while (m_reader &&
           m_reader->ReadBlock(factory, m_allParticleOutputArrayLHEF, m_stableParticleOutputArrayLHEF,
                               m_partonOutputArrayLHEF) &&
           !m_reader->EventReady())
      ;

    if (m_spareFlag1) {
      if ((m_spareMode1 >= 1 && m_spareMode1 <= 5) || m_spareMode1 == 21) {
        fillPartons(m_spareMode1, m_spareParm1, m_spareParm2, m_pythia->event, m_pythia->particleData, m_pythia->rndm);
      } else {
        fillParticle(m_spareMode1, m_spareParm1, m_spareParm2, m_pythia->event, m_pythia->particleData, m_pythia->rndm);
      }
    }

    if (!m_pythia->next()) {
      // If failure because reached end of file then exit event loop
      if (m_pythia->info.atEndOfFile()) {
        std::cerr << "Aborted since reached end of Les Houches Event File" << std::endl;
        return false;
      }
      // First few failures write off as "acceptable" errors, then quit
      if (++m_errorCounter > m_timeAllowErrors) {
        std::cerr << "Event generation aborted prematurely, owing to error!" << std::endl;
        return false;
      }
      modularDelphes->Clear();
      m_reader->Clear();
    } else {
      m_evtgen->decay();
    }
    m_readStopWatch.Stop();
    m_procStopWatch.Start();
    ConvertInput(m_eventCounter, m_pythia.get(), m_branchEvent, factory, allParticleOutputArray,
                 stableParticleOutputArray, partonOutputArray, &m_readStopWatch, &m_procStopWatch);
    ++m_eventCounter;
    return true;
  };

  bool finished() const override { return m_eventCounter >= m_numberOfEvents; };

  TTree* converterTree() override { return m_treeWriter->GetTree(); }

private:
  static constexpr const char*           m_appName = "DelphesPythia8EvtGen";
  std::unique_ptr<Pythia8::Pythia>       m_pythia{nullptr};
  std::unique_ptr<Pythia8::EvtGenDecays> m_evtgen{nullptr};
  FILE*                                  m_inputFile = 0;
  TStopwatch                             m_readStopWatch, m_procStopWatch;
  ExRootTreeWriter*                      m_treeWriter{nullptr};
  std::unique_ptr<TTree>                 m_converterTree{nullptr};

  ExRootTreeBranch* m_branchEvent     = 0;
  ExRootTreeBranch *m_branchEventLHEF = 0, *m_branchWeightLHEF = 0;
  TObjArray *m_stableParticleOutputArrayLHEF = 0, *m_allParticleOutputArrayLHEF = 0, *m_partonOutputArrayLHEF = 0;
  std::unique_ptr<DelphesLHEFReader> m_reader = 0;
  Long64_t                           m_eventCounter{0}, m_errorCounter{0};
  Long64_t                           m_numberOfEvents{0}, m_timeAllowErrors{0};
  Bool_t                             m_spareFlag1;
  Int_t                              m_spareMode1;
  Double_t                           m_spareParm1, m_spareParm2;

  TClonesArray* m_branchParticle;
  TClonesArray* m_branchHepMCEvent;

  // for matching
  Pythia8::CombineMatchingInput* combined   = 0;
  Pythia8::UserHooks*            m_matching = 0;
};

#endif
