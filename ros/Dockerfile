# This is an auto generated Dockerfile for ros:desktop-full
# generated from docker_images/create_ros_image.Dockerfile.em
ARG BASE_IMAGE=osrf/ros:noetic-desktop-focal
#################################
#   Librealsense Builder Stage  #
#################################
FROM $BASE_IMAGE as librealsense-builder

# To avoid waiting for input during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Builder dependencies installation
RUN apt-get update \
    && apt-get install -qq -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    libssl-dev \
    libusb-1.0-0-dev \
    pkg-config \
    libgtk-3-dev \
    libglfw3-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \    
    curl \
    python3 \
    python3-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

## add python path
ENV PYTHON_EXECUTABLE=/usr/bin/python3
# Download sources
WORKDIR /usr/src
RUN git clone https://github.com/IntelRealSense/librealsense.git

# # Build and install
RUN cd /usr/src/librealsense \
    && mkdir build && cd build \
    && cmake \
    -DCMAKE_C_FLAGS_RELEASE="${CMAKE_C_FLAGS_RELEASE} -s" \
    -DCMAKE_CXX_FLAGS_RELEASE="${CMAKE_CXX_FLAGS_RELEASE} -s" \
    -DCMAKE_INSTALL_PREFIX=/opt/librealsense \    
    # -DBUILD_GRAPHICAL_EXAMPLES=OFF \
    -DBUILD_PYTHON_BINDINGS:bool=true \
    -DCMAKE_BUILD_TYPE=Release \
    -DPYTHON_INCLUDE_DIR=$(python3 -c "import sysconfig; print(sysconfig.get_path('include'))")  \
    -DPYTHON_LIBRARY=$(python3 -c "import sysconfig; print(sysconfig.get_config_var('LIBDIR'))") \
    -DPYTHON_EXECUTABLE=/usr/bin/python3 ../ \
    && make -j$(($(nproc)-1)) all \
    && make install 


FROM osrf/ros:noetic-desktop-focal as baseimage
ENV DEBIAN_FRONTEND=noninteractive
# install ros packages
RUN apt-get update && apt-get install -y --no-install-recommends \
    ros-noetic-desktop-full=1.5.0-1* \
    ros-noetic-dynamic-reconfigure \
    iputils-ping net-tools \
    apt-transport-https \
    software-properties-common \
    && rm -rf /var/lib/apt/lists/*
# apt-key adv --keyserver keyserver.ubuntu.com --recv-key F6E65AC044F831AC80A06380C8B3A55A6F3EFCDE || sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-key F6E65AC044F831AC80A06380C8B3A55A6F3EFCD \

# realsense

# # Builder dependencies installation
# RUN apt-get update \
#     && apt-get install -qq -y --no-install-recommends \
#     build-essential \
#     cmake \
#     git \
#     libssl-dev \
#     libusb-1.0-0-dev \
#     pkg-config \
#     libgtk-3-dev \
#     libglfw3-dev \
#     libgl1-mesa-dev \
#     libglu1-mesa-dev \  
#     libudev-dev \  
#     udev \
#     curl \
#     python3 \
#     python3-dev \
#     ca-certificates \
#     v4l-utils \
#     && rm -rf /var/lib/apt/lists/*

# ## add python path
# ENV PYTHON_EXECUTABLE=/usr/bin/python3
# # Download sources
# WORKDIR /usr/src
# RUN git clone https://github.com/IntelRealSense/librealsense.git

# # Build and install
# RUN cd /usr/src/librealsense \
#     && mkdir build && cd build \
#     && cmake \
#     -DCMAKE_C_FLAGS_RELEASE="${CMAKE_C_FLAGS_RELEASE} -s" \
#     -DCMAKE_CXX_FLAGS_RELEASE="${CMAKE_CXX_FLAGS_RELEASE} -s" \
#     -DCMAKE_INSTALL_PREFIX=/opt/librealsense \    
#     # -DBUILD_GRAPHICAL_EXAMPLES=OFF \
#     -DBUILD_PYTHON_BINDINGS:bool=true \
#     -DCMAKE_BUILD_TYPE=Release \
#     -DPYTHON_INCLUDE_DIR=$(python3 -c "import sysconfig; print(sysconfig.get_path('include'))")  \
#     -DPYTHON_LIBRARY=$(python3 -c "import sysconfig; print(sysconfig.get_config_var('LIBDIR'))") \
#     -DPYTHON_EXECUTABLE=/usr/bin/python3 ../ \
#     && make -j$(($(nproc)-1)) all \
#     && make install 
RUN apt-get update \
    && apt-get install -y --no-install-recommends \	
    libusb-1.0-0 \
    udev \
    apt-transport-https \
    ca-certificates \
    curl \
    software-properties-common \
    && rm -rf /var/lib/apt/lists/*


RUN mkdir -p /etc/udev/rules.d/ 
COPY --from=librealsense-builder /opt/librealsense /usr/local/
COPY --from=librealsense-builder /usr/lib/python3/dist-packages/pyrealsense2 /usr/lib/python3/dist-packages/pyrealsense2
COPY --from=librealsense-builder /usr/src/librealsense/config/99-realsense-libusb.rules /etc/udev/rules.d/
COPY --from=librealsense-builder /usr/src/librealsense/config/99-realsense-d4xx-mipi-dfu.rules /etc/udev/rules.d/
ENV PYTHONPATH=$PYTHONPATH:/usr/local/


RUN apt-get update \
    && apt-get install -y --no-install-recommends \	
    ros-$ROS_DISTRO-realsense2-camera \
    ros-$ROS_DISTRO-realsense2-description \
    && rm -rf /var/lib/apt/lists/*
## LIMX config
# ENV ROS_IP=10.192.1.218
# ENV ROS_MASTER_URI=http://10.192.1.2:11311
