#!/bin/sh
if [ $# -lt 1 ]; then
    echo "Usage: generate.sh [ptr] <patch> <input_base> [cache]"
    exit 1
fi

OUTPATH=`dirname $PWD`/engine/dbc/generated

PTR=
if [ "$1" == "ptr" ]; then
    PTR=" --prefix=ptr"
    shift
fi

BUILD=$1
shift
INPUT_BASE=$1
shift

CACHE=
if [ $# -eq 1 ]; then
    CACHE=$1
    shift
fi

DBCINPUT=${INPUT_BASE}/${BUILD}/DBFilesClient
GTINPUT=${INPUT_BASE}/${BUILD}/GameTables
#echo "${OUTPATH},${BUILD},${INPUT_BASE},${CACHE},${PTR}"
#exit 1

if [ ! -d ${INPUT} ]; then
  echo Error: Unable to find input files in ${INPUT}.
  echo "Usage: generate.sh [ptr] <patch> <input_base> [cache]"
  exit 1
fi

echo "Generating ${OUTPATH}/sc_specialization_data.inc"
./dbc_extract.py -p $DBCINPUT -b $BUILD                         -t spec_enum              -o $OUTPATH/sc_specialization_data.inc

echo "Generating ${OUTPATH}/sc_talent_data${PTR:+_ptr}.inc"
./dbc_extract.py -p $DBCINPUT -b $BUILD $PTR --cache "${CACHE}" -t talent                 -o $OUTPATH/sc_talent_data${PTR:+_ptr}.inc

echo "Generating ${OUTPATH}/sc_spec_list${PTR:+_ptr}.inc"
./dbc_extract.py -p $DBCINPUT -b $BUILD $PTR --cache "${CACHE}" -t spec_list              -o $OUTPATH/sc_spec_list${PTR:+_ptr}.inc

echo "Generating ${OUTPATH}/sc_spell_data${PTR:+_ptr}.inc"
./dbc_extract.py -p $DBCINPUT -b $BUILD $PTR --cache "${CACHE}" -t spell                  -o $OUTPATH/sc_spell_data${PTR:+_ptr}.inc

echo "Generating ${OUTPATH}/sc_scale_data${PTR:+_ptr}.inc"
./dbc_extract.py -p $GTINPUT -b $BUILD $PTR --cache "${CACHE}" -t scale                  -o $OUTPATH/sc_scale_data${PTR:+_ptr}.inc

echo "Generating ${OUTPATH}/sc_spell_lists${PTR:+_ptr}.inc"
./dbc_extract.py -p $DBCINPUT -b $BUILD $PTR --cache "${CACHE}" -t class_list             -o $OUTPATH/sc_spell_lists${PTR:+_ptr}.inc
./dbc_extract.py -p $DBCINPUT -b $BUILD $PTR --cache "${CACHE}" -t spec_spell_list        -a $OUTPATH/sc_spell_lists${PTR:+_ptr}.inc
./dbc_extract.py -p $DBCINPUT -b $BUILD $PTR --cache "${CACHE}" -t mastery_list           -a $OUTPATH/sc_spell_lists${PTR:+_ptr}.inc
./dbc_extract.py -p $DBCINPUT -b $BUILD $PTR --cache "${CACHE}" -t racial_list            -a $OUTPATH/sc_spell_lists${PTR:+_ptr}.inc
./dbc_extract.py -p $DBCINPUT -b $BUILD $PTR --cache "${CACHE}" -t set_list2              -a $OUTPATH/sc_spell_lists${PTR:+_ptr}.inc
./dbc_extract.py -p $DBCINPUT -b $BUILD $PTR --cache "${CACHE}" -t rppm_coeff             -a $OUTPATH/sc_spell_lists${PTR:+_ptr}.inc
./dbc_extract.py -p $DBCINPUT -b $BUILD $PTR --cache "${CACHE}" -t artifact               -a $OUTPATH/sc_spell_lists${PTR:+_ptr}.inc

echo "Generating ${OUTPATH}/sc_item_data${PTR:+_ptr}.inc"
./dbc_extract.py -p $DBCINPUT -b $BUILD $PTR --cache "${CACHE}" -t item                   -o $OUTPATH/sc_item_data${PTR:+_ptr}.inc

echo "Generating ${OUTPATH}/sc_item_data${PTR:+_ptr}2.inc"
./dbc_extract.py -p $DBCINPUT -b $BUILD $PTR --cache "${CACHE}" -t random_property_points -o $OUTPATH/sc_item_data${PTR:+_ptr}2.inc
./dbc_extract.py -p $DBCINPUT -b $BUILD $PTR --cache "${CACHE}" -t random_suffix          -a $OUTPATH/sc_item_data${PTR:+_ptr}2.inc
./dbc_extract.py -p $DBCINPUT -b $BUILD $PTR --cache "${CACHE}" -t item_ench              -a $OUTPATH/sc_item_data${PTR:+_ptr}2.inc
./dbc_extract.py -p $DBCINPUT -b $BUILD $PTR --cache "${CACHE}" -t item_armor             -a $OUTPATH/sc_item_data${PTR:+_ptr}2.inc
./dbc_extract.py -p $DBCINPUT -b $BUILD $PTR --cache "${CACHE}" -t weapon_damage          -a $OUTPATH/sc_item_data${PTR:+_ptr}2.inc
./dbc_extract.py -p $DBCINPUT -b $BUILD $PTR --cache "${CACHE}" -t gem_properties         -a $OUTPATH/sc_item_data${PTR:+_ptr}2.inc
./dbc_extract.py -p $DBCINPUT -b $BUILD $PTR --cache "${CACHE}" -t item_upgrade           -a $OUTPATH/sc_item_data${PTR:+_ptr}2.inc
./dbc_extract.py -p $DBCINPUT -b $BUILD $PTR --cache "${CACHE}" -t item_bonus             -a $OUTPATH/sc_item_data${PTR:+_ptr}2.inc
./dbc_extract.py -p $DBCINPUT -b $BUILD $PTR --cache "${CACHE}" -t item_scaling           -a $OUTPATH/sc_item_data${PTR:+_ptr}2.inc
./dbc_extract.py -p $DBCINPUT -b $BUILD $PTR --cache "${CACHE}" -t item_name_desc         -a $OUTPATH/sc_item_data${PTR:+_ptr}2.inc
./dbc_extract.py -p $DBCINPUT -b $BUILD $PTR --cache "${CACHE}" -t item_child             -a $OUTPATH/sc_item_data${PTR:+_ptr}2.inc

