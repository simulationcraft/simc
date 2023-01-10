#!/usr/bin/env bash

echo Converting Affliction
py '../ConvertAPL.py' -i havoc.simc -o '../apl_warlock.cpp' -s affliction

echo Converting Demonology
py '../ConvertAPL.py' -i vengeance.simc -o '../apl_warlock.cpp' -s demonology

echo Converting Destruction
py '../ConvertAPL.py' -i vengeance.simc -o '../apl_warlock.cpp' -s destruction

echo Done!
read -rsp $'Press enter to continue...\n'
