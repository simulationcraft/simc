{
action_priority_list_t* precombat = get_action_priority_list( "precombat" );
action_priority_list_t* clearcasting = get_action_priority_list( "clearcasting" );
action_priority_list_t* builder_cycle = get_action_priority_list( "builder_cycle" );
action_priority_list_t* berserk_builders = get_action_priority_list( "berserk_builders" );
action_priority_list_t* finisher = get_action_priority_list( "finisher" );
action_priority_list_t* cooldown = get_action_priority_list( "cooldown" );
action_priority_list_t* owlweaving = get_action_priority_list( "owlweaving" );
action_priority_list_t* bloodtalons = get_action_priority_list( "bloodtalons" );
action_priority_list_t* def = get_action_priority_list( "default" );


precombat->add_action( "flask" );
precombat->add_action( "food" );
precombat->add_action( "augmentation" );
precombat->add_action( "snapshot_stats" );
precombat->add_action( "variable,name=swipe_v_shred,value=2" );
precombat->add_action( "variable,name=pw_v_fb,op=setif,value=0,condition=talent.tear_open_wounds.enabled&talent.rip_and_tear.enabled,value_else=4" );
precombat->add_action( "cat_form" );
precombat->add_action( "prowl" );

clearcasting->add_action( "thrash_cat,target_if=refreshable" );
clearcasting->add_action( "swipe_cat,if=spell_targets.swipe_cat>variable.swipe_v_shred" );
clearcasting->add_action( "shred" );
clearcasting->add_action( "brutal_slash" );

builder_cycle->add_action( "run_action_list,name=clearcasting,if=buff.clearcasting.react" );
builder_cycle->add_action( "rake,target_if=max:ticks_gained_on_refresh,if=refreshable|buff.sudden_ambush.up" );
builder_cycle->add_action( "moonfire_cat,target_if=refreshable" );
builder_cycle->add_action( "pool_resource,for_next=1" );
builder_cycle->add_action( "thrash_cat,target_if=refreshable" );
builder_cycle->add_action( "brutal_slash" );
builder_cycle->add_action( "swipe_cat,if=spell_targets.swipe_cat>variable.swipe_v_shred" );
builder_cycle->add_action( "shred" );

berserk_builders->add_action( "rake,target_if=refreshable" );
berserk_builders->add_action( "swipe_cat,if=spell_targets.swipe_cat>variable.swipe_v_shred" );
berserk_builders->add_action( "shred" );

finisher->add_action( "primal_wrath,if=spell_targets.primal_wrath>variable.pw_v_fb", "Threshold at which to spam PW" );
finisher->add_action( "primal_wrath,target_if=refreshable,if=spell_targets.primal_wrath>1", "PW for applying Rip" );
finisher->add_action( "rip,target_if=refreshable" );
finisher->add_action( "pool_resource,for_next=1" );
finisher->add_action( "ferocious_bite,max_energy=1,if=!buff.bs_inc.up" );
finisher->add_action( "ferocious_bite,if=(buff.bs_inc.up&talent.soul_of_the_forest.enabled)" );

cooldown->add_action( "berserk" );
cooldown->add_action( "incarnation" );
cooldown->add_action( "convoke_the_spirits,if=buff.tigers_fury.up&combo_points<3|fight_remains<5" );
cooldown->add_action( "berserking" );
cooldown->add_action( "shadowmeld,if=buff.tigers_fury.up&buff.bs_inc.down&combo_points<4&buff.sudden_ambush.down&dot.rake.pmultiplier<1.6&energy>40&druid.rake.ticks_gained_on_refresh>spell_targets.swipe_cat*2-2&target.time_to_die>5" );
cooldown->add_action( "potion,if=buff.bs_inc.up|fight_remains<cooldown.bs_inc.remains|fight_remains<25" );
cooldown->add_action( "use_items" );

owlweaving->add_action( "starsurge" );
owlweaving->add_action( "sunfire,line_cd=4*gcd" );

bloodtalons->add_action( "rake,target_if=max:druid.rake.ticks_gained_on_refresh,if=(!ticking|(1.2*persistent_multiplier>=dot.rake.pmultiplier)|(active_bt_triggers=2&refreshable))&buff.bt_rake.down", "actions.owlweaving+=/moonfire,line_cd=4*gcd" );
bloodtalons->add_action( "lunar_inspiration,if=refreshable&buff.bt_moonfire.down" );
bloodtalons->add_action( "thrash_cat,target_if=refreshable&buff.bt_thrash.down" );
bloodtalons->add_action( "brutal_slash,if=buff.bt_brutal_slash.down" );
bloodtalons->add_action( "swipe_cat,if=buff.bt_swipe.down" );
bloodtalons->add_action( "shred,if=buff.bt_shred.down" );
bloodtalons->add_action( "swipe_cat,if=buff.bt_swipe.down" );
bloodtalons->add_action( "thrash_cat,if=buff.bt_thrash.down" );
bloodtalons->add_action( "rake,if=buff.bt_rake.down&combo_points>4" );

def->add_action( "prowl" );
def->add_action( "tigers_fury,if=energy.deficit>40|buff.bs_inc.up" );
def->add_action( "cat_form,if=!buff.cat_form.up&energy>50" );
def->add_action( "auto_attack,if=!buff.prowl.up&!buff.shadowmeld.up" );
def->add_action( "call_action_list,name=cooldown" );
def->add_action( "adaptive_swarm,target_if=((!dot.adaptive_swarm_damage.ticking|dot.adaptive_swarm_damage.remains<2)&(dot.adaptive_swarm_damage.stack<3|!dot.adaptive_swarm_heal.stack>1)&!action.adaptive_swarm_heal.in_flight&!action.adaptive_swarm_damage.in_flight&!action.adaptive_swarm.in_flight)&target.time_to_die>5|active_enemies>2&!dot.adaptive_swarm_damage.ticking&energy<35&target.time_to_die>5" );
def->add_action( "ferocious_bite,if=buff.apex_predators_craving.up" );
def->add_action( "feral_frenzy,if=combo_points<2" );
def->add_action( "call_action_list,name=finisher,if=combo_points=5" );
def->add_action( "call_action_list,name=bloodtalons,if=buff.bloodtalons.down" );
def->add_action( "call_action_list,name=berserk_builders,if=combo_points<5&(buff.bs_inc.up|buff.shadowmeld.up|buff.prowl.up)" );
def->add_action( "call_action_list,name=builder_cycle,if=combo_points<5" );
def->add_action( "call_action_list,name=owlweaving,if=buff.bs_inc.down&energy<80" );
}
