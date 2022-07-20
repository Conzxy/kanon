#!/bin/bash
cd $KANON_BUILD_PATH

cmake .. -DCMAKE_BUILD_TYPE=Release &&
cmake --build . --target all -j 2


