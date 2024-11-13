# v00-07-03

* 2024-11-13 Juraj Smiesko ([PR#137](https://github.com/key4hep/k4SimDelphes/pull/137))
  - Make sure to not crash in cases where the LHEF reader has not been initialized in the Pythia reader

# v00-07-02

* 2024-10-23 tmadlener ([PR#131](https://github.com/key4hep/k4SimDelphes/pull/131))
  - Split the global `ParticleIDs` collection into several smaller collections to facilitate downstream usage after the reversal of the `ParticleID` - `ReconstructedParticle` relation direction in [EDM4hep#268](https://github.com/key4hep/EDM4hep/pull/268).
    - `Tracks` will get a `ParticleIDCollection` with the suffix `_PID`
    - `Jets` will get a `ParticleIDCollection` with the suffix `_HF_tags` for any heavy flavor tags, and `_tau_tags` for any tau tags.

* 2024-10-15 Juraj Smiesko ([PR#134](https://github.com/key4hep/k4SimDelphes/pull/134))
  - Print x-sec error for Pythia process
  - Print info from LHEF file, if used
  - Print info from MadGraph LHEF file, if used

# v00-07-01

* 2024-10-14 Thomas Madlener ([PR#133](https://github.com/key4hep/k4SimDelphes/pull/133))
  - Update the CI README bagde
  - Add a zenodo badge

# v00-07

* 2024-10-04 tmadlener ([PR#130](https://github.com/key4hep/k4SimDelphes/pull/130))
  - Make MC-Reco links comparisons pass even if they are not the same order in EDM4hep and Delphes as long as all the links are present.

* 2024-10-04 tmadlener ([PR#128](https://github.com/key4hep/k4SimDelphes/pull/128))
  - Move away from the deprecated `Association`s to the new `Link`s (see[EDM4hep#341](https://github.com/key4hep/EDM4hep/pull/341))
    - The **`MCRecoAssociationCollectionName` output configuration parameter has been deprecated** and is **replaced by `RecoMCParticleLinkCollectionName`**. For the time being both will keep working, but the former will be removed in one of the upcoming releases. The default configuration has been adjusted accordingly.
  - Remove a few long outdated examples

* 2024-10-04 jmcarcell ([PR#127](https://github.com/key4hep/k4SimDelphes/pull/127))
  - Use the Key4hepConfig flag to set the standard, compiler flags and rpath magic.
  - Fix compiler warnings that were uncovered by this

* 2024-07-30 jmcarcell ([PR#126](https://github.com/key4hep/k4SimDelphes/pull/126))
  - Do not link to GaudiAlgLib
  - Add the necessary changes to the algorithm, use `Gaudi::Algorithm`, make `execute()` const and add a `const EventContext&`
  - Add mutable to some elements that are changed; add const to some functions so that they can be called inside `execute(const EventContext&) const`

* 2024-07-29 tmadlener ([PR#125](https://github.com/key4hep/k4SimDelphes/pull/125))
  - Fix a bug where the event loop was not entered in `DelphesROOT_EDM4HEP`

* 2024-07-29 jmcarcell ([PR#120](https://github.com/key4hep/k4SimDelphes/pull/120))
  - Delete the version checks for Podio before 1.0

* 2024-07-16 tmadlener ([PR#121](https://github.com/key4hep/k4SimDelphes/pull/121))
  - Store the dN/dx information from Delphes into a dedicated `RecDqdx` collection and relate the objects back to the tracks to keep things working after [EDM4hep#311](https://github.com/key4hep/EDM4hep/pull/311)

* 2024-06-26 jmcarcell ([PR#119](https://github.com/key4hep/k4SimDelphes/pull/119))
  - Don't set setRadiusOfInnermostHit for EDM4hep tracks

* 2024-05-01 tmadlener ([PR#117](https://github.com/key4hep/k4SimDelphes/pull/117))
  - Make the `ParticleID`s that are populated point back to the `ReconstructedParticle`s they belong to as required by [key4hep/EDM4hep#268](https://github.com/key4hep/EDM4hep/pull/268)

* 2024-03-11 tmadlener ([PR#118](https://github.com/key4hep/k4SimDelphes/pull/118))
  - Switch to the non-deprecated accessor function after changes in EDM4hep ([key4hep/EDM4hep#273](https://github.com/key4hep/EDM4hep/pull/273))

* 2024-02-22 tmadlener ([PR#113](https://github.com/key4hep/k4SimDelphes/pull/113))
  - Switch from `edm4hep::TrackerHit` to `edm4hep::TrackerHit3D` after its renaming upstream (https://github.com/key4hep/EDM4hep/pull/252) allowing for a transparent migration.

# v00-06-02

* 2024-02-22 tmadlener ([PR#116](https://github.com/key4hep/k4simdelphes/pull/116))
  - Switch to alma9 and key4hep image for running doctest CI workflow

* 2024-02-22 jmcarcell ([PR#115](https://github.com/key4hep/k4simdelphes/pull/115))
  - Change ROOTFrame{Writer,Reader} to ROOT{Writer,Reader} following https://github.com/AIDASoft/podio/pull/549

# v00-06-01

* 2024-02-05 tmadlener ([PR#114](https://github.com/key4hep/k4SimDelphes/pull/114))
  - Allow to build with c++20
  - Switch to new key4hep build action and remove old workflow

# v00-06

* 2023-11-02 Perez ([PR#112](https://github.com/key4hep/k4SimDelphes/pull/112))
  - The track's omega is taken directly from the curvature of the delphes track, instead of recomputing it from the track pT. Converting the curvature to pT and then back to omega leads to a worse numerical precision.

* 2023-09-12 jmcarcell ([PR#111](https://github.com/key4hep/k4SimDelphes/pull/111))
  - Fix a cmake warning by changing the minimum version to 3.5, since support for versions < 3.5 is going to be removed in the future
  - Rename the podioDict or edm4hepDict targets to podio or edm4hep in generator expressions for when they will be removed
  - Add ROOT libraries at link time for building k4SimDelphes together with other packages
  - Fix a test because the steering file was missing an `import os` (behaviour changed after https://github.com/key4hep/k4FWCore/pull/134)

# v00-05

* 2023-07-16 jmcarcell ([PR#109](https://github.com/key4hep/k4SimDelphes/pull/109))
  - Rename `CMAKE_BINARY_DIR` to `PROJECT_BINARY_DIR`

* 2023-07-13 jmcarcell ([PR#108](https://github.com/key4hep/k4SimDelphes/pull/108))
  - Rename `CMAKE_{SOURCE,BIN}_DIR` to `PROJECT_{SOURCE,BIN}_DIR`

* 2023-04-21 Birgit Stapf ([PR#107](https://github.com/key4hep/k4SimDelphes/pull/107))
  -  Add the Isolation Variable as computed by Delphes to the edm4hep conversion output as a UserDataCollection
  -  Add ParticleFlowCandidates as possible collection to converter

* 2023-04-19 Birgit Stapf ([PR#106](https://github.com/key4hep/k4SimDelphes/pull/106))
  - Add the event header to conversion from Delphes to edm4hep output. The event weight (as returned by generator) as well as the event number will be filled as members.

# v00-04

* 2023-04-12 Thomas Madlener ([PR#105](https://github.com/key4hep/k4SimDelphes/pull/105))
  - Adapt the default values of the output configuration parameters, such that it is possible to remove some output configuration parameters from the config file to skip conversions of certain collections. Fixes #100

* 2023-03-21 Thomas Madlener ([PR#103](https://github.com/key4hep/k4SimDelphes/pull/103))
  - Fix the HepMC reader that erroneously overwrote one of its input arguments. Fixes #102 
  - Fix stopping of HepMC reader after reading only one event. Fixes #34 
  - Fix leaking of the internal `DelphesHepMC2Reader` by using a `unique_ptr`.

* 2023-03-14 Thomas Madlener ([PR#104](https://github.com/key4hep/k4SimDelphes/pull/104))
  - Fix the pre-commit workflow and update actions to latest versions

# v00-03-01

# v00-03-00

* 2022-12-13 Michele Selvaggi ([PR#97](https://github.com/key4hep/k4simdelphes/pull/97))
  - store magnetic field strength in "magFieldBz" UserData

* 2022-12-06 Valentin Volkl ([PR#98](https://github.com/key4hep/k4simdelphes/pull/98))
  - Add some missing stl includes to fix a build error on macOS.

* 2022-11-23 Birgit Stapf ([PR#95](https://github.com/key4hep/k4simdelphes/pull/95))
  -  Adding Pythia's ResonanceDecayFilter plug-in as described in issue #92

* 2022-11-21 Michele Selvaggi ([PR#93](https://github.com/key4hep/k4simdelphes/pull/93))
  - set mass to KL or Photon for neutral clusters respectively
  - store position and time of cluster via calorimeterHit
  - set first track hit as MC truth track vertex
  - added vertex time to MC truth particle

* 2022-11-18 Thomas Madlener ([PR#94](https://github.com/key4hep/k4simdelphes/pull/94))
  - Fix the long(er) standing issue of having jet constituents without matching reconstructed particle.

# v00-02-01

* 2022-10-26 Thomas Madlener ([PR#91](https://github.com/key4hep/k4SimDelphes/pull/91))
  - Add a test to make sure that the sum of the jet constituents 4 momenta and the Jet 4 momenta agree

# v00-02

* 2022-09-21 Thomas Madlener ([PR#85](https://github.com/key4hep/k4SimDelphes/pull/85))
  - Make the standalone executables use the `podio::Frame`

# v00-01-09

* 2022-06-02 clementhelsens ([PR#83](https://github.com/key4hep/k4SimDelphes/pull/83))
  - Add dNdx to Tracks properly as `dxQuantity` (added to EDM4hep in [EDM4hep#137](https://github.com/key4hep/EDM4hep/pull/137))

* 2022-06-01 clementhelsens ([PR#88](https://github.com/key4hep/k4SimDelphes/pull/88))
  - Add RPATH to Cmake such that the librairies are properly linked in ```install/bin```

* 2022-06-01 clementhelsens ([PR#87](https://github.com/key4hep/k4SimDelphes/pull/87))
  - Adding at the end of the job a print of the cross section calculated by Pythia

* 2022-06-01 Thomas Madlener ([PR#86](https://github.com/key4hep/k4SimDelphes/pull/86))
  - Fix segmentation fault when passing no arguments to standalone executables by explicitly checking whether arguments are passed

* 2022-05-30 Thomas Madlener ([PR#84](https://github.com/key4hep/k4SimDelphes/pull/84))
  - Store `podio::UserDataCollection` in the same map as other collections internally, so that all collections can be stored through the same interface.
    - By doing so making the new outputs also available to the framework use case.
  - Switch the internal collection map from using `std::string_view` keys to `std::string` keys to make it easier to define collection names that are derived from Delphes branch names.

* 2022-05-23 Thomas Madlener ([PR#82](https://github.com/key4hep/k4SimDelphes/pull/82))
  - Make sure clang-format run in CI works with older diffutils

* 2022-05-23 clementhelsens ([PR#81](https://github.com/key4hep/k4SimDelphes/pull/81))
  - This adds tracker hit information and tracker radiusas part of the EDM
  - And also adds path lenght and dndx as user data

# v00-01-08

* 2022-03-17 Thomas Madlener ([PR#75](https://github.com/key4hep/k4SimDelphes/pull/75))
  - Add `doc/ReleaseNotes.md` to be able to run the ilcsoft tagging scripts on k4SimDelphes for making new releases.

* 2022-03-03 Thomas Madlener ([PR#74](https://github.com/key4hep/k4SimDelphes/pull/74))
  - Add clang-format configuration and apply it
  - Add basic pre-commit configuration and run it in CI workflow

* 2022-03-03 Thomas Madlener ([PR#71](https://github.com/key4hep/k4SimDelphes/pull/71))
  - Start to populate a readme for the central Key4hep documentation page with an example of how to run a standalone executable
  - Add basic doctest setup that runs on the Key4hep nightlies to make sure the commands in the Readme are correct.

* 2022-03-02 Thomas Madlener ([PR#72](https://github.com/key4hep/k4SimDelphes/pull/72))
  - Make the standalone executables print the usage message to stdout and return 0 if the user specifically asked for this message via the `--help` or `-h` argument

* 2022-02-28 Thomas Madlener ([PR#69](https://github.com/key4hep/k4SimDelphes/pull/69))
  - Create a top level README with a basic introduction and refer to the detailed output configuration README from there.
  - Move the output configuration README to the `doc` folder.
  - Add CI status badge to README

* 2022-01-26 Thomas Madlener ([PR#66](https://github.com/key4hep/k4SimDelphes/pull/66))
  - Make the necessary adaptions for the changed class names that were introduced with AIDASoft/podio#205 and key4hep/EDM4hep#132

# Version 0.0.0

* Add versioning
