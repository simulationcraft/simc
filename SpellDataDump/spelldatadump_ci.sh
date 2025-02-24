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

if grep -q "#define SC_USE_PTR 1" engine/config.hpp; then
  HAS_PTR=1
else
  HAS_PTR=0
fi

for CLASS in "${CLASSES[@]}"
do
  echo "Processing $CLASS"
  FILE=SpellDataDump/$CLASS.txt
  echo $FILE
  ${SIMC_CLI_PATH} display_build="0" spell_query="spell.class=$CLASS" > $FILE.unix
  convert_line_ending $FILE

  if [ "$HAS_PTR" -eq 1 ]; then
    FILE=SpellDataDump/${CLASS}_ptr.txt
    echo $FILE
    ${SIMC_CLI_PATH} display_build="0" ptr="1" spell_query="spell.class=$CLASS" > $FILE.unix
    convert_line_ending $FILE
  fi
done

FILE=SpellDataDump/allspells.txt
${SIMC_CLI_PATH} display_build="0" spell_query="spell" > $FILE.unix
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

if [ "$HAS_PTR" -eq 1 ]; then
  FILE=SpellDataDump/allspells_ptr.txt
  ${SIMC_CLI_PATH} display_build="0" ptr="1" spell_query="spell" > $FILE.unix
  convert_line_ending $FILE

  FILE=SpellDataDump/nonclass_ptr.txt
  ${SIMC_CLI_PATH} display_build="0" ptr="1" spell_query="spell.class=none" > $FILE.unix
  convert_line_ending $FILE

  FILE=SpellDataDump/build_info_ptr.txt
  ${SIMC_CLI_PATH} display_build="2" ptr="1" > $FILE.unix
  convert_line_ending $FILE

  FILE=SpellDataDump/bonus_ids_ptr.txt
  ${SIMC_CLI_PATH} display_build="0" ptr="1" show_bonus_ids="1" > $FILE.unix
  convert_line_ending $FILE
fi
