for tier in 18N 18H 18M 19P 19H 19M
do
  PROFDIR='profiles/Tier'$tier'/'
  cd $PROFDIR
  pwd
  ../../engine/simc 'generate_T'$tier".simc"
  cd ../..
done
echo "done"
