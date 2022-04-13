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
