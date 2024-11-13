#ifndef DELPHESEDM4HEP_DELPHESPYTHIA8READER
#define DELPHESEDM4HEP_DELPHESPYTHIA8READER

#include "DelphesInputReader.h"
#include "DelphesPythia8Common.h"

#include "TChain.h"
#include "TClonesArray.h"
#include "TDatabasePDG.h"
#include "TObjArray.h"
#include "TParticlePDG.h"
#include "TStopwatch.h"

#include "ExRootAnalysis/ExRootTreeBranch.h"
#include "ExRootAnalysis/ExRootTreeWriter.h"
#include "classes/DelphesClasses.h"
#include "classes/DelphesFactory.h"
#include "classes/DelphesLHEFReader.h"
#include "classes/DelphesStream.h"
#include "modules/Delphes.h"

#include "Pythia.h"
#include "Pythia8Plugins/CombineMatchingInput.h"
#include "Pythia8Plugins/ResonanceDecayFilterHook.h"

#include <iostream>
#include <memory>
#include <sstream>

class DelphesPythia8Reader : public DelphesInputReader {
public:
  DelphesPythia8Reader(){};

  ~DelphesPythia8Reader() {
    if (m_pythia)
      PrintXS(m_pythia.get());
  }

  std::string init(Delphes* modularDelphes, int argc, char* argv[]) override {
    if (argc != 5) {
      return "";
    }
    std::string outputfile = argv[4];

    // simply pass the nullptr here for the TFile argument, we will in any case
    // define a different TTree that the TreeWriter will use internally
    m_treeWriter    = new ExRootTreeWriter(nullptr, "Delphes");
    m_converterTree = std::make_unique<TTree>("ConverterTree", "Analysis");
    // avoid having any connection with a TFile that might be opened later
    m_converterTree->SetDirectory(nullptr);
    m_treeWriter->SetTree(m_converterTree.get());
    modularDelphes->SetTreeWriter(m_treeWriter);

    m_branchEvent = std::unique_ptr<ExRootTreeBranch>(m_treeWriter->NewBranch("Event", HepMCEvent::Class()));

    // Initialize Pythia
    m_pythia = std::make_unique<Pythia8::Pythia>();

    // jet matching
#if PYTHIA_VERSION_INTEGER < 8300
    m_matching = combined->getHook(*m_pythia);
    if (!m_matching) {
      throw std::runtime_error("can't do matching");
    }
    m_pythia->setUserHooksPtr(m_matching);
#endif

    if (!m_pythia) {
      throw std::runtime_error("can't create Pythia instance");
    }

    //load and initialize the ResonanceDecayFilterUserhook
    m_resonanceDecayFilterHook = new Pythia8::ResonanceDecayFilterHook(m_pythia->settings);
    m_pythia->addUserHooksPtr((Pythia8::UserHooksPtr)m_resonanceDecayFilterHook);

    // Read in commands from configuration file
    const std::string pythia8configname(argv[3]);
    if (!m_pythia->readFile(pythia8configname)) {
      std::stringstream message;
      message << "can't read Pythia8 configuration file " << pythia8configname << std::endl;
      throw std::runtime_error(message.str());
    }

    // Extract settings to be used in the main program
    m_numberOfEvents   = m_pythia->mode("Main:numberOfEvents");
    m_timesAllowErrors = m_pythia->mode("Main:timesAllowErrors");
    m_spareFlag1       = m_pythia->flag("Main:spareFlag1");
    m_spareMode1       = m_pythia->mode("Main:spareMode1");
    m_spareParm1       = m_pythia->parm("Main:spareParm1");
    m_spareParm2       = m_pythia->parm("Main:spareParm2");

    // Check if particle gun
    if (!m_spareFlag1) {
      m_inputFile = fopen(m_pythia->word("Beams:LHEF").c_str(), "r");
      if (m_inputFile) {
        reader = new DelphesLHEFReader;
        reader->SetInputFile(m_inputFile);

        m_brancheEventLHEF = m_treeWriter->NewBranch("EventLHEF", LHEFEvent::Class());
        m_branchWeightLHEF = m_treeWriter->NewBranch("WeightLHEF", LHEFWeight::Class());

        m_allParticleOutputArrayLHEF    = modularDelphes->ExportArray("allParticlesLHEF");
        m_stableParticleOutputArrayLHEF = modularDelphes->ExportArray("stableParticlesLHEF");
        m_partonOutputArrayLHEF         = modularDelphes->ExportArray("partonsLHEF");
      }
    }

    m_pythia->init();

    return outputfile;
  }

