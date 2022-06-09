#!/usr/bin/env bash
### every exit != 0 fails the script
set -e

conda install opencv
pip install tensorflow jupyter
cd $HOME
git clone https://github.com/flow-project/flow.git
cd flow
pip install -e . --verbose
