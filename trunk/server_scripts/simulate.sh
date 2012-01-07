#!/bin/bash
GEAR="T12H"
GEAR_PTR="T12H"
/usr/bin/svn update /home/jon/simc --non-interactive
REV=`/usr/bin/svn info /home/jon/simc/engine | grep "Last Changed Rev" | sed "s/[^0-9]*//"`
REV2=`/usr/bin/svn info /home/jon/simc/profiles | grep "Last Changed Rev" | sed "s/[^0-9]*//"`
if [ "$REV2" -gt "$REV" ]; then
	REV=$REV2
fi
if [ ! -f "/home/jon/sites/simulationcraft.org/auto/r$REV.txt" ]; then
	make -C /home/jon/simc/engine/ || make -sC /home/jon/simc/engine/ 2>"/home/jon/sites/simulationcraft.org/auto/r$REV.txt"
fi
if [ ! -f "/home/jon/sites/simulationcraft.org/auto/r$REV.txt" ]; then
	cd /home/jon/simc
	/usr/bin/time -f "r$REV:     %U" -o "/home/jon/sites/simulationcraft.org/auto/runtimes.txt" -a engine/simc Raid_$GEAR.simc html=/home/jon/sites/simulationcraft.org/auto/r$REV.html output=/home/jon/sites/simulationcraft.org/auto/r$REV.txt iterations=10000 threads=2 hosted_html=1
	if [ -e "/home/jon/sites/simulationcraft.org/auto/r$REV.html" ]; then
		LIVEFOLDER=`head -50 /home/jon/sites/simulationcraft.org/auto/r$REV.html | grep "for World of Warcraft" | sed "s/.*\([0-9]\)\.\([0-9]\)\.\([0-9]\).*/\1\2\3/"`
		if [ ! -e "/home/jon/sites/simulationcraft.org/$LIVEFOLDER" ]; then
			mkdir "/home/jon/sites/simulationcraft.org/$LIVEFOLDER"
			/usr/bin/svn add "/home/jon/sites/simulationcraft.org/$LIVEFOLDER"
		fi
		cp /home/jon/sites/simulationcraft.org/auto/r$REV.html /home/jon/sites/simulationcraft.org/$LIVEFOLDER/Raid_$GEAR.html
		echo $LIVEFOLDER > /home/jon/sites/simulationcraft.org/LIVEFOLDER
	fi
	if grep -q "#define SC_USE_PTR ( 1 )" engine/simulationcraft.h; then
		/usr/bin/time -f "r$REV-ptr: %U" -o "/home/jon/sites/simulationcraft.org/auto/runtimes.txt" -a engine/simc ptr=1 Raid_$GEAR_PTR.simc html=/home/jon/sites/simulationcraft.org/auto/r$REV-ptr.html output=/home/jon/sites/simulationcraft.org/auto/r$REV-ptr.txt iterations=5000 threads=2 hosted_html=1
	fi
	PTRFOLDER="nil"
	if [ -e "/home/jon/sites/simulationcraft.org/auto/r$REV-ptr.html" ]; then
		PTRFOLDER=`head -50 /home/jon/sites/simulationcraft.org/auto/r$REV-ptr.html | grep "for World of Warcraft" | sed "s/.*\([0-9]\)\.\([0-9]\)\.\([0-9]\).*/\1\2\3/"`
		if [ ! -e "/home/jon/sites/simulationcraft.org/$PTRFOLDER" ]; then
			mkdir "/home/jon/sites/simulationcraft.org/$PTRFOLDER"
			/usr/bin/svn add "/home/jon/sites/simulationcraft.org/$PTRFOLDER"
		fi
		cp /home/jon/sites/simulationcraft.org/auto/r$REV-ptr.html /home/jon/sites/simulationcraft.org/$PTRFOLDER/Raid_$GEAR_PTR.html
	fi
	echo $PTRFOLDER > /home/jon/sites/simulationcraft.org/PTRFOLDER
fi

