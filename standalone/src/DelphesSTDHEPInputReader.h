#ifndef DELPHESEDM4HEP_DELPHESTDREADER
#define DELPHESEDM4HEP_DELPHESTDREADER


#include "DelphesInputReader.h"

#include "classes/DelphesClasses.h"
#include "classes/DelphesFactory.h"
#include "classes/DelphesStream.h"
#include "classes/DelphesSTDHEPReader.h"
#include "modules/Delphes.h"

#include "ExRootAnalysis/ExRootTreeBranch.h"
#include "ExRootAnalysis/ExRootTreeReader.h"

#include "TObjArray.h"
#include "TChain.h"
#include "TClonesArray.h"
#include "TStopwatch.h"

#include <iostream>


//// TODO: handle case of more than one input file

using std::stringstream;
using std::runtime_error;
using std::endl;
using std::cout;

class DelphesSTDHEPInputReader: public DelphesInputReader {
public:
  DelphesSTDHEPInputReader() {};

  std::string init(Delphes* modularDelphes, int argc, char *argv[]) override {
    if (argc < 4) {
      return "";
    }


    std::string outputfile = argv[3];

    int i = 4;

    branchEvent = new ExRootTreeBranch("Event", LHEFEvent::Class());
    reader = new DelphesSTDHEPReader;

      if(i == argc || strncmp(argv[i], "-", 2) == 0)
      {
        cout << "** Reading standard input" << endl;
        inputFile = stdin;
        length = -1;
      }
      else
      {
        cout << "** Reading " << argv[i] << endl;
        inputFile = fopen(argv[i], "r");

        if(inputFile == NULL)
        {
          message << "can't open " << argv[i];
          throw runtime_error(message.str());
        }

        fseek(inputFile, 0L, SEEK_END);
        length = ftello(inputFile);
        fseek(inputFile, 0L, SEEK_SET);

        if(length <= 0)
        {
          fclose(inputFile);
          ++i;
          //continue;
        }
      }
      // TODO: multiple input files
      reader->SetInputFile(inputFile);

    return outputfile;

    };

  inline int getNumberOfEvents() const override {return m_numberOfEvents;}

  inline std::string getUsage() const override {
    std::stringstream sstr;
    sstr << "Usage: " << m_appName << " config_file output_config_file output_file [input_file(s)]\n"
         << "config_file - configuration file in Tcl format,\n"
         << "output_config_file - configuration file steering the content of the edm4hep output in Tcl format,\n"
         << "output_file - output file in ROOT format,\n"
         << "input_file(s) - input file(s) in STDHEP format,\n"
         << "with no input_file, or when input_file is -, read standard input.\n";
    return sstr.str();
  };

  inline bool readEvent(Delphes* modularDelphes,
                        TObjArray* allParticleOutputArray,
                        TObjArray* stableParticleOutputArray,
                        TObjArray* partonOutputArray) override {
      reader->Clear();
      readStopWatch.Start();
      auto factory = modularDelphes->GetFactory();
      while(reader->ReadBlock(factory, allParticleOutputArray, stableParticleOutputArray, partonOutputArray)) {
        if (reader->EventReady()) {
          readStopWatch.Stop();
          reader->AnalyzeEvent(branchEvent, eventCounter, &readStopWatch, &procStopWatch);
          return true;
        }
      }
      m_finished = true; // ?
      return false;
    }

  inline bool finished() const override {return m_finished;}

private:
  static constexpr const char* m_appName = "DelphesHepMC";
  int m_numberOfEvents;
  int m_entry = 0;
  bool m_finished = false;
  TChain* m_chain;
  ExRootTreeReader* m_treeReader =  nullptr;
  TClonesArray* m_branchParticle;
  TClonesArray* m_branchHepMCEvent;


  std::stringstream message;
  FILE *inputFile = 0;
  TFile *outputFile = 0;
  TStopwatch readStopWatch, procStopWatch;
  ExRootTreeWriter *treeWriter = 0;
  ExRootTreeBranch *branchEvent = 0;
  ExRootConfReader *confReader = 0;
  Delphes *modularDelphes = 0;
  DelphesFactory *factory = 0;
  TObjArray *stableParticleOutputArray = 0, *allParticleOutputArray = 0, *partonOutputArray = 0;
  DelphesSTDHEPReader *reader = 0;
  Int_t i, maxEvents, skipEvents;
  Long64_t length, eventCounter;

};


#endif
