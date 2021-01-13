// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

class dbc_t;
struct spell_data_t;
struct xml_node_t;
struct talent_data_t;
struct item_set_bonus_t;
struct spelleffect_data_t;

#include <string>

// Spell information struct, holding static functions to output spell data in a human readable form

namespace spell_info
{
std::string to_str( const dbc_t& dbc, const spell_data_t* spell, int level );
void        to_xml( const dbc_t& dbc, const spell_data_t* spell, xml_node_t* parent, int level );
//static std::string to_str( sim_t* sim, uint32_t spell_id, int level = MAX_LEVEL );
std::string talent_to_str( const dbc_t& dbc, const talent_data_t* talent, int level );
std::string set_bonus_to_str( const dbc_t& dbc, const item_set_bonus_t* set_bonus, int level );
void        talent_to_xml( const dbc_t& dbc, const talent_data_t* talent, xml_node_t* parent, int level );
void set_bonus_to_xml( const dbc_t& dbc, const item_set_bonus_t* set_bonus, xml_node_t* parent, int level );
std::ostringstream& effect_to_str( const dbc_t& dbc, const spell_data_t* spell, const spelleffect_data_t* effect, std::ostringstream& s, int level );
void                effect_to_xml( const dbc_t& dbc, const spell_data_t* spell, const spelleffect_data_t* effect, xml_node_t*    parent, int level );
}
