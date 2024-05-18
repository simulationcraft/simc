action_priority_list_t* precombat = get_action_priority_list( "precombat" );
action_priority_list_t* def = get_action_priority_list( "default" );

precombat->add_action( "moonkin_form" );

def->add_action( "use_items" );
def->add_action( "auto_attack" );
def->add_action( "incarnation" );
def->add_action( "celestial_alignment" );
def->add_action( "starfire,if=!talent.lunar_calling&!buff.eclipse_solar.up" );
def->add_action( "moonfire,target_if=refreshable" );
def->add_action( "sunfire,target_if=refreshable" );
def->add_action( "stellar_flare,target_if=refreshable" );
def->add_action( "starfall,if=buff.starweavers_warp.up" );
def->add_action( "starsurge" );
def->add_action( "convoke_the_spirits" );
def->add_action( "force_of_nature" );
def->add_action( "fury_of_elune" );
def->add_action( "new_moon" );
def->add_action( "half_moon" );
def->add_action( "full_moon" );
def->add_action( "warrior_of_elune" );
def->add_action( "wild_mushroom" );
def->add_action( "starfire,if=talent.lunar_calling" );
def->add_action( "wrath" );
