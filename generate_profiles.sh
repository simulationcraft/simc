for tier in 19P 19H 19M 19H_NH 19M_NH 20H 20M
do
  PROFDIR='profiles/Tier'$tier'/'
  cd $PROFDIR
  pwd
  ../../engine/simc 'generate_T'$tier".simc"
  cd ../..
done
echo "done"
