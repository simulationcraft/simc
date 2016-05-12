for tier in 17B 17H 17M 17N 17P 18N 18H 18M 19P
do
  PROFDIR='profiles/Tier'$tier'/'
  cd $PROFDIR
  pwd
  ../../engine/simc 'generate_T'$tier".simc"
  cd ../..
done
echo "done"
