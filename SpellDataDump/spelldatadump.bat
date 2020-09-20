cd ..
set classes=warrior,hunter,monk,paladin,rogue,shaman,mage,warlock,druid,deathknight,priest,demonhunter

for %%i in (%classes%) do (
simc spell_query="spell.class=%%i">spelldatadump/%%i.txt
)

simc spell_query="spell">spelldatadump/allspells.txt

simc spell_query="soulbind_spell">spelldatadump/soulbind.txt
pause