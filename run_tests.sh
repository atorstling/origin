#!/bin/bash -eu
echo setting up chroot env
if [[ $(uname) == "Darwin" ]]; then
  ./testsetup-osx.sh
else
  ./testsetup.sh
fi
echo running tests
python3 tests.py
