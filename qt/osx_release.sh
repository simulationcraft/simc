#/bin/sh
MAJOR_VERSION=$(grep -E -e "^#define SC_MAJOR_VERSION" engine/simulationcraft.hpp|sed -E -e "s/#define SC_MAJOR_VERSION \"([0-9]+)\"/\1/g")
MINOR_VERSION=$(grep -E -e "^#define SC_MINOR_VERSION" engine/simulationcraft.hpp|sed -E -e "s/#define SC_MINOR_VERSION \"([0-9]+)\"/\1/g")
REVISION=$(git log --no-merges -1 --pretty="%h")
DEPLOY_DIR=$(mktemp -d -t SimulationCraft)
echo Revision is: $REVISION
echo Deploying to: $DEPLOY_DIR
echo Version is: $MAJOR_VERSION $MINOR_VERSION

if [ -d $DEPLOY_DIR ]; then
  cp -rf "SimulationCraft.app" $DEPLOY_DIR
  sed -i -e "s/GIT-REV-PARSE-HERE/$REVISION/" $DEPLOY_DIR/SimulationCraft.app/Contents/Info.plist
  sed -i -e "s/VERSION-PARSE-HERE/$MAJOR_VERSION\.$MINOR_VERSION/" $DEPLOY_DIR/SimulationCraft.app/Contents/Info.plist

  hdiutil create  -srcfolder $DEPLOY_DIR/SimulationCraft.app -srcfolder profiles -srcfolder COPYING -srcfolder readme.txt -srcfolder "simc" -nospotlight -format UDBZ -stretch 1g -volname "Simulationcraft v`defaults read $DEPLOY_DIR/SimulationCraft.app/Contents/Info CFBundleVersion`" -ov simc-$MAJOR_VERSION-$MINOR_VERSION-osx-x86.dmg
  rm -rf $DEPLOY_DIR
fi

