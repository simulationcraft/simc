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

cd $DIR/..

for CLASS in "${CLASSES[@]}"
do
  echo "Processing $CLASS"
  FILE=SpellDataDump/$CLASS.txt
  echo $FILE
  ${SIMC_CLI_PATH} display_build="0" spell_query="spell.class=$CLASS" > $FILE.unix
  convert_line_ending $FILE
done

FILE=SpellDataDump/allspells.txt
echo "WARNING: allspells.txt will be deprecated in the future. Please refer to the class files or nonclass.txt for non-class spells." > $FILE.unix
echo >> $FILE.unix
${SIMC_CLI_PATH} display_build="0" spell_query="spell" >> $FILE.unix
convert_line_ending $FILE

FILE=SpellDataDump/nonclass.txt
${SIMC_CLI_PATH} display_build="0" spell_query="spell.class=none" > $FILE.unix
convert_line_ending $FILE

FILE=SpellDataDump/build_info.txt
${SIMC_CLI_PATH} display_build="2" > $FILE.unix
convert_line_ending $FILE

FILE=SpellDataDump/bonus_ids.txt
${SIMC_CLI_PATH} display_build="0" show_bonus_ids="1" > $FILE.unix
convert_line_ending $FILE
