// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "parse_effects.hpp"

#include "dbc/dbc.hpp"
#include "dbc/sc_spell_info.hpp"
#include "player/stats.hpp"
#include "report/decorators.hpp"
#include "sim/cooldown.hpp"
#include "sim/sim.hpp"
#include "util/parse_util.hpp"
#include "action/action_state.hpp"

namespace opt_strings
{
std::string attributes( uint32_t opt )
{
  std::vector<std::string> str_list;

  for ( auto stat : { STAT_STRENGTH, STAT_AGILITY, STAT_STAMINA, STAT_INTELLECT, STAT_SPIRIT } )
    if ( opt & ( 1 << ( stat - 1 ) ) )
      str_list.emplace_back( util::stat_type_string( stat ) );

  return util::string_join( str_list );
}

std::string attributes_invalidate( const player_effect_t& data )
{
  std::vector<std::string> str_list;

  for ( auto stat : { STAT_STRENGTH, STAT_AGILITY, STAT_STAMINA, STAT_INTELLECT, STAT_SPIRIT } )
  {
    if ( data.opt_enum & ( 1 << ( stat - 1 ) ) )
    {
      str_list.emplace_back( util::stat_type_string( stat ) );
      if ( data.buff )
        data.buff->add_invalidate( cache_from_stat( stat ) );
    }
  }

  return util::string_join( str_list );
}

std::string ratings( uint32_t opt )
{
  std::vector<std::string> str_list;

  for ( auto stat : util::translate_all_rating_mod( opt ) )
    str_list.emplace_back( util::stat_type_string( stat ) );

  return util::string_join( str_list );
}

std::string ratings_invalidate( const player_effect_t& data )
{
  std::vector<std::string> str_list;

  for ( auto stat : util::translate_all_rating_mod( data.opt_enum ) )
  {
    str_list.emplace_back( util::stat_type_string( stat ) );
    if ( data.buff )
      data.buff->add_invalidate( cache_from_stat( stat ) );
  }

  return util::string_join( str_list );
}

std::string school( uint32_t opt )
{
  return opt == 0x7f ? "all" : util::school_type_string( dbc::get_school_type( opt ) );
}

std::string pet_type( uint32_t opt )
{
  return opt ? "Guardian" : "Pet";
}

std::string cooldown_type( uint32_t opt )
{
  return opt == 1 ? "Category" : "";
}

std::string parse_cb_str( parse_callback_e type )
{
  switch ( type )
  {
    case PARSE_CALLBACK_POST_SNAPSHOT: return "post-snapshot";
    case PARSE_CALLBACK_POST_IMPACT:   return "post-impact";
    case PARSE_CALLBACK_POST_EXECUTE:  return "post-execute";
    default:                           return "unknown";
  }
}
}  // namespace opt_strings

player_effect_t& player_effect_t::add_parse_callback( parse_effects_t* base, parse_callback_e cb_type, parse_cb_t fn )
{
  uint32_t mask = 1U << ( base->callback_list[ cb_type ].size() + 8 * static_cast<unsigned>( cb_type ) );
  idx |= mask;
  base->callback_mask[ cb_type ] |= mask;
  base->callback_list[ cb_type ].push_back( std::move( fn ) );
  simple = false;
  return *this;
}

player_effect_t& player_effect_t::print_debug( sim_t* sim, std::string prefix )
{
  std::string val_str;

  if ( buff && type & USE_CURRENT )
    val_str = "current value";
  else if ( mastery )
    val_str = fmt::format( "{:.5f}*mastery", value * 100 );
  else
    val_str = fmt::format( "{}", value );

  if ( buff && type & USE_DEFAULT )
    val_str = val_str + " (default value)";
  else if ( value_func )
    val_str = val_str + " (value function)";

  std::string mod_str;

  if ( buff && use_stacks && idx )
    mod_str = "with callback per stack of";
  else if ( buff && use_stacks )
    mod_str = "per stack of";
  else if ( buff && idx )
    mod_str = "with callback from";
  else if ( func )
    mod_str = "with condition from";
  else
    mod_str = "from";

  sim->print_debug( "{} manually modified by {} {} {}", prefix, val_str, mod_str, *eff );

  return *this;
}

player_effect_t& player_effect_t::print_debug( action_t* action )
{
  return print_debug( action->sim, fmt::format( "action-effects: {}", *action ) );
}

player_effect_t& player_effect_t::print_debug( player_t* player )
{
  return print_debug( player->sim, fmt::format( "player-effects: {}", *player ) );
}

std::string player_effect_t::value_type_name( uint16_t t ) const
{
  if ( t & VALUE_OVERRIDE )
    return "Value Override";
  else if ( t & USE_CURRENT )
    return "Current Value";
  else if ( t & USE_DEFAULT )
    return "Default Value";
  else
    return "Spell Data";
}

void player_effect_t::print_parsed_line( report::sc_html_stream& os, const sim_t& sim, bool decorate,
                                         const std::function<std::string( uint32_t )>& note_fn,
                                         const std::function<std::string( double )>& val_str_fn ) const
{
  std::vector<std::string> notes;

  if ( !use_stacks )
    notes.emplace_back( "No-stacks" );

  if ( mastery )
    notes.emplace_back( "Mastery" );

  if ( func )
    notes.emplace_back( "Conditional" );

  if ( !buff && !func )
    notes.emplace_back( "Passive" );

  if ( idx )
    notes.emplace_back( "Callback" );

  if ( type & AFFECTED_OVERRIDE )
    notes.emplace_back( "Value-override" );

  if ( type & MANUAL_ENTRY )
    notes.emplace_back( "Manual-entry" );

  if ( value_func )
    notes.emplace_back( "Value-function" );

  if ( note_fn )
  {
    if ( auto str = note_fn( opt_enum ); !str.empty() )
      notes.emplace_back( str );
  }

  if ( !note.empty() )
    notes.emplace_back( util::encode_html( note ) );

  range::for_each( notes, []( auto& s ) { s[ 0 ] = std::toupper( s[ 0 ] ); } );

  std::string val_str = val_str_fn ? val_str_fn( value )
                        : mastery  ? fmt::format( "{:.5f}", value * 100 )
                                   : fmt::format( "{:.1f}%", value * 100 );

  os.format(
    "<td class=\"left\">{}</td>"
    "<td class=\"right\">{}</td>"
    "<td>#{}</td>"
    "<td class=\"right\">{}</td>"
    "<td>{}</td>"
    "<td>{}</td>"
    "</tr>\n",
    decorate ? report_decorators::decorated_spell_data( sim, eff->spell() ) : eff->spell()->name_cstr(),
    eff->spell()->id(),
    eff->index() + 1,
    val_str,
    value_type_name( type ),
    util::string_join( notes ) );
}

std::string target_effect_t::value_type_name( uint16_t t ) const
{
  if ( t & VALUE_OVERRIDE )
    return "Value Override";
  else
    return "";
}

void target_effect_t::print_parsed_line( report::sc_html_stream& os, const sim_t& sim, bool decorate,
                                         const std::function<std::string( uint32_t )>& note_fn,
                                         const std::function<std::string( double )>& val_str_fn ) const
{
  std::vector<std::string> notes;

  if ( mastery )
    notes.emplace_back( "Mastery" );

  if ( type & AFFECTED_OVERRIDE )
    notes.emplace_back( "Scripted" );

  if ( type & MANUAL_ENTRY )
    notes.emplace_back( "Manual" );

  if ( note_fn )
  {
    if ( auto str = note_fn( opt_enum ); !str.empty() )
      notes.emplace_back( str );
  }

  if ( !note.empty() )
    notes.emplace_back( util::encode_html( note ) );

  range::for_each( notes, []( auto& s ) { s[ 0 ] = std::toupper( s[ 0 ] ); } );

  std::string val_str = val_str_fn ? val_str_fn( value )
                        : mastery  ? fmt::format( "{:.5f}", value * 100 )
                                   : fmt::format( "{:.1f}%", value * 100 );

  os.format(
    "<td class=\"left\">{}</td>"
    "<td class=\"right\">{}</td>"
    "<td>#{}</td>"
    "<td class=\"right\">{}</td>"
    "<td>{}</td>"
    "<td>{}</td></tr>\n",
    decorate ? report_decorators::decorated_spell_data( sim, eff->spell() ) : eff->spell()->name_cstr(),
    eff->spell()->id(),
    eff->index() + 1,
    val_str,
    value_type_name( type ),
    util::string_join( notes ) );
}