  inline int getNumberOfEvents() const override { return m_numberOfEvents; }

  inline std::string getUsage() const override {
    std::stringstream sstr;
    sstr << "Usage: " << m_appName << "config_file output_config_file pythia_card output_file\n"
         << "config_file - configuration file in Tcl format,\n"
         << "output_config_file - configuration file steering the content of the edm4hep output in Tcl format,\n"
         << "pythia_card - Pythia8 configuration file,\n"
         << "output_file - output file in ROOT format.\n";
    return sstr.str();
  }

  inline bool readEvent(Delphes* modularDelphes, TObjArray* allParticleOutputArray,
                        TObjArray* stableParticleOutputArray, TObjArray* partonOutputArray) override {
    m_treeWriter->Clear();
    auto factory = modularDelphes->GetFactory();
    while (reader &&
           reader->ReadBlock(factory, m_allParticleOutputArrayLHEF, m_stableParticleOutputArrayLHEF,
                             m_partonOutputArrayLHEF) &&
           !reader->EventReady())
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
      if (++m_errorCounter > m_timesAllowErrors) {
        std::cerr << "Event generation aborted prematurely, owing to error!" << std::endl;
        return false;
      }
      modularDelphes->Clear();
      if (reader) {
        reader->Clear();
      }
    }
    m_readStopWatch.Stop();
    m_procStopWatch.Start();
    ConvertInput(m_eventCounter, m_pythia.get(), m_branchEvent.get(), factory, allParticleOutputArray,
                 stableParticleOutputArray, partonOutputArray, &m_readStopWatch, &m_procStopWatch);
    ++m_eventCounter;
    return true;
  };

  inline bool finished() const override { return m_eventCounter >= m_numberOfEvents; };

  TTree* converterTree() override { return m_treeWriter->GetTree(); }

private:
  static constexpr const char*     m_appName = "DelphesPythia8";
  std::unique_ptr<Pythia8::Pythia> m_pythia{nullptr};

  FILE*                             m_inputFile = 0;
  TStopwatch                        m_readStopWatch, m_procStopWatch;
  ExRootTreeWriter*                 m_treeWriter{nullptr};
  std::unique_ptr<ExRootTreeBranch> m_branchEvent{nullptr};
  std::unique_ptr<TTree>            m_converterTree{nullptr};

  ExRootTreeBranch *m_brancheEventLHEF = 0, *m_branchWeightLHEF = 0;
  TObjArray *m_stableParticleOutputArrayLHEF = 0, *m_allParticleOutputArrayLHEF = 0, *m_partonOutputArrayLHEF = 0;
  DelphesLHEFReader* reader = 0;
  Long64_t           m_eventCounter{0}, m_errorCounter{0};
  Long64_t           m_numberOfEvents, m_timesAllowErrors;
  Bool_t             m_spareFlag1;
  Int_t              m_spareMode1;
  Double_t           m_spareParm1, m_spareParm2;

  TClonesArray* m_branchParticle;
  TClonesArray* m_branchHepMCEvent;

  // for matching
  Pythia8::CombineMatchingInput* combined   = 0;
  Pythia8::UserHooks*            m_matching = 0;

  //resonance decayfilter
  Pythia8::ResonanceDecayFilterHook* m_resonanceDecayFilterHook{nullptr};
};

#endif
