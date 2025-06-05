

FROM wangdawei1996/jetson_ros_noetic:tracker as baseimage
ENV DEBIAN_FRONTEND=noninteractive

WORKDIR /code/limx_ws
RUN apt-get update \
    && apt-get install -qq -y --no-install-recommends \
    ros-noetic-rosbridge-server libusb-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*
