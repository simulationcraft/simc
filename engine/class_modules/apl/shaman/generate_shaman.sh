#!/usr/bin/env bash

set -e

: ${PYTHON_EXE:="$(which py || which python3 || which python)"}

trap "echo failed" 1

echo "Generating C++ code for Elemental APLs..."
$PYTHON_EXE '../ConvertAPL.py' -i elemental.simc -o '../apl_shaman.cpp' -s elemental
$PYTHON_EXE '../ConvertAPL.py' -i elemental_ptr.simc -o '../apl_shaman.cpp' -s elemental_ptr
echo "success!"
