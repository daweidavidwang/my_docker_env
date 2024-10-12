FROM gaetanlandreau/3d-gaussian-splatting:latest
##install conda
RUN apt-get update \
    && apt-get install -y build-essential \
    && apt-get install -y wget \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Install miniconda
ENV CONDA_DIR /opt/conda
RUN wget --quiet https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh -O ~/miniconda.sh && \
    /bin/bash ~/miniconda.sh -b -p /opt/conda

# Put conda in path so we can use conda activate
ENV PATH=$CONDA_DIR/bin:$PATH



## REVIEWER
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -yq \
        git \ 
        libglew-dev libassimp-dev libboost-all-dev libgtk-3-dev libopencv-dev libglfw3-dev libavdevice-dev libavcodec-dev libeigen3-dev libxxf86vm-dev libembree-dev

COPY environment.yml /environment.yml

RUN  conda env create --file /environment.yml

SHELL ["conda", "run", "-n", "exavatar", "/bin/bash", "-c"]

RUN cd / && \
    # conda activate exavatar &&\
    git clone -b dev https://github.com/graphdeco-inria/gaussian-splatting.git --recursive && \
    cd /gaussian-splatting &&\
    pip uninstall diff_gaussian_rasterization &&\
    pip install submodules/diff-gaussian-rasterization &&\
    cd /gaussian-splatting/SIBR_viewers && \
    cmake -Bbuild . -DCMAKE_BUILD_TYPE=Release &&\
    cmake --build build -j24 --target install

##HUMAN BODY

RUN cd / && \   
    # conda activate exavatar &&\
    git clone https://github.com/mks0601/ExAvatar_RELEASE.git --recursive && \
    cd ExAvatar_RELEASE/fitting/tools &&\
    git clone https://github.com/yfeng95/DECA.git &&\
    git clone https://github.com/mks0601/Hand4Whole_RELEASE.git &&\
    git clone https://github.com/open-mmlab/mmpose.git &&\
    git clone https://github.com/facebookresearch/segment-anything.git &&\
    git clone https://github.com/DepthAnything/Depth-Anything-V2.git &&\
    # cd /ExAvatar_RELEASE/fitting/tools/DECA &&\
    # pip install -r requirements.txt &&\
    cd /ExAvatar_RELEASE/fitting/tools/mmpose &&\
    pip install -U openmim &&\
    mim install mmengine &&\
    mim install "mmcv>=2.0.1" &&\
    mim install "mmdet>=3.1.0" &&\
    pip install -r requirements.txt &&\
    pip install -v -e . &&\
    mim install "mmpose>=1.1.0" &&\
    cd /ExAvatar_RELEASE/fitting/tools/segment-anything &&\
    pip install -e . &&\
    pip install opencv-python pycocotools matplotlib onnxruntime onnx &&\
    cd /ExAvatar_RELEASE/fitting/tools/Depth-Anything-V2 &&\
    pip install -r requirements.txt



    
