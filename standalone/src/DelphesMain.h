#include "DelphesInputReader.h"
#include "k4SimDelphes/DelphesEDM4HepConverter.h"
#include "k4SimDelphes/DelphesEDM4HepOutputConfiguration.h"

#include "podio/Frame.h"
#include "podio/Writer.h"

#include "ExRootAnalysis/ExRootConfReader.h"
#include "ExRootAnalysis/ExRootProgressBar.h"
#include "modules/Delphes.h"

#include <iostream>
#include <memory>
#include <signal.h>
#include <string_view>
#include <vector>

static bool interrupted = false;
void SignalHandler(int /*si*/) { interrupted = true; }

int doit(int argc, char* argv[], DelphesInputReader& inputReader) {
  using namespace k4SimDelphes;

  // Look for an optional "--rntuple" flag, which can appear anywhere among the
  // arguments, to request writing the EDM4hep output with the RNTuple backend
  // instead of the default TTree based one. It is stripped out before the
  // purely positional argument parsing done by the reader below.
  std::string writerType = "default";
  std::vector<char*> args;
  args.reserve(argc);
  args.push_back(argv[0]);
  for (int i = 1; i < argc; ++i) {
    if (std::string_view(argv[i]) == "--rntuple") {
      writerType = "rntuple";
    } else {
      args.push_back(argv[i]);
    }
  }
  const int newArgc = static_cast<int>(args.size());
  char** const newArgv = args.data();

  // We can't make this a unique_ptr because it interferes with whatever ROOT is
  // doing under the hood to clean up
  auto* modularDelphes = new Delphes("Delphes");
  const auto outputFile = inputReader.init(modularDelphes, newArgc, newArgv);
  if (outputFile.empty()) {
    // Check if the user requested the help, and print the usage message and
    // return succesfully in that case
    if (newArgc > 1 && (newArgv[1] == std::string_view("--help") || newArgv[1] == std::string_view("-h"))) {
      std::cout << inputReader.getUsage() << std::endl;
      return 0;
    }
    std::cerr << inputReader.getUsage() << std::endl;
    return 1;
  }

  // "default" falls back to the PODIO_DEFAULT_WRITE_RNTUPLE environment
  // variable (unset/empty -> TTree, anything else -> RNTuple).
  auto podioWriter = podio::makeWriter(outputFile, writerType);

  signal(SIGINT, SignalHandler);
  try {
    auto confReader = std::make_unique<ExRootConfReader>();
    confReader->ReadFile(newArgv[1]);
    modularDelphes->SetConfReader(confReader.get());

    const auto branches = getBranchSettings(confReader->GetParam("TreeWriter::Branch"));
    const auto edm4hepOutputSettings = getEDM4hepOutputSettings(newArgv[2]);
    DelphesEDM4HepConverter edm4hepConverter(branches, edm4hepOutputSettings,
                                             confReader->GetDouble("ParticlePropagator::Bz", 0));

    // has to happen before InitTask
    TObjArray* allParticleOutputArray = modularDelphes->ExportArray("allParticles");
    TObjArray* stableParticleOutputArray = modularDelphes->ExportArray("stableParticles");
    TObjArray* partonOutputArray = modularDelphes->ExportArray("partons");

    modularDelphes->InitTask();
    modularDelphes->Clear();

    const int maxEvents = confReader->GetInt("::MaxEvents", 0);
    ExRootProgressBar progressBar(-1);
    Int_t eventCounter = 0;
    for (Int_t entry = 0; !inputReader.finished() && (maxEvents > 0 ? entry < maxEvents : true) && !interrupted;
         ++entry) {
      if (!inputReader.readEvent(modularDelphes, allParticleOutputArray, stableParticleOutputArray,
                                 partonOutputArray)) {
        break;
      }

      modularDelphes->ProcessTask();
      edm4hepConverter.process(inputReader.converterTree());

      // Put everything into a Frame and write it out
      podio::Frame frame;
      for (auto& [name, coll] : edm4hepConverter.getCollections()) {
        frame.put(std::move(coll), name);
      }
      podioWriter.writeFrame(frame, "events");

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
