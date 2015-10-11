Compiling Jitana on Linux
=========================

Updated: 2015-01-22

## Ubuntu 14.04 LTS

### Installing the Dependencies

    sudo apt-get update
    sudo apt-get install build-essential cmake

OpenGL:

    sudo apt-get install freeglut3-dev
    sudo apt-get install libxmu-dev libxi-dev

Boost:

    sudo apt-get install libboost-all-dev

g++-4.9 for the the partial C++14 support:

    sudo add-apt-repository ppa:ubuntu-toolchain-r/test
    sudo apt-get update
    sudo apt-get install g++-4.9

### Installing ADB

    sudo add-apt-repository ppa:phablet-team/tools
    sudo apt-get update
    sudo apt-get install android-tools-adb

### Compiling Jitana

    CXX=/usr/bin/g++-4.9 cmake ../jitana
    make -j8
