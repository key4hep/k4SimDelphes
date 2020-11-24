#ifndef DELPHESEDM4HEP_DELPHESTDREADER
#define DELPHESEDM4HEP_DELPHESTDREADER


#include "DelphesInputReader.h"

#include "classes/DelphesClasses.h"
#include "classes/DelphesFactory.h"
#include "classes/DelphesStream.h"
#include "classes/DelphesSTDHEPReader.h"
#include "modules/Delphes.h"

#include "ExRootAnalysis/ExRootTreeBranch.h"
#include "ExRootTreeWriter.h" // use local copy

#include "TObjArray.h"
#include "TTree.h"
#include "TClonesArray.h"
#include "TStopwatch.h"

#include <iostream>
#include <memory>

//// TODO: handle case of more than one input file

class DelphesSTDHEPInputReader : public DelphesInputReader {
public:
  DelphesSTDHEPInputReader() {};

  std::string init(Delphes* modularDelphes, int argc, char *argv[]) override {
    if (argc < 4) {
      return "";
    }


    std::string outputfile = argv[3];

    int i = 4;

    m_branchEvent = std::make_unique<ExRootTreeBranch>("Event", LHEFEvent::Class());
    m_reader = std::make_unique<DelphesSTDHEPReader>();

    Long64_t length = 0;

      if(i == argc || strncmp(argv[i], "-", 2) == 0)
      {
        std::cout << "** Reading standard input" << std::endl;
        m_inputFile = stdin;
        length = -1;
      }
      else
      {
        std::cout << "** Reading " << argv[i] << std::endl;
        m_inputFile = fopen(argv[i], "r");

        if(m_inputFile == NULL)
        {
          std::stringstream message;
          message << "can't open " << argv[i];
          throw std::runtime_error(message.str());
        }

        fseek(m_inputFile, 0L, SEEK_END);
        length = ftello(m_inputFile);
        fseek(m_inputFile, 0L, SEEK_SET);

        if(length <= 0)
        {
          fclose(m_inputFile);
          ++i;
          //continue;
        }
      }
      // TODO: multiple input files
      m_reader->SetInputFile(m_inputFile);

    m_treeWriter = new ExRootTreeWriter(nullptr, "Delphes");
    m_converterTree = std::make_unique<TTree>("ConverterTree", "Analysis");
    // avoid having any connection with a TFile that might be opened later
    m_converterTree->SetDirectory(nullptr);
    m_treeWriter->SetTree(m_converterTree.get());
    modularDelphes->SetTreeWriter(m_treeWriter);

    return outputfile;

    };

  int getNumberOfEvents() const override {return m_numberOfEvents;}

  std::string getUsage() const override {
    std::stringstream sstr;
    sstr << "Usage: " << m_appName << " config_file output_config_file output_file [input_file(s)]\n"
         << "config_file - configuration file in Tcl format,\n"
         << "output_config_file - configuration file steering the content of the edm4hep output in Tcl format,\n"
         << "output_file - output file in ROOT format,\n"
         << "input_file(s) - input file(s) in STDHEP format,\n"
         << "with no input_file, or when input_file is -, read standard input.\n";
    return sstr.str();
  };

  bool readEvent(Delphes* modularDelphes,
                 TObjArray* allParticleOutputArray,
                 TObjArray* stableParticleOutputArray,
                 TObjArray* partonOutputArray) override {
      m_reader->Clear();
      m_treeWriter->Clear();
      m_readStopWatch.Start();
      auto factory = modularDelphes->GetFactory();
      while(m_reader->ReadBlock(factory, allParticleOutputArray, stableParticleOutputArray, partonOutputArray)) {
        if (m_reader->EventReady()) {
          m_readStopWatch.Stop();
          m_eventCounter++;
          m_reader->AnalyzeEvent(m_branchEvent.get(), m_eventCounter, &m_readStopWatch, &m_procStopWatch);
          return true;
        }
      }
      m_finished = true; // ?
      return false;
    }

  bool finished() const override {return m_finished;}

  TTree* converterTree() override { return m_treeWriter->GetTree(); }

private:
  static constexpr const char* m_appName = "DelphesHepMC";
  int m_numberOfEvents;
  int m_entry = 0;
  bool m_finished = false;
  ExRootTreeWriter* m_treeReader = nullptr;

  FILE *m_inputFile = 0;
  TStopwatch m_readStopWatch, m_procStopWatch;
  ExRootTreeWriter *m_treeWriter{nullptr};
  std::unique_ptr<TTree> m_converterTree{nullptr};
  std::unique_ptr<ExRootTreeBranch> m_branchEvent{nullptr};

  std::unique_ptr<DelphesSTDHEPReader> m_reader{nullptr};

  Long64_t m_eventCounter;

};


#endif
