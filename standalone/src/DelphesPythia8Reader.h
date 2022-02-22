#ifndef DELPHESEDM4HEP_DELPHESPYTHIA8READER
#define DELPHESEDM4HEP_DELPHESPYTHIA8READER

#include <iostream>

#include "DelphesInputReader.h"
#include "DelphesPythia8Common.h"

#include "TObjArray.h"
#include "TChain.h"
#include "TClonesArray.h"
#include "TParticlePDG.h"
#include "TStopwatch.h"
#include "TDatabasePDG.h"

#include "classes/DelphesClasses.h"
#include "classes/DelphesFactory.h"
#include "classes/DelphesStream.h"
#include "classes/DelphesLHEFReader.h"
#include "modules/Delphes.h"
#include "ExRootAnalysis/ExRootTreeWriter.h"
#include "ExRootAnalysis/ExRootTreeBranch.h"

#include "Pythia.h"
#include "Pythia8Plugins/CombineMatchingInput.h"
#include "Pythia8Plugins/PowhegHooks.h"
#include "Pythia8Plugins/aMCatNLOHooks.h"
#include "Pythia8Plugins/JetMatching.h"
#include "ResonanceDecayFilterHook.h"

#include <iostream>
#include <memory>

class DelphesPythia8Reader: public DelphesInputReader {
public:
  DelphesPythia8Reader() {};

  ~DelphesPythia8Reader() {}

