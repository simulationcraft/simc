#!/bin/sh

/bin/mkdir $PROJECT_DIR/simcraftqt.app/Contents/Frameworks
/bin/cp -a /Library/Frameworks/Sparkle.framework $PROJECT_DIR/simcraftqt.app/Contents/Frameworks
/usr/bin/macdeployqt $PROJECT_DIR/simcraftqt.app -dmg
