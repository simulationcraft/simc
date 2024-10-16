cd ..
set classes=warrior,hunter,monk,paladin,rogue,shaman,mage,warlock,druid,deathknight,priest,demonhunter,evoker

for %%i in (%classes%) do (
simc display_build="0" spell_query="spell.class=%%i">spelldatadump/%%i.txt
)

simc display_build="0" spell_query="spell">spelldatadump/allspells.txt

simc display_build="0" spell_query="spell.class=none">spelldatadump/nonclass.txt

simc display_build="2">spelldatadump/build_info.txt

simc display_build="0" show_bonus_ids="1">spelldatadump/bonus_ids.txt