  std::string init(Delphes* modularDelphes, int argc, char *argv[]) override {
    if (argc != 5) {
      return "";
    }
    std::string outputfile = argv[4];

    // simply pass the nullptr here for the TFile argument, we will in any case
    // define a different TTree that the TreeWriter will use internally
    m_treeWriter = new ExRootTreeWriter(nullptr, "Delphes");
    m_converterTree = std::make_unique<TTree>("ConverterTree", "Analysis");
    // avoid having any connection with a TFile that might be opened later
    m_converterTree->SetDirectory(nullptr);
    m_treeWriter->SetTree(m_converterTree.get());
    modularDelphes->SetTreeWriter(m_treeWriter);

    // Initialize Pythia
    m_pythia = std::make_unique<Pythia8::Pythia>();


  //add settings for resonance decay filter
    m_pythia->settings.addFlag("ResonanceDecayFilter:filter", false);
    m_pythia->settings.addFlag("ResonanceDecayFilter:exclusive", false);
    m_pythia->settings.addFlag("ResonanceDecayFilter:eMuAsEquivalent", false);
    m_pythia->settings.addFlag("ResonanceDecayFilter:eMuTauAsEquivalent", false);
    m_pythia->settings.addFlag("ResonanceDecayFilter:allNuAsEquivalent", false);
    m_pythia->settings.addFlag("ResonanceDecayFilter:udscAsEquivalent", false);
    m_pythia->settings.addFlag("ResonanceDecayFilter:udscbAsEquivalent", false);
    m_pythia->settings.addFlag("ResonanceDecayFilter:wzAsEquivalent", false);
    m_pythia->settings.addMVec("ResonanceDecayFilter:mothers", std::vector<int>(), false, false, 0, 0);
    m_pythia->settings.addMVec("ResonanceDecayFilter:daughters", std::vector<int>(), false, false, 0, 0);

    /*
    // jet matching
#if PYTHIA_VERSION_INTEGER < 8300
    m_matching = combined->getHook(*m_pythia);
    if(!m_matching) {
      throw std::runtime_error("can't do matching");
    }
    m_pythia->setUserHooksPtr(m_matching);
#endif
    */

    if (!m_pythia) {
      throw std::runtime_error("can't create Pythia instance");
    }

    // Read in commands from configuration file
    const std::string pythia8configname(argv[3]);
    if(!m_pythia->readFile(pythia8configname))
    {
      std::stringstream message;
      message << "can't read Pythia8 configuration file " << pythia8configname << std::endl;
      throw std::runtime_error(message.str());
    }

    // Extract settings to be used in the main program
    m_numberOfEvents = m_pythia->mode("Main:numberOfEvents");
    m_timesAllowErrors = m_pythia->mode("Main:timesAllowErrors");
    m_spareFlag1 = m_pythia->flag("Main:spareFlag1");
    m_spareMode1 = m_pythia->mode("Main:spareMode1");
    m_spareParm1 = m_pythia->parm("Main:spareParm1");
    m_spareParm2 = m_pythia->parm("Main:spareParm2");

    // Begin ME/PS Matching specific code
    // Check if jet matching should be applied.
    
    m_doMePsMatching = m_pythia->settings.flag("JetMatching:merge");
    // Check if internal merging should be applied.
    m_doMePsMerging = !(m_pythia->settings.word("Merging:Process").compare("void") == 0);

    // Currently, only one scheme at a time is allowed.
    if (m_doMePsMerging && m_doMePsMatching) {
      std::stringstream message;
      message << "Jet matching and merging cannot be used simultaneously! " << std::endl;
      throw std::runtime_error(message.str());
    }


    // Allow to set the number of additional partons dynamically.
    if (m_doMePsMerging) {
      // Store merging scheme.
      int scheme;
      if (m_pythia->settings.flag("Merging:doUMEPSTree") || m_pythia->settings.flag("Merging:doUMEPSSubt")) {
	scheme = 1;
      } else if (m_pythia->settings.flag("Merging:doUNLOPSTree") ||
		 m_pythia->settings.flag("Merging:doUNLOPSSubt") ||
		 m_pythia->settings.flag("Merging:doUNLOPSLoop") ||
		 m_pythia->settings.flag("Merging:doUNLOPSSubtNLO")) {
	scheme = 2;
      } else {
	scheme = 0;
      }
      
      m_setting = std::unique_ptr<Pythia8::amcnlo_unitarised_interface>(new Pythia8::amcnlo_unitarised_interface(scheme));
#if PYTHIA_VERSION_INTEGER < 8300
      m_pythia->setUserHooksPtr(m_setting.get());
#else
      m_pythia->setUserHooksPtr((Pythia8::UserHooksPtr) m_setting.get());
#endif
    }

      // For jet matching, initialise the respective user hooks code.
      if (m_doMePsMatching) {
      m_matchingMG = std::unique_ptr<Pythia8::JetMatchingMadgraph>(new Pythia8::JetMatchingMadgraph());
      if (!m_matchingMG) {
	std::stringstream message;
	message << "Failed to initialise jet matching structures. " << std::endl;
	throw std::runtime_error(message.str());
      }
#if PYTHIA_VERSION_INTEGER < 8300
      m_pythia->setUserHooksPtr(m_matchingMG.get());
#else
      m_pythia->setUserHooksPtr((Pythia8::UserHooksPtr) m_matchingMG.get());
#endif
    }
 
  // jet clustering needed for matching
  m_slowJet = std::make_unique<Pythia8::SlowJet>(1, 0.4, 0, 4.4, 2, 2, nullptr, false);

  // End ME/PS Matching specific code


  // --  POWHEG settings
  int vetoMode    = m_pythia->settings.mode("POWHEG:veto");
  int MPIvetoMode = m_pythia->settings.mode("POWHEG:MPIveto");
  m_doPowheg  = (vetoMode > 0 || MPIvetoMode > 0);

  // Add in user hooks for shower vetoing
  if (m_doPowheg) {
  
    // Counters for number of ISR/FSR emissions vetoed
    m_nISRveto = 0, m_nFSRveto = 0;  
    
    // Set ISR and FSR to start at the kinematical limit
    if (vetoMode > 0) {
      m_pythia->readString("SpaceShower:pTmaxMatch = 2");
      m_pythia->readString("TimeShower:pTmaxMatch = 2");
    }

    // Set MPI to start at the kinematical limit
    if (MPIvetoMode > 0) {
      m_pythia->readString("MultipartonInteractions:pTmaxMatch = 2");
    }

    
    m_powhegHooks = new Pythia8::PowhegHooks();
    #if PYTHIA_VERSION_INTEGER < 8300
    m_pythia->addUserHooksPtr(m_powhegHooks);
    #else
    m_pythia->setUserHooksPtr((Pythia8::UserHooksPtr)m_powhegHooks);
    #endif
  }
  bool resonanceDecayFilter = m_pythia->settings.flag("ResonanceDecayFilter:filter");
  if (resonanceDecayFilter) {
    m_resonanceDecayFilterHook = new ResonanceDecayFilterHook();
    #if PYTHIA_VERSION_INTEGER < 8300
    m_pythia->addUserHooksPtr(m_resonanceDecayFilterHook);
    #else
    m_pythia->addUserHooksPtr((Pythia8::UserHooksPtr)m_resonanceDecayFilterHook);
    #endif
  }









    // Check if particle gun
    if(!m_spareFlag1) {
      m_inputFile = fopen(m_pythia->word("Beams:LHEF").c_str(), "r");
      if(m_inputFile) {
        reader = new DelphesLHEFReader;
        reader->SetInputFile(m_inputFile);

        m_brancheEventLHEF = m_treeWriter->NewBranch("EventLHEF", LHEFEvent::Class());
        m_branchWeightLHEF = m_treeWriter->NewBranch("WeightLHEF", LHEFWeight::Class());

        m_allParticleOutputArrayLHEF = modularDelphes->ExportArray("allParticlesLHEF");
        m_stableParticleOutputArrayLHEF = modularDelphes->ExportArray("stableParticlesLHEF");
        m_partonOutputArrayLHEF = modularDelphes->ExportArray("partonsLHEF");
      }
    }

    m_pythia->init();

    return outputfile;
  }

