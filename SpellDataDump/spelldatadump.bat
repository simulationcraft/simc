cd ..
set ptr="ptr=1"
set classes=warrior,hunter,monk,paladin,rogue,shaman,mage,warlock,druid,deathknight,priest

for %%i in (%classes%) do (
simc64 %ptr% spell_query="spell.class=%%i">spelldatadump/%%i.txt
)

simc64 %ptr% spell_query="spell">spelldatadump/allspells.txt
pause