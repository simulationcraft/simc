#!/bin/sh

/bin/mkdir $PROJECT_DIR/simucraftqt.app/Contents/Frameworks
/bin/cp -a /Library/Frameworks/Sparkle.framework $PROJECT_DIR/simucraftqt.app/Contents/Frameworks
/usr/bin/macdeployqt $PROJECT_DIR/simucraftqt.app -dmg
