Jitana-IAC
==========

This directory was contributed by Shakthi Bachala for the ISSTA specific
analysis.

Jitana-IAC is not mature yet as it handles only implicit and explicit intents.

We recognize that this implementation still has design/efficiency/cleanness
problems, so it is not officially part of Jitana. We will fix them in the
upcoming commits.

## Files

* Shell Scripts for Launching
    * `tools/jitana-iac/scripts/scan-directory`: a shell script to run Jitana-IAC
      to analyze apps in a directory.
    * `tools/jitana-iac/scripts/scan-device`: a shell script to run Jitana-IAC to
      analyze apps on a connected device.
* C++ Source Code
    * `include/jitana/analysis/resource_sharing.hpp`
    * `tools/jitana-iac/main.cpp`

## Building

Buiding Jitana-IAC is same as other Jitana tools:

    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ../jitana
    make -j8

## Running

The resulting graphs including resource sharing graph are written to the
`output` directory after running Jitana-IAC.

### Analyzing Apps in a Directory

Assuming a directory `../../../apks` contains a set of APK files, run

    cd build/tools/jitana-iac
    ./scan-directory ../../../apks

### Analyzing Apps on a Device

Assuming that the device is already connected, run

    cd build/tools/jitana-iac
    ./scan-device
