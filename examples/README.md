# Testing Delphes -> EDM4hep


## Setting up tests

The following environment variables are expected to be set for the following to
work:
- `DELPHES_DIR`: points to the Delphes install directory
- `EDM4HEP_DIR`: points to the EDM4hep install directory
- `ROOT_INCLUDE_PATH`: contains the podio, EDM4hep and Delphes include
  directories (e.g. via `$EDM4HEP_DIR/include`)
- `LD_LIBRARY_PATH`: contains the podio, EDM4hep and Delphes libs (and dicts)
  (e.g. via `$EDM4HEP_DIR/lib`)


### Setting up a test folder

To easily run some tests, a folder is set up and some of the Delphes examples
are copied such that they can be easily changed without risking messing up the
Delphes installation somehow (however unlikely).

    mkdir test_delphes && cd test_delphes
    mkdir cards inputs
    cp ${DELPHES_DIR}/cards/delphes_card_{ILD,CMS}.tcl cards
    cp ${DELPHES_DIR}/examples/Pythia8/configNoLHE.cmnd inputs

To retrieve comparable outputs the random seeds for the Delphes card and the
Pythia8 configuration file are fixed. Additionally a restriction to 1000 event
is put in place. This means that

    set MaxEvents 1000
    set RandomSeed 123

is added at the top of `cards/delphes_card_CMS.tcl` (resp.
`cards/delphes_card_ILD.tcl`) and

    Random:setSeed = on
    Random:seed = 12345

is added in the first section (`1) Settings used in the main program`) of
`inputs/configNoLHE.cmnd`

Finally also copy the `edm4hep` output configuration to the `cards` directory
(not strictly necessary but in this case makes for cleaner commands in the
examples below)

    cp path/to/EDM4hep/plugins/delphes/edm4hep_output_config.tcl cards

## Generating Delphes output

A Delhpes output file is generated to have a baseline comparison for the edm4hep
output. The output is generated using the following command

    ${DELPHES_DIR}/bin/DelphesPythia8 \
        cards/delphes_card_CMS.tcl \
        inputs/configNoLHE.cmnd \
        delphes_test_output.root


## Generating edm4hep output

The corresponding edm4hep output is produced using the following command

    ${EDM4HEP_DIR}/bin/DelphesPythia8_EDM4HEP \
        cards/delphes_card_CMS.tcl \
        cards/edm4hep_output_config.tcl \
        inputs/configNoLHE.cmnd \
        edm4hep_test_output.root


## Generating comparison histograms

Two example macros that produce the same histograms once from a Delphes output
file and once from a edm4hep output file are in
`EDM4hep/plugins/delphes/examples`. Both take as input parameters the
corresponding filename and produce a new `.root` file with some histograms that
should ideally be equivalent (but are currently not).

    root -q 'path/to/EDM4hep/plugins/delphes/examples/read_delphes.C("delphes_test_output.root")'
    root -q 'path/to/EDM4hep/plugins/delphes/examples/read_edm4hep.C("edm4hep_test_output.root")' 

After this the files `histograms_delphes.root` and `histograms_edm4hep.root`
should be present and filled with some histograms that can be compared. The
differences in the Jet energy and momentum are caused by a potential double
counting of tracks and towers (see [known issues](../README.md#known-issues)).


# Another example: Invariant Mass of Isolated Muons in ILD 

First of all, we need to download a `stdhep` file [`here`](https://syncandshare.desy.de/index.php/s/Kx7ygmgejpmnSwE) and put it into `data` folder.

Now, we are ready to launch our container;

```console
engin@local:$ pwd
 ~/k4SimDelphes/examples 
```
```bash
docker run -it -p 8888:8888 -v $PWD:/home/ilc/wdir ilcsoft/k4simdelphes:latest bash
## you're inside the container now
conda init bash
```


```console
no change     /opt/conda/condabin/conda
...
...
no change     /opt/conda/etc/profile.d/conda.csh
modified      /home/ilc/.bashrc
...
```
```bash
source .bashrc 
conda activate root_env
source init_env.sh 
```
Ready to generate output file
```bash
DelphesSTDHEP_EDM4HEP $DELPHES_DIR/cards/delphes_card_ILD.tcl \
         ./edm4hep_output_config.tcl \ 
          edm4hep_output.root \
         ./data/E250-TDR_ws.P4f_zzorww_l.Gwhizard-1_95.eL.pR.I106721.003.stdhep

```
```console
...
** Reading ./data/E250-TDR_ws.P4f_zzorww_l.Gwhizard-1_95.eL.pR.I106721.003.stdhep
...
** 179910 events processed
** Exiting ...

```

Now we have output root file.. Let us open jupyter-notebook from our container:
```console
(root_env) root@a19e37608dbf::~/wdir# jupyter notebook --port=8888 --ip=0.0.0.0 --allow-root 
...
 To access the notebook, open this file in a browser:
        file:///home/ilc/.local/share/jupyter/runtime/nbserver-2220-open.html
    Or copy and paste one of these URLs:
        http://c5a7dd737b14:8888/?token=daaea35f4d34fcc7206035d94a677474f6294d1ba95b886e
     or http://127.0.0.1:8888/?token=daaea35f4d34fcc7206035d94a677474f6294d1ba95b886e
...
```

Copy paste `http://127.0.0.1:8888/?token` to your browser. You may have a look at the notebook `edm4hep_IsoM.ipynb`


## Working Group Servers with Singularity via cvmfs

The docker image for this example was also deployed to `unpacked.cern.ch`, where images are unpacked. Then, it is easy for singularity to use this images via cvmfs.

As usual; pull the repo, place the data folder, go to example folder
```console
-bash-4.2$ pwd
/your-working-directory/k4SimDelphes/examples
```

Now ready to go inside the container
```bash
singularity shell -H $PWD --bind $(pwd):/home/ilc/data /cvmfs/unpacked.cern.ch/registry.hub.docker.com/ilcsoft/k4simdelphes:latest bash
conda init bash
source .bashrc
conda activate root_env
source /home/ilc/init_env.sh
cd $home
```
After this stage, commands are the same. Just watch out: You might need to tunnel working groups server's network (i.e cern.ch)







