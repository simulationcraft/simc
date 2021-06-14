action_priority_list_t* precombat = get_action_priority_list( "precombat" );
action_priority_list_t* def = get_action_priority_list( "default" );
action_priority_list_t* bear = get_action_priority_list( "bear" );
action_priority_list_t* catweave = get_action_priority_list( "catweave" );
action_priority_list_t* cconvoke = get_action_priority_list( "cconvoke" );
action_priority_list_t* lycarac = get_action_priority_list( "lycarac" );
action_priority_list_t* owlweave = get_action_priority_list( "owlweave" );
action_priority_list_t* oconvoke = get_action_priority_list( "oconvoke" );
action_priority_list_t* lycarao = get_action_priority_list( "lycarao" );

precombat->add_action( "cat_form,if=(druid.catweave_bear)|(covenant.night_fae&talent.feral_affinity.enabled)" );
precombat->add_action( "prowl,if=druid.catweave_bear" );
precombat->add_action( "moonkin_form,if=(druid.owlweave_bear)|(covenant.night_fae&talent.balance_affinity.enabled)" );
precombat->add_action( "bear_form,if=((!druid.owlweave_bear&!druid.catweave_bear)&(!covenant.night_fae))|((!druid.owlweave_bear&!druid.catweave_bear)&(covenant.night_fae&talent.restoration_affinity.enabled))" );
precombat->add_action( "heart_of_the_Wild,if=talent.heart_of_the_wild.enabled&(druid.catweave_bear|druid.owlweave_bear|talent.balance_affinity.enabled)" );
precombat->add_action( "wrath,if=druid.owlweave_bear&!covenant.night_fae" );
precombat->add_action( "starfire,if=druid.owlweave_bear&covenant.night_fae" );

def->add_action( "auto_attack,if=!buff.prowl.up" );
def->add_action( "use_items,if=!buff.prowl.up" );
def->add_action( "potion,if=(((talent.heart_of_the_wild.enabled&buff.heart_of_the_wild.up)&(druid.catweave_bear|druid.owlweave_bear)&!buff.prowl.up)|((buff.berserk_bear.up|buff.incarnation_guardian_of_ursoc.up)&(!druid.catweave_bear&!druid.owlweave_bear)))" );
def->add_action( "run_action_list,name=catweave,if=druid.catweave_bear&((cooldown.thrash_bear.remains>0&cooldown.mangle.remains>0&dot.moonfire.remains>=gcd+0.5&rage<40&buff.incarnation_guardian_of_ursoc.down&buff.berserk_bear.down&buff.galactic_guardian.down)|(buff.cat_form.up&energy>25)|(dot.rake.refreshable&dot.rip.refreshable)|(runeforge.oath_of_the_elder_druid.equipped&!buff.oath_of_the_elder_druid.up&(buff.cat_form.up&energy>20)&buff.heart_of_the_wild.remains<=10)|(covenant.kyrian&cooldown.empower_bond.remains<=1&active_enemies<2)|(buff.heart_of_the_wild.up&energy>90))","Catweaving action list will be ran if, mangle/thrash are on cd, rage is below 40, zerk and incarnation are down and Gualactic guardian buff is not active, or if, we're in catform and energy is above 25, Or if we have the Oath legendary equipped, the debuff linked to it is not up and energy is above 20,Or if we're kyrian and Empower bond cooldown is up and enemies are inferior to 2." );
def->add_action( "run_action_list,name=owlweave,if=druid.owlweave_bear&((cooldown.thrash_bear.remains>0&cooldown.mangle.remains>0&rage<15&buff.incarnation.down&buff.berserk_bear.down&buff.galactic_guardian.down)|(buff.moonkin_form.up&dot.sunfire.refreshable)|(buff.moonkin_form.up&buff.heart_of_the_wild.up)|(runeforge.oath_of_the_elder_druid.equipped&!buff.oath_of_the_elder_druid.up)|(covenant.night_fae&cooldown.convoke_the_spirits.remains<=1)|(covenant.kyrian&cooldown.empower_bond.remains<=1&active_enemies<2))","Owlweaving action list will be ran if, mangle/thrash are on cd, rage is below 15, zerk and incarnation are down and Gualactic guardian buff is not active. Or if, we're in moonkin form and sunfire is refreshable, Or if we have the Oath legendary equipped, the debuff linked to it is not up.Or if we're kyrian and Empower bond cooldown is up and enemies are below 2, or if we're Night fae and Convoke cd is up." );
def->add_action( "run_action_list,name=lycarao,if=((runeforge.lycaras_fleeting_glimpse.equipped)&(talent.balance_affinity.enabled)&(buff.lycaras_fleeting_glimpse.up)&(buff.lycaras_fleeting_glimpse.remains<=2))","If we have Lycara legendary equipped and balance affinity as a talent we switch into moonkin form whenever the lycara buff is at or below 2 sec" );
def->add_action( "run_action_list,name=lycarac,if=((runeforge.lycaras_fleeting_glimpse.equipped)&(talent.feral_affinity.enabled)&(buff.lycaras_fleeting_glimpse.up)&(buff.lycaras_fleeting_glimpse.remains<=2))","If we have Lycara legendary equipped and feral affinity as a talent we switch into feral form whenever the lycara buff is at or below 2 sec" );
def->add_action( "run_action_list,name=oconvoke,if=((talent.balance_affinity.enabled)&(!druid.catweave_bear)&(!druid.owlweave_bear)&(covenant.night_fae&cooldown.convoke_the_spirits.remains<=1))","If we're a nightfae and we don't want to catweave/owlweave, and we have balance/feral affinity talented, Whenever convoke cd is up we switch into affinity form to cast it, here moonkin form." );
def->add_action( "run_action_list,name=cconvoke,if=((talent.feral_affinity.enabled)&(!druid.catweave_bear)&(!druid.owlweave_bear)&(covenant.night_fae&cooldown.convoke_the_spirits.remains<=1))","If we're a nightfae and we don't want to catweave/owlweave, and we have balance/feral affinity talented, Whenever convoke cd is up we switch into affinity form to cast it, here catform form." );
def->add_action( "run_action_list,name=bear" );

