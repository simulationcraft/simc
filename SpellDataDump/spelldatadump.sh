#!/bin/sh
CLASSES=(warrior hunter monk paladin rogue shaman mage warlock druid deathknight priest demonhunter)

# get directory of this script
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd $DIR/..

for CLASS in "${CLASSES[@]}"
do
  echo "Processing $CLASS"
  FILE=SpellDataDump/$CLASS.txt
  echo $FILE
  ./engine/simc spell_query="spell.class=$CLASS" > $FILE.unix

  # convert unix line endings to windows since that's been the standard
  sed 's/$'"/`echo \\\r`/" $FILE.unix > $FILE
  rm $FILE.unix
done

FILE=SpellDataDump/allspells.txt
./engine/simc spell_query="spell" > $FILE.unix

# convert unix line endings to windows since that's been the standard
sed 's/$'"/`echo \\\r`/" $FILE.unix > $FILE
rm $FILE.unix

FILE=SpellDataDump/soulbind.txt
./engine/simc spell_query="soulbind_spell" > $FILE.unix

# convert unix line endings to windows since that's been the standard
sed 's/$'"/`echo \\\r`/" $FILE.unix > $FILE
rm $FILE.unix