// adjust value based on effect modifying effects from mod list.
// currently supports P_EFFECT_1-5 and A_PROC_TRIGGER_SPELL_WITH_VALUE
// TODO: add support for P_EFFECTS to modify all effects
template <typename T>
void parse_base_t::apply_affecting_mod( double& val, bool& mastery, const spell_data_t* base, size_t idx, T mod )
{
  bool mod_is_mastery = false;

  if ( mod->effect_count() && mod->flags( SX_MASTERY_AFFECTS_POINTS ) )
  {
    mastery = true;
    mod_is_mastery = true;
  }

  for ( size_t i = 1; i <= mod->effect_count(); i++ )
  {
    const auto& eff = mod->effectN( i );

    if ( eff.type() != E_APPLY_AURA )
      continue;

    if ( ( base->affected_by_all( eff ) &&
           ( ( eff.misc_value1() == P_EFFECT_1 && idx == 1 ) || ( eff.misc_value1() == P_EFFECT_2 && idx == 2 ) ||
             ( eff.misc_value1() == P_EFFECT_3 && idx == 3 ) || ( eff.misc_value1() == P_EFFECT_4 && idx == 4 ) ||
             ( eff.misc_value1() == P_EFFECT_5 && idx == 5 ) ) ) ||
         ( eff.subtype() == A_PROC_TRIGGER_SPELL_WITH_VALUE && eff.trigger_spell_id() == base->id() ) )
    {
      double pct = mod_is_mastery ? eff.mastery_value() : mod_spell_effects_value( mod, eff );

      if ( eff.subtype() == A_ADD_FLAT_MODIFIER || eff.subtype() == A_ADD_FLAT_LABEL_MODIFIER )
        val += pct;
      else if ( eff.subtype() == A_ADD_PCT_MODIFIER || eff.subtype() == A_ADD_PCT_LABEL_MODIFIER )
        val *= 1.0 + pct / 100;
      else if ( eff.subtype() == A_PROC_TRIGGER_SPELL_WITH_VALUE )
        val = pct;
    }
  }
}

// explicit template instantiation
// NOTE: currently only spell_data_t is required, but this can be expanded as has been in the past with conduits
template void parse_base_t::apply_affecting_mod<const spell_data_t*>( double&, bool&, const spell_data_t*, size_t,
                                                                      const spell_data_t* );

double modified_spelleffect_t::base_value( const action_t* action, const action_state_t* state ) const
{
  auto return_value = value;

  for ( const auto& i : conditional )
  {
    double eff_val = i.value;

    if ( i.func && !i.func( action, state ) )
      continue;  // continue to next effect if conditional effect function is false

    if ( i.buff )
    {
      auto stack = i.buff->check();

      if ( !stack )
        continue;  // continue to next effect if stacks == 0 (buff is down)

      if ( i.use_stacks )
        eff_val *= stack;
    }

    if ( i.flat )
      return_value += eff_val;
    else
      return_value *= 1.0 + eff_val;
  }

  return return_value;
}

bool modified_spelleffect_t::modified_by( const spelleffect_data_t& eff ) const
{
  return !eff.ok() || range::contains( permanent, &eff ) || range::contains( conditional, &eff, &modify_effect_t::eff );
}

void modified_spelleffect_t::print_parsed_line( report::sc_html_stream& os, const sim_t& sim,
                                                const modify_effect_t& i ) const
{
  std::string op_str = i.flat ? "ADD" : "PCT";
  std::string val_str = i.flat ? fmt::to_string( i.value ) : ( fmt::to_string( i.value * 100 ) + '%' );
  std::vector<std::string> notes;

  if ( !i.use_stacks )
    notes.emplace_back( "No-stacks" );

  if ( i.buff )
    notes.emplace_back( "w/ Buff" );

  if ( i.func )
    notes.emplace_back( "Conditional" );

  if ( !i.note.empty() )
    notes.emplace_back( util::encode_html( i.note ) );

  os.format(
    "<td>{}</td>"
    "<td class=\"right\">{}</td>"
    "<td>#{}</td>"
    "<td>{}</td>"
    "<td class=\"right\">{}</td>"
    "<td>{}</td></tr>\n",
    report_decorators::decorated_spell_data( sim, i.eff->spell() ),
    i.eff->spell()->id(),
    i.eff->index() + 1,
    op_str,
    val_str,
    util::string_join( notes ) );
}

void modified_spelleffect_t::print_parsed_line( report::sc_html_stream& os, const sim_t& sim,
                                                const spelleffect_data_t& eff ) const
{
  std::string op_str;
  std::string val_str;

  switch ( eff.subtype() )
  {
    case A_ADD_PCT_MODIFIER:
    case A_ADD_PCT_LABEL_MODIFIER:
      op_str = "PCT";
      val_str = fmt::to_string( eff.base_value() * 100 ) + '%';
      break;
    case A_ADD_FLAT_MODIFIER:
    case A_ADD_FLAT_LABEL_MODIFIER:
      op_str = "ADD";
      val_str = fmt::to_string( eff.base_value() );
      break;
    default:
      op_str = "UNK";
      break;
  }

  os.format(
    "<td>{}</td>"
    "<td class=\"right\">{}</td>"
    "<td>#{}</td>"
    "<td>{}</td>"
    "<td class=\"right\">{}</td>"
    "<td>Passive</td></tr>\n",
    report_decorators::decorated_spell_data( sim, eff.spell() ),
    eff.spell()->id(),
    eff.index() + 1,
    op_str,
    val_str );
}

void modified_spelleffect_t::print_parsed_effect( report::sc_html_stream& os, const sim_t& sim, size_t& row ) const
{
  auto c = size();
  if ( !c )
    return;

  if ( row != 0 )
    os << "<tr>";

  size_t row2 = 0;
  std::string eff_str;

  switch ( _eff.type() )
  {
    case E_APPLY_AURA:
    case E_APPLY_AURA_PET:
    case E_APPLY_AREA_AURA_PARTY:
    case E_APPLY_AREA_AURA_PET:
      switch ( _eff.subtype() )
      {
        case A_ADD_FLAT_MODIFIER:
        case A_ADD_FLAT_LABEL_MODIFIER:
          eff_str = fmt::format( "Flat {}", spell_info::effect_property_str( &_eff ) );
          break;
        case A_ADD_PCT_MODIFIER:
        case A_ADD_PCT_LABEL_MODIFIER:
          eff_str = fmt::format( "Percent {}", spell_info::effect_property_str( &_eff ) );
          break;
        default:
          eff_str = spell_info::effect_subtype_str( &_eff );
          break;
      }
      break;
    default:
      eff_str = spell_info::effect_type_str( &_eff );
      break;
  }

  os.format(
    "<td rowspan=\"{}\" class=\"dark\">#{}</td>"
    "<td rowspan=\"{}\" class=\"dark\">{}</td>\n",
    c, _eff.index() + 1,
    c, eff_str );

  for ( auto eff : permanent )
  {
    if ( row != 0 && row2 != 0 )
      os << "<tr>";

    print_parsed_line( os, sim, *eff );
    row++;
    row2++;
  }

  for ( const auto& eff : conditional )
  {
    if ( row != 0 && row2 != 0 )
      os << "<tr>";

    print_parsed_line( os, sim, eff );
    row++;
    row2++;
  }
}

const modified_spelleffect_t& modified_spell_data_t::effectN( size_t idx ) const
{
  assert( idx > 0 && "effect index must not be zero or less" );

  if ( idx > effects.size() )
    return modified_spelleffect_t::nil();

  return effects[ idx - 1 ];
}

