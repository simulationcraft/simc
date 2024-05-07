#!/usr/bin/env bash

echo Converting Havoc
py '../ConvertAPL.py' -i havoc.simc -o '../apl_demon_hunter.cpp' -s havoc

echo Converting Vengeance
py '../ConvertAPL.py' -i vengeance.simc -o '../apl_demon_hunter.cpp' -s vengeance

echo Done!
read -rsp $'Press enter to continue...\n'
