
for tier in 16H 17N
do
  PROFDIR='profiles/Tier'$tier'/'
  cd $PROFDIR
	../../simc 'generate_T'$tier".simc"
  cd ../..
done
echo "done"
