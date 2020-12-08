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
#include "classes/DelphesStream.h"
#include "classes/DelphesLHEFReader.h"
#include "modules/Delphes.h"
#include "ExRootAnalysis/ExRootTreeWriter.h"

#include "Pythia.h"
#include "Pythia8Plugins/CombineMatchingInput.h"
#include "Pythia8Plugins/PowhegHooks.h"
#include "Pythia8Plugins/aMCatNLOHooks.h"
#include "Pythia8Plugins/JetMatching.h"
#include "ResonanceDecayFilterHook.h"

#include <iostream>

//---------------------------------------------------------------------------


class DelphesPythia8Reader: public DelphesInputReader {
  public:
  inline DelphesPythia8Reader() {};
    inline ~DelphesPythia8Reader() {
      if (pythia) {
        delete pythia;
      }
    };

  inline bool init(Delphes* modularDelphes, int argc, char *argv[], std::string& outputfile) {
    if (argc != 5) {
      std::cout << "Usage: " << m_appName << "config_file output_config_file pythia_card output_file\n"
                << "config_file - configuration file in Tcl format,\n"
                << "output_config_file - configuration file steering the content of the edm4hep output in Tcl format,\n"
                << "pythia_card - Pythia8 configuration file,\n"
                << "output_file - output file in ROOT format." << std::endl;
      return false;
    }
    outputfile = argv[4];
    // Initialize Pythia
    pythia = new Pythia8::Pythia;


  //add settings for resonance decay filter
    pythia->settings.addFlag("ResonanceDecayFilter:filter", false);
    pythia->settings.addFlag("ResonanceDecayFilter:exclusive", false);
    pythia->settings.addFlag("ResonanceDecayFilter:eMuAsEquivalent", false);
    pythia->settings.addFlag("ResonanceDecayFilter:eMuTauAsEquivalent", false);
    pythia->settings.addFlag("ResonanceDecayFilter:allNuAsEquivalent", false);
    pythia->settings.addFlag("ResonanceDecayFilter:udscAsEquivalent", false);
    pythia->settings.addFlag("ResonanceDecayFilter:udscbAsEquivalent", false);
    pythia->settings.addFlag("ResonanceDecayFilter:wzAsEquivalent", false);
    pythia->settings.addMVec("ResonanceDecayFilter:mothers", std::vector<int>(), false, false, 0, 0);
    pythia->settings.addMVec("ResonanceDecayFilter:daughters", std::vector<int>(), false, false, 0, 0);

    /*
    // jet matching
#if PYTHIA_VERSION_INTEGER < 8300
    matching = combined->getHook(*pythia);
    if(!matching)
    {
      throw std::runtime_error("can't do matching");
    }
    pythia->setUserHooksPtr(matching);
#endif
    */

    if(pythia == NULL)
    {
      throw std::runtime_error("can't create Pythia instance");
    }

    // Read in commands from configuration file

    std::stringstream message;
    std::string pythia8configname(argv[3]);
    if(!pythia->readFile(pythia8configname))
    {
      message << "can't read Pythia8 configuration file " << pythia8configname << std::endl;
      throw std::runtime_error(message.str());
    }

    // Extract settings to be used in the main program
    numberOfEvents = pythia->mode("Main:numberOfEvents");

    m_numberOfEvents = pythia->mode("Main:numberOfEvents");
    timesAllowErrors = pythia->mode("Main:timesAllowErrors");
    spareFlag1 = pythia->flag("Main:spareFlag1");
    spareMode1 = pythia->mode("Main:spareMode1");
    spareParm1 = pythia->parm("Main:spareParm1");
    spareParm2 = pythia->parm("Main:spareParm2");

    // Begin ME/PS Matching specific code
    // Check if jet matching should be applied.
    
    m_doMePsMatching = pythia->settings.flag("JetMatching:merge");
    // Check if internal merging should be applied.
    m_doMePsMerging = !(pythia->settings.word("Merging:Process").compare("void") == 0);

    // Currently, only one scheme at a time is allowed.
    if (m_doMePsMerging && m_doMePsMatching) {
      message << "Jet matching and merging cannot be used simultaneously! " << std::endl;
      throw std::runtime_error(message.str());
    }


    // Allow to set the number of additional partons dynamically.
    if (m_doMePsMerging) {
      // Store merging scheme.
      int scheme;
      if (pythia->settings.flag("Merging:doUMEPSTree") || pythia->settings.flag("Merging:doUMEPSSubt")) {
	scheme = 1;
      } else if (pythia->settings.flag("Merging:doUNLOPSTree") ||
		 pythia->settings.flag("Merging:doUNLOPSSubt") ||
		 pythia->settings.flag("Merging:doUNLOPSLoop") ||
		 pythia->settings.flag("Merging:doUNLOPSSubtNLO")) {
	scheme = 2;
      } else {
	scheme = 0;
      }
      
      m_setting = std::unique_ptr<Pythia8::amcnlo_unitarised_interface>(new Pythia8::amcnlo_unitarised_interface(scheme));
#if PYTHIA_VERSION_INTEGER < 8300
      pythia->setUserHooksPtr(m_setting.get());
#else
      pythia->setUserHooksPtr((Pythia8::UserHooksPtr) m_setting.get());
#endif
    }

      // For jet matching, initialise the respective user hooks code.
      if (m_doMePsMatching) {
      m_matching = std::unique_ptr<Pythia8::JetMatchingMadgraph>(new Pythia8::JetMatchingMadgraph());
      if (!m_matching) {
      message << "Failed to initialise jet matching structures. " << std::endl;
      throw std::runtime_error(message.str());
    }
#if PYTHIA_VERSION_INTEGER < 8300
      pythia->setUserHooksPtr(m_matching.get());
#else
      pythia->setUserHooksPtr((Pythia8::UserHooksPtr) m_matching.get());
#endif
    }
 
  // jet clustering needed for matching
  m_slowJet = std::make_unique<Pythia8::SlowJet>(1, 0.4, 0, 4.4, 2, 2, nullptr, false);

  // End ME/PS Matching specific code


  // --  POWHEG settings
  int vetoMode    = pythia->settings.mode("POWHEG:veto");
  int MPIvetoMode = pythia->settings.mode("POWHEG:MPIveto");
  m_doPowheg  = (vetoMode > 0 || MPIvetoMode > 0);

  // Add in user hooks for shower vetoing
  if (m_doPowheg) {
  
    // Counters for number of ISR/FSR emissions vetoed
    m_nISRveto = 0, m_nFSRveto = 0;  
    
    // Set ISR and FSR to start at the kinematical limit
    if (vetoMode > 0) {
      pythia->readString("SpaceShower:pTmaxMatch = 2");
      pythia->readString("TimeShower:pTmaxMatch = 2");
    }

    // Set MPI to start at the kinematical limit
    if (MPIvetoMode > 0) {
      pythia->readString("MultipartonInteractions:pTmaxMatch = 2");
    }

    
    m_powhegHooks = new Pythia8::PowhegHooks();
    #if PYTHIA_VERSION_INTEGER < 8300
    pythia->addUserHooksPtr(m_powhegHooks);
    #else
    pythia->setUserHooksPtr((Pythia8::UserHooksPtr)m_powhegHooks);
    #endif
  }
  bool resonanceDecayFilter = pythia->settings.flag("ResonanceDecayFilter:filter");
  if (resonanceDecayFilter) {
    m_resonanceDecayFilterHook = new ResonanceDecayFilterHook();
    #if PYTHIA_VERSION_INTEGER < 8300
    pythia->addUserHooksPtr(m_resonanceDecayFilterHook);
    #else
    pythia->addUserHooksPtr((Pythia8::UserHooksPtr)m_resonanceDecayFilterHook);
    #endif
  }









    // Check if particle gun
    if(!spareFlag1) {
      inputFile = fopen(pythia->word("Beams:LHEF").c_str(), "r");
      if(inputFile) {
        reader = new DelphesLHEFReader;
        reader->SetInputFile(inputFile);

        //branchEventLHEF = treeWriter->NewBranch("EventLHEF", LHEFEvent::Class());
        //branchWeightLHEF = treeWriter->NewBranch("WeightLHEF", LHEFWeight::Class());

        allParticleOutputArrayLHEF = modularDelphes->ExportArray("allParticlesLHEF");
        stableParticleOutputArrayLHEF = modularDelphes->ExportArray("stableParticlesLHEF");
        partonOutputArrayLHEF = modularDelphes->ExportArray("partonsLHEF");
      }
    }

    pythia->init();

    return true;

  };
  inline int getNumberOfEvents() {return m_numberOfEvents;}

