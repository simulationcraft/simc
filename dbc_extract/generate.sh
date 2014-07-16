#!/bin/sh
if [ $# -lt 1 ]; then
    echo "Usage: generate.sh [ptr] [wowversion] <patch> <input_base>"
    exit 1
fi

OUTPATH=`dirname $PWD`/engine/dbc
INEXT=DBFilesClient

PTR=
WOWVERSION=
if [ "$1" == "ptr" ]; then
    PTR=" --prefix=ptr"
    shift
fi

if [[ "$1" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    WOWVERSION="-v $1"
    shift
fi

BUILD=$1
INPUT=${2}/${1}/${INEXT}
CACHEDIR="cache/"
if [ x"$PTR" != x"" ]; then
    CACHEDIR+="ptr"
else
    CACHEDIR+="live"
fi

if [ ! -d $INPUT ]; then
  echo Error: Unable to find input files.
  echo "Usage: generate.sh [ptr] <patch> <input_base>"
  exit 1
fi


./dbc_extract.py -p $INPUT $WOWVERSION -b $BUILD$PTR -t talent                  > $OUTPATH/sc_talent_data${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT $WOWVERSION -b $BUILD$PTR -t spec_list               > $OUTPATH/sc_spec_list${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT --cache=$CACHEDIR $WOWVERSION -b $BUILD$PTR -t spell> $OUTPATH/sc_spell_data${PTR:+_ptr}.inc
if [ x"$WOWVERSION" != x"-v 5.3.0" ]; then
  ./dbc_extract.py -p $INPUT $WOWVERSION -b $BUILD$PTR -t rppm_coeff             >> $OUTPATH/sc_spell_data${PTR:+_ptr}.inc
fi
./dbc_extract.py -p $INPUT $WOWVERSION -b $BUILD$PTR -t scale                   > $OUTPATH/sc_scale_data${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT $WOWVERSION -b $BUILD$PTR -t class_list              > $OUTPATH/sc_spell_lists${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT $WOWVERSION -b $BUILD$PTR -t spec_spell_list        >> $OUTPATH/sc_spell_lists${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT $WOWVERSION -b $BUILD$PTR -t mastery_list           >> $OUTPATH/sc_spell_lists${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT $WOWVERSION -b $BUILD$PTR -t racial_list            >> $OUTPATH/sc_spell_lists${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT $WOWVERSION -b $BUILD$PTR -t glyph_list             >> $OUTPATH/sc_spell_lists${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT $WOWVERSION -b $BUILD$PTR -t set_list               >> $OUTPATH/sc_spell_lists${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT $WOWVERSION -b $BUILD$PTR -t set_list2              >> $OUTPATH/sc_spell_lists${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT $WOWVERSION -b $BUILD$PTR -t perk_list              >> $OUTPATH/sc_spell_lists${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT $WOWVERSION -b $BUILD$PTR -t glyph_property_list    >> $OUTPATH/sc_spell_lists${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT --cache=$CACHEDIR $WOWVERSION -b $BUILD$PTR -t item > $OUTPATH/sc_item_data${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT $WOWVERSION -b $BUILD$PTR -t random_property_points >> $OUTPATH/sc_item_data${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT $WOWVERSION -b $BUILD$PTR -t random_suffix          >> $OUTPATH/sc_item_data${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT $WOWVERSION -b $BUILD$PTR -t item_ench              >> $OUTPATH/sc_item_data${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT $WOWVERSION -b $BUILD$PTR -t item_armor             >> $OUTPATH/sc_item_data${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT $WOWVERSION -b $BUILD$PTR -t weapon_damage          >> $OUTPATH/sc_item_data${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT $WOWVERSION -b $BUILD$PTR -t gem_properties         >> $OUTPATH/sc_item_data${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT $WOWVERSION -b $BUILD$PTR -t item_upgrade           >> $OUTPATH/sc_item_data${PTR:+_ptr}.inc
./dbc_extract.py -p $INPUT $WOWVERSION -b $BUILD$PTR -t item_bonus             >> $OUTPATH/sc_item_data${PTR:+_ptr}.inc

(
cat << zz1234
// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SPECIALIZATION_HPP
#define SPECIALIZATION_HPP

zz1234
) > $OUTPATH/specialization.hpp

./dbc_extract.py -p $INPUT $WOWVERSION -b $BUILD -t spec_enum >> $OUTPATH/specialization.hpp

echo "#endif\n" >> $OUTPATH/specialization.hpp

