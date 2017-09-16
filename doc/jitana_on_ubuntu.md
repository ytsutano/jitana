# Compiling Jitana on Linux

## Ubuntu 16.04 LTS

Updated: 2016-09-12

### Installing the Dependencies

    sudo apt-get update
    sudo apt-get install build-essential cmake

Boost:

    sudo apt-get install libboost-all-dev

Graphviz for viewing graphs:

    sudo apt-get install xdot

OpenGL for Jitana-Travis:

    sudo apt-get install freeglut3-dev
    sudo apt-get install libxmu-dev libxi-dev

Doxygen (optional):

    sudo apt-get install doxygen

The default g++ supports C++14.

### Installing ADB

    sudo apt-get install android-tools-adb

### Compiling Jitana

Jitana uses CMake which supports out-of-source build. In this document, the
following directory structure is assumed:

    .
    ├── jitana (source code downloaded)
    ├── dex    (DEX files)
    └── build  (build directory you make)

Make sure you have `jitana` and `dex` directories are in the current directory:

    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ../jitana
    make -j8

## Ubuntu 14.04 LTS (No Recommended)

Updated: 2016-03-15

### Installing the Dependencies

    sudo apt-get update
    sudo apt-get install build-essential cmake

OpenGL:

    sudo apt-get install freeglut3-dev
    sudo apt-get install libxmu-dev libxi-dev

Boost:

    sudo apt-get install libboost-all-dev

g++-4.9 for the the partial C++14 support (any newer version of GCC or Clang works, too):

    sudo add-apt-repository ppa:ubuntu-toolchain-r/test
    sudo apt-get update
    sudo apt-get install g++-4.9

Doxygen (optional):

    sudo apt-get install doxygen

You also need to install:

- [ApkTool](http://ibotpeaches.github.io/Apktool/) (and Java)

### Installing ADB

    sudo add-apt-repository ppa:phablet-team/tools
    sudo apt-get update
    sudo apt-get install android-tools-adb

### Compiling Jitana

Jitana uses CMake which supports out-of-source build. In this document, the
following directory structure is assumed:

    .
    ├── jitana (source code downloaded)
    ├── dex    (DEX files)
    └── build  (build directory you make)

Make sure you have `jitana` and `dex` directories are in the current directory:

    mkdir build
    cd build
    cmake -DCMAKE_CXX_COMPILER=g++-4.9 -DCMAKE_BUILD_TYPE=Release ../jitana
    make -j8