bear->add_action( "bear_form,if=!buff.bear_form.up" );
bear->add_action( "ravenous_frenzy" );
bear->add_action( "convoke_the_spirits,if=!druid.catweave_bear&!druid.owlweave_bear" );
bear->add_action( "berserk_bear,if=(buff.ravenous_frenzy.up|!covenant.venthyr)" );
bear->add_action( "incarnation,if=(buff.ravenous_frenzy.up|!covenant.venthyr)" );
bear->add_action( "berserking,if=(buff.berserk_bear.up|buff.incarnation_guardian_of_ursoc.up)" );
bear->add_action( "empower_bond,if=(!druid.catweave_bear&!druid.owlweave_bear)|active_enemies>=2" );
bear->add_action( "barkskin,if=talent.brambles.enabled" );
bear->add_action( "adaptive_swarm,if=(!dot.adaptive_swarm_damage.ticking&!action.adaptive_swarm_damage.in_flight&(!dot.adaptive_swarm_heal.ticking|dot.adaptive_swarm_heal.remains>3)|dot.adaptive_swarm_damage.stack<3&dot.adaptive_swarm_damage.remains<5&dot.adaptive_swarm_damage.ticking)" );
bear->add_action( "thrash_bear,target_if=refreshable|dot.thrash_bear.stack<3|(dot.thrash_bear.stack<4&runeforge.luffainfused_embrace.equipped)|active_enemies>=4" );
bear->add_action( "moonfire,if=((buff.galactic_guardian.up)&active_enemies<2)|((buff.galactic_guardian.up)&!dot.moonfire.ticking&active_enemies>1&target.time_to_die>12)" );
bear->add_action( "moonfire,if=(dot.moonfire.remains<=3&(buff.galactic_guardian.up)&active_enemies>5&target.time_to_die>12)" );
bear->add_action( "moonfire,if=(refreshable&active_enemies<2&target.time_to_die>12)|(!dot.moonfire.ticking&active_enemies>1&target.time_to_die>12)" );
bear->add_action( "swipe,if=buff.incarnation_guardian_of_ursoc.down&buff.berserk_bear.down&active_enemies>=4" );
bear->add_action( "maul,if=buff.incarnation.up&active_enemies<2|buff.berserk_bear.up&active_enemies<2" );
bear->add_action( "maul,if=(buff.savage_combatant.stack>=1)&(buff.tooth_and_claw.up)&buff.incarnation.up&active_enemies=2" );
bear->add_action( "mangle,if=buff.incarnation.up&active_enemies<=3" );
bear->add_action( "maul,if=(((buff.tooth_and_claw.stack>=2)|(buff.tooth_and_claw.up&buff.tooth_and_claw.remains<1.5)|(buff.savage_combatant.stack>=3))&active_enemies<3)" );
bear->add_action( "thrash_bear,if=active_enemies>1" );
bear->add_action( "mangle,if=((rage<90)&active_enemies<3)|((rage<85)&active_enemies<3&talent.soul_of_the_forest.enabled)" );
bear->add_action( "pulverize,target_if=dot.thrash_bear.stack>2" );
bear->add_action( "thrash_bear" );
bear->add_action( "maul,if=active_enemies<3" );
bear->add_action( "swipe_bear" );
bear->add_action( "ironfur,if=rage.deficit<40&buff.ironfur.remains<0.5" );

