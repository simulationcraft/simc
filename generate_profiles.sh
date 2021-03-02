if [ ! -d "profiles" ]; then
  echo 'You must run this script with simc root as current working directory.'
  exit 0
fi
cd 'profiles'
# TierXX profiles generation
for tier in 4
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
