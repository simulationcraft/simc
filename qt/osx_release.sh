#/bin/sh
MAJOR_VERSION=$(grep -E -e "^#define SC_MAJOR_VERSION" engine/simulationcraft.hpp|sed -E -e "s/#define SC_MAJOR_VERSION \"([0-9]+)\"/\1/g")
MINOR_VERSION=$(grep -E -e "^#define SC_MINOR_VERSION" engine/simulationcraft.hpp|sed -E -e "s/#define SC_MINOR_VERSION \"([0-9]+)\"/\1/g")
REVISION=$((svn info .||git --git-dir=".git")|grep "Revision"|sed -E -e "s/Revision:[^0-9]+//")
DEPLOY_DIR=$(mktemp -d -t SimulationCraft)
CONTENTS_FOLDER_PATH=SimulationCraft.app/Contents
echo Revision is: $REVISION
echo Deploying to: $DEPLOY_DIR
echo Version is: $MAJOR_VERSION $MINOR_VERSION

cp -rf "SimulationCraft.app" $DEPLOY_DIR
sed -i -e "s/GIT-REV-PARSE-HERE/$REVISION/" $DEPLOY_DIR/SimulationCraft.app/Contents/Info.plist
sed -i -e "s/VERSION-PARSE-HERE/$MAJOR_VERSION\.$MINOR_VERSION/" $DEPLOY_DIR/SimulationCraft.app/Contents/Info.plist

hdiutil create -srcfolder $DEPLOY_DIR/SimulationCraft.app -srcfolder profiles -srcfolder READ_ME_FIRST.txt -srcfolder "simc" -nospotlight -format UDBZ -volname "Simulationcraft v`defaults read $DEPLOY_DIR/SimulationCraft.app/Contents/Info CFBundleVersion`" -ov -puppetstrings simc-$MAJOR_VERSION-$MINOR_VERSION-osx-x86.dmg
[ -d $DEPLOY_DIR ] && rm -rf $DEPLOY_DIR

