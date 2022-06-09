apt-get update
apt-get install -y \
        cmake \
        build-essential \
        swig \
        libgdal-dev \
        libxerces-c-dev \
        libproj-dev \
        libfox-1.6-dev \
        libxml2-dev \
        libxslt1-dev \
        openjdk-8-jdk
cd $HOME
git clone --recursive https://github.com/eclipse/sumo.git
cd sumo
git checkout 2147d155b1
mkdir build/cmake-build
cd build/cmake-build
cmake ../..
make -j8
