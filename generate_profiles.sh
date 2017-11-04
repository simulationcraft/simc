for tier in 19P 19M 19M_NH 20M 21M
do
  PROFDIR='profiles/Tier'$tier'/'
  cd $PROFDIR
  pwd
  ../../engine/simc 'generate_T'$tier".simc"
  cd ../..
done
echo "done"