  inline std::string getUsage() {return m_appName;};

  inline bool readEvent(Delphes* modularDelphes, TObjArray* allParticleOutputArray,
  TObjArray* stableParticleOutputArray, TObjArray* partonOutputArray) {

    auto factory = modularDelphes->GetFactory();
      while(reader && reader->ReadBlock(factory, allParticleOutputArrayLHEF, stableParticleOutputArrayLHEF, partonOutputArrayLHEF) && !reader->EventReady()) ;

      if(spareFlag1) {
        if((spareMode1 >= 1 && spareMode1 <= 5) || spareMode1 == 21) {
          fillPartons(spareMode1, spareParm1, spareParm2, pythia->event, pythia->particleData, pythia->rndm);
        } else {
          fillParticle(spareMode1, spareParm1, spareParm2, pythia->event, pythia->particleData, pythia->rndm);
        }
      }

      if(!pythia->next()) {
        // If failure because reached end of file then exit event loop
        if(pythia->info.atEndOfFile()) {
          std::cerr << "Aborted since reached end of Les Houches Event File" << std::endl;
          return false;
        }
        // First few failures write off as "acceptable" errors, then quit
        if(++errorCounter > timesAllowErrors) {
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
      readStopWatch.Stop();
      procStopWatch.Start();
      ConvertInput(eventCounter, pythia, branchEvent, factory,
        allParticleOutputArray, stableParticleOutputArray, partonOutputArray,
        &readStopWatch, &procStopWatch);
      ++m_entry;
    return true;
    };

  inline bool finished() {return m_entry >= m_numberOfEvents;};

private:
  const std::string m_appName = "DelphesPythia8";
  const std::string m_usage;
  int m_numberOfEvents;
  int m_entry = 0;
  Pythia8::Pythia* pythia{nullptr};
  FILE *inputFile = 0;
  TFile *outputFile = 0;
  TStopwatch readStopWatch, procStopWatch;
  ExRootTreeWriter *treeWriter = 0;
  ExRootTreeBranch *branchEvent = 0;
  ExRootTreeBranch *branchEventLHEF = 0, *branchWeightLHEF = 0;
  ExRootConfReader *confReader = 0;
  TObjArray *stableParticleOutputArrayLHEF = 0, *allParticleOutputArrayLHEF = 0, *partonOutputArrayLHEF = 0;
  DelphesLHEFReader *reader = 0;
  Long64_t eventCounter, errorCounter;
  Long64_t numberOfEvents, timesAllowErrors;
  Bool_t spareFlag1;
  Int_t spareMode1;
  Double_t spareParm1, spareParm2;

  TClonesArray* m_branchParticle;
  TClonesArray* m_branchHepMCEvent;

  // for matching
  Pythia8::CombineMatchingInput *combined = 0;
  Pythia8::UserHooks *matching = 0;
  /// Pythia8 engine for jet clustering
  std::unique_ptr<Pythia8::SlowJet> m_slowJet{nullptr};

  // -- aMCatNLO
  bool m_doMePsMatching{false};
  bool m_doMePsMerging{false};
  /// Pythia8 engine for ME/PS matching
  std::unique_ptr<Pythia8::JetMatchingMadgraph> m_matching{nullptr};
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
