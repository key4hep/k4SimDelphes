#!/bin/bash
export DELPHES_DIR=/home/ilc/Delphes-3.5.0
export LCIO=/home/ilc/LCIO
export PODIO=/home/ilc/podio/install
export EDM4HEP=/home/ilc/EDM4hep/install
export K4SIM=/home/ilc/k4SimDelphes/install
export LD_LIBRARY_PATH=:/home/ilc/LCIO/lib:/home/ilc/Delphes-3.5.0:$PODIO/lib:$EDM4HEP/lib:$K4SIM/lib
export PATH=/opt/conda/envs/root_env/bin:/opt/conda/condabin:/opt/conda/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/home/ilc/delphes2lcio/build/:$K4SIM/bin/
export PYTHONPATH=/home/ilc/LCIO/src/python:/home/ilc/podio/install/python
cd /home/ilc/LCIO; source setup.sh; cd ..
