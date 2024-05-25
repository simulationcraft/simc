// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "parse_effects.hpp"

#include "dbc/sc_spell_info.hpp"

std::string player_effect_t::value_type_name( parse_flag_e t ) const
{
  switch ( t )
  {
    case USE_DEFAULT: return "Default Value";
    case USE_CURRENT: return "Current Value";
    default:          return "Spell Data";
  }
}

void player_effect_t::print_parsed_line( report::sc_html_stream& os, const sim_t& sim, bool decorate,
                                         std::function<std::string( uint32_t )> note_fn,
                                         std::function<std::string( double )> val_str_fn ) const
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

if ( note_fn )
    notes.emplace_back( note_fn( opt_enum ) );

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
  "<td>{}</td>""</tr>\n",
  decorate ? report_decorators::decorated_spell_data( sim, eff->spell() ) : eff->spell()->name_cstr(),
  eff->spell()->id(),
  eff->index() + 1,
  val_str,
  value_type_name( type ),
  util::string_join( notes ) );
}

double modified_spelleffect_t::base_value() const
{
  auto return_value = value;

  for ( const auto& i : conditional )
  {
    double eff_val = i.value;

    if ( i.func && !i.func() )
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

  os.format(
    "<td rowspan=\"{}\" class=\"dark right\">{}</td>"
    "<td rowspan=\"{}\" class=\"dark\">{}</td>"
    "<td rowspan=\"{}\" class=\"dark\">{}</td>\n",
    c, _eff.index() + 1,
    c, spell_info::effect_type_str( &_eff ),
    c, spell_info::effect_subtype_str( &_eff ) );

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

void modified_spell_data_t::parse_effect( pack_t<modify_effect_t>& tmp, const spell_data_t* s_data, size_t i )
{
  const auto& eff = s_data->effectN( i );
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
    default: return;
  }

  if ( !_spell.affected_by_all( eff ) )
    return;

  auto &effN = effects[ idx ];

  // dupes are not allowed
  if ( range::contains( effN.permanent, &eff ) || range::contains( effN.conditional, &eff, &modify_effect_t::eff ) )
    return;

  if ( tmp.data.value != 0.0 )
  {
    val = tmp.data.value;
    val_mul = 1.0;
  }
  else
  {
    apply_affecting_mods( tmp, val, m, s_data, i );
    val *= val_mul;
  }

  if ( !val )
    return;

  std::string val_str = flat ? fmt::format( "{}", val ) : fmt::format( "{}%", val * 100 );

  // always active
  if ( !tmp.data.buff && !tmp.data.func )
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
    tmp.data.value = val;
    tmp.data.flat = flat;
    tmp.data.eff = &eff;

    effN.conditional.push_back( tmp.data );
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
       << "<th>Type</th>"
       << "<th>Subtype</th>"
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

double parse_effects_t::get_effect_value( const player_effect_t& i, bool benefit ) const
{
  if ( i.func && !i.func() )
    return 0.0;

  double eff_val = i.value;

  if ( i.buff )
  {
    auto stack = benefit ? i.buff->stack() : i.buff->check();
    if ( !stack )
      return 0.0;

    if ( i.type == USE_CURRENT )
      eff_val = i.buff->check_value();

    if ( i.use_stacks )
      eff_val *= stack;
  }

  if ( i.mastery )
    eff_val *= _player->cache.mastery();

  return eff_val;
}
