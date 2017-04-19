Compiling Jitana on Windows
===========================

Updated: 2017-04-19

Jitana is not intended to run on Windows. Compilation may fail. However, you
can try to run it by:

1. Install Visual Studio 2015.
2. Install CMake. Make sure to add `cmake` to `PATH`
3. Install Boost:

        b2 mdvc address-model=64 install

4. Generate Visual Studio project files:

        cmake -G "Visual Studio 14 2015 Win64" -DBoost_USE_STATIC_LIBS=ON ../jitana

5. Compile it with Visual Studio.

With these steps, I was able to run `jitana-graph` and test cases on Windows 10.