void modified_spell_data_t::parse_effect( const pack_t<modify_effect_t>& pack, size_t i )
{
  const auto& eff = pack.spell->effectN( i );
  bool m;  // dummy throwaway
  double val = eff.base_value();
  double val_mul = 0.01;
  int idx = -1;
  bool flat = false;

  if ( eff.type() != E_APPLY_AURA )
    return;

  switch ( eff.subtype() )
  {
    case A_ADD_FLAT_MODIFIER:
    case A_ADD_FLAT_LABEL_MODIFIER:
      val_mul = 1.0;
      flat = true;
      break;
    case A_ADD_PCT_MODIFIER:
    case A_ADD_PCT_LABEL_MODIFIER:
      break;
    default:
      return;
  }

  switch ( eff.property_type() )
  {
    case P_EFFECT_1: idx = 0; break;
    case P_EFFECT_2: idx = 1; break;
    case P_EFFECT_3: idx = 2; break;
    case P_EFFECT_4: idx = 3; break;
    case P_EFFECT_5: idx = 4; break;
    default:         return;
  }

  if ( !_spell.affected_by_all( eff ) || idx >= as<int>( effects.size() ) )
    return;

  auto& effN = effects[ idx ];

  // dupes are not allowed
  if ( range::contains( effN.permanent, &eff ) || range::contains( effN.conditional, &eff, &modify_effect_t::eff ) )
    return;

  auto tmp = pack.data;  // local copy

  if ( tmp.value != 0.0 )
  {
    val = tmp.value;
    val_mul = 1.0;
  }
  else
  {
    apply_affecting_mods( pack, val, m, i );
    val *= val_mul;
  }

  if ( !val )
    return;

  std::string val_str = flat ? fmt::format( "{}", val ) : fmt::format( "{}%", val * 100 );

  // always active
  if ( !tmp.buff && !tmp.func )
  {
    if ( flat )
      effN.value += val;
    else
      effN.value *= 1.0 + val;

    effN.permanent.push_back( &eff );
  }
  // conditionally active
  else
  {
    tmp.value = val;
    tmp.flat = flat;
    tmp.eff = &eff;

    if ( eff.flags( EX_SUPPRESS_STACKING ) )
      tmp.use_stacks = false;

    effN.conditional.push_back( std::move( tmp ) );
  }
}

void modified_spell_data_t::print_parsed_spell( report::sc_html_stream& os, const sim_t& sim )
{
  auto c = range::accumulate( effects, 0, &modified_spelleffect_t::size );
  if ( !c )
    return;

  size_t row = 0;

  os.format(
    "<tr>"
    "<td rowspan=\"{}\" class=\"dark\">{}</td>"
    "<td rowspan=\"{}\" class=\"dark right\">{}</td>\n",
    c, report_decorators::decorated_spell_data( sim, &_spell ),
    c, _spell.id() );

  for ( const auto& eff : effects )
    eff.print_parsed_effect( os, sim, row );
}

void modified_spell_data_t::parsed_effects_html( report::sc_html_stream& os, const sim_t& sim,
                                                 std::vector<modified_spell_data_t*> entries )
{
  if ( entries.size() )
  {
    os << R"(<table class="sc even left">)"
       << "<thead><tr>"
       << R"(<th class="left">Dynamic Modified Spell</th>)"
       << R"(<th colspan="2">ID</th>)"
       << "<th>Effect Type</th>"
       << "<th>Modified By</th>"
       << R"(<th colspan="2">ID</th>)"
       << "<th>+/%</th>"
       << "<th>Value</th>"
       << "<th>Notes</th>"
       << "</tr></thead>\n";

    for ( auto m_data : entries )
      m_data->print_parsed_spell( os, sim );

    os << "</table>\n";
  }
}

// main effect parsing function with constexpr checks to handle player_effect_t vs target_effect_t
template <typename U>
bool parse_effects_t::parse_effect( pack_t<U>& pack, size_t i, bool force )
{
  const auto& eff = pack.spell->effectN( i );
  bool mastery = pack.spell->flags( SX_MASTERY_AFFECTS_POINTS );
  double val = 0.0;
  double val_mul = 0.01;

  if ( mastery )
  {
    val = eff.mastery_value();
    pack.data.base_mastery = eff.percent();
  }
  else
  {
    val = eff.base_value();
  }

  if constexpr ( is_detected_v<detect_buff, U> && is_detected_v<detect_type, U> )
  {
    if ( pack.data.buff && pack.data.type & USE_DEFAULT )
      val = pack.data.buff->default_value * 100;

    if ( !is_valid_aura( eff ) )
      return false;
  }
  else
  {
    if ( !is_valid_target_aura( eff ) )
      return false;
  }

  if ( pack.data.type & VALUE_OVERRIDE )
  {
    val = pack.data.value;
    val_mul = 1.0;
    mastery = false;
  }
  else
  {
    apply_affecting_mods( pack, val, mastery, i );

    if ( mastery )
      val_mul = 1.0;
  }

  auto tmp = pack.data;  // local copy

  std::string type_str;
  bool flat = false;
  std::vector<U>* vec = get_effect_vector( eff, tmp, val_mul, type_str, flat, force, pack );

  if ( !vec )
    return false;

  if constexpr ( is_detected_v<detect_type, U> )
  {
    if ( !val && !( tmp.type & ( USE_CURRENT | USE_DEFAULT | ALLOW_ZERO | VALUE_FUNCTION ) ) )
      return false;
  }
  else
  {
    if ( !val )
      return false;
  }

  if constexpr ( is_detected_v<detect_type, U> )
  {
    if ( tmp.type & ROUND_VALUE )
      val = std::round( val );
  }

  val *= val_mul;

  std::string val_str = mastery ? fmt::format( "{:.5f}*mastery", val * 100 )
                        : flat  ? fmt::format( "{}", val )
                                : fmt::format( "{:.1f}%", val * ( tmp.value != 0.0 ? 100 : 1 / val_mul ) );

  if ( tmp.value != 0.0 )
    val_str = val_str + " (value override)";

  if constexpr ( is_detected_v<detect_use_stacks, U> )
  {
    if ( eff.flags( EX_SUPPRESS_STACKING ) )
      tmp.use_stacks = false;
  }

  if constexpr ( is_detected_v<detect_buff, U> && is_detected_v<detect_type, U> )
  {
    if ( tmp.buff )
    {
      if ( tmp.type & USE_CURRENT )
        val_str = flat ? "current value" : "current value percent";
      else if ( tmp.type & USE_DEFAULT )
        val_str = val_str + " (default value)";
    }
  }

  if constexpr ( is_detected_v<detect_value_func, U> )
  {
    if ( tmp.value_func )
      val_str += " (value function)";
  }

  debug_message( tmp, type_str, val_str, eff );

  tmp.value = val;
  tmp.mastery = mastery;
  tmp.eff = &eff;

  if constexpr ( is_detected_v<detect_simple, U> )
  {
    if ( tmp.func || tmp.value_func || tmp.type & USE_CURRENT || tmp.mastery || !tmp.use_stacks ||
         pack.num_callbacks() )
    {
      tmp.simple = false;
    }

    if ( tmp.simple && !tmp.buff && !( tmp.type & PARSE_PASSIVE ) )
    {
      throw_passive_error( pack.spell );
      return false;
    }
  }

  if ( pack.copy )
    pack.copy->push_back( tmp );

  vec->push_back( std::move( tmp ) );

  return true;
}

// explicit template instantiation
template bool parse_effects_t::parse_effect<player_effect_t>( pack_t<player_effect_t>&, size_t, bool );
template bool parse_effects_t::parse_effect<target_effect_t>( pack_t<target_effect_t>&, size_t, bool );

double parse_effects_t::get_effect_value( const player_effect_t& i, bool benefit ) const
{
  if ( !i.simple )
    return get_effect_value_full( i, benefit );

  if ( i.buff )
  {
    auto stack = benefit ? i.buff->stack() : i.buff->check();
    if ( !stack )
      return 0.0;

    return i.value * stack;
  }

  return i.value;
}

double parse_effects_t::get_effect_value_full( const player_effect_t& i, bool benefit ) const
{
  if ( i.func && !i.func() )
    return 0.0;

  double eff_val = i.value_func ? i.value_func( i.value ) : i.value;

  if ( i.buff )
  {
    auto stack = benefit ? i.buff->stack() : i.buff->check();
    if ( !stack )
      return 0.0;

    if ( i.type & USE_CURRENT )
      eff_val = i.buff->check_value();

    if ( i.use_stacks )
      eff_val *= stack;
  }

  if ( i.mastery )
  {
    eff_val *= _player->cache.mastery();
    eff_val += i.base_mastery;
  }

  callback_idx |= i.idx;

  return eff_val;
}

double parse_effects_t::get_effect_value( const target_effect_t& i, actor_target_data_t* td ) const
{
  if ( auto check = i.func( td ) )
  {
    auto eff_val = i.value * check;

    if ( i.mastery )
      eff_val *= _player->cache.mastery();

    return eff_val;
  }

  return 0.0;
}

double parse_player_effects_t::composite_melee_auto_attack_speed() const
{
  auto ms = player_t::composite_melee_auto_attack_speed();

  for ( const auto& i : auto_attack_speed_effects )
    ms *= 1.0 / ( 1.0 + get_effect_value( i ) );

  return ms;
}

double parse_player_effects_t::composite_attribute_multiplier( attribute_e attr ) const
{
  auto am = player_t::composite_attribute_multiplier( attr );

  assert( attr != ATTRIBUTE_NONE && "ATTRIBUTE_NONE will be out of index" );

  for ( const auto& i : attribute_multiplier_effects )
    if ( i.opt_enum & ( 1 << ( attr - 1 ) ) )
      am *= 1.0 + get_effect_value( i );

  return am;
}

