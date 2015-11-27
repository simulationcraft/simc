#!/bin/sh
if [ $# -lt 1 ]; then
    echo "Usage: generate.sh [ptr] [wowversion] <patch> <input_base>"
    exit 1
fi

OUTPATH=`dirname $PWD`/engine/dbc/generated
INEXT=DBFilesClient

PTR=
if [ "$1" == "ptr" ]; then
    PTR=" --prefix=ptr"
    shift
fi

BUILD=$1
INPUT=${2}/${1}/${INEXT}

#if [ ! -d ${INPUT} ]; then
#  echo Error: Unable to find input files in ${INPUT}.
#  echo "Usage: generate.sh [ptr] <patch> <input_base>"
#  exit 1
#fi

./dbc_extract.py -p $INPUT -b $BUILD -t spec_enum -o $OUTPATH/sc_specialization_data.inc

./dbc_extract.py -p $INPUT -b $BUILD$PTR -t talent                 -o $OUTPATH/sc_talent_data${PTR:+_ptr}.inc

./dbc_extract.py -p $INPUT -b $BUILD$PTR -t spec_list              -o $OUTPATH/sc_spec_list${PTR:+_ptr}.inc

./dbc_extract.py -p $INPUT -b $BUILD$PTR -t spell                  -o $OUTPATH/sc_spell_data${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT -b $BUILD$PTR -t rppm_coeff             -a $OUTPATH/sc_spell_data${PTR:+_ptr}.inc

./dbc_extract.py -p $INPUT -b $BUILD$PTR -t scale                  -o $OUTPATH/sc_scale_data${PTR:+_ptr}.inc

./dbc_extract.py -p $INPUT -b $BUILD$PTR -t class_list             -o $OUTPATH/sc_spell_lists${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT -b $BUILD$PTR -t spec_spell_list        -a $OUTPATH/sc_spell_lists${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT -b $BUILD$PTR -t mastery_list           -a $OUTPATH/sc_spell_lists${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT -b $BUILD$PTR -t racial_list            -a $OUTPATH/sc_spell_lists${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT -b $BUILD$PTR -t glyph_list             -a $OUTPATH/sc_spell_lists${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT -b $BUILD$PTR -t set_list2              -a $OUTPATH/sc_spell_lists${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT -b $BUILD$PTR -t perk_list              -a $OUTPATH/sc_spell_lists${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT -b $BUILD$PTR -t glyph_property_list    -a $OUTPATH/sc_spell_lists${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT -b $BUILD$PTR -t artifact               -a $OUTPATH/sc_spell_lists${PTR:+_ptr}.inc

./dbc_extract.py -p $INPUT -b $BUILD$PTR -t item                   -o $OUTPATH/sc_item_data${PTR:+_ptr}.inc

./dbc_extract.py -p $INPUT -b $BUILD$PTR -t random_property_points -o $OUTPATH/sc_item_data${PTR:+_ptr}2.inc
./dbc_extract.py -p $INPUT -b $BUILD$PTR -t random_suffix          -a $OUTPATH/sc_item_data${PTR:+_ptr}2.inc
./dbc_extract.py -p $INPUT -b $BUILD$PTR -t item_ench              -a $OUTPATH/sc_item_data${PTR:+_ptr}2.inc
./dbc_extract.py -p $INPUT -b $BUILD$PTR -t item_armor             -a $OUTPATH/sc_item_data${PTR:+_ptr}2.inc
./dbc_extract.py -p $INPUT -b $BUILD$PTR -t weapon_damage          -a $OUTPATH/sc_item_data${PTR:+_ptr}2.inc
./dbc_extract.py -p $INPUT -b $BUILD$PTR -t gem_properties         -a $OUTPATH/sc_item_data${PTR:+_ptr}2.inc
./dbc_extract.py -p $INPUT -b $BUILD$PTR -t item_upgrade           -a $OUTPATH/sc_item_data${PTR:+_ptr}2.inc
./dbc_extract.py -p $INPUT -b $BUILD$PTR -t item_bonus             -a $OUTPATH/sc_item_data${PTR:+_ptr}2.inc
./dbc_extract.py -p $INPUT -b $BUILD$PTR -t item_scaling           -a $OUTPATH/sc_item_data${PTR:+_ptr}2.inc
./dbc_extract.py -p $INPUT -b $BUILD$PTR -t item_name_desc         -a $OUTPATH/sc_item_data${PTR:+_ptr}2.inc

