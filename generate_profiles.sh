# PreRaids doesn't match the typical pattern
cd 'profiles/PreRaids/'
pwd
../../engine/simc 'PR_Generate.simc'
cd ../..
# TierXX profiles generation
for tier in 19 20 21
do
  PROFDIR='profiles/Tier'$tier'/'
  cd $PROFDIR
  pwd
  ../../engine/simc 'T'$tier'_Generate.simc'
  cd ../..
done
echo 'done'
