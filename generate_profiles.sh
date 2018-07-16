# PreRaids doesn't match the typical pattern
cd 'profiles/PreRaids/'
pwd
rm -rf ./*
../../engine/simc '../generators/PreRaids/PR_Generate.simc'
cd ../
# TierXX profiles generation
for tier in 21
do
  PROFDIR='Tier'$tier'/'
  cd $PROFDIR
  pwd
  rm -rf ./*
  ../../engine/simc '../generators/Tier'$tier'/T'$tier'_Generate.simc'
  cd ../
done
echo 'done'
