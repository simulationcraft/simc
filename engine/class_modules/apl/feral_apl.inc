action_priority_list_t* precombat = get_action_priority_list( "precombat" );
action_priority_list_t* def = get_action_priority_list( "default" );
action_priority_list_t* aoe = get_action_priority_list( "aoe" );
action_priority_list_t* berserk_builders = get_action_priority_list( "berserk_builders" );
action_priority_list_t* bloodtalons = get_action_priority_list( "bloodtalons" );
action_priority_list_t* bloodtalons_aoe = get_action_priority_list( "bloodtalons_aoe" );
action_priority_list_t* builder = get_action_priority_list( "builder" );
action_priority_list_t* clearcasting = get_action_priority_list( "clearcasting" );
action_priority_list_t* cooldown = get_action_priority_list( "cooldown" );
action_priority_list_t* finisher = get_action_priority_list( "finisher" );
action_priority_list_t* owlweaving = get_action_priority_list( "owlweaving" );

precombat->add_action( "use_item,name=algethar_puzzle_box" );
precombat->add_action( "cat_form" );
precombat->add_action( "prowl" );

def->add_action( "prowl,if=buff.bs_inc.down" );
def->add_action( "invoke_external_buff,name=power_infusion,if=buff.bs_inc.up|fight_remains<cooldown.bs_inc.remains" );
def->add_action( "variable,name=need_bt,value=talent.bloodtalons.enabled&buff.bloodtalons.down" );
def->add_action( "tigers_fury" );
def->add_action( "rake,if=buff.prowl.up|buff.shadowmeld.up" );
def->add_action( "cat_form,if=!buff.cat_form.up" );
def->add_action( "auto_attack,if=!buff.prowl.up&!buff.shadowmeld.up" );
def->add_action( "call_action_list,name=cooldown" );
def->add_action( "adaptive_swarm,target_if=((!dot.adaptive_swarm_damage.ticking|dot.adaptive_swarm_damage.remains<2)&(dot.adaptive_swarm_damage.stack<3|!dot.adaptive_swarm_heal.stack>1)&!action.adaptive_swarm_heal.in_flight&!action.adaptive_swarm_damage.in_flight&!action.adaptive_swarm.in_flight)&target.time_to_die>5|active_enemies>2&!dot.adaptive_swarm_damage.ticking&energy<35&target.time_to_die>5" );
def->add_action( "feral_frenzy,if=combo_points<2|combo_points=2&buff.bs_inc.up" );
def->add_action( "run_action_list,name=aoe,if=spell_targets.swipe_cat>1&talent.primal_wrath.enabled" );
def->add_action( "ferocious_bite,if=buff.apex_predators_craving.up" );
def->add_action( "call_action_list,name=bloodtalons,if=variable.need_bt&!buff.bs_inc.up" );
def->add_action( "call_action_list,name=finisher,if=(combo_points>3&talent.lions_strength.enabled)|combo_points=5&!talent.lions_strength.enabled" );
def->add_action( "call_action_list,name=berserk_builders,if=combo_points<5&buff.bs_inc.up" );
def->add_action( "call_action_list,name=builder,if=combo_points<5" );

aoe->add_action( "pool_resource,for_next=1","AoE ---play with this some" );
aoe->add_action( "primal_wrath,if=combo_points>3" );
aoe->add_action( "ferocious_bite,if=buff.apex_predators_craving.up&(!buff.sabertooth.up|(!buff.bloodtalons.stack=1))" );
aoe->add_action( "run_action_list,name=bloodtalons_aoe,if=variable.need_bt&active_bt_triggers>=1" );
aoe->add_action( "pool_resource,for_next=1" );
aoe->add_action( "thrash_cat,target_if=refreshable" );
aoe->add_action( "brutal_slash","At this target count BRS also crushes everything except full thrashes" );
aoe->add_action( "pool_resource,for_next=1","This means that a full rake (5.5+ ticks) is stronger up to 10ish targets" );
aoe->add_action( "rake,target_if=max:dot.rake.ticks_gained_on_refresh.pmult,if=((dot.rake.ticks_gained_on_refresh.pmult*(1+talent.doubleclawed_rake.enabled))>(spell_targets.swipe_cat*0.216+3.32))" );
aoe->add_action( "lunar_inspiration,target_if=max:((ticks_gained_on_refresh+1)-(spell_targets.swipe_cat*2.492))","Full Lis beat Swipe up til around 3-ish targets depending on haste" );
aoe->add_action( "swipe_cat" );
aoe->add_action( "shred,if=action.shred.damage>action.thrash_cat.damage","If we have BrS and nothing better to cast, check if Thrash DD beats Shred" );
aoe->add_action( "thrash_cat" );

berserk_builders->add_action( "rake,target_if=refreshable" );
berserk_builders->add_action( "swipe_cat,if=spell_targets.swipe_cat>1" );
berserk_builders->add_action( "shred,if=active_bt_triggers=2&buff.bt_shred.down" );
berserk_builders->add_action( "brutal_slash,if=active_bt_triggers=2&buff.bt_brutal_slash.down" );
berserk_builders->add_action( "moonfire_cat,target_if=refreshable" );
berserk_builders->add_action( "shred" );

