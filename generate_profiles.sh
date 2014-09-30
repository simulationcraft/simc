for tier in 16H 17B 17H 17M 17N 17P
do
  PROFDIR='profiles/Tier'$tier'/'
  cd $PROFDIR
	../../simc 'generate_T'$tier".simc"
  cd ../..
done
echo "done"