double parse_player_effects_t::composite_rating_multiplier( rating_e rating ) const
{
  auto rm = player_t::composite_rating_multiplier( rating );
  auto mod = util::rating_to_rating_mod( rating );

  for ( const auto& i : rating_multiplier_effects )
    if ( i.opt_enum & mod )
      rm *= 1.0 + get_effect_value( i );

  return rm;
}

double parse_player_effects_t::composite_damage_versatility() const
{
  auto v = player_t::composite_damage_versatility();

  for ( const auto& i : versatility_effects )
    v += get_effect_value( i );

  return v;
}

double parse_player_effects_t::composite_heal_versatility() const
{
  auto v = player_t::composite_heal_versatility();

  for ( const auto& i : versatility_effects )
    v += get_effect_value( i );

  return v;
}

double parse_player_effects_t::composite_mitigation_versatility() const
{
  auto v = player_t::composite_mitigation_versatility();

  for ( const auto& i : versatility_effects )
    v += get_effect_value( i ) * 0.5;

  return v;
}

double parse_player_effects_t::composite_player_multiplier( school_e school ) const
{
  auto m = player_t::composite_player_multiplier( school );

  for ( const auto& i : player_multiplier_effects )
    if ( i.opt_enum & dbc::get_school_mask( school ) )
      m *= 1.0 + get_effect_value( i, true );

  return m;
}

double parse_player_effects_t::composite_player_pet_damage_multiplier( const action_state_t* s, bool guardian ) const
{
  auto dm = player_t::composite_player_pet_damage_multiplier( s, guardian );

  for ( const auto& i : pet_multiplier_effects )
    if ( static_cast<bool>( i.opt_enum ) == guardian )
      dm *= 1.0 + get_effect_value( i, true );

  return dm;
}

double parse_player_effects_t::composite_attack_power_multiplier() const
{
  auto apm = player_t::composite_attack_power_multiplier();

  for ( const auto& i : attack_power_multiplier_effects )
    apm *= 1.0 + get_effect_value( i );

  return apm;
}

double parse_player_effects_t::composite_melee_crit_chance() const
{
  auto mcc = player_t::composite_melee_crit_chance();

  for ( const auto& i : crit_chance_effects )
    mcc += get_effect_value( i );

  return mcc;
}

double parse_player_effects_t::composite_spell_crit_chance() const
{
  auto scc = player_t::composite_spell_crit_chance();

  for ( const auto& i : crit_chance_effects )
    scc += get_effect_value( i );

  for ( const auto& i : spell_crit_chance_effects )
    scc += get_effect_value( i );

  return scc;
}

double parse_player_effects_t::composite_player_critical_damage_multiplier( const action_state_t* s,
                                                                            school_e school ) const
{
  auto cdm = player_t::composite_player_critical_damage_multiplier( s, school );

  for ( const auto& i : crit_bonus_effects )
    if ( i.opt_enum & dbc::get_school_mask( school ) )
      cdm *= 1.0 + get_effect_value( i );

  return cdm;
}

double parse_player_effects_t::composite_leech() const
{
  auto leech = player_t::composite_leech();

  for ( const auto& i : leech_effects )
    leech += get_effect_value( i );

  return leech;
}

double parse_player_effects_t::composite_melee_expertise( const weapon_t* ) const
{
  auto me = player_t::composite_melee_expertise( nullptr );

  for ( const auto& i : expertise_effects )
    me += get_effect_value( i );

  return me;
}

double parse_player_effects_t::composite_crit_avoidance() const
{
  auto ca = player_t::composite_crit_avoidance();

  for ( const auto& i : crit_avoidance_effects )
    ca += get_effect_value( i );

  return ca;
}

double parse_player_effects_t::composite_parry() const
{
  auto parry = player_t::composite_parry();

  for ( const auto& i : parry_effects )
    parry += get_effect_value( i );

  return parry;
}

double parse_player_effects_t::composite_base_armor_multiplier() const
{
  auto bam = player_t::composite_base_armor_multiplier();

  for ( const auto& i : base_armor_multiplier_effects )
    bam *= 1.0 + get_effect_value( i );

  return bam;
}

double parse_player_effects_t::composite_armor_multiplier() const
{
  auto am = player_t::composite_armor_multiplier();

  for ( const auto& i : armor_multiplier_effects )
    am *= 1.0 + get_effect_value( i );

  return am;
}

double parse_player_effects_t::composite_melee_haste() const
{
  auto mh = player_t::composite_melee_haste();

  for ( const auto& i : haste_effects )
    mh *= 1.0 / ( 1.0 + get_effect_value( i ) );

  for ( const auto& i : melee_haste_effects )
    mh *= 1.0 / ( 1.0 + get_effect_value( i ) );

  return mh;
}

double parse_player_effects_t::composite_spell_haste() const
{
  auto sh = player_t::composite_spell_haste();

  for ( const auto& i : haste_effects )
    sh *= 1.0 / ( 1.0 + get_effect_value( i ) );

  for ( const auto& i : spell_haste_effects )
    sh *= 1.0 / ( 1.0 + get_effect_value( i ) );

  return sh;
}

double parse_player_effects_t::composite_mastery() const
{
  auto m = player_t::composite_mastery();

  for ( const auto& i : mastery_effects )
    m += get_effect_value( i );

  return m;
}

double parse_player_effects_t::composite_parry_rating() const
{
  auto pr = player_t::composite_parry_rating();

  for ( const auto& i : parry_rating_from_crit_effects )
    pr += player_t::composite_melee_crit_rating() * get_effect_value( i );

  return pr;
}

double parse_player_effects_t::composite_dodge() const
{
  auto dodge = player_t::composite_dodge();

  for ( const auto& i : dodge_effects )
    dodge += get_effect_value( i );

  return dodge;
}

double parse_player_effects_t::composite_player_absorb_multiplier( const action_state_t* s ) const
{
  auto am = player_t::composite_player_absorb_multiplier( s );

  for ( const auto& i : absorb_multiplier_effects )
    am *= 1.0 + get_effect_value( i );

  return am;
}

double parse_player_effects_t::composite_player_healing_received_multiplier() const
{
  auto hr = player_t::composite_player_healing_received_multiplier();

  for ( const auto& i : healing_received_effects )
    hr *= 1.0 + get_effect_value( i );

  return hr;
}

double parse_player_effects_t::composite_player_absorb_received_multiplier() const
{
  auto ar = player_t::composite_player_absorb_received_multiplier();

  for ( const auto& i : absorb_received_mult_effects )
    ar *= 1.0 + get_effect_value( i );

  return ar;
}

double parse_player_effects_t::composite_player_target_multiplier( player_t* t, school_e school ) const
{
  auto tm = player_t::composite_player_target_multiplier( t, school );
  auto td = get_target_data( t );

  for ( const auto& i : target_multiplier_effects )
    if ( i.opt_enum & dbc::get_school_mask( school ) )
      tm *= 1.0 + get_effect_value( i, td );

  return tm;
}

double parse_player_effects_t::composite_player_target_pet_damage_multiplier( player_t* t, bool guardian ) const
{
  auto tm = player_t::composite_player_target_pet_damage_multiplier( t, guardian );
  auto td = get_target_data( t );

  for ( const auto& i : target_pet_multiplier_effects )
    if ( static_cast<bool>( i.opt_enum ) == guardian )
      tm *= 1.0 + get_effect_value( i, td );

  return tm;
}

double parse_player_effects_t::composite_mitigation_multiplier( const action_state_t* s, school_e school, bool direct ) const
{
  auto m = player_t::composite_mitigation_multiplier( s, school, direct );

  for ( const auto& i : mitigation_multiplier_effects )
    if ( i.opt_enum & dbc::get_school_mask( school ) )
      m *= 1.0 + get_effect_value( i );

  return m;
}

double parse_player_effects_t::composite_mitigation_from_player_multiplier( player_t* source, const action_state_t* s,
                                                                            school_e school, bool direct ) const
{
  auto m = player_t::composite_mitigation_from_player_multiplier( source, s, school, direct );
  auto td = get_target_data( source );

  for ( const auto& i : mitigation_from_target_multiplier_effects )
    if ( i.opt_enum & dbc::get_school_mask( school ) )
      m *= 1.0 + get_effect_value( i, td );

  return m;
}

double parse_player_effects_t::non_stacking_movement_modifier() const
{
  auto m = player_t::non_stacking_movement_modifier();

  for ( const auto& i : non_stacking_movement_effects )
    m = std::max( get_effect_value( i ), m );

  return m;
}

