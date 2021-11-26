
cd /root/gazebo_source/ign-cmake
mkdir build
cd build
cmake ..
make -j
make install

cd /root/gazebo_source/ign-math
mkdir build
cd build
cmake ..
make -j
make install 

apt-get install build-essential \
                     cmake \
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
             libprotobuf-dev

cd /root/gazebo_source/ign-
mkdir build
cd build
cmake ..
make -j
make install 