cd ..
set ptr="ptr=1"
set classes=warrior,hunter,monk,paladin,rogue,shaman,mage,warlock,druid,deathknight,priest,demonhunter

for %%i in (%classes%) do (
simc %ptr% spell_query="spell.class=%%i">spelldatadump/%%i_ptr.txt
)

simc %ptr% spell_query="spell">spelldatadump/allspells_ptr.txt
pause