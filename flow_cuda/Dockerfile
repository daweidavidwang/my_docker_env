FROM tensorflow/tensorflow:1.15.2-gpu-py3-jupyter

RUN apt-get update && apt-get install -y \
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
        x11proto-gl-dev

RUN git clone https://github.com/daweidavidwang/flow.git &&\
    cd flow &&\
    pip install -e .

RUN echo "Installing system dependencies for SUMO" &&\
    apt-get update &&\
    apt-get install -y cmake swig libgtest-dev python-pygame python-scipy \
    autoconf libtool pkg-config libgdal-dev libxerces-c-dev\
    libproj-dev libfox-1.6-dev libxml2-dev libxslt1-dev\
    build-essential curl unzip flex bison wget &&\
    pip install cmake cython gputil psutil

RUN echo "Installing sumo binaries" &&\
    mkdir -p $HOME/sumo_binaries/bin &&\
    pushd $HOME/sumo_binaries/bin &&\
    wget https://akreidieh.s3.amazonaws.com/sumo/flow-0.4.0/binaries-ubuntu1804.tar.xz &&\
    tar -xf binaries-ubuntu1804.tar.xz &&\
    rm binaries-ubuntu1804.tar.xz &&\
    chmod +x * &&\
    popd &&\
    echo 'export PATH="$HOME/sumo_binaries/bin:$PATH"' >> ~/.bashrc &&\
    echo 'export SUMO_HOME="$HOME/sumo_binaries/bin"' >> ~/.bashrc

# COPY --from=nvidia/opengl:1.0-glvnd-runtime-ubuntu18.04 \
#   /usr/lib/x86_64-linux-gnu/* \
#   /usr/lib/x86_64-linux-gnu/

# COPY --from=nvidia/opengl:1.0-glvnd-runtime-ubuntu18.04 \
#   /usr/lib/x86_64-linux-gnu/libX* \
#   /usr/lib/x86_64-linux-gnu/

# COPY --from=nvidia/opengl:1.0-glvnd-runtime-ubuntu18.04 \
#   /usr/lib/x86_64-linux-gnu/libEGL* \
#   /usr/lib/x86_64-linux-gnu/


# COPY --from=nvidia/opengl:1.0-glvnd-runtime-ubuntu18.04 \
#   /usr/share/glvnd/egl_vendor.d/10_nvidia.json \
#   /usr/share/glvnd/egl_vendor.d/10_nvidia.json

# RUN echo '/usr/lib/x86_64-linux-gnu' >> /etc/ld.so.conf.d/glvnd.conf && \
#     ldconfig && \
#     echo '/usr/$LIB/libGL.so.1' >> /etc/ld.so.preload && \
#     echo '/usr/$LIB/libEGL.so.1' >> /etc/ld.so.preload

# nvidia-container-runtime
# ENV NVIDIA_VISIBLE_DEVICES \
#     ${NVIDIA_VISIBLE_DEVICES:-all}
# ENV NVIDIA_DRIVER_CAPABILITIES \
#     ${NVIDIA_DRIVER_CAPABILITIES:+$NVIDIA_DRIVER_CAPABILITIES,}all
# ENV NVIDIA_DRIVER_CAPABILITIES all

# RUN apt-key adv --keyserver keyserver.ubuntu.com --recv-keys A6DCF7707EBC211F &&\
#     apt-add-repository "deb http://ppa.launchpad.net/ubuntu-mozilla-security/ppa/ubuntu focal main" &&\
#     apt-get update &&\
#     apt-get -f install &&\
#     apt-get install -y firefox

# ADD installation /root/installation 

# RUN cp -r /root/installation/install /root/install
# RUN cp -r /root/installation/app /root/app

# RUN . /root/install/install_custom_fronts.sh
# RUN . /root/install/libnss_wrapper.sh
# RUN . /root/install/tools.sh
# RUN . /root/app/fireforx.sh
# RUN . /root/app/sumo.sh
# RUN . /root/app/flow.sh
# RUN . /root/install/set_user_permission.sh