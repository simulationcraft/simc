action_priority_list_t* precombat = get_action_priority_list( "precombat" );
action_priority_list_t* def = get_action_priority_list( "default" );
action_priority_list_t* assisted_combat = get_action_priority_list( "assisted_combat" );
action_priority_list_t* cooldowns = get_action_priority_list( "cooldowns" );

precombat->add_action( "mark_of_the_wild,if=aura.mark_of_the_wild.down","Executed before combat begins. Accepts non-harmful actions only." );
precombat->add_action( "moonkin_form,if=buff.moonkin_form.down" );

def->add_action( "call_action_list,name=cooldowns","Executed every time the actor is available." );
def->add_action( "call_action_list,name=assisted_combat" );

assisted_combat->add_action( "sunfire,if=!dot.sunfire.ticking","This is the default action priority list from the game's Assisted Combat system." );
assisted_combat->add_action( "force_of_nature,if=buff.dream_burst.down","(Overridden from 'debuff.dream_surge.up')" );
assisted_combat->add_action( "moonfire,if=!dot.moonfire.ticking&buff.harmony_of_the_grove.down" );
assisted_combat->add_action( "celestial_alignment,if=buff.eclipse_solar.down&buff.eclipse_lunar.down","This is for Blizzard automation and is not included in the game's Assisted Combat system." );
assisted_combat->add_action( "starfire,if=buff.eclipse_solar.down&buff.eclipse_lunar.down&active_enemies>3&buff.lunar_eclipse_override.down&talent.eclipse" );
assisted_combat->add_action( "wrath,if=!talent.lunar_calling&buff.eclipse_solar.down&buff.eclipse_lunar.down&active_enemies<=2&buff.lunar_eclipse_override.up&talent.eclipse" );
assisted_combat->add_action( "fury_of_elune,if=buff.eclipse_solar.down&buff.eclipse_lunar.down&astral_power>=55" );
assisted_combat->add_action( "solar_eclipse,if=talent.eclipse&buff.eclipse_solar.down&buff.eclipse_lunar.down&astral_power>=60" );
assisted_combat->add_action( "starfall,if=active_enemies>3&astral_power>=90" );
assisted_combat->add_action( "starsurge,if=active_enemies<=2&astral_power>=90" );
assisted_combat->add_action( "wrath,if=buff.eclipse_solar.down&buff.eclipse_lunar.down&buff.lunar_eclipse_override.down&astral_power<=59.9" );
assisted_combat->add_action( "starfire,if=buff.eclipse_solar.down&buff.eclipse_lunar.down&buff.lunar_eclipse_override.up&astral_power<=59.9" );
assisted_combat->add_action( "starfall,if=buff.starweavers_warp.up" );
assisted_combat->add_action( "starsurge,if=buff.starweavers_weft.up" );
assisted_combat->add_action( "starfall,if=active_enemies>3" );
assisted_combat->add_action( "starsurge,if=active_enemies<=2" );
assisted_combat->add_action( "starsurge,if=astral_power>=50" );
assisted_combat->add_action( "convoke_the_spirits,if=buff.eclipse_solar.up" );
assisted_combat->add_action( "convoke_the_spirits,if=buff.eclipse_lunar.up" );
assisted_combat->add_action( "fury_of_elune" );
assisted_combat->add_action( "wild_mushroom,if=!dot.fungal_growth.ticking" );
assisted_combat->add_action( "new_moon" );
assisted_combat->add_action( "half_moon" );
assisted_combat->add_action( "full_moon" );
assisted_combat->add_action( "starfire,if=active_enemies>3,cooldown_allow_casting_success=1" );
assisted_combat->add_action( "starfire,if=talent.lunar_calling,cooldown_allow_casting_success=1" );
assisted_combat->add_action( "wrath,cooldown_allow_casting_success=1" );
assisted_combat->add_action( "moonfire" );

cooldowns->add_action( "use_items" );
cooldowns->add_action( "potion" );
cooldowns->add_action( "blood_fury" );
cooldowns->add_action( "berserking" );
cooldowns->add_action( "fireblood" );
cooldowns->add_action( "ancestral_call" );