double parse_player_effects_t::stacking_movement_modifier() const
{
  auto m = player_t::stacking_movement_modifier();

  for ( const auto& i : stacking_movement_effects )
    m += get_effect_value( i );

  return m;
}

void parse_player_effects_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  for ( const auto& i : invalidate_with_parent )
  {
    if ( c == i.second )
      player_t::invalidate_cache( i.first );
  }
}

bool parse_player_effects_t::is_valid_aura( const spelleffect_data_t& eff ) const
{
  switch ( eff.type() )
  {
    case E_APPLY_AURA:
    case E_APPLY_AURA_PET:
      // TODO: more robust logic around 'party' buffs with radius
      // Effect targeting type 120 is "Apply to Summons in Area", so we should allow that.
      if ( eff.radius() && eff.target_1() != T_UNIT_SELF_AND_SUMMONS )
        return false;
      break;
    case E_APPLY_AREA_AURA_PARTY:
    case E_APPLY_AREA_AURA_PET:
      break;
    default:
      return false;
  }

  return true;
}

std::vector<player_effect_t>* parse_player_effects_t::get_effect_vector( const spelleffect_data_t& eff,
                                                                         player_effect_t& tmp, double& val_mul,
                                                                         std::string& str, bool& /* flat */,
                                                                         bool /* force */,
                                                                         const pack_t<player_effect_t>& /* pack */ )
{
  auto invalidate = [ &tmp ]( cache_e c ) {
    if ( tmp.buff )
      tmp.buff->add_invalidate( c );
  };

  switch ( eff.subtype() )
  {
    case A_MOD_MELEE_AUTO_ATTACK_SPEED:
    case A_MOD_RANGED_AND_MELEE_AUTO_ATTACK_SPEED:
      str = "auto attack speed";
      invalidate( CACHE_AUTO_ATTACK_SPEED );
      return &auto_attack_speed_effects;

    case A_MOD_TOTAL_STAT_PERCENTAGE:
      tmp.opt_enum = eff.misc_value2();
      str          = opt_strings::attributes_invalidate( tmp );
      return &attribute_multiplier_effects;

    case A_MOD_RATING_MULTIPLIER:
      tmp.opt_enum = eff.misc_value1();
      str = opt_strings::ratings_invalidate( tmp );
      return &rating_multiplier_effects;

    case A_MOD_VERSATILITY_PCT:
      str = "versatility";
      invalidate( CACHE_VERSATILITY );
      return &versatility_effects;

    case A_HASTE_ALL:
      str = "all haste";
      invalidate( CACHE_HASTE );
      return &haste_effects;

    case A_MOD_MELEE_HASTE:
      str = "melee haste";
      invalidate( CACHE_ATTACK_HASTE );
      return &melee_haste_effects;

    case A_MOD_SPELL_HASTE:
      str = "spell haste";
      invalidate( CACHE_SPELL_HASTE );
      return &spell_haste_effects;

    case A_MOD_MASTERY_PCT:
      str = "mastery";
      val_mul = 1.0;
      invalidate( CACHE_MASTERY );
      return &mastery_effects;

    case A_MOD_ALL_CRIT_CHANCE:
      str = "all crit chance";
      invalidate( CACHE_CRIT_CHANCE );
      return &crit_chance_effects;

    case A_MOD_SPELL_CRIT_CHANCE:
      str = "spell crit chance";
      invalidate( CACHE_CRIT_CHANCE );
      return &spell_crit_chance_effects;

    case A_MOD_CRIT_DAMAGE_MULTIPLIER:
      str = "crit damage bonus";
      tmp.opt_enum = eff.misc_value1();
      return &crit_bonus_effects;

    case A_MOD_DAMAGE_PERCENT_DONE:
      tmp.opt_enum = eff.misc_value1();
      str = opt_strings::school( tmp.opt_enum );
      str += " damage";
      invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
      return &player_multiplier_effects;

    case A_MOD_PET_DAMAGE_DONE:
      tmp.opt_enum = 0;
      str = "pet damage";
      invalidate( CACHE_PET_DAMAGE_MULTIPLIER );
      return &pet_multiplier_effects;

    case A_MOD_GUARDIAN_DAMAGE_DONE:
      tmp.opt_enum = 1;
      str = "guardian damage";
      invalidate( CACHE_GUARDIAN_DAMAGE_MULTIPLIER );
      return &pet_multiplier_effects;

    case A_MOD_ATTACK_POWER_PCT:
      str = "attack power";
      invalidate( CACHE_ATTACK_POWER );
      return &attack_power_multiplier_effects;

    case A_MOD_LEECH_PERCENT:
      str = "leech";
      invalidate( CACHE_LEECH );
      return &leech_effects;

    case A_MOD_EXPERTISE:
      str = "expertise";
      invalidate( CACHE_EXP );
      return &expertise_effects;

    case A_MOD_ATTACKER_MELEE_CRIT_CHANCE:
      str = "crit avoidance";
      invalidate( CACHE_CRIT_AVOIDANCE );
      return &crit_avoidance_effects;

    case A_MOD_PARRY_PERCENT:
      str = "parry";
      invalidate( CACHE_PARRY );
      return &parry_effects;

    case A_MOD_BASE_RESISTANCE_PCT:
      if ( eff.misc_value1() & SCHOOL_MASK_PHYSICAL )
      {
        str = "base armor multiplier";
        invalidate( CACHE_ARMOR );
        return &base_armor_multiplier_effects;
      }
      return nullptr;

    case A_MOD_RESISTANCE_PCT:
      if ( eff.misc_value1() & SCHOOL_MASK_PHYSICAL )
      {
        str = "armor multiplier";
        invalidate( CACHE_ARMOR );
        return &armor_multiplier_effects;
      }
      return nullptr;

    case A_MOD_PARRY_FROM_CRIT_RATING:
      str = "parry rating|of crit rating";
      invalidate_with_parent.emplace_back( CACHE_PARRY, CACHE_CRIT_CHANCE );
      return &parry_rating_from_crit_effects;

    case A_MOD_DODGE_PERCENT:
      str = "dodge";
      invalidate( CACHE_DODGE );
      return &dodge_effects;

    case A_MOD_DAMAGE_PERCENT_TAKEN:
      tmp.opt_enum = eff.misc_value1();
      str = opt_strings::school( tmp.opt_enum );
      str += " damage taken";
      return &mitigation_multiplier_effects;

    case A_MOD_ABSORB_DONE_PERCENT:
      str = "absorb multiplier";
      return &absorb_multiplier_effects;

    case A_MOD_HEALING_RECEIVED_PCT:
      str = "healing received";
      return &healing_received_effects;

    case A_MOD_ABSORB_RECEIVED_PERCENT:
      str = "absorb received";
      return &absorb_received_mult_effects;

    case A_MOD_INCREASE_SPEED:
      str = "exclusive move speed";
      invalidate( CACHE_RUN_SPEED );
      return &non_stacking_movement_effects;

    case A_MOD_SPEED_ALWAYS:
      str = "stacking move speed";
      invalidate( CACHE_RUN_SPEED );
      return &stacking_movement_effects;

    case A_MOD_PERCENT_STAT:
      sim->error( SEVERE,
                  "Parse Effects: {}(id={}), effectN {} is utilizing aura subtype 80, rather than 137. This is a bug, "
                  "as it only modifies base attributes. This effect will not be applied by Parse Effects.",
                  eff.spell()->name_cstr(), eff.spell()->id(), eff.index() + 1 );
      return nullptr;

    default:
      return nullptr;
  }

  return nullptr;
}

void parse_player_effects_t::debug_message( const player_effect_t& data, std::string_view type_str,
                                            std::string_view val_str, const spelleffect_data_t& eff )
{
  auto splits = util::string_split<std::string_view>( type_str, "|" );
  auto tok1 = splits[ 0 ];
  auto tok2 = std::string( val_str );

  if ( splits.size() > 1 )
    tok2 += fmt::format( " {}", splits[ 1 ] );

  if ( data.buff )
  {
    sim->print_debug( "player-effects: {} {} modified by {} {} buff {} ({}#{})", *this, tok1, tok2,
                      data.use_stacks ? "per stack of" : "with", data.buff->name(), eff.spell()->id(),
                      eff.index() + 1 );
  }
  else if ( data.func )
  {
    sim->print_debug( "player-effects: {} {} modified by {} with condition from {}", *this, tok1, tok2, eff );
  }
  else
  {
    sim->print_debug( "player-effects: {} {} modified by {} from {}", *this, tok1, tok2, eff );
  }
}

