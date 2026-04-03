action_priority_list_t* precombat = get_action_priority_list( "precombat" );
action_priority_list_t* def = get_action_priority_list( "default" );
action_priority_list_t* assisted_combat = get_action_priority_list( "assisted_combat" );
action_priority_list_t* cooldowns = get_action_priority_list( "cooldowns" );

precombat->add_action( "bear_form,if=buff.bear_form.down","Executed before combat begins. Accepts non-harmful actions only." );

def->add_action( "auto_attack","Executed every time the actor is available." );
def->add_action( "call_action_list,name=cooldowns" );
def->add_action( "call_action_list,name=assisted_combat" );

assisted_combat->add_action( "berserk","This is for Blizzard automation and is not included in the game's Assisted Combat system." );
assisted_combat->add_action( "convoke_the_spirits","This is for Blizzard automation and is not included in the game's Assisted Combat system." );
assisted_combat->add_action( "thrash,if=target.distance<=8&active_enemies>3" );
assisted_combat->add_action( "lunar_beam,if=target.distance<=8" );
assisted_combat->add_action( "convoke_the_spirits,if=target.distance<=5" );
assisted_combat->add_action( "moonfire,if=!dot.moonfire.ticking" );
assisted_combat->add_action( "mangle" );
assisted_combat->add_action( "thrash,if=target.distance<=8" );
assisted_combat->add_action( "moonfire,if=buff.galactic_guardian.up" );
assisted_combat->add_action( "maul,if=rage>=8" );
assisted_combat->add_action( "swipe,if=target.distance<=8" );
assisted_combat->add_action( "moonfire,if=target.distance>10" );
assisted_combat->add_action( "swipe" );

cooldowns->add_action( "use_items","Some cooldowns are not used by the Assisted Combat system. This simple default cooldown usage has been provided by simc." );
cooldowns->add_action( "potion" );
cooldowns->add_action( "blood_fury" );
cooldowns->add_action( "berserking" );
cooldowns->add_action( "fireblood" );
cooldowns->add_action( "ancestral_call" );