  inline int getNumberOfEvents() const override {return m_numberOfEvents;}

  inline std::string getUsage() const override {
    std::stringstream sstr;
    sstr << "Usage: " << m_appName << "config_file output_config_file pythia_card output_file\n"
         << "config_file - configuration file in Tcl format,\n"
         << "output_config_file - configuration file steering the content of the edm4hep output in Tcl format,\n"
         << "pythia_card - Pythia8 configuration file,\n"
         << "output_file - output file in ROOT format.\n";
    return sstr.str();
  }

  inline bool readEvent(Delphes* modularDelphes,
                        TObjArray* allParticleOutputArray,
                        TObjArray* stableParticleOutputArray,
                        TObjArray* partonOutputArray) override {

    m_treeWriter->Clear();
    auto factory = modularDelphes->GetFactory();
    while(reader && reader->ReadBlock(factory, m_allParticleOutputArrayLHEF, m_stableParticleOutputArrayLHEF, m_partonOutputArrayLHEF) && !reader->EventReady()) ;

      if(m_spareFlag1) {
        if((m_spareMode1 >= 1 && m_spareMode1 <= 5) || m_spareMode1 == 21) {
          fillPartons(m_spareMode1, m_spareParm1, m_spareParm2, m_pythia->event, m_pythia->particleData, m_pythia->rndm);
        } else {
          fillParticle(m_spareMode1, m_spareParm1, m_spareParm2, m_pythia->event, m_pythia->particleData, m_pythia->rndm);
        }
      }

      if(!m_pythia->next()) {
        // If failure because reached end of file then exit event loop
        if(m_pythia->info.atEndOfFile()) {
          std::cerr << "Aborted since reached end of Les Houches Event File" << std::endl;
          return false;
        }
        // First few failures write off as "acceptable" errors, then quit
        if(++m_errorCounter > m_timesAllowErrors) {
          std::cerr << "Event generation aborted prematurely, owing to error!" << std::endl;
          return false;
        }
        modularDelphes->Clear();
        reader->Clear();
      }

      /*
      if (m_doMePsMatching || m_doMePsMerging) {

	auto mePsMatchingVars = m_handleMePsMatchingVars.createAndPut();
	int njetNow = 0;
	std::vector<double> dijVec;
	
	// Construct input for jet algorithm.
	Pythia8::Event jetInput;
	jetInput.init("jet input", &(pythia->particleData));
	jetInput.clear();
	for (int i = 0; i < pythia->event.size(); ++i)
	  if (pythia->event[i].isFinal() &&
	      (pythia->event[i].colType() != 0 || pythia->event[i].isHadron()))
	    jetInput.append(pythia->event[i]);
	m_slowJet->setup(jetInput);
	// Run jet algorithm.
	std::vector<double> result;
	while (m_slowJet->sizeAll() - m_slowJet->sizeJet() > 0) {
	  result.push_back(sqrt(m_slowJet->dNext()));
	  m_slowJet->doStep();
	}
	
	// Reorder by decreasing multiplicity.
	for (int i = int(result.size()) - 1; i >= 0; --i)
	  dijVec.push_back(result[i]);
	
	// Now get the "number of partons" in the input event, so that
	// we may tag this event accordingly when histogramming. Note
	// that for MLM jet matching, this might not coincide with the
	// actual number of partons in the input LH event, since some
	// partons may be excluded from the matching.
	
	bool doShowerKt = pythia->settings.flag("JetMatching:doShowerKt");
	if (m_doMePsMatching && !doShowerKt) njetNow = m_matching->nMEpartons().first;
	else if (m_doMePsMatching && doShowerKt) {
	  njetNow = m_matching->getProcessSubset().size();
	} else if (m_doMePsMerging) {
	  njetNow = pythia->settings.mode("Merging:nRequested");
	  if (pythia->settings.flag("Merging:doUMEPSSubt") ||
	      pythia->settings.flag("Merging:doUNLOPSSubt") ||
	      pythia->settings.flag("Merging:doUNLOPSSubtNLO"))
	    njetNow--;
	}
	
	// Inclusive jet pTs as further validation plot.
	std::vector<double> ptVec;
	// Run jet algorithm.
	m_slowJet->analyze(jetInput);
	for (int i = 0; i < m_slowJet->sizeJet(); ++i)
	  ptVec.push_back(m_slowJet->pT(i));
	
	auto var = mePsMatchingVars->create();
	
	// 0th entry = number of generated partons
	var.value(njetNow);
	
	// odd  entries: d(ij) observables --- 1): d01, 3): d12, 5): d23, 7): d34
	// even entries: pT(i) observables --- 2): pT1, 4): pT2, 6): pT3, 8): pT4
	for (unsigned int i = 0; i < 4; ++i) {
	  var = mePsMatchingVars->create();
	  var.value(-999);
	  if (dijVec.size() > i) var.value(log10(dijVec[i]));
	  
	  var = mePsMatchingVars->create();
	  var.value(-999);
	  if (ptVec.size() > i) var.value(ptVec[i]);
	}
      }
      */
      m_readStopWatch.Stop();
      m_procStopWatch.Start();
      ConvertInput(m_eventCounter, m_pythia.get(), m_branchEvent.get(), factory,
        allParticleOutputArray, stableParticleOutputArray, partonOutputArray,
        &m_readStopWatch, &m_procStopWatch);
      ++m_eventCounter;
    return true;
    };

