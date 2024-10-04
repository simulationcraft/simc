#!/bin/bash
# convert unix line endings to windows since that's been the standard
convert_line_ending()
{
  sed 's/$'"/`echo \\\r`/" $1.unix > $1
  rm $1.unix
}

CLASSES=(warrior hunter monk paladin rogue shaman mage warlock druid deathknight priest demonhunter evoker)

# get directory of this script
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PTR="ptr=1"

cd $DIR/..

for CLASS in "${CLASSES[@]}"
do
  echo "Processing $CLASS"
  FILE="SpellDataDump/${CLASS}_ptr.txt"
  echo $FILE
  ./engine/simc display_build="0" $PTR spell_query="spell.class=$CLASS" > $FILE.unix
  convert_line_ending $FILE
done

FILE=SpellDataDump/allspells_ptr.txt
echo "WARNING: allspells_ptr.txt will be deprecated in the future. Please refer to the class files or nonclass_ptr.txt for non-class spells." > $FILE.unix
echo >> $FILE.unix
./engine/simc display_build="0" $PTR spell_query="spell" >> $FILE.unix
convert_line_ending $FILE

FILE=SpellDataDump/nonclass_ptr.txt
./engine/simc display_build="0" $PTR spell_query="spell.class=none" > $FILE.unix
convert_line_ending $FILE

FILE=SpellDataDump/build_info_ptr.txt
./engine/simc display_build="2" $PTR > $FILE.unix
convert_line_ending $FILE

FILE=SpellDataDump/bonus_ids_ptr.txt
./engine/simc display_build="0" $PTR show_bonus_ids="1" > $FILE.unix
convert_line_ending $FILE
