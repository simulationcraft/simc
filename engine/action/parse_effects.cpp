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
  return opt == 0x7f ? "All" : util::school_type_string( dbc::get_school_type( opt ) );
}

std::string pet_type( uint32_t opt )
{
  return opt ? "Guardian" : "Pet";
}
}  // namespace opt_strings

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

  range::for_each( notes, []( auto& s ) { s[ 0 ] = std::toupper( s[ 0 ] ); } );

  std::string val_str = val_str_fn ? val_str_fn( value )
                        : mastery  ? fmt::format( "{:.5f}", value * 100 )
                                   : fmt::format( "{:.1f}%", value * 100 );

  os.format(
    "<td class=\"left\">{}</td>"
    "<td class=\"right\">{}</td>"
    "<td class=\"right\">{}</td>"
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

  range::for_each( notes, []( auto& s ) { s[ 0 ] = std::toupper( s[ 0 ] ); } );

  std::string val_str = val_str_fn ? val_str_fn( value )
                        : mastery  ? fmt::format( "{:.5f}", value * 100 )
                                   : fmt::format( "{:.1f}%", value * 100 );

  os.format(
    "<td class=\"left\">{}</td>"
    "<td class=\"right\">{}</td>"
    "<td class=\"right\">{}</td>"
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

  os.format(
    "<td>{}</td>"
    "<td class=\"right\">{}</td>"
    "<td class=\"right\">{}</td>"
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
    "<td class=\"right\">{}</td>"
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
    "<td rowspan=\"{}\" class=\"dark right\">{}</td>"
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
    os << "<h3 class=\"toggle\">Modified Spell Data</h3>"
       << "<div class=\"toggle-content hide\">"
       << "<table class=\"sc even left\">\n";

    os << "<thead><tr>"
       << "<th>Spell</th>"
       << "<th>ID</th>"
       << "<th>#</th>"
       << "<th>Effect</th>"
       << "<th>Modified By</th>"
       << "<th>ID</th>"
       << "<th>#</th>"
       << "<th>+/%</th>"
       << "<th>Value</th>"
       << "<th>Notes</th>"
       << "</tr></thead>\n";

    for ( auto m_data : entries )
      m_data->print_parsed_spell( os, sim );

    os << "</table>\n"
       << "</div>\n";
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
    val = eff.mastery_value();
  else
    val = eff.base_value();

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

  if ( pack.data.value != 0.0 )
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
    if ( !val && tmp.type == USE_DATA )
      return false;
  }
  else
  {
    if ( !val )
      return false;
  }

  val *= val_mul;

  std::string val_str = mastery ? fmt::format( "{:.5f}*mastery", val * 100 )
                        : flat  ? fmt::format( "{}", val )
                                : fmt::format( "{:.1f}%", val * ( 1 / val_mul ) );

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

  debug_message( tmp, type_str, val_str, mastery, pack.spell, i );

  tmp.value = val;
  tmp.mastery = mastery;
  tmp.eff = &eff;

  if constexpr ( is_detected_v<detect_simple, U> )
  {
    if ( tmp.func || tmp.value_func || tmp.type & USE_CURRENT || tmp.mastery || !tmp.use_stacks || pack.callback )
    {
      tmp.simple = false;
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
    eff_val *= _player->cache.mastery();

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

  return scc;
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

  return mh;
}

double parse_player_effects_t::composite_spell_haste() const
{
  auto sh = player_t::composite_spell_haste();

  for ( const auto& i : haste_effects )
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

double parse_player_effects_t::matching_gear_multiplier( attribute_e attr ) const
{
  double mg = player_t::matching_gear_multiplier( attr );

  assert( attr != ATTRIBUTE_NONE && "ATTRIBUTE_NONE will be out of index" );

  for ( const auto& i : matching_armor_attribute_multiplier_effects )
    if ( i.opt_enum & ( 1 << ( attr - 1 ) ) )
      mg += get_effect_value( i );

  return mg;
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
      str = opt_strings::attributes_invalidate( tmp );

      if ( eff.spell()->equipped_class() == ITEM_CLASS_ARMOR && eff.spell()->flags( SX_REQUIRES_EQUIPPED_ARMOR_TYPE ) )
      {
        auto type_bit = 1U << static_cast<unsigned>( util::matching_armor_type( type ) );
        if ( eff.spell()->equipped_subclass_mask() == type_bit )
        {
          str += "|with matching armor";
          return &matching_armor_attribute_multiplier_effects;
        }
      }

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
      str = "haste";
      invalidate( CACHE_HASTE );
      return &haste_effects;

    case A_MOD_MASTERY_PCT:
      str = "mastery";
      val_mul = 1.0;
      invalidate( CACHE_MASTERY );
      return &mastery_effects;

    case A_MOD_ALL_CRIT_CHANCE:
      str = "all crit chance";
      invalidate( CACHE_CRIT_CHANCE );
      return &crit_chance_effects;

    case A_MOD_DAMAGE_PERCENT_DONE:
      tmp.opt_enum = eff.misc_value1();
      str = opt_strings::school( tmp.opt_enum );
      str += " damage";
      invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
      return &player_multiplier_effects;

    case A_MOD_PET_DAMAGE_DONE:
      tmp.opt_enum = 0;
      str = "pet damage";
      return &pet_multiplier_effects;

    case A_MOD_GUARDIAN_DAMAGE_DONE:
      tmp.opt_enum = 1;
      str = "guardian damage";
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

    case A_MOD_ABSORB_DONE_PERCENT:
      str = "absorb multiplier";
      return &absorb_multiplier_effects;

    case A_MOD_HEALING_RECEIVED_PCT:
      str = "healing received";
      return &healing_received_effects;

    default:
      return nullptr;
  }

  return nullptr;
}

void parse_player_effects_t::debug_message( const player_effect_t& data, std::string_view type_str,
                                            std::string_view val_str, bool mastery, const spell_data_t* s_data,
                                            size_t i )
{
  auto splits = util::string_split<std::string_view>( type_str, "|" );
  auto tok1 = splits[ 0 ];
  auto tok2 = std::string( val_str );

  if ( splits.size() > 1 )
    tok2 += fmt::format( " {}", splits[ 1 ] );

  if ( data.buff )
  {
    sim->print_debug( "player-effects: {} {} modified by {} {} buff {} ({}#{})", *this, tok1, tok2,
                      data.use_stacks ? "per stack of" : "with", data.buff->name(), data.buff->data().id(), i );
  }
  else if ( mastery && !data.func )
  {
    sim->print_debug( "player-effects: {} {} modified by {} from {} ({}#{})", *this, tok1, tok2, s_data->name_cstr(),
                      s_data->id(), i );
  }
  else if ( data.func )
  {
    sim->print_debug( "player-effects: {} {} modified by {} with condition from {} ({}#{})", *this, tok1, tok2,
                      s_data->name_cstr(), s_data->id(), i );
  }
  else
  {
    sim->print_debug( "player-effects: {} {} modified by {} from {} ({}#{})", *this, tok1, tok2, s_data->name_cstr(),
                      s_data->id(), i );
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
      str = opt_strings::school( tmp.opt_enum );
      return &target_multiplier_effects;

    case A_MOD_DAMAGE_FROM_CASTER_PET:
      tmp.opt_enum = 0;
      str = "pet";
      return &target_pet_multiplier_effects;

    case A_MOD_DAMAGE_FROM_CASTER_GUARDIAN:
      tmp.opt_enum = 1;
      str = "guardian";
      return &target_pet_multiplier_effects;

    default:
      return nullptr;
  }

  return nullptr;
}

void parse_player_effects_t::debug_message( const target_effect_t&, std::string_view type_str, std::string_view val_str,
                                            bool, const spell_data_t* s_data, size_t i )
{
  sim->print_debug( "target-effects: Target {} damage taken modified by {} from {} ({}#{})", type_str, val_str,
                    s_data->name_cstr(), s_data->id(), i );
}

void parse_player_effects_t::parsed_effects_html( report::sc_html_stream& os )
{
  if ( total_effects_count() )
  {
    os << "<h3 class=\"toggle\">Player Effects</h3>"
       << "<div class=\"toggle-content hide\">"
       << "<table class=\"sc left even\">\n";

    os << "<thead><tr>"
       << "<th>Type</th>"
       << "<th>Spell</th>"
       << "<th>ID</th>"
       << "<th>#</th>"
       << "<th>Value</th>"
       << "<th>Source</th>"
       << "<th>Notes</th>"
       << "</tr></thead>\n";

    auto mastery_val = [ this ]( double v ) {
      return fmt::format( "{:.1f}%", v * mastery_coefficient() * 100 );
    };

    print_parsed_type( os, auto_attack_speed_effects, "Auto Attack Speed" );
    print_parsed_type( os, attribute_multiplier_effects, "Attribute Multiplier", &opt_strings::attributes );
    print_parsed_type( os, matching_armor_attribute_multiplier_effects, "Matching Armor", &opt_strings::attributes );
    print_parsed_type( os, rating_multiplier_effects, "Rating Multiplier", &opt_strings::ratings );
    print_parsed_type( os, versatility_effects, "Versatility" );
    print_parsed_type( os, player_multiplier_effects, "Player Multiplier", &opt_strings::school );
    print_parsed_type( os, pet_multiplier_effects, "Pet Multiplier", &opt_strings::pet_type );
    print_parsed_type( os, attack_power_multiplier_effects, "Attack Power Multiplier" );
    print_parsed_type( os, crit_chance_effects, "Crit Chance" );
    print_parsed_type( os, leech_effects, "Leech" );
    print_parsed_type( os, expertise_effects, "Expertise" );
    print_parsed_type( os, crit_avoidance_effects, "Crit Avoidance" );
    print_parsed_type( os, parry_effects, "Parry" );
    print_parsed_type( os, base_armor_multiplier_effects, "Base Armor Multiplier" );
    print_parsed_type( os, armor_multiplier_effects, "Armor Multiplier" );
    print_parsed_type( os, haste_effects, "Haste" );
    print_parsed_type( os, mastery_effects, "Mastery", nullptr, mastery_val );
    print_parsed_type( os, parry_rating_from_crit_effects, "Parry Rating from Crit" );
    print_parsed_type( os, dodge_effects, "Dodge" );
    print_parsed_type( os, absorb_multiplier_effects, "Absorb Multiplier" );
    print_parsed_type( os, healing_received_effects, "Healing Received" );
    print_parsed_type( os, target_multiplier_effects, "Target Multiplier", &opt_strings::school );
    print_parsed_type( os, target_pet_multiplier_effects, "Target Pet Multiplier", &opt_strings::pet_type );
    print_parsed_custom_type( os );

    os << "</table>\n"
       << "</div>\n";
  }
}

size_t parse_player_effects_t::total_effects_count()
{
  return auto_attack_speed_effects.size() +
         attribute_multiplier_effects.size() +
         matching_armor_attribute_multiplier_effects.size() +
         rating_multiplier_effects.size() +
         versatility_effects.size() +
         player_multiplier_effects.size() +
         pet_multiplier_effects.size() +
         attack_power_multiplier_effects.size() +
         crit_chance_effects.size() +
         leech_effects.size() +
         expertise_effects.size() +
         crit_avoidance_effects.size() +
         parry_effects.size() +
         base_armor_multiplier_effects.size() +
         armor_multiplier_effects.size() +
         haste_effects.size() +
         mastery_effects.size() +
         parry_rating_from_crit_effects.size() +
         dodge_effects.size() +
         absorb_multiplier_effects.size() +
         healing_received_effects.size() +
         target_multiplier_effects.size() +
         target_pet_multiplier_effects.size();
}

void parse_action_base_t::parse_callback_function( pack_t<player_effect_t>& pack, parse_cb_t cb )
{
  assert( pack.data.idx == 0 && "cannot parse multiple parse callbacks" );
  // 32 max as parse_effects_t::callback_idx is uint32_t
  assert( callback_list.size() < 32 && "cannot register more than 32 parse callbacks" );

  // set values on main pack, to be propagated to all copies
  pack.callback = std::move( cb );
  // this is set BEFORE the callback is added to the vector, so the idx will be 1bit left of size()
  pack.data.idx = 1U << ( callback_list.size() );
}

void parse_action_base_t::parse_callback_function( pack_t<player_effect_t>& pack, parse_flag_e type )
{
  assert( pack.data.buff && "EXPIRE_BUFF/DECREMENT_BUFF requires a buff" );

  if ( type == EXPIRE_BUFF )
  {
    parse_callback_function( pack, [ a = _action, b = pack.data.buff ]( parse_callback_e cb_type ) {
      if ( cb_type == PARSE_CALLBACK_POST_EXECUTE )
        b->expire( a );
    } );
  }
  else if ( type == DECREMENT_BUFF )
  {
    parse_callback_function( pack, [ a = _action, b = pack.data.buff ]( parse_callback_e cb_type ) {
      if ( cb_type == PARSE_CALLBACK_POST_EXECUTE && b->can_expire( a ) )
        b->decrement();
    } );
  }
}

void parse_action_base_t::register_callback_function( pack_t<player_effect_t>& pack )
{
  callback_list.push_back( std::move( pack.callback ) );

  _player->sim->print_debug( "action-effects: {} ({}) registering parse callback on {} {} ({})", _action->name(),
                             _action->id, pack.data.buff ? "buff" : "spell", pack.spell->name_cstr(),
                             pack.spell->id() );
}

void parse_action_base_t::trigger_callbacks( parse_callback_e cb_type )
{
  if ( callback_idx )
    for ( size_t i = 0; i < callback_list.size(); i++ )
      if ( callback_idx & ( 1U << i ) )
        callback_list[ i ]( cb_type );
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

  if ( !force && !_action->data().affected_by_all( eff ) )
    return nullptr;

  if ( eff.subtype() == A_ADD_PCT_MODIFIER || eff.subtype() == A_ADD_PCT_LABEL_MODIFIER )
  {
    switch ( eff.misc_value1() )
    {
      case P_GENERIC:       str = "direct damage";          return &da_multiplier_effects;
      case P_DURATION:      str = "duration";               return &dot_duration_effects;
      case P_TICK_DAMAGE:   str = "tick damage";            return &ta_multiplier_effects;
      case P_CAST_TIME:     str = "cast time";              return &execute_time_effects;
      case P_GCD:           str = "gcd";                    return &gcd_effects;
      case P_TICK_TIME:     str = "tick time";              return &tick_time_effects;
      case P_RESOURCE_COST: str = "cost percent";           return &cost_effects;
      case P_CRIT:          str = "crit chance multiplier"; return &crit_chance_multiplier_effects;
      case P_CRIT_BONUS:    str = "crit bonus multiplier";  return &crit_bonus_effects;
      case P_COOLDOWN:      str = "cooldown";               return &recharge_multiplier_effects;
      default:              return nullptr;
    }
  }
  else if ( eff.subtype() == A_ADD_FLAT_MODIFIER || eff.subtype() == A_ADD_FLAT_LABEL_MODIFIER )
  {
    flat = true;

    switch ( eff.misc_value1() )
    {
      case P_CAST_TIME:     val_mul = 1.0;
                            str = "cast time";   return &flat_execute_time_effects;
      case P_TICK_TIME:     val_mul = 1.0;
                            str = "tick time";   return &flat_tick_time_effects;
      case P_CRIT:          str = "crit chance"; return &crit_chance_effects;
      case P_RESOURCE_COST: val_mul = spelleffect_data_t::resource_multiplier( _action->current_resource() );
                            str = "flat cost";   return &flat_cost_effects;
      default:              return nullptr;
    }
  }
  else if ( eff.subtype() == A_MOD_RECHARGE_TIME_PCT_CATEGORY )
  {
    str = "cooldown";
    return &recharge_multiplier_effects;
  }
  else if ( ( eff.subtype() == A_MOD_CHARGE_RECHARGE_RATE && _action->data().charges() ) ||
            ( ( eff.subtype() == A_MOD_RECHARGE_RATE || eff.subtype() == A_MOD_RECHARGE_RATE_LABEL ) &&
              !_action->data().charges() ) )
  {
    str = "recharge rate";
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
                                         std::string_view val_str, bool mastery, const spell_data_t* s_data, size_t i )
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

    _action->sim->print_debug( "action-effects: {} ({}) {} modified by {} {} buff {} ({}#{})", _action->name(),
                               _action->id, tok1, tok2, stack_str, data.buff->name(), data.buff->data().id(), i );
  }
  else if ( mastery && !data.func )
  {
    _action->sim->print_debug( "action-effects: {} ({}) {} modified by {} from {} ({}#{})", _action->name(),
                               _action->id, tok1, tok2, s_data->name_cstr(), s_data->id(), i );
  }
  else if ( data.func )
  {
    _action->sim->print_debug( "action-effects: {} ({}) {} modified by {} with condition from {} ({}#{})",
                               _action->name(), _action->id, tok1, tok2, s_data->name_cstr(), s_data->id(), i );
  }
  else
  {
    _action->sim->print_debug( "action-effects: {} ({}) {} modified by {} from {} ({}#{})", _action->name(),
                               _action->id, tok1, tok2, s_data->name_cstr(), s_data->id(), i );
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

  if ( !force && !_action->data().affected_by_all( eff ) )
    return nullptr;

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
                                         bool, const spell_data_t* s_data, size_t i )
{
  _action->sim->print_debug( "target-effects: {} ({}) {} modified by {} on targets with debuff {} ({}#{})",
                             _action->name(), _action->id, type_str, val_str, s_data->name_cstr(), s_data->id(), i );
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
      if ( _action->data().class_flag( std::abs( f ) ) )
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

void parse_action_base_t::parsed_effects_html( report::sc_html_stream& os )
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
       << "<th class=\"small\">ID</th>"
       << "<th class=\"small\">#</th>"
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
    print_parsed_type( os, &VEC::gcd_effects, "GCD" );
    print_parsed_type( os, &VEC::flat_dot_duration_effects, "Flat Duration", nullptr, timespan_fn );
    print_parsed_type( os, &VEC::dot_duration_effects, "Percent Duration" );
    print_parsed_type( os, &VEC::flat_tick_time_effects, "Flat Tick Time", nullptr, timespan_fn );
    print_parsed_type( os, &VEC::tick_time_effects, "Percent Tick Time" );
    print_parsed_type( os, &VEC::recharge_multiplier_effects, "Recharge Multiplier" );
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

size_t parse_action_base_t::total_effects_count()
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
