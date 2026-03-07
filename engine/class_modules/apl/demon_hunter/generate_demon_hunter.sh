#!/usr/bin/env bash

set -e

: ${PYTHON_EXE:="$(which py || which python3 || which python)"}

trap "echo failed" 1

echo Converting Devourer
$PYTHON_EXE '../ConvertAPL.py' -i devourer.simc -o '../apl_demon_hunter.cpp' -s devourer
#py '../ConvertAPL.py' -i devourer_ptr.simc -o '../apl_demon_hunter.cpp' -s devourer_ptr

echo Converting Havoc
$PYTHON_EXE '../ConvertAPL.py' -i havoc.simc -o '../apl_demon_hunter.cpp' -s havoc
#py '../ConvertAPL.py' -i havoc_ptr.simc -o '../apl_demon_hunter.cpp' -s havoc_ptr

echo Converting Vengeance
$PYTHON_EXE '../ConvertAPL.py' -i vengeance.simc -o '../apl_demon_hunter.cpp' -s vengeance
#py '../ConvertAPL.py' -i vengeance_ptr.simc -o '../apl_demon_hunter.cpp' -s vengeance_ptr

echo "Done!"