void parse_player_effects_t::throw_passive_error( const spell_data_t* s )
{
  if ( s->flags( SX_PASSIVE ) )
  {
    sim->error(
      "Parse Effects: {} is a passive spell and applied automatically. Please remove this from parse_effects().", *s );
  }
  else
  {
    sim->error(
      "Parse Effects: {} was determined to be a passive spell and ignored. Please report if incorrect.", *s );
  }
}

bool parse_player_effects_t::is_valid_target_aura( const spelleffect_data_t& eff ) const
{
  if ( eff.type() == E_APPLY_AURA )
    return true;

  return false;
}

std::vector<target_effect_t>* parse_player_effects_t::get_effect_vector( const spelleffect_data_t& eff,
                                                                         target_effect_t& tmp, double& /* val_mul */,
                                                                         std::string& str, bool& /* flat */,
                                                                         bool /* force */,
                                                                         const pack_t<target_effect_t>& /* pack */ )
{
  switch ( eff.subtype() )
  {
    case A_MOD_DAMAGE_FROM_CASTER:
      tmp.opt_enum = eff.misc_value1();
      str = fmt::format( "{} damage taken from {}", opt_strings::school( tmp.opt_enum ), *_player );
      return &target_multiplier_effects;

    case A_MOD_DAMAGE_FROM_CASTER_PET:
      tmp.opt_enum = 0;
      str = fmt::format( "pet damage taken from {}", *_player );
      return &target_pet_multiplier_effects;

    case A_MOD_DAMAGE_FROM_CASTER_GUARDIAN:
      tmp.opt_enum = 1;
      str = fmt::format( "guardian damage taken from {}", *_player );
      return &target_pet_multiplier_effects;

    case A_MOD_DAMAGE_TO_CASTER:
      tmp.opt_enum = eff.misc_value1();
      str = fmt::format( "{} damage done to {}", opt_strings::school( tmp.opt_enum ), *_player );
      return &mitigation_from_target_multiplier_effects;

    default:
      return nullptr;
  }

  return nullptr;
}

void parse_player_effects_t::debug_message( const target_effect_t&, std::string_view type_str, std::string_view val_str,
                                            const spelleffect_data_t& eff )
{
  sim->print_debug( "target-effects: Target {} modified by {} from {}", type_str, val_str, eff );
}

void parse_player_effects_t::print_custom_parsed_effects( report::sc_html_stream& os ) const
{
  if ( total_effects_count() )
  {
    os << R"(<table class="sc even left">)"
       << "<thead><tr>"
       << R"(<th class="left">Dynamic Effects</th>)"
       << "<th>Spell</th>"
       << R"(<th colspan="2">ID</th>)"
       << "<th>Value</th>"
       << "<th>Source</th>"
       << "<th>Notes</th>"
       << "</tr></thead>\n";

    auto mastery_val = [ this ]( double v ) {
      return fmt::format( "{:.1f}%", v * mastery_coefficient() * 100 );
    };

    print_parsed_type( os, absorb_multiplier_effects, "Absorb Multiplier" );
    print_parsed_type( os, absorb_received_mult_effects, "Absorb Received Multiplier" );
    print_parsed_type( os, attack_power_multiplier_effects, "Attack Power Multiplier" );
    print_parsed_type( os, armor_multiplier_effects, "Armor Multiplier" );
    print_parsed_type( os, auto_attack_speed_effects, "Auto Attack Speed" );
    print_parsed_type( os, attribute_multiplier_effects, "Attribute Multiplier", &opt_strings::attributes );
    print_parsed_type( os, base_armor_multiplier_effects, "Base Armor Multiplier" );
    print_parsed_type( os, crit_avoidance_effects, "Crit Avoidance" );
    print_parsed_type( os, crit_chance_effects, "Crit Chance" );
    print_parsed_type( os, crit_bonus_effects, "Crit Damage Bonus" );
    print_parsed_type( os, mitigation_multiplier_effects, "Damage Taken Multiplier", &opt_strings::school );
    print_parsed_type( os, mitigation_from_target_multiplier_effects, "Damage Taken From Target Multiplier",
                       &opt_strings::school );
    print_parsed_type( os, dodge_effects, "Dodge" );
    print_parsed_type( os, expertise_effects, "Expertise" );
    print_parsed_type( os, haste_effects, "All Haste" );
    print_parsed_type( os, melee_haste_effects, "Melee Haste" );
    print_parsed_type( os, spell_haste_effects, "Spell Haste" );
    print_parsed_type( os, healing_received_effects, "Healing Received" );
    print_parsed_type( os, leech_effects, "Leech" );
    print_parsed_type( os, mastery_effects, "Mastery", nullptr, mastery_val );
    print_parsed_type( os, non_stacking_movement_effects, "Move Speed Modifier - Exclusive" );
    print_parsed_type( os, stacking_movement_effects, "Move Speed Modifier - Stacking" );
    print_parsed_type( os, parry_effects, "Parry" );
    print_parsed_type( os, parry_rating_from_crit_effects, "Parry Rating from Crit" );
    print_parsed_type( os, pet_multiplier_effects, "Pet Multiplier", &opt_strings::pet_type );
    print_parsed_type( os, player_multiplier_effects, "Player Multiplier", &opt_strings::school );
    print_parsed_type( os, rating_multiplier_effects, "Rating Multiplier", &opt_strings::ratings );
    print_parsed_type( os, spell_crit_chance_effects, "Spell Crit Chance" );
    print_parsed_type( os, target_multiplier_effects, "Target Multiplier", &opt_strings::school );
    print_parsed_type( os, target_pet_multiplier_effects, "Target Pet Multiplier", &opt_strings::pet_type );
    print_parsed_type( os, versatility_effects, "Versatility" );
    print_parsed_custom_type( os );

    os << "</table>\n";
  }
}

size_t parse_player_effects_t::total_effects_count() const
{
  return auto_attack_speed_effects.size() +
         attribute_multiplier_effects.size() +
         rating_multiplier_effects.size() +
         versatility_effects.size() +
         player_multiplier_effects.size() +
         pet_multiplier_effects.size() +
         attack_power_multiplier_effects.size() +
         crit_chance_effects.size() +
         spell_crit_chance_effects.size() +
         crit_bonus_effects.size() +
         leech_effects.size() +
         expertise_effects.size() +
         crit_avoidance_effects.size() +
         parry_effects.size() +
         base_armor_multiplier_effects.size() +
         armor_multiplier_effects.size() +
         haste_effects.size() +
         melee_haste_effects.size() +
         spell_haste_effects.size() +
         mastery_effects.size() +
         parry_rating_from_crit_effects.size() +
         dodge_effects.size() +
         mitigation_multiplier_effects.size() +
         mitigation_from_target_multiplier_effects.size() +
         absorb_multiplier_effects.size() +
         healing_received_effects.size() +
         absorb_received_mult_effects.size() +
         target_multiplier_effects.size() +
         target_pet_multiplier_effects.size() +
         non_stacking_movement_effects.size() +
         stacking_movement_effects.size();
}

void parse_action_base_t::parse_callback_function( pack_t<player_effect_t>& pack, parse_cb_t cb )
{
  // 8 max per type as parse_effects_t::callback_idx is uint32_t
  assert( callback_list[ pack.callback_type ].size() < 8 && "cannot register more than 8 parse callbacks per type" );

  // set values on main pack, to be propagated to all copies
  pack.callback[ pack.callback_type ] = std::move( cb );

  // this is set BEFORE the callback is added to the vector, so the idx will be 1bit left of size()
  uint32_t mask = 1U << ( callback_list[ pack.callback_type ].size() + 8 * static_cast<unsigned>( pack.callback_type ) );
  pack.data.idx |= mask;
  callback_mask[ pack.callback_type ] |= mask;
}

void parse_action_base_t::parse_callback_function( pack_t<player_effect_t>& pack, parse_flag_e type )
{
  if ( type == CONSUME_BUFF )
  {
    assert( pack.data.buff && "CONSUME_BUFF requires a buff" );

    if ( !pack.data.buff->can_consume( _action ) )
    {
      _player->sim->print_debug( "action-effects: {} cannot consume buff {}, skipping CONSUME_BUFF.", *_action,
                                 *pack.data.buff );
    }
    else
    {
      parse_callback_function( pack, [ a = _action, b = pack.data.buff ]( action_state_t* ) {
        b->consume( a );
      } );
    }
  }
}

