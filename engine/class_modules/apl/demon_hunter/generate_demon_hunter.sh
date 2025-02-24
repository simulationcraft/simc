#!/usr/bin/env bash

echo Converting Havoc
py '../ConvertAPL.py' -i havoc.simc -o '../apl_demon_hunter.cpp' -s havoc
#py '../ConvertAPL.py' -i havoc_ptr.simc -o '../apl_demon_hunter.cpp' -s havoc_ptr

echo Converting Vengeance
py '../ConvertAPL.py' -i vengeance.simc -o '../apl_demon_hunter.cpp' -s vengeance
#py '../ConvertAPL.py' -i vengeance_ptr.simc -o '../apl_demon_hunter.cpp' -s vengeance_ptr

echo Done!
read -rsp $'Press enter to continue...\n'
