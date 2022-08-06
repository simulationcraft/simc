// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "config.hpp"
#include "simulationcraft.hpp"

namespace
{  // UNNAMED NAMESPACE
// ==========================================================================
// Evoker
// ==========================================================================

struct evoker_t : public player_t
{
  evoker_t( sim_t * sim, std::string_view name, race_e r = RACE_DRACTHYR_ALLIANCE ) : player_t( sim, EVOKER, name, r )
  {

  }

  void init_base_stats() override;

  action_t* create_action( std::string_view name, std::string_view options ) override;
};

// Template for base evoker action code.
template <class Base>
struct evoker_action_t : public Base
{
private:
  using ab = Base;  // action base, spell_t/heal_t/etc.

public:
  evoker_action_t( std::string_view name, evoker_t* player, const spell_data_t* s_data = spell_data_t::nil() )
    : ab( name, player, s_data )
  {

  }

  evoker_t* p()
  {
    return static_cast<evoker_t*>( ab::player );
  }

  const evoker_t* p() const
  {
    return static_cast<evoker_t*>( ab::player );
  }
};

namespace spells
{
struct evoker_spell_t : public evoker_action_t<spell_t>
{
private:
  using ab = evoker_action_t<spell_t>;

public:
  evoker_spell_t( std::string_view name, evoker_t* player, const spell_data_t* s_data = spell_data_t::nil(),
                  std::string_view options_str = {} )
    : ab( name, player, s_data )
  {
    parse_options( options_str );
  }
};

struct disintegrate_t : public evoker_spell_t
{
  disintegrate_t( evoker_t* p, std::string_view options_str )
    : evoker_spell_t( "disintegrate", p, p->find_class_spell( "Disintegrate" ) )
  {
    channeled = true;
  }
};
}  // end namespace spells

// Initialize Base Stats ====================================================
void evoker_t::init_base_stats()
{
  player_t::init_base_stats();

  base.spell_power_per_intellect = 1.0;
}

// Create Action ============================================================
action_t* evoker_t::create_action( std::string_view name, std::string_view options_str )
{
  using namespace spells;

  if ( name == "disintegrate" ) return new disintegrate_t( this, options_str );

  return player_t::create_action( name, options_str );
}


/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class evoker_report_t : public player_report_extension_t
{
public:
  evoker_report_t( evoker_t& player ) : p( player ) {}

  void html_customsection( report::sc_html_stream& os ) override {}

private:
  evoker_t& p;
};

// EVOKER MODULE INTERFACE ==================================================
struct evoker_module_t : public module_t
{
  evoker_module_t() : module_t( EVOKER ) {}

  player_t* create_player( sim_t* sim, std::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p = new evoker_t( sim, name, r );
    p->report_extension = std::make_unique<evoker_report_t>( *p );
    return p;
  }
  bool valid() const override { return true; }
  void init( player_t* p ) const override {}
  void static_init() const override {}
  void register_hotfixes() const override {}
  void combat_begin( sim_t* ) const override {}
  void combat_end( sim_t* ) const override {}
};
}

const module_t* module_t::evoker()
{
  static evoker_module_t m;
  return &m;
}
