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
for tier in 29 30 31
do
  PROFDIR="Tier$tier"
  echo "---$PROFDIR---"
  if [ ! -d $PROFDIR ]; then
    echo "Skipped $PROFDIR, directory not found."
    continue
  fi
  cd $PROFDIR/
  ${SIMC_CLI_PATH} '../generators/Tier'$tier'/T'$tier'_Generate.simc'
  cd ../
done
echo 'done'
