

FROM wangdawei1996/jetson_ros_noetic:v3 as baseimage
ENV DEBIAN_FRONTEND=noninteractive

WORKDIR /usr/src
RUN git clone https://github.com/nooploop-dev/serial.git

# install ros packages
RUN cd serial \
    && . /opt/ros/noetic/setup.sh \
    &&  make \
    && make install
