FROM rootproject/root:6.24.00-conda

RUN apt-get update && DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC apt-get -y install tzdata 
RUN apt-get update  && apt-get -y install cmake vim git

ENV HOME /home/ilc
WORKDIR ${HOME}

RUN conda create -n root_env root -c conda-forge 

SHELL ["conda", "run", "-n", "root_env", "/bin/bash", "-c"]


RUN git clone https://github.com/iLCSoft/LCIO.git \
   && cd LCIO \
   && git checkout v02-16-01 \ 
   && mkdir build \
   && cd build \
   && cmake -DBUILD_ROOTDICT=ON -D CMAKE_CXX_STANDARD=17 ..  \
   && make -j 8 install && cd .. ; rm -rf ./build


WORKDIR ${HOME}


RUN git clone https://github.com/delphes/delphes.git \
    && mv delphes Delphes-3.5.0 \
    && cd Delphes-3.5.0 \
    && make


## Installing with tarball is not suggested 
#RUN wget http://cp3.irmp.ucl.ac.be/downloads/Delphes-3.5.0.tar.gz \
#  && tar -zxf Delphes-3.5.0.tar.gz \
#  && cd Delphes-3.5.0 \
#  && make 



ENV DELPHES_DIR /home/ilc/Delphes-3.5.0
ENV LCIO /home/ilc/LCIO


RUN cp $LCIO/examples/cpp/delphes2lcio -r . \
  && source ${ROOTSYS}/bin/thisroot.sh  \
  && cd delphes2lcio \
  && mkdir build \
  && cd build \
  && cmake -D LCIO_DIR=$LCIO .. \
  && make -j 4 install 


COPY requirements.txt requirements.txt 
RUN pip install --upgrade pip \
    && pip install --upgrade --no-cache-dir -r requirements.txt  \
    && rm requirements.txt


SHELL ["conda", "run", "-n", "root_env", "/bin/bash", "-c"]

### PODIO 
RUN git clone https://github.com/AIDASoft/podio.git \
    && cd podio && source ./init.sh && source ./env.sh && mkdir build && mkdir install \
    && cd build && cmake -DCMAKE_INSTALL_PREFIX=../install -DBUILD_TESTING=OFF .. \
    && make -j 4 install && cd .. ; rm -rf ./build  

### EDM4HEP


WORKDIR ${HOME}

SHELL ["conda", "run", "-n", "root_env", "/bin/bash", "-c"]

RUN git clone https://github.com/key4hep/EDM4hep; cd EDM4hep; mkdir build; mkdir install; cd build  \
          && podio_DIR=/home/ilc/podio/install cmake -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX=../install .. \
          && make -j 4 install && cd .. ; rm -rf ./build

### k4SIM
WORKDIR ${HOME}
SHELL ["conda", "run", "-n", "root_env", "/bin/bash", "-c"]

RUN git clone https://github.com/key4hep/k4SimDelphes.git; cd k4SimDelphes; mkdir build; cd build \
          && podio_DIR=/home/ilc/podio/install EDM4HEP_DIR=/home/ilc/EDM4hep/install cmake \
          -DBUILD_TESTING=OFF -DBUILD_PYTHIA_READER=OFF \ 
          -DBUILD_EVTGEN_READER=OFF -DBUILD_HEPMC_READER=OFF -DBUILD_FRAMEWORK=OFF -DCMAKE_INSTALL_PREFIX=../install .. \
          && make -j 4 install && cd .. ; rm -rf ./build


COPY init_env.sh ${HOME}