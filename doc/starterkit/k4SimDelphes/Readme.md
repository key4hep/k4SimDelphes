# Running Delphes fast simulation with EDM4hep output

The [`k4SimDelphes`](https://github.com/key4hep/k4SimDelphes) package provides utilities to convert output from the [Delphes fast simulation framework](https://cp3.irmp.ucl.ac.be/projects/delphes) into the [EDM4hep](https://github.com/key4hep/EDM4hep) format. It offers standalone executables, similar to the ones Delphes offers, as well as integration into the Key4hep framework. Here we will provide examples of how to run the standalone executables as well as the usage in the Key4hep framework.

## Setup and prerequisites
The following examples assume that you have access to an existing installation of the Key4hep software stack. The easiest way to achieve this to use an existing installation on `cvmfs`
```bash
source /cvmfs/sw.hsf.org/key4hep/setup.sh
```
Alternatively it is possible to build the complete stack via `spack`, see the [instructions](https://key4hep.github.io/key4hep-doc/spack-build-instructions-for-librarians/README.html) for how to do this.

In order to run the examples below it is necessary to get some inputs that are used for the generation of the physics events. In the following we will be using pythia generator files that are also used by the FCCee. In particular we will be generating the following process

e+e- -> ZH -> mu+mu- X (i.e. the Z decays into a pair of oppositely charged muons and the H to anything)

To start we download the pythia cards for this process
```bash
wget https://raw.githubusercontent.com/HEP-FCC/FCC-config/spring2021/FCCee/Generator/Pythia8/p8_noBES_ee_ZH_ecm240.cmd
```

All the other resources are available from within a Key4hep release.

## Standalone executables

For this example we will be using the `DelphesPythia8_EDM4HEP` standalone executable, which is essentially the same as the `DelphesPythia8` executable that is provided by Delphes. Contrary to the Delphes executable the one provided by k4SimDelphes will produce output in the EDM4hep format.

To run the generation and fast detector simulation in one go run
```bash
DelphesPythia8_EDM4HEP ${DELPHES_DIR}/cards/delphes_card_IDEA.tcl \
                       ${K4SIMDELPHES}/edm4hep_output_config.tcl \
                       p8_noBES_ee_ZH_ecm240.cmd \
                       delphes_events_edm4hep.root
```

The arguments in this case are (in the order they are passed)
- The delphes card that describes the (parameterized) detector. In this case we use one that is shipped with the Delphes installation
- The output configuration for the Delphes to EDM4hep converter. Here we use the defaults that are provided by `k4SimDelphes`, which should cover the majority of use cases. It is however possible to [configure these to your needs](https://github.com/key4hep/k4SimDelphes/blob/main/doc/output_config.md) if necessary.
- The pythia card that describes the physics process [as described above](#setup-and-prerequisites)
- The name of the output file that will be created and that will contain the generated and simulated events in EDM4hep format

### Other standalone executables
k4SimDelphes provides other standalone executables that can read different inputs. They offer the same functionality as the ones available from Delphes:
- `DelphesSTDHEP_EDM4HEP` - for reading STDHEP inputs
- `DelphesROOT_EDM4HEP` - for reading ROOT files in the Delphes format
- `DelphesPythia8_EDM4HEP` - for running Pythia8 as part of the simulation

For all executables it is possible to at least get the order of the input arguments via `--help` or `-h`, e.g.
```bash
DelphesSTDHEP_EDM4HEP --help
```
will print
```console
Usage: DelphesHepMC config_file output_config_file output_file [input_file(s)]
config_file - configuration file in Tcl format,
output_config_file - configuration file steering the content of the edm4hep output in Tcl format,
output_file - output file in ROOT format,
input_file(s) - input file(s) in STDHEP format,
with no input_file, or when input_file is -, read standard input.

```

## Usage in the Key4hep framework

- [ ] TODO
