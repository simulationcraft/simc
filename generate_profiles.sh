# PreRaids doesn't match the typical pattern
cd 'profiles/PreRaids/'
pwd
../../engine/simc 'PR_Generate.simc'
cd ../..
# TierXX profiles generation
for tier in 21 22
do
  PROFDIR='profiles/Tier'$tier'/'
  cd $PROFDIR
  pwd
  ../../engine/simc 'T'$tier'_Generate.simc'
  cd ../..
done
echo 'done'
