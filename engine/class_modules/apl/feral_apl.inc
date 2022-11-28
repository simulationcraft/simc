action_priority_list_t* precombat = get_action_priority_list( "precombat" );
action_priority_list_t* clearcasting = get_action_priority_list( "clearcasting" );
action_priority_list_t* aoe = get_action_priority_list( "aoe" );
action_priority_list_t* builder = get_action_priority_list( "builder" );
action_priority_list_t* berserk_builders = get_action_priority_list( "berserk_builders" );
action_priority_list_t* finisher = get_action_priority_list( "finisher" );
action_priority_list_t* cooldown = get_action_priority_list( "cooldown" );
action_priority_list_t* owlweaving = get_action_priority_list( "owlweaving" );
action_priority_list_t* bloodtalons = get_action_priority_list( "bloodtalons" );

precombat->add_action( "cat_form" );
precombat->add_action( "prowl" );

clearcasting->add_action( "thrash_cat,if=refreshable" );
clearcasting->add_action( "swipe_cat,if=spell_targets.swipe_cat>2" );
clearcasting->add_action( "brutal_slash,if=spell_targets.brutal_slash>5&talent.moment_of_clarity.enabled" );
clearcasting->add_action( "shred" );

aoe->add_action( "pool_resource,for_next=1","AoE" );
aoe->add_action( "primal_wrath,if=combo_points=5" );
aoe->add_action( "ferocious_bite,if=buff.apex_predators_craving.up&buff.sabertooth.down" );
aoe->add_action( "run_action_list,name=bloodtalons,if=variable.need_bt&active_bt_triggers>=1" );
aoe->add_action( "pool_resource,for_next=1" );
aoe->add_action( "thrash_cat,target_if=refreshable" );
aoe->add_action( "brutal_slash","At this target count BRS also crushes everything except full thrashes" );
aoe->add_action( "pool_resource,for_next=1","This means that a full rake (5.5+ ticks) is stronger up to 10ish targets" );
aoe->add_action( "rake,target_if=max:((dot.rake.ticks_gained_on_refresh.pmult*(1+talent.doubleclawed_rake.enabled))-(spell_targets.swipe_cat*0.216+3.32))" );
aoe->add_action( "lunar_inspiration,target_if=max:((ticks_gained_on_refresh+1)-(spell_targets.swipe_cat*2.492))","Full Lis beat Swipe up til around 3-ish targets depending on haste" );
aoe->add_action( "swipe_cat" );
aoe->add_action( "shred,if=action.shred.damage>action.thrash_cat.damage","If we have BrS and nothing better to cast, check if Thrash DD beats Shred" );
aoe->add_action( "thrash_cat" );

builder->add_action( "run_action_list,name=clearcasting,if=buff.clearcasting.react" );
builder->add_action( "rake,target_if=max:ticks_gained_on_refresh,if=refreshable|(buff.sudden_ambush.up&persistent_multiplier>dot.rake.pmultiplier&dot.rake.duration>6)" );
builder->add_action( "moonfire_cat,target_if=refreshable" );
builder->add_action( "pool_resource,for_next=1" );
builder->add_action( "thrash_cat,target_if=refreshable" );
builder->add_action( "brutal_slash" );
builder->add_action( "swipe_cat,if=spell_targets.swipe_cat>1" );
builder->add_action( "shred" );

berserk_builders->add_action( "rake,target_if=refreshable" );
berserk_builders->add_action( "swipe_cat,if=spell_targets.swipe_cat>1" );
berserk_builders->add_action( "brutal_slash,if=active_bt_triggers=2&buff.bt_brutal_slash.down" );
berserk_builders->add_action( "moonfire_cat,target_if=refreshable" );
berserk_builders->add_action( "shred" );

finisher->add_action( "primal_wrath,if=spell_targets.primal_wrath>2" );
finisher->add_action( "primal_wrath,target_if=refreshable,if=spell_targets.primal_wrath>1" );
finisher->add_action( "rip,target_if=refreshable" );
finisher->add_action( "pool_resource,for_next=1" );
finisher->add_action( "ferocious_bite,max_energy=1,if=!buff.bs_inc.up|(buff.bs_inc.up&!talent.soul_of_the_forest.enabled)" );
finisher->add_action( "ferocious_bite,if=(buff.bs_inc.up&talent.soul_of_the_forest.enabled)" );

cooldown->add_action( "berserk" );
cooldown->add_action( "incarnation" );
cooldown->add_action( "convoke_the_spirits,if=buff.tigers_fury.up&combo_points<3|fight_remains<5" );
cooldown->add_action( "berserking" );
cooldown->add_action( "shadowmeld,if=buff.tigers_fury.up&buff.bs_inc.down&combo_points<4&buff.sudden_ambush.down&dot.rake.pmultiplier<1.6&energy>40&druid.rake.ticks_gained_on_refresh>spell_targets.swipe_cat*2-2&target.time_to_die>5" );
cooldown->add_action( "potion,if=buff.bs_inc.up|fight_remains<cooldown.bs_inc.remains|fight_remains<35" );
cooldown->add_action( "use_items" );

owlweaving->add_action( "sunfire,line_cd=4*gcd" );

bloodtalons->add_action( "rake,target_if=max:druid.rake.ticks_gained_on_refresh,if=(refreshable|1.4*persistent_multiplier>dot.rake.pmultiplier)&buff.bt_rake.down" );
bloodtalons->add_action( "lunar_inspiration,if=refreshable&buff.bt_moonfire.down" );
bloodtalons->add_action( "brutal_slash,if=buff.bt_brutal_slash.down" );
bloodtalons->add_action( "thrash_cat,target_if=refreshable&buff.bt_thrash.down" );
bloodtalons->add_action( "shred,if=buff.bt_shred.down" );
bloodtalons->add_action( "swipe_cat,if=buff.bt_swipe.down" );
bloodtalons->add_action( "swipe_cat,if=buff.bt_swipe.down" );
bloodtalons->add_action( "thrash_cat,if=buff.bt_thrash.down" );
bloodtalons->add_action( "rake,if=buff.bt_rake.down&combo_points>4" );
