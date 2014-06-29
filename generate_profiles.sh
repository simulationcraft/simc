
for tier in 16H 16N 15H 15N
do
  PROFDIR='profiles/Tier'$tier'/'
  cd $PROFDIR
	../../engine/simc 'generate_T'$tier".simc"
  cd ../..
done
cd profiles/PreRaid/
../../engine/simc 'generate_PreRaid.simc'
cd ../..
