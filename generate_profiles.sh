
for tier in 16H 16N 15H 15N 14H 14N
do
  PROFDIR='profiles/Tier'$tier'/'
  cd $PROFDIR
	../../simc 'generate_T'$tier".simc"
  cd ../..
done