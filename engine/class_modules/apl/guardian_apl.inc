action_priority_list_t* precombat = get_action_priority_list( "precombat" );
action_priority_list_t* def = get_action_priority_list( "default" );
action_priority_list_t* bear = get_action_priority_list( "bear" );
action_priority_list_t* catweave = get_action_priority_list( "catweave" );
action_priority_list_t* owlweave = get_action_priority_list( "owlweave" );
action_priority_list_t* cconvoke = get_action_priority_list( "cconvoke" );
action_priority_list_t* oconvoke = get_action_priority_list( "oconvoke" );

precombat->add_action( "cat_form,if=druid.catweave_bear=1&(cooldown.pause_action.remains|time>30)","Executed before combat begins. Accepts non-harmful actions only." );
precombat->add_action( "moonkin_form,if=(!druid.catweave_bear=1)&(cooldown.pause_action.remains|time>30)" );
precombat->add_action( "heart_of_the_Wild,if=talent.heart_of_the_wild.enabled" );
precombat->add_action( "prowl,if=druid.catweave_bear=1&(cooldown.pause_action.remains|time>30)" );
precombat->add_action( "bear_form,if=(!buff.prowl.up)" );

def->add_action( "auto_attack,if=!buff.prowl.up","Executed every time the actor is available." );
def->add_action( "use_item,slot=trinket1" );
def->add_action( "use_item,slot=trinket2" );
def->add_action( "potion,if=((talent.heart_of_the_wild.enabled&buff.heart_of_the_wild.up)|((buff.berserk_bear.up|buff.incarnation_guardian_of_ursoc.up)&(!druid.catweave_bear&!druid.owlweave_bear)))" );
def->add_action( "run_action_list,name=catweave,if=(target.cooldown.pause_action.remains|time>=30)&druid.catweave_bear=1&buff.tooth_and_claw.remains<1.5&(buff.incarnation_guardian_of_ursoc.down&buff.berserk_bear.down)&(cooldown.thrash_bear.remains>0&cooldown.mangle.remains>0&dot.moonfire.remains>=gcd+0.5)|(buff.cat_form.up&energy>25&druid.catweave_bear=1&buff.tooth_and_claw.remains<1.5)|(buff.heart_of_the_wild.up&energy>90&druid.catweave_bear=1&buff.tooth_and_claw.remains<1.5)" );
def->add_action( "run_action_list,name=owlweave,if=(target.cooldown.pause_action.remains|time>=30)&buff.tooth_and_claw.remains<1.5&((druid.owlweave_bear=1)&buff.incarnation_guardian_of_ursoc.down&buff.berserk_bear.down&cooldown.starsurge.up)" );
def->add_action( "run_action_list,name=bear" );

bear->add_action( "bear_form,if=!buff.bear_form.up" );
bear->add_action( "heart_of_the_Wild,if=talent.heart_of_the_wild.enabled" );
bear->add_action( "moonfire,cycle_targets=1,if=((!ticking&time_to_die>12)|(refreshable&time_to_die>12))&active_enemies<8" );
bear->add_action( "barkskin,if=buff.bear_form.up" );
bear->add_action( "convoke_the_spirits" );
bear->add_action( "berserk_bear" );
bear->add_action( "incarnation" );
bear->add_action( "rage_of_the_sleeper,if=buff.incarnation_guardian_of_ursoc.down&cooldown.incarnation_guardian_of_ursoc.remains>60|buff.incarnation_guardian_of_ursoc.up|(talent.convoke_the_spirits.enabled)" );
bear->add_action( "berserking,if=(buff.berserk_bear.up|buff.incarnation_guardian_of_ursoc.up)" );
bear->add_action( "maul,if=buff.rage_of_the_sleeper.up&buff.tooth_and_claw.stack>0&active_enemies<=6" );
bear->add_action( "ironfur,target_if=!debuff.tooth_and_claw_debuff.up,if=!buff.ironfur.up&!cooldown.pause_action.remains|rage>90" );
bear->add_action( "moonfire,if=buff.galactic_guardian.up&buff.galactic_guardian.remains<=gcd+0.5" );
bear->add_action( "mangle,if=buff.gore.up&active_enemies<11|buff.vicious_cycle_mangle.stack=3" );
bear->add_action( "maul,if=((buff.incarnation.up|buff.berserk_bear.up)&active_enemies<=5&(buff.tooth_and_claw.stack>=2))" );
bear->add_action( "thrash_bear,target_if=refreshable|dot.thrash_bear.stack<3|active_enemies>=5" );
bear->add_action( "swipe,if=buff.incarnation_guardian_of_ursoc.down&buff.berserk_bear.down&active_enemies>=11" );
bear->add_action( "maul,if=((buff.tooth_and_claw.up&buff.tooth_and_claw.remains<1.5)&active_enemies<=5)" );
bear->add_action( "maul,if=active_enemies<=5&buff.vicious_cycle_maul.stack>=3|active_enemies<4&!talent.vicious_cycle_maul.enabled" );
bear->add_action( "mangle,if=(buff.incarnation.up&active_enemies<=4)|(buff.incarnation.up&talent.soul_of_the_forest.enabled&active_enemies<=5)|((rage<90)&active_enemies<11)|((rage<85)&active_enemies<11&talent.soul_of_the_forest.enabled)" );
bear->add_action( "thrash_bear,if=active_enemies>1" );
bear->add_action( "pulverize,target_if=dot.thrash_bear.stack>2" );
bear->add_action( "thrash_bear" );
bear->add_action( "swipe_bear" );

catweave->add_action( "heart_of_the_wild,if=talent.heart_of_the_wild.enabled&!buff.heart_of_the_wild.up&!buff.cat_form.up" );
catweave->add_action( "cat_form,if=!buff.cat_form.up" );
catweave->add_action( "rake,if=buff.prowl.up" );
catweave->add_action( "heart_of_the_wild,if=talent.heart_of_the_wild.enabled&!buff.heart_of_the_wild.up" );
catweave->add_action( "rake,if=dot.rake.refreshable|energy<45" );
catweave->add_action( "rip,if=dot.rip.refreshable&combo_points>=1" );
catweave->add_action( "convoke_the_spirits" );
catweave->add_action( "ferocious_bite,if=combo_points>=4&energy>50" );
catweave->add_action( "shred,if=combo_points<=5" );

owlweave->add_action( "heart_of_the_wild,if=talent.heart_of_the_wild.enabled&!buff.heart_of_the_wild.up&!buff.cat_form.up" );
owlweave->add_action( "moonfire,if=buff.galactic_guardian.up&buff.galactic_guardian.remains<=gcd+1" );
owlweave->add_action( "sunfire,if=dot.sunfire.refreshable" );
owlweave->add_action( "starsurge" );

cconvoke->add_action( "heart_of_the_wild,if=talent.heart_of_the_wild.enabled&!buff.heart_of_the_wild.up&!buff.cat_form.up" );
cconvoke->add_action( "cat_form,if=!buff.cat_form.up" );
cconvoke->add_action( "convoke_the_spirits" );

oconvoke->add_action( "heart_of_the_wild,if=talent.heart_of_the_wild.enabled&!buff.heart_of_the_wild.up&!buff.moonkin_form.up" );
oconvoke->add_action( "moonkin_form,if=!buff.moonkin_form.up" );
oconvoke->add_action( "convoke_the_spirits" );
