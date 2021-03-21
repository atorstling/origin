#!/bin/bash -eu
echo setting up chroot env
./testsetup.sh
echo running tests
python tests.py