void parse_action_base_t::register_callback_function( pack_t<player_effect_t>& pack )
{
  for ( size_t i = 0; i < pack.callback.size(); i++ )
  {
    if ( !pack.callback[ i ] )
      continue;

    callback_list[ i ].push_back( std::move( pack.callback[ i ] ) );
    _player->sim->print_debug( "action-effects: {} registering {} parse callback ({:#010x}) on {} {}", *_action,
                               opt_strings::parse_cb_str( static_cast<parse_callback_e>( i ) ),
                               pack.data.idx & ( 0xFF << ( i * 8 ) ), pack.data.buff ? "buff" : "spell", *pack.spell );
  }
}

void parse_action_base_t::trigger_callbacks( parse_callback_e cb_type, action_state_t* state )
{
  if ( callback_idx )
  {
    auto i = 8 * static_cast<unsigned>( cb_type );
    for ( const auto& cb : callback_list[ cb_type ] )
      if ( callback_idx & ( 1U << i++ ) )
        cb( state );
  }
}

bool parse_action_base_t::is_valid_aura( const spelleffect_data_t& eff ) const
{
  // Only parse apply aura effects
  switch ( eff.type() )
  {
    case E_APPLY_AURA:
    case E_APPLY_AURA_PET:
      // TODO: more robust logic around 'party' buffs with radius
      if ( eff.radius() )
        return false;
      break;
    case E_APPLY_AREA_AURA_PARTY:
    case E_APPLY_AREA_AURA_PET:
      break;
    default:
      return false;
  }

  return true;
}

// non-templated implementation of parse_action_effects_t::get_effect_vector()
std::vector<player_effect_t>* parse_action_base_t::get_effect_vector( const spelleffect_data_t& eff,
                                                                      player_effect_t& tmp, double& val_mul,
                                                                      std::string& str, bool& flat, bool force,
                                                                      const pack_t<player_effect_t>& pack )
{
  if ( !_action->special && eff.subtype() == A_MOD_AUTO_ATTACK_PCT )
  {
    str = "auto attack";
    return &da_multiplier_effects;
  }

  if ( !force )
  {
    if ( !check_affected_list( pack.affect_lists, eff, force ) )
      return nullptr;
    else if ( force )
      tmp.type |= AFFECTED_OVERRIDE;
  }

  if ( !force )  // force_effect or positive affect_list_t will result in force == true
  {
    if ( pack.ignore_whitelist )  // if ignoring whitelist, anything not forced results in no match.
      return nullptr;
    else if ( !_action->data().affected_by_all( eff ) )  // standard DBC whitelist filter
      return nullptr;
  }

  if ( eff.subtype() == A_ADD_PCT_MODIFIER || eff.subtype() == A_ADD_PCT_LABEL_MODIFIER )
  {
    switch ( eff.misc_value1() )
    {
      case P_GENERIC:         str = "direct damage";          return &da_multiplier_effects;
      case P_DURATION:        str = "duration";               return &dot_duration_effects;
      case P_TICK_DAMAGE:     str = "tick damage";            return &ta_multiplier_effects;
      case P_CAST_TIME:       str = "cast time";              return &execute_time_effects;
      case P_GCD:             str = "gcd";                    return &gcd_effects;
      case P_TICK_TIME:       str = "tick time";              return &tick_time_effects;
      case P_RESOURCE_COST_1: str = "cost percent";           return &cost_effects;
      case P_CRIT:            str = "crit chance multiplier"; return &crit_chance_multiplier_effects;
      case P_CRIT_BONUS:      str = "crit bonus multiplier";  return &crit_bonus_effects;
      case P_COOLDOWN:        str = "cooldown";               return &recharge_multiplier_effects;
      default:                return nullptr;
    }
  }
  else if ( eff.subtype() == A_ADD_FLAT_MODIFIER || eff.subtype() == A_ADD_FLAT_LABEL_MODIFIER )
  {
    flat = true;

    switch ( eff.misc_value1() )
    {
      case P_CAST_TIME:
        val_mul = 1.0;
        str = "cast time";
        return &flat_execute_time_effects;
      case P_TICK_TIME:
        val_mul = 1.0;
        str = "tick time";
        return &flat_tick_time_effects;
      case P_CRIT:
        str = "crit chance";
        return &crit_chance_effects;
      case P_RESOURCE_COST_1:
        val_mul = power_type_data_t::multiplier( _action->current_resource(), _action->sim->dbc->ptr );
        str = "flat cost";
        return &flat_cost_effects;
      default:
        return nullptr;
    }
  }
  else if ( eff.subtype() == A_MOD_RECHARGE_TIME_PCT_CATEGORY )
  {
    tmp.opt_enum = 1;
    str = "category cooldown";
    return &recharge_multiplier_effects;
  }
  else if ( eff.subtype() == A_MOD_CHARGE_RECHARGE_RATE )
  {
    tmp.opt_enum = 1;
    str = "category recharge_rate";
    return &recharge_rate_effects;
  }
  else if ( eff.subtype() == A_MOD_RECHARGE_RATE || eff.subtype() == A_MOD_RECHARGE_RATE_LABEL )
  {
    str = "recharge_rate";
    return &recharge_rate_effects;
  }
  else if ( eff.subtype() == A_MODIFY_SCHOOL )
  {
    auto school = dbc::get_school_type( eff.misc_value1() );

    if ( tmp.buff )
    {
      tmp.buff->add_stack_change_callback( [ a = _action, school ]( buff_t*, int old_, int new_ ) {
        if ( !old_ )
          a->set_school_override( school );
        else if ( !new_ )
          a->clear_school_override();
      } );
    }

    tmp.type |= parse_flag_e::ALLOW_ZERO;
    tmp.opt_enum = eff.misc_value1();
    flat = true;
    str = fmt::format( "spell school|{}", util::school_type_string( school ) );
    return &spell_school_effects;
  }

  return nullptr;
}

void parse_action_base_t::debug_message( const player_effect_t& data, std::string_view type_str,
                                         std::string_view val_str, const spelleffect_data_t& eff )
{
  auto splits = util::string_split<std::string_view>( type_str, "|" );
  auto tok1 = splits[ 0 ];
  auto tok2 = std::string( val_str );

  if ( splits.size() > 1 )
    tok2 = splits[ 1 ];

  if ( data.buff )
  {
    std::string stack_str;
    if ( data.use_stacks )
    {
      if ( data.idx )
        stack_str = "per consumed stack of";
      else
        stack_str = "per stack of";
    }
    else
    {
      if ( data.idx )
        stack_str = "from consuming";
      else
        stack_str = "with";
    }

    _action->sim->print_debug( "action-effects: {} {} modified by {} {} buff {} ({}#{})", *_action, tok1, tok2,
                               stack_str, data.buff->name(), eff.spell()->id(), eff.index() + 1 );
  }
  else if ( data.func )
  {
    _action->sim->print_debug( "action-effects: {} {} modified by {} with condition from {}", *_action, tok1, tok2,
                               eff );
  }
  else
  {
    _action->sim->print_debug( "action-effects: {} {} modified by {} from {}", *_action, tok1, tok2, eff );
  }
}

void parse_action_base_t::throw_passive_error( const spell_data_t* s )
{
  if ( s->flags( SX_PASSIVE ) )
  {
    _action->sim->error(
      "Parse Effects: {} is a passive spell and applied automatically. Please remove this from parse_effects().", *s );
  }
  else
  {
    _action->sim->error(
      "Parse Effects: {} was determined to be a passive spell and ignored. Please report if incorrect.", *s );
  }
}

bool parse_action_base_t::is_valid_target_aura( const spelleffect_data_t& eff ) const
{
  if ( eff.type() == E_APPLY_AURA )
    return true;

  return false;
}

std::vector<target_effect_t>* parse_action_base_t::get_effect_vector( const spelleffect_data_t& eff,
                                                                      target_effect_t& tmp, double& /* val_mul */,
                                                                      std::string& str, bool& flat, bool force,
                                                                      const pack_t<target_effect_t>& pack )
{
  if ( !_action->special && eff.subtype() == A_MOD_AUTO_ATTACK_FROM_CASTER )
  {
    str = "auto attack";
    return &target_multiplier_effects;
  }

  if ( !force )
  {
    if ( !check_affected_list( pack.affect_lists, eff, force ) )
      return nullptr;
    else if ( force )
      tmp.type |= AFFECTED_OVERRIDE;
  }

  if ( !force )  // force_effect or positive affect_list_t will result in force == true
  {
    if ( pack.ignore_whitelist )  // if ignoring whitelist, anything not forced results in no match.
      return nullptr;
    else if ( !_action->data().affected_by_all( eff ) )  // standard DBC whitelist filter
      return nullptr;
  }

  switch ( eff.subtype() )
  {
    case A_MOD_DAMAGE_FROM_CASTER_SPELLS:
    case A_MOD_DAMAGE_FROM_CASTER_SPELLS_LABEL:    str = "damage";      return &target_multiplier_effects;
    case A_MOD_CRIT_CHANCE_FROM_CASTER_SPELLS:     flat = true;
                                                   str = "crit chance"; return &target_crit_chance_effects;
    case A_MOD_CRIT_DAMAGE_PCT_FROM_CASTER_SPELLS: str = "crit bonus";  return &target_crit_bonus_effects;
    default:                                       return nullptr;
  }

  return nullptr;
}

