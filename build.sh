#!/bin/bash
cd ~/kanon/build

BUILD_TYPE="Debug"
if [ "$1" == "Release" ]
then
  BUILD_TYPE="Release"
fi

cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE &&
cmake --build . --target all -j 8 &&
sudo cmake --install .


