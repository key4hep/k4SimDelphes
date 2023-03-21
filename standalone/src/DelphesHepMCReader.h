#ifndef DELPHESEDM4HEP_DELPHESHEPMCREADER
#define DELPHESEDM4HEP_DELPHESHEPMCREADER

#include "DelphesInputReader.h"

#include "TChain.h"
#include "TClonesArray.h"
#include "TObjArray.h"
#include "TStopwatch.h"

#include "classes/DelphesClasses.h"
#include "classes/DelphesFactory.h"
#include "classes/DelphesHepMC2Reader.h"
#include "classes/DelphesStream.h"
#include "modules/Delphes.h"

#include "ExRootAnalysis/ExRootTreeBranch.h"
#include "ExRootAnalysis/ExRootTreeReader.h"
#include "ExRootAnalysis/ExRootTreeWriter.h"

#include <iostream>
#include <memory>
#include <sstream>
#include <string>

//// TODO: handle case of more than one input file

class DelphesHepMCInputReader : public DelphesInputReader {
public:
  std::string init(Delphes* modularDelphes, int argc, char* argv[]) override {
    if (argc < 4) {
      return "";
    }
    std::string outputfile = argv[3];

    int i = 4;

    m_reader = std::make_unique<DelphesHepMC2Reader>();

    Long64_t length = 0;
    if (i == argc || strncmp(argv[i], "-", 2) == 0) {
      std::cout << "** Reading standard input" << std::endl;
      m_inputFile = stdin;
      length      = -1;
    } else {
      std::cout << "** Reading " << argv[i] << std::endl;
      m_inputFile = fopen(argv[i], "r");

      if (m_inputFile == NULL) {
        std::stringstream message;
        message << "can't open " << argv[i];
        throw std::runtime_error(message.str());
      }

      fseek(m_inputFile, 0L, SEEK_END);
      length = ftello(m_inputFile);
      fseek(m_inputFile, 0L, SEEK_SET);

      if (length <= 0) {
        fclose(m_inputFile);
        ++i;
        //continue;
      }
    }

    m_reader->SetInputFile(m_inputFile);

    m_treeWriter    = new ExRootTreeWriter(nullptr, "Delphes");
    m_converterTree = std::make_unique<TTree>("ConverterTree", "Analysis");
    // avoid having any connection with a TFile that might be opened later
    m_converterTree->SetDirectory(nullptr);
    m_treeWriter->SetTree(m_converterTree.get());
    modularDelphes->SetTreeWriter(m_treeWriter);

    m_branchEvent  = m_treeWriter->NewBranch("Event", HepMCEvent::Class());
    m_branchWeight = m_treeWriter->NewBranch("Weight", Weight::Class());

    return outputfile;
  };

  int getNumberOfEvents() const override { return m_numberOfEvents; }

  std::string getUsage() const override {
    std::stringstream sstr;
    sstr << "Usage: " << m_appName << " config_file output_config_file output_file [input_file(s)]\n"
         << "config_file - configuration file in Tcl format,\n"
         << "output_config_file - configuration file steering the content of the edm4hep output in Tcl format,\n"
         << "output_file - output file in ROOT format,\n"
         << "input_file(s) - input file(s) in HepMC format,\n"
         << "with no input_file, or when input_file is -, read standard input.\n";
    return sstr.str();
  }

  bool readEvent(Delphes* modularDelphes, TObjArray* allParticleOutputArray, TObjArray* stableParticleOutputArray,
                 TObjArray* partonOutputArray) override {
    m_treeWriter->Clear();
    m_readStopWatch.Start();
    auto factory  = modularDelphes->GetFactory();
    bool goodRead = false;
    while ((goodRead =
                m_reader->ReadBlock(factory, allParticleOutputArray, stableParticleOutputArray, partonOutputArray)) &&
           !m_reader->EventReady()) {
    }
    m_readStopWatch.Stop();
    m_reader->AnalyzeEvent(m_branchEvent, m_eventCounter, &m_readStopWatch, &m_procStopWatch);
    m_reader->AnalyzeWeight(m_branchWeight);
    m_reader->Clear();
    m_eventCounter++;
    return goodRead;
  }

  // For the HepMC reader there is no real way of determining on whether it is
  // finished or not, so we return always false here, because readEvent will
  // report on whether it was possible to read or not
  bool finished() const override { return false; };

  TTree* converterTree() override { return m_treeWriter->GetTree(); }

private:
  static constexpr const char* m_appName = "DelphesHepMC";
  int                          m_numberOfEvents;
  int                          m_entry      = 0;
  ExRootTreeReader*            m_treeReader = nullptr;
  TClonesArray*                m_branchParticle;
  TClonesArray*                m_branchHepMCEvent;

  ExRootTreeWriter*      m_treeWriter{nullptr};
  std::unique_ptr<TTree> m_converterTree{nullptr};

  FILE*                                m_inputFile = 0;
  TStopwatch                           m_readStopWatch, m_procStopWatch;
  ExRootTreeBranch *                   m_branchEvent = 0, *m_branchWeight = 0;
  std::unique_ptr<DelphesHepMC2Reader> m_reader = 0;
  Long64_t                             m_eventCounter;
};

#endif
