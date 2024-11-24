# generated from docker_images/create_ros_image.Dockerfile.em
ARG BASE_IMAGE=wangdawei1996/ros2_nav2:beta

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
    udev \
    apt-transport-https \
    software-properties-common \
    ros-rolling-rviz* \
    libudev-dev \
    v4l-utils \
    && rm -rf /var/lib/apt/lists/*

## add python path
ENV PYTHON_EXECUTABLE=/usr/bin/python3
# Download sources
WORKDIR /usr/src
RUN git config --global http.version HTTP/1.1 \
    && git clone https://github.com/IntelRealSense/librealsense.git

# # Build and install
RUN cd /usr/src/librealsense \
    && mkdir build && cd build \
    && cmake \
    -DCMAKE_C_FLAGS_RELEASE="${CMAKE_C_FLAGS_RELEASE} -s" \
    -DCMAKE_CXX_FLAGS_RELEASE="${CMAKE_CXX_FLAGS_RELEASE} -s" \
    -DCMAKE_INSTALL_PREFIX=/usr/local \    
    -DBUILD_PYTHON_BINDINGS:bool=true \
    -DCMAKE_BUILD_TYPE=Release \
    -DPYTHON_INCLUDE_DIR=$(python3 -c "import sysconfig; print(sysconfig.get_path('include'))")  \
    -DPYTHON_LIBRARY=$(python3 -c "import sysconfig; print(sysconfig.get_config_var('LIBDIR'))") \
    -DPYTHON_EXECUTABLE=/usr/bin/python3 ../ \
    && make -j$(($(nproc)-1)) all \
    && make install 



# FROM osrf/ros:rolling-desktop-full as baseimage
# ENV DEBIAN_FRONTEND=noninteractive


# ENV PYTHON_EXECUTABLE=/usr/bin/python3
# RUN mkdir -p /etc/udev/rules.d/ 
# COPY --from=librealsense-builder /opt/librealsense /usr/local/
# # COPY --from=librealsense-builder /usr/lib/python3/dist-packages/pyrealsense2 /usr/lib/python3/dist-packages/pyrealsense2
# COPY --from=librealsense-builder /usr/src/librealsense/config/99-realsense-libusb.rules /etc/udev/rules.d/
# COPY --from=librealsense-builder /usr/src/librealsense/config/99-realsense-d4xx-mipi-dfu.rules /etc/udev/rules.d/
# ENV PYTHONPATH=$PYTHONPATH:/usr/local/

# ENV PYTHON_EXECUTABLE=/usr/bin/python3
# RUN mkdir -p /etc/udev/rules.d/ 
# COPY /opt/librealsense /usr/local/
# COPY /usr/lib/python3/dist-packages/pyrealsense2 /usr/lib/python3/dist-packages/pyrealsense2
# COPY /usr/src/librealsense/config/99-realsense-libusb.rules /etc/udev/rules.d/
# COPY /usr/src/librealsense/config/99-realsense-d4xx-mipi-dfu.rules /etc/udev/rules.d/
# ENV PYTHONPATH=$PYTHONPATH:/usr/local/

# RUN apt-get update \
#     && apt-get install -y --no-install-recommends \	
#     # ros-$ROS_DISTRO-realsense2-camera \
#     # ros-$ROS_DISTRO-realsense2-description \
#     ros-rolling-navigation2 \ 
#     ros-rolling-nav2-bringup \
#     && rm -rf /var/lib/apt/lists/*