#!/bin/bash
CLASSES=(warrior hunter monk paladin rogue shaman mage warlock druid deathknight priest demonhunter)

# get directory of this script
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PTR="ptr=1"

cd $DIR/..

for CLASS in "${CLASSES[@]}"
do
  echo "Processing $CLASS"
  FILE="SpellDataDump/${CLASS}_ptr.txt"
  echo $FILE
  ./engine/simc $PTR spell_query="spell.class=$CLASS" > $FILE.unix

  # convert unix line endings to windows since that's been the standard
  sed 's/$'"/`echo \\\r`/" $FILE.unix > $FILE
  rm $FILE.unix
done

FILE=SpellDataDump/allspells_ptr.txt
./engine/simc $PTR spell_query="spell" > $FILE.unix

# convert unix line endings to windows since that's been the standard
sed 's/$'"/`echo \\\r`/" $FILE.unix > $FILE
rm $FILE.unix

FILE=SpellDataDump/soulbind_ptr.txt
./engine/simc $PTR spell_query="soulbind_spell" > $FILE.unix

# convert unix line endings to windows since that's been the standard
sed 's/$'"/`echo \\\r`/" $FILE.unix > $FILE
rm $FILE.unix
