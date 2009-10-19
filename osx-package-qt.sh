#!/bin/sh

/bin/mkdir $PROJECT_DIR/simcqt.app/Contents/Frameworks
/bin/cp -a /Library/Frameworks/Sparkle.framework $PROJECT_DIR/simcqt.app/Contents/Frameworks
/usr/bin/macdeployqt $PROJECT_DIR/simcqt.app -dmg
