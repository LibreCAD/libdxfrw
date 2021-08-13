#!/bin/bash

set -e

export CCACHE_CPP2=yes
export PROJ_DB_CACHE_DIR="$HOME/.ccache"

ccache -M 200M
ccache -s

export CC="ccache clang"
export CXX="ccache clang++"
export CFLAGS="-Werror -O2"
export CXXFLAGS="-Werror -O2"

mkdir -p build
cd build
cmake ..
make

ccache -s
