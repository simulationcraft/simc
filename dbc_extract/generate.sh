#!/bin/sh
if [ $# -lt 1 ]; then
    echo "Usage: generate.sh [ptr] <patch> <input_base>"
    exit 1
fi

OUTPATH=`dirname $0`/../engine
INEXT=DBFilesClient

PTR=
if [ "$1" == "ptr" ]; then
    PTR=" --prefix=ptr"
    shift
fi

BUILD=$1
INPUT=${2}/${1}/${INEXT}
OUTPUT=`dirname $0`

if [ ! -d $INPUT ]; then
  echo Error: Unable to find input files.
  echo "Usage: generate.sh [ptr] <patch> <input_base>"
  exit 1
fi

./dbc_extract.py -p $INPUT -b $BUILD$PTR -t talent           > $OUTPUT/sc_talent_data${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT -b $BUILD$PTR -t spell            > $OUTPUT/sc_spell_data${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT -b $BUILD$PTR -t scale            > $OUTPUT/sc_scale_data${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT -b $BUILD$PTR -t class_list       > $OUTPUT/sc_spell_lists${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT -b $BUILD$PTR -t spec_spell_list >> $OUTPUT/sc_spell_lists${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT -b $BUILD$PTR -t mastery_list    >> $OUTPUT/sc_spell_lists${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT -b $BUILD$PTR -t racial_list     >> $OUTPUT/sc_spell_lists${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT -b $BUILD$PTR -t glyph_list      >> $OUTPUT/sc_spell_lists${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT -b $BUILD$PTR -t set_list        >> $OUTPUT/sc_spell_lists${PTR:+_ptr}.inc
