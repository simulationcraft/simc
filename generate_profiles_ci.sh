#!/usr/bin/env sh
set -e

if [ ! -d "profiles" ]; then
  echo 'You must run this script with simc root as current working directory.'
  exit 0
fi
cd 'profiles'
PROFDIR='PreRaids'
echo "---$PROFDIR---"
if [ -d $PROFDIR ]; then
  cd $PROFDIR/
  ${SIMC_CLI_PATH} "../generators/$PROFDIR/PR_Generate.simc"
  cd ../
else
  echo 'Skipped PreRaids, directory not found.'
fi
for season in TWW1 TWW2
do
  PROFDIR="$season"
  echo "---$PROFDIR---"
  if [ ! -d $PROFDIR ]; then
    echo "Skipped $PROFDIR, directory not found."
    continue
  fi
  cd $PROFDIR/
  ${SIMC_CLI_PATH} '../generators/'$season'/'$season'_Generate.simc'
  cd ../
done
echo 'done'
