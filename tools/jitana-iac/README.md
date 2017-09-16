# Jitana-IAC

Jitana-IAC is not mature yet as it handles only implicit and explicit intents.

We recognize that this implementation still has design/efficiency/cleanness
problems. We will fix them in the upcoming commits.

## Files

* Shell Scripts for Launching
    * `tools/jitana-iac/scripts/launch`
* C++ Source Code
    * `include/jitana/analysis/intent_flow.hpp`
    * `include/jitana/analysis/intent_flow_string.hpp`
    * `include/jitana/analysis/intent_flow_intraprocedural.hpp`
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
    ./launch --path ../../../apks

### Analyzing Apps on a Device

Assuming that the device is already connected, run

    cd build/tools/jitana-iac
    ./launch --device
