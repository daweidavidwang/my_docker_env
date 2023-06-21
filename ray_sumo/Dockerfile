FROM rayproject/ray:latest-gpu

# RUN apt-get update && DEBIAN_FRONTEND="noninteractive" apt-get install -y python3-opencv

RUN pip install gym opencv-python

RUN sudo apt-get update && sudo apt-get install -y \
        git \
        ca-certificates \
        make cmake\
        automake \
        autoconf \
        libtool \
        pkg-config \
        software-properties-common\ 
        libnvidia-gl-530

RUN echo "Installing system dependencies for SUMO" &&\
    sudo add-apt-repository ppa:sumo/stable &&\
    sudo apt-get update &&\
    sudo apt-get install -y swig libgtest-dev python-pygame \
    pkg-config libgdal-dev libxerces-c-dev\
    libproj-dev libfox-1.6-dev libxml2-dev libxslt1-dev\
    build-essential curl unzip flex bison wget sumo sumo-tools sumo-doc &&\
    pip install cmake cython gputil psutil 

RUN pip uninstall -y enum34 && pip install setuptools scipy dm_tree traci \
    pygame lxml joblib tabulate pandas requests \
    Pillow sympy easydict statsmodels networkx lz4 tensorboardX pettingzoo \
    pymunk tensorflow_probability torch torchvision torchaudio higher


RUN echo "export LIBGL_ALWAYS_INDIRECT=1" >> ~/.bashrc 