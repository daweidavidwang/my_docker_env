FROM ubuntu:bionic

RUN apt-get update
RUN apt-get install -y software-properties-common apt-utils python3.8 python3.8-distutils git wget
RUN ln -s /usr/bin/python3.8 /usr/bin/python &&\
    wget https://bootstrap.pypa.io/get-pip.py &&\
    python3.8 get-pip.py &&\
    pip install torch 
    
RUN cd home && \
    git clone https://github.com/daweidavidwang/mixed_autonomy_intersections.git &&\
    cd mixed_autonomy_intersections &&\
    bash setup/setup_sumo_docker.sh &&\
    pip install -r requirements.txt 


