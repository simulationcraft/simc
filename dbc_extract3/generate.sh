#!/bin/sh
if [ $# -lt 1 ]; then
    echo "Usage: generate.sh [ptr] <patch> <input_base> [cache]"
    exit 1
fi

OUTPATH=`dirname $PWD`/engine/dbc/generated

BATCH_FILE=live.conf

PTR=
if [ "$1" = "ptr" ]; then
    PTR=" --prefix=ptr"
    BATCH_FILE=ptr.conf
    shift
fi

BUILD=$1
shift
INPUT_BASE=$1
shift

CACHE=
if [ $# -eq 1 ]; then
    CACHE="--hotfix=$1"
    shift
fi

DBCINPUT=${INPUT_BASE}/${BUILD}/DBFilesClient
GTINPUT=${INPUT_BASE}/${BUILD}/GameTables
#echo "${OUTPATH},${BUILD},${INPUT_BASE},${CACHE},${PTR}"
#exit 1

if [ ! -d ${INPUT} ]; then
  echo Error: Unable to find input files in ${INPUT}.
  echo "Usage: generate.sh [ptr] <patch> <input_base> [hotfix_file]"
  exit 1
fi

./dbc_extract.py          -p $GTINPUT  -b $BUILD $PTR -t scale  -o $OUTPATH/sc_scale_data${PTR:+_ptr}.inc
./dbc_extract.py ${CACHE} -p $DBCINPUT -b $BUILD $PTR -t output ${BATCH_FILE}
