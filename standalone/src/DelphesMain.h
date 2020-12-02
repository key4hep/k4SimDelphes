#include "DelphesInputReader.h"
#include "k4SimDelphes/DelphesEDM4HepOutputConfiguration.h"
#include "k4SimDelphes/DelphesEDM4HepConverter.h"

#include "podio/EventStore.h"
#include "podio/ROOTWriter.h"

#include "modules/Delphes.h"
#include "ExRootAnalysis/ExRootConfReader.h"
#include "ExRootAnalysis/ExRootProgressBar.h"

#include <memory>
#include <signal.h>
#include <iostream>
#include <memory>

static bool interrupted = false;
void SignalHandler(int /*si*/) {
  interrupted = true;
}


template<typename WriterT=podio::ROOTWriter>
int doit(int argc, char* argv[], DelphesInputReader& inputReader) {
  using namespace k4SimDelphes;

  // We can't make this a unique_ptr because it interferes with whatever ROOT is
  // doing under the hood to clean up
  auto* modularDelphes = new Delphes("Delphes");
  const auto outputFile = inputReader.init(modularDelphes, argc, argv);
  if (outputFile.empty()) {
    std::cerr << inputReader.getUsage() << std::endl;
    return 1;
  }

  signal(SIGINT, SignalHandler);
  try {
    auto confReader = std::make_unique<ExRootConfReader>();
    confReader->ReadFile(argv[1]);
    modularDelphes->SetConfReader(confReader.get());

    const auto branches = getBranchSettings(confReader->GetParam("TreeWriter::Branch"));
    const auto edm4hepOutputSettings = getEDM4hepOutputSettings(argv[2]);
    DelphesEDM4HepConverter edm4hepConverter(branches,
                                             edm4hepOutputSettings,
                                             confReader->GetDouble("ParticlePropagator::Bz", 0));

    // Now that the converter is setup, we can also actually register the
    // collections with the EventStore and add them to the writer for output
    podio::EventStore eventStore;
    WriterT podioWriter(outputFile, &eventStore);
    auto collections = edm4hepConverter.getCollections();
    for (auto& c: collections) {
      eventStore.registerCollection(std::string(c.first), c.second);
      podioWriter.registerForWrite(std::string(c.first));
    }

    // has to happen before InitTask
    TObjArray* allParticleOutputArray = modularDelphes->ExportArray("allParticles");
    TObjArray* stableParticleOutputArray = modularDelphes->ExportArray("stableParticles");
    TObjArray* partonOutputArray = modularDelphes->ExportArray("partons");

    modularDelphes->InitTask();
    modularDelphes->Clear();

    const int maxEvents = confReader->GetInt("::MaxEvents", 0);
    ExRootProgressBar progressBar(-1);
    Int_t eventCounter = 0;
    for (Int_t entry = 0;
         !inputReader.finished() && (maxEvents > 0 ?  entry < maxEvents : true) && !interrupted;
         ++entry) {

      if (!inputReader.readEvent(modularDelphes,
                                 allParticleOutputArray,
                                 stableParticleOutputArray,
                                 partonOutputArray)) {
        break;
      }

      modularDelphes->ProcessTask();
      edm4hepConverter.process(inputReader.converterTree());
      podioWriter.writeEvent();
      eventStore.clearCollections();

      modularDelphes->Clear();
      progressBar.Update(eventCounter, eventCounter);
      eventCounter++;
    }

    progressBar.Update(eventCounter, eventCounter, true);
    progressBar.Finish();
    podioWriter.finish();
    modularDelphes->Finish();
    std::cout << "** Exiting ..." << std::endl;

  } catch (std::runtime_error& e) {
    std::cerr << "** ERROR: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
