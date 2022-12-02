#!/usr/bin/env bash

echo Converting Vengeance
py '../ConvertAPL.py' -i vengeance.simc -o '../apl_demon_hunter.cpp' -s vengeance

echo Done!
read -rsp $'Press enter to continue...\n'