  inline bool finished() const override {return m_eventCounter >= m_numberOfEvents;};

  TTree* converterTree() override { return m_treeWriter->GetTree(); }

private:
  static constexpr const char* m_appName = "DelphesPythia8";
  std::unique_ptr<Pythia8::Pythia> m_pythia{nullptr};

  FILE *m_inputFile = 0;
  TStopwatch m_readStopWatch, m_procStopWatch;
  ExRootTreeWriter* m_treeWriter{nullptr};
  std::unique_ptr<ExRootTreeBranch> m_branchEvent{nullptr};
  std::unique_ptr<TTree> m_converterTree{nullptr};

  ExRootTreeBranch *m_brancheEventLHEF = 0, *m_branchWeightLHEF = 0;
  TObjArray *m_stableParticleOutputArrayLHEF = 0, *m_allParticleOutputArrayLHEF = 0, *m_partonOutputArrayLHEF = 0;
  DelphesLHEFReader *reader = 0;
  Long64_t m_eventCounter{0}, m_errorCounter{0};
  Long64_t m_numberOfEvents, m_timesAllowErrors;
  Bool_t m_spareFlag1;
  Int_t m_spareMode1;
  Double_t m_spareParm1, m_spareParm2;

  TClonesArray* m_branchParticle;
  TClonesArray* m_branchHepMCEvent;

  // for matching
  Pythia8::CombineMatchingInput *combined = 0;
  Pythia8::UserHooks *m_matching = 0;
  /// Pythia8 engine for jet clustering
  std::unique_ptr<Pythia8::SlowJet> m_slowJet{nullptr};

  // -- aMCatNLO
  bool m_doMePsMatching{false};
  bool m_doMePsMerging{false};
  /// Pythia8 engine for ME/PS matching
  std::unique_ptr<Pythia8::JetMatchingMadgraph> m_matchingMG{nullptr};
  /// Pythia8 engine for NLO ME/PS merging
  std::unique_ptr<Pythia8::amcnlo_unitarised_interface> m_setting{nullptr};


// Powheg
  bool m_doPowheg{false};
  unsigned long int m_nISRveto{0};
  unsigned long int m_nFSRveto{0};    
  /// Pythia8 engine for Powheg ME/PS merging
  Pythia8::PowhegHooks* m_powhegHooks{nullptr};

  ResonanceDecayFilterHook* m_resonanceDecayFilterHook{nullptr};

};


#endif
