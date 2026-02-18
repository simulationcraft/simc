action_priority_list_t* precombat = get_action_priority_list( "precombat" );
action_priority_list_t* def = get_action_priority_list( "default" );
action_priority_list_t* assisted_combat = get_action_priority_list( "assisted_combat" );
action_priority_list_t* cooldowns = get_action_priority_list( "cooldowns" );

precombat->add_action( "mark_of_the_wild,if=aura.mark_of_the_wild.down","Executed before combat begins. Accepts non-harmful actions only." );
precombat->add_action( "cat_form,if=buff.cat_form.down" );
precombat->add_action( "prowl" );

def->add_action( "auto_attack,if=!buff.prowl.up","Executed every time the actor is available." );
def->add_action( "call_action_list,name=cooldowns" );
def->add_action( "call_action_list,name=assisted_combat" );

assisted_combat->add_action( "tigers_fury,if=combo_points>=5&target.distance<=8","This is the default action priority list from the game's Assisted Combat system." );
assisted_combat->add_action( "rake,if=buff.prowl.up" );
assisted_combat->add_action( "berserk","This is for Blizzard automation and is not included in the game's Assisted Combat system." );
assisted_combat->add_action( "chomp,if=buff.chomp_enabler.up" );
assisted_combat->add_action( "primal_wrath,if=active_enemies>3&!dot.rip.ticking&combo_points>=5&target.distance<=10" );
assisted_combat->add_action( "rip,if=!dot.rip.ticking&combo_points>=5" );
assisted_combat->add_action( "ferocious_bite,if=combo_points>=5" );
assisted_combat->add_action( "ferocious_bite,if=buff.apex_predators_craving.up","(Overridden from '(buff.apex_predators_craving.up|buff.unknown_spell_252752.up)')" );
assisted_combat->add_action( "feral_frenzy,if=combo_points<=1&buff.tigers_fury.up" );
assisted_combat->add_action( "feral_frenzy,if=combo_points<=1&!cooldown.tigers_fury.ready" );
assisted_combat->add_action( "frantic_frenzy,if=combo_points<=1&target.distance<=8&buff.tigers_fury.up" );
assisted_combat->add_action( "frantic_frenzy,if=combo_points<=1&target.distance<=8&!cooldown.tigers_fury.ready" );
assisted_combat->add_action( "convoke_the_spirits,if=target.distance<=5&combo_points<=3&buff.tigers_fury.up" );
assisted_combat->add_action( "rake,if=!dot.rake.ticking" );
assisted_combat->add_action( "moonfire,if=talent.lunar_inspiration&!dot.moonfire.ticking" );
assisted_combat->add_action( "swipe,if=target.distance<=8&active_enemies>3" );
assisted_combat->add_action( "shred" );
assisted_combat->add_action( "moonfire,if=talent.lunar_inspiration&target.distance>15" );
assisted_combat->add_action( "rake" );

cooldowns->add_action( "use_items" );
cooldowns->add_action( "potion" );
cooldowns->add_action( "blood_fury" );
cooldowns->add_action( "berserking" );
cooldowns->add_action( "fireblood" );
cooldowns->add_action( "ancestral_call" );
