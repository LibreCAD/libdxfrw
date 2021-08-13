#!/bin/bash

set -e


conda update -n base -c defaults conda
conda install compilers -y

conda config --set channel_priority strict
conda install --yes --quiet libtool ccache ninja -y

