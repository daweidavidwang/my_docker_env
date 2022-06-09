FROM tensorflow/tensorflow:1.15.2-gpu-py3-jupyter

RUN apt-key del A4B469963BF863CC && apt-key adv --fetch-keys https://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/3bf863cc.pub &&\
        apt-get update && apt-get install -y \
        git \
        ca-certificates \
        make cmake\
        automake \
        autoconf \
        libtool \
        pkg-config \
        libxext-dev \
        libx11-dev \
        mesa-common-dev libgl1-mesa-dev libglu1-mesa-dev \
        software-properties-common \
        x11proto-gl-dev \
        mesa-utils libgl1-mesa-glx \
        libnvidia-gl-510

RUN echo "Installing system dependencies for SUMO" &&\
    add-apt-repository ppa:sumo/stable &&\
    apt-get update &&\
    apt-get install -y cmake swig libgtest-dev python-pygame python-scipy \
    autoconf libtool pkg-config libgdal-dev libxerces-c-dev\
    libproj-dev libfox-1.6-dev libxml2-dev libxslt1-dev\
    build-essential curl unzip flex bison wget sumo sumo-tools sumo-doc &&\
    pip install cmake cython gputil psutil 

RUN pip uninstall -y enum34 && pip install setuptools scipy dm_tree traci pygame lxml joblib tabulate gym pandas opencv-python requests ray==1.10.0 Pillow sympy

RUN echo "export LIBGL_ALWAYS_INDIRECT=1" >> ~/.bashrc 