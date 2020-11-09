#include "DelphesInputReader.h"

#include "k4simdelphes/DelphesEDM4HepConverter.h"
#include "k4simdelphes/DelphesEDM4HepOutputConfiguration.h"

#include "TObjArray.h"

#include "ExRootAnalysis/ExRootProgressBar.h"
#include "Delphes.h"

#include <iostream>
#include <signal.h> // SIGINT
#include <memory>

using namespace k4simdelphes;

// gracefully handle ctrl+c
static bool interrupted = false;
void SignalHandler(int /*sig*/) {
  interrupted = true;
}


int doit(int argc, char* argv[], DelphesInputReader& inputReader) {
    Delphes* modularDelphes = new Delphes("Delphes");
    std::string outputfile;
    if (!inputReader.init(modularDelphes, argc, argv, outputfile)) {
        return 1;
    }
    signal(SIGINT, SignalHandler);

    try {
      auto confReader = std::make_unique<ExRootConfReader>();
      confReader->ReadFile(argv[1]);
      modularDelphes->SetConfReader(confReader.get());

      // since even ExRootConfParam::GetSize() is not marked const it is useless
      // to get a const version of it here
      auto branches = confReader->GetParam("TreeWriter::Branch");

      auto confReaderEDM4HEP = std::make_unique<ExRootConfReader>();
      confReaderEDM4HEP->ReadFile(argv[2]);
      const auto edm4hepOutSettings = getEDM4hepOutputSettings(confReaderEDM4HEP.get());
      podio::EventStore eventstore;
      podio::ROOTWriter podio_writer(outputfile, &eventstore);
      DelphesEDM4HepConverter edm4hepConverter(branches,
                                               edm4hepOutSettings,
                                               confReader->GetDouble("ParticlePropagator::Bz", 0) );
      auto collections = edm4hepConverter.getCollections();
      for (auto& c: collections) {
        eventstore.registerCollection(std::string(c.first), c.second);
        podio_writer.registerForWrite(std::string(c.first));
      }

      // has to happen before InitTask
      TObjArray* allParticleOutputArray = modularDelphes->ExportArray("allParticles");
      TObjArray* stableParticleOutputArray = modularDelphes->ExportArray("stableParticles");
      TObjArray* partonOutputArray = modularDelphes->ExportArray("partons");

      modularDelphes->InitTask();
      modularDelphes->Clear();

      int maxEvents = confReader->GetInt("::MaxEvents", 0);
      ExRootProgressBar progressBar(-1);
      Int_t eventCounter = 0;
      for (Int_t entry = 0;
           !inputReader.finished() && (maxEvents > 0 ?  entry < maxEvents : true) && !interrupted;
           ++entry) {

        bool success = inputReader.readEvent(modularDelphes,
                                             allParticleOutputArray,
                                             stableParticleOutputArray,
                                             partonOutputArray);
        if (!success) {
          break;
        }

        modularDelphes->ProcessTask();
        edm4hepConverter.process(modularDelphes);
        podio_writer.writeEvent();
        eventstore.clearCollections();

        modularDelphes->Clear();
        progressBar.Update(eventCounter, eventCounter);
        eventCounter++;
      }

      progressBar.Update(eventCounter, eventCounter, true);
      progressBar.Finish();
      podio_writer.finish();
      modularDelphes->Finish();
      std::cout << "** Exiting ..." << std::endl;


    } catch (std::runtime_error& e) {
      std::cerr << "** ERROR: " << e.what() << std::endl;
      return 1;
    }

    return 0;
}