catweave->add_action( "cat_form,if=!buff.cat_form.up" );
catweave->add_action( "rake,if=buff.prowl.up" );
catweave->add_action( "heart_of_the_wild,if=talent.heart_of_the_wild.enabled&!buff.heart_of_the_wild.up" );
catweave->add_action( "empower_bond,if=druid.catweave_bear" );
catweave->add_action( "rake,if=dot.rake.refreshable&combo_points<4" );
catweave->add_action( "rip,if=dot.rip.refreshable&combo_points>=1" );
catweave->add_action( "convoke_the_spirits,if=druid.catweave_bear" );
catweave->add_action( "ferocious_bite,if=combo_points>=4&energy>50" );
catweave->add_action( "adaptive_swarm,if=(!dot.adaptive_swarm_damage.ticking&!action.adaptive_swarm_damage.in_flight&(!dot.adaptive_swarm_heal.ticking|dot.adaptive_swarm_heal.remains>3)|dot.adaptive_swarm_damage.stack<3&dot.adaptive_swarm_damage.remains<5&dot.adaptive_swarm_damage.ticking)" );
catweave->add_action( "shred" );

cconvoke->add_action( "cat_form" );
cconvoke->add_action( "heart_of_the_wild,if=talent.heart_of_the_wild.enabled&!buff.heart_of_the_wild.up" );
cconvoke->add_action( "convoke_the_spirits,if=soulbind.first_strike.enabled&buff.first_strike.up" );
cconvoke->add_action( "convoke_the_spirits,if=talent.heart_of_the_wild.enabled&buff.heart_of_the_wild.up" );
cconvoke->add_action( "convoke_the_spirits,if=talent.heart_of_the_wild.enabled&cooldown.heart_of_the_wild.remains>15" );
cconvoke->add_action( "convoke_the_spirits,if=!talent.heart_of_the_wild.enabled" );

lycarac->add_action( "cat_form" );

owlweave->add_action( "moonkin_form,if=!buff.moonkin_form.up" );
owlweave->add_action( "heart_of_the_wild,if=talent.heart_of_the_wild.enabled&!buff.heart_of_the_wild.up" );
owlweave->add_action( "starsurge" );
owlweave->add_action( "convoke_the_spirits,if=soulbind.first_strike.enabled" );
owlweave->add_action( "empower_bond,if=druid.owlweave_bear" );
owlweave->add_action( "adaptive_swarm,if=(!dot.adaptive_swarm_damage.ticking&!action.adaptive_swarm_damage.in_flight&(!dot.adaptive_swarm_heal.ticking|dot.adaptive_swarm_heal.remains>3)|dot.adaptive_swarm_damage.stack<3&dot.adaptive_swarm_damage.remains<5&dot.adaptive_swarm_damage.ticking)" );
owlweave->add_action( "sunfire,target_if=refreshable" );
owlweave->add_action( "moonfire,target_if=refreshable|buff.galactic_guardian.up" );
owlweave->add_action( "starfire,if=covenant.night_fae&eclipse.any_next" );
owlweave->add_action( "wrath,if=!covenant.night_fae&eclipse.any_next" );
owlweave->add_action( "convoke_the_spirits,if=(buff.eclipse_lunar.up|buff.eclipse_solar.up)" );
owlweave->add_action( "starfire,if=(eclipse.in_lunar|eclipse.solar_next)|(eclipse.in_lunar&buff.starsurge_empowerment_lunar.up)" );
owlweave->add_action( "wrath" );

oconvoke->add_action( "moonkin_form" );
oconvoke->add_action( "heart_of_the_wild,if=talent.heart_of_the_wild.enabled&!buff.heart_of_the_wild.up" );
oconvoke->add_action( "convoke_the_spirits,if=soulbind.first_strike.enabled&buff.first_strike.up" );
oconvoke->add_action( "starfire,if=eclipse.any_next|eclipse.solar_next" );
oconvoke->add_action( "wrath,if=eclipse.any_next|eclipse.lunar_next" );
oconvoke->add_action( "convoke_the_spirits,if=talent.heart_of_the_wild.enabled&buff.heart_of_the_wild.up" );
oconvoke->add_action( "convoke_the_spirits,if=talent.heart_of_the_wild.enabled&cooldown.heart_of_the_wild.remains>15" );
oconvoke->add_action( "convoke_the_spirits,if=!talent.heart_of_the_wild.enabled" );

lycarao->add_action( "moonkin_form" );
