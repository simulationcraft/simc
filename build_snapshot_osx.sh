#!/bin/sh

SUFFIX=$1
shift

if [ ! -f engine/simulationcraft.hpp ]; then
  exit 1
fi

MAJOR_VERSION=$(grep -E -e "^#define SC_MAJOR_VERSION" engine/simulationcraft.hpp|sed -E -e "s/#define SC_MAJOR_VERSION \"([0-9]+)\"/\1/g")
MINOR_VERSION=$(grep -E -e "^#define SC_MINOR_VERSION" engine/simulationcraft.hpp|sed -E -e "s/#define SC_MINOR_VERSION \"([0-9]+)\"/\1/g")
REVISION=$(git log --no-merges -1 --pretty="%h")
DATE=$(date "+%m-%d")
BRANCH=$(git rev-parse --abbrev-ref HEAD)
QT_DIR=${QTDIR:-${HOME}/Qt/5.3/clang_64/bin}
INPUT_NAME=simc-${MAJOR_VERSION}-${MINOR_VERSION}-osx-x86.dmg
OUTPUT_NAME=simc-${MAJOR_VERSION}-${MINOR_VERSION}-${SUFFIX:-${BRANCH}}-osx-x86-${DATE}-${REVISION}.dmg

( ${QT_DIR}/qmake simcqt.pro && make distclean ) || exit 1
( ${QT_DIR}/qmake simcqt.pro && make -j 4 create_release ) || exit 1

if [ -f ${INPUT_NAME} ]; then
  mv ${INPUT_NAME} ${OUTPUT_NAME}
else
  exit 1
fi

