#!/bin/sh

if [ ! -f engine/simulationcraft.hpp ]; then
  exit 1
fi

SUFFIX=$1
shift

APIKEY=$1
shift

if [ -z ${APIKEY} ]; then
  APIKEY=$(cat ${HOME}/.simc-snapshot-apikey 2>/dev/null)
  if [ -z ${APIKEY} ]; then
    echo "No Battle.net API key defined, exiting..."
    exit 1
  fi
fi

MAJOR_VERSION=$(grep -E -e "^#define SC_MAJOR_VERSION" engine/simulationcraft.hpp|sed -E -e "s/#define SC_MAJOR_VERSION \"([0-9]+)\"/\1/g")
MINOR_VERSION=$(grep -E -e "^#define SC_MINOR_VERSION" engine/simulationcraft.hpp|sed -E -e "s/#define SC_MINOR_VERSION \"([0-9]+)\"/\1/g")
REVISION=$(git log --no-merges -1 --pretty="%h")
BRANCH=$(git rev-parse --abbrev-ref HEAD)
QT_DIR=${QTDIR:-${HOME}/Qt5.6/5.6}
INPUT_NAME=simc-${MAJOR_VERSION}-${MINOR_VERSION}-osx-x86.dmg
OUTPUT_NAME=simc-${MAJOR_VERSION}-${MINOR_VERSION}-${SUFFIX:-${BRANCH}}-osx-x86-${REVISION}.dmg

if [ -f .qmake.stash ]; then
  make distclean
fi

( ${QT_DIR}/clang_64/bin/qmake LTO=1 SC_DEFAULT_APIKEY=${APIKEY} simulationcraft.pro && make -j 4 create_release ) || exit 1

if [ -f ${INPUT_NAME} ]; then
  mv ${INPUT_NAME} ${OUTPUT_NAME}
else
  exit 1
fi

