action_priority_list_t* precombat = get_action_priority_list( "precombat" );
action_priority_list_t* def = get_action_priority_list( "default" );
action_priority_list_t* assisted_combat = get_action_priority_list( "assisted_combat" );
action_priority_list_t* cooldowns = get_action_priority_list( "cooldowns" );

precombat->add_action( "mark_of_the_wild,if=aura.mark_of_the_wild.down","Executed before combat begins. Accepts non-harmful actions only." );

def->add_action( "call_action_list,name=cooldowns","Executed every time the actor is available." );
def->add_action( "call_action_list,name=assisted_combat" );

assisted_combat->add_action( "rip,if=buff.cat_form.up&combo_points>=5&!dot.rip.ticking","This is the default action priority list from the game's Assisted Combat system." );
assisted_combat->add_action( "ferocious_bite,if=buff.cat_form.up&combo_points>=5" );
assisted_combat->add_action( "thrash,if=target.distance<=5&buff.cat_form.up&!dot.thrash.ticking&active_enemies>3" );
assisted_combat->add_action( "swipe,if=target.distance<=5&buff.cat_form.up&active_enemies>3" );
assisted_combat->add_action( "rake,if=buff.cat_form.up&!dot.rake.ticking" );
assisted_combat->add_action( "shred,if=buff.cat_form.up" );
assisted_combat->add_action( "sunfire,if=!dot.sunfire.ticking&buff.cat_form.down" );
assisted_combat->add_action( "moonfire,if=!dot.moonfire.ticking&buff.cat_form.down" );
assisted_combat->add_action( "starsurge,if=buff.cat_form.down" );
assisted_combat->add_action( "starfire,if=active_enemies>3&buff.cat_form.down,cooldown_allow_casting_success=1" );
assisted_combat->add_action( "wrath,if=buff.cat_form.down,cooldown_allow_casting_success=1" );
assisted_combat->add_action( "moonfire" );

cooldowns->add_action( "use_items" );
cooldowns->add_action( "potion" );
cooldowns->add_action( "blood_fury" );
cooldowns->add_action( "berserking" );
cooldowns->add_action( "fireblood" );
cooldowns->add_action( "ancestral_call" );
