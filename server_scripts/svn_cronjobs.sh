/usr/bin/svn update /home/jon/sites/simulationcraft.org --non-interactive --quiet
/usr/bin/svn list http://simulationcraft.googlecode.com/svn/tags/ | grep release-4 | tail -1 | sed "s/.*\([0-9][0-9][0-9]\-[0-9]\).*/\1/" > /home/jon/sites/simulationcraft.org/FRESHRELEASE
/usr/bin/svn list http://simulationcraft.googlecode.com/svn/tags/ | grep release-5 | tail -1 | sed "s/.*\([0-9][0-9][0-9]\-[0-9]\).*/\1/" > /home/jon/sites/simulationcraft.org/FRESHRELEASE_BETA
/home/jon/simulate.sh
