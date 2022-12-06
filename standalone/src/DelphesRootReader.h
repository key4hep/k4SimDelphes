#ifndef DELPHESEDM4HEP_DELPHESROOTREADER
#define DELPHESEDM4HEP_DELPHESROOTREADER

#include "DelphesInputReader.h"

#include "TChain.h"
#include "TClonesArray.h"
#include "TObjArray.h"
#include "TStopwatch.h"

#include "classes/DelphesClasses.h"
#include "classes/DelphesFactory.h"
#include "classes/DelphesStream.h"
#include "modules/Delphes.h"

#include "ExRootAnalysis/ExRootTreeBranch.h"
#include "ExRootAnalysis/ExRootTreeReader.h"
#include "ExRootAnalysis/ExRootTreeWriter.h"

#include <iostream>
#include <sstream>

class DelphesRootReader : public DelphesInputReader {
public:
  std::string init(Delphes* modularDelphes, int argc, char* argv[]) override {
    if (argc < 5) {
      return "";
    }

    std::string outputfile = argv[3];

    m_chain = new TChain("Delphes");

    for (int i = 4; i < argc; ++i) {
      //std::cout << argv[i] << std::endl;
      m_chain->Add(argv[i]);
    }
    m_treeReader       = new ExRootTreeReader(m_chain);
    m_numberOfEvents   = m_treeReader->GetEntries();
    m_branchParticle   = m_treeReader->UseBranch("Particle");
    m_branchHepMCEvent = m_treeReader->UseBranch("Event");

    m_treeWriter    = new ExRootTreeWriter(nullptr, "Delphes");
    m_converterTree = std::make_unique<TTree>("ConverterTree", "Analysis");
    // avoid having any connection with a TFile that might be opened later
    m_converterTree->SetDirectory(nullptr);
    m_treeWriter->SetTree(m_converterTree.get());
    modularDelphes->SetTreeWriter(m_treeWriter);

    return outputfile;
  }

  int  getNumberOfEvents() const override { return m_numberOfEvents; }
  bool finished() const override { return m_entry >= m_numberOfEvents; }

  std::string getUsage() const override {
    std::stringstream sstr;
    sstr << "Usage: " << m_appName << " config_file output_config_file output_file input_file(s)\n"
         << "config_file - configuration file in Tcl format,\n"
         << "output_config_file - configuration file steering the content of the edm4hep output in Tcl format,\n"
         << "output_file - output file in ROOT format\n"
         << "input_file(s) - input file(s) in ROOT format\n";
    return sstr.str();
  }

  bool readEvent(Delphes* modularDelphes, TObjArray* allParticleOutputArray, TObjArray* stableParticleOutputArray,
                 TObjArray* partonOutputArray) override {
    m_treeWriter->Clear();
    m_treeReader->ReadEntry(m_entry);
    for (Int_t j = 0; j < m_branchParticle->GetEntriesFast(); j++) {
      gen                      = (GenParticle*)m_branchParticle->At(j);
      candidate                = modularDelphes->GetFactory()->NewCandidate();
      candidate->Momentum      = gen->P4();
      constexpr double c_light = 2.99792458e+8;
      candidate->Position.SetXYZT(gen->X, gen->Y, gen->Z, gen->T * 1.0E3 * c_light);
      candidate->PID    = gen->PID;
      candidate->Status = gen->Status;
      candidate->M1     = gen->M1;
      candidate->M2     = gen->M2;
      candidate->D1     = gen->D1;
      candidate->D2     = gen->D2;
      candidate->Charge = gen->Charge;
      candidate->Mass   = gen->Mass;
      allParticleOutputArray->Add(candidate);
      pdgCode = TMath::Abs(gen->PID);
      if (gen->Status == 1) {
        stableParticleOutputArray->Add(candidate);
      } else if (pdgCode <= 5 || pdgCode == 21 || pdgCode == 15) {
        partonOutputArray->Add(candidate);
      }
    }
    ++m_entry;

    return finished();
  };

  TTree* converterTree() override { return m_treeWriter->GetTree(); }

private:
  static constexpr const char* m_appName = "DelphesROOT";
  int                          m_numberOfEvents;
  int                          m_entry = 0;
  TChain*                      m_chain;
  ExRootTreeReader*            m_treeReader = nullptr;
  TClonesArray*                m_branchParticle;
  TClonesArray*                m_branchHepMCEvent;
  ExRootTreeWriter*            m_treeWriter{nullptr};
  std::unique_ptr<TTree>       m_converterTree{nullptr};

  GenParticle* gen;
  Candidate*   candidate;
  int          pdgCode;
};

#endif
