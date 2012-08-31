#!/bin/bash
HOME_DIR="/home/jon"
SIMC_DIR="$HOME_DIR/simc_mop"
SITE_DIR="$HOME_DIR/sites/simulationcraft.org"
AUTO_DIR="$SITE_DIR/auto"
SPEC_DIR="$HOME_DIR/autosim.d"
MAIL_ADR="\"jon@valvatne.com\" \"autosimc@mailinator.com\""
if [ ! -f "$HOME_DIR/simulate.lock" ]; then
	/usr/bin/svn update "$SIMC_DIR" --non-interactive
	REV=`/usr/bin/svn info "$SIMC_DIR/engine" | grep "Last Changed Rev" | sed "s/[^0-9]*//"`
	REV2=`/usr/bin/svn info "$SIMC_DIR/profiles" | grep "Last Changed Rev" | sed "s/[^0-9]*//"`
	if [ "$REV2" -gt "$REV" ]; then
        	REV=$REV2
	fi
	if [ ! -f "$AUTO_DIR/r$REV-compile.txt" ]; then
		echo "r$REV" >"$HOME_DIR/simulate.lock"
		cd "$SIMC_DIR/engine"
	        make OS=UNIX >"$AUTO_DIR/r$REV-compile.txt" || make -s OS=UNIX 2>"$AUTO_DIR/r$REV-errors.txt"
		if [ -f "$AUTO_DIR/r$REV-errors.txt" ]; then
			cat "$AUTO_DIR/r$REV-errors.txt" | /usr/bin/mail -s "Compile error (r$REV)" $MAIL_ADR
		else
			for SIM_FILE in $SPEC_DIR/*; do
				SIM_NAME=$(basename "$SIM_FILE")
				if [ ! -e "$AUTO_DIR/$SIM_NAME" ]; then
					mkdir "$AUTO_DIR/$SIM_NAME"
				fi
				SIM_SPEC=$(cat "$SPEC_DIR/$SIM_NAME")
				"$SIMC_DIR/engine/simc" $SIM_SPEC "html=$AUTO_DIR/$SIM_NAME/r$REV.html" "output=$AUTO_DIR/$SIM_NAME/r$REV.txt" >"$AUTO_DIR/$SIM_NAME/r$REV.output.txt" 2>"$AUTO_DIR/$SIM_NAME/r$REV.errors.txt"
	                        if [ -s "$AUTO_DIR/$SIM_NAME/r$REV.errors.txt" -o ! -f "$AUTO_DIR/$SIM_NAME/r$REV.html" ]; then
        	                        cat "$AUTO_DIR/$SIM_NAME/r$REV.output.txt" >>"$AUTO_DIR/$SIM_NAME/r$REV.txt"
        	                        cat "$AUTO_DIR/$SIM_NAME/r$REV.errors.txt" >>"$AUTO_DIR/$SIM_NAME/r$REV.txt"
					cat "$AUTO_DIR/$SIM_NAME/r$REV.txt" | /usr/bin/mail -s "Runtime error (r$REV)" $MAIL_ADR
                	        fi
                        	rm "$AUTO_DIR/$SIM_NAME/r$REV.output.txt"
                        	rm "$AUTO_DIR/$SIM_NAME/r$REV.errors.txt"
				if [ -f "$AUTO_DIR/$SIM_NAME/r$REV.html" ]; then
					sed -ri 's/(SimulationCraft [0-9-]+<\/a>)/\1 <a href="http:\/\/code.google.com\/p\/simulationcraft\/source\/detail?r='$REV'">(r'$REV')<\/a>/g' "$AUTO_DIR/$SIM_NAME/r$REV.html"
					BETAFOLDER=`head -50 "$AUTO_DIR/$SIM_NAME/r$REV.html" | grep "for World of Warcraft" | sed "s/.*\([0-9]\)\.\([0-9]\)\.\([0-9]\).*/\1\2\3/"`
					echo $BETAFOLDER > "$SITE_DIR/BETAFOLDER"
					if [ ! -e "$SITE_DIR/$BETAFOLDER" ]; then
						mkdir "$SITE_DIR/$BETAFOLDER"
						/usr/bin/svn add "$SITE_DIR/$BETAFOLDER"
					fi
					cp "$AUTO_DIR/$SIM_NAME/r$REV.html" "$SITE_DIR/$BETAFOLDER/$SIM_NAME.html"
				fi
			done
		fi
		rm "$HOME_DIR/simulate.lock"
	fi
fi

