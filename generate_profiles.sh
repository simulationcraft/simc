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
  ../../engine/simc "../generators/$PROFDIR/PR_Generate.simc"
  cd ../
else
  echo 'Skipped PreRaids, directory not found.'
fi
# DungeonSlice doesn't match the typical pattern
PROFDIR='DungeonSlice'
echo "---$PROFDIR---"
if [ -d $PROFDIR ]; then
  cd $PROFDIR/
  ../../engine/simc "../generators/$PROFDIR/DS_Generate.simc"
  cd ../
else
  echo 'Skipped DungeonSlice, directory not found.'
fi
# TierXX profiles generation
for tier in 25
do
  PROFDIR="Tier$tier"
  echo "---$PROFDIR---"
  if [ ! -d $PROFDIR ]; then
    echo "Skipped $PROFDIR, directory not found."
    continue
  fi
  cd $PROFDIR/
  ../../engine/simc '../generators/Tier'$tier'/T'$tier'_Generate.simc'
  cd ../
done
echo 'done'
