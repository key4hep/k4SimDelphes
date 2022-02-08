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
   && make -j 8 install 


WORKDIR ${HOME}

## Delphes installation from local (unfortunate) due to TF1.h error in ROOT 6.22
## details : https://cp3.irmp.ucl.ac.be/projects/delphes/ticket/1449

RUN wget http://cp3.irmp.ucl.ac.be/downloads/Delphes-3.5.0.tar.gz \
  && tar -zxf Delphes-3.5.0.tar.gz \
  && cd Delphes-3.5.0 \
  && make 

#COPY Delphes-3.4.2 ${HOME}/Delphes-3.4.2
#RUN cd ${HOME}/Delphes-3.4.2 \
#    && make 

ENV DELPHES_DIR /home/ilc/Delphes-3.5.0
ENV LCIO /home/ilc/LCIO

COPY requirements.txt requirements.txt 
RUN pip install --upgrade pip \
    && pip install --upgrade --no-cache-dir -r requirements.txt  \
    && rm requirements.txt


SHELL ["conda", "run", "-n", "root_env", "/bin/bash", "-c"]

### PODIO 
RUN git clone https://github.com/AIDASoft/podio.git \
    && cd podio && source ./init.sh && source ./env.sh && mkdir build && mkdir install \
    && cd build && cmake -DCMAKE_INSTALL_PREFIX=../install -DBUILD_TESTING=OFF .. \
    && make -j 4 install

### EDM4HEP


WORKDIR ${HOME}

SHELL ["conda", "run", "-n", "root_env", "/bin/bash", "-c"]

RUN git clone https://github.com/key4hep/EDM4hep; cd EDM4hep; mkdir build; mkdir install; cd build  \
          && podio_DIR=/home/ilc/podio/install cmake -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX=../install .. \
          && make -j 4 install

### k4SIM
WORKDIR ${HOME}
SHELL ["conda", "run", "-n", "root_env", "/bin/bash", "-c"]

RUN git clone https://github.com/key4hep/k4SimDelphes.git; cd k4SimDelphes; mkdir build; cd build \
          && podio_DIR=/home/ilc/podio/install EDM4HEP_DIR=/home/ilc/EDM4hep/install cmake \
          -DBUILD_TESTING=OFF -DBUILD_PYTHIA_READER=OFF \ 
          -DBUILD_EVTGEN_READER=OFF -DBUILD_HEPMC_READER=OFF -DBUILD_FRAMEWORK=OFF -DCMAKE_INSTALL_PREFIX=../install .. \
          && make -j 4 install


COPY init_env.sh ${HOME}







