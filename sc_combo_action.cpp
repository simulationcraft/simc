// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

bool Combo_Action_allowed=false;

// ==========================================================================
// Combo Action 
// ==========================================================================



	bool combo_action_t::isCombo(const std::string& combo_str){
		if (!Combo_Action_allowed) return false;
		return (combo_str.npos>6)&&(combo_str.substr(0,6)=="combo:")&&(combo_str.find_first_of('+')<combo_str.npos) ;
	}


	combo_action_t::combo_action_t( player_t* player, const std::string& combo_str, const std::string& options_str ) :
		action_t( ACTION_OTHER, "combo_action", player )
	{

		option_t caoptions[] =
		{
		  { "break", OPT_INT, &chkBreak },
		  { NULL }
		};
		//set parameters needed for combo action itself
		comboN=0;
		lastItem=0;
		chkBreak=false;		
		bool oldUO= player->allowUnknownOptions;
		player->allowUnknownOptions=true; //allow this for combo, since they all share options
		parse_options( caoptions, options_str );
		//set inherited values that may be important for behaviour of combo_action
		trigger_gcd = 0;
		// split name into each separate spell name and create child actions
		std::string cList= combo_str.substr(6,combo_str.npos-6);
		if (cList.empty() || (cList.npos<=0) ) return;
    	std::vector<std::string> splits;
		int size = util_t::string_split( splits, cList, "+" );
    	for( int i=0; i < size; i++ )  {
			action_t* act= action_t::create_action( player, splits[i], options_str );
			//if this is valid spell in list, add it to internal list
			if (act){ 
				if (comboN<maxCA){
					comboN++;
					combo_list[comboN]=act;
				}else{
					printf( "combo_action_t:: maximal number of actions (%d) exceeded on : %s\n", comboN, splits[ i ].c_str());
					assert( false );
				}
			}else{
				printf( "combo_action_t:: Unable to create action: %s with options %s\n", splits[ i ].c_str(), options_str.c_str() );
				assert( false );
			}
		}
		//clear some values
		player->action_list=0; // important, since action_t automatically adds to player->action_list 
		player->allowUnknownOptions= oldUO;
	}

	combo_action_t::~combo_action_t() {
		for (int i=1; i<=comboN; i++) delete combo_list[i];
	}

	void combo_action_t::schedule_execute(){
	}


	bool combo_action_t::ready()
	{
	    return false;
	}




