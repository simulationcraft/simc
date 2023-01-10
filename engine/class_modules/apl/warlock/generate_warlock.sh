#!/usr/bin/env bash

echo Converting Affliction
py '../ConvertAPL.py' -i affliction.simc -o '../apl_warlock.cpp' -s affliction

echo Converting Demonology
py '../ConvertAPL.py' -i demonology.simc -o '../apl_warlock.cpp' -s demonology

echo Converting Destruction
py '../ConvertAPL.py' -i destruction.simc -o '../apl_warlock.cpp' -s destruction

echo Done!
read -rsp $'Press enter to continue...\n'
