cd ..
set ptr="ptr=1"
set classes=warrior,hunter,monk,paladin,rogue,shaman,mage,warlock,druid,deathknight,priest,demonhunter,evoker

for %%i in (%classes%) do (
simc display_build="0" %ptr% spell_query="spell.class=%%i">spelldatadump/%%i_ptr.txt
)

simc display_build="0" %ptr% spell_query="spell">spelldatadump/allspells_ptr.txt

simc display_build="2" %ptr%>spelldatadump/build_info_ptr.txt
