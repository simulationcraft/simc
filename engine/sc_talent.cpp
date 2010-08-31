// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Talent
// ==========================================================================

// talent_t::talent_t =======================================================

talent_t::talent_t( player_t* p, unsigned talent_id, unsigned r ) :
	sim( p -> sim ), player ( p ), id ( talent_id ), rank ( r ), name_str( "" ),
    dependance ( 0 ), depend_rank ( 0 ),
    col ( 0 ), row ( 0 ), max_rank ( 0 ), spell_id ( 0 )
{}



// target_t::get_talent_data =============================================================

void talent_t::get_talent_data( unsigned talent_id )
{

	name_str 	= player -> player_data.talent_name_str ( talent_id );
	dependance	= player -> player_data.talent_depends_id ( talent_id );
	depend_rank	= player -> player_data.talent_depends_rank	( talent_id );
	col			= player -> player_data.talent_col ( talent_id );
	row			= player -> player_data.talent_row ( talent_id );
	max_rank	= player -> player_data.talent_max_rank ( talent_id );
	spell_id	= player -> player_data.talent_rank_spell_id ( talent_id, rank );


}

// talent_t::init ============================================================

void talent_t::init()
{
	if ( sim -> debug ) log_t::output( sim, "Initializing talent %s", name_str );
	get_talent_data ( id );
}