void parse_action_base_t::debug_message( const target_effect_t&, std::string_view type_str, std::string_view val_str,
                                         const spelleffect_data_t& eff )
{
  _action->sim->print_debug( "target-effects: {} {} modified by {} on targets with debuff {}", *_action, type_str,
                             val_str, eff );
}

bool parse_action_base_t::can_force( const spelleffect_data_t& eff ) const
{
  if ( _action->data().affected_by_all( eff ) )
  {
    assert( false && "Effect already affects action, no need to force" );
    return false;
  }

  return true;
}

bool parse_action_base_t::check_affected_list( const std::vector<affect_list_t>& lists, const spelleffect_data_t& eff,
                                               bool& force )
{
  for ( const auto& list : lists )
  {
    if ( list.idx.size() && !range::contains( list.idx, eff.index() + 1 ) )
      continue;

    for ( auto f : list.family )
    {
      if ( _action->data().class_flag( std::abs( f ) - 1 ) )
      {
        if ( f > 0 )
        {
          force = true;
          return true;
        }
        else if ( f < 0 )
        {
          return false;
        }
      }
    }

    for ( auto l : list.label )
    {
      if ( _action->data().affected_by_label( std::abs( l ) ) )
      {
        if ( l > 0 )
        {
          force = true;
          return true;
        }
        else if ( l < 0 )
        {
          return false;
        }
      }
    }

    for ( auto s : list.spell )
    {
      if ( _action->data().id() == as<unsigned>( std::abs( s ) ) )
      {
        if ( s > 0 )
        {
          force = true;
          return true;
        }
        else if ( s < 0 )
        {
          return false;
        }
      }
    }
  }

  return true;
}

void parse_action_base_t::parsed_effects_html( report::sc_html_stream& os ) const
{
  size_t c = 0;
  for ( auto a : _action->stats->action_list )
    if ( auto tmp = dynamic_cast<parse_action_base_t*>( a ) )
      c += tmp->total_effects_count();

  if ( c )
  {
    os << "<div>"
       << "<h4>Affected By (Dynamic)</h4>"
       << "<table class=\"details nowrap\" style=\"width:min-content\">\n";

    os << "<tr>"
       << "<th class=\"small\">Type</th>"
       << "<th class=\"small\">Spell</th>"
       << "<th class=\"small\" colspan=\"2\">ID</th>"
       << "<th class=\"small\">Value</th>"
       << "<th class=\"small\">Source</th>"
       << "<th class=\"small\">Notes</th>"
       << "</tr>\n";

    auto timespan_fn = []( double v ) { return fmt::format( "{}s", timespan_t::from_millis( v ) ); };
    auto flat_fn = []( double v ) { return fmt::to_string( v ); };
    auto empty_fn = []( double ) { return ""; };
    auto disabled_fn = [ this ]( uint32_t ) {
      return _action->snapshot_flags & ( STATE_TGT_MUL_DA | STATE_TGT_MUL_TA ) ? "" : "DISABLED";
    };

    using VEC = parse_action_base_t;
    print_parsed_type( os, &VEC::da_multiplier_effects, "Direct Damage" );
    print_parsed_type( os, &VEC::ta_multiplier_effects, "Periodic Damage" );
    print_parsed_type( os, &VEC::crit_chance_effects, "Critical Strike Chance" );
    print_parsed_type( os, &VEC::crit_bonus_effects, "Critical Strike Bonus Damage" );
    print_parsed_type( os, &VEC::flat_execute_time_effects, "Flat Cast Time", nullptr, timespan_fn );
    print_parsed_type( os, &VEC::execute_time_effects, "Percent Cast Time" );
    print_parsed_type( os, &VEC::gcd_effects, "Percent GCD" );
    print_parsed_type( os, &VEC::flat_dot_duration_effects, "Flat Duration", nullptr, timespan_fn );
    print_parsed_type( os, &VEC::dot_duration_effects, "Percent Duration" );
    print_parsed_type( os, &VEC::flat_tick_time_effects, "Flat Tick Time", nullptr, timespan_fn );
    print_parsed_type( os, &VEC::tick_time_effects, "Percent Tick Time" );
    print_parsed_type( os, &VEC::recharge_multiplier_effects, "Recharge Multiplier", &opt_strings::cooldown_type );
    print_parsed_type( os, &VEC::recharge_rate_effects, "Recharge Rate" );
    print_parsed_type( os, &VEC::flat_cost_effects, "Flat Cost", nullptr, flat_fn );
    print_parsed_type( os, &VEC::cost_effects, "Percent Cost" );
    print_parsed_type( os, &VEC::spell_school_effects, "Spell School", &opt_strings::school, empty_fn );
    print_parsed_type( os, &VEC::target_multiplier_effects, "Damage on Debuff", disabled_fn );
    print_parsed_type( os, &VEC::target_crit_chance_effects, "Crit Chance on Debuff" );
    print_parsed_type( os, &VEC::target_crit_bonus_effects, "Critical Strike Bonus on Debuff" );
    print_parsed_custom_type( os );

    os << "</table>\n"
       << "</div>\n";
  }
}

size_t parse_action_base_t::total_effects_count() const
{
  return ta_multiplier_effects.size() +
         da_multiplier_effects.size() +
         execute_time_effects.size() +
         flat_execute_time_effects.size() +
         gcd_effects.size() +
         dot_duration_effects.size() +
         flat_dot_duration_effects.size() +
         tick_time_effects.size() +
         flat_tick_time_effects.size() +
         recharge_multiplier_effects.size() +
         recharge_rate_effects.size() +
         cost_effects.size() +
         flat_cost_effects.size() +
         crit_chance_effects.size() +
         crit_chance_multiplier_effects.size() +
         crit_bonus_effects.size() +
         spell_school_effects.size() +
         target_multiplier_effects.size() +
         target_crit_chance_effects.size() +
         target_crit_bonus_effects.size();
}

void parse_action_base_t::process_cooldown_buffs( bool dynamic, std::vector<player_effect_t>& vec )
{
  // remove category effects for non-category cooldowns and vice versa. we need to process this here in init_finished
  // as actions may get assigned different cooldowns during construction.
  range::erase_remove( vec, [ is_category = _action->cooldown->is_category() ]( const auto& i ) {
    return is_category ? i.opt_enum != 1 : i.opt_enum == 1;
  } );

  for ( const auto& i : vec )
  {
    if ( i.buff && ( i.buff->gcd_type == gcd_haste_type::NONE || !dynamic ) && !range::contains( _cd_buffs, i.buff ) )
    {
      _player->sim->print_debug( "action-effects: cooldown {} recharge multiplier adjusted by buff {} ({})",
                                 _action->cooldown->name(), i.buff->name(), i.buff->data().id() );

      if ( _action->internal_cooldown->charges )
      {
        i.buff->add_stack_change_callback( [ this ]( auto, auto, auto ) {
          _action->cooldown->adjust_recharge_multiplier();
          _action->internal_cooldown->adjust_recharge_multiplier();
        } );
      }
      else
      {
        i.buff->add_stack_change_callback( [ this ]( auto, auto, auto ) {
          _action->cooldown->adjust_recharge_multiplier();
        } );
      }

      // actions do not necessarily have unique cooldowns, so we need to go through every action and check to see if the
      // cooldown matches, then add the buff to _cd_buffs so that we don't add duplicate stack_change_callbacks.
      for ( auto a : _player->action_list )
        if ( a->cooldown == _action->cooldown )
          if ( auto tmp = dynamic_cast<parse_action_base_t*>( a ) )
            tmp->_cd_buffs.push_back( i.buff );
    }
  }
}

void parse_action_base_t::initialize_cooldown_buffs()
{
  if ( !_action->cooldown )
    return;

  auto dynamic = range::contains( _player->dynamic_cooldown_list, _action->cooldown );

  process_cooldown_buffs( dynamic, recharge_multiplier_effects );
  process_cooldown_buffs( dynamic, recharge_rate_effects );
}
