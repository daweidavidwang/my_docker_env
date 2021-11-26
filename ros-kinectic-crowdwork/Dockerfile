# syntax=docker/dockerfile:1
FROM ubuntu:16.04
RUN apt-get update && apt-get install -y --no-install-recommends \
        git \
        ca-certificates \
        make \
        automake \
        autoconf \
        libtool \
        pkg-config \
        python \
        libxext-dev \
        libx11-dev \
        mesa-common-dev libgl1-mesa-dev libglu1-mesa-dev \
        x11proto-gl-dev && \
    rm -rf /var/lib/apt/lists/*
COPY --from=nvidia/opengl:1.0-glvnd-runtime-ubuntu16.04 \
  /usr/local/lib/x86_64-linux-gnu \
  /usr/local/lib/x86_64-linux-gnu

COPY --from=nvidia/opengl:1.0-glvnd-runtime-ubuntu16.04 \
  /usr/local/share/glvnd/egl_vendor.d/10_nvidia.json \
  /usr/local/share/glvnd/egl_vendor.d/10_nvidia.json

RUN echo '/usr/local/lib/x86_64-linux-gnu' >> /etc/ld.so.conf.d/glvnd.conf && \
    ldconfig && \
    echo '/usr/local/$LIB/libGL.so.1' >> /etc/ld.so.preload && \
    echo '/usr/local/$LIB/libEGL.so.1' >> /etc/ld.so.preload

# nvidia-container-runtime
ENV NVIDIA_VISIBLE_DEVICES \
    ${NVIDIA_VISIBLE_DEVICES:-all}
ENV NVIDIA_DRIVER_CAPABILITIES \
    ${NVIDIA_DRIVER_CAPABILITIES:+$NVIDIA_DRIVER_CAPABILITIES,}all

RUN . /etc/lsb-release
RUN echo "deb http://mirrors.tuna.tsinghua.edu.cn/ros/ubuntu/ xenial main" > /etc/apt/sources.list.d/ros-latest.list
RUN echo "deb http://packages.osrfoundation.org/gazebo/ubuntu-stable xenial main" > /etc/apt/sources.list.d/gazebo-stable.list
RUN apt-get update
RUN apt-get install -y curl apt-utils wget firefox unzip cmake pkg-config
RUN wget http://packages.osrfoundation.org/gazebo.key -O - | apt-key add -
RUN curl -s https://raw.githubusercontent.com/ros/rosdistro/master/ros.asc | apt-key add -
RUN apt-get update
RUN apt-get install -y ros-kinetic-desktop
RUN echo "source /opt/ros/kinetic/setup.bash" >> ~/.bashrc
RUN apt-get install -y python-rosdep python-rosinstall python-rosinstall-generator python-wstool build-essential \
        libfreeimage-dev \  
         libtinyxml2-dev \
         uuid-dev \
         libgts-dev \
         libavdevice-dev \
         libavformat-dev \
         libavcodec-dev \
         libswscale-dev \
         libavutil-dev \
         libprotoc-dev \
        libprotobuf-dev \
        libboost-system-dev \
        libtinyxml-dev \
        libxml2-utils \
        ruby-dev \
        ruby \
        libprotoc-dev \
        protobuf-compiler \
          libzip-dev \
         libjsoncpp-dev \
         libcurl4-openssl-dev \
         libyaml-dev \
         qt4-default \
         libqwt-dev \
         libtar-dev \
         libtbb-dev \
         python-catkin-tools \
         libzmq3-dev \
         ruby-ronn

RUN rosdep init
RUN rosdep update
RUN mkdir -p /root/.ssh
ADD id_rsa /root/.ssh/id_rsa
RUN chmod 700 /root/.ssh/id_rsa
RUN echo "Host github.com\n\tStrictHostKeyChecking no\n" >> /root/.ssh/config
RUN git config --global user.name "dawei docker"
RUN git config --global user.email dawei@connect.hku.hk
RUN mkdir -p /root/gazebo_source
ADD gazebo_source /root/gazebo_source
RUN cd /root/gazebo_source && \
    git clone https://github.com/ignitionrobotics/ign-cmake && \
    cd /root/gazebo_source/ign-cmake && \
    git checkout ign-cmake0 &&\
    mkdir build && cd build &&\
    cmake .. && \
    make -j12 && \
    make install 
RUN cd /root/gazebo_source && \
    git clone https://github.com/ignitionrobotics/ign-math.git && \
    cd /root/gazebo_source/ign-math && \
    git checkout ignition-math2_2.9.0 &&\
    mkdir build && cd build &&\
    cmake .. && \
    make -j12 && \
    make install 
RUN cd /root/gazebo_source && \
    git clone https://github.com/ignitionrobotics/ign-common && \
    cd /root/gazebo_source/ign-common && \
    git checkout ignition-common_0.1.0 &&\
    mkdir build && cd build &&\
    cmake .. && \
    make -j12 && \
    make install 
RUN cd /root/gazebo_source && \
    git clone https://github.com/ignitionrobotics/sdformat.git && \
    cd /root/gazebo_source/sdformat && \
    git checkout sdf4 &&\
    mkdir build && cd build &&\
    cmake .. && \
    make -j12 && \
    make install 
RUN cd /root/gazebo_source && \
    git clone https://github.com/ignitionrobotics/ign-msgs.git && \
    cd /root/gazebo_source/ign-msgs && \
    git checkout ignition-msgs_0.4.0 &&\
    mkdir build && cd build &&\
    cmake .. && \
    make -j12 && \
    make install
RUN cd /root/gazebo_source && \
    git clone https://github.com/ignitionrobotics/ign-transport.git && \
    cd /root/gazebo_source/ign-transport && \
    git checkout ign-transport2 &&\
    mkdir build && cd build &&\
    cmake .. && \
    make -j12 && \
    make install 
RUN cd /root/gazebo_source/gazebo/build && \
    cmake .. && \
    make -j12 && \
    make install 
RUN apt-get install -y libsdl2-dev \
         libsdl-image1.2-dev \
         libsdl-ttf2.0-dev
RUN mkdir -p /root/menge_ws && \
    cp -r /root/gazebo_source/Menge/ /root/menge_ws/Menge/ &&\
     cd /root/menge_ws/Menge/projects/g++/build && \
    cmake -DCMAKE_BUILD_TYPE=Debug .. &&\
    make -j12 && \
    make install 
RUN cd /root/menge_ws && \
    mkdir src && \
    /bin/bash --login -c "source ~/.bashrc" && \
    . /opt/ros/kinetic/setup.sh && \
    catkin build

RUN apt-get install -y ros-kinetic-controller-manager ros-kinetic-joint-state-controller ros-kinetic-joint-trajectory-controller ros-kinetic-rqt ros-kinetic-rqt-controller-manager \
                    ros-kinetic-rqt-joint-trajectory-controller ros-kinetic-ros-control ros-kinetic-rqt-gui\
                  ros-kinetic-rqt-plot ros-kinetic-rqt-graph ros-kinetic-rqt-rviz ros-kinetic-rqt-tf-tree \
                  ros-kinetic-ros-control ros-kinetic-ros-controllers ros-kinetic-polled-camera ros-kinetic-camera-info-manager ros-kinetic-moveit-msgs\
                  ros-kinetic-kdl-conversions ros-kinetic-kdl-parser ros-kinetic-forward-command-controller ros-kinetic-tf-conversions ros-kinetic-xacro ros-kinetic-joint-state-publisher ros-kinetic-robot-state-publisher &&\
                  ros-kinetic-turtlebot

RUN cp -r /root/gazebo_source/menge_gazebo /root/menge_ws/src/menge_gazebo &&\
     cd /root/menge_ws/src &&\
     git clone https://github.com/ros-simulation/gazebo_ros_pkgs.git &&\
      git clone https://github.com/turtlebot/turtlebot_simulator.git &&\
      git clone https://github.com/srl-freiburg/pedsim_ros.git &&\
     cd /root/menge_ws/src/gazebo_ros_pkgs && git checkout 2.5.13

RUN cd /root/menge_ws &&\
     echo "export CMAKE_PREFIX_PATH=/usr/local/lib/cmake/gazebo/:$CMAKE_PREFIX_PATH" >> ~/.bashrc && \
     /bin/bash --login -c "source ~/.bashrc" && \
    . /opt/ros/kinetic/setup.sh && \
    catkin build
RUN echo "export LD_LIBRARY_PATH=/usr/local/lib/:/opt/ros/kinetic/lib:$LD_LIBRARY_PATH" >> ~/.bashrc 
# CMD ["gazebo"]