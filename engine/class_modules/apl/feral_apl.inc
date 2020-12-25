{
action_priority_list_t* precombat = get_action_priority_list( "precombat" );
action_priority_list_t* def = get_action_priority_list( "default" );
action_priority_list_t* stealth = get_action_priority_list( "stealth" );
action_priority_list_t* bloodtalons = get_action_priority_list( "bloodtalons" );
action_priority_list_t* cooldown = get_action_priority_list( "cooldown" );
action_priority_list_t* finisher = get_action_priority_list( "finisher" );
action_priority_list_t* filler = get_action_priority_list( "filler" );


precombat->add_action( "flask" );
precombat->add_action( "food" );
precombat->add_action( "augmentation" );
precombat->add_action( "variable,name=4cp_bite,value=0" );
precombat->add_action( "variable,name=filler,value=0", "Shred = 0, Non-snapshot Rake = 1, Snapshot Rake = 2, LI = 3, Swipe = 4" );
precombat->add_action( "variable,name=rip_ticks,value=7" );
precombat->add_action( "variable,name=thrash_ticks,value=8" );
precombat->add_action( "variable,name=thrash_ticks,value=1,if=talent.bloodtalons.enabled|conduit.taste_for_blood.enabled" );
precombat->add_action( "variable,name=owlweave,value=druid.owlweave_cat" );
precombat->add_action( "snapshot_stats" );
precombat->add_action( "cat_form" );
precombat->add_action( "prowl" );

def->add_action( "starsurge,if=buff.heart_of_the_wild.up", "Balance weaving investigation" );
def->add_action( "sunfire,if=!prev_gcd.1.sunfire" );
def->add_action( "tigers_fury,if=buff.cat_form.down" );
def->add_action( "cat_form,if=buff.cat_form.down" );
def->add_action( "prowl", "Prowl whenever possible" );
def->add_action( "auto_attack,if=!buff.prowl.up&!buff.shadowmeld.up" );
def->add_action( "heart_of_the_wild,if=energy<40&dot.rake.remains>4.5&(dot.rip.remains>4.5|combo_points<5)&cooldown.tigers_fury.remains>=4.5&buff.clearcasting.stack<1&!buff.apex_predators_craving.up&!cooldown.convoke_the_spirits.up&variable.owlweave=1" );
def->add_action( "moonkin_form,if=energy<40&dot.rake.remains>4.5&(dot.rip.remains>4.5|combo_points<5)&cooldown.tigers_fury.remains>=4.5&buff.clearcasting.stack<1&!buff.apex_predators_craving.up&!cooldown.convoke_the_spirits.up&variable.owlweave=1" );
def->add_action( "variable,name=shortest_ttd,value=target.time_to_die,if=variable.shortest_ttd=0" );
def->add_action( "cycling_variable,name=shortest_ttd,op=min,value=target.time_to_die" );
def->add_action( "run_action_list,name=stealth,if=buff.shadowmeld.up|buff.prowl.up", "One shot stealth" );
def->add_action( "call_action_list,name=cooldown" );
def->add_action( "run_action_list,name=finisher,if=combo_points>=(5-variable.4cp_bite)" );
def->add_action( "call_action_list,name=stealth,if=buff.bs_inc.up|buff.sudden_ambush.up", "Multi-gcd stealth" );
def->add_action( "pool_resource,if=talent.bloodtalons.enabled&buff.bloodtalons.down&(energy+3.5*energy.regen+(40*buff.clearcasting.up))<(115-23*buff.incarnation_king_of_the_jungle.up)&active_bt_triggers=0", "The most expensive BT cycle (Thresh Shred Swipe cost 115 energy, make sure we can make that in 4 globals)" );
def->add_action( "run_action_list,name=bloodtalons,if=talent.bloodtalons.enabled&buff.bloodtalons.down" );
def->add_action( "ferocious_bite,target_if=max:target.time_to_die,if=buff.apex_predators_craving.up&(!talent.bloodtalons.enabled|buff.bloodtalons.up)" );
def->add_action( "feral_frenzy,if=combo_points<3" );
def->add_action( "pool_resource,for_next=1" );
def->add_action( "rake,target_if=(refreshable|persistent_multiplier>dot.rake.pmultiplier)&druid.rake.ticks_gained_on_refresh>spell_targets.swipe_cat*2-2" );
def->add_action( "moonfire_cat,target_if=refreshable&druid.moonfire.ticks_gained_on_refresh>spell_targets.swipe_cat*2-2" );
def->add_action( "pool_resource,for_next=1" );
def->add_action( "brutal_slash,if=(raid_event.adds.in>(1+max_charges-charges_fractional)*recharge_time)&(spell_targets.brutal_slash*action.brutal_slash.damage%action.brutal_slash.cost)>(action.shred.damage%action.shred.cost)" );
def->add_action( "swipe_cat,if=spell_targets.swipe_cat>1+buff.bs_inc.up*2" );
def->add_action( "shred,if=buff.clearcasting.up" );
def->add_action( "rake,target_if=buff.bs_inc.up&druid.rake.ticks_gained_on_refresh>2" );
def->add_action( "call_action_list,name=filler" );

stealth->add_action( "run_action_list,name=bloodtalons,if=talent.bloodtalons.enabled&buff.bloodtalons.down", "Rake needs roughly 50% of its length at a minimum to surpass shreds dpe" );
stealth->add_action( "pool_resource,for_next=1" );
stealth->add_action( "rake,target_if=(dot.rake.pmultiplier<1.5|refreshable)&druid.rake.ticks_gained_on_refresh>2" );
stealth->add_action( "brutal_slash,if=spell_targets.brutal_slash>2", "Brutal Slash is better than stealth Shred at 3 targets" );
stealth->add_action( "pool_resource,for_next=1" );
stealth->add_action( "shred,if=combo_points<4", "Shred for faster CP generation" );

bloodtalons->add_action( "rake,target_if=(!ticking|(refreshable&persistent_multiplier>dot.rake.pmultiplier))&buff.bt_rake.down&druid.rake.ticks_gained_on_refresh>=2" );
bloodtalons->add_action( "lunar_inspiration,target_if=refreshable&buff.bt_moonfire.down" );
bloodtalons->add_action( "thrash_cat,target_if=refreshable&buff.bt_thrash.down&druid.thrash_cat.ticks_gained_on_refresh>8" );
bloodtalons->add_action( "brutal_slash,if=buff.bt_brutal_slash.down" );
bloodtalons->add_action( "swipe_cat,if=buff.bt_swipe.down&spell_targets.swipe_cat>1" );
bloodtalons->add_action( "shred,if=buff.bt_shred.down" );
bloodtalons->add_action( "swipe_cat,if=buff.bt_swipe.down" );
bloodtalons->add_action( "thrash_cat,if=buff.bt_thrash.down" );

cooldown->add_action( "berserk,if=combo_points>=3", "TODO: Need to sync with with ravenous frenzy when we run frenzyband, or atleast check if its worth it" );
cooldown->add_action( "incarnation,if=combo_points>=3" );
cooldown->add_action( "tigers_fury,if=energy.deficit>55|buff.bs_inc.up|(talent.predator.enabled&variable.shortest_ttd<3)", "Try and not waste TF energy, but also just use it for zerk and incarns" );
cooldown->add_action( "shadowmeld,if=buff.tigers_fury.up&buff.bs_inc.down&combo_points<4&dot.rake.pmultiplier<1.6&energy>40", "Might be wrong to use Smeld on Rake in very very niche situations, but very rarely" );
cooldown->add_action( "berserking,if=buff.tigers_fury.up|buff.bs_inc.up" );
cooldown->add_action( "potion,if=buff.bs_inc.up" );
cooldown->add_action( "ravenous_frenzy,if=buff.bs_inc.up|fight_remains<21" );
cooldown->add_action( "convoke_the_spirits,if=(dot.rip.remains>4&combo_points<3&dot.rake.ticking)|fight_remains<5", "TODO: Check moonkin hotw and stuff like this, and maybe better combat remain resync" );
cooldown->add_action( "kindred_spirits,if=buff.tigers_fury.up|(conduit.deep_allegiance.enabled)", "Maybe there should be more conditions on the conduit" );
cooldown->add_action( "adaptive_swarm,target_if=max:time_to_die*(combo_points=5&!dot.adaptive_swarm_damage.ticking)" );
cooldown->add_action( "use_items" );

finisher->add_action( "savage_roar,if=buff.savage_roar.down|buff.savage_roar.remains<(combo_points*6+1)*0.3" );
finisher->add_action( "variable,name=best_rip,value=0,if=talent.primal_wrath.enabled", "Make sure to zero the variable so some old value don't end up lingering" );
finisher->add_action( "cycling_variable,name=best_rip,op=max,value=druid.rip.ticks_gained_on_refresh,if=talent.primal_wrath.enabled" );
finisher->add_action( "primal_wrath,if=druid.primal_wrath.ticks_gained_on_refresh>(variable.rip_ticks>?variable.best_rip)|spell_targets.primal_wrath>(3+1*talent.sabertooth.enabled)" );
finisher->add_action( "rip,target_if=refreshable&druid.rip.ticks_gained_on_refresh>variable.rip_ticks&((buff.tigers_fury.up|cooldown.tigers_fury.remains>5)&(buff.bloodtalons.up|!talent.bloodtalons.enabled)&dot.rip.pmultiplier<=persistent_multiplier|!talent.sabertooth.enabled)" );
finisher->add_action( "ferocious_bite,max_energy=1,target_if=max:time_to_die" );

filler->add_action( "rake,target_if=variable.filler=1&dot.rake.pmultiplier<=persistent_multiplier" );
filler->add_action( "rake,if=variable.filler=2" );
filler->add_action( "lunar_inspiration,if=variable.filler=3" );
filler->add_action( "swipe,if=variable.filler=4" );
filler->add_action( "shred" );
}
