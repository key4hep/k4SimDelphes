# k4SimDelphes

This repository holds the code for converting the
[Delphes](https://cp3.irmp.ucl.ac.be/projects/delphes) EDM to the
[EDM4hep](https://github.com/key4hep/EDM4hep) format. It offers standalone
executables as well as integration into the [Key4hep
framework](https://github.com/key4hep) via Gaudi.

`k4SimDelphes` allows some configuration of its output, more information about
this configuration can be found [in this doc](doc/output_config.md)

## Build status
[![linux](https://github.com/key4hep/k4SimDelphes/actions/workflows/test.yml/badge.svg)](https://github.com/key4hep/k4SimDelphes/actions/workflows/test.yml)

## Dependencies
Required:
- [`Delphes`](https://github.com/delphes/delphes) >= 3.5.0
- [`EDM4hep`](https://github.com/key4hep/edm4hep)

Required for framework integration:
- [`Gaudi`](https://gitlab.cern.ch/gaudi/Gaudi) >= 36.0
- [`k4FWCore`](https://github.com/key4hep/k4FWCore)

Optional for standalone executables:
- [`Pythia8`](https://pythia.org/)
- [`EvtGen`](https://evtgen.hepforge.org/) >= 02-02-00

The [Spack recipe for k4SimDelphes](https://github.com/key4hep/key4hep-spack/blob/release/packages/k4simdelphes/package.py) has more detailed information on build requirements  and conflicts.

## Build and install
The easiest way to build and install `k4SimDelphes` is to use an existing
Key4hep installation that brings all the required dependencies along. From there
the steps follow the usual chain of events: cloning the repository, running
cmake and then building and installing

``` bash
source /cvmfs/sw-nightlies.hsf.org/key4hep/setup.sh
git clone https://github.com/k4SimDelphes
cd k4SimDelphes
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=../install
make install
```

To use this newly installed version (instead of the one that comes with the
Key4hep installation) it is necessary to alter the environment slightly

``` bash
cd ../install
export PATH=$(pwd)/bin:${PATH}
export LD_LIBRARY_PATH=$(pwd)/lib64:${LD_LIBRARY_PATH}
```

If you are not on a RedHat based system you might have to change `lib64` to
`lib` in the last command above.

## Testing
Using the `BUILD_TESTING=ON` (default) enables and builds some additional tests
for `k4SimDelphes`. After the build has completed these can be run via

``` bash
# list all available tests
ctest -N
# Run all tests
ctest
# Run a specific test (matching on the name) with verbose output
ctest -R PythiaConverter_ee_Z_bbbar --verbose
```
