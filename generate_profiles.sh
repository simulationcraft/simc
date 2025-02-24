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
# TWWX profiles generation
for season in TWW1 TWW2
do
  PROFDIR="$season"
  echo "---$PROFDIR---"
  if [ ! -d $PROFDIR ]; then
    echo "Skipped $PROFDIR, directory not found."
    continue
  fi
  cd $PROFDIR/
  ${SIMC} '../generators/'$season'/'$season'_Generate.simc'
  cd ../
done
echo 'done'
