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
