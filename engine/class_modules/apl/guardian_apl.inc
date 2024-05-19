action_priority_list_t* precombat = get_action_priority_list( "precombat" );
action_priority_list_t* def = get_action_priority_list( "default" );

precombat->add_action( "bear_form" );

def->add_action( "use_items" );
def->add_action( "auto_attack" );
def->add_action( "incarnation" );
def->add_action( "berserk" );
def->add_action( "heart_of_the_wild" );
def->add_action( "natures_vigil" );
def->add_action( "convoke_the_spirits" );
def->add_action( "stampeding_roar" );
def->add_action( "growl" );
def->add_action( "furious_regeneration" );
def->add_action( "barkskin" );
def->add_action( "survival_instincts" );
def->add_action( "pulverize" );
def->add_action( "rage_of_the_sleeper" );
def->add_action( "lunar_beam" );
def->add_action( "bristling_fur" );
def->add_action( "ironfur,if=!buff.ironfur.up" );
def->add_action( "moonfire,target_if=refreshable" );
def->add_action( "maul" );
def->add_action( "mangle" );
def->add_action( "thrash_bear" );
def->add_action( "swipe_bear" );
