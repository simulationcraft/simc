action_priority_list_t* precombat = get_action_priority_list( "precombat" );
action_priority_list_t* def = get_action_priority_list( "default" );

precombat->add_action( "cat_form" );
precombat->add_action( "prowl" );

def->add_action( "use_items" );
def->add_action( "auto_attack" );
def->add_action( "incarnation" );
def->add_action( "berserk" );
def->add_action( "convoke_the_spirits" );
def->add_action( "tigers_fury" );
def->add_action( "feral_frenzy" );
def->add_action( "heart_of_the_wild" );
def->add_action( "adaptive_swarm" );
def->add_action( "rip,target_if=refreshable,if=combo_points=5" );
def->add_action( "ferocious_bite,if=combo_points=5&energy>=50" );
def->add_action( "thrash_cat,target_if=refreshable" );
def->add_action( "rake,target_if=refreshable" );
def->add_action( "lunar_inspiration,target_if=refreshable" );
def->add_action( "brutal_slash" );
def->add_action( "shred" );