bloodtalons->add_action( "rake,target_if=max:druid.rake.ticks_gained_on_refresh,if=(refreshable|1.4*persistent_multiplier>dot.rake.pmultiplier)&buff.bt_rake.down" );
bloodtalons->add_action( "shred,if=buff.bt_shred.down&buff.clearcasting.up&spell_targets.swipe_cat=1" );
bloodtalons->add_action( "lunar_inspiration,if=refreshable&buff.bt_moonfire.down" );
bloodtalons->add_action( "shred,if=buff.bt_shred.down" );
bloodtalons->add_action( "thrash_cat,target_if=refreshable&buff.bt_thrash.down" );
bloodtalons->add_action( "brutal_slash,if=buff.bt_brutal_slash.down" );
bloodtalons->add_action( "swipe_cat,if=spell_targets.swipe_cat>1&buff.bt_swipe.down" );
bloodtalons->add_action( "swipe_cat,if=buff.bt_swipe.down" );
bloodtalons->add_action( "thrash_cat,if=buff.bt_thrash.down" );
bloodtalons->add_action( "rake,if=buff.bt_rake.down&combo_points>4" );

bloodtalons_aoe->add_action( "rake,target_if=max:druid.rake.ticks_gained_on_refresh,if=(refreshable|1.4*persistent_multiplier>dot.rake.pmultiplier)&buff.bt_rake.down" );
bloodtalons_aoe->add_action( "lunar_inspiration,if=refreshable&buff.bt_moonfire.down" );
bloodtalons_aoe->add_action( "shred,if=buff.bt_shred.down&buff.clearcasting.up&spell_targets.swipe_cat=1" );
bloodtalons_aoe->add_action( "brutal_slash,if=buff.bt_brutal_slash.down" );
bloodtalons_aoe->add_action( "thrash_cat,target_if=refreshable&buff.bt_thrash.down" );
bloodtalons_aoe->add_action( "swipe_cat,if=spell_targets.swipe_cat>1&buff.bt_swipe.down" );
bloodtalons_aoe->add_action( "shred,if=buff.bt_shred.down" );
bloodtalons_aoe->add_action( "swipe_cat,if=buff.bt_swipe.down" );
bloodtalons_aoe->add_action( "thrash_cat,if=buff.bt_thrash.down" );
bloodtalons_aoe->add_action( "rake,if=buff.bt_rake.down&combo_points>4" );

builder->add_action( "run_action_list,name=clearcasting,if=buff.clearcasting.react" );
builder->add_action( "rake,target_if=max:ticks_gained_on_refresh,if=refreshable|(buff.sudden_ambush.up&persistent_multiplier>dot.rake.pmultiplier&dot.rake.duration>6)" );
builder->add_action( "moonfire_cat,target_if=refreshable" );
builder->add_action( "pool_resource,for_next=1" );
builder->add_action( "thrash_cat,target_if=refreshable" );
builder->add_action( "brutal_slash" );
builder->add_action( "swipe_cat,if=spell_targets.swipe_cat>1" );
builder->add_action( "shred" );

clearcasting->add_action( "thrash_cat,if=refreshable" );
clearcasting->add_action( "swipe_cat,if=spell_targets.swipe_cat>1" );
clearcasting->add_action( "brutal_slash,if=spell_targets.brutal_slash>2&talent.moment_of_clarity.enabled" );
clearcasting->add_action( "shred" );

cooldown->add_action( "incarnation" );
cooldown->add_action( "berserk,if=!talent.convoke_the_spirits.enabled|talent.convoke_the_spirits.enabled&!(fight_remains<cooldown.convoke_the_spirits.remains)|talent.convoke_the_spirits.enabled&cooldown.convoke_the_spirits.remains<10|talent.convoke_the_spirits.enabled&fight_remains>120&cooldown.convoke_the_spirits.remains>25" );
cooldown->add_action( "potion,if=buff.bs_inc.up|fight_remains<32|buff.tigers_fury.up&cooldown.convoke_the_spirits.up&talent.convoke_the_spirits.enabled&fight_remains<cooldown.bs_inc.remains" );
cooldown->add_action( "convoke_the_spirits,if=dot.rip.duration>5&(buff.tigers_fury.up&combo_points<4&cooldown.bs_inc.remains>20|fight_remains<5)" );
cooldown->add_action( "berserking,if=buff.bs_inc.up|fight_remains<15" );
cooldown->add_action( "shadowmeld,if=buff.tigers_fury.up&buff.bs_inc.down&combo_points<4&buff.sudden_ambush.down&dot.rake.pmultiplier<1.6&energy>40&druid.rake.ticks_gained_on_refresh>spell_targets.swipe_cat*2-2&target.time_to_die>5" );
cooldown->add_action( "use_item,name=manic_grieftorch,if=energy.deficit>40" );
cooldown->add_action( "use_items" );

finisher->add_action( "primal_wrath,if=spell_targets.primal_wrath>2" );
finisher->add_action( "primal_wrath,target_if=refreshable,if=spell_targets.primal_wrath>1" );
finisher->add_action( "rip,target_if=refreshable" );
finisher->add_action( "pool_resource,for_next=1" );
finisher->add_action( "ferocious_bite,max_energy=1,if=!buff.bs_inc.up|(buff.bs_inc.up&!talent.soul_of_the_forest.enabled)" );
finisher->add_action( "ferocious_bite,if=(buff.bs_inc.up&talent.soul_of_the_forest.enabled)" );

owlweaving->add_action( "sunfire,line_cd=4*gcd" );
