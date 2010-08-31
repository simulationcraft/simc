// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Talent
// ==========================================================================

// talent_t::talent_t =======================================================

talent_t::talent_t( player_t* p, const char * name ) :
	sim( p -> sim ), player ( p ), id ( 0 ), rank ( 0 ), name_str( name ),
    dependance ( 0 ), depend_rank ( 0 ),
    col ( 0 ), row ( 0 ), max_rank ( 0 ), spell_id ( 0 )
{}


// talent_t::get_talent_id ============================================================

void talent_t::get_talent_id ()
{

 int i = 0;
    for ( unsigned j=0; j<3; j++)
    {
           while ( player -> player_data.talent_player_get_id_by_num( player -> type,j,i) != 0)
           {

        	   if ( !strcmp (player -> player_data.talent_name_str( player -> player_data.talent_player_get_id_by_num( player -> type, j, i ) ), name_str) )
                   {

                	   id = player -> player_data.talent_player_get_id_by_num( player -> type,j,i);
                   }
           i++;
           }
           i=0;
    }
}

// target_t::get_talent_data =============================================================

void talent_t::get_talent_data( unsigned talent_id )
{
	name_str 	= player -> player_data.talent_name_str ( talent_id );
	dependance	= player -> player_data.talent_depends_id ( talent_id );
	depend_rank	= player -> player_data.talent_depends_rank	( talent_id );
	col			= player -> player_data.talent_col ( talent_id );
	row			= player -> player_data.talent_row ( talent_id );
	max_rank	= player -> player_data.talent_max_rank ( talent_id );
	if (rank!=0) spell_id	= player -> player_data.talent_rank_spell_id ( talent_id, rank );


}



// talent_t::init ============================================================

void talent_t::init()
{
	if ( sim -> debug ) log_t::output( sim, "Initializing talent %s", name_str );

}


