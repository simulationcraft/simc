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
./engine/simc display_build="0" $PTR spell_query="spell" > $FILE.unix
convert_line_ending $FILE

FILE=SpellDataDump/build_info_ptr.txt
./engine/simc display_build="2" > $FILE.unix
convert_line_ending $FILE
