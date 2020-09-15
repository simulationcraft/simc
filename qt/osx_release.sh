#/bin/sh
MAJOR_VERSION=$(grep -E -e "^#define SC_MAJOR_VERSION" engine/config.hpp|sed -E -e "s/#define SC_MAJOR_VERSION \"([0-9]+)\"/\1/g")
MINOR_VERSION=$(grep -E -e "^#define SC_MINOR_VERSION" engine/config.hpp|sed -E -e "s/#define SC_MINOR_VERSION \"([0-9]+)\"/\1/g")
REVISION=$(git log --no-merges -1 --pretty="%h")
DEPLOY_DIR=$(mktemp -d -t SimulationCraft)
DEPLOY_FILES="SimulationCraft.app profiles COPYING LICENSE.BOOST LICENSE.BSD LICENSE.BSD2 LICENSE.MIT LICENSE.LGPL LICENSE.UNLICENSE README.md simc"
echo Revision is: $REVISION
echo Deploying to: $DEPLOY_DIR
echo Version is: $MAJOR_VERSION $MINOR_VERSION

if [ -d $DEPLOY_DIR ]; then
  touch $DEPLOY_DIR/.Trash
  for file in ${DEPLOY_FILES}; do
    cp -pPR "${file}" $DEPLOY_DIR
  done

  sed -i -e "s/GIT-REV-PARSE-HERE/$REVISION/" $DEPLOY_DIR/SimulationCraft.app/Contents/Info.plist
  sed -i -e "s/VERSION-PARSE-HERE/$MAJOR_VERSION\.$MINOR_VERSION/" $DEPLOY_DIR/SimulationCraft.app/Contents/Info.plist

  hdiutil create -fs "HFS+" -srcfolder $DEPLOY_DIR/ -format UDBZ -stretch 1g -volname "Simulationcraft v`defaults read $DEPLOY_DIR/SimulationCraft.app/Contents/Info CFBundleVersion`" -ov simc-$MAJOR_VERSION-$MINOR_VERSION-osx-x86.dmg
  rm -rf $DEPLOY_DIR
fi

