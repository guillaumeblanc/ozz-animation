#!/bin/bash

# Setup emscripten
cd extern/emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
cd ../..


# Setup cmake
mkdir build-wasm
cd build-wasm

if [[ $# -eq 0 ]] ; then
  emcmake cmake -DCMAKE_BUILD_TYPE=Release ..
else
  emcmake cmake -DCMAKE_BUILD_TYPE=$1 ..
fi

# Build
cmake --build .
