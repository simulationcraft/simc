#!/usr/bin/env sh

SIMC=${SIMC:-../../engine/simc}

if [ ! -d "profiles" ]; then
  echo 'You must run this script with simc root as current working directory.'
  exit 0
fi
cd 'profiles'
# PreRaids doesn't match the typical pattern
PROFDIR='PreRaids'
echo "---$PROFDIR---"
if [ -d $PROFDIR ]; then
  cd $PROFDIR/
  ${SIMC} "../generators/$PROFDIR/PR_Generate.simc"
  cd ../
else
  echo 'Skipped PreRaids, directory not found.'
fi
# DungeonSlice doesn't match the typical pattern
# PROFDIR='DungeonSlice'
# echo "---$PROFDIR---"
# if [ -d $PROFDIR ]; then
#   cd $PROFDIR/
#   ../../engine/simc "../generators/$PROFDIR/DS_Generate.simc"
#   cd ../
# else
#   echo 'Skipped DungeonSlice, directory not found.'
# fi
# TierXX profiles generation
for tier in 29 30
do
  PROFDIR="Tier$tier"
  echo "---$PROFDIR---"
  if [ ! -d $PROFDIR ]; then
    echo "Skipped $PROFDIR, directory not found."
    continue
  fi
  cd $PROFDIR/
  ${SIMC} '../generators/Tier'$tier'/T'$tier'_Generate.simc'
  cd ../
done
echo 'done'
