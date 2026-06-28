// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "config.hpp"

#include "action/action.hpp"
#include "action/action_state.hpp"
#include "action/attack.hpp"
#include "action/dot.hpp"
#include "action/heal.hpp"
#include "action/residual_action.hpp"
#include "action/spell.hpp"
#include "class_modules/apl/apl_shaman.hpp"
#include "class_modules/class_module.hpp"
#include "dbc/data_enums.hh"
#include "dbc/dbc.hpp"
#include "item/item.hpp"
#include "item/special_effect.hpp"
#include "player/action_priority_list.hpp"
#include "player/actor_target_data.hpp"
#include "player/ground_aoe.hpp"
#include "player/pet.hpp"
#include "player/pet_spawner.hpp"
#include "action/action_callback.hpp"
#include "action/parse_effects.hpp"
#include "report/highchart.hpp"
#include "player/player_scaling.hpp"
#include "player/set_bonus.hpp"
#include "report/decorators.hpp"
#include "sc_enums.hpp"
#include "sim/cooldown.hpp"
#include "sim/proc.hpp"
#include "sim/proc_rng.hpp"
#include "util/string_view.hpp"

#include <cassert>
#include <string>
#include <sstream>

// TODO 12.0
//
// Enhancement
// TODO-midnight-talent: Does Overcharge include target-based crit chance% debuffs?
// TODO-midnight-talent: Do all nature damage benefit from Overcharge, or just "abilities"?
// TODO-midnight-talent: Ride the Lightning interactions with Arc Discharge, Thorim's etc?
//
// Elemental
// TODO-midnight-talents:

namespace eff
{
template <typename BUILDER, typename OBJ>
class effect_builder_base_t
{
  protected:
    const spell_data_t*              m_spell = spell_data_t::nil();
    buff_t*                          m_buff = nullptr;
    std::vector<const spell_data_t*> m_list;
    std::function<bool()>            m_state_fn = nullptr;
    unsigned                         m_flags = 0U;
    double                           m_value = 0.0;
    std::function<double(double)>    m_value_fn = nullptr;
    effect_mask_t                    m_mask = { true };
    std::vector<affect_list_t>       m_affect_list;

  public:
    effect_builder_base_t() = delete;

    effect_builder_base_t( buff_t* b ) : m_buff( b )
    { }

    effect_builder_base_t( const spell_data_t* s_ptr ) : m_spell( s_ptr )
    { }

    effect_builder_base_t( const spell_data_t& s ) : m_spell( &( s ) )
    { }

    virtual ~effect_builder_base_t() = default;

    const spell_data_t* target() const
    {
      if ( m_spell->ok() )
      {
        return m_spell;
      }
      else if ( m_buff && m_buff->data().ok() )
      {
        return &( m_buff->data() );
      }

      return nullptr;
    }

    BUILDER& add_affecting_spell( const spell_data_t* s_ptr )
    {
      m_list.emplace_back( s_ptr );
      return *debug_cast<BUILDER*>( this );
    }

    template <typename... ARGS>
    BUILDER& add_affecting_spell( const spell_data_t* s_ptr, ARGS... args )
    {
      add_affecting_spell( s_ptr );
      return add_affecting_spell( args... );
    }

    BUILDER& add_affecting_spell( const spell_data_t& s )
    { return add_affecting_spell( &( s ) ); }

    template <typename... ARGS>
    BUILDER& add_affecting_spell( const spell_data_t& s, ARGS... args )
    {
      add_affecting_spell( s );
      return add_affecting_spell( args... );
    }

    BUILDER& set_state_fn( std::function<bool()> fn )
    {
      m_state_fn = std::move( fn );
      return *debug_cast<BUILDER*>( this );
    }

    BUILDER& set_flag( parse_flag_e flag )
    {
      m_flags |= flag;
      return *debug_cast<BUILDER*>( this );
    }

    template <typename... ARGS>
    BUILDER& set_flag( parse_flag_e flag, ARGS... args )
    {
      set_flag( flag );
      return set_flag( args... );
    }

    BUILDER& unset_flag( parse_flag_e flag )
    {
      m_flags &= ~flag;
      return *debug_cast<BUILDER*>( this );
    }

    template <typename... ARGS>
    BUILDER& unset_flag( parse_flag_e flag, ARGS... args )
    {
      unset_flag( flag );
      return unset_flag( args... );
    }

    BUILDER& set_value( double v )
    {
      m_value = v;
      return *debug_cast<BUILDER*>( this );
    }

    BUILDER& set_value( const std::function<double(double)>& fn )
    {
      m_value_fn = fn;
      return *debug_cast<BUILDER*>( this );
    }

    BUILDER& set_effect_mask( effect_mask_t mask )
    {
      m_mask = std::move( mask );
      return *debug_cast<BUILDER*>( this );
    }

    BUILDER& add_affect_list( affect_list_t list )
    {
      m_affect_list.emplace_back( std::move( list ) );
      return *debug_cast<BUILDER*>( this );
    }

    template <typename... ARGS>
    BUILDER& add_affect_list( affect_list_t list, ARGS... args )
    {
      add_affect_list( list );
      return add_affect_list( args... );
    }

    OBJ create_base() const
    {
      OBJ pe( target() );

      if ( m_buff )
      {
        pe.data.buff = m_buff;
      }

      if ( !m_list.empty() )
      {
        pe.list = m_list;
      }

      if ( m_state_fn )
      {
        pe.data.func = m_state_fn;
      }

      if ( m_flags & IGNORE_STACKS )
      {
        pe.data.use_stacks = false;
      }

      if ( m_flags & USE_CURRENT )
      {
        pe.data.type |= USE_CURRENT;
      }

      if ( m_flags & USE_DEFAULT )
      {
        pe.data.type |= USE_DEFAULT;
      }

      if ( m_value != 0.0 )
      {
        pe.data.value = m_value;
        pe.data.type &= ~( USE_DEFAULT | USE_CURRENT );
        pe.data.type |= VALUE_OVERRIDE;
      }

      if ( m_value_fn )
      {
        pe.data.value_func = m_value_fn;
        pe.data.type &= ~( USE_DEFAULT | USE_CURRENT | VALUE_OVERRIDE );
        pe.data.type |= VALUE_FUNCTION;
      }

      pe.mask = m_mask;

      if ( !m_affect_list.empty() )
      {
        pe.affect_lists = m_affect_list;
      }

      return pe;
    }

    virtual void build( parse_effects_t* obj ) const = 0;
};

class source_eff_builder_t : public effect_builder_base_t<source_eff_builder_t, pack_t<player_effect_t>>
{
public:
  source_eff_builder_t( buff_t* b ) :
    effect_builder_base_t<source_eff_builder_t, pack_t<player_effect_t>>( b )
  { }

  source_eff_builder_t( const spell_data_t* s_ptr ) :
    effect_builder_base_t<source_eff_builder_t, pack_t<player_effect_t>>( s_ptr )
  { }

  source_eff_builder_t( const spell_data_t& s ) :
    effect_builder_base_t<source_eff_builder_t, pack_t<player_effect_t>>( s )
  { }

  void build( parse_effects_t* obj ) const override
  {
    if ( !this->target() )
    {
      return;
    }

    auto pe = this->create_base();

    for ( auto idx = 1U; idx <= this->target()->effect_count(); ++idx )
    {
      if ( pe.mask & 1 << ( idx - 1U ) )
      {
        continue;
      }

      // local copy of pack per effect
      auto tmp = pe;

      obj->parse_effect( tmp, idx, false );
    }
  }
};
} // Namespace eff ends

namespace stats
{
class proc_tracker_t
{
  const spell_data_t* m_source;
  const action_t*     m_action;
  proc_t*             m_proc;
  proc_t*             m_total;

public:
  proc_tracker_t( const spell_data_t* s, const action_t* a, proc_t* total ) :
    m_source( s ), m_action( a ), m_total( total )
  {
    m_proc = m_action->player->get_proc( fmt::format( "{}: {}",
      s->name_cstr(),
      m_action->name() ),
      proc_report_e::REPORT_PROC_JSON | proc_report_e::REPORT_PROC_TEXT );
  }

  const action_t* action() const
  { return m_action; }

  const spell_data_t* source() const
  { return m_source; }

  std::string decorated_action_str() const
  { return report_decorators::decorated_action( *action() ); }

  std::string decorated_source_str() const
  { return report_decorators::decorated_spell_data( *action()->sim, source() ); }

  const proc_t* proc() const
  { return m_proc; }

  virtual ~proc_tracker_t() = default;

  void occur()
  {
    m_total->occur();
    m_proc->occur();
  }
};

class proc_track_db_t
{
  class tracker_entry_t
  {
    const spell_data_t*          m_proc_spell;
    proc_t*                      m_total_procs;
    std::vector<std::unique_ptr<proc_tracker_t>> m_sources;

    public:
      tracker_entry_t( const spell_data_t* proc_spell ) :
        m_proc_spell( proc_spell ), m_total_procs( nullptr )
      { }

      proc_t* total()
      { return m_total_procs; }

      bool has_data() const
      { return m_total_procs->count.mean() > 0.0; }

      const spell_data_t* proc_spell() const
      { return m_proc_spell; }

      std::vector<proc_tracker_t*> report_entries() const
      {
        std::vector<proc_tracker_t*> e;

        for ( auto& entry : m_sources )
        {
          if ( entry->proc()->count.mean() > 0.0 )
          {
            e.emplace_back( entry.get() );
          }
        }

        return e;
      }

      void sort()
      {
        std::sort( m_sources.begin(), m_sources.end(), []( auto& a, auto& b ) {
          if ( &a == &b )
          {
            return false;
          }

          return a->proc()->name_str < b->proc()->name_str;
        } );
      }

      proc_tracker_t* register_proc( const action_t* a )
      {
        if ( m_total_procs == nullptr )
        {
          auto total_str = fmt::format( "{}: Total", m_proc_spell->name_cstr() );

          m_total_procs = a->player->get_proc( total_str,
            proc_report_e::REPORT_PROC_JSON | proc_report_e::REPORT_PROC_TEXT );
        }

        auto it = range::find_if( m_sources, [ a ]( std::unique_ptr<proc_tracker_t>& pt ) {
          return pt->action()->id == a->id;
        } );

        if ( it != m_sources.end() )
        {
          return it->get();
        }
        else
        {
          return m_sources.emplace_back( new proc_tracker_t( m_proc_spell, a, m_total_procs ) ).get();
        }
      }
  };

  std::vector<tracker_entry_t> m_db;

public:
  proc_track_db_t( player_t* /* p */ )
  { }

  virtual ~proc_track_db_t() = default;

  bool has_data() const
  {
    for ( auto& entry : m_db )
    {
      if ( entry.has_data() )
      {
        return true;
      }
    }

    return false;
  }

  void sort()
  {
    std::sort( m_db.begin(), m_db.end(), []( auto& a, auto& b ) {
      if ( &a == &b )
      {
        return false;
      }

      return strcmp( a.proc_spell()->name_cstr(), b.proc_spell()->name_cstr() ) < 0;
    } );

    for ( auto& entry : m_db )
    {
      if ( !entry.has_data() )
      {
        continue;
      }

      entry.sort();
    }
  }

  proc_tracker_t* register_proc( const spell_data_t* proc, const action_t* source )
  {
    auto proc_it = range::find_if( m_db, [ proc ]( const auto& entry ) {
        return proc->id() == entry.proc_spell()->id();
    } );

    if ( proc_it == m_db.end() )
    {
      auto& entry = m_db.emplace_back( proc );

      return entry.register_proc( source );
    }
    else
    {
      return proc_it->register_proc( source );
    }
  }

  void output_html( report::sc_html_stream& os )
  {
    unsigned row = 0U;

    sort();

    os << R"(<table class="sc stripebody">)" << "\n"
      << "<thead>\n"
      << "<tr>\n";

    os << R"(<th class="left">Proc Spell</th>)"
        << R"(<th class="left">Source Ability</th>)"
        << R"(<th class="left">Count</th>)"
        << R"(<th class="left">Min</th>)"
        << R"(<th class="left">Max</th>)"
        << R"(<th class="left">Interval</th>)"
        << R"(<th class="left">Min</th>)"
        << R"(<th class="left">Max</th>)"
        << "\n";

    os << "</tr>\n"
        << "</thead>\n"
        << "<tbody>\n";

    for ( auto& entry : m_db )
    {
      if ( !entry.has_data() )
      {
        continue;
      }

      auto sources = entry.report_entries();
      bool first = true;

      for ( auto tracker : sources )
      {
        os << fmt::format( R"(<tr class="{}">)""\n", row++ & 1 ? "odd" : "even" );
        if ( first )
        {
          os << fmt::format( R"(<td class="left" rowspan="{}">{}</td>)",
            sources.size() + 1,
            report_decorators::decorated_spell_data( *sources.front()->action()->sim,
              entry.proc_spell() ) );
          first = false;
        }

        os << fmt::format(
          R"(<td class="left">{}</td>)"
          "<td>{:.1f}</td>"
          "<td>{:.1f}</td>"
          "<td>{:.1f}</td>"
          "<td>{:.1f}s</td>"
          "<td>{:.1f}s</td>"
          "<td>{:.1f}s</td>\n",
          report_decorators::decorated_action( *tracker->action() ),
          tracker->proc()->count.mean(),
          tracker->proc()->count.min(),
          tracker->proc()->count.max(),
          tracker->proc()->interval_sum.mean(),
          tracker->proc()->interval_sum.min(),
          tracker->proc()->interval_sum.max() );

        os << "</tr>\n";
      }

      os << fmt::format( R"(<tr class="{}">)""\n", row++ & 1 ? "odd" : "even" );

      os << fmt::format(
        R"(<td class="left"><strong>Total</strong></td>)"
        "<td>{:.1f}</td>"
        "<td>{:.1f}</td>"
        "<td>{:.1f}</td>"
        "<td>{:.1f}s</td>"
        "<td>{:.1f}s</td>"
        "<td>{:.1f}s</td>\n",
        entry.total()->count.mean(),
        entry.total()->count.min(),
        entry.total()->count.max(),
        entry.total()->interval_sum.mean(),
        entry.total()->interval_sum.min(),
        entry.total()->interval_sum.max() );

      os << "</tr>\n";


    }

    os << "</tbody>\n"
        << "</table>\n";
  }
};
} // Namespace stats ends

namespace rng
{
class dre_deck_rng_t : public shuffled_rng_t
{
private:
  size_t  m_success,   // Number of successes
          m_high_idx;  // Index of the highest successful draw
  int     m_max_draw;  // Maximum number of cards to draw per proc event
public:
  dre_deck_rng_t( std::string_view n, player_t* p, initializer data ) = delete;

  dre_deck_rng_t( std::string_view n, player_t* p, unsigned success_entries, unsigned total_entries, int max_draw ) :
    shuffled_rng_t( n, p, success_entries, total_entries ), m_success( success_entries ),
    m_high_idx( 0U ), m_max_draw( max_draw )
  { }

  void reset( reset_type_e reset_type ) override
  {
    // Generate full set of fail conditions
    range::fill( entries, FAIL );

    std::vector<size_t> pos;
    // Distance from the high success index to the end of the previous deck
    auto end_distance = reset_type == reset_type_e::ITERATION
      ? m_max_draw + 1
      : entries.size() - m_high_idx;

    m_high_idx = 0U;
    // Generate randomized m_success number of draws, that honor:
    // 1) The draw must be at least max_draw number of draws away from the previous deck's
    //    highest successful draw position
    // 2) The successful draws must be spaced at least max_draw number of draws away from eachother
    //
    // These constraints guarantee that no single (per resource) event can draw two successful DRE
    // procs from the deck.
    for ( auto i = 0U; i < m_success; ++i )
    {
      auto rng_idx = 0U;
      auto shuffle_attempts = 0U; // Cap shuffle attempts if people use weird options
      bool gap = false;
      do {
        rng_idx = player->rng().range( 0U, as<unsigned>( entries.size() ) );

        // Ensure that there is enough of a gap (at least max_draw) between the existing successes
        // and the new randomized success position
        gap = pos.empty() || range::find_if( pos, [ rng_idx, this ]( size_t idx ) {
          int distance = rng_idx - as<int>( idx );
          return ( distance >= 0 && distance <= m_max_draw ) ||
                 ( distance < 0 && distance >= -m_max_draw );
        } ) == pos.end();

        if ( ++shuffle_attempts > 100 )
        {
          range::fill( entries, FAIL );
          position = entries.begin();
          throw sc_runtime_error( fmt::format( "{} unable to find success card position for dre_deck_rng_t.", *player ) );
          return;
        }
      } while ( as<int>( end_distance + rng_idx ) <= m_max_draw || !gap );

      if ( rng_idx > m_high_idx )
      {
        m_high_idx = rng_idx;
      }

      entries[ rng_idx ] = SUCCESS;
      pos.emplace_back( rng_idx );

      player->sim->print_debug(
        "{} DRE deck reset type={}, success={}, index={}, gap={}, prev_dist={}, high_idx={}, max_draw={}, deck_size={}",
        player->name(), static_cast<int>( reset_type ), i, rng_idx, gap, as<int>( end_distance + rng_idx ), m_high_idx,
        m_max_draw, entries.size() );
    }

    position = entries.begin();

#ifndef NDEBUG
    // Validate gaps
    size_t seek_start = 0U;
    for ( auto i = 0U; i < m_success; ++i )
    {
      for ( auto idx = seek_start; idx < entries.size(); ++idx )
      {
        if ( !entries[ idx ] )
        {
          continue;
        }

        if ( i == 0 )
        {
          assert( end_distance + idx > as<size_t>( m_max_draw ) &&
            "Distance from previous success is less than max draw" );
        }
        else
        {
          assert( idx - seek_start + 1 > as<size_t>( m_max_draw ) &&
            "Distance from previous success is less than max draw" );
        }
        seek_start = idx + 1;
        break;
      }
    }
#endif // NDEBUG
  }
};

template <typename T>
class deck_rng_wrapper_t
{
  public:
  using param_fn_t = std::function<std::tuple<unsigned, unsigned, unsigned>(deck_rng_wrapper_t&)>;

  private:
  std::string     m_name;
  player_t*       m_actor;
  shuffled_rng_t* m_rng;

  unsigned        m_total;
  int             m_success;

  // Deck parametrization callback, returns a tuple <total_cards, success_cards, max_card_draw>
  param_fn_t      m_param_fn;

  public:
  deck_rng_wrapper_t() = default;

  deck_rng_wrapper_t( util::string_view name_, player_t* a_ ) : m_name( name_ ), m_actor( a_ ),
    m_rng( nullptr ), m_total( 0U ), m_success( -1 )
  { }

  unsigned opt_total() const
  { return m_total; }

  int opt_success() const
  { return m_success; }

  player_t* actor() const
  { return m_actor; }

  bool trigger() const
  {
      if ( m_rng == nullptr )
      {
          return false;
      }

      return m_rng->trigger();
  }

  void create_options()
  {
    std::string success_str = fmt::format( "shaman.{}_deck_success", m_name );
    std::string total_str = fmt::format( "shaman.{}_deck_total", m_name );

    m_actor->add_option( opt_uint( total_str, m_total, 0U, 10000U ) );
    m_actor->add_option( opt_int( success_str, m_success, 0U, 10000U ) );
  }

  std::unique_ptr<expr_t> create_expression( util::string_view name ) const
  {
    if ( util::str_compare_ci( name, fmt::format( "{}_proc_left", m_name ) ) )
    {
      return make_fn_expr( name, [ rng = this->m_rng ]() {
        return rng->count_remains( shuffled_rng_e::SUCCESS );
      } );
    }
    else if ( util::str_compare_ci( name, fmt::format( "{}_fail_left", m_name ) ) )
    {
      return make_fn_expr( name, [ rng = this->m_rng ]() {
        return rng->count_remains( shuffled_rng_e::FAIL );
      } );
    }
    else if ( util::str_compare_ci( name, fmt::format( "{}_draws_left", m_name ) ) )
    {
      return make_fn_expr( name, [ rng = this->m_rng ]() {
        return rng->entry_remains();
      } );
    }

    return nullptr;
  }

  deck_rng_wrapper_t& set_param_fn( const param_fn_t& fn_ )
  { m_param_fn = fn_; return *this; }

  void build()
  {
    auto params = m_param_fn( *this );

    if ( std::get<1>( params ) == 0 )
    {
        return;
    }

    if ( std::get<0>( params ) < std::get<1>( params ) * ( std::get<2>( params ) + 1 ) )
    {
      m_actor->sim->error(
        "{} cannot build a deck {} with parameters draws_per_event={}, "
        "successful_cards={}, total_cards={}",
        m_actor->name(), m_name, std::get<2>( params ), std::get<1>( params ),
        std::get<0>( params ) );
      m_actor->sim->cancel();
      return;
    }

    m_actor->sim->print_debug( "{} constructing dre_deck {}, n_success={}, n_total={}, max_draw={}",
      m_actor->name(), m_name, std::get<1>( params ), std::get<0>( params ), std::get<2>( params ) );

    m_rng = m_actor->get_rng<T>( m_name,
      std::get<1>( params ), std::get<0>( params ), std::get<2>( params )
    );
  }
};
} // Namespace rng ends

// ==========================================================================
// Shaman
// ==========================================================================

// Dragonflight TODO
//
// Elemental
// - Liquid Magma Totem: Randomize target
// - Inundate
//
// Enhancement
// - Review target caps

// Resto DPS?

namespace
{  // UNNAMED NAMESPACE

struct shaman_t;

enum class mw_proc_state
{
  DEFAULT,
  ENABLED,
  DISABLED
};

enum wolf_type_e
{
  SPIRIT_WOLF = 0,
  FIRE_WOLF,
  FROST_WOLF,
  LIGHTNING_WOLF,
  UNSPECIFIED
};

enum class feral_spirit_cast : unsigned
{
  NORMAL = 0U,
  ROLLING_THUNDER
};

enum class elemental
{
  GREATER_FIRE,
  PRIMAL_FIRE,
  GREATER_STORM,
  PRIMAL_STORM,
  GREATER_EARTH,
  PRIMAL_EARTH
};

enum class spell_variant : unsigned
{
  NORMAL = 0,
  ASCENDANCE,
  DEEPLY_ROOTED_ELEMENTS,
  TWW3_SPELL,
  THORIMS_INVOCATION,
  FUSION_OF_ELEMENTS,
  ARC_DISCHARGE,
  EARTHSURGE,
  PRIMORDIAL_STORM,
  RIDE_THE_LIGHTNING,
  PURGING_FLAMES,
  VOLTAIC_BLAZE,
  MAX
};

static constexpr std::array<util::string_view, static_cast<unsigned>( spell_variant::MAX )>
_variant_suffix {
  "",
  "_asc",
  "_dre",
  "",
  "_ti",
  "_foe",
  "_ad",
  "_es",
  "_ps",
  "_rtl",
  "_pf",
  ""
};

static constexpr std::array<util::string_view, static_cast<unsigned>( spell_variant::MAX )>
_variant_str {
  "normal",
  "ascendance",
  "deeply_rooted_elements",
  "tww3",
  "thorims_invocation",
  "fusion_of_elements",
  "arc_discharge",
  "earthsurge",
  "primordial_storm",
  "ride_the_lightning",
  "purging_flames",
  "voltaic_blaze"
};

enum class strike_variant : unsigned
{
  NORMAL = 0,
  STORMFLURRY,
};

enum class ancestor_cast : unsigned
{
  LAVA_BURST = 0,
  CHAIN_LIGHTNING,
  ELEMENTAL_BLAST,
  DISABLED
};

enum imbue_e
{
  IMBUE_NONE = 0,
  FLAMETONGUE_IMBUE,
  WINDFURY_IMBUE,
  EARTHLIVING_IMBUE,
  THUNDERSTRIKE_WARD
};

enum rotation_type_e
{
  ROTATION_INVALID,
  ROTATION_STANDARD,
  ROTATION_SIMPLE,
  ROTATION_FUNNEL
};

static std::vector<std::pair<util::string_view, rotation_type_e>> __rotation_options = {
  { "simple",   ROTATION_SIMPLE   },
  { "standard", ROTATION_STANDARD },
  { "funnel",   ROTATION_FUNNEL   },
};

static rotation_type_e parse_rotation( util::string_view rotation_str )
{
  auto it = range::find_if( __rotation_options, [ rotation_str ]( const auto& entry ) {
    return util::str_compare_ci( entry.first, rotation_str );
  } );

  if ( it != __rotation_options.end() )
  {
    return it->second;
  }

  return ROTATION_INVALID;
}

static std::string rotation_options()
{
  std::vector<util::string_view> opts;
  range::for_each( __rotation_options, [ &opts ]( const auto& entry ) {
    opts.emplace_back( entry.first );
  } );

  return util::string_join( opts, ", " );
}

/**
  Check_distance_targeting is only called when distance_targeting_enabled is true. Otherwise,
  available_targets is called.  The following code is intended to generate a target list that
  properly accounts for range from each target during chain lightning.  On a very basic level, it
  starts at the original target, and then finds a path that will hit 4 more, if possible.  The
  code below randomly cycles through targets until it finds said path, or hits the maximum amount
  of attempts, in which it gives up and just returns the current best path.  I wouldn't be
  terribly surprised if Blizz did something like this in game.


  TODO: Electrified Shocks?
**/
static std::vector<player_t*>& __check_distance_targeting( const action_t* action, std::vector<player_t*>& tl )
{
  sim_t* sim = action->sim;
  if ( !sim->distance_targeting_enabled )
  {
    return tl;
  }

  player_t* target = action->target;
  player_t* player = action->player;
  double radius    = action->radius;
  int aoe          = action->aoe;

  player_t* last_chain;  // We have to track the last target that it hit.
  last_chain = target;
  std::vector<player_t*>
      best_so_far;  // Keeps track of the best chain path found so far, so we can use it if we give up.
  std::vector<player_t*> current_attempt;
  best_so_far.push_back( last_chain );
  current_attempt.push_back( last_chain );

  size_t num_targets  = sim->target_non_sleeping_list.size();
  size_t max_attempts = static_cast<size_t>(
      std::min( ( num_targets - 1.0 ) * 2.0, 30.0 ) );  // With a lot of targets this can get pretty high. Cap it at 30.
  size_t local_attempts = 0;
  size_t attempts = 0;
  size_t chain_number = 1;
  std::vector<player_t*> targets_left_to_try(
      sim->target_non_sleeping_list.data() );  // This list contains members of a vector that haven't been tried yet.
  auto position = std::find( targets_left_to_try.begin(), targets_left_to_try.end(), target );
  if ( position != targets_left_to_try.end() )
    targets_left_to_try.erase( position );

  std::vector<player_t*> original_targets(
      targets_left_to_try );  // This is just so we don't have to constantly remove the original target.

  bool stop_trying = false;

  while ( !stop_trying )
  {
    local_attempts = 0;
    attempts++;
    if ( attempts >= max_attempts )
      stop_trying = true;
    while ( !targets_left_to_try.empty() && local_attempts < num_targets * 2 )
    {
      player_t* possibletarget;
      size_t rng_target = static_cast<size_t>(
          sim->rng().range( 0.0, ( static_cast<double>( targets_left_to_try.size() ) - 0.000001 ) ) );
      possibletarget = targets_left_to_try[ rng_target ];

      double distance_from_last_chain = last_chain->get_player_distance( *possibletarget );
      if ( distance_from_last_chain <= radius + possibletarget->combat_reach )
      {
        last_chain = possibletarget;
        current_attempt.push_back( last_chain );
        targets_left_to_try.erase( targets_left_to_try.begin() + rng_target );
        chain_number++;
      }
      else
      {
        // If there is no hope of this target being chained to, there's no need to test it again
        // for other possibilities.
        if ( distance_from_last_chain > ( ( radius + possibletarget->combat_reach ) * ( aoe - chain_number ) ) )
          targets_left_to_try.erase( targets_left_to_try.begin() + rng_target );
        local_attempts++;  // Only count failures towards the limit-cap.
      }
      // If we run out of targets to hit, or have hit 5 already. Break.
      if ( static_cast<int>( current_attempt.size() ) == aoe || current_attempt.size() == num_targets )
      {
        stop_trying = true;
        break;
      }
    }
    if ( current_attempt.size() > best_so_far.size() )
      best_so_far = current_attempt;

    current_attempt.clear();
    current_attempt.push_back( target );
    last_chain          = target;
    targets_left_to_try = original_targets;
    chain_number        = 1;
  }

  if ( sim->log )
    sim->out_debug.printf( "%s Total attempts at finding path: %.3f - %.3f targets found - %s target is first chain",
                           player->name(), static_cast<double>( attempts ), static_cast<double>( best_so_far.size() ),
                           target->name() );
  tl.swap( best_so_far );
  return tl;
}

static std::string elemental_name( elemental type )
{
  std::string name_;

  switch ( type )
  {
    case elemental::GREATER_FIRE:
      name_ += "fire_elemental";
      break;
    case elemental::PRIMAL_FIRE:
      name_ += "primal_fire_elemental";
      break;
    case elemental::GREATER_STORM:
      name_ += "storm_elemental";
      break;
    case elemental::PRIMAL_STORM:
      name_ += "primal_storm_elemental";
      break;
    case elemental::GREATER_EARTH:
      name_ += "earth_elemental";
      break;
    case elemental::PRIMAL_EARTH:
      name_ += "primal_earth_elemental";
      break;
    default:
      assert( 0 );
  }

  return name_;
}

static bool is_pet_elemental( elemental type )
{
  return type == elemental::PRIMAL_FIRE || type == elemental::PRIMAL_STORM || type == elemental::PRIMAL_EARTH;
}

static bool elemental_autoattack( elemental type )
{
  return type == elemental::PRIMAL_EARTH || type == elemental::GREATER_EARTH;
}

static util::string_view ancestor_cast_str( ancestor_cast cast )
{
  switch ( cast )
  {
    case ancestor_cast::LAVA_BURST: return "lava_burst";
    case ancestor_cast::CHAIN_LIGHTNING: return "chain_lightning";
    case ancestor_cast::ELEMENTAL_BLAST: return "elemental_blast";
    default: return "disabled";
  }
}

static std::string action_name( util::string_view name, unsigned variants_ )
{
  std::string name_ { name };

  for ( auto bit = static_cast<unsigned>( spell_variant::NORMAL );
    bit < static_cast<unsigned>( spell_variant::MAX ); ++bit )
  {
    if ( variants_ & ( 1 << bit ) )
    {
      name_ += _variant_suffix[ bit ];
    }
  }

  return name_;
}

static std::vector<util::string_view> exec_type_str( unsigned variants_ )
{
  std::vector<util::string_view> strings_;

  for ( auto bit = static_cast<unsigned>( spell_variant::NORMAL );
    bit < static_cast<unsigned>( spell_variant::MAX ); ++bit )
  {
    if ( variants_ & ( 1 << bit ) )
    {
      strings_.emplace_back( _variant_str[ bit ] );
    }
  }

  return strings_;
}

template <typename... Args>
unsigned variant_flag( Args... var_ )
{
  unsigned ret = 0;

  for ( const auto var : { var_... } )
  {
    ret |= 1 << static_cast<unsigned>( var );
  }

  return ret;
}

struct shaman_attack_t;
struct shaman_spell_t;
struct shaman_heal_t;

template <typename T>
struct shaman_totem_pet_t;

template <typename T>
struct totem_pulse_event_t;

template <typename T>
struct totem_pulse_action_t;

using spell_totem_action_t = totem_pulse_action_t<parse_action_effects_t<spell_t>>;
using spell_totem_pet_t = shaman_totem_pet_t<parse_action_effects_t<spell_t>>;
using heal_totem_action_t = totem_pulse_action_t<parse_action_effects_t<heal_t>>;
using heal_totem_pet_t = shaman_totem_pet_t<parse_action_effects_t<heal_t>>;

namespace pet
{
struct base_wolf_t;
struct primal_elemental_t;
}

struct shaman_td_t : public actor_target_data_t
{
  struct dots
  {
    dot_t* flame_shock;
  } dot;

  struct debuffs
  {
    // Elemental
    buff_t* lightning_rod;

    // Enhancement
    buff_t* lashing_flames;
    buff_t* flametongue_attack;
  } debuff;

  struct heals
  {
    dot_t* riptide;
    dot_t* earthliving;
  } heal;

  shaman_td_t( player_t* target, shaman_t* p );

  shaman_t* actor() const
  {
    return debug_cast<shaman_t*>( source );
  }
};

struct shaman_t : public parse_player_effects_t
{
public:
  // Misc

  bool sk_during_cast;
  bool lava_surge_during_lvb;
  std::unordered_map<std::string, std::tuple<timespan_t, double>> active_wolf_expr_cache;

  /// Shaman ability cooldowns
  std::vector<cooldown_t*> ability_cooldowns;
  player_t* earthen_rage_target =
      nullptr;  // required for Earthen Rage, whichs' ticks damage the most recently attacked target
  event_t* earthen_rage_event;

  /// Lightning Strikes counter
  unsigned ls_counter;

  // Options
  bool raptor_glyph;

  // A vector of action objects that need target cache invalidation whenever the number of
  // Flame Shocks change
  std::vector<action_t*> flame_shock_dependants;
  /// A time-ordered list of active Flame Shocks on enemies
  std::vector<std::pair<timespan_t, dot_t*>> active_flame_shock;
  /// Maximum number of active flame shocks
  unsigned max_active_flame_shock;

  /// Maelstrom Weapon blocklist, allowlist; (spell_id, { override_state, proc tracking object })
  std::vector<mw_proc_state> mw_proc_state_list;

  /// Maelstrom generator/spender tracking
  std::vector<std::pair<simple_sample_data_t, simple_sample_data_t>> mw_source_list;
  std::vector<std::array<simple_sample_data_t, 11>> mw_spend_list;

  /// Deeply Rooted Elements tracking
  extended_sample_data_t dre_samples;
  extended_sample_data_t dre_uptime_samples;

  extended_sample_data_t lvs_samples;

  unsigned dre_attempts;
  unsigned aws_counter;
  double lava_surge_attempts_normalized;

  /// Rolling Thunder last trigger
  timespan_t rt_last_trigger;

  /// Crash Lightnings for cooldown management of Storm Unleashed
  std::vector<action_t*> crash_lightning;

  /// Buff state tracking
  unsigned buff_state_lightning_rod;
  unsigned buff_state_lashing_flames;

  /// Generic proc tracking
  stats::proc_track_db_t tracker;

  // Cached actions
  struct actions_t
  {
    spell_t* lightning_shield;
    attack_t* crash_lightning_aoe;
    attack_t* crash_lightning_unleashed;
    action_t* lightning_bolt_ti;
    action_t* lightning_bolt_ps;
    action_t* tempest_ti;
    action_t* chain_lightning_ti;
    action_t* chain_lightning_ps;
    action_t* chain_lightning_ll_rtl;
    action_t* chain_lightning_ss_rtl;
    action_t* chain_lightning_ws_rtl;
    action_t* ti_trigger;
    action_t* flame_shock_asc;
    action_t* flame_shock_vb;
    action_t* flame_shock;
    action_t* elemental_blast;
    action_t* lava_burst_pf;

    action_t* lightning_rod;

    action_t* stormflurry_ss;
    action_t* stormflurry_ws;

    action_t* stormblast;
    action_t* ascendance_damage;

    // Legendaries
    action_t* dre_ascendance; // Deeply Rooted Elements

    // Cached action pointers
    action_t* feral_spirits; // MW Tracking
    action_t* ascendance; // MW Tracking

    // TWW Hero Talent stuff
    action_t* whirling_air_ss;
    action_t* whirling_air_ws;

    action_t* elemental_blast_foe; // Fusion of Elements

    action_t* thunderstrike_ward;

    action_t* earthen_rage;

    // Arc Discharge doublers
    action_t* lightning_bolt_ad;
    action_t* chain_lightning_ad;
    action_t* chain_lightning_rtl_ad;

    // Imbuement Mastery damage
    action_t* imbuement_mastery;

    // Splitstream
    action_t* splitstream;

    // Doom Winds damage
    action_t* doom_winds;

    // Fire Nova from Voltaic Blaze
    action_t* fire_nova;

    // Doom Winds triggred by Enhancement Ascendance
    action_t* doom_winds_asc;

    action_t* set_ascendance;
    action_t* tww3_primordial_storm;
    action_t* tww3_lava_lash;
    action_t* tww3_fire_nova;
  } action;

  // Set of dummy actions for reporting (stats collection) purposes
  struct dummy_actions_t
  {
    action_t* arc_discharge;
    action_t* stormblast;
    action_t* thorims_invocation;
    action_t* ride_the_lightning;
    action_t* deeply_rooted_elements;
  } dummy;

  // Pets
  struct pets_t
  {
    spawner::pet_spawner_t<pet::primal_elemental_t, shaman_t> fire_elemental;
    spawner::pet_spawner_t<pet::primal_elemental_t, shaman_t> storm_elemental;
    spawner::pet_spawner_t<pet::primal_elemental_t, shaman_t> earth_elemental;

    spawner::pet_spawner_t<pet_t, shaman_t> ancestor;

    spawner::pet_spawner_t<pet::base_wolf_t, shaman_t> fire_wolves;
    spawner::pet_spawner_t<pet::base_wolf_t, shaman_t> lightning_wolves;

    spawner::pet_spawner_t<heal_totem_pet_t, shaman_t> healing_stream_totem;
    spawner::pet_spawner_t<spell_totem_pet_t, shaman_t> capacitor_totem;
    spawner::pet_spawner_t<spell_totem_pet_t, shaman_t> surging_totem;
    spawner::pet_spawner_t<spell_totem_pet_t, shaman_t> searing_totem;

    std::vector<pet::base_wolf_t*> all_wolves;

    pets_t( shaman_t* );
  } pet;

  // Constants
  struct
  {
    /// Lightning Rod damage_multiplier
    double mul_lightning_rod;
  } constant;

  // Buffs
  struct
  {
    // shared between all three specs
    buff_t* ascendance;
    buff_t* ghost_wolf;
    buff_t* flurry;
    buff_t* natures_swiftness;

    // Elemental, Restoration
    buff_t* lava_surge;

    // Elemental, Enhancement
    buff_t* elemental_blast_crit;
    buff_t* elemental_blast_haste;
    buff_t* elemental_blast_mastery;
    buff_t* flametongue_weapon;

    // Elemental
    buff_t* fire_elemental;
    buff_t* storm_elemental;
    buff_t* earth_elemental;
    buff_t* master_of_the_elements;
    buff_t* power_of_the_maelstrom;
    buff_t* stormkeeper;
    buff_t* wind_gust;
    buff_t* call_of_the_ancestors;
    buff_t* ancestral_swiftness;
    buff_t* thunderstrike_ward;
    buff_t* purging_flames;
    buff_t* mid1_ele_2pc;

    buff_t* storms_eye;

    // Enhancement
    buff_t* maelstrom_weapon;
    buff_t* feral_spirit_maelstrom;

    buff_t* crash_lightning;     // Buffs stormstrike and lava lash after using crash lightning
    buff_t* hot_hand;
    buff_t* lightning_shield;
    buff_t* stormsurge;
    buff_t* windfury_weapon;

    buff_t* molten_weapon;
    buff_t* crackling_surge;
    buff_t* converging_storms;
    buff_t* static_accumulation;
    buff_t* doom_winds;

    buff_t* stormblast;

    buff_t* primordial_storm;
    buff_t* lightning_strikes;
    buff_t* surging_elements;
    buff_t* storm_unleashed;
    buff_t* lively_totems;

    buff_t* tww2_enh_2pc; // Winning Streak!
    buff_t* tww2_enh_4pc; // Electrostatic Wager (visible buff)
    buff_t* tww2_enh_4pc_damage; // Electrostatic Wager (hidden damage to CL)
    buff_t* elemental_overflow; // Elemental Overflow

    // Shared talent stuff
    buff_t* tempest;
    buff_t* unlimited_power;
    buff_t* arc_discharge;
    buff_t* storm_swell;
    buff_t* amplification_core;

    buff_t* whirling_air;
    buff_t* whirling_fire;
    buff_t* whirling_earth;

    buff_t* totemic_rebound;
    buff_t* surging_totem;

    // Restoration
    buff_t* spirit_walk;
    buff_t* spiritwalkers_grace;
    buff_t* tidal_waves;

    // PvP
    buff_t* thundercharge;

  } buff;

  // Options
  struct options_t
  {
    rotation_type_e rotation = ROTATION_STANDARD;

    // Ancient Fellowship Deck-of-Cards RNG parametrization
    unsigned ancient_fellowship_positive = 0U;
    unsigned ancient_fellowship_total = 0U;

    // Routine Communication Deck-of-Cards RNG parametrization
    unsigned routine_communication_positive = 0U;
    unsigned routine_communication_total = 0U;

    // Thunderstrike Ward Uniform RNG proc chance
    // TODO: Double check for CL. A ~5h LB test resulted in a ~30% chance.
    double thunderstrike_ward_proc_chance = 0.3;

    double earthquake_spell_power_coefficient = 0.3884;

    double imbuement_mastery_base_chance = 0.07;
    double lively_totems_base_chance = 0.06;

    // Surging totem whiff
    double surging_totem_miss_chance = 0.0;

    // Chain Lightning target randomizer
    double   chain_lightning_target_rng = 0.15; // Chance to shuffle individual targets of chained casts

    int tww3_farseer_set = 0;
    int tww3_stormbringer_set = 0;

    // Chance on Crash Lightning target to sit in the Crash Lightning (Unleashed) puddle
    double crash_lightning_su_hit_chance = 0.85;
  } options;

  // Cooldowns
  struct
  {
    cooldown_t* ascendance;
    cooldown_t* crash_lightning;
    cooldown_t* crash_lightning_su; // CD ignore for Crash Lightning
    cooldown_t* feral_spirits;
    cooldown_t* fire_elemental;
    cooldown_t* flame_shock;
    cooldown_t* frost_shock;
    cooldown_t* lava_burst;
    cooldown_t* lava_lash;
    cooldown_t* liquid_magma_totem;
    cooldown_t* natures_swiftness;
    cooldown_t* shock;  // shared CD of flame shock/frost shock for enhance
    cooldown_t* storm_elemental;
    cooldown_t* stormkeeper;
    cooldown_t* strike;  // shared CD of Storm Strike and Windstrike
    cooldown_t* sundering;
    cooldown_t* totemic_recall;
    cooldown_t* ancestral_swiftness;
  } cooldown;

  // Expansion-specific Legendaries
  struct legendary_t
  {
  } legendary;

  // Gains
  struct
  {
    gain_t* aftershock;
    gain_t* resurgence;
    gain_t* feral_spirit;
    gain_t* spirit_of_the_maelstrom;
    gain_t* searing_flames;
    gain_t* inundate;
  } gain;

  // Tracked Procs
  struct
  {
    // Elemental, Restoration
    proc_t* lava_surge;
    proc_t* wasted_lava_surge;
    proc_t* surge_during_lvb;

    // Elemental
    proc_t* aftershock;
    proc_t* lightning_rod;
    proc_t* searing_flames;

    proc_t* ascendance_tempest_overload;
    proc_t* ascendance_lightning_bolt_overload;
    proc_t* ascendance_chain_ligtning_overload;
    proc_t* ascendance_lava_burst_overload;
    proc_t* ascendance_earth_shock_overload;
    proc_t* ascendance_elemental_blast_overload;
    proc_t* ascendance_earthquake_overload;
    proc_t* potm_tempest_overload;

    proc_t* elemental_blast_haste;
    proc_t* elemental_blast_crit;
    proc_t* elemental_blast_mastery;

    // Enhancement
    proc_t* stormflurry_failed;
    proc_t* windfury_uw;
    proc_t* reset_swing_mw;

    // TWW Trackers
    proc_t* tempest_awakening_storms;
  } proc;

  // Class Specializations
  struct
  {
    // Generic
    const spell_data_t* mail_specialization;
    const spell_data_t* shaman;

    // Elemental
    const spell_data_t* elemental_shaman;   // general spec multiplier
    const spell_data_t* elemental_shaman2;  // .. and another
    const spell_data_t* elemental_shaman3;  // ...... and another
    const spell_data_t* lava_burst_2;       // 7.1 Lava Burst autocrit with FS passive
    const spell_data_t* maelstrom;
    const spell_data_t* lava_surge;
    const spell_data_t* inundate;
    const spell_data_t* stormkeeper_2;      // charges

    // Enhancement
    const spell_data_t* critical_strikes;
    const spell_data_t* dual_wield;
    const spell_data_t* enhancement_shaman;
    const spell_data_t* enhancement_shaman2;
    const spell_data_t* maelstrom_weapon;
    const spell_data_t* stormstrike;

    const spell_data_t* windfury;
    const spell_data_t* lava_lash_2;
    const spell_data_t* stormbringer;
    const spell_data_t* feral_lunge;

    // Restoration
    const spell_data_t* resurgence;
    const spell_data_t* riptide;
    const spell_data_t* tidal_waves;
    const spell_data_t* spiritwalkers_grace;
    const spell_data_t* restoration_shaman;  // general spec multiplier
  } spec;

  // Masteries
  struct
  {
    const spell_data_t* elemental_overload;
    const spell_data_t* enhanced_elements;
    const spell_data_t* deep_healing;
  } mastery;

  // Uptimes
  struct
  {
    uptime_t* hot_hand;
  } uptime;

  // Talents
  struct
  {
    // Class tree
    // Row 1
    player_talent_t lava_burst;
    player_talent_t lava_lash;
    player_talent_t chain_lightning;
    // Row 2
    player_talent_t earth_elemental;
    player_talent_t wind_shear;
    player_talent_t spirit_wolf; // TODO: NYU
    player_talent_t thunderous_paws; // TODO: NYI
    player_talent_t frost_shock;
    // Row 3
    player_talent_t earth_shield;
    player_talent_t fire_and_ice;
    player_talent_t capacitor_totem;
    // Row 4
    player_talent_t spiritwalkers_grace;
    player_talent_t static_charge;
    // Row 5
    player_talent_t graceful_spirit; // TODO: Movement Speed
    player_talent_t natures_fury;
    player_talent_t spiritual_awakening;
    // Row 6
    player_talent_t totemic_surge;
    player_talent_t winds_of_alakir; // TODO: NYI
    // Row 7
    player_talent_t healing_stream_totem;
    player_talent_t improved_lightning_bolt;
    player_talent_t spirit_walk;
    player_talent_t gust_of_wind; // TODO: NYI
    player_talent_t enhanced_imbues;
    // Row 8
    player_talent_t natures_swiftness;
    player_talent_t thunderstorm;
    player_talent_t totemic_focus; // TODO: NYI
    player_talent_t surging_shields; // TODO: NYI
    // Row 9
    player_talent_t lightning_lasso;
    player_talent_t thundershock;
    player_talent_t totemic_recall;
    // Row 10
    player_talent_t ancestral_guidance;
    player_talent_t creation_core; // TODO: NYI
    player_talent_t call_of_the_elements;
    player_talent_t instinctive_imbuements;

    // Spec - Shared
    player_talent_t ancestral_wolf_affinity; // TODO: NYI
    player_talent_t elemental_blast;
    player_talent_t ascendance;

    // Enhancement
    // Row 1
    player_talent_t maelstrom_weapon;
    // Row 2
    player_talent_t windfury_weapon;
    player_talent_t flametongue_weapon;
    // Row 3
    player_talent_t forceful_winds;
    player_talent_t crash_lightning;
    player_talent_t molten_assault;
    // Row 4
    player_talent_t unruly_winds;
    player_talent_t raging_maelstrom;
    player_talent_t ashen_catalyst;
    // Row 5
    player_talent_t stormblast;
    player_talent_t overcharge;
    player_talent_t overflowing_maelstrom;
    player_talent_t flurry;
    player_talent_t hot_hand;
    // Row 6
    player_talent_t storms_wrath;
    player_talent_t elemental_tempo;
    player_talent_t voltaic_blaze;
    // Row 7
    player_talent_t chaining_storms;
    player_talent_t converging_storms;
    player_talent_t stormflurry;
    player_talent_t stormbind;
    player_talent_t elemental_weapons;
    player_talent_t fire_nova;
    player_talent_t lashing_flames;
    // Row 8
    player_talent_t ride_the_lightning;
    player_talent_t doom_winds;
    player_talent_t sundering;
    // Row 9
    player_talent_t lightning_strikes;
    player_talent_t elemental_assault;
    player_talent_t static_accumulation;
    player_talent_t feral_spirit;
    player_talent_t surging_elements;
    // Row 10
    player_talent_t thunder_capacitor;
    player_talent_t deeply_rooted_elements;
    player_talent_t thorims_invocation;
    player_talent_t primordial_storm;
    // Row 11 (Keystone - Storm Unleashed)
    player_talent_t storm_unleashed_1;
    player_talent_t storm_unleashed_2;
    player_talent_t storm_unleashed_3;

    // Elemental

    // Row 1
    player_talent_t earth_shock;
    // Row 2
    player_talent_t earthquake_reticle;
    player_talent_t earthquake_target;
    player_talent_t elemental_fury;
    player_talent_t echo_of_the_elements;
    // Row 3
    player_talent_t flash_of_lightning;
    player_talent_t tectonic_collapse;
    player_talent_t aftershock;
    player_talent_t molten_wrath;
    player_talent_t master_of_the_elements;
    // Row 4
    player_talent_t lightning_capacitor;
    player_talent_t stormkeeper;
    // player_talent_t flametongue_weapon;
    // Row 5
    player_talent_t storm_frenzy;
    player_talent_t swelling_maelstrom;
    player_talent_t primordial_fury;
    player_talent_t fury_of_the_storms; // TODO Hawk: NYI
    player_talent_t herald_of_the_storms;
    player_talent_t flames_of_the_cauldron;
    // Row 6
    player_talent_t amped_up;
    player_talent_t elemental_resonance;
    player_talent_t thunderstrike_ward;
    player_talent_t path_of_the_seer;
    player_talent_t elemental_unity;
    player_talent_t searing_flames;
    // Row 7
    player_talent_t power_of_the_maelstrom;
    player_talent_t earthshatter;
    player_talent_t storm_infusion;
    player_talent_t echo_chamber;
    player_talent_t everlasting_elements;
    player_talent_t earthen_rage; // NEW Partial implementation
    player_talent_t lava_flows;
    // Row 8
    player_talent_t fusion_of_elements;
    player_talent_t eye_of_the_storm;
    player_talent_t inferno_arc;
    player_talent_t flames_of_the_firelord;
    // Row 9
    player_talent_t lightning_rod;
    player_talent_t mountains_will_fall;
    player_talent_t call_of_fire;
    //player_talent_t voltaic_blaze;
    // Row 10
    player_talent_t charged_conduit;
    player_talent_t echo_of_the_elementals;
    player_talent_t first_ascendant;
    player_talent_t preeminence;
    player_talent_t primal_elementalist;
    player_talent_t crackling_fury;
    player_talent_t purging_flames;
    // Row 11
    player_talent_t feedback_loop_1;
    player_talent_t feedback_loop_2;
    player_talent_t feedback_loop_3;

    // Stormbringer

    // Row 1
    player_talent_t tempest;

    // Row 2
    player_talent_t unlimited_power;
    player_talent_t stormcaller;
    player_talent_t electroshock;
    player_talent_t stormwell;

    // Row 3
    player_talent_t supercharge;
    player_talent_t storm_swell;
    player_talent_t arc_discharge;
    player_talent_t rolling_thunder;
    player_talent_t natural_gift;

    // Row 4
    player_talent_t voltaic_surge;
    player_talent_t conductive_energy;
    player_talent_t descending_skies;

    // Row 5
    player_talent_t awakening_storms;

    // Totemic

    // Row 1
    player_talent_t surging_totem;

    // Row 2
    player_talent_t totemic_rebound;
    player_talent_t amplification_core;
    player_talent_t oversurge;
    player_talent_t lively_totems;
    player_talent_t totemic_momentum;

    // Row 3
    player_talent_t splitstream;
    player_talent_t elemental_attunement;

    // Row 4
    player_talent_t imbuement_mastery;
    player_talent_t pulse_capacitor;
    player_talent_t supportive_imbuements;
    player_talent_t totemic_coordination;
    player_talent_t earthsurge;
    player_talent_t primal_catalyst;

    // Row 5
    player_talent_t whirling_elements;

    // Farseer

    // Row 1
    player_talent_t call_of_the_ancestors;

    // Row 2
    player_talent_t latent_wisdom;
    player_talent_t ancient_fellowship;
    player_talent_t heed_my_call;
    player_talent_t routine_communication;
    player_talent_t elemental_reverb;
    player_talent_t ancestral_influence;

    // Row 3
    player_talent_t offering_from_beyond;
    player_talent_t primordial_capacity;
    player_talent_t spiritwalkers_momentum;
    player_talent_t windspeaker;

    // Row 4
    player_talent_t maelstrom_supremacy;
    player_talent_t final_calling; // NEW Partial implementation (rest are bugs?)
    player_talent_t mystic_knowledge;

    // Row 5
    player_talent_t ancestral_swiftness;

  } talent;

  // Misc Spells
  struct
  {
    const spell_data_t* ascendance;  // proxy spell data for normal & dre ascendance
    const spell_data_t* ascendance_mw_passive;
    const spell_data_t* resurgence;
    const spell_data_t* maelstrom_weapon;
    const spell_data_t* maelstrom_weapon_driver;
    const spell_data_t* feral_spirit;
    const spell_data_t* earth_elemental;
    const spell_data_t* fire_elemental;
    const spell_data_t* storm_elemental;
    const spell_data_t* flametongue_weapon;
    const spell_data_t* windfury_weapon;
    const spell_data_t* inundate;
    const spell_data_t* storm_swell;
    const spell_data_t* lightning_rod;
    const spell_data_t* improved_flametongue_weapon;
    const spell_data_t* earthen_rage;
    const spell_data_t* flowing_spirits_feral_spirit;
    const spell_data_t* hot_hand;
    const spell_data_t* elemental_weapons;
    const spell_data_t* tww3_farseer_2pc;
    const spell_data_t* tww3_farseer_4pc;
    const spell_data_t* tww3_stormbringer_2pc;
    const spell_data_t* tww3_stormbringer_4pc;
  } spell;

  struct rng_obj_t
  {
    real_ppm_t* awakening_storms;
    real_ppm_t* lively_totems;
    real_ppm_t* totemic_rebound;

    shuffled_rng_t* ancient_fellowship;
    shuffled_rng_t* routine_communication;

    accumulated_rng_t* imbuement_mastery;
    accumulated_rng_t* lively_totems_ptr;

    // New deeply rooted elements RNG
    rng::deck_rng_wrapper_t<rng::dre_deck_rng_t> deeply_rooted_elements;
    rng::deck_rng_wrapper_t<rng::dre_deck_rng_t> tempest_enh;
    rng::deck_rng_wrapper_t<rng::dre_deck_rng_t> tempest_ele;
    rng::deck_rng_wrapper_t<rng::dre_deck_rng_t> storm_unleashed;
    rng::deck_rng_wrapper_t<rng::dre_deck_rng_t> asc_dw;

    rng_obj_t( shaman_t* s ) :
      awakening_storms( nullptr ), lively_totems( nullptr ), totemic_rebound( nullptr ),
      ancient_fellowship( nullptr ), routine_communication( nullptr ), imbuement_mastery( nullptr ),
      lively_totems_ptr( nullptr ),
        deeply_rooted_elements( "dre", s ),
        tempest_enh( "tempest", s ),
        tempest_ele( "tempest_ele", s ),
        storm_unleashed( "storm_unleashed", s ),
        asc_dw( "asc_dw", s )
    { }
  } rng_obj;

  // Cached pointer for ascendance / normal white melee
  shaman_attack_t* melee_mh;
  shaman_attack_t* melee_oh;
  shaman_attack_t* ascendance_mh;
  shaman_attack_t* ascendance_oh;

  // Weapon Enchants
  shaman_attack_t* windfury_mh;
  shaman_spell_t* flametongue;

  shaman_t( sim_t* sim, util::string_view name, race_e r = RACE_TAUREN )
    : parse_player_effects_t( sim, SHAMAN, name, r ),
      sk_during_cast( false ),
      lava_surge_during_lvb( false ),
      ls_counter( 0U ),
      raptor_glyph( false ),
      dre_samples( "dre_tracker", false ),
      dre_uptime_samples( "dre_uptime_tracker", false ),
      lvs_samples( "lvs_tracker", false ),
      dre_attempts( 0U ),
      aws_counter(0U),
      lava_surge_attempts_normalized( 0.0 ),
      tracker( this ),
      action(),
      pet( this ),
      constant(),
      buff(),
      cooldown(),
      legendary( legendary_t() ),
      gain(),
      proc(),
      spec(),
      mastery(),
      uptime(),
      talent(),
      spell(),
      rng_obj( this )
  {
    // Cooldowns
    cooldown.ascendance         = get_cooldown( "ascendance" );
    cooldown.crash_lightning    = get_cooldown( "crash_lightning" );
    cooldown.crash_lightning_su = get_cooldown( "crash_lighting_su" );
    cooldown.fire_elemental     = get_cooldown( "fire_elemental" );
    cooldown.flame_shock        = get_cooldown( "flame_shock" );
    cooldown.frost_shock        = get_cooldown( "frost_shock" );
    cooldown.lava_burst         = get_cooldown( "lava_burst" );
    cooldown.lava_lash          = get_cooldown( "lava_lash" );
    cooldown.liquid_magma_totem = get_cooldown( "liquid_magma_totem" );
    cooldown.natures_swiftness  = get_cooldown( "natures_swiftness" );
    cooldown.shock              = get_cooldown( "shock" );
    cooldown.storm_elemental    = get_cooldown( "storm_elemental" );
    cooldown.stormkeeper        = get_cooldown( "stormkeeper" );
    cooldown.strike             = get_cooldown( "strike" );
    cooldown.sundering          = get_cooldown( "sundering" );
    cooldown.totemic_recall     = get_cooldown( "totemic_recall" );
    cooldown.ancestral_swiftness= get_cooldown( "ancestral_swiftness" );

    melee_mh      = nullptr;
    melee_oh      = nullptr;
    ascendance_mh = nullptr;
    ascendance_oh = nullptr;

    // Weapon Enchants
    windfury_mh = nullptr;
    flametongue = nullptr;

    if ( specialization() == SHAMAN_ELEMENTAL || specialization() == SHAMAN_ENHANCEMENT )
      resource_regeneration = regen_type::DISABLED;
    else
      resource_regeneration = regen_type::DYNAMIC;

    dre_samples.reserve( 8192 );
    dre_uptime_samples.reserve( 8192 );

    lvs_samples.reserve( 8192 );

    // Buff States
    buff_state_lightning_rod = 0U;
    buff_state_lashing_flames = 0U;

    // Reserve enough space so that references won't invalidate due to rehashing
    active_wolf_expr_cache.reserve( 32 );
  }

  ~shaman_t() override = default;

  // Misc
  // bool is_elemental_pet_active() const;
  // pet_t* get_active_elemental_pet() const;
  void summon_elemental( elemental type, timespan_t override_duration = 0_ms );
  void summon_ancestor( double proc_chance = 1.0 );
  void trigger_elemental_blast_proc();
  void summon_feral_spirit( wolf_type_e, unsigned n, timespan_t duration );

  mw_proc_state set_mw_proc_state( action_t* action, mw_proc_state state )
  {
    if ( as<unsigned>( action->internal_id ) >= mw_proc_state_list.size() )
    {
      mw_proc_state_list.resize( action->internal_id + 1U, mw_proc_state::DEFAULT );
    }

    // Use explicit mw_proc_state::DEFAULT set as initialization in shaman_t::action_init_finished
    if ( state == mw_proc_state::DEFAULT )
    {
      return mw_proc_state_list[ action->internal_id ];
    }

    mw_proc_state_list[ action->internal_id ] = state;

    return mw_proc_state_list[ action->internal_id ];
  }

  mw_proc_state set_mw_proc_state( action_t& action, mw_proc_state state )
  { return set_mw_proc_state( &( action ), state ); }

  mw_proc_state get_mw_proc_state( action_t* action ) const
  {
    assert( as<unsigned>( action->internal_id ) < mw_proc_state_list.size() &&
            "No Maelstrom Weapon proc-state found" );

    return mw_proc_state_list[ action->internal_id ];
  }

  mw_proc_state get_mw_proc_state( action_t& action ) const
  { return get_mw_proc_state( &( action ) ); }

  double windfury_proc_chance();

  // triggers
  void trigger_maelstrom_gain( double maelstrom_gain, gain_t* gain = nullptr );
  void trigger_windfury_weapon( const action_state_t*, double override_chance = -1.0 );
  void trigger_flametongue_weapon( const action_state_t* );
  void trigger_hot_hand( const action_state_t* state );
  void trigger_lava_surge();
  void trigger_elemental_assault( const action_state_t* state );
  void trigger_stormflurry( const action_state_t* state );
  void trigger_imbuement_mastery( const action_state_t* state );
  void trigger_whirling_fire( const action_state_t* state );
  void trigger_stormblast( const action_state_t* state );

  // TWW Triggers
  template <typename T>
  void trigger_tempest( T resource_count );
  void trigger_awakening_storms( player_t* target );
  void trigger_awakening_storms( double maelstrom_consumed, player_t* target );
  void trigger_awakening_storms( const action_state_t* state );
  void trigger_earthsurge( const action_state_t* state, double mul = 1.0 );
  void trigger_whirling_air( const action_state_t* state );
  void trigger_splitstream( const action_state_t* state );
  void trigger_thunderstrike_ward( const action_state_t* state );
  void trigger_earthen_rage( const action_state_t* state );
  void trigger_totemic_rebound( const action_state_t* state, bool whirl = false, timespan_t delay = 300_ms );
  void trigger_ancestor( ancestor_cast cast, const action_state_t* state );
  void trigger_arc_discharge( const action_state_t* state );
  void trigger_lively_totems( const action_state_t* state );
  void trigger_tww3_totemic_enh_2pc( const action_state_t* state );
  void trigger_elemental_overflow( const action_state_t* state, action_t* trigger );

  // Midnight Triggers
  void trigger_ride_the_lightning( const action_state_t* state, action_t* trigger );
  void trigger_thorims_invocation( const action_state_t* state );
  void trigger_crash_lightning_proc( const action_state_t* state, strike_variant t );

  // Legendary
  void trigger_deeply_rooted_elements( const action_state_t* state ); // 11.1 version

  void trigger_secondary_flame_shock( player_t* target, spell_variant variant = spell_variant::NORMAL ) const;
  void trigger_secondary_flame_shock( const action_state_t* state, spell_variant variant = spell_variant::NORMAL ) const;
  void regenerate_flame_shock_dependent_target_list( const action_t* action ) const;

  void generate_maelstrom_weapon( const action_t* action, int stacks = 1 );
  void generate_maelstrom_weapon( const action_state_t* state, int stacks = 1 );
  void consume_maelstrom_weapon( const action_state_t* state, int stacks );


  // Character Definition
  void init_spells() override;
  void init_base_stats() override;
  void init_scaling() override;
  void create_buffs() override;
  void create_actions() override;
  void create_options() override;
  void init_gains() override;
  void init_procs() override;
  void init_uptimes() override;
  void init_assessors() override;
  void init_rng() override;
  bool validate_fight_style( fight_style_e style ) const override;
  void init_special_effects() override;
  void init_finished() override;
  bool validate_actor() override;
  std::string create_profile( save_e ) override;
  void create_special_effects() override;
  action_t* create_proc_action( util::string_view /* name */, const special_effect_t& /* effect */ ) override;
  void action_init_finished( action_t& action ) override;
  void analyze( sim_t& sim ) override;
  void datacollection_end() override;
  const spell_data_t* conditional_spell_lookup( bool fn, int id );

  // APL releated methods
  void init_action_list() override;
  void init_action_list_enhancement();
  void init_action_list_restoration_dps();
  void init_blizzard_action_list() override;
  std::vector<std::string> action_names_from_spell_id( unsigned int spell_id ) const override;
  parsed_assisted_combat_rule_t parse_assisted_combat_rule( const assisted_combat_rule_data_t& rule,
                                                            const assisted_combat_step_data_t& step ) const override;
  void parse_assisted_combat_step( const assisted_combat_step_data_t& step,
                                   action_priority_list_t* assisted_combat ) override;
  std::string generate_bloodlust_options();
  std::string default_potion() const override;
  std::string default_flask() const override;
  std::string default_food() const override;
  std::string default_rune() const override;
  std::string default_temporary_enchant() const override;

  double resource_loss( resource_e resource_type, double amount, gain_t* source, action_t* ) override;

  void apply_action_effects( parse_effects_t* );
  void apply_player_effects();

  void moving() override;
  void invalidate_cache( cache_e c ) override;
  double composite_attribute( attribute_e ) const override;
  double composite_player_critical_damage_multiplier( const action_state_t* s, school_e school ) const override;
  double composite_player_target_multiplier( player_t* target, school_e school ) const override;
  double composite_maelstrom_gain_coefficient( const action_state_t* /* state */ = nullptr ) const
  { return 1.0; }
  action_t* create_action( util::string_view name, util::string_view options ) override;
  pet_t* create_pet( util::string_view name, util::string_view type = {} ) override;
  void create_pets() override;
  std::unique_ptr<expr_t> create_expression( util::string_view name ) override;
  resource_e primary_resource() const override
  {
    return RESOURCE_MANA;
  }
  role_e primary_role() const override;
  stat_e convert_hybrid_stat( stat_e s ) const override;
  void reset() override;
  void merge( player_t& other ) override;
  void copy_from( player_t* ) override;

  target_specific_t<shaman_td_t> target_data;

  const shaman_td_t* find_target_data( const player_t* target ) const override
  {
    return target_data[ target ];
  }

  shaman_td_t* get_target_data( player_t* target ) const override
  {
    shaman_td_t*& td = target_data[ target ];
    if ( !td )
    {
      td = new shaman_td_t( target, const_cast<shaman_t*>( this ) );
    }
    return td;
  }

  // Helper to trigger a secondary ability through action scheduling (i.e., schedule_execute()),
  // without breaking targeting information. Note, causes overhead as an extra action_state_t object
  // is needed to carry the information.
  void trigger_secondary_ability( const action_state_t* source_state, action_t* secondary_action,
                                  bool inherit_state = false );

  template <typename T_CONTAINER, typename T_DATA>
  T_CONTAINER* get_data_entry( util::string_view name, std::vector<T_DATA*>& entries )
  {
    for ( size_t i = 0; i < entries.size(); i++ )
    {
      if ( entries[ i ]->first == name )
      {
        return &( entries[ i ]->second );
      }
    }

    entries.push_back( new T_DATA( name, T_CONTAINER() ) );
    return &( entries.back()->second );
  }
};

// ==========================================================================
// Shaman Custom Buff Declaration
// ==========================================================================
//

struct ascendance_buff_t : public buff_t
{
  action_t* lava_burst;

  ascendance_buff_t( shaman_t* p )
    : buff_t( p, "ascendance", p->spell.ascendance ),
      lava_burst( nullptr )
  {
    set_cooldown( timespan_t::zero() );  // Cooldown is handled by the action
  }

  void ascendance( attack_t* mh, attack_t* oh );
  bool trigger( int stacks, double value, double chance, timespan_t duration ) override;
  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override;
};

shaman_td_t::shaman_td_t( player_t* target, shaman_t* p ) : actor_target_data_t( target, p )
{
  heal.riptide = nullptr;
  heal.earthliving = nullptr;
  // Shared
  dot.flame_shock = target->get_dot( "flame_shock", p );

  // Elemental
  debuff.lightning_rod = make_buff( *this, "lightning_rod", p->find_spell( 197209 ) )
    ->set_default_value( p->constant.mul_lightning_rod )
    ->set_stack_change_callback(
      [ p ]( buff_t*, int old, int new_ ) {
        if ( new_ - old > 0 )
        {
          p->buff_state_lightning_rod++;
        }
        else
        {
          p->buff_state_lightning_rod--;
        }
      }
    );

  // Enhancement
  debuff.lashing_flames = make_buff( *this, "lashing_flames", p->find_spell( 334168 ) )
    ->set_trigger_spell( p->talent.lashing_flames )
    ->set_stack_change_callback(
      [ p ]( buff_t*, int old, int new_ ) {
        if ( new_ - old > 0 )
        {
          p->buff_state_lashing_flames++;
        }
        else
        {
          p->buff_state_lashing_flames--;
        }
      }
    )
    ->set_default_value_from_effect( 1 );

  debuff.flametongue_attack = make_buff( *this, "flametongue_attack", p->find_spell( 467390 ) )
    ->set_trigger_spell( p->talent.imbuement_mastery );
}

namespace expr
{
template <typename T>
struct wolves_active_for_t : public expr_t
{
  action_t* action_;
  std::unique_ptr<expr_t> expr_;
  shaman_t* p_;
  T cmp_;
  std::tuple<timespan_t, double>& val_cache;

  wolves_active_for_t( action_t* a, util::string_view subexpr ) : expr_t( "wolves_active_for" ),
    action_( a ), expr_( a->create_expression( subexpr ) ),
    p_( debug_cast<shaman_t*>( a->player ) ), cmp_(),
    val_cache( p_->active_wolf_expr_cache[ std::string( subexpr ) ] )
  {
    if ( subexpr.size() == 0 || ( subexpr.size() > 0 && !expr_ ) )
    {
      throw sc_invalid_apl_argument( fmt::format( "Unable to generate expression from '{}'.", subexpr ) );
    }
  }

  double evaluate() override
  {
    auto& time_ = std::get<0>( val_cache );
    auto& n_wolves_ = std::get<1>( val_cache );
    if ( p_->sim->current_time() > time_ )
    {
      auto val = expr_->evaluate();
      n_wolves_ = 0U;
      for ( const auto it : p_->pet.all_wolves )
      {
        auto wolf = debug_cast<const pet_t*>( it );
        n_wolves_ += cmp_( wolf->expiration->remains().total_seconds(), val );
      }

      time_ = p_->sim->current_time();
    }

    return as<double>( n_wolves_ );
  }
};

struct hprio_cd_min_remains_expr_t : public expr_t
{
  action_t* action_;
  std::vector<cooldown_t*> cd_;

  // TODO: Line_cd support
  hprio_cd_min_remains_expr_t( action_t* a ) : expr_t( "min_remains" ), action_( a )
  {
    action_priority_list_t* list = a->player->get_action_priority_list( a->action_list->name_str );
    for ( auto list_action : list->foreground_action_list )
    {
      // Jump out when we reach this action
      if ( list_action == action_ )
        break;

      // Skip if this action's cooldown is the same as the list action's cooldown
      if ( list_action->cooldown == action_->cooldown )
        continue;

      // Skip actions with no cooldown
      if ( list_action->cooldown && list_action->cooldown->duration == timespan_t::zero() )
        continue;

      // Skip cooldowns that are already accounted for
      if ( std::find( cd_.begin(), cd_.end(), list_action->cooldown ) != cd_.end() )
        continue;

      // std::cout << "Appending " << list_action -> name() << " to check list" << std::endl;
      cd_.push_back( list_action->cooldown );
    }
  }

  double evaluate() override
  {
    if ( cd_.empty() )
      return 0;

    timespan_t min_cd = cd_[ 0 ]->remains();
    for ( size_t i = 1, end = cd_.size(); i < end; i++ )
    {
      timespan_t remains = cd_[ i ]->remains();
      // std::cout << "cooldown.higher_priority.min_remains " << cd_[ i ] -> name_str << " remains=" <<
      // remains.total_seconds() << std::endl;
      if ( remains < min_cd )
        min_cd = remains;
    }

    // std::cout << "cooldown.higher_priority.min_remains=" << min_cd.total_seconds() << std::endl;
    return min_cd.total_seconds();
  }
};
} // namespace expr ends

// ==========================================================================
// Shaman Action Base Template
// ==========================================================================

struct dummy_action_t : public action_t
{
  dummy_action_t( player_t* actor, const spell_data_t* spell, util::string_view name_str = {} ) :
    action_t( ACTION_OTHER, name_str.empty() ? spell->name_cstr() : name_str, actor, spell )
  {
    background = true;
    callbacks = may_crit = false;
  }

  dummy_action_t( player_t* actor, const player_talent_t& talent, util::string_view name_str = {} ) :
    dummy_action_t( actor, talent.spell(), name_str )
  { }

  bool ready() override
  { return false; }
};

// ==========================================================================
// Shaman Action Base Template
// ==========================================================================

struct shaman_action_state_t : public action_state_t
{
protected:
  unsigned exec_type = static_cast<unsigned>( spell_variant::NORMAL );

public:
  double mw_mul = 0.0;

  shaman_action_state_t( action_t* action_, player_t* target_ ) :
    action_state_t( action_, target_ )
  { }

  bool is_variant( spell_variant variant_ ) const
  { return exec_type & ( 1 << static_cast<unsigned>( variant_ ) ); }

  bool variant_flags() const
  { return exec_type; }

  void variant_flags( unsigned variant_data_ )
  { exec_type = variant_data_; }

  void initialize() override
  {
    action_state_t::initialize();
    exec_type = static_cast<unsigned>( spell_variant::NORMAL );
    mw_mul = 0.0;
  }

  void copy_state( const action_state_t* s ) override
  {
    action_state_t::copy_state( s );

    auto lbs = debug_cast<const shaman_action_state_t*>( s );
    variant_flags( lbs->variant_flags() );
    mw_mul = lbs->mw_mul;
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    action_state_t::debug_str( s );

    s << " exec_type=" << util::string_join( exec_type_str( exec_type ), "," );
    s << " mw_mul=" << mw_mul;

    return s;
  }
};

template <class Base>
struct shaman_action_t : public parse_action_effects_t<Base>
{
private:
  using ab = parse_action_effects_t<Base>;  // action base, eg. spell_t

protected:
  unsigned exec_type;
public:
  using base_t = shaman_action_t<Base>;

  // General things
  //
  // Ghost wolf unshift
  bool unshift_ghost_wolf;

  // Maelstrom stuff
  gain_t* gain;
  double maelstrom_gain;
  double maelstrom_gain_coefficient;
  bool maelstrom_gain_per_target;

  bool affected_by_ns_cost;
  bool affected_by_ns_cast_time;
  bool affected_by_ans_cost;
  bool affected_by_ans_cast_time;

  bool affected_by_stormkeeper_cast_time;
  bool affected_by_stormkeeper_damage;
  bool affected_by_stormkeeper_damage_tier;

  bool affected_by_elemental_unity_fe_da;
  bool affected_by_elemental_unity_fe_ta;
  bool affected_by_elemental_unity_se_da;
  bool affected_by_elemental_unity_se_ta;
  bool affected_by_flametongue_da;
  bool affected_by_flametongue_ta;
  bool affected_by_lightning_cap_da;
  bool affected_by_lightning_cap_ta;

  bool affected_by_maelstrom_weapon = false;
  int mw_consumed_stacks, mw_affected_stacks;
  // Maelstrom-consuming parent spell to inherit stacks from its cast
  action_t* mw_parent;

  shaman_action_t( util::string_view n, shaman_t* player, const spell_data_t* s = spell_data_t::nil(),
                  unsigned variant_ = static_cast<unsigned>( spell_variant::NORMAL ) )
    : ab( n, player, s ),
      exec_type( variant_ ),
      unshift_ghost_wolf( true ),
      gain( player->get_gain( s->id() > 0 ? s->name_cstr() : n ) ),
      maelstrom_gain( 0 ),
      maelstrom_gain_coefficient( 1.0 ),
      maelstrom_gain_per_target( true ),
      affected_by_ns_cost( false ),
      affected_by_ns_cast_time( false ),
      affected_by_ans_cost( false ),
      affected_by_ans_cast_time( false ),
      affected_by_stormkeeper_cast_time( false ),
      affected_by_stormkeeper_damage( false ),
      affected_by_stormkeeper_damage_tier( false ),
      affected_by_elemental_unity_fe_da( false ),
      affected_by_elemental_unity_fe_ta( false ),
      affected_by_elemental_unity_se_da( false ),
      affected_by_elemental_unity_se_ta( false ),
      affected_by_flametongue_da( false ),
      affected_by_flametongue_ta( false ),
      affected_by_lightning_cap_da( false ),
      affected_by_lightning_cap_ta( false ),
      affected_by_maelstrom_weapon( false ),
      mw_consumed_stacks( 0 ), mw_affected_stacks( 0 ),
      mw_parent( nullptr )
  {
    ab::may_crit = true;
    ab::track_cd_waste = s->cooldown() > timespan_t::zero() || s->charge_cooldown() > timespan_t::zero();

    // Auto-parse maelstrom gain from energize
    for ( size_t i = 1; i <= ab::data().effect_count(); i++ )
    {
      const spelleffect_data_t& effect = ab::data().effectN( i );
      if ( effect.type() != E_ENERGIZE || static_cast<power_e>( effect.misc_value1() ) != POWER_MAELSTROM )
      {
        continue;
      }

      maelstrom_gain    = effect.resource( RESOURCE_MAELSTROM );
      ab::energize_type = action_energize::NONE;  // disable resource generation from spell data.
    }

    if ( this->data().affected_by( player->spell.maelstrom_weapon->effectN( 1 ) ) )
    {
      affected_by_maelstrom_weapon = true;
    }

    affected_by_stormkeeper_cast_time =
        ab::data().affected_by( player->find_spell( 191634 )->effectN( 1 ) );
    affected_by_stormkeeper_damage    =
        ab::data().affected_by( player->find_spell( 191634 )->effectN( 2 ) );
    affected_by_stormkeeper_damage_tier = ab::data().affected_by( player->find_spell( 191634 )->effectN( 4 ) );

    affected_by_ns_cost = ab::data().affected_by( player->talent.natures_swiftness->effectN( 1 ) ) ||
                          ab::data().affected_by( player->talent.natures_swiftness->effectN( 3 ) );
    affected_by_ans_cost = ab::data().affected_by( player->buff.ancestral_swiftness->data().effectN( 1 ) ) ||
                           ab::data().affected_by( player->buff.ancestral_swiftness->data().effectN( 3 ) );
    affected_by_ns_cast_time = ab::data().affected_by( player->talent.natures_swiftness->effectN( 2 ) );
    affected_by_ans_cast_time = ab::data().affected_by( player->buff.ancestral_swiftness->data().effectN( 2 ) );

    affected_by_elemental_unity_fe_da = ab::data().affected_by( player->buff.fire_elemental->data().effectN( 4 ) );
    affected_by_elemental_unity_fe_ta = ab::data().affected_by( player->buff.fire_elemental->data().effectN( 5 ) );
    affected_by_elemental_unity_se_da = ab::data().affected_by( player->buff.storm_elemental->data().effectN( 3 ) );
    affected_by_elemental_unity_se_ta = ab::data().affected_by( player->buff.storm_elemental->data().effectN( 4 ) );
    affected_by_flametongue_da = ab::data().affected_by( player->spell.improved_flametongue_weapon->effectN( 1 ) );
    affected_by_flametongue_ta = ab::data().affected_by( player->spell.improved_flametongue_weapon->effectN( 2 ) );
    affected_by_lightning_cap_da = ab::data().affected_by( player->talent.lightning_capacitor->effectN( 1 ) );
    affected_by_lightning_cap_ta = ab::data().affected_by( player->talent.lightning_capacitor->effectN( 2 ) );

    if ( this->data().ok() )
    {
      p()->apply_action_effects( this );
    }
  }

  bool is_variant( spell_variant variant_ ) const
  { return exec_type & ( 1 << static_cast<unsigned>( variant_ ) ); }

  unsigned variant_flags() const
  { return exec_type; }

  shaman_t* p()
  { return debug_cast<shaman_t*>( ab::player ); }

  const shaman_t* p() const
  { return debug_cast<shaman_t*>( ab::player ); }

  shaman_td_t* td( player_t* t ) const
  { return p()->get_target_data( t ); }

  static shaman_action_state_t* cast_state( action_state_t* s )
  { return debug_cast<shaman_action_state_t*>( s ); }

  static const shaman_action_state_t* cast_state( const action_state_t* s )
  { return debug_cast<const shaman_action_state_t*>( s ); }

  std::string full_name() const
  {
    std::string n = ab::data().name_cstr();
    return n.empty() ? ab::name_str : n;
  }

  virtual bool benefit_from_maelstrom_weapon() const
  {
    return affected_by_maelstrom_weapon && this->p()->buff.maelstrom_weapon->up();
  }

  // Some spells do not consume Maelstrom Weapon stacks always, so need to control this on
  // a spell to spell level
  virtual bool consume_maelstrom_weapon() const
  {
    if ( is_variant( spell_variant::THORIMS_INVOCATION ) )
    {
      return true;
    }

    // Don't consume MW stacks if the spell inherits MW stacks from a parent MW-consuming spell
    if ( this->mw_parent != nullptr )
    {
      return false;
    }

    return benefit_from_maelstrom_weapon() && !this->background;
  }

  virtual int maelstrom_weapon_stacks() const
  {
    if ( !benefit_from_maelstrom_weapon() )
    {
      return 0;
    }

    auto mw_stacks = std::min(
      as<int>( this->p()->spell.maelstrom_weapon_driver->effectN( 2 ).base_value() ),
      this->p()->buff.maelstrom_weapon->check()
    );

    if ( is_variant( spell_variant::THORIMS_INVOCATION ) )
    {
      mw_stacks = std::min( mw_stacks,
        as<int>( this->p()->talent.thorims_invocation->effectN( 1 ).base_value() ) );
    }

    return mw_stacks;
  }

  double compute_mw_multiplier()
  {
    if ( !this->p()->talent.maelstrom_weapon.ok() || !affected_by_maelstrom_weapon )
    {
      return 0.0;
    }

    double mw_multiplier = 0.0;
    mw_affected_stacks = maelstrom_weapon_stacks();
    mw_consumed_stacks = consume_maelstrom_weapon() ? mw_affected_stacks : 0;

    if ( mw_affected_stacks && affected_by_maelstrom_weapon )
    {
      mw_multiplier = this->p()->talent.maelstrom_weapon->effectN( 3 ).percent() *
        mw_affected_stacks;
    }

    if ( this->sim->debug && mw_multiplier )
    {
      this->sim->out_debug.print(
        "{} {} mw_affected={}, mw_benefit={}, mw_consumed={}, mw_stacks={}, mw_multiplier={}",
        this->player->name(), this->name(), affected_by_maelstrom_weapon,
        benefit_from_maelstrom_weapon(), mw_consumed_stacks,
        mw_affected_stacks, mw_multiplier );
    }

    return mw_multiplier;
  }

  virtual double composite_maelstrom_gain_coefficient( const action_state_t* state = nullptr ) const
  {
    double m = maelstrom_gain_coefficient;

    m *= p()->composite_maelstrom_gain_coefficient( state );

    return m;
  }

  virtual void trigger_maelstrom_gain( const action_state_t* state )
  {
    if ( maelstrom_gain == 0 )
    {
      return;
    }

    if (!state)
    {
      return;
    }

    double g = maelstrom_gain;
    g *= composite_maelstrom_gain_coefficient( state );

    if ( maelstrom_gain_per_target ) {
      g *= state->n_targets;
    }

    ab::player->resource_gain( RESOURCE_MAELSTROM, g, gain, this );
  }

  void init_finished() override
  {
    ab::init_finished();

    // Re-apply hasted cooldown if it has been parsed for the spell data, as shamans have shared cooldowns that may
    // replace data-derived individual action cooldowns.
    if ( ab::player->get_passive_value( ab::data(), "hasted_cooldown" )[ 1 ] )
    {
      ab::cooldown->hasted = true;
    }

    if ( ab::cooldown->duration > timespan_t::zero() )
    {
      p()->ability_cooldowns.push_back( this->cooldown );
    }
  }

  action_state_t* new_state() override
  { return new shaman_action_state_t( this, this->target ); }

  void snapshot_state( action_state_t* s, result_amount_type rt ) override
  {
    auto shaman_state = cast_state( s );

    shaman_state->variant_flags( this->exec_type );

    // Inherit Maelstrom Weapon multiplier from the parent. Presumes that the parent always executes
    // before this action.
    if ( mw_parent )
    {
        shaman_state->mw_mul = cast_state( mw_parent->execute_state )->mw_mul;
    }
    // Compute and cache Maelstrom Weapon multiplier before executing the spell. MW multiplier is
    // used to compute the damage of the spell, either during execute or during impact (Lava Burst).
    else
    {
      if ( affected_by_maelstrom_weapon )
      {
        shaman_state->mw_mul = compute_mw_multiplier();
      }
    }

    ab::snapshot_state( s, rt );
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    auto m = ab::composite_da_multiplier( state );

    m *= 1.0 + this->cast_state( state )->mw_mul;

    return m;
  }

  double recharge_multiplier( const cooldown_t& cd ) const override
  {
    double m = ab::recharge_multiplier( cd );

    m *= 1.0 / ( 1.0 + p()->buff.thundercharge->stack_value() );

    // TODO: Current presumption is self-cast, giving multiplicative effect
    m *= 1.0 / ( 1.0 + p()->buff.thundercharge->stack_value() );

    return m;
  }

  double action_da_multiplier() const override  // TODO Hawk: This is automated right?
  {
    double m = ab::action_da_multiplier();

    if ( ( affected_by_elemental_unity_fe_da && p()->talent.elemental_unity.ok() && p()->buff.fire_elemental->check() &&
           !p()->talent.primal_elementalist.ok() ) )
    {
      m *= 1.0 + p()->buff.fire_elemental->data().effectN( 4 ).percent();
    }

    if ( ( affected_by_elemental_unity_fe_da && p()->talent.elemental_unity.ok() && p()->buff.fire_elemental->check() &&
           p()->talent.primal_elementalist.ok() ) )
    {
      m *= 1.0 + p()->talent.elemental_unity->effectN( 1 ).percent();
    }

    if ( ( affected_by_elemental_unity_se_da && p()->talent.elemental_unity.ok() &&
           p()->buff.storm_elemental->check() && !p()->talent.primal_elementalist.ok() ) )
    {
      m *= 1.0 + p()->buff.storm_elemental->data().effectN( 4 ).percent();
    }
    if ( ( affected_by_elemental_unity_se_da && p()->talent.elemental_unity.ok() &&
           p()->buff.storm_elemental->check() && p()->talent.primal_elementalist.ok() ) )
    {
      m *= 1.0 + p()->talent.elemental_unity->effectN( 3 ).percent();
    }

    if ( ( affected_by_flametongue_da && p()->talent.flametongue_weapon.ok() &&
           p()->main_hand_weapon.buff_type == FLAMETONGUE_IMBUE ) )
    {
      m *= 1.0 + p()->spell.improved_flametongue_weapon->effectN( 1 ).percent();
    }

    if ( ( affected_by_lightning_cap_da && p()->talent.lightning_capacitor.ok() &&
           p()->buff.lightning_shield->check() ) )
    {
      m *= 1.0 + p()->talent.lightning_capacitor->effectN( 3 ).percent();
    }

    return m;
  }

  double action_ta_multiplier() const override
  {
    double m = ab::action_ta_multiplier();
    if ( affected_by_elemental_unity_fe_ta && p()->talent.elemental_unity.ok() &&
         !p()->talent.primal_elementalist.ok() )
    {
      m *= 1.0 + p()->buff.fire_elemental->data().effectN( 4 ).percent();
    }
    if ( affected_by_elemental_unity_fe_ta && p()->talent.elemental_unity.ok() &&
         p()->talent.primal_elementalist.ok() )
    {
      m *= 1.0 + p()->talent.elemental_unity->effectN( 1 ).percent();
    }

    if ( affected_by_elemental_unity_se_ta && p()->talent.elemental_unity.ok() &&
         !p()->talent.primal_elementalist.ok() )
    {
      m *= 1.0 + p()->buff.storm_elemental->data().effectN( 4 ).percent();
    }

    if ( affected_by_elemental_unity_se_ta && p()->talent.elemental_unity.ok() &&
        p()->talent.primal_elementalist.ok() )
    {
      m *= 1.0 + p()->talent.elemental_unity->effectN( 3 ).percent();
    }

    if ( ( affected_by_flametongue_ta && p()->talent.flametongue_weapon.ok() &&
           p()->main_hand_weapon.buff_type == FLAMETONGUE_IMBUE ) )
    {
      m *= 1.0 + p()->spell.improved_flametongue_weapon->effectN( 2 ).percent();
    }

    if ( ( affected_by_lightning_cap_ta && p()->talent.lightning_capacitor.ok() &&
           p()->buff.lightning_shield->check() ) )
    {
      m *= 1.0 + p()->talent.lightning_capacitor->effectN( 3 ).percent();
    }

    return m;
  }



  double execute_time_pct_multiplier() const override
  {
    auto mul = ab::execute_time_pct_multiplier();

    if ( affected_by_maelstrom_weapon )
    {
      mul *= 1.0 + this->p()->talent.maelstrom_weapon->effectN( 5 ).percent() * this->maelstrom_weapon_stacks();
    }

    if ( affected_by_ns_cast_time && p()->buff.natures_swiftness->check() && !ab::background )
    {
      mul *= 1.0 + p()->talent.natures_swiftness->effectN( 2 ).percent();
    }

    if ( affected_by_ans_cast_time && p()->buff.ancestral_swiftness->check() && !ab::background )
    {
      mul *= 1.0 + p()->buff.ancestral_swiftness->data().effectN( 2 ).percent();
    }

    return mul;
  }

  double cost_pct_multiplier() const override
  {
    double c = ab::cost_pct_multiplier();

    if ( affected_by_ns_cost && p()->buff.natures_swiftness->check() && !ab::background && ab::current_resource() != RESOURCE_MAELSTROM )
    {
      c *= 1.0 + p()->talent.natures_swiftness->effectN( 1 ).percent();
    }

    if ( affected_by_ans_cost && p()->buff.ancestral_swiftness->check() && !ab::background && ab::current_resource() != RESOURCE_MAELSTROM )
    {
      c *= 1.0 + p()->buff.ancestral_swiftness->data().effectN( 1 ).percent();
    }


    return c;
  }

  void execute() override
  {
    ab::execute();

    // Main hand swing timer resets if the MW-affected spell is not instant cast
    // Need to check this before spending the MW or autos will be lost.
    if ( affected_by_maelstrom_weapon && mw_affected_stacks < 5 )
    {
      if ( this->p()->main_hand_attack && this->p()->main_hand_attack->execute_event &&
           !this->background )
      {
        if ( this->sim->debug )
        {
          this->sim->out_debug.print( "{} resetting {} due to MW spell cast of {}",
                                     this->p()->name(), this->p()->main_hand_attack->name(),
                                     this->name() );
        }
        event_t::cancel( this->p()->main_hand_attack->execute_event );
        this->p()->main_hand_attack->schedule_execute();
        this->p()->proc.reset_swing_mw->occur();
      }
    }

    if ( p()->specialization() == SHAMAN_ELEMENTAL )
    {
      trigger_maelstrom_gain( ab::execute_state );
    }

    if ( p()->talent.flurry.ok() && this->execute_state->result == RESULT_CRIT )
    {
      p()->buff.flurry->trigger( p()->buff.flurry->max_stack() );
    }

    if ( ( affected_by_ns_cast_time ) && !(affected_by_stormkeeper_cast_time && p()->buff.stormkeeper->up()) && !ab::background && p()->buff.natures_swiftness->up())
    {
      p()->buff.natures_swiftness->decrement();
    }

    if ( ( affected_by_ans_cast_time ) && !(affected_by_stormkeeper_cast_time && p()->buff.stormkeeper->up()) && !ab::background && p()->buff.ancestral_swiftness->up())
    {
      p()->buff.ancestral_swiftness->decrement();
    }

    this->p()->consume_maelstrom_weapon( this->execute_state, mw_consumed_stacks );
  }

  void schedule_execute( action_state_t* execute_state = nullptr ) override
  {
    if ( !ab::background && unshift_ghost_wolf )
    {
      p()->buff.ghost_wolf->expire();
    }

    ab::schedule_execute( execute_state );
  }

  std::unique_ptr<expr_t> create_expression( util::string_view name ) override
  {
    auto split = util::string_split( name, "." );

    if ( util::str_compare_ci( split[ 0 ], "wolves_active_for" ) )
    {
      auto subexpr = name.substr( 18 );
      return std::make_unique<expr::wolves_active_for_t<std::greater_equal<double>>>( this, subexpr );
    }
    else if ( util::str_compare_ci( split[ 0 ], "wolves_expiring_in" ) )
    {
      auto subexpr = name.substr( 18 );
      return std::make_unique<expr::wolves_active_for_t<std::less<double>>>( this, subexpr );
    }
    else if ( util::str_compare_ci( name, "cooldown.higher_priority.min_remains" ) )
    {
      return std::make_unique<expr::hprio_cd_min_remains_expr_t>( this );
    }

    return ab::create_expression( name );
  }

};

// ==========================================================================
// Shaman Attack
// ==========================================================================

struct shaman_attack_t : public shaman_action_t<melee_attack_t>
{
private:
  using ab = shaman_action_t<melee_attack_t>;

public:
  bool may_proc_windfury;
  bool may_proc_flametongue;
  bool may_proc_lightning_shield;
  bool may_proc_hot_hand;
  bool may_proc_ability_procs;  // For things that explicitly state they proc from "abilities"

  stats::proc_tracker_t* proc_wf, *proc_ls, *proc_hh, *proc_ft;

  shaman_attack_t( util::string_view token, shaman_t* p, const spell_data_t* s,
                   unsigned variant_ = static_cast<unsigned>( spell_variant::NORMAL ) )
    : base_t( token, p, s, variant_ ),
      may_proc_windfury( p->talent.windfury_weapon.ok() ),
      may_proc_flametongue( p->talent.flametongue_weapon.ok() ),
      may_proc_lightning_shield( false ),
      may_proc_hot_hand( p->talent.hot_hand.ok() ),
      may_proc_ability_procs( true ),
      proc_wf( nullptr ),
      proc_hh( nullptr ),
      proc_ft( nullptr )
  {
    special    = true;
    may_glance = false;
  }

  void init() override
  {
    ab::init();

    if ( may_proc_flametongue )
    {
      may_proc_flametongue = this->id == 1 || this->id == 2 ||
        ( this->does_direct_damage() && data().dmg_class() == spell_type::SPELL_TYPE_MELEE &&
          ( data().flags( SX_REQ_MAIN_HAND ) || data().flags( SX_REQ_OFF_HAND ) ) );
    }

    if ( may_proc_windfury )
    {
      may_proc_windfury = this->id == 1 ||
        ( this->does_direct_damage() && data().dmg_class() == spell_type::SPELL_TYPE_MELEE &&
          data().flags( SX_REQ_MAIN_HAND ) );
    }

    if ( may_proc_hot_hand )
    {
      may_proc_hot_hand = ab::weapon != nullptr && !special;
    }

    may_proc_lightning_shield = ab::weapon != nullptr;

  }

  void init_finished() override
  {
    if ( may_proc_flametongue )
    {
      proc_ft = p()->tracker.register_proc( p()->talent.flametongue_weapon, this );
    }

    if ( may_proc_hot_hand )
    {
      proc_hh = p()->tracker.register_proc( p()->talent.hot_hand, this );
    }

    if ( may_proc_windfury )
    {
      proc_wf = p()->tracker.register_proc( p()->talent.windfury_weapon, this );
    }

    base_t::init_finished();
  }

  void execute() override
  {
    base_t::execute();

    if ( !special )
    {
      p()->buff.flurry->decrement();
    }

    p()->buff.tww2_enh_2pc->trigger();
  }

  void impact( action_state_t* state ) override
  {
    base_t::impact( state );

    // Bail out early if the result is a miss/dodge/parry/ms
    if ( !result_is_hit( state->result ) )
      return;

    p()->trigger_windfury_weapon( state );
    p()->trigger_flametongue_weapon( state );
    p()->trigger_hot_hand( state );
  }
};

// ==========================================================================
// Shaman Base Spell
// ==========================================================================

template <class Base>
struct shaman_spell_base_t : public shaman_action_t<Base>
{
private:
  using ab = shaman_action_t<Base>;

public:
  using base_t = shaman_spell_base_t<Base>;

  ancestor_cast ancestor_trigger;

  stats::proc_tracker_t* proc_deeply_rooted_elements;

  shaman_spell_base_t( util::string_view n, shaman_t* player,
                       const spell_data_t* s = spell_data_t::nil(),
                       unsigned type_ = static_cast<unsigned>( spell_variant::NORMAL ) )
    : ab( n, player, s, type_ ), ancestor_trigger( ancestor_cast::DISABLED ),
      proc_deeply_rooted_elements( nullptr )
  { }

  void init_finished() override
  {
    ab::init_finished();

    proc_deeply_rooted_elements = this->p()->tracker.register_proc(
      this->p()->talent.deeply_rooted_elements, this );
  }

  void execute() override
  {
    ab::execute();

    // for benefit tracking purpose
    this->p()->buff.spiritwalkers_grace->up();

    if ( this->p()->talent.aftershock->ok() &&
         this->current_resource() == RESOURCE_MAELSTROM &&
         this->last_resource_cost > 0 &&
         this->rng().roll( this->p()->talent.aftershock->effectN( 1 ).percent() ) )
    {
      this->p()->trigger_maelstrom_gain( this->last_resource_cost,
          this->p()->gain.aftershock );
      this->p()->proc.aftershock->occur();
    }
  }
  
  void impact( action_state_t* s ) override
  {
    ab::impact( s );
    
    if ( ( this->execute_state->action->id == 188389 ) ||
         ( this->is_variant( spell_variant::NORMAL ) && !this->background && s->chain_target == 0 ) )
    {
      this->p()->trigger_ancestor( ancestor_trigger, this->execute_state );
    }
  }
};

// ==========================================================================
// Shaman Offensive Spell
// ==========================================================================

struct elemental_overload_event_t : public event_t
{
  action_state_t* state;

  elemental_overload_event_t( action_state_t* s )
    : event_t( *s->action->player, timespan_t::from_millis( 400 ) ), state( s )
  { }

  ~elemental_overload_event_t() override
  {
    if ( state )
      action_state_t::release( state );
  }

  const char* name() const override
  {
    return "elemental_overload_event_t";
  }

  void execute() override
  {
    state->action->schedule_execute( state );
    state = nullptr;
  }
};

struct shaman_spell_t : public shaman_spell_base_t<spell_t>
{
  action_t* overload;

  bool affected_by_master_of_the_elements = false;
  proc_t* proc_moe;

  // Lightning Rod management
  double accumulated_lightning_rod_damage;
  event_t* lr_event;

  shaman_spell_t( util::string_view token, shaman_t* p, const spell_data_t* s = spell_data_t::nil(),
                 unsigned type_ = static_cast<unsigned>( spell_variant::NORMAL ) ) :
    base_t( token, p, s, type_ ), overload( nullptr ), proc_moe( nullptr ),
    accumulated_lightning_rod_damage( 0.0 ), lr_event( nullptr )
  { }

  void init_finished() override
  {
    if ( affected_by_master_of_the_elements && p()->talent.master_of_the_elements.ok() )
    {
      proc_moe = p()->get_proc( "Master of the Elements: " + full_name() );
    }

    base_t::init_finished();
  }

  void reset() override
  {
    base_t::reset();

    accumulated_lightning_rod_damage = 0.0;
    lr_event = nullptr;
  }

  double action_multiplier() const override
  {
    double m = base_t::action_multiplier();

    if ( affected_by_master_of_the_elements && p()->talent.master_of_the_elements.ok() )
    {
      m *= 1.0 + p()->buff.master_of_the_elements->value();
    }

    if ( affected_by_stormkeeper_damage && p()->buff.stormkeeper->up() && !p()->sk_during_cast )
    {
      m *= 1.0 + p()->buff.stormkeeper->value();

    }

    if ( affected_by_stormkeeper_damage_tier && p()->buff.stormkeeper->up() && !p()->sk_during_cast && !background)
    {
      m *= 1.0 + p()->buff.stormkeeper->data().effectN(4).percent();
    }

    return m;
  }


  double execute_time_pct_multiplier() const override
  {
    auto mul = base_t::execute_time_pct_multiplier();

    if ( affected_by_stormkeeper_cast_time && p()->buff.stormkeeper->up() && !p()->sk_during_cast )
    {
      // stormkeeper has a -100% value as effect 1
      mul *= 1.0 + p()->buff.stormkeeper->data().effectN( 1 ).percent();
    }

    return mul;
  }

  void execute() override
  {
    base_t::execute();

    if ( affected_by_master_of_the_elements && (!background || id == 188389) && p()->buff.master_of_the_elements->check() )
    {
      p()->buff.master_of_the_elements->decrement();
      proc_moe->occur();
    }

    p()->trigger_earthen_rage( execute_state );

    p()->buff.tww2_enh_2pc->trigger();
  }

  void schedule_travel( action_state_t* s ) override
  {
    trigger_elemental_overload( s );

    if ( p()->buff.ascendance->up() && s->chain_target == 0 )
    {
      switch ( id ) //TODO Hawk: needs refactoring. bind it at overload spell initialization
      {
          case 452201:
            p()->proc.ascendance_tempest_overload->occur();
            break;
          case 51505:
            p()->proc.ascendance_lava_burst_overload->occur();
            break;
          case 8042:
            p()->proc.ascendance_earth_shock_overload->occur();
            break;
          case 188196:
            p()->proc.ascendance_lightning_bolt_overload->occur();
            break;
          case 117014:
            p()->proc.ascendance_elemental_blast_overload->occur();
            break;
          case 61882:
          case 462620:
            p()->proc.ascendance_earthquake_overload->occur();
            break;
          case 188443:
            p()->proc.ascendance_chain_ligtning_overload->occur();
            break;
      }

      trigger_elemental_overload( s, 1.0 );
    }

    base_t::schedule_travel( s );
  }

  bool usable_moving() const override
  {
    if ( p()->buff.spiritwalkers_grace->check() || execute_time() == timespan_t::zero() )
      return true;

    return base_t::usable_moving();
  }

  virtual double overload_chance( const action_state_t* ) const
  {
    double chance = 0.0;

    if ( p()->mastery.elemental_overload->ok() )
    {
      chance += p()->cache.mastery_value();
    }

    // Add excessive amount to ensure overload proc with SK,
    // since chain spells divide chance by X
    if ( affected_by_stormkeeper_cast_time && p()->buff.stormkeeper->check() )
    {
      chance += 10.0;
    }

    return chance;
  }

  bool trigger_elemental_overload( const action_state_t* source_state, double override_chance = -1.0, bool proc_of_proc = false ) const
  {
    if ( !p()->mastery.elemental_overload->ok() )
    {
      return false;
    }

    if ( !overload )
    {
      return false;
    }

    double proc_chance = override_chance == -1.0
      ? overload_chance( source_state )
      : override_chance;

    if ( !rng().roll( proc_chance ) )
    {
      return false;
    }

    action_state_t* s = overload->get_state();
    s->target = source_state->target;
    overload->snapshot_state( s, result_amount_type::DMG_DIRECT );

    make_event<elemental_overload_event_t>( *sim, s );

    if ( sim->debug )
    {
      sim->out_debug.print( "{} elemental overload {}, chance={:.5f}{}, target={}", p()->name(),
        name(), proc_chance, override_chance != -1.0 ? " (overridden)" : "",
        source_state->target->name() );
    }

    if (!proc_of_proc && p()->talent.feedback_loop_3.ok())
    {
      trigger_elemental_overload( source_state, p()->talent.feedback_loop_3->effectN( 1 ).percent(), true );
    }

    return true;
  }

  void trigger_lightning_rod_debuff( player_t* target, timespan_t override_delay = timespan_t::min() )
  {
    auto delay = override_delay == timespan_t::min() ? rng().range( 10_ms, 100_ms ) : override_delay;

    sim->print_debug( "{} trigger_lightning_rod_debuff, action={}, delay={}, target={}",
      player->name(), name(), delay, target->name() );

    make_event( *sim, delay,
      [ this, target ]() { td( target )->debuff.lightning_rod->trigger(); } );
  }

  void accumulate_lightning_rod_damage( const action_state_t* state )
  {
    if ( !p()->talent.lightning_rod.ok() && !p()->talent.conductive_energy.ok() )
    {
      return;
    }

    if ( p()->buff_state_lightning_rod == 0 )
    {
      return;
    }

    accumulated_lightning_rod_damage += state->result_amount;

    sim->print_debug( "{} accumulate_lightning_rod_damage, action={}, amount={}, total={}",
      player->name(), name(), state->result_amount, accumulated_lightning_rod_damage );

    // Trigger a single "damage event" for Lightning Rod that after the cast, will iterate over
    // all the LR targets and proc the accumulated damage of a single cast on it. Note that this
    // event needs to be triggered before the debuff application event below to ensure that the
    // first application of the LR debuff will not trigger any damage on the target.
    if ( lr_event == nullptr )
    {
      sim->print_debug( "{} accumulate_lightning_rod_damage creating deferred damage event",
        player->name() );

      lr_event = make_event( *sim, [ this ]() {
        trigger_lightning_rod_damage();
        lr_event = nullptr;
      } );
    }
  }

  void trigger_lightning_rod_damage()
  {
    if ( !p()->talent.lightning_rod.ok() && !p()->talent.conductive_energy.ok() )
    {
      return;
    }

    range::for_each( sim->target_non_sleeping_list, [ this ]( player_t* target ) {
      if ( !td( target )->debuff.lightning_rod->up() )
      {
        return;
      }

      sim->print_debug( "{} trigger_lightning_rod_damage, action={}, target={}, amount={}",
        player->name(), name(), target->name(),
        accumulated_lightning_rod_damage * p()->constant.mul_lightning_rod );
      p()->action.lightning_rod->execute_on_target( target,
        accumulated_lightning_rod_damage * p()->constant.mul_lightning_rod );
    } );

    accumulated_lightning_rod_damage = 0.0;
  }

  virtual double stormbringer_proc_chance() const
  {
    double base_mul = p()->mastery.enhanced_elements->effectN( 3 ).mastery_value() *
      ( 1.0 + p()->talent.storms_wrath->effectN( 1 ).percent() );
    double base_chance = p()->spec.stormbringer->proc_chance() +
                         p()->cache.mastery() * base_mul;

    return base_chance;
  }
};

// ==========================================================================
// Shaman Heal
// ==========================================================================

struct shaman_heal_t : public shaman_spell_base_t<heal_t>
{
  double elw_proc_high, elw_proc_low, resurgence_gain;

  bool proc_tidal_waves, consume_tidal_waves;

  shaman_heal_t( util::string_view n, shaman_t* p, const spell_data_t* s = spell_data_t::nil(),
                 util::string_view options = {} )
    : base_t( n, p, s ),
      elw_proc_high( .2 ),
      elw_proc_low( 1.0 ),
      resurgence_gain( 0 ),
      proc_tidal_waves( false ),
      consume_tidal_waves( false )
  {
    parse_options( options );
  }

  double composite_total_spell_power() const override
  {
    double sp = base_t::composite_total_spell_power();

    if ( p()->main_hand_weapon.buff_type == EARTHLIVING_IMBUE )
      sp += p()->main_hand_weapon.buff_value * p()->composite_spell_power_multiplier();

    return sp;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = base_t::composite_target_multiplier( target );
    return m;
  }

  void impact( action_state_t* s ) override;

  void execute() override
  {
    base_t::execute();

    if ( consume_tidal_waves )
      p()->buff.tidal_waves->decrement();
  }

  virtual double deep_healing( const action_state_t* s )
  {
    if ( !p()->mastery.deep_healing->ok() )
      return 0.0;

    double hpp = ( 1.0 - s->target->health_percentage() / 100.0 );

    return 1.0 + hpp * p()->cache.mastery_value();
  }
};

namespace pet
{
// Simple helper to summon n (default 1) sleeping pet(s) from a container
template <typename T>
void summon( const T& container, timespan_t duration, size_t n = 1 )
{
  size_t summoned = 0;

  for ( size_t i = 0, end = container.size(); i < end; ++i )
  {
    auto ptr = container[ i ];
    if ( !ptr->is_sleeping() )
    {
      continue;
    }

    ptr->summon( duration );
    if ( ++summoned == n )
    {
      break;
    }
  }
}
// ==========================================================================
// Base Shaman Pet
// ==========================================================================

struct shaman_pet_t : public pet_t
{
  bool use_auto_attack;

  shaman_pet_t( shaman_t* owner, util::string_view name, bool guardian = true, bool auto_attack = true )
    : pet_t( owner->sim, owner, name, guardian ), use_auto_attack( auto_attack )
  {
    resource_regeneration = regen_type::DISABLED;

    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
  }

  shaman_t* o() const
  {
    return debug_cast<shaman_t*>( owner );
  }

  virtual void create_default_apl()
  {
    action_priority_list_t* def = get_action_priority_list( "default" );
    if ( use_auto_attack )
    {
      def->add_action( "auto_attack" );
    }
  }

  void init_action_list() override
  {
    pet_t::init_action_list();

    if ( action_list_str.empty() )
    {
      create_default_apl();
    }
  }

  void summon(timespan_t duration) override
  {
    pet_t::summon( duration );
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override;

  virtual attack_t* create_auto_attack()
  {
    return nullptr;
  }

  // Apparently shaman pets by default do not inherit attack speed buffs from owner
  double composite_melee_auto_attack_speed() const override
  {
    return o()->cache.attack_haste();
  }
};

// ==========================================================================
// Base Shaman Pet Action
// ==========================================================================

template <typename T_PET, typename T_ACTION>
struct pet_action_t : public T_ACTION
{
  using super = pet_action_t<T_PET, T_ACTION>;

  bool affected_by_elemental_unity_fe_da;
  bool affected_by_elemental_unity_fe_ta;
  bool affected_by_elemental_unity_se_da;
  bool affected_by_elemental_unity_se_ta;

  pet_action_t( T_PET* pet, util::string_view name, const spell_data_t* spell = spell_data_t::nil(),
                util::string_view options = {} )
    : T_ACTION( name, pet, spell ),
      affected_by_elemental_unity_fe_da( false ),
      affected_by_elemental_unity_fe_ta( false ),
      affected_by_elemental_unity_se_da( false ),
      affected_by_elemental_unity_se_ta( false )
  {
    this->parse_options( options );

    this->special  = true;
    this->may_crit = true;

        affected_by_elemental_unity_fe_da =
        T_ACTION::data().affected_by( o()->buff.fire_elemental->data().effectN( 4 ) );
    affected_by_elemental_unity_fe_ta =
        T_ACTION::data().affected_by( o()->buff.fire_elemental->data().effectN( 5 ) );
    affected_by_elemental_unity_se_da =
        T_ACTION::data().affected_by( o()->buff.storm_elemental->data().effectN( 3 ) );
    affected_by_elemental_unity_se_ta =
        T_ACTION::data().affected_by( o()->buff.storm_elemental->data().effectN( 4 ) );
  }

  T_PET* p() const
  {
    return debug_cast<T_PET*>( this->player );
  }

  shaman_t* o() const
  { return debug_cast<shaman_t*>( p()->owner ); }

  void init() override
  {
    T_ACTION::init();

    if ( !this->player->sim->report_pets_separately )
    {
      auto it = range::find_if( p()->o()->pet_list,
                                [ this ]( pet_t* pet ) { return this->player->name_str == pet->name_str; } );

      if ( it != p()->o()->pet_list.end() && this->player != *it )
      {
        this->stats = ( *it )->get_stats( this->name(), this );
      }
    }
  }

  double action_da_multiplier() const override
  {
    double m = T_ACTION::action_da_multiplier();

    if ( affected_by_elemental_unity_fe_da && o()->talent.elemental_unity.ok() &&
         o()->buff.fire_elemental->check())
    {
      m *= 1.0 + o()->buff.fire_elemental->data().effectN( 4 ).percent();
    }

    if ( affected_by_elemental_unity_se_da && o()->talent.elemental_unity.ok() &&
         o()->buff.storm_elemental->check() )
    {
      m *= 1.0 + o()->buff.storm_elemental->data().effectN( 4 ).percent();
    }

    return m;
  }

  double action_ta_multiplier() const override
  {
    double m = T_ACTION::action_ta_multiplier();

    if ( affected_by_elemental_unity_fe_ta && o()->talent.elemental_unity.ok() )
    {
      m *= 1.0 + o()->buff.fire_elemental->data().effectN( 5 ).percent();
    }

    if ( affected_by_elemental_unity_se_ta && o()->talent.elemental_unity.ok() )
    {
      m *= 1.0 + o()->buff.storm_elemental->data().effectN( 5 ).percent();
    }

    return m;
  }

  double cost() const override
  { return 0; }
};

// ==========================================================================
// Base Shaman Pet Melee Attack
// ==========================================================================

template <typename T_PET>
struct pet_melee_attack_t : public pet_action_t<T_PET, parse_action_effects_t<melee_attack_t>>
{
  using super = pet_melee_attack_t<T_PET>;

  pet_melee_attack_t( T_PET* pet, util::string_view name, const spell_data_t* spell = spell_data_t::nil(),
                      util::string_view options = {} )
    : pet_action_t<T_PET, parse_action_effects_t<melee_attack_t>>( pet, name, spell, options )
  {
    if ( this->school == SCHOOL_NONE )
      this->school = SCHOOL_PHYSICAL;

    if ( this->p()->owner_coeff.sp_from_sp > 0 || this->p()->owner_coeff.sp_from_ap > 0 )
    {
      this->spell_power_mod.direct = 1.0;
    }

    if ( this->data().ok() )
    {
      this->o()->apply_action_effects( this );
    }
  }

  void init() override
  {
    pet_action_t<T_PET, parse_action_effects_t<melee_attack_t>>::init();

    if ( !this->special )
    {
      this->weapon            = &( this->p()->main_hand_weapon );
      this->base_execute_time = this->weapon->swing_time;
    }
  }

  void execute() override
  {
    // If we're casting, we should clip a swing
    if ( this->time_to_execute > timespan_t::zero() && this->player->executing )
      this->schedule_execute();
    else
      pet_action_t<T_PET, parse_action_effects_t<melee_attack_t>>::execute();
  }
};

// ==========================================================================
// Generalized Auto Attack Action
// ==========================================================================

struct auto_attack_t : public melee_attack_t
{
  auto_attack_t( shaman_pet_t* player ) : melee_attack_t( "auto_attack", player )
  {
    assert( player->main_hand_weapon.type != WEAPON_NONE );
    player->main_hand_attack = player->create_auto_attack();
  }

  void execute() override
  {
    player->main_hand_attack->schedule_execute();
  }

  bool ready() override
  {
    if ( player->is_moving() )
      return false;
    return ( player->main_hand_attack->execute_event == nullptr );
  }
};

// ==========================================================================
// Base Shaman Pet Spell
// ==========================================================================

template <typename T_PET>
struct pet_spell_t : public pet_action_t<T_PET, parse_action_effects_t<spell_t>>
{
  using super = pet_spell_t<T_PET>;

  pet_spell_t( T_PET* pet, util::string_view name, const spell_data_t* spell = spell_data_t::nil(),
               util::string_view options = {} )
    : pet_action_t<T_PET, parse_action_effects_t<spell_t>>( pet, name, spell, options )
  {
    if ( this->data().ok() )
    {
      this->o()->apply_action_effects( this );
    }
  }
};

// ==========================================================================
// Base Shaman Pet Method Definitions
// ==========================================================================

action_t* shaman_pet_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "auto_attack" )
    return new auto_attack_t( this );

  return pet_t::create_action( name, options_str );
}

// ==========================================================================
// Feral Spirit
// ==========================================================================

struct base_wolf_t : public shaman_pet_t
{
  wolf_type_e wolf_type;

  base_wolf_t( shaman_t* owner, util::string_view name )
    : shaman_pet_t( owner, name ), wolf_type( SPIRIT_WOLF )
  {
    owner_coeff.ap_from_ap = 1.125;

    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.5 );
  }
};

template <typename T>
struct wolf_base_auto_attack_t : public pet_melee_attack_t<T>
{
  using super = wolf_base_auto_attack_t<T>;

  wolf_base_auto_attack_t( T* wolf, util::string_view n, const spell_data_t* spell = spell_data_t::nil(),
                           util::string_view options_str = {} )
    : pet_melee_attack_t<T>( wolf, n, spell )
  {
    this->parse_options( options_str );

    this->background = this->repeating = true;
    this->special                      = false;

    this->weapon            = &( this->p()->main_hand_weapon );
    this->weapon_multiplier = 1.0;

    this->base_execute_time = this->weapon->swing_time;
    this->school            = SCHOOL_PHYSICAL;
  }
};

struct spirit_wolf_t : public base_wolf_t
{
  struct fs_melee_t : public wolf_base_auto_attack_t<spirit_wolf_t>
  {
    fs_melee_t( spirit_wolf_t* player ) : super( player, "melee" )
    { }
  };

  spirit_wolf_t( shaman_t* owner ) : base_wolf_t( owner, owner->raptor_glyph ? "spirit_raptor" : "spirit_wolf" )
  {
    dynamic = true;
    npc_id = 29264;
  }

  attack_t* create_auto_attack() override
  {
    return new fs_melee_t( this );
  }
};

// ==========================================================================
// DOOM WOLVES OF NOT REALLY DOOM ANYMORE
// ==========================================================================

struct elemental_wolf_base_t : public base_wolf_t
{
  struct dw_melee_t : public wolf_base_auto_attack_t<elemental_wolf_base_t>
  {
    dw_melee_t( elemental_wolf_base_t* player ) : super( player, "melee" )
    { }
  };

  cooldown_t* special_ability_cd;

  elemental_wolf_base_t( shaman_t* owner, util::string_view name )
    : base_wolf_t( owner, name ), special_ability_cd( nullptr )
  {
    dynamic = true;
  }

  attack_t* create_auto_attack() override
  {
    return new dw_melee_t( this );
  }
};

struct frost_wolf_t : public elemental_wolf_base_t
{
  frost_wolf_t( shaman_t* owner ) : elemental_wolf_base_t( owner, owner->raptor_glyph ? "frost_raptor" : "frost_wolf" )
  {
    wolf_type = FROST_WOLF;
    npc_id = 100820;
    npc_suffix = "Frost";
  }
};

struct fire_wolf_t : public elemental_wolf_base_t
{
  fire_wolf_t( shaman_t* owner ) : elemental_wolf_base_t( owner, owner->raptor_glyph ? "fiery_raptor" : "fiery_wolf" )
  {
    wolf_type = FIRE_WOLF;
    npc_id = 100820;
    npc_suffix = "Fire";
  }
};

struct lightning_wolf_t : public elemental_wolf_base_t
{
  lightning_wolf_t( shaman_t* owner )
    : elemental_wolf_base_t( owner, owner->raptor_glyph ? "lightning_raptor" : "lightning_wolf" )
  {
    wolf_type = LIGHTNING_WOLF;
    npc_id = 100820;
    npc_suffix = "Lightning";
  }
};

// ==========================================================================
// Primal Elemental Base
// ==========================================================================

struct primal_elemental_t : public shaman_pet_t
{
  struct travel_t : public action_t
  {
    travel_t( player_t* player ) : action_t( ACTION_OTHER, "travel", player )
    { background = true; }

    void execute() override
    { player->current.distance = 1; }

    timespan_t execute_time() const override
    { return timespan_t::from_seconds( player->current.distance / 10.0 ); }

    bool ready() override
    { return ( player->current.distance > 1 ); }

    bool usable_moving() const override
    { return true; }
  };

  elemental type;

  primal_elemental_t( shaman_t* owner, elemental type_ )
    : shaman_pet_t( owner, elemental_name( type_ ), !is_pet_elemental( type_ ),
                    elemental_autoattack( type_ ) ),
      type( type_ )
  { }

  void create_default_apl() override
  {
    if ( use_auto_attack )
    {
      // Travel must come before auto attacks
      action_priority_list_t* def = get_action_priority_list( "default" );
      def->add_action( "travel" );
    }

    shaman_pet_t::create_default_apl();
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override
  {
    if ( name == "travel" )
      return new travel_t( this );

    return shaman_pet_t::create_action( name, options_str );
  }

  double composite_attack_power_multiplier() const override
  {
    double m = pet_t::composite_attack_power_multiplier();

    m *= 1.0 + o()->talent.primal_elementalist->effectN( 1 ).percent();

    return m;
  }

  double composite_spell_power_multiplier() const override
  {
    double m = pet_t::composite_spell_power_multiplier();

    m *= 1.0 + o()->talent.primal_elementalist->effectN( 1 ).percent();

    return m;
  }

  attack_t* create_auto_attack() override
  {
    auto attack               = new pet_melee_attack_t<primal_elemental_t>( this, "melee" );
    attack->background        = true;
    attack->repeating         = true;
    attack->special           = false;
    attack->school            = SCHOOL_PHYSICAL;
    attack->weapon_multiplier = 1.0;
    return attack;
  }
};

// ==========================================================================
// Earth Elemental
// ==========================================================================

struct earth_elemental_t : public primal_elemental_t
{
  earth_elemental_t( shaman_t* owner, elemental type_ ) :
    primal_elemental_t( owner, type_ )
  {
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
    owner_coeff.ap_from_sp      = 0.25;

    npc_id = type_ == elemental::GREATER_EARTH ? 95072 : 187322;
  }
};

// ==========================================================================
// Fire Elemental
// ==========================================================================

struct fire_elemental_t : public primal_elemental_t
{
  cooldown_t* meteor_cd;

  fire_elemental_t( shaman_t* owner, elemental type_ ) :
    primal_elemental_t( owner, type_ )
  {
    owner_coeff.sp_from_sp = 1.0;
    switch ( type_ )
    {
      case elemental::GREATER_FIRE:
        npc_id = 95061;
        break;
      case elemental::PRIMAL_FIRE:
        npc_id = 61029;
        break;
      default:
        break;
    }

    meteor_cd = get_cooldown( "meteor" );
  }

  struct meteor_t : public pet_spell_t<fire_elemental_t>
  {
    meteor_t( fire_elemental_t* player, util::string_view options )
      : super( player, "meteor", player->find_spell( 117588 ), options )
    {
      aoe = -1;
    }
  };

  struct fire_blast_t : public pet_spell_t<fire_elemental_t>
  {
    fire_blast_t( fire_elemental_t* player, util::string_view options )
      : super( player, "fire_blast", player->find_spell( 57984 ), options )
    { }

    bool usable_moving() const override
    { return true; }
  };

  struct immolate_t : public pet_spell_t<fire_elemental_t>
  {
    immolate_t( fire_elemental_t* player, util::string_view options )
      : super( player, "immolate", player->find_spell( 118297 ), options )
    {
      hasted_ticks = tick_may_crit = true;
    }
  };

  void create_default_apl() override
  {
    primal_elemental_t::create_default_apl();

    action_priority_list_t* def = get_action_priority_list( "default" );

    if ( type == elemental::PRIMAL_FIRE )
    {
      def->add_action( "meteor" );

      def->add_action( "immolate,target_if=!ticking" );
    }

    def->add_action( "fire_blast" );
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override
  {
    if ( name == "fire_blast" )
      return new fire_blast_t( this, options_str );
    if ( name == "meteor" )
      return new meteor_t( this, options_str );
    if ( name == "immolate" )
      return new immolate_t( this, options_str );

    return primal_elemental_t::create_action( name, options_str );
  }

  void summon( timespan_t duration ) override
  {
    primal_elemental_t::summon( duration );

    if ( type == elemental::PRIMAL_FIRE )
    {
      meteor_cd->reset( false );
    }
  }

  void dismiss( bool expired ) override
  {
    primal_elemental_t::dismiss( expired );

    o()->buff.fire_elemental->expire();
  }
};

// ==========================================================================
// Storm Elemental
// ==========================================================================

struct storm_elemental_t : public primal_elemental_t
{
  struct stormfury_aoe_t : public pet_spell_t<storm_elemental_t>
  {
    int tick_number   = 0;
    double damage_amp = 0.0;

    stormfury_aoe_t( storm_elemental_t* player, util::string_view options )
      : super( player, "stormfury_aoe", player->find_spell( 269005 ), options )
    {
      aoe        = -1;
      background = true;

      // parent spell (stormfury_t) has the damage increase percentage
      damage_amp = player->o()->find_spell( 157375 )->effectN( 2 ).percent();
    }

    double action_multiplier() const override
    {
      double m = pet_spell_t::action_multiplier();
      m *= std::pow( 1.0 + damage_amp, tick_number );
      return m;
    }
  };

  struct stormfury_t : public pet_spell_t<storm_elemental_t>
  {
    stormfury_aoe_t* breeze = nullptr;

    stormfury_t( storm_elemental_t* player, util::string_view options )
      : super( player, "stormfury", player->find_spell( 157375 ), options )
    {
      channeled   = true;
      tick_action = breeze = new stormfury_aoe_t( player, options );
    }

    void tick( dot_t* d ) override
    {
      breeze->tick_number = d->current_tick;
      pet_spell_t::tick( d );
    }

    bool ready() override
    {
      // 2025-08-27
      // Once uppon a time this spell was exclusive to Primal Elementalist Storm Elemental.
      // Right now it is suddenly available to all variants. I suspect this is
      // a bug. But time will tell.
      // if ( p()->o()->talent.primal_elementalist->ok() )
      // {
        return pet_spell_t<storm_elemental_t>::ready();
      // }
      // return false;
    }
  };

  struct wind_gust_t : public pet_spell_t<storm_elemental_t>
  {
    wind_gust_t( storm_elemental_t* player, util::string_view options )
      : super( player, "wind_gust", player->find_spell( 157331 ), options )
    { }
  };

  struct call_lightning_t : public pet_spell_t<storm_elemental_t>
  {
    call_lightning_t( storm_elemental_t* player, util::string_view options )
      : super( player, "call_lightning", player->find_spell( 157348 ), options )
    { }

    void execute() override
    {
      super::execute();

      p()->call_lightning->trigger();
    }
  };

  buff_t* call_lightning;
  cooldown_t* stormfury_cd;

  storm_elemental_t( shaman_t* owner, elemental type_ )
    : primal_elemental_t( owner, type_ ), call_lightning( nullptr )
  {
    owner_coeff.sp_from_sp = 1.0;
    switch ( type_ )
    {
      case elemental::GREATER_STORM:
        npc_id = 77936;
        break;
      case elemental::PRIMAL_STORM:
        npc_id = 77942;
        break;
      default:
        break;
    }

    stormfury_cd = get_cooldown( "stormfury" );
  }

  void create_default_apl() override
  {
    primal_elemental_t::create_default_apl();

    action_priority_list_t* def = get_action_priority_list( "default" );
    // 2025-08-27
    // Once uppon a time this spell was exclusive to Primal Elementalist Storm Elemental.
    // Right now it is suddenly available to all variants. I suspect this is
    // a bug. But time will tell.
    if ( type == elemental::PRIMAL_STORM )
    {
      def->add_action( "stormfury,if=buff.call_lightning.remains>=10" );
    }
    def->add_action( "call_lightning" );

    def->add_action( "wind_gust" );

  }

  void create_buffs() override
  {
    primal_elemental_t::create_buffs();

    call_lightning = make_buff( this, "call_lightning",
      find_spell( 157348 ) )->set_cooldown( timespan_t::zero() );
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = primal_elemental_t::composite_player_multiplier( school );

    if ( call_lightning->up() )
    {
      m *= 1.0 + call_lightning->data().effectN( 2 ).percent();
    }

    return m;
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override
  {
    if ( name == "stormfury" )
      return new stormfury_t( this, options_str );
    if ( name == "call_lightning" )
      return new call_lightning_t( this, options_str );
    if ( name == "wind_gust" )
      return new wind_gust_t( this, options_str );


    return primal_elemental_t::create_action( name, options_str );
  }

  void summon( timespan_t duration ) override
  {
    primal_elemental_t::summon( duration );

    if ( type == elemental::PRIMAL_STORM )
    {
      stormfury_cd->reset( false );
    }
  }

  void dismiss( bool expired ) override
  {
    primal_elemental_t::dismiss( expired );

    o()->buff.storm_elemental->expire();
    o()->buff.wind_gust->expire();
  }
};

// ==========================================================================
// Ancestor (Call of the Ancestors Talent)
// ==========================================================================

struct ancestor_t : public shaman_pet_t
{
  action_t* lava_burst;
  action_t* chain_lightning;
  action_t* elemental_blast;

  struct lava_burst_t : public pet_spell_t<ancestor_t>
  {
    lava_burst_t( ancestor_t* p ) : super( p, "lava_burst", p->find_spell( 447419 ) )
    {
      background = true;
      base_crit = 1.0;
    }
  };

  struct chain_lightning_t : public pet_spell_t<ancestor_t>
  {
    chain_lightning_t( ancestor_t* p ) : super( p, "chain_lightning", p->find_spell( 447425 ) )
    { background = true; }
  };

  struct elemental_blast_t : public pet_spell_t<ancestor_t>
  {
    elemental_blast_t( ancestor_t* p ) : super( p, "elemental_blast", p->find_spell( 465717 ) )
    {
        background = true;
        spell_power_mod.direct = data().effectN( 1 ).sp_coeff();
    }

    void execute() override
    {
      o()->trigger_elemental_blast_proc();
      pet_spell_t::execute();
    }
  };

  ancestor_t( shaman_t* owner )
    : shaman_pet_t( owner, "ancestor", true, false ),
    lava_burst( nullptr ), chain_lightning( nullptr ), elemental_blast( nullptr )
  {
    owner_coeff.sp_from_sp = 1.0;
    npc_id = 221177;
  }

  void init_background_actions() override
  {
    shaman_pet_t::init_background_actions();

    lava_burst = new lava_burst_t( this );
    chain_lightning = new chain_lightning_t( this );
    elemental_blast = new elemental_blast_t( this );
  }

  void trigger_cast( ancestor_cast type, player_t* target )
  {
    switch ( type )
    {
      case ancestor_cast::LAVA_BURST:
        lava_burst->execute_on_target( target );
        break;
      case ancestor_cast::CHAIN_LIGHTNING:
        chain_lightning->execute_on_target( target );
        break;
      case ancestor_cast::ELEMENTAL_BLAST:
        elemental_blast->execute_on_target( target );
        break;
      default:
        break;
    }
  }

  void dismiss( bool expiration ) override
  {
    if ( expiration && o()->talent.final_calling.ok() )
    {
      // Pick a random target to shoot the Elemental Blast on for now
      if ( !elemental_blast->target_list().empty() )
      {
        auto idx = static_cast<unsigned>( rng().range(
          as<double>( elemental_blast->target_list().size() ) ) );

        trigger_cast( ancestor_cast::ELEMENTAL_BLAST, elemental_blast->target_list()[ idx ] );
      }
    }

    shaman_pet_t::dismiss( expiration );
    if ( expiration && o()->rng_obj.ancient_fellowship->trigger() )
    {
      o()->summon_ancestor();
    }
  }
};
}  // end namespace pet

// ==========================================================================
// Shaman Secondary Spells / Attacks
// ==========================================================================

struct stormblast_t : public shaman_attack_t
{
  stormblast_t( shaman_t* p, util::string_view name ) :
    shaman_attack_t( name, p, p->find_spell( 390287 ) )
  {
    weapon = &( p->main_hand_weapon );
    background = may_crit = callbacks = false;
  }

  void init() override
  {
    shaman_attack_t::init();

    snapshot_flags = update_flags = ~STATE_MUL_PLAYER_DAM & ( STATE_MUL_DA | STATE_TGT_MUL_DA );

    may_proc_hot_hand = false;
    may_proc_ability_procs = false;

    p()->set_mw_proc_state( this, mw_proc_state::DISABLED );
  }
};

struct lightning_rod_damage_t : public shaman_spell_t
{
  lightning_rod_damage_t( shaman_t* p ) :
    shaman_spell_t( "lightning_rod", p, p->find_spell( 197568 ) )
  {
    background = true;
    may_crit = false;
  }

  void init() override
  {
    shaman_spell_t::init();

    // Apparently only Enhancement gains the benefits of target modifiers for Lightning Rod.
    snapshot_flags = update_flags = p()->specialization() == SHAMAN_ENHANCEMENT ? STATE_TGT_MUL_DA : 0;
  }
};

struct flametongue_weapon_spell_t : public shaman_spell_t  // flametongue_attack
{
  flametongue_weapon_spell_t( util::string_view n, shaman_t* player, weapon_t* /* w */ )
    : shaman_spell_t( n, player, player->find_spell( 10444 ) )
  {
    may_crit = background      = true;

    snapshot_flags          = STATE_AP;

    if ( player->main_hand_weapon.type != WEAPON_NONE )
    {
      attack_power_mod.direct *= player->main_hand_weapon.swing_time.total_seconds() / 2.6;
    }
  }
};

struct windfury_attack_t : public shaman_attack_t
{
  windfury_attack_t( util::string_view n, shaman_t* player, const spell_data_t* s, weapon_t* w )
    : shaman_attack_t( n, player, s )
  {
    weapon     = w;
    school     = SCHOOL_PHYSICAL;
    background = true;

    // Windfury can not proc itself
    may_proc_windfury = false;
  }
};

struct crash_lightning_unleashed_t : public shaman_attack_t
{
  crash_lightning_unleashed_t( shaman_t* p ) :
    shaman_attack_t( "crash_lightning_unleashed", p, p->find_spell( 1252431 ) )
  {
    weapon     = &( p->main_hand_weapon );
    background = true;
  }
};

struct crash_lightning_attack_t : public shaman_attack_t
{
  strike_variant strike_type; // Type of strike proccing the Crash Lightning damage

  crash_lightning_attack_t( shaman_t* p ) :
    shaman_attack_t( "crash_lightning_proc", p, p->find_spell( 195592 ) ),
    strike_type( strike_variant::NORMAL )
  {
    weapon     = &( p->main_hand_weapon );
    background = true;
    split_aoe_damage = true;
    may_proc_ability_procs = false;
  }

  double action_multiplier() const override
  {
    double m = shaman_attack_t::action_multiplier();

    if ( p()->buff.crash_lightning->up() )
    {
      m *= as<double>( p()->buff.crash_lightning->check() );
    }

    if ( strike_type == strike_variant::STORMFLURRY )
    {
      m *= p()->talent.stormflurry->effectN( 2 ).percent();
    }

    return m;
  }

  void init() override
  {
    shaman_attack_t::init();

    may_proc_hot_hand = false;
  }

  // Inherit "strike type" from the ability that triggers the aoe Crash Lightning proc
  void trigger( const action_state_t* strike_state, strike_variant st )
  {
    strike_type = st;
    set_target( strike_state->target );

    // Snapshot state here as Crash Lightning buff may be scheduled to expire on the same timestamp.
    auto state = get_state();
    state->target = strike_state->target;
    snapshot_state( state, result_amount_type::DMG_DIRECT );
    schedule_execute( state );
  }
};

struct stormstrike_attack_state_t : public shaman_action_state_t
{
  bool stormblast;

  stormstrike_attack_state_t( action_t* action_, player_t* target_ ) :
    shaman_action_state_t( action_, target_ ), stormblast( false )
  { }

  void initialize() override
  {
    shaman_action_state_t::initialize();

    stormblast = false;
  }

  void copy_state( const action_state_t* s ) override
  {
    shaman_action_state_t::copy_state( s );

    auto lbs = debug_cast<const stormstrike_attack_state_t*>( s );
    stormblast = lbs->stormblast;
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    shaman_action_state_t::debug_str( s );

    s << " stormblast=" << stormblast;

    return s;
  }
};

struct stormstrike_attack_t : public shaman_attack_t
{
  bool stormblast_trigger;
  strike_variant strike_type;

  action_t* stormblast;

  stormstrike_attack_t( util::string_view n, shaman_t* player, const spell_data_t* s, weapon_t* w,
                        strike_variant sf = strike_variant::NORMAL )
    : shaman_attack_t( n, player, s ), stormblast_trigger( false ), strike_type( sf ),
      stormblast( nullptr )
  {
    background = true;
    may_miss = may_dodge = may_parry = false;
    weapon = w;
    school = SCHOOL_PHYSICAL;

    if ( player->talent.stormblast.ok() )
    {
      std::string name_str { "stormblast_" };
      name_str += n;
      stormblast = new stormblast_t( player, name_str );
    }
  }

  void init() override
  {
    shaman_attack_t::init();

    if ( p()->talent.stormblast.ok() )
    {
      p()->dummy.stormblast->add_child( stormblast );
    }
  }

  virtual double stormsurge_proc_chance() const
  {
    double base_mul = p()->mastery.enhanced_elements->effectN( 3 ).mastery_value() *
      ( 1.0 + p()->talent.storms_wrath->effectN( 1 ).percent() );
    double base_chance = p()->spec.stormbringer->proc_chance() +
                          p()->cache.mastery() * base_mul;

    sim->print_debug( "{} stormsurge_proc_chance={}", player->name(), base_chance );
    return base_chance;
  }

  action_state_t* new_state() override
  { return new stormstrike_attack_state_t( this, target ); }

  double action_multiplier() const override
  {
    double m = shaman_attack_t::action_multiplier();

    if ( strike_type == strike_variant::STORMFLURRY )
    {
      m *= p()->talent.stormflurry->effectN( 2 ).percent();
    }

    return m;
  }

  void snapshot_internal( action_state_t* s, unsigned flags, result_amount_type rt ) override
  {
    shaman_attack_t::snapshot_internal( s, flags, rt );

    auto state = debug_cast<stormstrike_attack_state_t*>( s );
    state->stormblast = stormblast_trigger;
  }

  void execute() override
  {
    shaman_attack_t::execute();

    if ( p()->spec.stormbringer->ok() && rng().roll( stormsurge_proc_chance() ) )
    {
      p()->buff.stormsurge->trigger();
      p()->buff.stormblast->trigger();
      p()->cooldown.strike->reset( true );
    }

    stormblast_trigger = false;
  }

  void impact( action_state_t* s ) override
  {
    shaman_attack_t::impact( s );

    p()->trigger_stormblast( s );
  }
};

struct windstrike_attack_t : public stormstrike_attack_t
{
  windstrike_attack_t( util::string_view n, shaman_t* player, const spell_data_t* s, weapon_t* w,
                       strike_variant sf = strike_variant::NORMAL )
    : stormstrike_attack_t( n, player, s, w, sf )
  {
    ignores_armor = true;
  }
};

struct windlash_t : public shaman_attack_t
{
  double swing_timer_variance;

  windlash_t( util::string_view n, const spell_data_t* s, shaman_t* player, weapon_t* w, double stv )
    : shaman_attack_t( n, player, s ), swing_timer_variance( stv )
  {
    background = repeating = may_miss = may_dodge = may_parry = ignores_armor = true;
    may_proc_ability_procs = may_glance = special = false;
    weapon                                        = w;
    weapon_multiplier                             = 1.0;
    base_execute_time                             = w->swing_time;
    trigger_gcd                                   = timespan_t::zero();
  }

  // Windlash is a special ability, but treated as an autoattack in terms of proccing
  proc_types proc_type() const override
  {
    return PROC1_MELEE;
  }

  timespan_t execute_time() const override
  {
    timespan_t t = shaman_attack_t::execute_time();

    if ( swing_timer_variance > 0 )
    {
      timespan_t st = timespan_t::from_seconds(
          const_cast<windlash_t*>( this )->rng().gauss( t.total_seconds(), t.total_seconds() * swing_timer_variance ) );
      if ( sim->debug )
        sim->out_debug.printf( "Swing timer variance for %s, real_time=%.3f swing_timer=%.3f", name(),
                               t.total_seconds(), st.total_seconds() );

      return st;
    }
    else
      return t;
  }
};

// Ground AOE pulse
struct ground_aoe_spell_t : public spell_t
{
  ground_aoe_spell_t( shaman_t* p, util::string_view name, const spell_data_t* spell ) : spell_t( name, p, spell )
  {
    aoe        = -1;
    callbacks  = false;
    ground_aoe = background = may_crit = true;
  }
};

struct lightning_shield_damage_t : public shaman_spell_t
{
  lightning_shield_damage_t( shaman_t* player )
    : shaman_spell_t( "lightning_shield", player, player->find_spell( 273324 ) )
  {
    background = true;
    callbacks  = false;
  }
};

struct lightning_shield_defense_damage_t : public shaman_spell_t
{
  lightning_shield_defense_damage_t( shaman_t* player )
    : shaman_spell_t( "lifghtning_shield_defense_damage", player, player->find_spell( 192109 ) )
  {
    background = true;
    callbacks  = false;
  }
};

struct imbuement_mastery_t : public shaman_spell_t  // Imbuement Mastery damage
{
  imbuement_mastery_t( shaman_t* player )
    : shaman_spell_t( "flametongue_attack_imbuement_mastery", player,
        player->find_spell( 467386 ) )
  {
    may_crit = background = true;
    aoe = -1;
    reduced_aoe_targets = 5.0;
  }
};

struct sundering_splitstream_t : public shaman_attack_t
{
  sundering_splitstream_t( shaman_t* player ) :
    shaman_attack_t( "sundering_splitstream", player, player->find_spell( 467283 ) )
  {
    weapon = &( player->main_hand_weapon );
    aoe    = -1;  // TODO: This is likely not going to affect all enemies but it will do for now
    base_multiplier = player->talent.splitstream->effectN( 1 ).percent();
  }

  void execute() override
  {
    shaman_attack_t::execute();

    p()->trigger_earthsurge( execute_state, p()->talent.splitstream->effectN( 1 ).percent() );
  }
};

// Elemental overloads

struct elemental_overload_spell_t : public shaman_spell_t
{
  shaman_spell_t* parent;

  elemental_overload_spell_t( shaman_t* p, util::string_view name, const spell_data_t* s,
                              shaman_spell_t* parent_, double multiplier = -1.0,
                              unsigned type_ = static_cast<unsigned>( spell_variant::NORMAL ) )
    : shaman_spell_t( name, p, s, type_ ), parent( parent_ )
  {
    base_execute_time = timespan_t::zero();
    background        = true;
    callbacks         = false;

    base_multiplier *= p->mastery.elemental_overload->effectN( 2 ).percent();

    // multiplier is used by Mountains Will Fall and is applied after
    // overload damage multiplier is calculated.
    if ( multiplier != -1.0 )
    {
      base_multiplier *= multiplier;
    }
  }

  void init_finished() override
  {
    shaman_spell_t::init_finished();

    // Generate a new stats object for the elemental overload spell based on the parent
    // stats object name. This will more or less always let us build correct stats
    // hierarchies for the overload-capable spells, so that the various different
    // (reporting) hierarchies function correctly.
    /*
    auto stats_ = player->get_stats( parent->stats->name_str + "_overload", this );
    stats_->school = get_school();
    stats = stats_;
    parent->stats->add_child( stats );
    */
    parent->add_child( this );
  }

  void snapshot_internal( action_state_t* s, unsigned flags, result_amount_type rt ) override
  {
    shaman_spell_t::snapshot_internal( s, flags, rt );

    cast_state( s )->variant_flags( parent->variant_flags() );
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    if ( p()->buff.ascendance->up() )
    {
      m *= 1.0 + p()->spell.ascendance->effectN( 8 ).percent();
    }

    return m;
  }
};


struct thunderstrike_ward_damage_t : public shaman_spell_t
{
  thunderstrike_ward_damage_t( shaman_t* player )
    : shaman_spell_t( "thunderstrike", player, player->find_spell( 462763 ) )
  {

    background = true;
  }
};

struct earthen_rage_damage_t : public shaman_spell_t
{
  earthen_rage_damage_t( shaman_t* p ) : shaman_spell_t( "earthen_rage", p, p -> find_spell( 170379 ) )
  {
    background = proc = true;
    callbacks = false;
  }
};

struct earthen_rage_event_t : public event_t
{
  shaman_t* player;
  timespan_t end_time;

  earthen_rage_event_t( shaman_t* p, timespan_t et ) :
    event_t( *p, next_event( p ) ), player( p ), end_time( et )
  { }

  timespan_t next_event( shaman_t* p ) const
  {
    return p->rng().range(
      1_ms,
      2.0 * p->spell.earthen_rage->effectN( 1 ).period() * p->composite_spell_cast_speed()
    );
  }

  void set_end_time( timespan_t t )
  { end_time = t; }

  void execute() override
  {
    if ( sim().current_time() > end_time )
    {
      sim().print_debug( "{} earthen_rage fades", player->name() );
      player->earthen_rage_event = nullptr;
      return;
    }

    if ( !player->earthen_rage_target->is_sleeping() )
    {
      sim().print_debug( "{} triggers earthen_rage on target={}", player->name(),
        player->earthen_rage_target->name() );

      player->action.earthen_rage->execute_on_target( player->earthen_rage_target );
    }

    player->earthen_rage_event = make_event<earthen_rage_event_t>( sim(), player, end_time );
    sim().print_debug( "{} schedules earthen_rage, next_event={}, tick_time={}",
      player->name(), player->earthen_rage_event->occurs(),
      player->earthen_rage_event->occurs() - sim().current_time() );
  }
};

// Honestly why even bother with resto heals?
// shaman_heal_t::impact ====================================================

void shaman_heal_t::impact( action_state_t* s )
{
  // Todo deep healing to adjust s -> result_amount by x% before impacting
  if ( sim->debug && p()->mastery.deep_healing->ok() )
  {
    sim->out_debug.printf( "%s Deep Heals %s@%.2f%% mul=%.3f %.0f -> %.0f", player->name(), s->target->name(),
                           s->target->health_percentage(), deep_healing( s ), s->result_amount,
                           s->result_amount * deep_healing( s ) );
  }

  s->result_amount *= deep_healing( s );

  base_t::impact( s );

  if ( proc_tidal_waves )
    p()->buff.tidal_waves->trigger( p()->buff.tidal_waves->data().initial_stacks() );

  if ( s->result == RESULT_CRIT )
  {
    if ( resurgence_gain > 0 )
      p()->resource_gain( RESOURCE_MANA, resurgence_gain, p()->gain.resurgence );
  }

  if ( p()->main_hand_weapon.buff_type == EARTHLIVING_IMBUE )
  {
    double chance = ( s->target->resources.pct( RESOURCE_HEALTH ) > .35 ) ? elw_proc_high : elw_proc_low;

    if ( rng().roll( chance ) )
    {
      // Todo proc earthliving on target
    }
  }
}

// ==========================================================================
// Shaman Attack
// ==========================================================================

// shaman_attack_t::impact ============================================

// Melee Attack =============================================================

struct melee_t : public shaman_attack_t
{
  int sync_weapons;
  bool first;
  double swing_timer_variance;

  melee_t( util::string_view name, const spell_data_t* s, shaman_t* player, weapon_t* w, int sw, double stv )
    : shaman_attack_t( name, player, s ), sync_weapons( sw ), first( true ), swing_timer_variance( stv )
  {
    id                                  = w->slot == SLOT_MAIN_HAND ? 1U : 2U;
    background = repeating = may_glance = true;
    allow_class_ability_procs           = true;
    not_a_proc                          = true;
    special                             = false;
    trigger_gcd                         = timespan_t::zero();
    weapon                              = w;
    weapon_multiplier                   = 1.0;
    base_execute_time                   = w->swing_time;

    if ( p()->specialization() == SHAMAN_ENHANCEMENT && p()->dual_wield() )
      base_hit -= 0.19;
  }

  void reset() override
  {
    shaman_attack_t::reset();

    first = true;
  }

  timespan_t execute_time() const override
  {
    timespan_t t = shaman_attack_t::execute_time();
    if ( first )
    {
      return ( weapon->slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t / 2, timespan_t::zero() ) : t / 2 )
                                               : timespan_t::zero();
    }

    if ( swing_timer_variance > 0 )
    {
      timespan_t st = timespan_t::from_seconds(
          const_cast<melee_t*>( this )->rng().gauss( t.total_seconds(), t.total_seconds() * swing_timer_variance ) );
      if ( sim->debug )
        sim->out_debug.printf( "Swing timer variance for %s, real_time=%.3f swing_timer=%.3f", name(),
                               t.total_seconds(), st.total_seconds() );
      return st;
    }
    else
      return t;
  }

  void execute() override
  {
    if ( first )
    {
      first = false;
    }

    shaman_attack_t::execute();
  }

  void impact( action_state_t* state ) override
  {
    shaman_attack_t::impact( state );
  }
};

// Auto Attack ==============================================================

struct auto_attack_t : public shaman_attack_t
{
  int sync_weapons;
  double swing_timer_variance;

  auto_attack_t( shaman_t* player, util::string_view options_str )
    : shaman_attack_t( "auto_attack", player, spell_data_t::nil() ), sync_weapons( 0 ), swing_timer_variance( 0.00 )
  {
    add_option( opt_bool( "sync_weapons", sync_weapons ) );
    add_option( opt_float( "swing_timer_variance", swing_timer_variance ) );
    parse_options( options_str );
    ignore_false_positive  = true;
    may_proc_ability_procs = false;

    assert( p()->main_hand_weapon.type != WEAPON_NONE );

    p()->melee_mh = new melee_t( "Main Hand", spell_data_t::nil(), player, &( p()->main_hand_weapon ), sync_weapons,
                                 swing_timer_variance );
    p()->melee_mh->school = SCHOOL_PHYSICAL;

    if ( ( player->talent.deeply_rooted_elements.ok() || player->talent.ascendance.ok() ||
           player->sets->has_set_bonus( HERO_STORMBRINGER, TWW3, B2 ) ) &&
         player->specialization() == SHAMAN_ENHANCEMENT )
    {
      p()->ascendance_mh = new windlash_t( "Windlash", player->find_spell( 114089 ), player, &( p()->main_hand_weapon ),
                                           swing_timer_variance );
    }

    p()->main_hand_attack = p()->melee_mh;

    if ( p()->off_hand_weapon.type != WEAPON_NONE && player->specialization() == SHAMAN_ENHANCEMENT )
    {
      if ( !p()->dual_wield() )
        return;

      p()->melee_oh = new melee_t( "Off-Hand", spell_data_t::nil(), player, &( p()->off_hand_weapon ), sync_weapons,
                                   swing_timer_variance );
      p()->melee_oh->school = SCHOOL_PHYSICAL;

      if ( player->talent.deeply_rooted_elements.ok() || player->talent.ascendance.ok() ||
           player->sets->has_set_bonus( HERO_STORMBRINGER, TWW3, B2 ) )
      {
        p()->ascendance_oh = new windlash_t( "Windlash Off-Hand", player->find_spell( 114093 ), player,
            &( p()->off_hand_weapon ), swing_timer_variance );
      }

      p()->off_hand_attack = p()->melee_oh;
    }

    trigger_gcd = timespan_t::zero();
  }

  void execute() override
  {
    p()->main_hand_attack->schedule_execute();
    if ( p()->off_hand_attack )
      p()->off_hand_attack->schedule_execute();
  }

  bool ready() override
  {
    if ( p()->is_moving() )
      return false;
    return ( p()->main_hand_attack->execute_event == nullptr );  // not swinging
  }
};

// Molten Weapon Dot ============================================================

struct molten_weapon_dot_t : public residual_action::residual_periodic_action_t<spell_t>
{
  molten_weapon_dot_t( shaman_t* p ) : base_t( "molten_weapon", p, p->find_spell( 271924 ) )
  {
    // spell data seems messed up - need to whitelist?
    dual           = true;
    dot_duration   = timespan_t::from_seconds( 4 );
    base_tick_time = timespan_t::from_seconds( 2 );
    tick_zero      = false;
    hasted_ticks   = false;
  }
};

// Lava Lash Attack =========================================================

struct lava_lash_t : public shaman_attack_t
{
  molten_weapon_dot_t* mw_dot;
  unsigned max_spread_targets;

  stats::proc_tracker_t* proc_lively_totems;

  lava_lash_t( shaman_t* player, unsigned type_, util::string_view options_str = {} ) :
    shaman_attack_t( "lava_lash", player, player->talent.lava_lash, type_ ),
    max_spread_targets( as<unsigned>( p()->talent.molten_assault->effectN( 2 ).base_value() ) ),
    proc_lively_totems( nullptr )
  {
    school = SCHOOL_FIRE;
    // Add a 12 yard radius to support Flame Shock spreading in 11.0
    radius = 12.0;

    parse_options( options_str );
    weapon = &( player->off_hand_weapon );

    if ( weapon->type == WEAPON_NONE )
      background = true;  // Do not allow execution.

    if ( is_variant( spell_variant::TWW3_SPELL ) )
    {
      background = true;
      cooldown = player->get_cooldown( "lava_lash_tww3" );
      base_multiplier *= player->buff.elemental_overflow->data().effectN( 1 ).percent();
    }
  }

  void init() override
  {
    shaman_attack_t::init();

    add_child( p()->action.splitstream );
  }

  void init_finished() override
  {
    shaman_attack_t::init_finished();

    if ( p()->talent.lively_totems.ok() )
    {
      proc_lively_totems = p()->tracker.register_proc( p()->talent.lively_totems, this );
    }
  }

  double action_multiplier() const override
  {
    double m = shaman_attack_t::action_multiplier();

    // Flametongue imbue only increases Lava Lash damage if it is imbued on the off-hand
    // weapon
    if ( p()->off_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
    {
      m *= 1.0 + data().effectN( 2 ).percent();
    }

    return m;
  }

  void execute() override
  {
    shaman_attack_t::execute();

    p()->trigger_elemental_assault( execute_state );

    p()->trigger_whirling_fire( execute_state );

    p()->trigger_splitstream( execute_state );

    if ( p()->talent.lively_totems.ok() && p()->rng_obj.lively_totems_ptr->trigger() )
    {
      // 2024-07-10: Searing Totem death seems to be delayed from basically nothing to approximately
      // 850ms. Makes it possible to get an extra searing bolt if the timing is right.
      auto duration = p()->buff.lively_totems->buff_duration() +
        timespan_t::from_seconds( rng().range( 0.85 ) );
      p()->pet.searing_totem.spawn( duration );
      // 2026-02-08: In-game, the Lively Totems buff is not synchronized to Searing Totem duration,
      // and will fade before the totem does.
      p()->buff.lively_totems->trigger();
      proc_lively_totems->occur();
    }

    p()->trigger_lively_totems( execute_state );

    if ( p()->sets->has_set_bonus( HERO_TOTEMIC, TWW3, B4 ) || p()->talent.primal_catalyst.ok() )
    {
      p()->trigger_elemental_overflow( execute_state, p()->action.tww3_lava_lash );
    }

    if ( p()->buff.lightning_strikes->up() )
    {
      p()->generate_maelstrom_weapon( this, 1 );
      p()->buff.lightning_strikes->decrement();
    }

    if ( p()->talent.ashen_catalyst.ok() &&
      td( execute_state->target )->dot.flame_shock->is_ticking() )
    {
      p()->cooldown.lava_lash->adjust( -p()->talent.ashen_catalyst->effectN( 1 ).time_value(),
        false );
    }
  }

  void impact( action_state_t* state ) override
  {
    shaman_attack_t::impact( state );

    td( state->target )->debuff.lashing_flames->trigger();

    trigger_flame_shock( state );

    if ( result_is_hit( state->result ) )
    {
      p()->trigger_crash_lightning_proc( execute_state, strike_variant::NORMAL );
    }

    p()->trigger_ride_the_lightning( state, p()->action.chain_lightning_ll_rtl );
  }

  void move_random_target( std::vector<player_t*>& in, std::vector<player_t*>& out ) const
  {
    auto idx = rng().range( 0U, as<unsigned>( in.size() ) );
    out.push_back( in[ idx ] );
    in.erase( in.begin() + idx );
  }

  static std::string actor_list_str( const std::vector<player_t*>& actors,
                                     util::string_view             delim = ", " )
  {
    static const auto transform_fn = []( player_t* t ) { return t->name(); };
    std::vector<const char*> tmp;

    range::transform( actors, std::back_inserter( tmp ), transform_fn );

    return tmp.size() ? util::string_join( tmp, delim ) : "none";
  }

  void trigger_flame_shock( const action_state_t* state ) const
  {
    if ( !p()->talent.molten_assault->ok() )
    {
      return;
    }

    if ( !td( state->target )->dot.flame_shock->is_ticking() )
    {
      return;
    }

    // Targets to spread Flame Shock to
    std::vector<player_t*> targets;
    // Maximum number of spreads, deduct one from available targets since the target of this Lava
    // Lash execution (always receives it) is in there
    unsigned actual_spread_targets = std::min( max_spread_targets,
        as<unsigned>( target_list().size() ) - 1U );

    if ( actual_spread_targets == 0 )
    {
      // Always trigger Flame Shock on main target
      p()->trigger_secondary_flame_shock( state->target, spell_variant::NORMAL );
      return;
    }

    // Lashing Flames, no Flame Shock
    std::vector<player_t*> lf_no_fs_targets,
    // Lashing Flames, Flame Shock
                           lf_fs_targets,
    // No Lashing Flames, no Flame Shock
                           no_lf_no_fs_targets,
    // No Lashing Flames, Flame Shock
                           no_lf_fs_targets;

    // Target of the Lava Lash has Lashing Flames
    bool mt_has_lf = td( state->target )->debuff.lashing_flames->check();

    // Categorize all available targets (within 8 yards of the main target) based on Lashing
    // Flames and Flame Shock state.
    range::for_each( target_list(), [&]( player_t* t ) {
      // Ignore main target
      if ( t == state->target )
      {
        return;
      }

      if ( td( t )->debuff.lashing_flames->check() &&
           !td( t )->dot.flame_shock->is_ticking() )
      {
        lf_no_fs_targets.push_back( t );
      }
      else if ( td( t )->debuff.lashing_flames->check() &&
                td( t )->dot.flame_shock->is_ticking() )
      {
        lf_fs_targets.push_back( t );
      }
      else if ( !td( t )->debuff.lashing_flames->check() &&
                 td( t )->dot.flame_shock->is_ticking() )
      {
        no_lf_fs_targets.push_back( t );
      }
      else if ( !td( t )->debuff.lashing_flames->check() &&
                !td( t )->dot.flame_shock->is_ticking() )
      {
        no_lf_no_fs_targets.push_back( t );
      }
    } );

    if ( sim->debug )
    {
      sim->out_debug.print( "{} spreads flame_shock, n_fs={} ll_target={} "
                            "state={}LF{}FS, targets_in_range={{ {} }}",
        player->name(), p()->active_flame_shock.size(), state->target->name(),
        td( state->target )->debuff.lashing_flames->check() ? '+' : '-',
        td( state->target )->dot.flame_shock->is_ticking() ? '+' : '-',
        actor_list_str( target_list() ) );

      sim->out_debug.print( "{} +LF-FS: targets={{ {} }}", player->name(),
          actor_list_str( lf_no_fs_targets ) );
      sim->out_debug.print( "{} -LF-FS: targets={{ {} }}", player->name(),
          actor_list_str( no_lf_no_fs_targets ) );
      sim->out_debug.print( "{} +LF+FS: targets={{ {} }}", player->name(),
          actor_list_str( lf_fs_targets ) );
      sim->out_debug.print( "{} -LF+FS: targets={{ {} }}", player->name(),
          actor_list_str( no_lf_fs_targets ) );
    }

    // 1) Randomly select targets with Lashing Flame and no Flame Shock, unless there already are
    // the maximum number of Flame Shocked targets with Lashing Flames up.
    while ( lf_no_fs_targets.size() > 0 &&
            ( lf_fs_targets.size() + mt_has_lf ) < p()->max_active_flame_shock &&
            targets.size() < actual_spread_targets )
    {
      move_random_target( lf_no_fs_targets, targets );
    }

    // 2) Randomly select targets without Lashing Flames and Flame Shock, but only if we are not at
    // Flame Shock cap.
    while ( no_lf_no_fs_targets.size() > 0 &&
            ( lf_fs_targets.size() + no_lf_fs_targets.size() + 1U ) < p()->max_active_flame_shock &&
            targets.size() < actual_spread_targets )
    {
      move_random_target( no_lf_no_fs_targets, targets );
    }

    // 3) Randomly select targets that have Lashing Flames and Flame Shock on them. This prioritizes
    // refreshing existing Flame Shocks on targets with Lashing Flames up.
    while ( lf_fs_targets.size() > 0 && targets.size() < actual_spread_targets )
    {
      move_random_target( lf_fs_targets, targets );
    }

    // 4) Randomly select targets that don't have Lashing Flames but have Flame Shock on them. This
    // prioritizes refreshing existing Flame Shocks on targets when we are at maximum Flame Shocks,
    // preventing random expirations.
    while ( no_lf_fs_targets.size() > 0 && targets.size() < actual_spread_targets )
    {
      move_random_target( no_lf_fs_targets, targets );
    }

    if ( sim->debug )
    {
      sim->out_debug.print( "{} selected targets={{ {} }}",
          player->name(), actor_list_str( targets ) );
    }

    // Always trigger Flame Shock on main target
    p()->trigger_secondary_flame_shock( state->target, spell_variant::NORMAL );

    range::for_each( targets, [ shaman = p() ]( player_t* target ) {
      shaman->trigger_secondary_flame_shock( target, spell_variant::NORMAL );
    } );
  }
};

// Stormstrike Attack =======================================================

struct stormstrike_state_t : public shaman_action_state_t
{
  bool stormblast;

  stormstrike_state_t( action_t* action_, player_t* target_ ) :
    shaman_action_state_t( action_, target_ ), stormblast( false )
  { }

  void initialize() override
  {
    shaman_action_state_t::initialize();

    stormblast = false;
  }

  void copy_state( const action_state_t* s ) override
  {
    shaman_action_state_t::copy_state( s );

    auto lbs = debug_cast<const stormstrike_state_t*>( s );
    stormblast = lbs->stormblast;
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    shaman_action_state_t::debug_str( s );

    s << " stormblast=" << stormblast;

    return s;
  }
};

struct stormstrike_base_t : public shaman_attack_t
{
  struct stormflurry_event_t : public event_t
  {
    stormstrike_base_t* action;
    player_t* target;

    bool stormblast;

    stormflurry_event_t( stormstrike_base_t* a, player_t* t, timespan_t delay, bool sb ) :
      event_t( *a->player, delay ), action( a ), target( t ), stormblast( sb )
    { }

    const char* name() const override
    { return "stormflurry_event"; }

    void execute() override
    {
      // Ensure we can execute on target, before doing anything
      if ( !action->target_ready( target ) )
      {
        action->p()->proc.stormflurry_failed->occur();
        return;
      }

      action->trigger_stormflurry( target, stormblast );
      action->proc_stormflurry->occur();
    }
  };

  stormstrike_attack_t *mh, *oh;
  strike_variant strike_type;
  stats::proc_tracker_t* proc_stormflurry;

  stormstrike_base_t( shaman_t* player, util::string_view name, const spell_data_t* spell,
                      util::string_view options_str, strike_variant t = strike_variant::NORMAL )
    : shaman_attack_t( name, player, spell ), mh( nullptr ), oh( nullptr ),
      strike_type( t ), proc_stormflurry( nullptr )
  {
    parse_options( options_str );

    weapon             = &( p()->main_hand_weapon );
    weapon_multiplier  = 0.0;
    may_crit           = false;
    school             = SCHOOL_PHYSICAL;

    switch ( strike_type )
    {
      case strike_variant::STORMFLURRY:
        cooldown = player->get_cooldown( "__strike_secondary" );
        cooldown->duration = 0_ms;

        dual = true;
        background = true;
        base_costs[ RESOURCE_MANA ] = 0.0;
        break;
      default:
        cooldown = p()->cooldown.strike;
        cooldown->charges = data().charges();
        cooldown->duration = data().charge_cooldown();
        cooldown->action = this;
        break;
    }
  }

  void trigger_stormflurry( player_t* t, bool stormblast )
  {
    auto s = get_state();

    snapshot_state( s, amount_type( s ) );

    auto ss = debug_cast<stormstrike_state_t*>( s );
    // On 11.0.5, the stormblast state of the original strike that triggered the stormflurry is
    // carried over. On live, stormflurries never "benefited" from Stormbringer in terms of being
    // able to proc a Stormblast.
    ss->stormblast = stormblast;

    pre_execute_state = s;

    execute_on_target( t );
  }

  action_state_t* new_state() override
  { return new stormstrike_state_t( this, target ); }

  void snapshot_internal( action_state_t* s, unsigned flags, result_amount_type rt ) override
  {
    shaman_attack_t::snapshot_internal( s, flags, rt );

    auto state = debug_cast<stormstrike_state_t*>( s );
    state->stormblast = p()->talent.stormblast.ok() && p()->buff.stormblast->check() != 0;
  }

  void init() override
  {
    shaman_attack_t::init();

    p()->set_mw_proc_state( this, mw_proc_state::DISABLED );

    if ( p()->talent.stormflurry.ok() )
    {
      proc_stormflurry = p()->tracker.register_proc( p()->talent.stormflurry, this );
    }
  }

  void execute() override
  {
    shaman_attack_t::execute();

    if ( strike_type == strike_variant::NORMAL )
    {
      p()->buff.stormsurge->consume( this, 1 );
      p()->buff.stormblast->consume( this, 1 );
    }

    if ( result_is_hit( execute_state->result ) )
    {
      auto ss = debug_cast<const stormstrike_state_t*>( execute_state );
      mh->stormblast_trigger = ss->stormblast;
      mh->execute_on_target( execute_state->target );
      if ( oh )
      {
        oh->stormblast_trigger = ss->stormblast;
        oh->execute_on_target( execute_state->target );
      }

      p()->trigger_crash_lightning_proc( execute_state, strike_type );
    }

    p()->trigger_stormflurry( execute_state );

    p()->buff.converging_storms->expire();

    if ( strike_type == strike_variant::NORMAL )
    {
      p()->trigger_elemental_assault( execute_state );
    }

    p()->trigger_awakening_storms( execute_state );

    if ( p()->buff.lightning_strikes->up() )
    {
      p()->generate_maelstrom_weapon( this, 1 );
      p()->buff.lightning_strikes->decrement();
    }
  }
};

struct stormstrike_t : public stormstrike_base_t
{
  stormstrike_t( shaman_t* player, util::string_view options_str, strike_variant sf = strike_variant::NORMAL )
    : stormstrike_base_t( player, "stormstrike", player->spec.stormstrike, options_str, sf )
  {
    // Actual damaging attacks are done by stormstrike_attack_t
    mh = new stormstrike_attack_t( "stormstrike_mh", player, data().effectN( 1 ).trigger(),
                                   &( player->main_hand_weapon ), strike_type );
    add_child( mh );

    if ( p()->off_hand_weapon.type != WEAPON_NONE )
    {
      oh = new stormstrike_attack_t( "stormstrike_offhand", player, data().effectN( 2 ).trigger(),
                                     &( player->off_hand_weapon ), strike_type );
      add_child( oh );
    }
  }

  void impact( action_state_t* state ) override
  {
    stormstrike_base_t::impact( state );

    if ( strike_type == strike_variant::NORMAL && p()->buff.doom_winds->up() )
    {
      p()->trigger_thorims_invocation( state );
    }

    p()->trigger_ride_the_lightning( state, p()->action.chain_lightning_ss_rtl );
  }

  bool ready() override
  {
    if ( p()->buff.ascendance->check() )
    {
      return false;
    }

    return stormstrike_base_t::ready();
  }
};

// Windstrike Attack ========================================================

struct windstrike_t : public stormstrike_base_t
{
  windstrike_t( shaman_t* player, util::string_view options_str, strike_variant sf = strike_variant::NORMAL )
    : stormstrike_base_t( player, "windstrike", player->find_spell( 115356 ), options_str, sf )
  {
    // Actual damaging attacks are done by stormstrike_attack_t
    mh = new windstrike_attack_t( "windstrike_mh", player, data().effectN( 1 ).trigger(),
                                  &( player->main_hand_weapon ), strike_type );
    add_child( mh );

    if ( p()->off_hand_weapon.type != WEAPON_NONE )
    {
      oh = new windstrike_attack_t( "windstrike_offhand", player, data().effectN( 2 ).trigger(),
                                    &( player->off_hand_weapon ), strike_type );
      add_child( oh );
    }
  }

  void impact( action_state_t* state ) override
  {
    stormstrike_base_t::impact( state );

    if ( strike_type == strike_variant::NORMAL )
    {
      p()->trigger_thorims_invocation( state );
    }

    p()->trigger_ride_the_lightning( state, p()->action.chain_lightning_ws_rtl );
  }

  bool ready() override
  {
    if ( p()->buff.ascendance->remains() <= cooldown->queue_delay() )
    {
      return false;
    }

    return stormstrike_base_t::ready();
  }
};

// Sundering Spell =========================================================

struct sundering_t : public shaman_attack_t
{
  sundering_t( shaman_t* player, util::string_view options_str )
    : shaman_attack_t( "sundering", player, player->talent.sundering )
  {
    weapon = &( player->main_hand_weapon );

    parse_options( options_str );
    aoe    = -1;  // TODO: This is likely not going to affect all enemies but it will do for now
  }

  void execute() override
  {
    shaman_attack_t::execute();

    p()->trigger_earthsurge( execute_state );

    if ( p()->buff.surging_elements->trigger() )
    {
      p()->generate_maelstrom_weapon( this,
        as<int>( p()->talent.surging_elements->effectN( 3 ).base_value() ) );
    }

    p()->buff.primordial_storm->trigger();

    if ( p()->talent.feral_spirit.ok() )
    {
      p()->summon_feral_spirit( wolf_type_e::FIRE_WOLF,
        as<int>( p()->talent.feral_spirit->effectN( 1 ).base_value() ),
        p()->spell.flowing_spirits_feral_spirit->duration() );
    }

    if ( p()->buff.whirling_earth->consume( this ) )
    {
      p()->pet.searing_totem.spawn( timespan_t::from_seconds( 8.0 + rng().range( 0.85 ) ) );
      p()->trigger_tww3_totemic_enh_2pc( execute_state );
    }
  }

  void impact( action_state_t* s ) override
  {
    shaman_attack_t::impact( s );

    td( s->target )->debuff.lashing_flames->trigger();
  }

  bool ready() override
  {
    if ( p()->buff.primordial_storm->check() )
    {
      return false;
    }

    return shaman_attack_t::ready();
  }
};

// Weapon imbues

struct weapon_imbue_t : public shaman_spell_t
{
  std::string slot_str;
  slot_e slot, default_slot;
  imbue_e imbue;
  buff_t* imbue_buff;

  weapon_imbue_t( util::string_view name, shaman_t* player, slot_e d_, const spell_data_t* spell, util::string_view options_str ) :
    shaman_spell_t( name, player, spell ), slot( SLOT_INVALID ), default_slot( d_ ), imbue( IMBUE_NONE ),
    imbue_buff( nullptr )
  {
    harmful = callbacks = false;
    target = player;

    add_option( opt_string( "slot", slot_str ) );

    parse_options( options_str );

    if ( slot_str.empty() )
    {
      slot = default_slot;
    }
    else
    {
      slot = util::parse_slot_type( slot_str );
    }
  }

  void init_finished() override
  {
    shaman_spell_t::init_finished();

    if ( player->items[ slot ].active() &&
         player->items[ slot ].selected_temporary_enchant() > 0 &&
         ( !if_expr || if_expr->evaluate() != 0.0 ) )
    {
      sim->error( "Player {} has a temporary enchant {} on slot {}, disabling {}",
        player->name(),
        player->items[ slot ].selected_temporary_enchant(),
        util::slot_type_string( slot ),
        name()
      );
    }
  }

  void execute() override
  {
    shaman_spell_t::execute();

    if ( slot == SLOT_MAIN_HAND && player->main_hand_weapon.type != WEAPON_NONE )
    {
      player->main_hand_weapon.buff_type = imbue;
    }
    else if ( slot == SLOT_OFF_HAND && player->off_hand_weapon.type != WEAPON_NONE )
    {
      player->off_hand_weapon.buff_type = imbue;
    }

    if ( imbue_buff != nullptr )
    {
      imbue_buff->trigger();
    }
  }

  bool ready() override
  {
    if ( slot == SLOT_INVALID )
    {
      return false;
    }

    if ( player->items[ slot ].active() &&
         player->items[ slot ].selected_temporary_enchant() > 0 )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

// Windfury Imbue =========================================================

struct windfury_weapon_t : public weapon_imbue_t
{
  windfury_weapon_t( shaman_t* player, util::string_view options_str ) :
    weapon_imbue_t( "windfury_weapon", player, SLOT_MAIN_HAND, player->talent.windfury_weapon,
                    options_str )
  {
    imbue = WINDFURY_IMBUE;
    imbue_buff = player->buff.windfury_weapon;

    if ( slot == SLOT_MAIN_HAND )
    {
      add_child( player->windfury_mh );
    }
    // Technically, you can put Windfury on the off-hand slot but it disables the proc
    else if ( slot == SLOT_OFF_HAND )
    {
      ;
    }
    else
    {
      sim->error( "{} invalid windfury slot '{}'", player->name(), slot_str );
    }
  }
};

// Flametongue Imbue =========================================================

struct flametongue_weapon_t : public weapon_imbue_t
{
  flametongue_weapon_t( shaman_t* player, util::string_view options_str ) :
    weapon_imbue_t( "flametongue_weapon", player,
                    player->specialization() == SHAMAN_ENHANCEMENT
                    ? SLOT_OFF_HAND
                    : SLOT_MAIN_HAND,
                    player->talent.flametongue_weapon, options_str )
  {
    imbue = FLAMETONGUE_IMBUE;
    imbue_buff = player->buff.flametongue_weapon;

    if ( slot == SLOT_MAIN_HAND || slot == SLOT_OFF_HAND )
    {
      add_child( player->flametongue );
    }
    else
    {
      sim->error( "{} invalid flametongue slot '{}'", player->name(), slot_str );
    }
  }
};

// Thunderstrike Ward Imbue ================================================

struct thunderstrike_ward_t : public weapon_imbue_t
{
  thunderstrike_ward_t( shaman_t* player, util::string_view options_str ) :
    weapon_imbue_t( "thunderstrike_ward", player, SLOT_OFF_HAND,
                    player->talent.thunderstrike_ward, options_str )
  {
    if ( !player->has_shield_equipped() && player->talent.thunderstrike_ward.enabled() )
    {
      sim->errorf( "%s: %s only usable with shield equipped in offhand\n", player->name(), name() );
    }
    else
    {
      imbue      = THUNDERSTRIKE_WARD;
      imbue_buff = player->buff.thunderstrike_ward;
    }
  }
};

// Crash Lightning Attack ===================================================

struct crash_lightning_t : public shaman_attack_t
{
  crash_lightning_t( shaman_t* player, util::string_view options_str )
    : shaman_attack_t( "crash_lightning", player, player->talent.crash_lightning )
  {
    parse_options( options_str );

    aoe     = -1;
    reduced_aoe_targets = p()->talent.crash_lightning->effectN( 2 ).base_value();

    weapon  = &( p()->main_hand_weapon );
    ap_type = attack_power_type::WEAPON_BOTH;

    player->crash_lightning.emplace_back( this );
  }

  void init() override
  {
    shaman_attack_t::init();

    add_child( p()->action.crash_lightning_aoe );
    add_child( p()->action.crash_lightning_unleashed );
  }

  std::unique_ptr<expr_t> create_expression( util::string_view expression_str ) override
  {
    struct cl_cd_t : public expr_t
    {
      std::unique_ptr<expr_t> normal, su;
      shaman_t* actor;

      cl_cd_t( shaman_t* player, std::string_view expr_str ) : expr_t( expr_str ),
        normal( player->cooldown.crash_lightning->create_expression( expr_str ) ),
        su( player->cooldown.crash_lightning_su->create_expression( expr_str ) ),
        actor( player )
      { }

      double evaluate() override
      {
        if ( actor->buff.storm_unleashed->check() )
        {
          return su->evaluate();
        }
        else
        {
          return normal->evaluate();
        }
      }
    };

    std::vector<util::string_view> cd_expr_str {
      "cooldown", "charges", "charges_fractional", "max_charges", "recharge_time",
      "full_recharge_time"
    };
    auto split = util::string_split( expression_str, "." );

    if ( range::find( cd_expr_str, split.front() ) != cd_expr_str.end() )
    {
      if ( split.size() == 1U )
      {
        return std::make_unique<cl_cd_t>( p(), split.front() );
      }
      else if ( split.size() == 3U && util::str_compare_ci( split.front(), "cooldown" ) &&
        util::str_compare_ci( split[ 1 ], "crash_lightning" ) )
      {
        return std::make_unique<cl_cd_t>( p(), split[ 2 ] );
      }
    }

    return shaman_attack_t::create_expression( expression_str );
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = shaman_attack_t::composite_da_multiplier( state );

    if ( state->chain_target == 0 &&
         p()->sets->has_set_bonus( SHAMAN_ENHANCEMENT, TWW2, B4 ) )
    {
      m *= 1.0 + p()->sets->set( SHAMAN_ENHANCEMENT, TWW2, B4 )->effectN( 2 ).percent();
    }

    return m;
  }

  void execute() override
  {
    shaman_attack_t::execute();

    if ( result_is_hit( execute_state->result ) )
    {
      p()->buff.crash_lightning->trigger();

      if ( p()->talent.converging_storms->ok() )
      {
        p()->buff.converging_storms->trigger( num_targets_hit );
      }
    }

    p()->buff.tww2_enh_4pc->decrement( p()->buff.tww2_enh_4pc_damage->check() );
    p()->buff.tww2_enh_4pc_damage->expire();
    if ( p()->buff.tww2_enh_4pc->check() )
    {
      p()->buff.tww2_enh_4pc_damage->trigger( p()->buff.tww2_enh_4pc->check() );
    }

    if ( p()->buff.doom_winds->up() )
    {
      p()->trigger_thorims_invocation( execute_state );
    }

    p()->buff.storm_unleashed->consume( this, 1 );

    if ( p()->talent.storm_unleashed_3.ok() )
    {
      make_repeating_event( sim, 1_s, [ this ]() {
        for ( auto t : target_list() )
        {
          if ( !rng().roll( p()->options.crash_lightning_su_hit_chance ) )
          {
            continue;
          }

          p()->action.crash_lightning_unleashed->execute_on_target( t );
        }
      }, as<int>( p()->talent.storm_unleashed_3->effectN( 2 ).base_value() ) );
    }
  }

  void impact( action_state_t* state ) override
  {
    shaman_attack_t::impact( state );

    if ( state->chain_target == 0 && p()->talent.stormwell.ok() )
    {
      p()->trigger_windfury_weapon( execute_state, 1.0 );
    }
  }
};

// Earth Elemental ===========================================================

struct earth_elemental_t : public shaman_spell_t
{
  earth_elemental_t( shaman_t* player, util::string_view options_str )
    : shaman_spell_t( "earth_elemental", player, player->talent.earth_elemental )
  {
    parse_options( options_str );

    harmful = may_crit = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->summon_elemental( elemental::GREATER_EARTH );
  }
};

// Fire Elemental ===========================================================

struct fire_elemental_t : public shaman_spell_t
{
  fire_elemental_t( shaman_t* player, util::string_view options_str )
    : shaman_spell_t( "fire_elemental", player, player->spell.fire_elemental )
  {
    parse_options( options_str );
    harmful  = true;
    may_crit = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->summon_elemental( elemental::GREATER_FIRE );
  }

  bool ready() override
  {
    if ( p()->talent.ascendance->ok() )
    {
      return false;
    }
    return shaman_spell_t::ready();
  }
};

// Lightning Shield Spell ===================================================

struct lightning_shield_t : public shaman_spell_t
{
  lightning_shield_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "lightning_shield", player, player->find_class_spell( "Lightning Shield" ) )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->buff.lightning_shield->trigger();
  }
};

// Earth Shield Spell =======================================================

// Barebones implementation to consume Vesper Totem charges for damage specs
struct earth_shield_t : public shaman_heal_t
{
  earth_shield_t( shaman_t* player, util::string_view options_str ) :
    shaman_heal_t( "earth_shield", player, player->talent.earth_shield )
  {
    parse_options( options_str );
  }

  // Needed to work around a combined Specialization and Talent spell
  bool verify_actor_spec() const override
  { return player->specialization() == SHAMAN_RESTORATION || data().ok(); }

  void execute() override
  {
    shaman_heal_t::execute();

    p()->buff.lightning_shield->expire();
  }
};

// ==========================================================================
// Shaman Spells
// ==========================================================================

// Bloodlust Spell ==========================================================

struct bloodlust_t : public shaman_spell_t
{
  bloodlust_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "bloodlust", player, player->find_class_spell( "Bloodlust" ) )
  {
    parse_options( options_str );
    harmful = false;
    track_cd_waste = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    // use indices since it's possible to spawn new actors when bloodlust is triggered
    for ( size_t i = 0; i < sim->player_non_sleeping_list.size(); i++ )
    {
      auto* p = sim->player_non_sleeping_list[ i ];
      if ( p->is_pet() || p->buffs.exhaustion->check() )
        continue;

      p->buffs.bloodlust->trigger();
      p->buffs.exhaustion->trigger();
    }
  }

  bool ready() override
  {
    // If the global bloodlust override doesn't allow bloodlust, disable bloodlust
    if ( !sim->overrides.bloodlust )
      return false;

    return shaman_spell_t::ready();
  }
};

// Chain Lightning and Lava Beam Spells =========================================

struct chained_overload_base_t : public elemental_overload_spell_t
{
  chained_overload_base_t( shaman_t* p, util::string_view name, unsigned t,
                           const spell_data_t* spell, double mg, shaman_spell_t* parent_ )
    : elemental_overload_spell_t( p, ::action_name( name, t ), spell, parent_, -1.0, t )
  {
    if ( p->specialization() == SHAMAN_ELEMENTAL )
    {
      maelstrom_gain = mg;
      energize_type  = action_energize::NONE;  // disable resource generation from spell data.
    }
    radius = 10.0;
  }

  std::vector<player_t*>& check_distance_targeting( std::vector<player_t*>& tl ) const override
  {
    return __check_distance_targeting( this, tl );
  }
};

struct chain_lightning_overload_t : public chained_overload_base_t
{
  chain_lightning_overload_t( shaman_t* p, unsigned t, shaman_spell_t* parent_ ) :
    chained_overload_base_t( p, "chain_lightning_overload", t, p->find_spell( 45297 ),
        p->spec.maelstrom->effectN( 6 ).resource( RESOURCE_MAELSTROM ), parent_ )
  {
    affected_by_master_of_the_elements  = true;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = chained_overload_base_t::composite_target_multiplier( t );
    if ( p()->talent.inferno_arc.ok() )
    {
      m *= 1.0 + td( t )->dot.flame_shock->is_ticking() * p()->talent.inferno_arc->effectN( 1 ).percent();
    }

    return m;
  }

  void impact( action_state_t* state ) override
  {
    chained_overload_base_t::impact( state );

    // Accumulate Lightning Rod damage from all targets hit by this cast.
    if ( p()->talent.lightning_rod.ok() || p()->talent.conductive_energy.ok() )
    {
      accumulate_lightning_rod_damage( state );
    }
  }
};

struct chained_base_t : public shaman_spell_t
{
  mutable bool targets_randomized;

  chained_base_t( shaman_t* player, util::string_view name, unsigned t,
                  const spell_data_t* spell, double mg, util::string_view options_str )
    : shaman_spell_t( ::action_name( name, t ), player, spell, t ), targets_randomized( false )
  {
    parse_options( options_str );

    radius = 10.0;
    ancestor_trigger = ancestor_cast::CHAIN_LIGHTNING;

    if ( player->specialization() == SHAMAN_ELEMENTAL )
    {
      maelstrom_gain = mg;
      energize_type  = action_energize::NONE;  // disable resource generation from spell data.
    }
  }

  void reset() override
  {
    shaman_spell_t::reset();

    targets_randomized = false;
  }

  double overload_chance( const action_state_t* s ) const override
  {
    double base_chance = shaman_spell_t::overload_chance( s );

    return base_chance / 3.0;
  }

  // Add some randomization to chained spells when target count > 0 and target randomization is
  // enabled. Never shuffles the primary target. Always happens, regardless of whether the target
  // cache is fresh or not.
  void shuffle_targets()
  {
    auto& tl = shaman_spell_t::target_list();
    if ( tl.size() <= as<unsigned>( n_targets() ) ||
      p()->options.chain_lightning_target_rng == 0.0 )
    {
      return;
    }

    std::vector<player_t*> shuffled_targets;

    for ( size_t i = 1U; i < tl.size(); ++i )
    {
      // Don't shuffle already shuffled targets
      if ( range::find( shuffled_targets, tl[ i ] ) != shuffled_targets.end() )
      {
        continue;
      }

      if ( rng().roll( p()->options.chain_lightning_target_rng ) )
      {
        auto shuffled_target = tl[ i ];
        auto new_idx = i;
        do
        {
          new_idx = rng().range<size_t>( 1U, tl.size() );
        } while ( new_idx == i );

        sim->print_debug( "{} randomized {} target, target={} (idx={}), new_pos={}",
          player->name(), name(), shuffled_target->name(), i, new_idx );
        tl.erase( tl.begin() + i );
        if ( new_idx >= tl.size() )
        {
          tl.emplace_back( shuffled_target );
        }
        else
        {
          tl.insert( tl.begin() + new_idx, shuffled_target );
        }

        shuffled_targets.emplace_back( shuffled_target );
      }
    }

    targets_randomized = !shuffled_targets.empty();
  }

  void execute() override
  {
    shuffle_targets();

    shaman_spell_t::execute();

    if ( targets_randomized )
    {
      target_cache.is_valid = false;
      targets_randomized = false;
    }

    if ( is_variant( spell_variant::NORMAL ) )
    {
      if ( !p()->sk_during_cast )
      {
        p()->buff.stormkeeper->decrement();
      }
      p()->sk_during_cast = false;
    }
  }

  std::vector<player_t*>& check_distance_targeting( std::vector<player_t*>& tl ) const override
  {
    return __check_distance_targeting( this, tl );
  }
};

struct chain_lightning_t : public chained_base_t
{
  strike_variant sv;

  chain_lightning_t( shaman_t* player, util::string_view options_str ) :
    chain_lightning_t( player, "chain_lightning", player->talent.chain_lightning,
      variant_flag( spell_variant::NORMAL ), options_str )
  { }

  chain_lightning_t( shaman_t* player, const spell_data_t* spell, unsigned variant_flags ) :
    chain_lightning_t( player, "chain_lightning", spell, variant_flags, "" )
  { }

  chain_lightning_t( shaman_t* player, const spell_data_t* spell, unsigned variant_flags,
      util::string_view name_str ) :
    chain_lightning_t( player, name_str, spell, variant_flags, "" )
  { }

  chain_lightning_t( shaman_t* player, util::string_view name_str, const spell_data_t* spell,
    unsigned t, util::string_view options_str )
    : chained_base_t( player, name_str, t, spell,
        player->spec.maelstrom->effectN( 5 ).resource( RESOURCE_MAELSTROM ), options_str ),
        sv( strike_variant::NORMAL )
  {
    if ( player->mastery.elemental_overload->ok() )
    {
      overload = new chain_lightning_overload_t( player, t, this );
    }

    affected_by_master_of_the_elements = true;

    if ( is_variant( spell_variant::THORIMS_INVOCATION ) )
    {
      background = true;
      base_execute_time = 0_s;
      base_costs[ RESOURCE_MANA ] = 0;
      if ( auto parent = p()->find_action( "thorims_invocation" ) )
      {
        parent->add_child( this );
      }
    }

    if ( is_variant( spell_variant::RIDE_THE_LIGHTNING ) )
    {
      background = true;
      base_execute_time = 0_s;
      base_costs[ RESOURCE_MANA ] = 0;
      // Arc Discharge + Ride The Lightning CL gets grouped under AD in reports
      if ( !is_variant( spell_variant::ARC_DISCHARGE ) )
      {
        if ( auto parent = player->find_action( "ride_the_lightning" ) )
        {
          parent->add_child( this );
        }
      }
    }

    if ( is_variant( spell_variant::ARC_DISCHARGE ) )
    {
      background = true;
      base_execute_time = 0_s;
      base_costs[ RESOURCE_MANA ] = 0;
      if ( auto parent = p()->find_action( "arc_discharge" ) )
      {
        parent->add_child( this );
      }
    }

    if ( is_variant( spell_variant::PRIMORDIAL_STORM ) )
    {
      background = true;
      base_execute_time = 0_s;
      base_costs[ RESOURCE_MANA ] = 0;
      if ( auto ps_action = p()->find_action( "primordial_storm" ) )
      {
        ps_action->add_child( this );
      }
    }
  }

  void trigger( strike_variant sv_, player_t* target )
  {
    sv = sv_;

    execute_on_target( target );
  }

  void proc_lightning_rod()
  {
    if ( p()->specialization() != SHAMAN_ENHANCEMENT || !p()->talent.conductive_energy.ok() )
    {
      return;
    }

    auto t = target_list()[ 0 ];
    for ( size_t i = 0; i < std::min( as<size_t>( n_targets() ), target_list().size() ); ++i )
    {
      if ( !td( target_list()[ i ] )->debuff.lightning_rod->check() )
      {
        t = target_list()[ i ];
        break;
      }
    }

    trigger_lightning_rod_debuff( t );
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    tl.clear();

    if ( !target->is_sleeping() )
    {
      tl.push_back( target );
    }

    // The rest
    range::for_each( sim->target_non_sleeping_list, [&tl]( player_t* t ) {
      if ( t->is_enemy() && !range::contains( tl, t ) )
      {
        tl.emplace_back( t );
      }
    } );

    return tl.size();
  }

  bool benefit_from_maelstrom_weapon() const override
  {
    if ( p()->buff.stormkeeper->check() )
    {
      return false;
    }

    if ( is_variant( spell_variant::RIDE_THE_LIGHTNING ) )
    {
      return false;
    }

    return shaman_spell_t::benefit_from_maelstrom_weapon();
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    if ( is_variant( spell_variant::PRIMORDIAL_STORM ) )
    {
      m *= p()->talent.primordial_storm->effectN( 2 ).percent();
    }

    if ( sv == strike_variant::STORMFLURRY )
    {
      m *= p()->talent.stormflurry->effectN( 2 ).percent();
    }

    return m;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = shaman_spell_t::composite_target_multiplier( t );
    if ( p()->talent.inferno_arc.ok() )
    {
      m *= 1.0 + td( t )->dot.flame_shock->is_ticking() * p()->talent.inferno_arc->effectN( 1 ).percent();
    }

    return m;
  }

  // If Stormkeeper is up, Chain Lightning will not consume Maelstrom Weapon stacks, but
  // will allow Chain Lightning to fully benefit from the stacks.
  bool consume_maelstrom_weapon() const override
  {
    if ( p()->buff.stormkeeper->check() )
    {
      return false;
    }

    if ( is_variant( spell_variant::ARC_DISCHARGE ) )
    {
      return false;
    }

    return shaman_spell_t::consume_maelstrom_weapon();
  }

  void execute() override
  {
    chained_base_t::execute();

    if ( is_variant( spell_variant::NORMAL ) && p()->specialization() == SHAMAN_ELEMENTAL )
    {
      if ( p()->talent.routine_communication.ok() && p()->rng_obj.routine_communication->trigger() )
      {
        p()->summon_ancestor();
      }
    }

    if ( p()->buff.storm_elemental->check() && p()->talent.primal_elementalist.ok() )
    {
      p()->buff.wind_gust->trigger();
    }

    // Track last cast for LB / CL because of Thorim's Invocation
    if ( p()->talent.thorims_invocation.ok() && ( is_variant( spell_variant::NORMAL ) || is_variant( spell_variant::PRIMORDIAL_STORM ) ) )
    {
      p()->action.ti_trigger = p()->action.chain_lightning_ti;
    }

    if ( ( is_variant( spell_variant::NORMAL ) || is_variant( spell_variant::THORIMS_INVOCATION ) ) &&
         p()->specialization() == SHAMAN_ENHANCEMENT &&
         rng().roll( p()->talent.supercharge->effectN( 2 ).percent() ) )
    {
      p()->generate_maelstrom_weapon( execute_state->action,
                                      as<int>( p()->talent.supercharge->effectN( 3 ).base_value() ) );
    }

    // TODO-midnight-talent: Uniform RNG, or what?
    if ( ( is_variant( spell_variant::NORMAL ) || is_variant( spell_variant::THORIMS_INVOCATION ) ) &&
         rng().roll( p()->talent.thunder_capacitor->effectN( 2 ).percent() ) )
    {
      sim->print_debug( "{} procs thunder_capacitor", player->name() );
      p()->generate_maelstrom_weapon( execute_state->action, mw_consumed_stacks );
    }

    p()->trigger_thunderstrike_ward( execute_state );

    proc_lightning_rod();
  }

  void impact( action_state_t* state ) override
  {
    chained_base_t::impact( state );

    // Accumulate Lightning Rod damage from all targets hit by this cast.
    if ( p()->talent.lightning_rod.ok() || p()->talent.conductive_energy.ok() )
    {
      accumulate_lightning_rod_damage( state );
    }

    // Chain Lightning Arc Discharge actually targets the last target hit, not the first one.
    if ( as<unsigned>( state->chain_target ) == state->n_targets - 1 )
    {
      p()->trigger_arc_discharge( state );
    }

    if ( state->chain_target == 0 )
    {
      p()->trigger_totemic_rebound( state );
    }

    if ( ( is_variant( spell_variant::NORMAL ) || is_variant( spell_variant::THORIMS_INVOCATION ) )
      && state->chain_target == 0 )
    {
      p()->trigger_whirling_air( state );
    }
  }

  void schedule_travel(action_state_t* s) override
  {
    if ( s->chain_target == 0 && p()->buff.power_of_the_maelstrom->up() )
    {
      trigger_elemental_overload( s, 1.0 );
      p()->buff.power_of_the_maelstrom->decrement();
      if ( p()->talent.fusion_of_elements->ok() )
      {
        p()->action.elemental_blast_foe->execute_on_target( s->target );
      }
    }

    if ( s->chain_target == 0 && p()->talent.supercharge.ok() )
    {
      trigger_elemental_overload( s, p()->talent.supercharge->effectN( 1 ).percent() );

      if ( p()->buff.ascendance->up() )
      {
        trigger_elemental_overload( s, 1.0 );
      }
    }

    chained_base_t::schedule_travel( s );
  }
};

struct storms_eye_t : public shaman_spell_t
{
  storms_eye_t( shaman_t* player ) : shaman_spell_t( "storms_eye", player, player->find_spell( 1235840 ) )
  {
    background = true;
    may_crit   = true;
    dual       = true;
    aoe = -1;
    reduced_aoe_targets = 1.0;
    full_amount_targets = 1;
    if ( p()->specialization() == SHAMAN_ELEMENTAL )
    {
      spell_power_mod.direct = data().effectN( 2 ).sp_coeff();
      attack_power_mod.direct = 0;
    }
    else
    {
      spell_power_mod.direct  = 0;
      attack_power_mod.direct = data().effectN( 1 ).ap_coeff();
    }
  }
};



// Lava Burst Spell =========================================================

struct lava_burst_state_t : public shaman_action_state_t
{
  lava_burst_state_t( action_t* action_, player_t* target_ ) :
    shaman_action_state_t( action_, target_ )
  { }
};

// As of 8.1 Lava Burst checks its state on impact. Lava Burst -> Flame Shock now forces the critical strike
struct lava_burst_overload_t : public elemental_overload_spell_t
{
  unsigned impact_flags;

  lava_burst_overload_t( shaman_t* player, unsigned type, shaman_spell_t* parent_ )
    : elemental_overload_spell_t( player, ::action_name( "lava_burst_overload", type ),
        player->find_spell( 285466 ), parent_, -1.0, type ),
      impact_flags()
  {
    spell_power_mod.direct = data().effectN( 1 ).sp_coeff();
    travel_speed = player->find_spell( 77451 )->missile_speed();
  }

  static lava_burst_state_t* cast_state( action_state_t* s )
  { return debug_cast<lava_burst_state_t*>( s ); }

  static const lava_burst_state_t* cast_state( const action_state_t* s )
  { return debug_cast<const lava_burst_state_t*>( s ); }

  action_state_t* new_state() override
  { return new lava_burst_state_t( this, target ); }

  void snapshot_impact_state( action_state_t* s, result_amount_type rt )
  {
    auto var_ = cast_state( s )->variant_flags();

    snapshot_internal( s, impact_flags, rt );

    cast_state( s )->variant_flags( var_ );
  }

  double calculate_direct_amount( action_state_t* /* s */ ) const override
  {
    // Don't do any extra work, this result won't be used.
    return 0.0;
  }

  result_e calculate_result( action_state_t* /* s */ ) const override
  {
    // Don't do any extra work, this result won't be used.
    return RESULT_NONE;
  }

  void impact( action_state_t* s ) override
  {
    // Re-call functions here, before the impact call to do the damage calculations as we impact.
    snapshot_impact_state( s, amount_type( s ) );

    s->result        = elemental_overload_spell_t::calculate_result( s );
    s->result_amount = elemental_overload_spell_t::calculate_direct_amount( s );

    elemental_overload_spell_t::impact( s );
  }

  double action_multiplier() const override
  {
    double m = elemental_overload_spell_t::action_multiplier();

    if ( is_variant( spell_variant::ASCENDANCE ) )
    {
      m *= p()->spell.ascendance->effectN( 10 ).percent();
    }

    if ( is_variant( spell_variant::PURGING_FLAMES ) && p()->bugs )
    {
      m *= p()->buff.purging_flames->data().effectN( 1 ).percent();
    }

    if ( player->specialization() == SHAMAN_ELEMENTAL )
    {
      m *= 1.0 + this->composite_crit_chance();
    }


    return m;
  }

  double composite_target_crit_chance( player_t* t ) const override
  {
    double m = shaman_spell_t::composite_target_crit_chance( t );

    // TODO Elemental: confirm is this effect needs to be hardcoded
    /* if ( p()->spec.lava_burst_2->ok() && td( target )->dot.flame_shock->is_ticking() ) */
    if ( td( target )->dot.flame_shock->is_ticking() )
    {
      // hardcoded because I didn't find it in spell data
      m = 1.0;
    }

    return m;
  }

};

struct flame_shock_spreader_t : public shaman_spell_t
{
  flame_shock_spreader_t( shaman_t* p ) : shaman_spell_t( "flame_shock_spreader", p )
  {
    quiet = background = true;
    may_miss = may_crit = callbacks = false;
  }

  player_t* shortest_duration_target() const
  {
    player_t* copy_target  = nullptr;
    timespan_t min_remains = timespan_t::zero();

    for ( auto t : sim->target_non_sleeping_list )
    {
      // Skip source target
      if ( t == target )
      {
        continue;
      }

      // Skip targets that are further than 8 yards from the original target
      if ( sim->distance_targeting_enabled && t->get_player_distance( *target ) > 8 + t->combat_reach )
      {
        continue;
      }

      shaman_td_t* target_td = td( t );
      if ( min_remains == timespan_t::zero() || min_remains > target_td->dot.flame_shock->remains() )
      {
        min_remains = target_td->dot.flame_shock->remains();
        copy_target = t;
      }
    }

    if ( copy_target && sim->debug )
    {
      sim->out_debug.printf(
          "%s spreads flame_shock from %s to shortest remaining target %s (remains=%.3f)",
          player->name(), target->name(), copy_target->name(), min_remains.total_seconds() );
    }

    return copy_target;
  }

  player_t* closest_target() const
  {
    player_t* copy_target = nullptr;
    double min_distance   = -1;

    for ( auto t : sim->target_non_sleeping_list )
    {
      // Skip source target
      if ( t == target )
      {
        continue;
      }

      double distance = 0;
      if ( sim->distance_targeting_enabled )
      {
        distance = t->get_player_distance( *target );
      }

      // Skip targets that are further than 8 yards from the original target
      if ( sim->distance_targeting_enabled && distance > 8 + t->combat_reach )
      {
        continue;
      }

      shaman_td_t* target_td = td( t );
      if ( target_td->dot.flame_shock->is_ticking() )
      {
        continue;
      }

      // If we are not using distance-based targeting, just return the first available target
      if ( !sim->distance_targeting_enabled )
      {
        copy_target = t;
        break;
      }
      else if ( min_distance < 0 || min_distance > distance )
      {
        min_distance = distance;
        copy_target  = t;
      }
    }

    if ( copy_target && sim->debug )
    {
      sim->out_debug.printf( "%s spreads flame_shock from %s to closest target %s (distance=%.3f)",
                             player->name(), target->name(), copy_target->name(), min_distance );
    }

    return copy_target;
  }

  void execute() override
  {
    shaman_td_t* source_td = td( target );
    player_t* copy_target  = nullptr;
    if ( !source_td->dot.flame_shock->is_ticking() )
    {
      return;
    }

    // If all targets have flame shock, pick the shortest remaining time
    if ( player->get_active_dots( source_td->dot.flame_shock ) ==
         sim->target_non_sleeping_list.size() )
    {
      copy_target = shortest_duration_target();
    }
    // Pick closest target without Flame Shock
    else
    {
      copy_target = closest_target();
    }

    // With distance targeting it is possible that no target will be around to spread flame shock to
    if ( copy_target )
    {
      source_td->dot.flame_shock->copy( copy_target, DOT_COPY_CLONE );
    }
  }
};

// Fire Nova Spell ==========================================================

struct fire_nova_explosion_t : public shaman_spell_t
{
  fire_nova_explosion_t( shaman_t* p, unsigned type_ ) :
    shaman_spell_t( "fire_nova_explosion", p, p->find_spell( 333977 ), type_ )
  {
    background = true;

    if ( is_variant( spell_variant::TWW3_SPELL ) )
    {
      base_multiplier *= player->sets->set( HERO_TOTEMIC, TWW3, B4 )->effectN( 2 ).percent();
    }
  }
};

struct fire_nova_t : public shaman_spell_t
{
  fire_nova_t( shaman_t* p, unsigned type_ )
    : shaman_spell_t( "fire_nova", p, p->find_spell( 333974 ), type_ )
  {
    may_crit = may_miss = callbacks = false;
    background                      = true;
    aoe                             = -1;

    impact_action = new fire_nova_explosion_t( p, type_ );

    p->flame_shock_dependants.push_back( this );

    add_child( impact_action );
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    shaman_spell_t::available_targets( tl );

    p()->regenerate_flame_shock_dependent_target_list( this );

    return tl.size();
  }

  void execute() override
  {
    shaman_spell_t::execute();

    if ( p()->sets->has_set_bonus( HERO_TOTEMIC, TWW3, B4 ) )
    {
      p()->trigger_elemental_overflow( execute_state, p()->action.tww3_fire_nova );
    }
  }
};

/**
 * As of 8.1 Lava Burst checks its state on impact. Lava Burst -> Flame Shock now forces the critical strike
 */
struct lava_burst_t : public shaman_spell_t
{
  unsigned impact_flags;

  lava_burst_t( shaman_t* player, unsigned type_, util::string_view options_str = {} )
    : shaman_spell_t( ::action_name( "lava_burst", type_ ), player, player->talent.lava_burst, type_ ),
      impact_flags()
  {
    parse_options( options_str );
    // Manacost is only for resto
    if ( p()->specialization() == SHAMAN_ELEMENTAL )
    {
      base_costs[ RESOURCE_MANA ] = 0;
    }

    if ( player->mastery.elemental_overload->ok() )
    {
      overload = new lava_burst_overload_t( player, type_, this );
    }

    spell_power_mod.direct = player->find_spell( 285452 )->effectN( 1 ).sp_coeff();
    ancestor_trigger = ancestor_cast::LAVA_BURST;

    if ( !is_variant( spell_variant::NORMAL ) )
    {
      background = true;
      base_execute_time = 0_s;
      cooldown->duration = 0_s;
      ancestor_trigger = ancestor_cast::DISABLED;

      if ( is_variant( spell_variant::ASCENDANCE ) )
      {
        aoe = 6;
        auto asc_action = p()->find_action( "ascendance" );
        if ( p()->talent.ascendance->ok() && asc_action )
        {
          asc_action->add_child( this );
        }
      }

      if ( is_variant( spell_variant::PURGING_FLAMES ) )
      {
        // If anyone knows which spelldata to use here, that would be great.
        // Currently there exists a stackable ingame bug to apply this ms gain
        // to the base lvb cast AND ALSO with additional stacks decrease ms
        // gain further to 1 and 0. Which makes me believe this would need to
        // be an ms gain multiplier instead of a fixed value. But this is now
        // still better than not lowering ms gain.
        maelstrom_gain = 2;
        if ( auto vb = p()->find_action( "voltaic_blaze" ) )
        {
          vb->add_child( this );
        }
      }
    }
  }

  static lava_burst_state_t* cast_state( action_state_t* s )
  { return debug_cast<lava_burst_state_t*>( s ); }

  static const lava_burst_state_t* cast_state( const action_state_t* s )
  { return debug_cast<const lava_burst_state_t*>( s ); }

  action_state_t* new_state() override
  { return new lava_burst_state_t( this, target ); }

  void init() override
  {
    shaman_spell_t::init();

    std::swap( snapshot_flags, impact_flags );
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    shaman_spell_t::available_targets( tl );
    p()->regenerate_flame_shock_dependent_target_list( this );

    return tl.size();
  }

  void snapshot_impact_state( action_state_t* s, result_amount_type rt )
  {
    snapshot_internal( s, impact_flags, rt );
  }

  double calculate_direct_amount( action_state_t* /* s */ ) const override
  {
    // Don't do any extra work, this result won't be used.
    return 0.0;
  }

  result_e calculate_result( action_state_t* /* s */ ) const override
  {
    // Don't do any extra work, this result won't be used.
    return RESULT_NONE;
  }

  void impact( action_state_t* s ) override
  {
    // Re-call functions here, before the impact call to do the damage calculations as we impact.
    snapshot_impact_state( s, amount_type( s ) );

    s->result        = shaman_spell_t::calculate_result( s );
    s->result_amount = shaman_spell_t::calculate_direct_amount( s );

    shaman_spell_t::impact( s );
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    if ( is_variant( spell_variant::PURGING_FLAMES ) && p()->bugs )
    {
      m *= p()->buff.purging_flames->data().effectN( 1 ).percent();
    }

    if (player->specialization() == SHAMAN_ELEMENTAL)
    {
      m *= 1.0 + this->composite_crit_chance();
    }

    return m;
  }

  double composite_target_crit_chance( player_t* t ) const override
  {
    double m = shaman_spell_t::composite_target_crit_chance( t );

    // TODO Elemental: confirm is this effect needs to be hardcoded
    /* if ( p()->spec.lava_burst_2->ok() && td( target )->dot.flame_shock->is_ticking() ) */
    if ( td( target )->dot.flame_shock->is_ticking() )
    {
      // hardcoded because I didn't find it in spell data
      m = 1.0;
    }

    return m;
  }

  void update_ready( timespan_t /* cd_duration */ ) override
  {
    timespan_t d = cooldown->duration;

    // Lava Surge has procced during the cast of Lava Burst, the cooldown
    // reset is deferred to the finished cast, instead of "eating" it.

    if ( p()->lava_surge_during_lvb )
    {
      d                      = timespan_t::zero();
      cooldown->last_charged = sim->current_time();
    }

    shaman_spell_t::update_ready( d );
  }

  void execute() override
  {
    bool had_ancestral_swiftness_buff = p()->buff.ancestral_swiftness->check();
    shaman_spell_t::execute();
    bool ancestral_swiftness_consumed = had_ancestral_swiftness_buff && !p()->buff.ancestral_swiftness->check();

    if ( is_variant( spell_variant::NORMAL ) && p()->talent.master_of_the_elements->ok() )
    {
      p()->buff.master_of_the_elements->trigger();
    }

    // Lava Surge buff does not get eaten, if the Lava Surge proc happened
    // during the Lava Burst cast
    if (!ancestral_swiftness_consumed
      && is_variant( spell_variant::NORMAL ) && !p()->lava_surge_during_lvb &&
      p()->buff.lava_surge->check() )
    {
      p()->buff.lava_surge->decrement();
    }

    p()->lava_surge_during_lvb = false;

    if ( is_variant( spell_variant::NORMAL ) &&
      rng().roll( p()->talent.power_of_the_maelstrom->effectN( 1 ).percent() ) )
    {
      p()->buff.power_of_the_maelstrom->trigger();
    }

    if ( p()->talent.routine_communication.ok() && p()->rng_obj.routine_communication->trigger() &&
      is_variant( spell_variant::NORMAL ) )
    {
      p()->summon_ancestor();
    }

    if (p()->buff.purging_flames->check() && !background)
    {
      assert( p()->action.lava_burst_pf );
      for ( auto t : target_list() )
      {
        if (t == target)
        {
          continue;
        }
        p()->action.lava_burst_pf->execute_on_target( t );
      }
      p()->buff.purging_flames->decrement();
    }

    // [BUG] 2024-08-23 Supercharge works on Lava Burst in-game
    if ( p()->bugs && is_variant( spell_variant::NORMAL ) &&
         p()->specialization() == SHAMAN_ENHANCEMENT &&
         rng().roll( p()->talent.supercharge->effectN( 2 ).percent() ) )
    {
      p()->generate_maelstrom_weapon( this, as<int>( p()->talent.supercharge->effectN( 3 ).base_value() ) );
    }
  }

  timespan_t execute_time() const override
  {
    if ( p()->buff.lava_surge->up() )
    {
      return timespan_t::zero();
    }

    return shaman_spell_t::execute_time();
  }

  bool ready() override
  {
    if ( player->specialization() == SHAMAN_ENHANCEMENT &&
         p()->talent.elemental_blast.ok() )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

// Lightning Bolt Spell =====================================================

struct lightning_bolt_overload_t : public elemental_overload_spell_t
{
  lightning_bolt_overload_t( shaman_t* p, shaman_spell_t* parent_ )
    : elemental_overload_spell_t( p, "lightning_bolt_overload", p->find_spell( 45284 ), parent_ )
  {
    maelstrom_gain  = p->spec.maelstrom->effectN( 2 ).resource( RESOURCE_MAELSTROM );

    affected_by_master_of_the_elements = true;
    // Stormkeeper affected by flagging is applied to the Energize spell ...
    affected_by_stormkeeper_damage = p->talent.stormkeeper.ok() && p->specialization() == SHAMAN_ELEMENTAL;
    affected_by_stormkeeper_damage_tier = p->talent.stormkeeper.ok() && p->specialization() == SHAMAN_ELEMENTAL;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = elemental_overload_spell_t::composite_target_multiplier( t );
    if ( p()->talent.inferno_arc.ok() )
    {
      m *= 1.0 + td( t )->dot.flame_shock->is_ticking() * p()->talent.inferno_arc->effectN( 1 ).percent();
    }

    return m;
  }

  void impact( action_state_t* state ) override
  {
    elemental_overload_spell_t::impact( state );

    if ( p()->talent.lightning_rod.ok() || p()->talent.conductive_energy.ok() )
    {
      accumulate_lightning_rod_damage( state );
    }
  }
};

struct lightning_bolt_t : public shaman_spell_t
{
  timespan_t lr_delay;

  lightning_bolt_t( shaman_t* player, unsigned type_, util::string_view options_str = {} ) :
    shaman_spell_t( ::action_name( "lightning_bolt", type_ ),
        player, player->find_class_spell( "Lightning Bolt" ), type_ )
  {
    parse_options( options_str );
    if ( player->specialization() == SHAMAN_ELEMENTAL )
    {
      affected_by_master_of_the_elements = true;

      maelstrom_gain = player->spec.maelstrom->effectN( 1 ).resource( RESOURCE_MAELSTROM );
    }

    if ( player->mastery.elemental_overload->ok() )
    {
      overload = new lightning_bolt_overload_t( player, this );
      //add_child( overload );
    }

    ancestor_trigger = ancestor_cast::LAVA_BURST;

    if ( is_variant( spell_variant::THORIMS_INVOCATION ) )
    {
      background = true;
      base_execute_time = 0_s;
      base_costs[ RESOURCE_MANA ] = 0;
      if ( auto parent = p()->find_action( "thorims_invocation" ) )
      {
        parent->add_child( this );
      }
    }

    if ( is_variant( spell_variant::PRIMORDIAL_STORM ) )
    {
      background = true;
      base_execute_time = 0_s;
      base_costs[ RESOURCE_MANA ] = 0;
      if ( auto ps_action = p()->find_action( "primordial_storm" ) )
      {
        ps_action->add_child( this );
      }
    }
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    if ( is_variant( spell_variant::PRIMORDIAL_STORM ) )
    {
      m *= p()->talent.primordial_storm->effectN( 2 ).percent();
    }

    return m;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = shaman_spell_t::composite_target_multiplier( t );
    if ( p()->talent.inferno_arc.ok() )
    {
      m *= 1.0 + td( t )->dot.flame_shock->is_ticking() * p()->talent.inferno_arc->effectN( 1 ).percent();
    }

    return m;
  }


  void execute() override
  {
    shaman_spell_t::execute();

    if ( is_variant( spell_variant::NORMAL ) && p()->specialization() == SHAMAN_ELEMENTAL )
    {
      if ( !p()->sk_during_cast )
      {
        p()->buff.stormkeeper->decrement();
      }
      p()->sk_during_cast = false;

      if ( p()->talent.routine_communication.ok() && p()->rng_obj.routine_communication->trigger() )
      {
        p()->summon_ancestor();
      }
    }

    // Track last cast for LB / CL because of Thorim's Invocation
    if ( p()->talent.thorims_invocation.ok() && ( is_variant( spell_variant::NORMAL ) || is_variant( spell_variant::PRIMORDIAL_STORM ) ) )
    {
      p()->action.ti_trigger = p()->action.lightning_bolt_ti;
    }

    if ( ( is_variant( spell_variant::NORMAL ) || is_variant( spell_variant::THORIMS_INVOCATION ) )
      && p()->specialization() == SHAMAN_ENHANCEMENT &&
      rng().roll( p()->talent.supercharge->effectN( 2 ).percent() ) )
    {
      p()->generate_maelstrom_weapon( execute_state->action,
                                      as<int>( p()->talent.supercharge->effectN( 3 ).base_value() ) );
    }

    // TODO-midnight-talent: Uniform RNG, or what?
    if ( ( is_variant( spell_variant::NORMAL ) || is_variant( spell_variant::THORIMS_INVOCATION ) )
      && rng().roll( p()->talent.thunder_capacitor->effectN( 2 ).percent() ) )
    {
      sim->print_debug( "{} procs thunder_capacitor", player->name() );
      p()->generate_maelstrom_weapon( execute_state->action, mw_consumed_stacks );
    }

    p()->trigger_thunderstrike_ward( execute_state );

    p()->trigger_totemic_rebound( execute_state );

    if ( is_variant( spell_variant::NORMAL ) || is_variant( spell_variant::THORIMS_INVOCATION ) )
    {
      p()->trigger_whirling_air( execute_state );
    }

    if ( p()->buff.storm_elemental->check() && p()->talent.primal_elementalist.ok() )
    {
      p()->buff.wind_gust->trigger();
    }
  }

  void schedule_travel( action_state_t* s ) override
  {
    if ( is_variant( spell_variant::NORMAL ) && p()->buff.power_of_the_maelstrom->up() )
    {
      trigger_elemental_overload( s, 1.0 );

      p()->buff.power_of_the_maelstrom->decrement();
      if ( p()->talent.fusion_of_elements->ok() )
      {
        p()->action.elemental_blast_foe->execute_on_target( s->target );
      }
    }

    if ( p()->talent.supercharge.ok())
    {
      trigger_elemental_overload( s, p()->talent.supercharge->effectN( 1 ).percent() );
    }

    shaman_spell_t::schedule_travel( s );
  }
  //void reset_swing_timers()
  //{
  //  if ( player->main_hand_attack && player->main_hand_attack->execute_event )
  //  {
  //    event_t::cancel( player->main_hand_attack->execute_event );
  //    player->main_hand_attack->schedule_execute();
  //  }

  //  if ( player->off_hand_attack && player->off_hand_attack->execute_event )
  //  {
  //    event_t::cancel( player->off_hand_attack->execute_event );
  //    player->off_hand_attack->schedule_execute();
  //  }
  //}

  void impact( action_state_t* state ) override
  {
    shaman_spell_t::impact( state );

    if ( p()->talent.conductive_energy.ok() || p()->talent.lightning_rod.ok() )
    {
      accumulate_lightning_rod_damage( state );
    }

    if ( p()->talent.conductive_energy.ok() && p()->specialization() == SHAMAN_ENHANCEMENT )
    {
      // On first impact, randomize a delay for the lightning rod debuff that is associated with all
      // the subsequent debuff triggers
      if ( state->chain_target == 0 )
      {
        lr_delay = rng().range( 10_ms, 100_ms );
      }

      trigger_lightning_rod_debuff( state->target, lr_delay );
    }
  }

  bool ready() override
  {
    if ( is_variant( spell_variant::NORMAL ) && p()->buff.tempest->check() )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

// Elemental Blast Spell ====================================================

void trigger_elemental_blast_proc( shaman_t* p )
{
  unsigned b = static_cast<unsigned>( p->rng().range( 0, 3 ) );

  // if for some reason (Ineffable Truth, corruption) Elemental Blast can trigger four times, just let it overwrite
  // something
  if ( !p->buff.elemental_blast_crit->check() || !p->buff.elemental_blast_haste->check() ||
       !p->buff.elemental_blast_mastery->check() )
  {
    // EB can no longer proc the same buff twice
    while ( ( b == 0 && p->buff.elemental_blast_crit->check() ) ||
            ( b == 1 && p->buff.elemental_blast_haste->check() ) ||
            ( b == 2 && p->buff.elemental_blast_mastery->check() ) )
    {
      b = static_cast<unsigned>( p->rng().range( 0, 3 ) );
    }
  }

  if ( b == 0 )
  {
    p->buff.elemental_blast_crit->trigger();
    p->proc.elemental_blast_crit->occur();
  }
  else if ( b == 1 )
  {
    p->buff.elemental_blast_haste->trigger();
    p->proc.elemental_blast_haste->occur();
  }
  else if ( b == 2 )
  {
    p->buff.elemental_blast_mastery->trigger();
    p->proc.elemental_blast_mastery->occur();
  }
}

struct elemental_blast_overload_t : public elemental_overload_spell_t
{
  elemental_blast_overload_t( shaman_t* p, unsigned type, shaman_spell_t* parent_ )
    : elemental_overload_spell_t( p, ::action_name( "elemental_blast_overload", type ), p->find_spell( 120588 ), parent_,
        p->talent.mountains_will_fall->effectN( 1 ).percent(), type )
  {
    affected_by_master_of_the_elements = true;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = elemental_overload_spell_t::composite_target_multiplier( t );
    if ( p()->talent.inferno_arc.ok() )
    {
      m *= 1.0 + td( t )->dot.flame_shock->is_ticking() * p()->talent.inferno_arc->effectN( 1 ).percent();
    }

    return m;
  }

  double action_multiplier() const override
  {
    double m = elemental_overload_spell_t::action_multiplier();


    if ( is_variant( spell_variant::FUSION_OF_ELEMENTS ) )
    {
      m *= p()->talent.fusion_of_elements->effectN( 1 ).percent();
      m /= p()->talent.mountains_will_fall->effectN( 1 )
               .percent();  // Remove MWTF multiplier, cause bug
    }
    return m;
  }


  void execute() override
  {
    // Trigger buff before executing the spell, because apparently the buffs affect the cast result
    // itself.
    ::trigger_elemental_blast_proc( p() );
    elemental_overload_spell_t::execute();
  }
};

struct elemental_blast_t : public shaman_spell_t
{
  elemental_blast_t( shaman_t* player, unsigned type_, util::string_view options_str = {}) :
    shaman_spell_t(
      ::action_name("elemental_blast", type_),
      player,
      player->find_spell( 117014 ),
      type_
    )
  {
    parse_options( options_str );

    ancestor_trigger = ancestor_cast::LAVA_BURST;

    if ( player->specialization() == SHAMAN_ELEMENTAL )
    {
      affected_by_master_of_the_elements = true;

      if ( p()->talent.mountains_will_fall.enabled() )
      {
        overload = new elemental_blast_overload_t( player, type_, this );
      }

      resource_current = RESOURCE_MAELSTROM;
    }
    else if ( player->specialization() == SHAMAN_ENHANCEMENT )
    {
      if ( player->talent.elemental_blast.ok() && player->talent.lava_burst.ok() )
      {
        cooldown->charges += as<int>( player->find_spell( 394152 )->effectN( 2 ).base_value() );
      }
    }

    if ( is_variant( spell_variant::FUSION_OF_ELEMENTS ) )
    {
      base_execute_time = 0_ms;
      background = true;
      base_costs[ RESOURCE_MANA ] = 0;
      base_costs[ RESOURCE_MAELSTROM ] = 0;
    }
  }

  double action_multiplier() const override
  {
    double m = shaman_spell_t::action_multiplier();

    if ( is_variant( spell_variant::FUSION_OF_ELEMENTS ) )
    {
      m *= p()->talent.fusion_of_elements->effectN( 1 ).percent();
    }

    return m;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = shaman_spell_t::composite_target_multiplier( t );
    if ( p()->talent.inferno_arc.ok() )
    {
      m *= 1.0 + td( t )->dot.flame_shock->is_ticking() * p()->talent.inferno_arc->effectN( 1 ).percent();
    }

    return m;
  }

  bool ready() override
  {
    if ( !p()->talent.elemental_blast->ok() )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }

  void execute() override
  {
    shaman_spell_t::execute();

    ::trigger_elemental_blast_proc( p() );

    // these are effects which ONLY trigger when the player cast the spell directly
    if ( is_variant( spell_variant::NORMAL ) )
    {
      p()->trigger_totemic_rebound( execute_state );
    }

    // [BUG] 2024-08-23 Supercharge works on Elemental Blast in-game
    if ( p()->bugs && is_variant( spell_variant::NORMAL ) &&
         p()->specialization() == SHAMAN_ENHANCEMENT &&
         rng().roll( p()->talent.supercharge->effectN( 2 ).percent() ) )
    {
      p()->generate_maelstrom_weapon( this, as<int>( p()->talent.supercharge->effectN( 3 ).base_value() ) );
    }

    if ( is_variant( spell_variant::NORMAL ) )
    {
      p()->trigger_whirling_air( execute_state );
    }
  }

  void impact( action_state_t* state ) override
  {
    shaman_spell_t::impact( state );

    if ( p()->specialization() == SHAMAN_ENHANCEMENT && p()->talent.conductive_energy.ok() )
    {
        if ( p()->bugs )
        {
            accumulate_lightning_rod_damage( state );
        }

        trigger_lightning_rod_debuff( state->target );
    }

    if ( p()->specialization() == SHAMAN_ELEMENTAL && p()->talent.lightning_rod.ok() )
    {
        trigger_lightning_rod_debuff( state->target );
    }
  }
};

// Thunderstorm Spell =======================================================

struct thunderstorm_t : public shaman_spell_t
{
  thunderstorm_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "thunderstorm", player, player->talent.thunderstorm )
  {
    parse_options( options_str );
    aoe = -1;
  }
};

struct spiritwalkers_grace_t : public shaman_spell_t
{
  spiritwalkers_grace_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "spiritwalkers_grace", player, player->talent.spiritwalkers_grace )
  {
    parse_options( options_str );
    may_miss = may_crit = harmful = callbacks = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->buff.spiritwalkers_grace->trigger();
  }
};

// Earthquake ===============================================================

struct earthquake_damage_base_t : public shaman_spell_t
{
  // Deeptremor Totem needs special handling to enable persistent MoTE buff. Normal
  // Earthquake can use the persistent multiplier below
  bool mote_buffed;

  action_t* parent;

  earthquake_damage_base_t( shaman_t* player, util::string_view name, const spell_data_t* spell, action_t* p = nullptr ) :
    shaman_spell_t( name, player, spell ), mote_buffed( false ), parent( p )
  {
    aoe        = -1;
    ground_aoe = background = ignores_armor = true;
  }

  // Snapshot base state from the parent to grab proper persistent multiplier for all damage
  // (normal, overload)
  void snapshot_state( action_state_t* s, result_amount_type rt ) override
  {
    // TODO: remove check for parent when we remove runeforged effects (Shadowlands legendaries)
    if ( parent )
    {
      s->copy_state( parent->execute_state );
    }
    else
    {
      shaman_spell_t::snapshot_state( s, rt );
    }
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = shaman_spell_t::composite_target_multiplier( t );
    if ( p()->talent.inferno_arc.ok() )
    {
      m *= 1.0 + td( t )->dot.flame_shock->is_ticking() * p()->talent.inferno_arc->effectN( 1 ).percent();
    }

    return m;
  }

  // Persistent multiplier handling is also here to support Deeptremor Totem, since it will not have
  // a parent defined
  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = shaman_spell_t::composite_persistent_multiplier( state );

    if ( mote_buffed || p()->buff.master_of_the_elements->up() )
    {
      m *= 1.0 + p()->buff.master_of_the_elements->default_value;
    }

    return m;
  }

  double get_spell_power_coefficient_from_options() {
      auto default_options = new shaman_t::options_t();
      auto default_value = default_options->earthquake_spell_power_coefficient;
      auto option_value = p()->options.earthquake_spell_power_coefficient;
      delete( default_options );

      if ( option_value != default_value )
      {
          return option_value;
      }
      return 0.0;
  }
};

struct earthquake_base_t : public shaman_spell_t
{
  earthquake_damage_base_t* rumble;

  earthquake_base_t( shaman_t* player, util::string_view name, const spell_data_t* spell ) :
    shaman_spell_t( name, player, spell ),
    rumble( nullptr )
  {
    dot_duration = timespan_t::zero();  // The periodic effect is handled by ground_aoe_event_t

    ancestor_trigger = ancestor_cast::CHAIN_LIGHTNING;
  }

  void init_finished() override
  {
    shaman_spell_t::init_finished();

    // Copy state flagging from the damage spell so we an inherit snapshot state in the damage spell
    // properly when the ground aoe event below is executed. This ensures proper inheritance of
    // persistent multipliers to the base earthquake, as well as the overload.
    snapshot_flags = rumble->snapshot_flags;
    update_flags = rumble->update_flags;
  }

  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = shaman_spell_t::composite_persistent_multiplier( state );

    if ( p()->buff.master_of_the_elements->up() )
    {
      m *= 1.0 + p()->buff.master_of_the_elements->default_value;
    }

    return m;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    auto eq_duration = data().duration();

    make_event<ground_aoe_event_t>(
        *sim, p(),
        ground_aoe_params_t()
          .target( execute_state->target )
          .duration( eq_duration )
          .action( rumble ) );
  }
};

struct earthquake_overload_damage_t : public earthquake_damage_base_t
{
  earthquake_overload_damage_t( shaman_t* player, earthquake_base_t* parent ) :
    earthquake_damage_base_t( player, "earthquake_overload_damage", player->find_spell( 298765 ), parent )
  {

  }

  double action_multiplier() const override
  {
    double m = earthquake_damage_base_t::action_multiplier();

    if ( p()->buff.ascendance->up() )
    {
      m *= (1.0 + p()->spell.ascendance->effectN( 8 ).percent());
    }

    m *= p()->talent.mountains_will_fall->effectN( 1 ).percent() *
         p()->mastery.elemental_overload->effectN( 2 ).percent();

    return m;
  }
};

struct earthquake_overload_t : public earthquake_base_t
{
  earthquake_base_t* parent;

  earthquake_overload_t( shaman_t* player, earthquake_base_t* p ) :
    earthquake_base_t( player, "earthquake_overload", player->find_spell( 298762 ) ),
    parent( p )
  {
    background = true;
    callbacks = false;
    base_execute_time = 0_s;

    rumble = new earthquake_overload_damage_t( player, this );
    add_child( rumble );
  }

  void snapshot_internal( action_state_t* s, unsigned flags, result_amount_type rt ) override
  {
    earthquake_base_t::snapshot_internal( s, flags, rt );

    cast_state( s )->variant_flags( parent->variant_flags() );
  }
};

struct earthquake_damage_t : public earthquake_damage_base_t
{
  earthquake_damage_t( shaman_t* player, earthquake_base_t* parent = nullptr ) :
    earthquake_damage_base_t( player, "earthquake_damage", player->find_spell( 77478 ), parent )
  {
  }
};

struct earthquake_t : public earthquake_base_t
{
  earthquake_t( shaman_t* player, util::string_view options_str ) :
    earthquake_base_t( player, "earthquake", player->talent.earthquake_reticle.ok()
      ? player->talent.earthquake_reticle
      : player->talent.earthquake_target )
  {
    parse_options( options_str );

    rumble = new earthquake_damage_t( player, this );
    add_child( rumble );
    affected_by_master_of_the_elements = true;

    if ( player->talent.mountains_will_fall.ok() )
    {
      overload = new earthquake_overload_t( player, this );
      add_child( overload );
    }
  }

  // Earthquake uses a "smart" Lightning Rod targeting system
  // 1) Current target, if Lightning Rod is not enabled on it
  // 2) A close-by target without Lightning Rod
  //
  // Note that Earthquake does not refresh existing Lightning Rod debuffs
  void trigger_lightning_rod( const action_state_t* state )
  {
    if ( !p()->talent.lightning_rod.ok() )
    {
      return;
    }

    auto tdata = td( state->target );
    if ( !tdata->debuff.lightning_rod->check() )
    {
      trigger_lightning_rod_debuff( state->target );
    }
    else
    {
      std::vector<player_t*> eligible_targets;
      range::for_each( target_list(), [ this, &eligible_targets ]( player_t* t ) {
        if ( !td( t )->debuff.lightning_rod->check() )
        {
          eligible_targets.emplace_back( t );
        }
      } );

      if ( !eligible_targets.empty() )
      {
        auto idx = rng().range( 0U, as<unsigned>( eligible_targets.size() ) );
        trigger_lightning_rod_debuff( eligible_targets[ idx ] );
      }
    }
  }

  void execute() override
  {
    earthquake_base_t::execute();

    trigger_lightning_rod( execute_state );

    // Note, needs to be decremented after ground_aoe_event_t is created so that the rumble gets the
    // buff multiplier as persistent.

    p()->buff.master_of_the_elements->decrement();

  }
};

struct spirit_walk_t : public shaman_spell_t
{
  spirit_walk_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "spirit_walk", player, player->talent.spirit_walk )
  {
    parse_options( options_str );
    may_miss = may_crit = harmful = callbacks = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->buff.spirit_walk->trigger();
  }
};

struct ghost_wolf_t : public shaman_spell_t
{
  ghost_wolf_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "ghost_wolf", player, player->find_class_spell( "Ghost Wolf" ) )
  {
    parse_options( options_str );
    unshift_ghost_wolf = false;  // Customize unshifting logic here
    harmful = callbacks = may_crit = false;
  }

  timespan_t gcd() const override
  {
    if ( p()->buff.ghost_wolf->check() )
    {
      return timespan_t::zero();
    }

    return shaman_spell_t::gcd();
  }

  void execute() override
  {
    shaman_spell_t::execute();

    if ( !p()->buff.ghost_wolf->check() )
    {
      p()->buff.ghost_wolf->trigger();
    }
    else
    {
      p()->buff.ghost_wolf->expire();
    }
  }
};

struct feral_lunge_t : public shaman_spell_t
{
  struct feral_lunge_attack_t : public shaman_attack_t
  {
    feral_lunge_attack_t( shaman_t* p ) : shaman_attack_t( "feral_lunge_attack", p, p->find_spell( 215802 ) )
    {
      background = true;
      callbacks = false;
    }

    void init() override
    {
      shaman_attack_t::init();

      p()->set_mw_proc_state( this, mw_proc_state::DISABLED );
    }
  };

  feral_lunge_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "feral_lunge", player, player->spec.feral_lunge )
  {
    parse_options( options_str );
    unshift_ghost_wolf = false;

    impact_action = new feral_lunge_attack_t( player );
  }

  bool ready() override
  {
    if ( !p()->buff.ghost_wolf->check() )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

// Nature's Swiftness Spell =================================================

struct natures_swiftness_t : public shaman_spell_t
{
  natures_swiftness_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "natures_swiftness", player, player->talent.natures_swiftness )
  {
    parse_options( options_str );

    harmful = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->buff.natures_swiftness->trigger();
  }

  bool ready() override
  {
    if ( p()->talent.ancestral_swiftness.ok() )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

// Ancestral Swiftness Spell =================================================

struct ancestral_swiftness_t : public shaman_spell_t
{
  ancestral_swiftness_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "ancestral_swiftness", player, player->find_spell( 443454 ) )
  {
    parse_options( options_str );

    harmful = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->buff.ancestral_swiftness->trigger();

    if ( p()->talent.natures_swiftness.ok() )
    {
      p()->summon_ancestor();
    }
  }

  bool ready() override
  {
    if ( !p()->talent.ancestral_swiftness.ok() )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

// ==========================================================================
// Shaman Shock Spells
// ==========================================================================

// Earth Shock Spell ========================================================
struct earth_shock_overload_t : public elemental_overload_spell_t
{
  earth_shock_overload_t( shaman_t* p, shaman_spell_t* parent_ )
    : elemental_overload_spell_t( p, "earth_shock_overload", p->find_spell( 381725 ), parent_,
        p->talent.mountains_will_fall->effectN( 1 ).percent() )
  {
    affected_by_master_of_the_elements = true;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = elemental_overload_spell_t::composite_target_multiplier( t );
    if ( p()->talent.inferno_arc.ok() )
    {
      m *= 1.0 + td( t )->dot.flame_shock->is_ticking() * p()->talent.inferno_arc->effectN( 1 ).percent();
    }

    return m;
  }
};

struct earth_shock_t : public shaman_spell_t
{
  earth_shock_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "earth_shock", player, player->talent.earth_shock )
  {
    parse_options( options_str );
    // hardcoded because spelldata doesn't provide the resource type
    resource_current                   = RESOURCE_MAELSTROM;
    affected_by_master_of_the_elements = true;
    ancestor_trigger = ancestor_cast::LAVA_BURST;

    if ( p()->talent.mountains_will_fall.enabled() )
    {
      overload = new earth_shock_overload_t( player, this );
    }
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = shaman_spell_t::composite_target_multiplier( t );
    if ( p()->talent.inferno_arc.ok() )
    {
      m *= 1.0 + td( t )->dot.flame_shock->is_ticking() * p()->talent.inferno_arc->effectN( 1 ).percent();
    }

    return m;
  }

  void impact( action_state_t* state ) override
  {
    shaman_spell_t::impact( state );

    if ( p()->talent.lightning_rod.ok() )
    {
      trigger_lightning_rod_debuff( state->target );
    }
  }

  bool ready() override
  {
    bool r = shaman_spell_t::ready();
    if ( p()->talent.elemental_blast.enabled() )
    {
      r = false;
    }
    return r;
  }
};
// Flame Shock Spell ========================================================

struct flame_shock_t : public shaman_spell_t
{
private:
  const spell_data_t* elemental_resource;

  void track_flame_shock( const action_state_t* state )
  {
    // No need to track anything if there are not enough enemies
    if ( sim->target_list.size() <= p()->max_active_flame_shock )
    {
      return;
    }

    // Remove tracking on the newly applied dot. It'll be re-added to the tracking at the
    // end of this method to keep ascending start-time order intact.
    auto dot = get_dot( state->target );
    untrack_flame_shock( dot );

    // Max targets reached (the new Flame Shock application is on a target without a dot
    // active), remove one of the oldest applied dots to make room.
    if ( p()->active_flame_shock.size() == p()->max_active_flame_shock )
    {
      auto start_time = p()->active_flame_shock.front().first;
      auto entry = range::find_if( p()->active_flame_shock, [ start_time ]( const auto& entry ) {
          return entry.first != start_time;
      } );

      // Randomize equal start time application removal
      auto candidate_targets = as<double>(
          std::distance( p()->active_flame_shock.begin(), entry ) );
      auto idx = static_cast<unsigned>( rng().range( 0.0, candidate_targets ) );

      if ( sim->debug )
      {
        std::vector<util::string_view> enemies;
        for ( auto it = p()->active_flame_shock.begin(); it < entry; ++it )
        {
          enemies.emplace_back( it->second->target->name() );
        }

        sim->out_debug.print(
          "{} canceling oldest {}: new_target={} cancel_target={} (index={}), start_time={}, "
          "candidate_targets={} ({})",
          player->name(), name(), state->target->name(),
          p()->active_flame_shock[ idx ].second->state->target->name(), idx,
          p()->active_flame_shock[ idx ].first, as<unsigned>( candidate_targets ),
          util::string_join( enemies ) );
      }

      p()->active_flame_shock[ idx ].second->cancel();
    }

    p()->active_flame_shock.emplace_back( sim->current_time(), dot );
  }

  void untrack_flame_shock( const dot_t* d )
  {
    unsigned max_targets = as<unsigned>( data().max_targets() );

    // No need to track anything if there are not enough enemies
    if ( sim->target_list.size() <= max_targets )
    {
      return;
    }

    auto it = range::find_if( p()->active_flame_shock, [ d ]( const auto& dot_state ) {
      return dot_state.second == d;
    } );

    if ( it != p()->active_flame_shock.end() )
    {
      p()->active_flame_shock.erase( it );
    }
  }

  void invalidate_dependant_targets()
  {
    range::for_each( p()->flame_shock_dependants, []( action_t* a ) {
      a->target_cache.is_valid = false;
    } );
  }

public:
  flame_shock_t( shaman_t* player, unsigned type_, util::string_view options_str = {} )
    // Specifically not using a spell_variant aware name to prevent the creation of separate flame shock dots.
    // All separate variants shall debuff the same dot.
    : shaman_spell_t( "flame_shock", player, player->find_spell( 188389 ), type_),
      elemental_resource( player->find_spell( 263819 ) )
  {
    parse_options( options_str );
    affected_by_master_of_the_elements = true;

    // Ensure Flame Shock is single target, since Simulationcraft naively interprets a
    // Max Targets value on a spell to mean "aoe this many targets"
    aoe = 0;
    ancestor_trigger = ancestor_cast::LAVA_BURST;

    if ( is_variant( spell_variant::ASCENDANCE ) )
    {
      maelstrom_gain = 0;
      background = true;
      cooldown = player->get_cooldown( "__flame_shock_secondary" );
      base_costs[ RESOURCE_MANA ] = 0;
    }

    if ( is_variant( spell_variant::VOLTAIC_BLAZE ) )
    {
      maelstrom_gain = 0;
      background     = true;
      cooldown       = player->get_cooldown( "__flame_shock_secondary" );
      base_costs[ RESOURCE_MANA ] = 0;
    }
  }

  void trigger_dot( action_state_t* state ) override
  {
    if ( !get_dot( state->target )->is_ticking() )
    {
      invalidate_dependant_targets();
    }

    track_flame_shock( state );

    shaman_spell_t::trigger_dot( state );
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = shaman_spell_t::composite_target_multiplier( t );

    m *= 1.0 + td( t )->debuff.lashing_flames->stack_value();

    return m;
  }

  double dot_duration_pct_multiplier( const action_state_t* s ) const override
  {
    auto mul = shaman_spell_t::dot_duration_pct_multiplier( s );

    if ( p()->buff.fire_elemental->check() && p()->spell.fire_elemental->ok() )
    {
      mul *= 1.0 + p()->spell.fire_elemental->effectN( 3 ).percent();
    }

    return mul;
  }

  double tick_time_pct_multiplier( const action_state_t* state ) const override
  {
    auto mul = shaman_spell_t::tick_time_pct_multiplier( state );

    mul *= 1.0 + p()->buff.fire_elemental->stack_value();

    return mul;
  }

  void tick( dot_t* d ) override
  {
    shaman_spell_t::tick( d );

    if ( p()->spec.lava_surge->ok() )
    {
      double active_flame_shocks = p()->get_active_dots( d );
      p()->lava_surge_attempts_normalized += 1.0/active_flame_shocks;
      double proc_chance =
          std::max( 0.0, 0.6-std::pow(1.16, -2*(p()->lava_surge_attempts_normalized-5)));

      proc_chance *= 1.0 + p()->talent.mystic_knowledge->effectN( 1 ).percent();

      if ( p()->spec.restoration_shaman->ok() )
      {
        proc_chance += p()->spec.restoration_shaman->effectN( 7 ).percent();
      }

      if ( rng().roll( proc_chance ) )
      {
        p()->trigger_lava_surge();
        p()->lvs_samples.add( p()->lava_surge_attempts_normalized );
        p()->lava_surge_attempts_normalized = 0.0;
      }
    }

    // TODO: Determine proc chance / model
    // First single target test showed a 25% chance. I didn't find it in
    // spelldata.
    if ( p()->talent.searing_flames->ok() && rng().roll( 0.25 ) )
    {
      p()->trigger_maelstrom_gain( p()->talent.searing_flames->effectN( 1 ).base_value(), p()->gain.searing_flames );
      p()->proc.searing_flames->occur();
    }
  }

  void last_tick( dot_t* d ) override
  {
    shaman_spell_t::last_tick( d );

    untrack_flame_shock( d );
    invalidate_dependant_targets();
  }

  bool ready() override
  {
    if ( p()->talent.voltaic_blaze.ok() )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }

  void execute() override
  {
    shaman_spell_t::execute();
    if ( p()->talent.routine_communication.ok() && p()->rng_obj.routine_communication->trigger() )
    {
      p()->summon_ancestor();
    }
  }
};

// Frost Shock Spell ========================================================

struct frost_shock_t : public shaman_spell_t
{
  frost_shock_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "frost_shock", player, player->talent.frost_shock )
  {
    parse_options( options_str );
    affected_by_master_of_the_elements = true;
    ancestor_trigger = ancestor_cast::LAVA_BURST;

    if ( player->specialization() == SHAMAN_ENHANCEMENT )
    {
      track_cd_waste = true;
    }
  }
};

// Wind Shear Spell =========================================================

struct wind_shear_t : public shaman_spell_t
{
  wind_shear_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "wind_shear", player, player->talent.wind_shear )
  {
    parse_options( options_str );
    may_miss = may_crit   = false;
    ignore_false_positive = true;
    is_interrupt = true;
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( candidate_target->debuffs.casting && !candidate_target->debuffs.casting->check() )
    {
      return false;
    }

    return shaman_spell_t::target_ready( candidate_target );
  }

  void execute() override
  {
    shaman_spell_t::execute();

    if ( p()->spec.inundate->ok() )
    {
      p()->trigger_maelstrom_gain( p()->spell.inundate->effectN( 1 ).base_value(), p()->gain.inundate );
    }
  }
};

// Ascendance Enhance Damage Spell =========================================================

struct ascendance_damage_t : public shaman_spell_t
{
  ascendance_damage_t( shaman_t* player, util::string_view name_str )
    : shaman_spell_t( name_str, player, player->find_spell( 344548 ) )
  {
    aoe = -1;
    background = true;
  }
};

// Ascendance Spell =========================================================

struct ascendance_t : public shaman_spell_t
{
  lava_burst_t* lvb;
  lava_burst_overload_t* lvb_ol;

  ascendance_t( shaman_t* player, util::string_view name_str, util::string_view options_str = {},
                unsigned var_ = static_cast<unsigned>( spell_variant::NORMAL ) )
    : shaman_spell_t( name_str, player, player->spell.ascendance, var_ ),
      lvb( nullptr ), lvb_ol( nullptr )
  {
    parse_options( options_str );
    harmful = false;

    // Periodic effect for Enhancement handled by the buff
    dot_duration = base_tick_time = timespan_t::zero();
    ancestor_trigger = ancestor_cast::CHAIN_LIGHTNING;

    // Cache pointer for MW tracking uses
    p()->action.ascendance = this;
  }

  void init() override
  {
    shaman_spell_t::init();

    if ( p()->specialization() == SHAMAN_ELEMENTAL )
    {
      if ( auto trigger_spell = p()->find_action( "lava_burst_ascendance" ) )
      {
        lvb = debug_cast<lava_burst_t*>( trigger_spell );
      }
      else
      {
        lvb = new lava_burst_t( p(), variant_flag( spell_variant::ASCENDANCE ) );
        add_child( lvb );
      }
    }
  }

  void execute() override
  {
    shaman_spell_t::execute();

    if ( p()->spell.tww3_stormbringer_2pc->ok() )
    {
        p()->buff.tempest->trigger();
    }
    if (p()->spell.tww3_stormbringer_4pc->ok())
    {
      p()->buff.storms_eye->trigger(2);
    }

    p()->cooldown.strike->reset( false );

    timespan_t duration = 0_ms;

    if ( background )
    {
      assert( is_variant( spell_variant::DEEPLY_ROOTED_ELEMENTS ) ||
        is_variant( spell_variant::TWW3_SPELL ) );

      if ( is_variant( spell_variant::DEEPLY_ROOTED_ELEMENTS ) )
      {
        duration = p()->talent.deeply_rooted_elements->effectN( 1 ).time_value();
      }
      else
      {
        if ( p()->specialization() == SHAMAN_ENHANCEMENT )
        {
          duration = p()->spell.tww3_stormbringer_2pc->effectN( 1 )
                         .time_value();
        }
        else
        {
          duration = p()->spell.tww3_stormbringer_2pc->effectN( 5 )
                         .time_value();
        }
      }
      assert( ( duration != timespan_t::zero() ) );
      p()->buff.ascendance->extend_duration_or_trigger( duration );
    }
    else
    {
      p()->buff.ascendance->trigger();
    }

    if ( p()->talent.call_of_fire.ok() )
    {
      p()->summon_elemental( p()->talent.primal_elementalist.ok() ? elemental::PRIMAL_FIRE : elemental::GREATER_FIRE );
    }

    // Refresh Flame Shock to max duration
    if ( p()->specialization() == SHAMAN_ELEMENTAL )
    {
      // Apparently the Flame Shock durations get set to current Flame Shock max duration,
      // bypassing normal dot refresh behavior.
      auto tl = target_list();
      for ( size_t i = 0; i < std::min( tl.size(), as<size_t>( data().effectN( 7 ).base_value() ) ); ++i )
      {
        p()->trigger_secondary_flame_shock( tl[ i ], spell_variant::ASCENDANCE );
      }
    }

    if ( lvb )
    {
      lvb->set_target( player->target );
      lvb->target_cache.is_valid = false;
      if ( !lvb->target_list().empty() )
      {
        lvb->execute();
      }
    }

    if ( p()->specialization() == SHAMAN_ENHANCEMENT )
    {
      p()->action.ascendance_damage->execute_on_target( target );
      p()->action.doom_winds_asc->execute_on_target( target );
    }
  }

  bool ready() override
  {
    if ( !p()->talent.ascendance->ok() )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

struct ascendance_dre_t : public ascendance_t
{
  ascendance_dre_t( shaman_t* player, unsigned var_ )
    : ascendance_t( player, "ascendance_dre", {}, var_ )
  {
    background = true;
    cooldown->duration = 0_s;
  }

  // Note, bypasses calling ascendance_t::init() to not bother initializing the ascendance
  // version for the lava burst
  void init() override
  {
    shaman_spell_t::init();

    if ( p()->specialization() == SHAMAN_ELEMENTAL )
    {
      if ( auto trigger_spell = p()->find_action( "lava_burst_dre" ) )
      {
        lvb = debug_cast<lava_burst_t*>( trigger_spell );
      }
      else
      {
        lvb = new lava_burst_t( p(), variant_flag( spell_variant::DEEPLY_ROOTED_ELEMENTS ) );
        add_child( lvb );
      }
      lvb_ol = debug_cast<lava_burst_overload_t*>( p()->find_action( "lava_burst_overload" ) );
    }
  }
};

// Stormkeeper Spell ========================================================

struct stormkeeper_t : public shaman_spell_t
{
  stormkeeper_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "stormkeeper", player, player->find_spell( 191634 ) )
  {
    parse_options( options_str );
    may_crit = harmful = false;
  }

  void execute() override
  {
    // If Stormkeeper buff is already active, casting Stormkeeper does not generate Maelstrom.
    auto original_maelstrom_gain = maelstrom_gain;
    bool reset_maelstrom_gain = false;
    if ( p()->bugs && p()->talent.stormwell.ok() && p()->buff.stormkeeper->up() )
    {
      maelstrom_gain = 0.0;
      reset_maelstrom_gain = true;
    }

    shaman_spell_t::execute();

    if ( p()->talent.fury_of_the_storms.ok() )
    {
      p()->summon_elemental( p()->talent.primal_elementalist.ok() ? elemental::PRIMAL_STORM : elemental::GREATER_STORM,
                             p()->spell.storm_elemental->duration() );
    }

    p()->summon_ancestor();
    p()->buff.stormkeeper->trigger( as<int>( data().effectN( 5 ).base_value() ) );

    p()->buff.mid1_ele_2pc->trigger();

    if ( reset_maelstrom_gain )
    {
      maelstrom_gain = original_maelstrom_gain;
    }

  }

  bool ready() override
  {
    if ( !p()->talent.stormkeeper.ok() )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

// Doom Winds Spell ===========================================================

struct doom_winds_damage_t : public shaman_attack_t
{
  doom_winds_damage_t( shaman_t* player, unsigned t ) :
    shaman_attack_t( ::action_name( "doom_winds_damage", t ), player, player->find_spell( 469270 ), t )
  {
    background = true;
    aoe = -1;
    reduced_aoe_targets = 5.0;
  }

  void execute() override
  {
    shaman_attack_t::execute();

    p()->buff.tww2_enh_4pc->trigger();
    p()->buff.tww2_enh_4pc_damage->trigger();
  }
};

struct doom_winds_t : public shaman_attack_t
{
  doom_winds_t( shaman_t* player, unsigned t, util::string_view options_str = {} ) :
    shaman_attack_t( ::action_name( "doom_winds", t ), player, player->talent.doom_winds, t )
  {
    parse_options( options_str );

    weapon = &( player->main_hand_weapon );
    weapon_multiplier = 0.0;

    if ( is_variant( spell_variant::ASCENDANCE ) )
    {
      cooldown = player->get_cooldown( "doom_winds_ascendance" );
      background = true;
    }
    else
    {
      add_child( player->action.doom_winds );
    }
  }

  void execute() override
  {
    shaman_attack_t::execute();

    p()->buff.doom_winds->extend_duration_or_trigger(
      data().effectN( 1 ).trigger()->duration() );

    if ( p()->talent.feral_spirit.ok() )
    {
      p()->summon_feral_spirit( wolf_type_e::LIGHTNING_WOLF,
        as<int>( p()->talent.feral_spirit->effectN( 1 ).base_value() ),
        p()->spell.flowing_spirits_feral_spirit->duration() );
    }

    // TODO-midnight-talent: Crackling Thunder duration
    if ( p()->talent.rolling_thunder.ok() && p()->specialization() == SHAMAN_ENHANCEMENT )
    {
      p()->summon_feral_spirit( wolf_type_e::LIGHTNING_WOLF, 1,
        p()->talent.rolling_thunder->effectN( 2 ).time_value() );
    }
  }

  bool ready() override
  {
    if ( p()->talent.ascendance.ok() || p()->talent.deeply_rooted_elements.ok() )
    {
      return false;
    }

    return shaman_attack_t::ready();
  }
};

// Ancestral Guidance Spell ===================================================

struct ancestral_guidance_t : public shaman_heal_t
{
  ancestral_guidance_t( shaman_t* player, util::string_view options_str ) :
    shaman_heal_t( "ancestral_guidance", player, player->talent.ancestral_guidance )
  {
    parse_options( options_str );
  }
};

// Healing Surge Spell ======================================================

struct healing_surge_t : public shaman_heal_t
{
  healing_surge_t( shaman_t* player, util::string_view options_str )
    : shaman_heal_t("healing_surge", player, player->find_class_spell( "Healing Surge" ), options_str )
  {
    resurgence_gain =
        0.6 * p()->spell.resurgence->effectN( 1 ).average( player ) * p()->spec.resurgence->effectN( 1 ).percent();
  }

  double composite_crit_chance() const override
  {
    double c = shaman_heal_t::composite_crit_chance();

    if ( p()->buff.tidal_waves->up() )
    {
      c += p()->spec.tidal_waves->effectN( 1 ).percent();
    }

    return c;
  }
};

// Healing Wave Spell =======================================================

struct healing_wave_t : public shaman_heal_t
{
  healing_wave_t( shaman_t* player, util::string_view options_str )
    : shaman_heal_t("healing_wave", player, player->find_specialization_spell( "Healing Wave" ), options_str )
  {
    resurgence_gain =
        p()->spell.resurgence->effectN( 1 ).average( player ) * p()->spec.resurgence->effectN( 1 ).percent();
  }

  double execute_time_pct_multiplier() const override
  {
    auto mul = shaman_heal_t::execute_time_pct_multiplier();

    if ( p()->buff.tidal_waves->up() )
    {
      mul *= 1.0 - p()->spec.tidal_waves->effectN( 1 ).percent();
    }

    return mul;
  }
};

// Riptide Spell ============================================================

struct riptide_t : public shaman_heal_t
{
  riptide_t( shaman_t* player, util::string_view options_str )
    : shaman_heal_t("riptide", player, player->find_specialization_spell( "Riptide" ), options_str )
  {
    resurgence_gain =
        0.6 * p()->spell.resurgence->effectN( 1 ).average( player ) * p()->spec.resurgence->effectN( 1 ).percent();
  }
};

// Chain Heal Spell =========================================================

struct chain_heal_t : public shaman_heal_t
{
  chain_heal_t( shaman_t* player, util::string_view options_str )
    : shaman_heal_t("chain_heal", player, player->find_class_spell( "Chain Heal" ), options_str )
  {
    resurgence_gain =
        0.333 * p()->spell.resurgence->effectN( 1 ).average( player ) * p()->spec.resurgence->effectN( 1 ).percent();
  }

  double composite_target_da_multiplier( player_t* t ) const override
  {
    double m = shaman_heal_t::composite_target_da_multiplier( t );

    if ( td( t )->heal.riptide && td( t )->heal.riptide->is_ticking() )
      m *= 1.0 + p()->spec.riptide->effectN( 3 ).percent();

    return m;
  }
};

// Healing Rain Spell =======================================================

struct healing_rain_t : public shaman_heal_t
{
  struct healing_rain_aoe_tick_t : public shaman_heal_t
  {
    healing_rain_aoe_tick_t( shaman_t* player )
      : shaman_heal_t( "healing_rain_tick", player, player->find_spell( 73921 ) )
    {
      background = true;
      aoe        = -1;
    }
  };

  healing_rain_t( shaman_t* player, util::string_view options_str )
    : shaman_heal_t( "healing_rain", player, player->find_specialization_spell( "Healing Rain" ),
                     options_str )
  {
    base_tick_time = data().effectN( 2 ).period();
    dot_duration   = data().duration();
    hasted_ticks   = false;
    tick_action    = new healing_rain_aoe_tick_t( player );
  }
};

// ==========================================================================
// Shaman Totem System
// ==========================================================================

template <typename T>
struct shaman_totem_pet_t : public pet_t
{
  // Pulse related functionality
  totem_pulse_action_t<T>* pulse_action;
  event_t* pulse_event;
  timespan_t pulse_amplitude;
  bool pulse_on_expire;

  // Summon related functionality
  std::string pet_name;
  pet_t* summon_pet;

  shaman_totem_pet_t( shaman_t* p, util::string_view n )
    : pet_t( p->sim, p, n, true ),
      pulse_action( nullptr ),
      pulse_event( nullptr ),
      pulse_amplitude( timespan_t::zero() ),
      pulse_on_expire( true ),
      summon_pet( nullptr )
  {
    resource_regeneration = regen_type::DISABLED;
  }

  void summon( timespan_t = timespan_t::zero() ) override;
  void dismiss( bool expired = false ) override;

  void init_finished() override
  {
    if ( !pet_name.empty() )
    {
      summon_pet = owner->find_pet( pet_name );
    }

    pet_t::init_finished();
  }

  shaman_t* o()
  {
    return debug_cast<shaman_t*>( owner );
  }

  /*
  //  Code to make a totem double dip on player multipliers.
  //  As of 7.3.5 this is no longer needed for Liquid Magma Totem (Elemental)
  virtual double composite_player_multiplier( school_e school ) const override
  { return owner -> cache.player_multiplier( school ); }
  //*/

  double composite_spell_hit() const override
  {
    return owner->cache.spell_hit();
  }

  double composite_spell_crit_chance() const override
  {
    return owner->cache.spell_crit_chance();
  }

  double composite_spell_power( school_e school ) const override
  {
    return owner->composite_spell_power( school );
  }

  double composite_spell_power_multiplier() const override
  {
    return owner->composite_spell_power_multiplier();
  }

  double composite_total_spell_power( school_e school ) const override
  {
    return owner->composite_total_spell_power( school );
  }

  double composite_total_attack_power_by_type( attack_power_type type ) const override
  {
    return owner->composite_total_attack_power_by_type( type );
  }

  std::unique_ptr<expr_t> create_expression( util::string_view name ) override
  {
    if ( util::str_compare_ci( name, "duration" ) )
      return make_ref_expr( name, duration );

    return pet_t::create_expression( name );
  }
};

template <typename TOTEM, typename BASE>
struct shaman_totem_t : public BASE
{
  timespan_t totem_duration;
  spawner::pet_spawner_t<TOTEM, shaman_t>& spawner;

  shaman_totem_t( util::string_view totem_name, shaman_t* player, util::string_view options_str,
                  const spell_data_t* spell_data, spawner::pet_spawner_t<TOTEM, shaman_t>& sp ) :
    BASE( totem_name, player, spell_data ),
    totem_duration( this->data().duration() ), spawner( sp )
  {
    this->parse_options( options_str );
    this->harmful = this->callbacks = this->may_miss = this->may_crit = false;
    this->ignore_false_positive = true;
    this->dot_duration = timespan_t::zero();
  }

  void execute() override
  {
    BASE::execute();

    spawner.spawn( totem_duration );

  }

  std::unique_ptr<expr_t> create_expression( util::string_view name ) override
  {
    // Redirect active/remains to "pet.<totem name>.active/remains" so things work ok with the
    // pet initialization order shenanigans. Otherwise, at this point in time (when
    // create_expression is called), the pets don't actually exist yet.
    if ( util::str_compare_ci( name, "active" ) )
      return this->player->create_expression( "pet." + this->name_str + ".active" );
    else if ( util::str_compare_ci( name, "remains" ) )
      return this->player->create_expression( "pet." + this->name_str + ".remains" );
    else if ( util::str_compare_ci( name, "duration" ) )
      return make_ref_expr( name, totem_duration );
    return BASE::create_expression( name );
  }
};

template <typename T>
struct totem_pulse_action_t : public T
{
  bool hasted_pulse;
  double pulse_multiplier;
  shaman_totem_pet_t<T>* totem;
  unsigned pulse;

  bool affected_by_totemic_rebound_da;

  totem_pulse_action_t( const std::string& token, shaman_totem_pet_t<T>* p, const spell_data_t* s )
    : T( token, p, s ), hasted_pulse( false ), pulse_multiplier( 1.0 ), totem( p ), pulse ( 0 )
  {
    this->may_crit = this->background = true;
    this->callbacks = false;

    if ( this->type == ACTION_HEAL )
    {
      this->harmful = false;
      this->target = totem->owner;
    }
    else
    {
      this->harmful = true;
    }

    affected_by_totemic_rebound_da = T::data().affected_by_all( o()->buff.totemic_rebound->data().effectN( 1 ) ) ||
                                     T::data().affected_by_all( o()->buff.totemic_rebound->data().effectN( 2 ) );

    if ( T::data().ok() )
    {
      // Override source of stats/state for parse_effects to the owner, since that's where the stats
      // for totem actions come from.
      this->_player = o();
      o()->apply_action_effects( this );
    }
  }

  void init() override
  {
    T::init();

    if ( !this->player->sim->report_pets_separately )
    {
      auto it = range::find_if( totem->o()->pet_list,
          [ this ]( pet_t* pet ) { return this->player->name_str == pet->name_str; } );

      if ( it != totem->o()->pet_list.end() && this->player != *it )
      {
        this->stats = ( *it )->get_stats( this->name(), this );
      }
    }

    this->snapshot_flags = this->update_flags = this->snapshot_flags & ~( STATE_MUL_PET | STATE_TGT_MUL_PET );
  }

  shaman_t* o() const
  {
    return debug_cast<shaman_t*>( this->player->cast_pet()->owner );
  }

  shaman_td_t* td( player_t* target ) const
  {
    return o()->get_target_data( target );
  }

  double action_multiplier() const override
  {
    double m = T::action_multiplier();

    m *= pulse_multiplier;

    return m;
  }

  double action_da_multiplier() const override
  {
    double m = T::action_da_multiplier();

    if ( affected_by_totemic_rebound_da )
    {
      m *= 1.0 + o()->buff.totemic_rebound->stack_value();
    }

    return m;
  }

  double action_ta_multiplier() const override
  {
    double m = T::action_ta_multiplier();

    return m;
  }

  void execute() override
  {
    T::execute();

    pulse++;
  }

  void reset() override
  {
    T::reset();
    pulse_multiplier = 1.0;
    pulse = 0;
  }

  /// Reset the internal counters relating to totem pulsing
  void reset_pulse()
  {
    pulse_multiplier = 1.0;
    pulse = 0;
  }

};

template <typename T>
struct totem_pulse_event_t : public event_t
{
  shaman_totem_pet_t<T>* totem;
  timespan_t real_amplitude;

  totem_pulse_event_t( shaman_totem_pet_t<T>& t, timespan_t amplitude )
    : event_t( t ), totem( &t ), real_amplitude( amplitude )
  {
    if ( totem->pulse_action->hasted_pulse )
      real_amplitude *= totem->cache.spell_cast_speed();

    schedule( real_amplitude );
  }

  const char* name() const override
  { return "totem_pulse"; }

  void execute() override
  {
    if ( totem->pulse_action )
      totem->pulse_action->execute();

    totem->pulse_event = make_event<totem_pulse_event_t<T>>( sim(), *totem, totem->pulse_amplitude );
  }
};

template <typename T>
void shaman_totem_pet_t<T>::summon( timespan_t duration )
{
  pet_t::summon( duration );

  if ( this->pulse_action )
  {
    this->pulse_action->reset_pulse();
    this->pulse_event = make_event<totem_pulse_event_t<T>>( *sim, *this, this->pulse_amplitude );
  }

  if ( this->summon_pet )
    this->summon_pet->summon();
}

template <typename T>
void shaman_totem_pet_t<T>::dismiss( bool expired )
{
  if ( pulse_action && pulse_event && expired && pulse_on_expire )
  {
    auto e = debug_cast<totem_pulse_event_t<T>*>( pulse_event );
    if ( pulse_event->remains() > timespan_t::zero() && pulse_event->remains() != e->real_amplitude )
    {
      pulse_action->pulse_multiplier = ( e->real_amplitude - pulse_event->remains() ) / e->real_amplitude;
    }
    pulse_action->execute();
  }

  event_t::cancel( pulse_event );

  if ( summon_pet )
  {
    summon_pet->dismiss();
  }

  pet_t::dismiss( expired );
}

// Capacitor Totem =========================================================

struct capacitor_totem_pulse_t : public spell_totem_action_t
{
  cooldown_t* totem_cooldown;

  capacitor_totem_pulse_t( spell_totem_pet_t* totem )
    : spell_totem_action_t( "static_charge", totem, totem->find_spell( 118905 ) )
  {
    aoe   = 1;
    quiet = dual   = true;
    totem_cooldown = totem->o()->get_cooldown( "capacitor_totem" );
  }
};

struct capacitor_totem_t : public spell_totem_pet_t
{
  capacitor_totem_t( shaman_t* owner ) : spell_totem_pet_t( owner, "capacitor_totem" )
  {
    pulse_amplitude = owner->find_spell( 192058 )->duration();
    npc_id = 199672;
  }

  void init_spells() override
  {
    spell_totem_pet_t::init_spells();

    pulse_action = new capacitor_totem_pulse_t( this );
  }
};

// Healing Stream Totem =====================================================

struct healing_stream_totem_pulse_t : public heal_totem_action_t
{
  healing_stream_totem_pulse_t( heal_totem_pet_t* totem )
    : heal_totem_action_t( "healing_stream_totem_heal", totem, totem->find_spell( 52042 ) )
  { }
};

struct healing_stream_totem_t : public heal_totem_pet_t
{
  healing_stream_totem_t( shaman_t* owner ) :
    heal_totem_pet_t( owner, "healing_stream_totem" )
  {
    pulse_amplitude = owner->find_spell( 5672 )->effectN( 1 ).period();
    npc_id = 3527;
  }

  void init_spells() override
  {
    heal_totem_pet_t::init_spells();

    pulse_action = new healing_stream_totem_pulse_t( this );
  }
};

struct healing_stream_totem_spell_t : public shaman_totem_t<heal_totem_pet_t, shaman_heal_t>
{
  healing_stream_totem_spell_t( shaman_t* p, util::string_view options_str ) :
    shaman_totem_t<heal_totem_pet_t, shaman_heal_t>( "healing_stream_totem", p, options_str,
        p->find_spell( 5394 ), p->pet.healing_stream_totem )
  { }

  void execute() override
  {
    shaman_totem_t<heal_totem_pet_t, shaman_heal_t>::execute();

    if ( p()->spec.inundate->ok() )
    {
      p()->trigger_maelstrom_gain( p()->spell.inundate->effectN( 1 ).base_value(), p()->gain.inundate );
    }
  }
};

// Surging Totem ============================================================

struct surging_totem_pulse_t : public spell_totem_action_t
{
  unsigned variant;

  bool is_variant( spell_variant variant_ ) const
  { return variant & ( 1 << static_cast<unsigned>( variant_ ) ); }

  surging_totem_pulse_t( spell_totem_pet_t* totem,
    unsigned var_ = static_cast<unsigned>( spell_variant::NORMAL ) ) :
    spell_totem_action_t( ::action_name( "tremor", var_ ), totem,
      totem->find_spell( 455622 ) ), variant( var_ )
  {
    aoe          = -1;
    reduced_aoe_targets = as<double>( data().effectN( 2 ).base_value() );
    hasted_pulse = true;
  }

  double miss_chance( double hit, player_t* t ) const override
  {
    if ( is_variant( spell_variant::EARTHSURGE ) || o()->options.surging_totem_miss_chance == 0.0 )
    {
      return spell_totem_action_t::miss_chance( hit, t );
    }
    return o()->options.surging_totem_miss_chance;
  }

  void init() override
  {
    spell_totem_action_t::init();

    if ( !this->player->sim->report_pets_separately )
    {
      auto it = range::find_if( totem->o()->pet_list,
          [ this ]( pet_t* pet ) { return this->player->name_str == pet->name_str; } );

      if ( it != totem->o()->pet_list.end() && this->player != *it )
      {
        this->stats = ( *it )->get_stats( this->name(), this );
      }
    }
  }

  double action_multiplier() const override
  {
    double m = spell_totem_action_t::action_multiplier();

    if ( o()->buff.ascendance->up() && o()->talent.oversurge.ok() )
    {
      m *= 1.0 + o()->talent.oversurge->effectN( 2 ).percent();
    }

    if ( is_variant( spell_variant::EARTHSURGE ) )
    {
      m *= o()->talent.earthsurge->effectN( 1 ).percent();
    }

    return m;
  }
};

struct surging_bolt_t : public spell_totem_action_t
{
  surging_bolt_t( spell_totem_pet_t* totem )
    : spell_totem_action_t( "surging_bolt", totem, totem->find_spell( 458267 ) )
  {
    background = true;
  }

  void init() override
  {
    spell_totem_action_t::init();

    // Surging Bolt appears to be special and actually inherit guardian modifiers, so enable them
    snapshot_flags = update_flags = snapshot_flags | STATE_MUL_PET;
  }

  double action_da_multiplier() const override
  {
    auto m = spell_totem_action_t::action_da_multiplier();

    if ( o()->buff.ascendance->up() && o()->talent.oversurge.ok() )
    {
      m *= 1.0 + o()->talent.oversurge->effectN( 2 ).percent();
    }

    return m;
  }
};

struct surging_totem_t : public spell_totem_pet_t
{
  surging_bolt_t* surging_bolt;
  surging_totem_pulse_t* earthsurge;

  surging_totem_t( shaman_t* owner ) : spell_totem_pet_t( owner, "surging_totem" ),
    surging_bolt( nullptr ), earthsurge( nullptr )
  {
    pulse_amplitude = owner->find_spell(
      owner->specialization() == SHAMAN_ENHANCEMENT ? 455593 : 45594 )->effectN( 1 ).period();
    npc_id = 225409;
  }

  void trigger_surging_bolt( player_t* target )
  {
    if ( surging_bolt )
    {
      surging_bolt->execute_on_target( target );
    }
  }

  void trigger_earthsurge( player_t* target, double mul = 1.0 )
  {
    if ( earthsurge )
    {
      earthsurge->base_multiplier = mul;
      earthsurge->execute_on_target( target );
    }
  }

  void init_spells() override
  {
    spell_totem_pet_t::init_spells();

    pulse_action = new surging_totem_pulse_t( this );

    if ( o()->talent.earthsurge->ok() )
    {
      earthsurge = new surging_totem_pulse_t( this, variant_flag( spell_variant::EARTHSURGE ) );
    }

    if ( o()->talent.totemic_rebound.ok() )
    {
      surging_bolt = new surging_bolt_t( this );
    }
  }

  void summon( timespan_t duration ) override
  {
    spell_totem_pet_t::summon( duration );

    pulse_action->execute_on_target( target );
  }

  void demise() override
  {
    spell_totem_pet_t::demise();

    o()->buff.whirling_air->expire();
    o()->buff.whirling_fire->expire();
    o()->buff.whirling_earth->expire();
    o()->buff.totemic_rebound->expire();
    o()->buff.surging_totem->expire();
    o()->buff.amplification_core->expire();
  }
};

struct surging_totem_spell_t : public shaman_totem_t<spell_totem_pet_t, shaman_spell_t>
{
  surging_totem_spell_t( shaman_t* p, util::string_view options_str ) :
    shaman_totem_t<spell_totem_pet_t, shaman_spell_t>( "surging_totem", p, options_str,
        p->find_spell( 444995 ), p->pet.surging_totem )
  { }

  void execute() override
  {
    shaman_totem_t<spell_totem_pet_t, shaman_spell_t>::execute();

    p()->buff.surging_totem->trigger();
    p()->buff.amplification_core->trigger();
    p()->buff.whirling_air->trigger();
    p()->buff.whirling_fire->trigger();
    p()->buff.whirling_earth->trigger();

    if ( p()->talent.primal_catalyst.ok() )
    {
      p()->buff.elemental_overflow->trigger();
    }
  }

  bool ready() override
  {
    if ( !p()->talent.surging_totem.ok() )
    {
      return false;
    }

    return shaman_totem_t<spell_totem_pet_t, shaman_spell_t>::ready();
  }
};

// Searing Totem ============================================================

struct searing_totem_pulse_t : public spell_totem_action_t
{
  searing_totem_pulse_t( spell_totem_pet_t* totem )
    : spell_totem_action_t( "searing_bolt", totem, totem->find_spell( 3606 ) )
  {
    //hasted_pulse = true;
  }

  void init() override
  {
    spell_totem_action_t::init();

    if ( !this->player->sim->report_pets_separately )
    {
      auto it = range::find_if( totem->o()->pet_list,
          [ this ]( pet_t* pet ) { return this->player->name_str == pet->name_str; } );

      if ( it != totem->o()->pet_list.end() && this->player != *it )
      {
        this->stats = ( *it )->get_stats( this->name(), this );
      }
    }
  }
};

struct searing_volley_t : public spell_totem_action_t
{
  searing_volley_t( spell_totem_pet_t* totem )
    : spell_totem_action_t( "searing_volley", totem, totem->find_spell( 458147 ) )
  { }

  void init() override
  {
    spell_totem_action_t::init();

    if ( !this->player->sim->report_pets_separately )
    {
      auto it = range::find_if( totem->o()->pet_list,
          [ this ]( pet_t* pet ) { return this->player->name_str == pet->name_str; } );

      if ( it != totem->o()->pet_list.end() && this->player != *it )
      {
        this->stats = ( *it )->get_stats( this->name(), this );
      }
    }
  }
};

struct searing_totem_t : public spell_totem_pet_t
{
  searing_volley_t* volley;

  searing_totem_t( shaman_t* owner ) : spell_totem_pet_t( owner, "searing_totem" ), volley( nullptr )
  {
    pulse_amplitude = owner->find_spell( 3606 )->cast_time();
    npc_id = 2523;
  }

  void init_spells() override
  {
    spell_totem_pet_t::init_spells();

    pulse_action = new searing_totem_pulse_t( this );
  }

  void create_actions() override
  {
    spell_totem_pet_t::create_actions();

    volley = new searing_volley_t( this );
  }
};


// ==========================================================================
// PvP talents/abilities
// ==========================================================================

struct thundercharge_t : public shaman_spell_t
{
  thundercharge_t( shaman_t* player, util::string_view options_str ) :
    shaman_spell_t( "thundercharge", player, player->find_spell( 204366 ) )
  {
    parse_options( options_str );
    background = true;
    harmful    = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->buff.thundercharge->trigger();
  }
};

// ==========================================================================
// Primordial Storm
// ==========================================================================

struct primordial_storm_t : public shaman_spell_t
{
  struct primordial_damage_t : public shaman_attack_t
  {
    primordial_damage_t( primordial_storm_t* parent, util::string_view name, const spell_data_t* s,
      unsigned type_ ) :
      shaman_attack_t( ::action_name( name, type_ ), parent->p(), s, type_ )
    {
      background = true;

      aoe          = -1;
      reduced_aoe_targets = p()->talent.primordial_storm->effectN( 3 ).base_value();

      // Note, 11.2 totemic set bonus spells do not benefit from Maelstrom Weapon
      if ( is_variant( spell_variant::TWW3_SPELL ) )
      {
        base_multiplier *= p()->sets->set( HERO_TOTEMIC, TWW3, B2 )->effectN( 1 ).percent();
        may_proc_flametongue = may_proc_windfury = false;
      }

      // Inherit Maelstrom Weapon stacks from the parent cast for normal casts
      if ( is_variant( spell_variant::NORMAL ) )
      {
        mw_parent = parent;
      }
    }
  };

  primordial_damage_t* fire, *frost, *nature;

  primordial_storm_t( shaman_t* player, unsigned type_, util::string_view options_str = {} ) :
    shaman_spell_t( ::action_name( "primordial_storm", type_ ), player,
      player->find_spell( 1218090 ), type_ )
  {
    parse_options( options_str );

    fire = new primordial_damage_t( this, "primordial_fire",
      player->find_spell( 1218113 ), type_ );
    frost = new primordial_damage_t( this, "primordial_frost",
      player->find_spell( 1218116 ), type_ );
    nature = new primordial_damage_t( this, "primordial_lightning",
      player->find_spell( 1218118 ), type_ );

    add_child( fire );
    add_child( frost );
    add_child( nature );

    // Spell data does not indicate this, textual description does
    affected_by_maelstrom_weapon = true;

    if ( is_variant( spell_variant::TWW3_SPELL ) )
    {
      background = dual = true;
    }
  }

  void trigger_lightning_damage()
  {
    // Surging Totem-triggered Primordial Storm deos not proc the extra LB/CL cast
    if ( is_variant( spell_variant::TWW3_SPELL ) )
    {
      return;
    }

    shaman_spell_t* damage = nullptr;
    if ( fire->target_list().size() == 1 )
    {
      damage = debug_cast<shaman_spell_t*>( p()->action.lightning_bolt_ps );
    }
    else if ( fire->target_list().size() > 1 )
    {
      damage = debug_cast<shaman_spell_t*>( p()->action.chain_lightning_ps );
    }

    if ( damage == nullptr )
    {
      return;
    }

    make_event( sim, rng().gauss( 950_ms, 25_ms ),
      [ this, damage, t = execute_state->target ]() {
      if ( t->is_sleeping() )
      {
        return;
      }

      damage->mw_parent = this;
      damage->execute_on_target( t );
    } );
  }

  void execute() override
  {
    shaman_spell_t::execute();

    // Set targets early so we can use fire target list to figure out whether LB or CL can be shot,
    // before the fire damage spell executes.
    fire->set_target( execute_state->target );
    frost->set_target( execute_state->target );
    nature->set_target( execute_state->target );

    // Primordial Fire seems to execute instantly
    fire->execute();

    // Frost follows roughly 300ms later
    make_event( sim, rng().gauss( 300_ms, 20_ms ) ,
      [ this, t = execute_state->target ]() {
      if ( t->is_sleeping() )
      {
        return;
      }

      frost->execute();
    } );

    // Lightning follows roughly 600ms later
    make_event( sim, rng().gauss( 600_ms, 30_ms ),
      [ this, t = execute_state->target ]() {
      if ( t->is_sleeping() )
      {
        return;
      }

      nature->execute();
    } );

    // Triggered LB/CL follows roughly 950ms from initial cast
    trigger_lightning_damage();

    if ( is_variant( spell_variant::NORMAL ) )
    {
      p()->buff.primordial_storm->decrement();
    }

    // [BUG] 2025-02-24 Supercharge works on Primordial Storm in-game
    if ( p()->bugs && is_variant( spell_variant::NORMAL ) &&
         p()->specialization() == SHAMAN_ENHANCEMENT &&
         rng().roll( p()->talent.supercharge->effectN( 2 ).percent() ) )
    {
      p()->generate_maelstrom_weapon( this, as<int>( p()->talent.supercharge->effectN( 3 ).base_value() ) );
    }

    if ( p()->sets->has_set_bonus( HERO_TOTEMIC, TWW3, B4 ) )
    {
      p()->buff.elemental_overflow->trigger();
    }
  }

  bool ready() override
  {
    if ( !p()->buff.primordial_storm->check() )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

// ==========================================================================
// Tempest
// ==========================================================================

struct tempest_overload_t : public elemental_overload_spell_t
{
  tempest_overload_t( shaman_t* p, shaman_spell_t* parent_ )
    : elemental_overload_spell_t( p, "tempest_overload", p->find_spell( 463351 ), parent_ )
  {
    aoe = -1;
    // Blizzard forgot to apply Tempest's AOE soft cap hotfix to its overload spell
    // reduced_aoe_targets = as<double>( data().effectN( 3 ).base_value() );
    base_aoe_multiplier = data().effectN( 2 ).percent();
    affected_by_master_of_the_elements = true;
  }

  void impact( action_state_t* state ) override
  {
    elemental_overload_spell_t::impact( state );

    // Accumulate Lightning Rod damage from all targets hit by this cast.
    if ( p()->talent.lightning_rod.ok() || p()->talent.conductive_energy.ok() )
    {
      accumulate_lightning_rod_damage( state );
    }
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = shaman_spell_t::composite_target_multiplier( t );
    if ( p()->talent.inferno_arc.ok() )
    {
      m *= 1.0 + td( t )->dot.flame_shock->is_ticking() * p()->talent.inferno_arc->effectN( 1 ).percent();
    }

    return m;
  }

};

struct tempest_t : public shaman_spell_t
{
  storms_eye_t* storms_eye;

  tempest_t( shaman_t* player, unsigned type_, util::string_view options_str = {} ) :
    shaman_spell_t( ::action_name( "tempest", type_ ), player, player->find_spell( 452201 ), type_ )
  {
    parse_options( options_str );

    aoe = -1;
    reduced_aoe_targets = data().effectN( 3 ).base_value();
    base_aoe_multiplier = data().effectN( 2 ).percent();

    if ( player->mastery.elemental_overload->ok() )
    {
      overload = new tempest_overload_t( player, this );
    }

    if (p()->spell.tww3_stormbringer_4pc->ok())
    {
      storms_eye = new storms_eye_t( player );
    }

    if ( is_variant( spell_variant::THORIMS_INVOCATION ) )
    {
      background = true;
      base_execute_time = 0_s;
      base_costs[ RESOURCE_MANA ] = 0;
      if ( auto parent = p()->find_action( "thorims_invocation" ) )
      {
        parent->add_child( this );
      }
    }

    if ( is_variant( spell_variant::NORMAL ) )
    {
      affected_by_master_of_the_elements = true;
    }
  }

  int maelstrom_weapon_stacks() const override
  {
    if ( !benefit_from_maelstrom_weapon() )
    {
      return 0;
    }

    auto mw_stacks = std::min(
      as<int>( this->p()->spell.maelstrom_weapon->effectN( 2 ).base_value() ),
      this->p()->buff.maelstrom_weapon->check() );

    if ( is_variant( spell_variant::THORIMS_INVOCATION ) )
    {
      mw_stacks = std::min( mw_stacks,
        as<int>( this->p()->talent.thorims_invocation->effectN( 6 ).base_value() ) );
    }

    return mw_stacks;
  }

  void execute() override
  {
    if ( p()->buff.storms_eye->up())
    {
      make_event(sim, p()->find_spell(1235836)->duration(), [ this, t = target ]() {
        if ( t->is_sleeping() )
        {
          return;
        }

        storms_eye->execute_on_target( t );
      } );
    }

    if ( p()->buff.storm_elemental->check() && p()->talent.primal_elementalist.ok() )
    {
      p()->buff.wind_gust->trigger();
    }

    // Bug: If Tempest would apply a new Stormkeeper buff (so none was present beforehand), it'll generate Maelstrom
    auto original_maelstrom_gain = maelstrom_gain;
    auto original_maelstrom_gain_per_target = maelstrom_gain_per_target;
    bool buggy_maelstrom_gain = false;
    if ( p()->bugs && p()->specialization() == SHAMAN_ELEMENTAL && p()->talent.stormwell.ok() && p()->talent.arc_discharge.ok() && !p()->buff.stormkeeper->up() )
    {
      maelstrom_gain = p()->talent.stormwell->effectN(2).base_value();
      maelstrom_gain_per_target = false;
      buggy_maelstrom_gain = true;
    }

    p()->buff.tempest->decrement();

    shaman_spell_t::execute();

    if ( ( is_variant( spell_variant::NORMAL ) || is_variant( spell_variant::THORIMS_INVOCATION ) )
      && p()->specialization() == SHAMAN_ENHANCEMENT &&
      rng().roll( p()->talent.supercharge->effectN( 2 ).percent() ) )
    {
      p()->generate_maelstrom_weapon( execute_state->action,
                                      as<int>( p()->talent.supercharge->effectN( 3 ).base_value() ) );
    }

    // resetting maelstrom gain
    if ( p()->bugs && buggy_maelstrom_gain )
    {
      maelstrom_gain = original_maelstrom_gain;
      maelstrom_gain_per_target = original_maelstrom_gain_per_target;
    }

    if ( p()->talent.storm_swell.ok() )
    {
      p()->buff.storm_swell->trigger();
    }

    if ( p()->talent.arc_discharge.ok() )
    {
      if ( p()->specialization() == SHAMAN_ELEMENTAL )
      {
        p()->buff.stormkeeper->trigger(1);
      }
      else
      {
        p()->buff.arc_discharge->trigger();
      }
    }

    if ( p()->talent.thorims_invocation.ok() && is_variant( spell_variant::NORMAL ) )
    {
      if ( execute_state->n_targets == 1 )
      {
        p()->action.ti_trigger = p()->action.lightning_bolt_ti;
      }
      else if ( execute_state->n_targets > 1 )
      {
        p()->action.ti_trigger = p()->action.chain_lightning_ti;
      }
    }

    p()->buff.storms_eye->decrement();
  }

  void impact( action_state_t* state ) override
  {
    shaman_spell_t::impact( state );

    if ( p()->talent.conductive_energy.ok() )
    {
      accumulate_lightning_rod_damage( state );
    }

    if ( state->chain_target == 0 && p()->talent.conductive_energy.ok() )
    {
      trigger_lightning_rod_debuff( state->target );
    }
  }

  bool ready() override
  {
    if ( !p()->buff.tempest->check() )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }

  void schedule_travel(action_state_t* s) override
  {
    if ( s->chain_target == 0 )
    {
      if ( p()->buff.power_of_the_maelstrom->up() )
      {
        p()->proc.potm_tempest_overload->occur();
        trigger_elemental_overload( s, 1.0 );
        p()->buff.power_of_the_maelstrom->decrement();
        if ( p()->talent.fusion_of_elements->ok() )
        {
          p()->action.elemental_blast_foe->execute_on_target( s->target );
        }
      }

      if ( p()->talent.supercharge.ok() )
      {
        trigger_elemental_overload( s, p()->talent.supercharge->effectN( 1 ).percent() );
      }
      shaman_spell_t::schedule_travel( s );
    }
    else
    {
      // Tempest overloads only on primary target. While calling base_t here
      // is pretty ugly it's the only way we believe to be able to model this.
      base_t::schedule_travel( s );
    }
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = shaman_spell_t::composite_target_multiplier( t );
    if ( p()->talent.inferno_arc.ok() )
    {
      m *= 1.0 + td( t )->dot.flame_shock->is_ticking() * p()->talent.inferno_arc->effectN( 1 ).percent();
    }

    return m;
  }

};

// ==========================================================================
// Voltaic Blaze
// ==========================================================================

struct voltaic_blaze_t : public shaman_spell_t
{
  struct voltaic_blaze_damage_t : public shaman_spell_t
  {
    voltaic_blaze_damage_t( shaman_t* player ) :
      shaman_spell_t( "voltaic_blaze_damage", player, player->find_spell( 1259101 ) )
    {
      background = dual = true;
      stats = player->get_stats( "voltaic_blaze" );
      aoe = 1 + as<int>( player->talent.voltaic_blaze->effectN( 4 ).base_value() );
    }

    double composite_target_crit_chance( player_t* /* t */ ) const override
    { return 1.0; }

    void impact( action_state_t* state ) override
    {
      shaman_spell_t::impact( state );

      p()->trigger_secondary_flame_shock( state->target, spell_variant::VOLTAIC_BLAZE );
    }
  };

  voltaic_blaze_t( shaman_t* player, util::string_view options_str = {} ) :
    shaman_spell_t( "voltaic_blaze", player, player->talent.voltaic_blaze )
  {
    parse_options( options_str );

    impact_action = new voltaic_blaze_damage_t( player );
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p()->generate_maelstrom_weapon( execute_state, as<int>( data().effectN( 2 ).base_value() ) );

    if ( p()->talent.fire_nova.ok() && rng().roll( p()->talent.fire_nova->effectN( 2 ).percent() ) &&
        !p()->action.fire_nova->target_list().empty() )
    {
      p()->action.fire_nova->execute_on_target( execute_state->target );
    }

    if ( p()->talent.routine_communication.ok() && p()->rng_obj.routine_communication->trigger() )
    {
      p()->summon_ancestor();
    }

    if ( p() ->talent.purging_flames.ok() )
    {
      p()->buff.purging_flames->trigger();
    }

    p()->trigger_lively_totems( execute_state );
  }
};

// ==========================================================================
// Shaman Custom Buff implementation
// ==========================================================================

void ascendance_buff_t::ascendance( attack_t* mh, attack_t* oh )
{
  // Presume that ascendance trigger and expiration will not reset the swing
  // timer, so we need to cancel and reschedule autoattack with the
  // remaining swing time of main/off hands
  if ( player->specialization() == SHAMAN_ENHANCEMENT )
  {
    bool executing         = false;
    timespan_t time_to_hit = timespan_t::zero();
    if ( player->main_hand_attack && player->main_hand_attack->execute_event )
    {
      executing   = true;
      time_to_hit = player->main_hand_attack->execute_event->remains();
#ifndef NDEBUG
      if ( time_to_hit < timespan_t::zero() )
      {
        fmt::print( stderr, "Ascendance {} time_to_hit={}", player->main_hand_attack->name(), time_to_hit );
        assert( 0 );
      }
#endif
      event_t::cancel( player->main_hand_attack->execute_event );
    }

    if ( sim->debug )
    {
      sim->out_debug.print( "{} ascendance swing timer for main-hand, executing={}, time_to_hit={}",
                            player->name(), executing, time_to_hit );
    }

    player->main_hand_attack = mh;
    if ( executing )
    {
      // Kick off the new main hand attack, by instantly scheduling
      // and rescheduling it to the remaining time to hit. We cannot use
      // normal reschedule mechanism here (i.e., simply use
      // event_t::reschedule() and leave it be), because the rescheduled
      // event would be triggered before the full swing time (of the new
      // auto attack) in most cases.
      player->main_hand_attack->base_execute_time = timespan_t::zero();
      player->main_hand_attack->schedule_execute();
      player->main_hand_attack->base_execute_time = player->main_hand_attack->weapon->swing_time;
      if ( player->main_hand_attack->execute_event )
      {
        player->main_hand_attack->execute_event->reschedule( time_to_hit );
      }
    }

    if ( player->off_hand_attack )
    {
      time_to_hit = timespan_t::zero();
      executing   = false;

      if ( player->off_hand_attack->execute_event )
      {
        executing   = true;
        time_to_hit = player->off_hand_attack->execute_event->remains();
#ifndef NDEBUG
        if ( time_to_hit < timespan_t::zero() )
        {
          fmt::print( stderr, "Ascendance {} time_to_hit={}", player->off_hand_attack->name(), time_to_hit );
          assert( 0 );
        }
#endif
        event_t::cancel( player->off_hand_attack->execute_event );
      }

      if ( sim->debug )
      {
        sim->out_debug.print( "{} ascendance swing timer for off-hand, executing={}, time_to_hit={}",
                              player->name(), executing, time_to_hit );
      }

      player->off_hand_attack = oh;
      if ( executing )
      {
        // Kick off the new off hand attack, by instantly scheduling
        // and rescheduling it to the remaining time to hit. We cannot use
        // normal reschedule mechanism here (i.e., simply use
        // event_t::reschedule() and leave it be), because the rescheduled
        // event would be triggered before the full swing time (of the new
        // auto attack) in most cases.
        player->off_hand_attack->base_execute_time = timespan_t::zero();
        player->off_hand_attack->schedule_execute();
        player->off_hand_attack->base_execute_time = player->off_hand_attack->weapon->swing_time;
        if ( player->off_hand_attack->execute_event )
        {
          player->off_hand_attack->execute_event->reschedule( time_to_hit );
        }
      }
    }
  }
  // Elemental simply resets the Lava Burst cooldown, Lava Beam replacement
  // will be handled by action list and ready() in Chain Lightning / Lava
  // Beam
  else if ( player->specialization() == SHAMAN_ELEMENTAL )
  {
    if ( lava_burst )
    {
      lava_burst->cooldown->reset( false );
    }
  }
}

inline bool ascendance_buff_t::trigger( int stacks, double value, double chance, timespan_t duration )
{
  shaman_t* p = debug_cast<shaman_t*>( player );

  if ( player->specialization() == SHAMAN_ELEMENTAL && !lava_burst )
  {
    lava_burst = player->find_action( "lava_burst" );
  }

  ascendance( p->ascendance_mh, p->ascendance_oh );
  // Don't record CD waste during Ascendance.
  if ( lava_burst )
  {
    lava_burst->cooldown->last_charged = timespan_t::zero();
  }

  buff_t::trigger( stacks, value, chance, duration );

  p->cooldown.strike->adjust_recharge_multiplier();

  if ( p->talent.descending_skies.ok() )
  {
    p->buff.tempest->trigger();
  }

  if ( !p->buff.static_accumulation->check() )
  {
    p->buff.static_accumulation->trigger();
  }

  return true;
}

inline void ascendance_buff_t::expire_override( int expiration_stacks, timespan_t remaining_duration )
{
  shaman_t* p = debug_cast<shaman_t*>( player );

  ascendance( p->melee_mh, p->melee_oh );

  // Start CD waste recollection from when Ascendance buff fades, since Lava
  // Burst is guaranteed to be very much ready when Ascendance ends.
  if ( lava_burst )
  {
    lava_burst->cooldown->last_charged = sim->current_time();
  }
  buff_t::expire_override( expiration_stacks, remaining_duration );

  p->cooldown.strike->adjust_recharge_multiplier();

  if ( p->buff.static_accumulation->check() && !p->buff.doom_winds->check() )
  {
    p->buff.static_accumulation->expire();
  }

}

// ==========================================================================
// Shaman Character Definition
// ==========================================================================

// shaman_t::trigger_secondary_ability ======================================

void shaman_t::trigger_secondary_ability( const action_state_t* source_state, action_t* secondary_action,
                                          bool inherit_state )
{
  auto secondary_state = secondary_action->get_state( inherit_state ? source_state : nullptr );
  // Snapshot the state if no inheritance is defined
  if ( !inherit_state )
  {
    secondary_state->target = source_state->target;
    secondary_action->snapshot_state( secondary_state, secondary_action->amount_type( secondary_state ) );
  }

  secondary_action->schedule_execute( secondary_state );
}

// shaman_t::create_action  =================================================

action_t* shaman_t::create_action( util::string_view name, util::string_view options_str )
{
  // shared
  if ( name == "ascendance" )
    return new ascendance_t( this, "ascendance", options_str );
  if ( name == "auto_attack" )
    return new auto_attack_t( this, options_str );
  if ( name == "bloodlust" )
    return new bloodlust_t( this, options_str );
  if ( name == "capacitor_totem" )
    return new shaman_totem_t<spell_totem_pet_t, shaman_spell_t>( "capacitor_totem",
        this, options_str, talent.capacitor_totem, pet.capacitor_totem );
  if ( name == "elemental_blast" )
    return new elemental_blast_t( this, variant_flag( spell_variant::NORMAL ), options_str );
  if ( name == "flame_shock" )
    return new flame_shock_t( this, variant_flag( spell_variant::NORMAL ), options_str );
  if ( name == "frost_shock" )
    return new frost_shock_t( this, options_str );
  if ( name == "ghost_wolf" )
    return new ghost_wolf_t( this, options_str );
  if ( name == "lightning_bolt" )
    return new lightning_bolt_t( this, variant_flag( spell_variant::NORMAL ), options_str );
  if ( name == "chain_lightning" )
    return new chain_lightning_t( this, options_str );
  if ( name == "stormkeeper" )
    return new stormkeeper_t( this, options_str );
  if ( name == "wind_shear" )
    return new wind_shear_t( this, options_str );
  if ( name == "healing_stream_totem" )
    return new healing_stream_totem_spell_t( this, options_str );
  if ( name == "earth_shield" )
    return new earth_shield_t( this, options_str );
  if ( name == "natures_swiftness" )
    return new natures_swiftness_t( this, options_str );
  if ( name == "tempest" )
    return new tempest_t( this, variant_flag( spell_variant::NORMAL ), options_str );

  // elemental

  if ( name == "earth_elemental" )
    return new earth_elemental_t( this, options_str );
  if ( name == "earth_shock" )
    return new earth_shock_t( this, options_str );
  if ( name == "earthquake" )
    return new earthquake_t( this, options_str );
  if ( name == "fire_elemental" )
    return new fire_elemental_t( this, options_str );
  if ( name == "lava_burst" )
    return new lava_burst_t( this, variant_flag( spell_variant::NORMAL ), options_str );
  if ( name == "ancestral_guidance" )
    return new ancestral_guidance_t( this, options_str );
  if ( name == "thunderstorm" )
    return new thunderstorm_t( this, options_str );

  // enhancement
  if ( name == "crash_lightning" )
    return new crash_lightning_t( this, options_str );
  if ( name == "feral_lunge" )
    return new feral_lunge_t( this, options_str );
  if ( name == "flametongue_weapon" )
    return new flametongue_weapon_t( this, options_str );
  if ( name == "windfury_weapon" )
    return new windfury_weapon_t( this, options_str );
  if ( name == "lava_lash" )
    return new lava_lash_t( this, variant_flag( spell_variant::NORMAL ), options_str );
  if ( name == "lightning_shield" )
    return new lightning_shield_t( this, options_str );
  if ( name == "spirit_walk" )
    return new spirit_walk_t( this, options_str );
  if ( name == "stormstrike" )
    return new stormstrike_t( this, options_str );
  if ( name == "sundering" )
    return new sundering_t( this, options_str );
  if ( name == "windstrike" )
    return new windstrike_t( this, options_str );
  if ( util::str_compare_ci( name, "thundercharge" ) )
    return new thundercharge_t( this, options_str );
  if ( name == "doom_winds" )
    return new doom_winds_t( this, variant_flag( spell_variant::NORMAL ), options_str );
  if ( name == "voltaic_blaze" )
    return new voltaic_blaze_t( this, options_str );
  if ( name == "primordial_storm" )
    return new primordial_storm_t( this, variant_flag( spell_variant::NORMAL ), options_str );

  // restoration
  if ( name == "spiritwalkers_grace" )
    return new spiritwalkers_grace_t( this, options_str );
  if ( name == "chain_heal" )
    return new chain_heal_t( this, options_str );
  if ( name == "healing_rain" )
    return new healing_rain_t( this, options_str );
  if ( name == "healing_surge" )
    return new healing_surge_t( this, options_str );
  if ( name == "healing_wave" )
    return new healing_wave_t( this, options_str );
  if ( name == "riptide" )
    return new riptide_t( this, options_str );

  // Hero talents
  if ( name == "surging_totem" )
    return new surging_totem_spell_t( this, options_str );
  if ( name == "thunderstrike_ward" )
    return new thunderstrike_ward_t( this, options_str );
  if ( name == "ancestral_swiftness" )
    return new ancestral_swiftness_t( this, options_str );

  return parse_player_effects_t::create_action( name, options_str );
}

// shaman_t::create_pet =====================================================

pet_t* shaman_t::create_pet( util::string_view pet_name, util::string_view /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p )
    return p;

  return nullptr;
}

// shaman_t::create_pets ====================================================

void shaman_t::create_pets()
{
  parse_player_effects_t::create_pets();
}

// shaman_t::create_expression ==============================================

std::unique_ptr<expr_t> shaman_t::create_expression( util::string_view name )
{
  if ( util::str_compare_ci( name, "rolling_thunder.next_tick" ) )
  {
    return make_fn_expr( name, [ this ]() {
      return rt_last_trigger + timespan_t::from_seconds( talent.rolling_thunder->effectN( 1 ).base_value()) - sim->current_time();
    } );
  }

  if ( util::str_compare_ci( name, "total_awaken_count" ) )
    return make_fn_expr( name, [ this ]() { return as<double>( aws_counter ); } );

  if ( auto expr = rng_obj.deeply_rooted_elements.create_expression( name ) )
  {
    return expr;
  }

  if ( auto expr = rng_obj.tempest_enh.create_expression( name ) )
  {
    return expr;
  }

  if (auto expr = rng_obj.tempest_ele.create_expression( name ) )
  {
    return expr;
  }

  if ( util::str_compare_ci( name, "tww3_procs_to_asc" ) )
    return make_fn_expr( name, [ this ]() {
      if ( !spell.tww3_stormbringer_2pc->ok() )
        return 0.0;
      unsigned int tww3_mod_value = static_cast<unsigned int>( specialization() == SHAMAN_ELEMENTAL
                                                      ? spell.tww3_stormbringer_2pc->effectN( 3 ).base_value()
                                                      : spell.tww3_stormbringer_2pc->effectN( 4 ).base_value() );
      return as<double>( tww3_mod_value-(aws_counter % tww3_mod_value) ); } );

  auto splits = util::string_split<util::string_view>( name, "." );

  if ( util::str_compare_ci( splits[ 0 ], "feral_spirit" ) )
  {
    if ( !talent.feral_spirit.ok() )
    {
      return expr_t::create_constant( splits[ 0 ], 0 );
    }

    if ( talent.feral_spirit.ok() && !find_action( "feral_spirit" ) )
    {
      return expr_t::create_constant( name, 0 );
    }

    if ( util::str_compare_ci( splits[ 1 ], "active" ) )
    {
      return make_fn_expr( name, [ this ]() {
        return as<double>( pet.all_wolves.size() );
      } );
    }
    else if ( util::str_compare_ci( splits[ 1 ], "remains" ) )
    {
      auto max_remains_fn = []( const pet_t* l, const pet_t* r ) {
        if ( !l->expiration && r->expiration )
        {
          return true;
        }
        else if ( l->expiration && !r->expiration )
        {
          return false;
        }
        else if ( !l->expiration && !r->expiration )
        {
          return false;
        }
        else
        {
          return l->expiration->remains() < r->expiration->remains();
        }
      };

      return make_fn_expr( name, [ this, &max_remains_fn ]() {
        auto it = std::max_element( pet.all_wolves.cbegin(), pet.all_wolves.cend(), max_remains_fn );
        if ( it == pet.all_wolves.end() )
          {
            return 0.0;
          }

          return ( *it )->expiration ? ( *it )->expiration->remains().total_seconds() : 0.0;
      } );
    }
  }

  if ( util::str_compare_ci( splits[ 0 ], "ti_lightning_bolt" ) )
  {
    return make_fn_expr( name, [ this ]() {
        return !action.ti_trigger || action.ti_trigger == action.lightning_bolt_ti ||
               action.ti_trigger == action.tempest_ti;
    } );
  }

  if ( util::str_compare_ci( splits[ 0 ], "ti_chain_lightning" ) )
  {
    return make_fn_expr( name, [ this ]() {
        return action.ti_trigger == action.chain_lightning_ti;
    } );
  }

  if ( util::str_compare_ci( splits[ 0 ], "rotation" ) )
  {
    auto rotation_type = parse_rotation( splits[ 1 ] );
    if ( rotation_type == ROTATION_INVALID )
    {
      throw sc_invalid_apl_argument(
        fmt::format( "Invalid rotation type {}, available values: {}", splits[ 1 ], rotation_options() ) );
    }

    return expr_t::create_constant( name, rotation_type == options.rotation );
  }

  if ( util::str_compare_ci( splits[ 0 ], "windfury_chance" ) )
  {
    return make_fn_expr( splits[ 0 ], [ this ]() {
      return std::min( 1.0, windfury_proc_chance() );
    } );
  }

  if ( util::str_compare_ci( splits[ 0 ], "lashing_flames" ) )
  {
    return make_ref_expr( splits[ 0 ], buff_state_lashing_flames );
  }

  if ( util::str_compare_ci( splits[ 0 ], "lightning_rod" ) )
  {
    return make_ref_expr( splits[ 0 ], buff_state_lightning_rod );
  }

  return parse_player_effects_t::create_expression( name );
}

// shaman_t::create_actions =================================================

void shaman_t::create_actions()
{
  parse_player_effects_t::create_actions();

  windfury_mh = new windfury_attack_t( "windfury_attack", this, find_spell( 25504 ), &( main_hand_weapon ) );
  flametongue = new flametongue_weapon_spell_t( "flametongue_attack", this,
      specialization() == SHAMAN_ENHANCEMENT
      ? &( off_hand_weapon )
      : &( main_hand_weapon ) );

  if ( talent.voltaic_blaze.ok() )
  {
    action.fire_nova = new fire_nova_t( this, variant_flag( spell_variant::NORMAL ) );
  }

  if ( talent.deeply_rooted_elements.ok() ||
       ( talent.ascendance.ok() && specialization() == SHAMAN_ENHANCEMENT ) )
  {
    action.doom_winds_asc = new doom_winds_t( this, variant_flag( spell_variant::ASCENDANCE ) );
  }

  if ( talent.crash_lightning.ok() )
  {
    action.crash_lightning_aoe = new crash_lightning_attack_t( this );
  }

  if ( talent.storm_unleashed_3.ok() )
  {
    action.crash_lightning_unleashed = new crash_lightning_unleashed_t( this );
  }

  if ( talent.stormblast.ok() )
  {
    dummy.stormblast = new dummy_action_t( this, talent.stormblast, "stormblast" );
  }

  if ( specialization() == SHAMAN_ELEMENTAL && ( talent.ascendance.ok() || talent.deeply_rooted_elements.ok() ) )
  {
    action.flame_shock_asc = new flame_shock_t( this, variant_flag( spell_variant::ASCENDANCE ) );
  }

    if ( talent.voltaic_blaze.ok() )
  {
    action.flame_shock_vb = new flame_shock_t( this, variant_flag( spell_variant::VOLTAIC_BLAZE ) );
  }

  if ( talent.thorims_invocation.ok() )
  {
    dummy.thorims_invocation = new dummy_action_t( this,
      talent.thorims_invocation, "thorims_invocation" );
    action.lightning_bolt_ti = new lightning_bolt_t( this,
      variant_flag( spell_variant::THORIMS_INVOCATION ) );
    action.tempest_ti = new tempest_t( this, variant_flag( spell_variant::THORIMS_INVOCATION ) );
    action.chain_lightning_ti = new chain_lightning_t( this, talent.chain_lightning,
      variant_flag( spell_variant::THORIMS_INVOCATION ) );
  }

  if ( talent.lightning_rod.ok() || talent.conductive_energy.ok() )
  {
    action.lightning_rod = new lightning_rod_damage_t( this );
  }

  if ( talent.deeply_rooted_elements.ok() )
  {
    action.dre_ascendance = new ascendance_dre_t( this,
      variant_flag( spell_variant::DEEPLY_ROOTED_ELEMENTS ) );
  }

  if ( spell.tww3_stormbringer_2pc->ok() )
  {
    action.set_ascendance = new ascendance_dre_t( this, variant_flag( spell_variant::TWW3_SPELL ) );
  }

  if ( sets->has_set_bonus( HERO_TOTEMIC, TWW3, B2 ) && specialization() == SHAMAN_ENHANCEMENT )
  {
    action.tww3_primordial_storm = new primordial_storm_t( this,
      variant_flag( spell_variant::TWW3_SPELL ) );
  }

  if ( ( talent.primal_catalyst.ok() || sets->has_set_bonus( HERO_TOTEMIC, TWW3, B4 ) ) &&
    specialization() == SHAMAN_ENHANCEMENT )
  {
    action.tww3_lava_lash = new lava_lash_t( this, variant_flag( spell_variant::TWW3_SPELL ) );
    action.tww3_fire_nova = new fire_nova_t( this, variant_flag( spell_variant::TWW3_SPELL ) );
  }

  if ( talent.stormflurry.ok() )
  {
    action.stormflurry_ss = new stormstrike_t( this, "", strike_variant::STORMFLURRY );
    action.stormflurry_ws = new windstrike_t( this, "", strike_variant::STORMFLURRY );
  }

  if ( talent.fusion_of_elements.ok() )
  {
    action.elemental_blast_foe = new elemental_blast_t( this,
      variant_flag( spell_variant::FUSION_OF_ELEMENTS ) );
  }

  if ( talent.thunderstrike_ward.ok() )
  {
    action.thunderstrike_ward = new thunderstrike_ward_damage_t( this );
  }

  if ( talent.earthen_rage.ok() )
  {
    action.earthen_rage = new earthen_rage_damage_t( this );
  }

  if ( talent.arc_discharge.ok() && specialization() == SHAMAN_ENHANCEMENT )
  {
    dummy.arc_discharge = new dummy_action_t( this, talent.arc_discharge, "arc_discharge" );
    action.chain_lightning_ad = new chain_lightning_t( this, talent.chain_lightning,
      variant_flag( spell_variant::ARC_DISCHARGE ) );
    if ( talent.ride_the_lightning.ok() )
    {
      action.chain_lightning_rtl_ad = new chain_lightning_t( this, find_spell( 211094 ),
        variant_flag( spell_variant::ARC_DISCHARGE, spell_variant::RIDE_THE_LIGHTNING ) );
    }
  }

  if ( talent.imbuement_mastery.ok() )
  {
    action.imbuement_mastery = new imbuement_mastery_t( this );
  }

  if ( talent.splitstream.ok() )
  {
    action.splitstream = new sundering_splitstream_t( this );
  }


  if ( talent.primordial_storm.ok() )
  {
    action.lightning_bolt_ps = new lightning_bolt_t( this,
      variant_flag( spell_variant::PRIMORDIAL_STORM ) );
    action.chain_lightning_ps = new chain_lightning_t( this, talent.chain_lightning,
      variant_flag( spell_variant::PRIMORDIAL_STORM ) );
  }

  if ( talent.ride_the_lightning.ok() )
  {
    dummy.ride_the_lightning = new dummy_action_t( this,
      talent.ride_the_lightning, "ride_the_lightning" );
    action.chain_lightning_ll_rtl = new chain_lightning_t( this, find_spell( 211094 ),
      variant_flag( spell_variant::RIDE_THE_LIGHTNING ), "chain_lightning_ll" );
    action.chain_lightning_ss_rtl = new chain_lightning_t( this, find_spell( 211094 ),
      variant_flag( spell_variant::RIDE_THE_LIGHTNING ), "chain_lightning_ss" );
    action.chain_lightning_ws_rtl = new chain_lightning_t( this, find_spell( 211094 ),
      variant_flag( spell_variant::RIDE_THE_LIGHTNING ), "chain_lightning_ws" );
  }

  if ( talent.purging_flames.ok() )
    action.lava_burst_pf = new lava_burst_t( this, variant_flag( spell_variant::PURGING_FLAMES ) );

  // Generic Actions
  action.flame_shock = new flame_shock_t( this, variant_flag( spell_variant::NORMAL ) );
  action.flame_shock->background = true;
  action.flame_shock->cooldown = get_cooldown( "flame_shock_secondary" );
  action.flame_shock->base_costs[ RESOURCE_MANA ] = 0;

  if ( talent.deeply_rooted_elements.ok() )
  {
    dummy.deeply_rooted_elements = new dummy_action_t( this, talent.deeply_rooted_elements,
      "deeply_rooted_elements" );
    action.ascendance_damage = new ascendance_damage_t( this, "ascendance_damage" );

    dummy.deeply_rooted_elements->add_child( action.ascendance_damage );
  }
  else if ( talent.ascendance.ok() && action.ascendance)
  {
    action.ascendance_damage = new ascendance_damage_t( this, "ascendance_damage" );
    action.ascendance->add_child( action.ascendance_damage );
  }

  if ( specialization() == SHAMAN_ENHANCEMENT )
  {
    action.doom_winds = new doom_winds_damage_t( this, variant_flag( spell_variant::NORMAL ) );
  }
}

// shaman_t::create_options =================================================

void shaman_t::create_options()
{
  parse_player_effects_t::create_options();
  add_option( opt_bool( "raptor_glyph", raptor_glyph ) );
  // option allows Shamans to switch to a different APL
  add_option( opt_func( "rotation", [ this ]( sim_t*, util::string_view, util::string_view val ) {
    options.rotation = parse_rotation( val );
    if ( options.rotation == ROTATION_INVALID )
    {
      throw sc_invalid_player_argument( fmt::format( "Available options: {}", rotation_options() ) );
    }

    return true;
  } ) );

  add_option( opt_obsoleted( "shaman.chain_harvest_allies" ) );
  add_option( opt_obsoleted( "shaman.dre_flat_chance" ) );
  add_option( opt_obsoleted( "shaman.dre_forced_failures" ) );

  add_option( opt_uint( "shaman.ancient_fellowship_positive", options.ancient_fellowship_positive, 0U, 100U ) );
  add_option( opt_uint( "shaman.ancient_fellowship_total", options.ancient_fellowship_total, 0U, 100U ) );

  add_option( opt_uint( "shaman.routine_communication_positive", options.routine_communication_positive, 0U, 100U ) );
  add_option( opt_uint( "shaman.routine_communication_total", options.routine_communication_total, 0U, 100U ) );

  add_option( opt_float( "shaman.thunderstrike_ward_proc_chance", options.thunderstrike_ward_proc_chance,
                         0.0, 1.0 ) );

  add_option( opt_float( "shaman.earthquake_spell_power_coefficient", options.earthquake_spell_power_coefficient, 0.0, 100.0 ) );

  add_option( opt_float( "shaman.imbuement_mastery_base_chance", options.imbuement_mastery_base_chance, 0.0, 1.0 ) );

  add_option( opt_obsoleted( "shaman.dre_enhancement_base_chance" ) );
  add_option( opt_obsoleted( "shaman.dre_enhancement_forced_failures" ) );

  add_option( opt_float( "shaman.lively_totems_base_chance", options.lively_totems_base_chance, 0.0, 1.0 ) );

  add_option( opt_float( "shaman.surging_totem_miss_chance", options.surging_totem_miss_chance, 0.0, 1.0 ) );

  add_option( opt_float( "shaman.chain_lightning_target_rng",
    options.chain_lightning_target_rng, 0.0, 1.0 ) );

  add_option( opt_int( "shaman.tww3_farseer_set", options.tww3_farseer_set, 0, 4 ) );
  add_option( opt_int( "shaman.tww3_stormbringer_set", options.tww3_stormbringer_set, 0, 4 ) );

  add_option( opt_float( "shaman.crash_lightning_su_hit_chance",
    options.crash_lightning_su_hit_chance, 0.0 , 1.0 ) );

  rng_obj.tempest_enh.create_options();
  rng_obj.tempest_ele.create_options();
  rng_obj.deeply_rooted_elements.create_options();
  rng_obj.asc_dw.create_options();
  rng_obj.storm_unleashed.create_options();
}

// shaman_t::create_profile ================================================

std::string shaman_t::create_profile( save_e save_type )
{
  std::string profile = parse_player_effects_t::create_profile( save_type );

  if ( save_type & SAVE_PLAYER )
  {
    if ( options.rotation == ROTATION_SIMPLE )
      profile += "rotation=simple\n";
  }

  return profile;
}

// shaman_t::copy_from =====================================================

void shaman_t::copy_from( player_t* source )
{
  parse_player_effects_t::copy_from( source );

  shaman_t* p  = debug_cast<shaman_t*>( source );

  raptor_glyph = p->raptor_glyph;
  options.rotation = p->options.rotation;
  options.earthquake_spell_power_coefficient = p->options.earthquake_spell_power_coefficient;

  options.ancient_fellowship_positive = p->options.ancient_fellowship_positive;
  options.ancient_fellowship_total = p->options.ancient_fellowship_total;
  options.routine_communication_positive = p->options.routine_communication_positive;
  options.routine_communication_total = p->options.routine_communication_total;

  options.thunderstrike_ward_proc_chance = p->options.thunderstrike_ward_proc_chance;
  options.imbuement_mastery_base_chance = p->options.imbuement_mastery_base_chance;
  options.lively_totems_base_chance = p->options.lively_totems_base_chance;

  options.surging_totem_miss_chance = p->options.surging_totem_miss_chance;

  options.chain_lightning_target_rng = p->options.chain_lightning_target_rng;
}

// shaman_t::create_special_effects ========================================

struct maelstrom_weapon_cb_t : public dbc_proc_callback_t
{
  shaman_t* shaman;

  maelstrom_weapon_cb_t( const special_effect_t& effect ) :
    dbc_proc_callback_t( effect.player, effect ), shaman( debug_cast<shaman_t*>( effect.player ) )
  { }

  // Fully override trigger + execute behavior of the proc
  void trigger( const proc_data_t&, player_t*, action_state_t* state, proc_trigger_type_e ) override
  {
    auto override_state = shaman->get_mw_proc_state( state->action );
    assert( override_state != mw_proc_state::DEFAULT );

    if ( override_state == mw_proc_state::DISABLED )
    {
      return;
    }

    if ( shaman->buff.ghost_wolf->check() )
    {
      return;
    }

    auto triggered = rng().roll( proc_chance );

    if ( listener->sim->debug )
    {
      listener->sim->print_debug( "{} attempts to proc {} on {}: {:d}", listener->name(),
          effect, state->action->name(), triggered );
    }

    if ( triggered )
    {
      shaman->generate_maelstrom_weapon( state );
      //shaman->buff.maelstrom_weapon->increment();
    }
  }
};

void shaman_t::create_special_effects()
{
  parse_player_effects_t::create_special_effects();

  if ( talent.maelstrom_weapon->ok() )
  {
    auto mw_effect = new special_effect_t( this );
    mw_effect->spell_id = talent.maelstrom_weapon->id();
    mw_effect->proc_flags2_ = PF2_ALL_HIT;

    special_effects.push_back( mw_effect );

    new maelstrom_weapon_cb_t( *mw_effect );
  }
}

// shaman_t::create_proc_action ============================================

action_t* shaman_t::create_proc_action( util::string_view name, const special_effect_t& effect )
{
  if ( effect.spell_id == 469927 )
  {
    struct quick_strike_t : public shaman_attack_t
    {
      quick_strike_t( shaman_t* p, const special_effect_t& effect ) :
        shaman_attack_t( "quick_strike", p, p->find_spell( 469928 ) )
      {
        background = true;
        base_dd_min = base_dd_max = effect.driver()->effectN( 1 ).average( effect );
      }

      void init() override
      {
        shaman_attack_t::init();

        may_proc_flametongue = false;
        may_proc_windfury = false;
      }
    };

    return new quick_strike_t( this, effect );
  }
  return parse_player_effects_t::create_proc_action( name, effect );
}

// shaman_t::action_init_finished ==========================================

void shaman_t::action_init_finished( action_t& action )
{
  // Always initialize Maelstrom Weapon proc state for the action
  set_mw_proc_state( action, mw_proc_state::DEFAULT );

  // Enable Maelstrom Weapon proccing for selected abilities, if they
  // fulfill the basic conditions for the proc
  if ( ( talent.maelstrom_weapon.ok() ) && action.callbacks &&
       get_mw_proc_state( action ) == mw_proc_state::DEFAULT && (
         // Auto-attacks (shaman-module convention to set mh to action id 1, oh to id 2)
         ( action.id == 1 || action.id == 2 ) ||
         // Actions with spell data associated
         ( action.data().id() != 0 &&
           !action.data().flags( spell_attribute::SX_SUPPRESS_CASTER_PROCS ) &&
           action.data().dmg_class() == 2U )
       ) )
  {
    set_mw_proc_state( action, mw_proc_state::ENABLED );
  }

  // Explicitly disable any action from proccing Maelstrom Weapon that does not have
  // it set enabled (above), or had its state adjusted to enabled or disabled during
  // action initialization.
  if ( get_mw_proc_state( action ) == mw_proc_state::DEFAULT )
  {
    set_mw_proc_state( action, mw_proc_state::DISABLED );
  }
}

void shaman_t::analyze( sim_t& sim )
{
  parse_player_effects_t::analyze( sim );

  int iterations = collected_data.total_iterations > 0
    ? collected_data.total_iterations
    : sim.iterations;

  if ( iterations > 1 )
  {
    // Re-use MW stack containers to report iteration average of stacks generated
    range::for_each( mw_source_list, [ iterations ]( auto& container ) {
      auto sum_actual = container.first.sum();
      auto sum_overflow = container.second.sum();

      container.first.reset();
      container.first.add( sum_actual / as<double>( iterations ) );

      container.second.reset();
      container.second.add( sum_overflow / as<double>( iterations ) );
    } );

    // Re-use MW spend containers to report iteration average over stacks consumed
    range::for_each( mw_spend_list, [ iterations ]( auto& container_wrapper ) {
      range::for_each( container_wrapper, [ idx = 0, iterations ]( auto& container ) mutable {
        auto sum = container.sum();
        auto count = container.count();

        container.reset();
        // 0-stack MW casts are just the count divided by iterations, not the sum
        if ( idx++ == 0 )
        {
          container.add( count / as<double>( iterations ) );
        }
        else
        {
          container.add( sum / as<double>( iterations ) );
        }
      } );
    } );
  }

  if ( talent.deeply_rooted_elements.ok() )
  {
    dre_samples.analyze();
    dre_samples.create_histogram( static_cast<unsigned>( dre_samples.max() - dre_samples.min() + 1 ) );
    dre_uptime_samples.analyze();
    dre_uptime_samples.create_histogram( static_cast<unsigned>( std::ceil( dre_uptime_samples.max() ) - std::floor( dre_uptime_samples.min() ) + 1 ) );
  }

  lvs_samples.analyze();
  lvs_samples.create_histogram( static_cast<unsigned>( lvs_samples.max() - lvs_samples.min() + 1 ) );
}

// shaman_t::datacollection_end ============================================

void shaman_t::datacollection_end()
{
  parse_player_effects_t::datacollection_end();

  if ( buff.ascendance->iteration_uptime() > 0_ms )
  {
    dre_uptime_samples.add( 100.0 * buff.ascendance->iteration_uptime() / iteration_fight_length );
  }
}

const spell_data_t* shaman_t::conditional_spell_lookup( bool fn, int id )
{
  if ( !fn )
  {
    return spell_data_t::not_found();
  }
  return find_spell( id );
}

// shaman_t::init_spells ===================================================

void shaman_t::init_spells()
{
  //
  // Generic spells
  //
  spec.mail_specialization          = find_specialization_spell( "Mail Specialization" );
  spec.shaman                       = find_spell( 137038 );

  // Elemental
  spec.elemental_shaman  = find_specialization_spell( "Elemental Shaman" );
  spec.elemental_shaman2 = find_specialization_spell( 462107 );
  spec.elemental_shaman3 = find_specialization_spell( 1231772 );
  spec.maelstrom         = find_specialization_spell( 343725 );
  spec.lava_surge        = find_specialization_spell( "Lava Surge" );
  spec.lava_burst_2      = find_rank_spell( "Lava Burst", "Rank 2" );
  spec.inundate          = find_specialization_spell( "Inundate" );
  spec.stormkeeper_2     = find_spell( 383009 );

  // Enhancement
  spec.critical_strikes   = find_specialization_spell( "Critical Strikes" );
  spec.dual_wield         = find_specialization_spell( "Dual Wield" );
  spec.enhancement_shaman = find_specialization_spell( 137041 );
  spec.enhancement_shaman2= find_specialization_spell( 1214207 );
  spec.stormbringer       = find_specialization_spell( "Stormsurge" );
  spec.stormstrike        = find_specialization_spell( "Stormstrike" );

  // Restoration
  spec.resurgence         = find_specialization_spell( "Resurgence" );
  spec.riptide            = find_specialization_spell( "Riptide" );
  spec.tidal_waves        = find_specialization_spell( "Tidal Waves" );
  spec.restoration_shaman = find_specialization_spell( "Restoration Shaman" );

  //
  // Masteries
  //
  mastery.elemental_overload = find_mastery_spell( SHAMAN_ELEMENTAL );
  mastery.enhanced_elements  = find_mastery_spell( SHAMAN_ENHANCEMENT );
  mastery.deep_healing       = find_mastery_spell( SHAMAN_RESTORATION );

  // Talents
  auto _CT = [this]( util::string_view name ) {
    return find_talent_spell( talent_tree::CLASS, name );
  };

  auto _ST = [this]( util::string_view name ) {
    return find_talent_spell( talent_tree::SPECIALIZATION, name );
  };

  // Class tree
  // Row 1
  talent.lava_burst      = _CT( "Lava Burst" );
  talent.lava_lash       = _CT( "Lava Lash" );
  talent.chain_lightning = _CT( "Chain Lightning" );
  // Row 2
  talent.earth_elemental = _CT( "Earth Elemental" );
  talent.wind_shear      = _CT( "Wind Shear" );
  talent.spirit_wolf     = _CT( "Spirit Wolf" );
  talent.thunderous_paws = _CT( "Thunderous Paws" );
  talent.frost_shock     = _CT( "Frost Shock" );
  // Row 3
  talent.earth_shield     = _CT( "Earth Shield" );
  talent.fire_and_ice     = _CT( "Fire and Ice" );
  talent.capacitor_totem  = _CT( "Capacitor Totem" );
  // Row 4
  talent.spiritwalkers_grace = _CT( "Spiritwalker's Grace" );
  talent.static_charge       = _CT( "Static Charge" );
  // Row 5
  talent.spiritual_awakening = _CT( "Spiritual Awakening" );
  talent.graceful_spirit     = _CT( "Graceful Spirit" );
  talent.natures_fury        = _CT( "Nature's Fury" );
  // Row 6
  talent.totemic_surge       = _CT( "Totemic Surge" );
  talent.winds_of_alakir     = _CT( "Winds of Al'Akir" );
  // Row 7
  talent.healing_stream_totem    = _CT( "Healing Stream Totem" );
  talent.improved_lightning_bolt = _CT( "Improved Lightning Bolt" );
  talent.spirit_walk             = _CT( "Spirit Walk" );
  talent.gust_of_wind            = _CT( "Gust of Wind" );
  talent.enhanced_imbues         = _CT( "Enhanced Imbues" );
  // Row 8
  talent.natures_swiftness       = _CT( "Nature's Swiftness" );
  talent.thunderstorm            = _CT( "Thunderstorm" );
  talent.totemic_focus           = _CT( "Totemic Focus ");
  talent.surging_shields         = _CT( "Surging Shields" );
  // Row 9
  talent.lightning_lasso         = _CT( "Lightning Lasso" );
  talent.thundershock            = _CT( "Thundershock" );
  talent.totemic_recall          = _CT( "Totemic Recall" );
  // Row 10
  talent.ancestral_guidance      = _CT( "Ancestral Guidance" );
  talent.creation_core           = _CT( "Creation Core" );
  talent.call_of_the_elements = _CT( "Call of the Elements" );
  talent.instinctive_imbuements  = _CT( "Instinctive Imbuements" );

  // Spec - Shared
  talent.ancestral_wolf_affinity = _ST( "Ancestral Wolf Affinity" );
  talent.elemental_blast         = _ST( "Elemental Blast" );

  std::vector<std::pair<player_talent_t&, util::string_view>> spec_talents {
    // Shared
    { talent.ascendance,             "Ascendance"            },

    // Elemental
    // Row 1
    // Row 2
    // Row 3
    // Row 4
    // Row 5
    // Row 6
    // Row 7
    // Row 8
    // Row 9
    // Row 10

    // Enhancement
    // Row 1
    { talent.maelstrom_weapon,       "Maelstrom Weapon"       },
    // Row 2
    { talent.windfury_weapon,        "Windfury Weapon"        },
    { talent.flametongue_weapon,     "Flametongue Weapon"     },
    // Row 3
    { talent.forceful_winds,         "Forceful Winds"         },
    { talent.crash_lightning,        "Crash Lightning"        },
    { talent.molten_assault,         "Molten Assault"         },
    // Row 4
    { talent.unruly_winds,           "Unruly Winds"           },
    { talent.raging_maelstrom,       "Raging Maelstrom"       },
    { talent.ashen_catalyst,         "Ashen Catalyst"         },
    // Row 5
    { talent.stormblast,             "Stormblast"             },
    { talent.overcharge,             "Overcharge"             },
    { talent.overflowing_maelstrom,  "Overflowing Maelstrom"  },
    { talent.flurry,                 "Flurry"                 },
    { talent.hot_hand,               "Hot Hand"               },
    // Row 6
    { talent.voltaic_blaze,          "Voltaic Blaze"          },
    { talent.elemental_tempo,        "Elemental Tempo"        },
    { talent.storms_wrath,           "Storm's Wrath"          },
    // Row 7
    { talent.chaining_storms,        "Chaining Storms"        },
    { talent.converging_storms,      "Converging Storms"      },
    { talent.stormflurry,            "Stormflurry"            },
    { talent.stormbind,              "Stormbind"              },
    { talent.elemental_weapons,      "Elemental Weapons"      },
    { talent.fire_nova,              "Fire Nova"              },
    { talent.lashing_flames,         "Lashing Flames"         },
    // Row 8
    { talent.ride_the_lightning,     "Ride the Lightning"     },
    { talent.doom_winds,             "Doom Winds"             },
    { talent.sundering,              "Sundering"              },
    // Row 9
    { talent.lightning_strikes,      "Lightning Strikes"      },
    { talent.elemental_assault,      "Elemental Assault"      },
    { talent.static_accumulation,    "Static Accumulation"    },
    { talent.feral_spirit,           "Feral Spirit"           },
    { talent.surging_elements,       "Surging Elements"       },
    // Row 10
    { talent.thunder_capacitor,      "Thunder Capacitor"      },
    { talent.deeply_rooted_elements, "Deeply Rooted Elements" },
    { talent.thorims_invocation,     "Thorim's Invocation"    },
    { talent.primordial_storm,       "Primordial Storm"       },
  };

  std::vector<std::pair<player_talent_t&, util::string_view>> hero_talents {
    // Stormbringer
    // Row 1
    { talent.tempest,           "Tempest"           },

    // Row 2
    { talent.unlimited_power,   "Unlimited Power"   },
    { talent.stormcaller,       "Stormcaller"       },
    { talent.electroshock,      "Electroshock"      },
    { talent.stormwell,         "Stormwell"         },

    // Row 3
    { talent.supercharge,       "Supercharge"       },
    { talent.storm_swell,       "Storm Swell"       },
    { talent.arc_discharge,     "Arc Discharge"     },
    { talent.rolling_thunder,   "Rolling Thunder"   },
    { talent.natural_gift,      "Natural Gift"      },

    // Row 4
    { talent.voltaic_surge,     "Voltaic Surge"     },
    { talent.conductive_energy, "Conductive Energy" },
    { talent.descending_skies,  "Descending Skies"  },

    // Row 5
    { talent.awakening_storms,  "Awakening Storms"  },
    // Totemic
    // Row 1
    { talent.surging_totem,         "Surging Totem"         },
    // Row 2
    { talent.totemic_rebound,       "Totemic Rebound"       },
    { talent.amplification_core,    "Amplification Core"    },
    { talent.oversurge,             "Oversurge"             },
    { talent.lively_totems,         "Lively Totems"         },
    { talent.totemic_momentum,      "Totemic Momentum"      },
    // Row 3
    { talent.splitstream,            "Splitstream"            },
    { talent.elemental_attunement,  "Elemental Attunement"  },
    // Row 4
    { talent.imbuement_mastery,     "Imbuement Mastery"     },
    { talent.pulse_capacitor,       "Pulse Capacitor"       },
    { talent.supportive_imbuements, "Supportive Imbuements" },
    { talent.totemic_coordination,  "Totemic Coordination"  },
    { talent.earthsurge,            "Earthsurge"            },
    { talent.primal_catalyst,       "Primal Catalyst"       },
    // Row 5
    { talent.whirling_elements,     "Whirling Elements"     },
  };

  // Initialize Specialization tree talents
  for ( const auto& entry : spec_talents )
  {
    std::get<0>( entry ) = find_talent_spell( talent_tree::SPECIALIZATION, std::get<1>( entry ) );
  }

  // Initialize Hero tree talents
  for ( const auto& entry : hero_talents )
  {
    std::get<0>( entry ) = find_talent_spell( talent_tree::HERO, std::get<1>( entry ) );
  }

  // Enhancement keystones
  talent.storm_unleashed_1 = find_talent_spell( talent_tree::SPECIALIZATION, 1262713 );
  talent.storm_unleashed_2 = find_talent_spell( talent_tree::SPECIALIZATION, 1262761 );
  talent.storm_unleashed_3 = find_talent_spell( talent_tree::SPECIALIZATION, 1252373 );

  // Elemental
  // Row 1
  talent.earth_shock = _ST( "Earth Shock" );
  // Row 2
  talent.earthquake_reticle = find_talent_spell( talent_tree::SPECIALIZATION, 61882 );
  talent.earthquake_target = find_talent_spell( talent_tree::SPECIALIZATION, 462620 );

  talent.elemental_fury = _ST( "Elemental Fury" );
  // Row 3
  talent.flash_of_lightning     = _ST( "Flash of Lightning" );
  talent.tectonic_collapse      = _ST( "Tectonic Collapse" );
  talent.aftershock             = _ST( "Aftershock" );
  talent.echo_of_the_elements   = _ST( "Echo of the Elements" );
  // Row 4
  talent.lightning_capacitor    = _ST( "Lightning Capacitor" );
  talent.stormkeeper            = _ST( "Stormkeeper" );
  talent.master_of_the_elements = _ST( "Master of the Elements" );
  talent.molten_wrath           = _ST( "Molten Wrath" );
  // Row 5
  talent.storm_frenzy           = _ST( "Storm Frenzy" );
  talent.swelling_maelstrom     = _ST( "Swelling Maelstrom" );
  talent.primordial_fury        = _ST( "Primordial Fury" );
  talent.fury_of_the_storms     = _ST( "Fury of the Storms" );
  talent.herald_of_the_storms   = _ST( "Herald of the Storms" ); // Added in PTR
  talent.flames_of_the_cauldron = _ST( "Flames of the Cauldron" );
  // Row 6
  talent.amped_up               = _ST( "Amped Up" );
  talent.elemental_resonance    = _ST( "Elemental Resonance" );
  talent.thunderstrike_ward     = _ST( "Thunderstrike Ward" );
  talent.path_of_the_seer       = _ST( "Path of the Seer" );
  talent.flametongue_weapon     = _ST( "Flametongue Weapon" );
  talent.elemental_unity        = _ST( "Elemental Unity" );
  // Row 7
  talent.power_of_the_maelstrom = _ST( "Power of the Maelstrom" );
  talent.earthshatter           = _ST( "Earthshatter" );
  talent.storm_infusion         = _ST( "Storm Infusion" );
  talent.echo_chamber           = _ST( "Echo Chamber" );
  talent.searing_flames         = _ST( "Searing Flames" );
  talent.everlasting_elements   = _ST( "Everlasting Elements" );
  talent.earthen_rage           = _ST( "Earthen Rage" );
  // Row 8
  talent.fusion_of_elements     = _ST( "Fusion of Elements" );
  talent.eye_of_the_storm       = _ST( "Eye of the Storm" );
  talent.inferno_arc            = _ST( "Inferno Arc" );
  // Row 9
  talent.lightning_rod          = _ST( "Lightning Rod" );
  talent.mountains_will_fall    = _ST( "Mountains Will Fall" );
  talent.call_of_fire           = _ST( "Call of Fire" );
  talent.flames_of_the_firelord = _ST( "Flames of the Firelord" );
  talent.primal_elementalist    = _ST( "Primal Elementalist" );
  // Row 10
  talent.charged_conduit           = _ST( "Charged Conduit" );
  talent.first_ascendant           = _ST( "First Ascendant" );
  talent.preeminence               = _ST( "Preeminence" );
  talent.crackling_fury            = _ST( "Crackling Fury" );
  talent.purging_flames            = _ST( "Purging Flames" );

  talent.feedback_loop_1 = find_talent_spell( talent_tree::SPECIALIZATION, "Feedback Loop", 1 );
  talent.feedback_loop_2 = find_talent_spell( talent_tree::SPECIALIZATION, "Feedback Loop", 2 );
  talent.feedback_loop_3 = find_talent_spell( talent_tree::SPECIALIZATION, "Feedback Loop", 3 );

  // Farseer

  talent.call_of_the_ancestors  = find_talent_spell( talent_tree::HERO, "Call of the Ancestors" );

  talent.latent_wisdom          = find_talent_spell( talent_tree::HERO, "Latent Wisdom" );
  talent.ancient_fellowship     = find_talent_spell( talent_tree::HERO, "Ancient Fellowship" );
  talent.heed_my_call           = find_talent_spell( talent_tree::HERO, "Heed My Call" );
  talent.routine_communication  = find_talent_spell( talent_tree::HERO, "Routine Communication" );
  talent.elemental_reverb       = find_talent_spell( talent_tree::HERO, "Elemental Reverb" );
  talent.ancestral_influence    = find_talent_spell( talent_tree::HERO, "Ancestral Influence" );

  talent.offering_from_beyond   = find_talent_spell( talent_tree::HERO, "Offering from Beyond" );
  talent.primordial_capacity    = find_talent_spell( talent_tree::HERO, "Primordial Capacity" );
  talent.spiritwalkers_momentum = find_talent_spell( talent_tree::HERO, "Spiritwalker's Momentum" );
  talent.windspeaker            = find_talent_spell( talent_tree::HERO, "Windspeaker" );

  talent.maelstrom_supremacy    = find_talent_spell( talent_tree::HERO, "Maelstrom Supremacy" );
  talent.final_calling          = find_talent_spell( talent_tree::HERO, "Final Calling" );
  talent.mystic_knowledge       = find_talent_spell( talent_tree::HERO, "Mystic Knowledge" );

  talent.ancestral_swiftness    = find_talent_spell( talent_tree::HERO, "Ancestral Swiftness" );

  //
  // Misc spells
  //

  switch ( specialization() )
  {
    case SHAMAN_ELEMENTAL:   spell.ascendance = find_spell( 1219480 ); break;
    case SHAMAN_ENHANCEMENT: spell.ascendance = find_spell( 114051 ); break;
    case SHAMAN_RESTORATION: spell.ascendance = find_spell( 114052 ); break;
    default:                 break;
  }

  spell.ascendance_mw_passive = find_spell( 1252197 );
  spell.resurgence          = find_spell( 101033 );
  spell.maelstrom_weapon    = find_spell( 187881 );
  spell.maelstrom_weapon_driver = find_spell( 187880 );
  spell.feral_spirit        = find_spell( 228562 );
  spell.fire_elemental      = find_spell( 188592 );
  spell.storm_elemental     = find_spell( 157299 );
  spell.earth_elemental     = find_spell( 188616 );
  spell.flametongue_weapon  = find_spell( 318038 );
  spell.windfury_weapon     = find_spell( 319773 );
  spell.inundate            = find_spell( 378777 );
  spell.storm_swell         = find_spell( 455089 );
  spell.lightning_rod       = find_spell( 210689 );
  spell.improved_flametongue_weapon = find_spell( 382028 );
  spell.earthen_rage        = find_spell( 170377 );
  spell.flowing_spirits_feral_spirit = find_spell( 469329 );
  spell.hot_hand            = find_spell( 201900 );
  spell.elemental_weapons   = find_spell( 408390 );
  spell.tww3_farseer_2pc      = conditional_spell_lookup( sets->has_set_bonus( HERO_FARSEER, TWW3, B2), 1236406 );
  spell.tww3_farseer_4pc      = conditional_spell_lookup( sets->has_set_bonus( HERO_FARSEER, TWW3, B4 ), 1236407 );
  spell.tww3_stormbringer_2pc = conditional_spell_lookup( sets->has_set_bonus( HERO_STORMBRINGER, TWW3, B2 ), 1236408 );
  spell.tww3_stormbringer_4pc = conditional_spell_lookup( sets->has_set_bonus( HERO_STORMBRINGER, TWW3, B4 ), 1236409 );

  // Misc spell-related init
  max_active_flame_shock   = as<unsigned>( find_spell( 470411 )->max_targets() );

  parse_player_effects_t::init_spells();

  // Register passives
  register_passive_effect_mask( talent.enhanced_imbues,
    specialization() == SHAMAN_ELEMENTAL     ? effect_mask_t( false ).enable( 1, 4, 5, 9 )
    : specialization() == SHAMAN_ENHANCEMENT ? effect_mask_t( false ).enable( 2 )
                                             : effect_mask_t( false ).enable( 3, 6, 7, 8, 10 ) );

  register_passive_effect_mask( spell.tww3_stormbringer_4pc,
    specialization() == SHAMAN_ELEMENTAL ? effect_mask_t( false ).enable( 3, 4 )
                                         : effect_mask_t( false ).enable( 1, 2 ) );

  deregister_passive_spell( talent.overcharge );

  // custom overrides
  // Ancestor Lava Burst, Chain Lightning, Elemental Blast are now affected by Elemental Fury, but spelldata does not reflect this.
  register_passive_affect_list( talent.elemental_fury, affect_list_t( 1 ).add_spell( 447419, 447425, 465717 ) );

  // general parsing
  parse_all_class_passives();
  parse_all_passive_talents();
  parse_all_passive_sets();
  parse_raid_buffs();

  // Constants
  constant.mul_lightning_rod = find_spell( 210689 )->effectN( 2 ).percent();
}

// shaman_t::init_base ======================================================

void shaman_t::init_base_stats()
{
  if ( base.distance < 1 )
    base.distance = ( specialization() == SHAMAN_ENHANCEMENT ) ? 5 : 30;

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility  = 1.0;
  base.spell_power_per_intellect = 1.0;

  parse_player_effects_t::init_base_stats();
}

// shaman_t::init_scaling ===================================================

void shaman_t::init_scaling()
{
  parse_player_effects_t::init_scaling();

  switch ( specialization() )
  {
    case SHAMAN_ENHANCEMENT:
      scaling->enable( STAT_WEAPON_OFFHAND_DPS );
      scaling->disable( STAT_STRENGTH );
      scaling->disable( STAT_SPELL_POWER );
      scaling->disable( STAT_INTELLECT );
      break;
    case SHAMAN_RESTORATION:
      scaling->disable( STAT_MASTERY_RATING );
      break;
    default:
      break;
  }
}

// ==========================================================================
// Shaman Misc helpers
// ==========================================================================


void shaman_t::summon_elemental( elemental type, timespan_t override_duration )
{
  spawner::pet_spawner_t<pet::primal_elemental_t, shaman_t>* spawner_ptr = nullptr;
  buff_t* elemental_buff = nullptr;

  switch ( type )
  {
    case elemental::GREATER_FIRE:
    case elemental::PRIMAL_FIRE:
    {
      elemental_buff = buff.fire_elemental;
      spawner_ptr = &( pet.fire_elemental );
      break;
    }
    case elemental::GREATER_STORM:
    case elemental::PRIMAL_STORM:
    {
      elemental_buff = buff.storm_elemental;
      spawner_ptr = &( pet.storm_elemental );
      break;
    }
    case elemental::GREATER_EARTH:
    case elemental::PRIMAL_EARTH:
    {
      elemental_buff = buff.earth_elemental;
      spawner_ptr = &( pet.earth_elemental );
      break;
    }
    default:
      assert( 0 );
      break;
  }

  if ( spawner_ptr->n_active_pets() > 0 )
  {
    timespan_t new_duration = spawner_ptr->active_pet()->expiration->remains();
    new_duration += override_duration > 0_ms ? override_duration : elemental_buff->buff_duration();

    elemental_buff->extend_duration( override_duration > 0_ms ? override_duration : elemental_buff->buff_duration() );
    spawner_ptr->active_pet()->expiration->reschedule( new_duration );
    for (auto action : spawner_ptr->active_pet()->action_list)
    {
        action->cooldown->reset(false);
    }
  }
  else
  {
    elemental_buff->trigger( override_duration > 0_ms ? override_duration : elemental_buff->buff_duration() );
    spawner_ptr->spawn( override_duration > 0_ms ? override_duration : elemental_buff->buff_duration() );
  }
}

void shaman_t::trigger_elemental_blast_proc()
{
    ::trigger_elemental_blast_proc( this );
}

void shaman_t::summon_ancestor( double proc_chance )
{
  if ( !talent.call_of_the_ancestors.ok() )
  {
    return;
  }

  if ( !rng().roll( proc_chance ) )
  {
    return;
  }

  if ( talent.offering_from_beyond.ok() )
  {
    cooldown.stormkeeper->adjust( talent.offering_from_beyond->effectN( 1 ).time_value() );
  }

  pet.ancestor.spawn( buff.call_of_the_ancestors->buff_duration() );
  buff.call_of_the_ancestors->trigger( buff.call_of_the_ancestors->buff_duration() );
}

void shaman_t::summon_feral_spirit( wolf_type_e type, unsigned n, timespan_t duration )
{
  for ( unsigned i = 0U; i < n; ++i )
  {
    switch ( type )
    {
      case wolf_type_e::FIRE_WOLF:
        buff.molten_weapon->trigger( duration );
        pet.fire_wolves.spawn( duration );
        break;
      case wolf_type_e::LIGHTNING_WOLF:
        buff.crackling_surge->trigger( duration );
        pet.lightning_wolves.spawn( duration );
        break;
      default:
        break;
    }
  }
}


// shaman_t::validate_fight_style ==========================================
bool shaman_t::validate_fight_style( fight_style_e style ) const
{
  if ( specialization() == SHAMAN_ELEMENTAL )
  {
    switch ( style )
    {
      case FIGHT_STYLE_DUNGEON_ROUTE:
      case FIGHT_STYLE_DUNGEON_SLICE:
        return false;
      default:
        return true;
    }
  }

  return true;
}

// ==========================================================================
// Shaman Ability Triggers
// ==========================================================================

void shaman_t::trigger_hot_hand( const action_state_t* state )
{
  if ( !talent.hot_hand.ok() )
  {
    return;
  }

  if ( buff.hot_hand->check() )
  {
    return;
  }

  assert( debug_cast<shaman_attack_t*>( state->action ) != nullptr && "Hot Hand called on invalid action type" );
  shaman_attack_t* attack = debug_cast<shaman_attack_t*>( state->action );

  if ( !attack->may_proc_hot_hand )
  {
    return;
  }

  if ( main_hand_weapon.buff_type != FLAMETONGUE_IMBUE &&
       off_hand_weapon.buff_type != FLAMETONGUE_IMBUE )
  {
    return;
  }

  if ( buff.hot_hand->trigger() && attack->proc_hh )
  {
    attack->proc_hh->occur();
  }
}

void shaman_t::trigger_deeply_rooted_elements( const action_state_t* state )
{
  if ( !talent.deeply_rooted_elements.ok() || specialization() == SHAMAN_ELEMENTAL )
  {
    return;
  }

  auto spell = debug_cast<shaman_spell_t*>( state->action );
  unsigned draws = spell->mw_consumed_stacks;

  bool success = false;
  for ( auto draw = 0U; draw < draws; ++draw )
  {
    dre_attempts++;
    if ( rng_obj.deeply_rooted_elements.trigger() )
    {
      assert( !success );
      success = true;
    }
  }

  if ( success )
  {
    dre_samples.add( as<double>( dre_attempts ) );
    dre_attempts = 0U;

    action.dre_ascendance->execute_on_target( state->target );
    spell->proc_deeply_rooted_elements->occur();
  }
}

void shaman_t::trigger_secondary_flame_shock( player_t* target, spell_variant variant ) const
{
  action_t* fs = action.flame_shock;

  if ( variant == spell_variant::ASCENDANCE )
  {
    fs = action.flame_shock_asc;
  }
  if ( variant == spell_variant::VOLTAIC_BLAZE )
  {
    fs = action.flame_shock_vb;
  }

  fs->execute_on_target( target );
}

void shaman_t::trigger_secondary_flame_shock( const action_state_t* state, spell_variant variant ) const
{
  if ( !state->action->result_is_hit( state->result ) )
  {
    return;
  }

  trigger_secondary_flame_shock( state->target, variant );
}

void shaman_t::regenerate_flame_shock_dependent_target_list( const action_t* action ) const
{
  auto& tl = action->target_cache.list;

  auto it = std::remove_if( tl.begin(), tl.end(), [ this ]( player_t* target ) {
    return !get_target_data( target )->dot.flame_shock->is_ticking();
  } );

  tl.erase( it, tl.end() );

  if ( sim->debug )
  {
    sim->print_debug("{} targets with flame_shock on:", *this );
    for ( size_t i = 0; i < tl.size(); i++ )
    {
      sim->print_debug( "[{}, {} (id={})]", i, *tl[ i ], tl[ i ]->actor_index );
    }
  }
}

void shaman_t::consume_maelstrom_weapon( const action_state_t* state, int stacks )
{
  if ( !talent.maelstrom_weapon.ok() )
  {
    return;
  }

  if ( state->action->internal_id >= as<int>( mw_spend_list.size() ) )
  {
    mw_spend_list.resize( state->action->internal_id + 1 );
  }

  mw_spend_list[ state->action->internal_id ][ stacks ].add( stacks );

  if ( stacks > 0 )
  {
    buff.maelstrom_weapon->decrement( stacks );

    trigger_tempest( stacks );

    if ( talent.unlimited_power.ok() )
    {
      buff.unlimited_power->trigger();
    }

    trigger_deeply_rooted_elements( state );
  }

  if ( buff.tww2_enh_2pc->check() &&
    rng().roll( sets->set( SHAMAN_ENHANCEMENT, TWW2, B2 )->effectN( 1 ).base_value() * 0.001 * stacks ) )
  {
    buff.tww2_enh_2pc->expire();
    if ( sets->has_set_bonus( SHAMAN_ENHANCEMENT, TWW2, B4 ) )
    {
      buff.doom_winds->extend_duration_or_trigger(
        sets->set( SHAMAN_ENHANCEMENT, TWW2, B4 )->effectN( 1 ).time_value() );
    }
  }

  if ( talent.elemental_tempo.ok() && stacks > 0 )
  {
    cooldown.strike->adjust(
      timespan_t::from_seconds( -1.0 * stacks * talent.elemental_tempo->effectN( 3 ).base_value() / 1000.0 ),
      false );
    cooldown.lava_lash->adjust(
      timespan_t::from_seconds( -1.0 * stacks * talent.elemental_tempo->effectN( 3 ).base_value() / 1000.0 ),
      false );
  }

  if ( talent.lightning_strikes.ok() && stacks > 0 )
  {
    // [BUG] 2026-03-13: Lightning Strikes in-game only triggers if you consume exactly 10 stacks.
    // There is no counter-based mechanism.
    if ( bugs && stacks == as<int>( talent.lightning_strikes->effectN( 2 ).base_value() ) )
    {
      buff.lightning_strikes->trigger();
    }
    else if ( !bugs )
    {
      ls_counter += as<unsigned>( stacks );
      if ( ls_counter >= as<unsigned>( talent.lightning_strikes->effectN( 2 ).base_value() ) )
      {
        ls_counter -= as<unsigned>( talent.lightning_strikes->effectN( 2 ).base_value() );
        buff.lightning_strikes->trigger();
      }
    }
  }

  if ( talent.ascendance.ok() && !buff.ascendance->check() && stacks > 0 )
  {
    auto success = false;
    for ( auto draw = 0U; draw < as<unsigned>( stacks ); ++draw )
    {
      if ( rng_obj.asc_dw.trigger() )
      {
        assert( !success );
        success = true;
      }
    }

    sim->print_debug("{} attempts to proc doom_winds on {}, mw_stacks={}, success={}",
      name(), state->action->name(), stacks, success );

    if ( success )
    {
      action.doom_winds_asc->execute_on_target( state->target );
    }
  }

  if ( talent.storm_unleashed_1.ok() && stacks > 0 )
  {
    auto success = false;
    for ( auto draw = 0U; draw < as<unsigned>( stacks ); ++draw )
    {
      if ( rng_obj.storm_unleashed.trigger() )
      {
        assert( !success );
        success = true;
      }
    }

    if ( success )
    {
      buff.storm_unleashed->trigger();
    }
  }

  if ( talent.totemic_momentum.ok() && stacks > 0 && buff.hot_hand->check() )
  {
    auto extension = timespan_t::from_seconds(
      talent.totemic_momentum->effectN( 1 ).base_value() * 0.001 * stacks );

    buff.hot_hand->extend_duration( extension );

  }
}

void shaman_t::trigger_maelstrom_gain( double maelstrom_gain, gain_t* gain )
{
  if ( maelstrom_gain <= 0 )
  {
    return;
  }

  double g = maelstrom_gain;
  g *= composite_maelstrom_gain_coefficient();
  resource_gain( RESOURCE_MAELSTROM, g, gain );
}

void shaman_t::generate_maelstrom_weapon( const action_t* action, int stacks )
{
  if ( !talent.maelstrom_weapon.ok() )
  {
    return;
  }

  auto stacks_avail = buff.maelstrom_weapon->max_stack() - buff.maelstrom_weapon->check();
  auto stacks_added = std::min( stacks_avail, stacks );
  auto overflow = stacks - stacks_added;

  if ( action != nullptr )
  {
    if ( as<unsigned>( action->internal_id ) >= mw_source_list.size() )
    {
      mw_source_list.resize( action->internal_id + 1 );
    }

    mw_source_list[ action->internal_id ].first.add( as<double>( stacks_added ) );

    if ( overflow > 0 )
    {
      mw_source_list[ action->internal_id ].second.add( as<double>( overflow ) );
    }
  }

  buff.maelstrom_weapon->trigger( stacks );
}

void shaman_t::generate_maelstrom_weapon( const action_state_t* state, int stacks )
{
  generate_maelstrom_weapon( state->action, stacks );
}

double shaman_t::windfury_proc_chance()
{
  double proc_chance = spell.windfury_weapon->proc_chance();
  double proc_mul = mastery.enhanced_elements->effectN( 4 ).mastery_value() *
    ( 1.0 + talent.storms_wrath->effectN( 2 ).percent() );

  proc_chance += cache.mastery() * proc_mul;
  if ( buff.doom_winds->up() )
  {
    proc_chance *= 1 + talent.doom_winds->effectN( 1 ).trigger()->effectN( 1 ).percent();
  }

  return proc_chance;
}

void shaman_t::trigger_windfury_weapon( const action_state_t* state, double override_chance )
{
  assert( debug_cast<shaman_attack_t*>( state->action ) != nullptr && "Windfury Weapon called on invalid action type" );
  shaman_attack_t* attack = debug_cast<shaman_attack_t*>( state->action );
  if ( !attack->may_proc_windfury && override_chance == -1.0 )
    return;

  if ( buff.ghost_wolf->check() )
  {
    return;
  }

  // Note, applying Windfury-imbue to off-hand disables procs in game.
  if ( main_hand_weapon.buff_type != WINDFURY_IMBUE ||
      off_hand_weapon.buff_type == WINDFURY_IMBUE )
  {
    return;
  }

  if ( state->action->weapon->slot == SLOT_MAIN_HAND &&
       rng().roll( override_chance != -1.0 ? override_chance : windfury_proc_chance() ) )
  {
    action_t* a = windfury_mh;

    // Note, windfury needs to do a discrete execute event because in AoE situations, Forceful Winds
    // must be let to stack (fully) before any Windfury Attacks are executed. In this case, the
    // schedule must be done through a pre-snapshotted state object to preserve targeting
    // information.
    trigger_secondary_ability( state, a );

    trigger_secondary_ability( state, a );

    double chance = talent.unruly_winds->effectN( 1 ).percent();

    if ( rng().roll( chance ) )
    {
      trigger_secondary_ability( state, a );
      proc.windfury_uw->occur();
    }

    attack->proc_wf->occur();
  }
}

void shaman_t::trigger_flametongue_weapon( const action_state_t* state )
{
  assert( debug_cast<shaman_attack_t*>( state->action ) != nullptr &&
          "Flametongue Weapon called on invalid action type" );
  shaman_attack_t* attack = debug_cast<shaman_attack_t*>( state->action );
  if ( !attack->may_proc_flametongue )
  {
    return;
  }

  if ( main_hand_weapon.buff_type != FLAMETONGUE_IMBUE &&
       off_hand_weapon.buff_type != FLAMETONGUE_IMBUE )
  {
    return;
  }

  if ( buff.ghost_wolf->check() )
  {
    return;
  }

  flametongue->set_target( state->target );
  flametongue->execute();
  attack->proc_ft->occur();

  // Windfury Flametongues can proc Imbuement Mastery
  if ( state->action->id == 25504 )
  {
    trigger_imbuement_mastery( flametongue->execute_state );
  }

}

void shaman_t::trigger_lava_surge()
{
  if ( !spec.lava_surge->ok() )
  {
    return;
  }

  if ( buff.lava_surge->check() )
  {
    proc.wasted_lava_surge->occur();
  }

  proc.lava_surge->occur();

  if ( !executing || executing->id != 51505 )
  {
    cooldown.lava_burst->reset( true );
  }
  else
  {
    proc.surge_during_lvb->occur();
    lava_surge_during_lvb = true;
  }

  buff.lava_surge->trigger();
}

void shaman_t::trigger_elemental_assault( const action_state_t* state )
{
  if ( !talent.elemental_assault.ok() )
  {
    return;
  }

  if ( !rng().roll( talent.elemental_assault->effectN( 3 ).percent() )  )
  {
    return;
  }

  make_event( sim, 0_s, [ this, state ]() {
    generate_maelstrom_weapon( state,
                               as<int>( talent.elemental_assault->effectN( 2 ).base_value() ) );
    } );
}

void shaman_t::trigger_stormflurry( const action_state_t* state )
{
  if ( !talent.stormflurry.ok() )
  {
    return;
  }

  if ( !rng().roll( talent.stormflurry->effectN( 1 ).percent() ) )
  {
    return;
  }

  auto a = state->action->id == 115356 ? action.stormflurry_ws : action.stormflurry_ss;
  auto s = debug_cast<const stormstrike_state_t*>( state );

  timespan_t delay = rng().gauss<200,25>();
  if ( sim->debug )
  {
    auto ss = static_cast<stormstrike_base_t*>( state->action );
    sim->out_debug.print(
      "{} scheduling stormflurry source={}, action={}, target={}, delay={}, chained={} stormblast={}",
      name(), state->action->name(), a->name(), state->target->name(), delay,
      static_cast<unsigned>( ss->strike_type ), s->stormblast );
  }

  // Note, on live, the stormblast does not propagate to the stormflurried strikes, but rather
  // determines the state upon executing the strike
  make_event<stormstrike_t::stormflurry_event_t>( *sim, static_cast<stormstrike_base_t*>( a ),
                                                 state->target, delay,
                                                 s->stormblast );
}

void shaman_t::trigger_imbuement_mastery( const action_state_t* state )
{
  if ( !talent.imbuement_mastery.ok() )
  {
    return;
  }

  if ( !rng_obj.imbuement_mastery->trigger() )
  {
    return;
  }

  get_target_data( state->target )->debuff.flametongue_attack->trigger();
  action.imbuement_mastery->execute_on_target( state->target );
}

void shaman_t::trigger_whirling_fire( const action_state_t* state )
{
  if ( !talent.whirling_elements.ok() )
  {
    return;
  }

  if ( !buff.whirling_fire->check() )
  {
    return;
  }

  if ( buff.whirling_fire->consume( state->action, 1 ) )
  {
    // [BUG] 2025-03-08 Apparently in-game, a Mote of Fire consuming Lava Lash will trigger an
    // additional Splitstream Sundering on the target.
    if ( bugs && buff.hot_hand->check() )
    {
      trigger_splitstream( state );
    }

    // Mote of Fire extends an existing Hot Hand buff, or triggers a new one with its duration
    if ( buff.hot_hand->check() )
    {
      buff.hot_hand->extend_duration( buff.whirling_fire->data().effectN( 1 ).time_value() );
    }
    else
    {
      buff.hot_hand->trigger( 1, buff_t::DEFAULT_VALUE(), 1.0,
        buff.whirling_fire->data().effectN( 1 ).time_value() );
    }

    trigger_tww3_totemic_enh_2pc( state );
  }
}

void shaman_t::trigger_stormblast( const action_state_t* state )
{
  if ( !talent.stormblast.ok() )
  {
    return;
  }

  auto s = debug_cast<const stormstrike_attack_state_t*>( state );
  if ( !s->stormblast )
  {
    return;
  }

  auto a= debug_cast<stormstrike_attack_t*>( state->action );

  if ( a->result_is_hit( state->result ) )
  {
    auto dmg = talent.stormblast->effectN( 1 ).percent() * state->result_amount;
    a->stormblast->base_dd_min = a->stormblast->base_dd_max = dmg;

    a->stormblast->execute_on_target( state->target );
  }
}

template <typename T>
void shaman_t::trigger_tempest( T resource_count )
{
  if ( !talent.tempest.ok() )
  {
    return;
  }
  double tempest_chance = 0.0;
  if ( specialization() == SHAMAN_ELEMENTAL )
  {
    tempest_chance = talent.tempest->effectN( 1 ).percent() * 0.01 * resource_count;
  }
  else if ( specialization() == SHAMAN_ENHANCEMENT )
  {
    tempest_chance = talent.tempest->effectN( 2 ).percent() * 0.01 * resource_count;
  }
  else
  {
    return;
  }

  sim->print_debug( "{} attempts to proc tempest on {} consumed: chance={}", name(), resource_count,
    tempest_chance );

  if ( specialization() == SHAMAN_ENHANCEMENT )
  {
    bool success = false;

    for ( auto draw = 0U; draw < as<unsigned>( resource_count ); ++draw )
    {
      if ( rng_obj.tempest_enh.trigger() )
      {
        assert( !success );
        success = true;
      }
    }

    if ( success )
    {
      buff.tempest->trigger();
    }
  }
  else if ( specialization() == SHAMAN_ELEMENTAL )
  {
    bool success = false;

    for ( auto draw = 0U; draw < as<unsigned>( resource_count ); ++draw )
    {
      if ( rng_obj.tempest_ele.trigger() )
      {
        assert( !success );
        success = true;
      }
    }

    if ( success )
    {
      buff.tempest->trigger();
    }
  }
}

void shaman_t::trigger_awakening_storms( player_t* target )
{
  if ( spell.tww3_stormbringer_2pc->ok() )
  {
    aws_counter++;

    unsigned int tww3_mod_value = static_cast<unsigned int>( specialization() == SHAMAN_ELEMENTAL
      ? spell.tww3_stormbringer_2pc->effectN( 3 ).base_value()
      : spell.tww3_stormbringer_2pc->effectN( 4 ).base_value() );

    if ( aws_counter % tww3_mod_value == 0 )
    {
      action.set_ascendance->execute_on_target( target );
    }
  }

  if ( buff.tempest->check() )
  {
    proc.tempest_awakening_storms->occur();
  }
  buff.tempest->trigger();
}

void shaman_t::trigger_awakening_storms( double maelstrom_consumed, player_t* target )
{
  if ( !talent.awakening_storms.ok() )
  {
    return;
  }

  if ( !rng().roll( maelstrom_consumed * talent.awakening_storms->effectN( 3 ).percent() * 0.01 ) )
  {
    return;
  }

  trigger_awakening_storms( target );
}

void shaman_t::trigger_awakening_storms( const action_state_t* state )
{
  if ( !talent.awakening_storms.ok() )
  {
    return;
  }

  if ( !rng_obj.awakening_storms->trigger() )
  {
    return;
  }

  trigger_awakening_storms( state->target );
}

void shaman_t::trigger_earthsurge( const action_state_t* state, double mul )
{
  if ( !talent.earthsurge.ok() )
  {
    return;
  }

  surging_totem_t* totem = debug_cast<surging_totem_t*>( pet.surging_totem.active_pet() );
  if ( totem == nullptr )
  {
    return;
  }

  totem->trigger_earthsurge( state->target, mul );
}

void shaman_t::trigger_whirling_air( const action_state_t* state )
{
  if ( !talent.whirling_elements.ok() )
  {
    return;
  }

  if ( specialization() != SHAMAN_ENHANCEMENT )
  {
    return;
  }

  if ( !buff.whirling_air->up() )
  {
    return;
  }

  for ( auto i = 0U;
        i < as<unsigned>( buff.whirling_air->data().effectN( 3 ).base_value() ); ++i )
  {
    // First Whirling Air Surging Bolt seems to trigger around 300ms later
    trigger_totemic_rebound( state, true, 300_ms + i * 500_ms );
  }

  buff.whirling_air->decrement();

  trigger_tww3_totemic_enh_2pc( state );
}

void shaman_t::trigger_splitstream( const action_state_t* state )
{
  if ( !talent.splitstream.ok() || !buff.hot_hand->up() )
  {
    return;
  }

  action.splitstream->execute_on_target( state->target );
}

void shaman_t::trigger_thunderstrike_ward( const action_state_t* state )
{
  if ( !buff.thunderstrike_ward->up() )
  {
    return;
  }

  if ( !rng().roll( options.thunderstrike_ward_proc_chance * ( 1.0 + talent.storm_infusion->effectN( 2 ).percent()) ) )
  {
    return;
  }

  for ( int i = 0; i < talent.thunderstrike_ward->effectN( 1 ).base_value(); ++i )
  {
    action.thunderstrike_ward->execute_on_target( state->target );
  }
}

// TODO: Target swaps
void shaman_t::trigger_earthen_rage( const action_state_t* state )
{
  if ( !talent.earthen_rage -> ok() )
  {
    return;
  }

  if ( !state->action->harmful )
  {
    return;
  }

  if ( !state->action->result_is_hit( state -> result ) )
  {
    return;
  }

  // Earthen Rage damage does not trigger itself
  if ( state->action == action.earthen_rage )
  {
    return;
  }

  if ( sim->debug )
  {
    sim->out_debug.print( "{} earthen_rage proc by {}", *this, *state->action );
  }

  earthen_rage_target = state->target;
  if ( earthen_rage_event == nullptr )
  {
    earthen_rage_event = make_event<earthen_rage_event_t>( *sim, this,
      sim->current_time() + spell.earthen_rage->duration() );
  }
  else
  {
    debug_cast<earthen_rage_event_t*>( earthen_rage_event )
        ->set_end_time( sim->current_time() + spell.earthen_rage->duration() );
  }
}

void shaman_t::trigger_totemic_rebound( const action_state_t* state, bool whirl, timespan_t delay )
{
  if ( !pet.surging_totem.n_active_pets() )
  {
    return;
  }

  if ( !whirl && !rng_obj.totemic_rebound->trigger() )
  {
    return;
  }

  buff.totemic_rebound->trigger();

  make_event( *sim, delay, [ this, t = state->target ] {
    if ( t->is_sleeping() || !pet.surging_totem.n_active_pets() )
    {
      return;
    }

    for ( auto totem : pet.surging_totem )
    {
      debug_cast<surging_totem_t*>( totem )->trigger_surging_bolt( t );
    }
  } );
}

void shaman_t::trigger_ancestor( ancestor_cast cast, const action_state_t* state )
{
  if ( cast == ancestor_cast::DISABLED )
  {
    return;
  }

  if ( !talent.call_of_the_ancestors.ok() )
  {
    return;
  }

  for ( auto ancestor : pet.ancestor )
  {
    if ( sim->debug )
    {
      sim->out_debug.print( "{} ancestor triggers {} from {} at {}", name(), ancestor_cast_str( cast ),
                            state->action->name(), state->target->name() );
    }

    debug_cast<pet::ancestor_t*>( ancestor )->trigger_cast( cast, state->target );
  }
}

void shaman_t::trigger_arc_discharge( const action_state_t* state )
{
  if ( specialization() != SHAMAN_ENHANCEMENT )
  {
    return;
  }

  if ( !talent.arc_discharge.ok() || !buff.arc_discharge->up() )
  {
    return;
  }

  auto s = shaman_spell_t::cast_state( state );

  if ( !s->is_variant( spell_variant::NORMAL ) &&
       !s->is_variant( spell_variant::THORIMS_INVOCATION ) &&
       !s->is_variant( spell_variant::PRIMORDIAL_STORM ) &&
       !s->is_variant( spell_variant::ARC_DISCHARGE ) &&
       !s->is_variant( spell_variant::RIDE_THE_LIGHTNING ) )
  {
    return;
  }

  if ( buff.arc_discharge->consume( state->action, 1 ) )
  {
    // Arc Discharge is capable of self-proccing; experimentally verified in game to roughly 3/4 of
    // re-triggering within the ICD (400ms), vs 1/4 of re-triggering outside of the ICD in which
    // case the Arc Discharge Lightning Bolt will consume the remaining stack of Arc Discharge and
    // trigger again.
    make_event( *sim, rng().range( 325_ms, 425_ms ),
      [ this, t = state->target, rtl = s->is_variant( spell_variant::RIDE_THE_LIGHTNING ) ]() {
        if ( t->is_sleeping() )
        {
          return;
        }

        if ( rtl )
        {
          action.chain_lightning_rtl_ad->execute_on_target( t );
        }
        else
        {
          action.chain_lightning_ad->execute_on_target( t );
        }
    } );
  }
}

void shaman_t::trigger_lively_totems( const action_state_t* state )
{
  if ( !talent.lively_totems.ok() )
  {
    return;
  }

  for ( auto totem_ptr : pet.searing_totem )
  {
    searing_totem_t* st = debug_cast<searing_totem_t*>( totem_ptr );
    st->volley->execute_on_target( state->target );
  }
}

void shaman_t::trigger_tww3_totemic_enh_2pc( const action_state_t* state )
{
  if ( !sets->has_set_bonus( HERO_TOTEMIC, TWW3, B2 ) || specialization() != SHAMAN_ENHANCEMENT )
  {
    return;
  }

  if ( buff.whirling_air->check() || buff.whirling_earth->check() || buff.whirling_fire->check() )
  {
    return;
  }

  sim->print_debug( "{} triggering tww3 totemic enhancement 2pc set bonus", this->name() );
  action.tww3_primordial_storm->execute_on_target( state->target );
}

void shaman_t::trigger_elemental_overflow( const action_state_t* state, action_t* trigger )
{
  if ( buff.elemental_overflow->consume( state->action, 1 ) )
  {
    auto delay = rng().gauss( 500_ms, 33_ms );
    sim->print_debug( "{} triggering enhancement tww3 4 piece set bonus / primal catalyst using {} on {}, delay={}",
      name(), trigger->name(), state->target->name(), delay );
    make_event( sim, delay, [ t = state->target, trigger ]() {
      if ( t->is_sleeping() )
      {
        return;
      }
      trigger->execute_on_target( t );
    } );
  }
}

void shaman_t::trigger_ride_the_lightning( const action_state_t* state, action_t* trigger )
{
  if ( !talent.ride_the_lightning.ok() )
  {
    return;
  }

  auto cl = debug_cast<chain_lightning_t*>( trigger );
  switch( state->action->id )
  {
    case 60103: // Lava Lash
      cl->trigger( strike_variant::NORMAL, state->target );
      break;
    default: // Strikes
    {
      auto strike = debug_cast<stormstrike_base_t*>( state->action );
      cl->trigger( strike->strike_type, state->target );
      break;
    }
  }
}

void shaman_t::trigger_thorims_invocation( const action_state_t* state )
{
  if ( !talent.thorims_invocation.ok() )
  {
    return;
  }

  if ( buff.maelstrom_weapon->check() == 0 )
  {
    return;
  }

  // On 11.2, Tempest overrides the TI primer completely
  if ( buff.tempest->check() )
  {
    action.tempest_ti->execute_on_target( state->target );
  }
  else if ( action.ti_trigger )
  {
    action.ti_trigger->execute_on_target( state->target );
  }
  // Default to Lightning Bolt
  else
  {
    action.lightning_bolt_ti->execute_on_target( state->target );
  }
}

void shaman_t::trigger_crash_lightning_proc( const action_state_t* state, strike_variant t )
{
  if ( !talent.crash_lightning.ok() )
  {
    return;
  }

  debug_cast<crash_lightning_attack_t*>( action.crash_lightning_aoe )->trigger( state, t );
}

// shaman_t::init_buffs =====================================================

void shaman_t::create_buffs()
{
  parse_player_effects_t::create_buffs();

  //
  // Shared
  //
  buff.ascendance = new ascendance_buff_t( this );
  buff.ghost_wolf = make_buff( this, "ghost_wolf", find_class_spell( "Ghost Wolf" ) );
  buff.flurry = make_buff( this, "flurry", talent.flurry->effectN( 1 ).trigger() )
    ->set_default_value( talent.flurry->effectN( 1 ).trigger()->effectN( 1 ).percent() )
    ->add_invalidate( CACHE_AUTO_ATTACK_SPEED );
  buff.natures_swiftness = make_buff( this, "natures_swiftness", talent.natures_swiftness )
    ->set_cooldown(0_ms)
    ->set_stack_change_callback( [ this ]( buff_t*, int, int cur ) {
      if ( cur == 0 )
        cooldown.natures_swiftness->start( cooldown.natures_swiftness->action );
    } );

  buff.elemental_blast_crit = make_buff<buff_t>( this, "elemental_blast_critical_strike", find_spell( 118522 ) )
    ->set_default_value_from_effect_type(A_MOD_ALL_CRIT_CHANCE)
    ->set_pct_buff_type( STAT_PCT_BUFF_CRIT )
    ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC );

  buff.elemental_blast_haste = make_buff<buff_t>( this, "elemental_blast_haste", find_spell( 173183 ) )
    ->set_default_value_from_effect_type(A_HASTE_ALL)
    ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
    ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC );

  buff.elemental_blast_mastery = make_buff<buff_t>( this, "elemental_blast_mastery", find_spell( 173184 ) )
                                     ->set_default_value_from_effect_type( A_MOD_MASTERY_PCT )
                                     ->set_pct_buff_type( STAT_PCT_BUFF_MASTERY )
                                     ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC );

  buff.stormkeeper = make_buff( this, "stormkeeper", find_spell( 191634 ) )
    ->set_cooldown( timespan_t::zero() )  // Handled by the action
    ->set_default_value_from_effect( 2 ); // Damage bonus as default value

  buff.tempest = make_buff( this, "tempest", find_spell( 454015 ) );
  buff.unlimited_power = make_buff( this, "unlimited_power", find_spell( 454394 ) )
    ->set_default_value_from_effect_type( A_HASTE_ALL )
    ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
    ->set_refresh_behavior( buff_refresh_behavior::DISABLED );

  buff.arc_discharge = make_buff( this, "arc_discharge", find_spell( 470532 ) )
    ->set_trigger_spell( talent.arc_discharge ); //TODO Hawk: Disable this for Ele?

  buff.storm_swell = make_buff( this, "storm_swell", find_spell( 455089 ) )
    ->set_default_value_from_effect_type(A_MOD_MASTERY_PCT)
    ->set_pct_buff_type( STAT_PCT_BUFF_MASTERY )
    ->set_trigger_spell( talent.storm_swell );

  buff.amplification_core = make_buff( this, "amplification_core", find_spell( 456369 ) )
    ->set_default_value_from_effect( 1 )
    ->set_trigger_spell( talent.amplification_core );

  buff.whirling_air = make_buff( this, "whirling_air", find_spell( 453409 ) )
    ->set_trigger_spell( talent.whirling_elements );
  buff.whirling_fire = make_buff( this, "whirling_fire", find_spell( 453405 ) )
    ->set_trigger_spell( talent.whirling_elements );
  buff.whirling_earth = make_buff( this, "whirling_earth", find_spell( 453406 ) )
    ->set_default_value_from_effect( 1 )
    ->set_trigger_spell( talent.whirling_elements );
  buff.lightning_shield = make_buff( this, "lightning_shield", find_spell( 192106 ) )
      ->add_invalidate(CACHE_PLAYER_DAMAGE_MULTIPLIER);

  buff.totemic_rebound = make_buff( this, "totemic_rebound", find_spell( 458269 ) )
    ->set_default_value_from_effect( 1 );

  buff.surging_totem = make_buff( this, "surging_totem", find_spell( 1221347 ) )
    ->set_trigger_spell( talent.surging_totem );

  buff.flametongue_weapon = make_buff( this, "flametongue_weapon", find_class_spell( "Flametongue Weapon") );

  //
  // Elemental
  //
  buff.lava_surge = make_buff( this, "lava_surge", find_spell( 77762 ) )
                        ->set_activated( false )
                        ->set_chance( 1.0 );  // Proc chance is handled externally

  buff.master_of_the_elements = make_buff( this, "master_of_the_elements", talent.master_of_the_elements->effectN(1).trigger() )
          ->set_default_value( talent.master_of_the_elements->effectN( 2 ).percent() );

  buff.wind_gust = make_buff( this, "wind_gust", find_spell( 263806 ) )
                       ->set_default_value( find_spell( 263806 )->effectN( 1 ).percent() )
                       ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
                       ->set_default_value_from_effect_type( A_HASTE_ALL );

  buff.power_of_the_maelstrom =
      make_buff( this, "power_of_the_maelstrom", talent.power_of_the_maelstrom )
          ->set_default_value( talent.power_of_the_maelstrom->effectN( 1 ).trigger()->effectN( 1 ).base_value() );

  // PvP
  buff.thundercharge = make_buff( this, "thundercharge", find_spell( 204366 ) )
                           ->set_cooldown( timespan_t::zero() )
                           ->set_default_value( find_spell( 204366 )->effectN( 1 ).percent() )
                           ->set_stack_change_callback( [ this ]( buff_t*, int, int ) {
                             range::for_each( ability_cooldowns, []( cooldown_t* cd ) {
                               if ( cd->down() )
                               {
                                 cd->adjust_recharge_multiplier();
                               }
                             } );
                           } );

  buff.earth_elemental = make_buff( this, "earth_elemental", find_spell( 188616 ));
  buff.fire_elemental = make_buff( this, "fire_elemental", spell.fire_elemental )
                        ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_TICK_TIME );
  buff.storm_elemental = make_buff( this, "storm_elemental", spell.storm_elemental );

  buff.call_of_the_ancestors = make_buff( this, "call_of_the_ancestors", find_spell( 447244 ) )
                                   ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
                                   ->set_trigger_spell( talent.call_of_the_ancestors )
                                   ->set_pct_buff_type( STAT_PCT_BUFF_INTELLECT )
                                   ->set_default_value_from_effect( 2 );
  buff.ancestral_swiftness = make_buff( this, "ancestral_swiftness", find_spell( 443454 ) )
    ->set_trigger_spell( talent.ancestral_swiftness )
    ->set_cooldown( 0_ms )
    ->set_stack_change_callback( [ this ]( buff_t*, int, int cur ) {
      if ( cur == 0 )
          cooldown.ancestral_swiftness->start( cooldown.ancestral_swiftness->action );
    } );
  buff.thunderstrike_ward = make_buff( this, "thunderstrike_ward", talent.thunderstrike_ward );

  buff.purging_flames = make_buff( this, "purging_flames", find_spell( 1259491 ) );

  buff.mid1_ele_2pc = make_buff( this, "thunderous_velocity", find_spell( 1272101 ) )
                          ->set_trigger_spell( sets->set( SHAMAN_ELEMENTAL, MID1, B2 ) )
                          ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
                          ->set_default_value_from_effect_type( A_HASTE_ALL );

  //
  // Enhancement
  //

  buff.molten_weapon = make_buff( this, "molten_weapon", find_spell( 224125 ) )
    ->set_trigger_spell( talent.feral_spirit );
  buff.crackling_surge  = make_buff( this, "crackling_surge", find_spell( 224127 ) )
    ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );
  buff.converging_storms = make_buff( this, "converging_storms", find_spell( 198300 ) )
      ->set_default_value_from_effect( 1 );
  // Buffs stormstrike and lava lash after using crash lightning
  buff.crash_lightning = make_buff( this, "crash_lightning",
    find_spell( talent.storm_unleashed_1.ok() ? 1252415 : 187878 ) )
    ->set_stack_behavior( talent.storm_unleashed_1.ok()
      ? buff_stack_behavior::ASYNCHRONOUS
      : buff_stack_behavior::DEFAULT
    )
    ->set_chance( talent.crash_lightning.ok() ? 1.0 : 0.0 );

  buff.hot_hand = make_buff( this, "hot_hand", find_spell( 215785 ) )
    ->set_chance( talent.hot_hand.ok()
      ? talent.hot_hand->proc_chance()
      : talent.whirling_elements.ok()
        ? 1.0
        : 0.0 );
  buff.spirit_walk  = make_buff( this, "spirit_walk", talent.spirit_walk );
  buff.stormsurge = make_buff( this, "stormsurge", find_spell( 201846 ) );
  buff.maelstrom_weapon = make_buff( this, "maelstrom_weapon", find_spell( 344179 ) )
    ->set_chance( talent.maelstrom_weapon.ok() ? 1.0 : 0.0 );
  buff.static_accumulation = make_buff( this, "static_accumulation", find_spell( 384437 ) )
    ->set_default_value( talent.static_accumulation->effectN( 1 ).base_value() )
    ->set_trigger_spell( talent.static_accumulation )
    ->set_duration( 0_ms ) // Buff state controlled by Ascendance and Doom winds buffs
    ->set_tick_callback( [ this ]( buff_t* b, int, timespan_t ) {
      generate_maelstrom_weapon( action.ascendance, as<int>( b->value() ) );
    } );
  buff.doom_winds = make_buff( this, "doom_winds", find_spell( 466772 ) )
    ->set_tick_on_application( true )
    ->set_period( timespan_t::from_seconds( find_spell( 466772 )->effectN( 5 ).base_value() ) )
    ->set_cooldown( 0_ms ) // Handled by the action
    ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
      if ( new_ == 1 && !buff.static_accumulation->check() )
      {
        buff.static_accumulation->trigger();
      }
      else if ( new_ == 0 && !buff.ascendance->check() )
      {
        buff.static_accumulation->expire();
      }
    } )
    ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
      if ( target->is_sleeping() )
      {
        return;
      }

      action.doom_winds->execute_on_target( target );
    } );

  buff.windfury_weapon = make_buff( this, "windfury_weapon", find_spell( 319773 ) )
    ->set_trigger_spell( talent.windfury_weapon );

  buff.stormblast = make_buff( this, "stormblast", find_spell( 470466 ) )
    ->set_cooldown( 0_ms ) // Stormblast uses ICD for something else than applications
    ->set_trigger_spell( talent.stormblast );

  buff.primordial_storm = make_buff( this, "primordial_storm",
    talent.primordial_storm->effectN( 1 ).trigger() );

  buff.lightning_strikes = make_buff( this, "lightning_strikes", find_spell( 384451 ) )
    ->set_trigger_spell( talent.lightning_strikes );

  buff.surging_elements = make_buff( this, "surging_elements", find_spell( 382043 ) )
    ->set_trigger_spell( talent.surging_elements );

  buff.storm_unleashed = make_buff( this, "storm_unleashed", find_spell( 1262830 ) )
    ->set_stack_change_callback( [ this ]( buff_t*, int old, int new_ ) {
      if ( old == 0 )
      {
        range::for_each( crash_lightning, [ this ]( action_t* a ) {
          a->cooldown = cooldown.crash_lightning_su;
        } );
      }
      else if ( new_ == 0 )
      {
        range::for_each( crash_lightning, [ this ]( action_t* a ) {
          a->cooldown = cooldown.crash_lightning;
        } );
      }
    } )
    ->set_trigger_spell( talent.storm_unleashed_1 );

  buff.tww2_enh_2pc = make_buff( this, "winning_streak", find_spell( 1218616 ) )
    ->set_trigger_spell( sets->set( SHAMAN_ENHANCEMENT, TWW2, B2 ) );
  buff.tww2_enh_4pc = make_buff( this, "electrostatic_wager", find_spell( 1223410 ) )
    ->set_trigger_spell( sets->set( SHAMAN_ENHANCEMENT, TWW2, B4 ) );
  buff.tww2_enh_4pc_damage = make_buff( this, "electrostatic_wager_dmg", find_spell( 1223332 ) )
    ->set_quiet( true )
    ->set_trigger_spell( sets->set( SHAMAN_ENHANCEMENT, TWW2, B4 ) );
  buff.elemental_overflow = make_buff( this, "elemental_overflow", find_spell( 1239170 ) )
    ->set_chance( sets->has_set_bonus( HERO_TOTEMIC, TWW3, B4 ) || talent.primal_catalyst.ok() ? 1.0 : 0.0 );
  buff.storms_eye = make_buff( this, "storms_eye", find_spell(1239315) )
                        ->set_trigger_spell( spell.tww3_stormbringer_4pc )
                        ->set_max_stack(6);  //TODO: retest. assumption is that 6 is max

  buff.lively_totems = make_buff( this, "lively_totems", find_spell( 461242 ) )
    ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );
  //
  // Restoration
  //
  buff.spiritwalkers_grace =
      make_buff( this, "spiritwalkers_grace", find_specialization_spell( "Spiritwalker's Grace" ) )
          ->set_cooldown( timespan_t::zero() );
  buff.tidal_waves =
      make_buff( this, "tidal_waves", spec.tidal_waves->ok() ? find_spell( 53390 ) : spell_data_t::not_found() );

}

// shaman_t::init_gains =====================================================

void shaman_t::init_gains()
{
  parse_player_effects_t::init_gains();

  gain.aftershock              = get_gain( "Aftershock" );
  gain.searing_flames          = get_gain( "Searing Flames" );
  gain.resurgence              = get_gain( "resurgence" );
  gain.feral_spirit            = get_gain( "Feral Spirit" );
  gain.spirit_of_the_maelstrom = get_gain( "Spirit of the Maelstrom" );
  gain.inundate                = get_gain( "Inundate" );
}

// shaman_t::init_procs =====================================================

void shaman_t::init_procs()
{
  parse_player_effects_t::init_procs();

  proc.lava_surge                               = get_proc( "Lava Surge" );
  proc.wasted_lava_surge                        = get_proc( "Lava Surge: Wasted" );
  proc.surge_during_lvb                         = get_proc( "Lava Surge: During Lava Burst" );

  proc.ascendance_tempest_overload              = get_proc( "Ascendance: Tempest" );
  proc.ascendance_lightning_bolt_overload       = get_proc( "Ascendance: Lightning Bolt" );
  proc.ascendance_chain_ligtning_overload       = get_proc( "Ascendance: Chain Lightning" );
  proc.ascendance_lava_burst_overload           = get_proc( "Ascendance: Lava Burst" );
  proc.ascendance_earth_shock_overload          = get_proc( "Ascendance: Earth Shock" );
  proc.ascendance_elemental_blast_overload      = get_proc( "Ascendance: Elemental Blast" );
  proc.ascendance_earthquake_overload           = get_proc( "Ascendance: Earthquake" );

  proc.potm_tempest_overload                    = get_proc( "PotM: Tempest" );

  proc.aftershock                               = get_proc( "Aftershock" );
  proc.lightning_rod                            = get_proc( "Lightning Rod" );
  proc.searing_flames                           = get_proc( "Searing Flames" );

  proc.elemental_blast_crit                     = get_proc( "Elemental Blast: Critical Strike" );
  proc.elemental_blast_haste                    = get_proc( "Elemental Blast: Haste" );
  proc.elemental_blast_mastery                  = get_proc( "Elemental Blast: Mastery" );

  proc.windfury_uw            = get_proc( "Windfury: Unruly Winds" );
  proc.stormflurry_failed     = get_proc( "Stormflurry (failed)" );

  proc.reset_swing_mw            = get_proc( "Maelstrom Weapon Swing Reset" );

  proc.tempest_awakening_storms = get_proc( "Awakened Storms w/ Tempest");
}

// shaman_t::init_uptimes ====================================================
void shaman_t::init_uptimes()
{
  parse_player_effects_t::init_uptimes();

  uptime.hot_hand = get_uptime( "Hot Hand" )->collect_uptime( *sim )->collect_duration( *sim );
}

// shaman_t::init_assessors =================================================

void shaman_t::init_assessors()
{
  parse_player_effects_t::init_assessors();
}

// shaman_t::init_rng =======================================================

void shaman_t::init_rng()
{
  parse_player_effects_t::init_rng();

  rng_obj.awakening_storms = get_rppm( "awakening_storms", talent.awakening_storms );
  rng_obj.lively_totems = get_rppm( "lively_totems", talent.lively_totems );
  rng_obj.totemic_rebound = get_rppm( "totemic_rebound", talent.totemic_rebound );

  if ( options.ancient_fellowship_positive == 0 ) {
    options.ancient_fellowship_positive = as<unsigned>( talent.ancient_fellowship->effectN( 3 ).base_value() );
  }
  if ( options.ancient_fellowship_total == 0 ) {
    options.ancient_fellowship_total = as<unsigned>( talent.ancient_fellowship->effectN( 2 ).base_value() );
  }
  rng_obj.ancient_fellowship =
    get_shuffled_rng( "ancient_fellowship", options.ancient_fellowship_positive, options.ancient_fellowship_total );

  if ( options.routine_communication_positive == 0 ) {
    options.routine_communication_positive = as<unsigned>( talent.routine_communication->effectN( 5 ).base_value() );
  }
  if ( options.routine_communication_total == 0 ) {
    // This is effect 6 based on live data. PTR data is confusing in comparison.
    options.routine_communication_total = as<unsigned>( talent.routine_communication->effectN( 6 ).base_value() );
  }
  rng_obj.routine_communication =
    get_shuffled_rng( "routine_communication", options.routine_communication_positive, options.routine_communication_total );

  rng_obj.imbuement_mastery = get_accumulated_rng( "imbuement_mastery",
    options.imbuement_mastery_base_chance );
  rng_obj.lively_totems_ptr = get_accumulated_rng( "lively_totems_ptr",
    options.lively_totems_base_chance );

  if ( talent.deeply_rooted_elements.ok() )
  {
    rng_obj.deeply_rooted_elements
      .set_param_fn( [ this ]( rng::deck_rng_wrapper_t<rng::dre_deck_rng_t>& obj ) {
        // Default: 2 successful procs per deck
        auto n_draws = obj.opt_success() != -1 ? as<unsigned>( obj.opt_success() ) : 2U;
        // Default: Total based on deeply rooted elements script value (333)
        auto n_total = obj.opt_total() > 0
          ? obj.opt_total()
          : static_cast<unsigned>(
              n_draws / ( talent.deeply_rooted_elements->effectN( 3 ).base_value() / 10.0 / 100.0 )
            );
        return std::make_tuple( n_total, n_draws, 10U );
      } )
      .build();
  }

  if ( talent.tempest.ok() && specialization() == SHAMAN_ENHANCEMENT )
  {
    rng_obj.tempest_enh
        .set_param_fn( []( rng::deck_rng_wrapper_t<rng::dre_deck_rng_t>& obj ) {
          // Default: 2 successful procs per deck
          auto n_draws = obj.opt_success() != -1 ? as<unsigned>( obj.opt_success() ) : 2U;
          // Default: 200 total cards
          auto n_total = obj.opt_total() > 0 ? obj.opt_total() : 100U;
          return std::make_tuple( n_total, n_draws, 10U );
        } )
        .build();
  }

  if ( talent.tempest.ok() && specialization() == SHAMAN_ELEMENTAL )
  {
      unsigned int successes_per_deck = 2;
      double tempest_chance           = talent.tempest->effectN( 1 ).percent() * 0.01;
      unsigned int deck_size          = static_cast<unsigned int>( successes_per_deck / tempest_chance );
    rng_obj.tempest_ele
        .set_param_fn( [&]( rng::deck_rng_wrapper_t<rng::dre_deck_rng_t>& obj ) {
          // Default: 2 successful procs per deck
            auto n_draws = obj.opt_success() != -1 ? as<unsigned>( obj.opt_success() ) : successes_per_deck;
          // Default: 333 total cards
            auto n_total = obj.opt_total() > 0 ? obj.opt_total() : deck_size;
          return std::make_tuple( n_total, n_draws, 90U );
        } )
        .build();
  }

  if ( talent.storm_unleashed_1.ok() )
  {
    rng_obj.storm_unleashed
        .set_param_fn( []( rng::deck_rng_wrapper_t<rng::dre_deck_rng_t>& obj ) {
          // Default: 5 successful procs per deck
          auto n_draws = obj.opt_success() != -1 ? as<unsigned>( obj.opt_success() ) : 5U;
          // Default: 250 total cards
          auto n_total = obj.opt_total() > 0 ? obj.opt_total() : 250U;
          return std::make_tuple( n_total, n_draws, 10U );
        } )
        .build();
  }

  if ( talent.ascendance.ok() )
  {
    rng_obj.asc_dw
        .set_param_fn( []( rng::deck_rng_wrapper_t<rng::dre_deck_rng_t>& obj ) {
          // Default: 3 successful procs per deck
          auto n_draws = obj.opt_success() != -1 ? as<unsigned>( obj.opt_success() ) : 3U;
          // Default: 600 total cards
          auto n_total = obj.opt_total() > 0 ? obj.opt_total() : 600U;
          return std::make_tuple( n_total, n_draws, 10U );
        } )
        .build();
  }
}

void shaman_t::init_special_effects()
{
  callbacks.register_callback_trigger_function(
      452030, dbc_proc_callback_t::trigger_fn_type::CONDITION,
      [ id = 51505U ]( const dbc_proc_callback_t*, const proc_data_t& data, player_t*, action_state_t* s,
                       proc_trigger_type_e ) {
        if ( data->id() == id )
        {
          lava_burst_t* lvb = debug_cast<lava_burst_t*>( s->action );
          return lvb->is_variant( spell_variant::NORMAL );
        }
        return false;
      } );

    parse_player_effects_t::init_special_effects();

}

void shaman_t::init_finished()
{
  parse_player_effects_t::init_finished();

  apply_player_effects();
}

bool shaman_t::validate_actor()
{
  if ( !( primary_role() == ROLE_ATTACK && specialization() == SHAMAN_ENHANCEMENT ) &&
       !( primary_role() == ROLE_SPELL && specialization() == SHAMAN_ELEMENTAL ) &&
       !( primary_role() == ROLE_SPELL && specialization() == SHAMAN_RESTORATION ) )
  {
    if ( !quiet )
      sim->errorf( "Player %s's role (%s) or spec(%s) isn't supported yet.", name(),
                   util::role_type_string( primary_role() ), util::specialization_string( specialization() ) );
    return false;
  }

  // Restoration isn't supported atm
  if ( !sim->allow_experimental_specializations && specialization() == SHAMAN_RESTORATION &&
       primary_role() == ROLE_HEAL )
  {
    if ( !quiet )
      sim->errorf( "Restoration Shaman healing for player %s is not currently supported.", name() );

    return false;
  }

  return true;
}

void shaman_t::apply_player_effects()
{
  // Shared
  eff::source_eff_builder_t( buff.ghost_wolf ).build( this );
  eff::source_eff_builder_t( buff.spirit_walk ).build( this );

  // Enhancement
  eff::source_eff_builder_t( buff.flurry ).set_flag( IGNORE_STACKS ).build( this );
  // Enable attack speed bonus individually, so we can apply a bug to the mastery bonus (12.0 4PC)
  eff::source_eff_builder_t( buff.crash_lightning )
    .set_effect_mask( effect_mask_t( false ).enable( 2 ) )
    .build( this );
  // [20260328] BUG: Enhancement 12.0 4PC gives half as much mastery as is on the tin
  eff::source_eff_builder_t( buff.crash_lightning )
    .set_effect_mask( effect_mask_t( false ).enable( 3 ) )
    .set_value( [ this ]( double value ) -> double {
      return value * ( bugs ? 0.5 : 1.0 );
    } )
    .build( this );

  // Elemental
  eff::source_eff_builder_t( mastery.elemental_overload ).build( this );
  eff::source_eff_builder_t( buff.lightning_shield ).build( this );
  eff::source_eff_builder_t( buff.ascendance ).build( this );
  eff::source_eff_builder_t( buff.surging_elements ).build( this );
}

void shaman_t::apply_action_effects( parse_effects_t* a )
{
  // Enhancement
  eff::source_eff_builder_t( mastery.enhanced_elements )
    .add_affect_list( affect_list_t( 1 ).add_spell( 390287 ) ) // Stormblast
    .build( a );

  eff::source_eff_builder_t( buff.amplification_core ).build( a );
  eff::source_eff_builder_t( buff.crackling_surge ).build( a );
  eff::source_eff_builder_t( buff.molten_weapon ).build( a );
  eff::source_eff_builder_t( buff.doom_winds ).build( a );
  eff::source_eff_builder_t( buff.converging_storms ).build( a );
  eff::source_eff_builder_t( buff.lightning_strikes ).build( a );
  eff::source_eff_builder_t( buff.ascendance ).build( a );
  eff::source_eff_builder_t( buff.hot_hand ).build( a );
  eff::source_eff_builder_t( buff.arc_discharge ).build( a );
  eff::source_eff_builder_t( buff.whirling_earth )
    .add_affect_list( affect_list_t( 1 ).remove_spell( 467283 ) ) // Splitstream Sundering
    .build( a );

/*eff::source_eff_builder_t( talent.enhanced_imbues )
    .set_state_fn( [ this ] { return buff.flametongue_weapon->check(); } )
    .set_effect_mask( effect_mask_t( false ).enable( 2 ) )
    .add_affect_list( affect_list_t( 2 ).remove_spell( 10444, 318038, 319778, 467386, 467390 ))
    .build( a );
  eff::source_eff_builder_t( talent.enhanced_imbues )
    .set_state_fn( [ this ] { return buff.windfury_weapon->check(); } )
    .set_effect_mask( effect_mask_t( false ).enable( 2 ) )
    .add_affect_list( affect_list_t( 2 ).remove_spell( 25504, 33750 ))
    .build( a );*/
  eff::source_eff_builder_t( spell.elemental_weapons )
    .add_affect_list( affect_list_t( 1 ).add_spell( 390287 ) )  // Stormblast
    .set_state_fn( [ this ] { return talent.elemental_weapons.ok(); } )
    .set_value( [ this ]( double ) {
      unsigned n_imbues = ( main_hand_weapon.buff_type != 0 ) + ( off_hand_weapon.buff_type != 0 );
      return talent.elemental_weapons->effectN( 1 ).percent() * 0.1 * n_imbues;
    } )
    .build( a );

  eff::source_eff_builder_t( buff.tww2_enh_2pc ).build( a );
  eff::source_eff_builder_t( buff.tww2_enh_4pc_damage ).build( a );

  // Elemental
  eff::source_eff_builder_t( mastery.elemental_overload ).build( a );
}

// shaman_t::generate_bloodlust_options =====================================

std::string shaman_t::generate_bloodlust_options()
{
  std::string bloodlust_options = "if=";

  if ( sim->bloodlust_percent > 0 )
    bloodlust_options += "target.health.pct<" + util::to_string( sim->bloodlust_percent ) + "|";

  if ( sim->bloodlust_time < timespan_t::zero() )
    bloodlust_options += "target.time_to_die<" + util::to_string( -sim->bloodlust_time.total_seconds() ) + "|";

  if ( sim->bloodlust_time > timespan_t::zero() )
    bloodlust_options += "time>" + util::to_string( sim->bloodlust_time.total_seconds() ) + "|";
  bloodlust_options.erase( bloodlust_options.end() - 1 );

  return bloodlust_options;
}

// shaman_t::default_potion =================================================

std::string shaman_t::default_potion() const
{
  std::string enhancement_potion = ( true_level >= 81 ) ? "lights_potential_2" :
                                   ( true_level >= 71 ) ? "tempered_potion_3" :
                                   ( true_level >= 61 ) ? "elemental_potion_of_ultimate_power_3" :
                                   ( true_level >= 51 ) ? "potion_of_spectral_agility" :
                                   ( true_level >= 45 ) ? "potion_of_unbridled_fury" :
                                   "disabled";

  std::string restoration_potion = ( true_level >= 61 ) ? "elemental_potion_of_ultimate_power_3" :
                                   ( true_level >= 51 ) ? "potion_of_spectral_intellect" :
                                   ( true_level >= 45 ) ? "potion_of_unbridled_fury" :
                                   "disabled";

  switch(specialization()) {
    case SHAMAN_ELEMENTAL:
      return shaman_apl::potion_elemental( this );
    case SHAMAN_ENHANCEMENT:
      return enhancement_potion;
    case SHAMAN_RESTORATION:
      return restoration_potion;
    default:
      return "disabled";
  }
}

// shaman_t::default_flask ==================================================

std::string shaman_t::default_flask() const
{
  std::string enhancement_flask = ( true_level >= 81 ) ? "flask_of_the_blood_knights_2" :
                                  ( true_level >= 71 ) ? "flask_of_alchemical_chaos_3" :
                                  ( true_level >= 61 ) ? "iced_phial_of_corrupting_rage_3" :
                                  ( true_level >= 51 ) ? "spectral_flask_of_power" :
                                  ( true_level >= 45 ) ? "greater_flask_of_the_currents" :
                                  "disabled";

  std::string restoration_flask = ( true_level >= 61 ) ? "phial_of_static_empowerment_3" :
                                  ( true_level >= 51 ) ? "spectral_flask_of_power" :
                                  ( true_level >= 45 ) ? "greater_flask_of_endless_fathoms" :
                                  "disabled";

  switch(specialization()) {
    case SHAMAN_ELEMENTAL:
      return shaman_apl::flask_elemental( this );
    case SHAMAN_ENHANCEMENT:
      return enhancement_flask;
    case SHAMAN_RESTORATION:
      return restoration_flask;
    default:
      return "disabled";
  }
}

// shaman_t::default_food ===================================================

std::string shaman_t::default_food() const
{
  std::string enhancement_food = ( true_level >= 81 ) ? "harandar_celebration" :
                                 ( true_level >= 71 ) ? "chippy_tea" :
                                 ( true_level >= 61 ) ? "fated_fortune_cookie" :
                                 ( true_level >= 51 ) ? "feast_of_gluttonous_hedonism" :
                                 ( true_level >= 45 ) ? "baked_port_tato" :
                                 "disabled";

  std::string restoration_food = ( true_level >= 61 ) ? "fated_fortune_cookie" :
                                 ( true_level >= 51 ) ? "feast_of_gluttonous_hedonism" :
                                 ( true_level >= 45 ) ? "baked_port_tato" :
                                 "disabled";

  switch(specialization()) {
    case SHAMAN_ELEMENTAL:
      return shaman_apl::food_elemental( this );
    case SHAMAN_ENHANCEMENT:
      return enhancement_food;
    case SHAMAN_RESTORATION:
      return restoration_food;
    default:
      return "disabled";
  }
}

// shaman_t::default_rune ===================================================

std::string shaman_t::default_rune() const
{
  return shaman_apl::rune( this );
}

// shaman_t::default_temporary_enchant ======================================

std::string shaman_t::default_temporary_enchant() const
{
  switch ( specialization() )
  {
    case SHAMAN_ELEMENTAL:
      return shaman_apl::temporary_enchant_elemental( this );
    case SHAMAN_ENHANCEMENT:
      return "disabled";
    case SHAMAN_RESTORATION:
      if ( true_level >= 60 )
        return "main_hand:shadowcore_oil";
      SC_FALLTHROUGH;
    default:
      return "disabled";
  }
}

// shaman_t::resource_loss ==================================================

double shaman_t::resource_loss( resource_e resource_type, double amount, gain_t* source, action_t* a )
{
  double loss = parse_player_effects_t::resource_loss( resource_type, amount, source, a );

  if ( resource_type == RESOURCE_MAELSTROM && loss > 0 )
  {
    trigger_tempest( loss );

    if ( talent.unlimited_power.ok() )
    {
      buff.unlimited_power->trigger();
    }
  }

  return loss;
}

// shaman_t::init_action_list_enhancement ===================================

void shaman_t::init_action_list_enhancement()
{
  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
      sim->errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  action_priority_list_t* precombat           = get_action_priority_list( "precombat" );
  action_priority_list_t* def                 = get_action_priority_list( "default" );
  action_priority_list_t* single_sb           = get_action_priority_list( "single_sb", "Single target action priority list for the Stormbringer hero talent tree" );
  action_priority_list_t* single_totemic      = get_action_priority_list( "single_totemic", "Single target action priority list for the Totemic hero talent tree" );
  action_priority_list_t* aoe                 = get_action_priority_list( "aoe", "Multi target action priority list" );
  action_priority_list_t* buffs               = get_action_priority_list( "buffs", "Buff action priority list" );

  // Pre-Combat
  precombat->add_action( "windfury_weapon" );
  precombat->add_action( "flametongue_weapon" );
  precombat->add_action( "lightning_shield" );
  precombat->add_action( "variable,name=trinket1_is_weird,value=trinket.1.is.algethar_puzzle_box|trinket.1.is.unyielding_netherprism" );
  precombat->add_action( "variable,name=trinket2_is_weird,value=trinket.2.is.algethar_puzzle_box|trinket.2.is.unyielding_netherprism" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "use_item,name=algethar_puzzle_box" );

  // Dynamic variables
  def->add_action( "variable,name=target_nature_mod,value=(1+debuff.chaos_brand.up*debuff.chaos_brand.value)*(1+(debuff.hunters_mark.up*target.health.pct>=80)*debuff.hunters_mark.value)" );
  def->add_action( "variable,name=expected_lb_funnel,value=action.lightning_bolt.damage*(1+debuff.lightning_rod.up*variable.target_nature_mod*(1+active_dot.flame_shock)*debuff.lightning_rod.value)" );
  def->add_action( "variable,name=expected_cl_funnel,value=action.chain_lightning.damage*(1+debuff.lightning_rod.up*variable.target_nature_mod*active_enemies*debuff.lightning_rod.value)" );
  def->add_action( "variable,name=flame_shock_saturated,value=((active_dot.flame_shock=active_enemies)|(active_dot.flame_shock=6))" );

  // Default Actions
  def->add_action( this, "Bloodlust", "line_cd=600" );
  def->add_action( this, "Wind Shear", "", "Interrupt of casts." );
  def->add_action( "auto_attack" );
  def->add_action( "call_action_list,name=single_sb,if=active_enemies=1&!talent.surging_totem.enabled" );
  def->add_action( "call_action_list,name=single_totemic,if=active_enemies=1&talent.surging_totem.enabled" );
  def->add_action( "call_action_list,name=aoe,if=active_enemies>1" );

  // AOE
  aoe->add_action( "voltaic_blaze,if=talent.surging_totem.enabled&dot.flame_shock.remains=0" );
  aoe->add_action( "flame_shock,if=!ticking" );
  aoe->add_action( "surging_totem" );
  aoe->add_action( "ascendance,if=ti_chain_lightning" );
  aoe->add_action( "call_action_list,name=buffs" );
  aoe->add_action( "sundering,if=talent.surging_elements.enabled|buff.whirling_earth.up" );
  aoe->add_action( "lava_lash,if=buff.whirling_fire.up" );
  aoe->add_action( "doom_winds" );
  aoe->add_action( "crash_lightning,if=talent.thorims_invocation.enabled&buff.whirling_air.up&(buff.doom_winds.up|buff.ascendance.up)" );
  aoe->add_action( "windstrike,if=talent.thorims_invocation.enabled&buff.whirling_air.up" );
  aoe->add_action( "stormstrike,if=talent.thorims_invocation.enabled&buff.doom_winds.up&buff.whirling_air.up" );
  aoe->add_action( "lava_lash,if=talent.splitstream.enabled&buff.hot_hand.up" );
  aoe->add_action( "tempest,if=buff.maelstrom_weapon.stack>=10&(!buff.ascendance.up|!buff.doom_winds.up)" );
  aoe->add_action( "primordial_storm,if=buff.maelstrom_weapon.stack>=10" );
  aoe->add_action( "crash_lightning,if=talent.thorims_invocation.enabled&(buff.doom_winds.up|buff.ascendance.up)&talent.splitstream.enabled&buff.hot_hand.up" );
  aoe->add_action( "windstrike,if=talent.thorims_invocation.enabled&talent.splitstream.enabled&buff.hot_hand.up" );
  aoe->add_action( "stormstrike,if=talent.thorims_invocation.enabled&buff.doom_winds.up&talent.splitstream.enabled&buff.hot_hand.up" );
  aoe->add_action( "chain_lightning,if=buff.maelstrom_weapon.stack>=(9+1*talent.surging_totem.enabled)&talent.splitstream.enabled&buff.hot_hand.up" );
  aoe->add_action( "voltaic_blaze,if=talent.fire_nova.enabled" );
  aoe->add_action( "crash_lightning" );
  aoe->add_action( "windstrike,if=talent.thorims_invocation.enabled" );
  aoe->add_action( "stormstrike,if=talent.thorims_invocation.enabled&buff.doom_winds.up" );
  aoe->add_action( "chain_lightning,if=buff.maelstrom_weapon.stack>=(9+1*talent.surging_totem.enabled)" );
  aoe->add_action( "sundering,if=talent.feral_spirit.enabled" );
  aoe->add_action( "voltaic_blaze" );
  aoe->add_action( "lava_lash,if=pet.searing_totem.active" );
  aoe->add_action( "windstrike" );
  aoe->add_action( "stormstrike,if=charges_fractional>=1.8|buff.converging_storms.stack=buff.converging_storms.max_stack" );
  aoe->add_action( "sundering,if=cooldown.surging_totem.remains>25" );
  aoe->add_action( "stormstrike,if=!talent.surging_totem.enabled" );
  aoe->add_action( "lava_lash" );
  aoe->add_action( "stormstrike" );
  aoe->add_action( "chain_lightning,if=buff.maelstrom_weapon.stack>=5" );
  aoe->add_action( "flame_shock" );

  // Buffs
  buffs->add_action( "use_item,name=algethar_puzzle_box,if=(talent.ascendance.enabled&(cooldown.ascendance.remains<2*gcd.max))|(talent.doom_winds.enabled&!talent.ascendance.enabled&(cooldown.doom_winds.remains<2*gcd.max))|(fight_remains%%120<=20)" );
  buffs->add_action( "use_item,name=unyielding_netherprism,if=(talent.ascendance.enabled&(cooldown.ascendance.remains<2*gcd.max))|(talent.doom_winds.enabled&!talent.ascendance.enabled&(cooldown.doom_winds.remains<2*gcd.max))|fight_remains<=20" );
  buffs->add_action( "use_item,slot=trinket1,if=!variable.trinket1_is_weird&((buff.ascendance.up|buff.doom_winds.up|pet.surging_totem.active|(fight_remains=20)|(!talent.ascendance.enabled&!talent.doom_winds.enabled&!talent.surging_totem.enabled)))|!trinket.1.has_use_buff" );
  buffs->add_action( "use_item,slot=trinket2,if=!variable.trinket2_is_weird&((buff.ascendance.up|buff.doom_winds.up|pet.surging_totem.active|(fight_remains=20)|(!talent.ascendance.enabled&!talent.doom_winds.enabled&!talent.surging_totem.enabled)))|!trinket.2.has_use_buff" );
  buffs->add_action( "potion,if=(buff.ascendance.up|buff.doom_winds.up|pet.surging_totem.active|(fight_remains%%300<=30)|(!talent.ascendance.enabled&!talent.doom_winds.enabled&!talent.surging_totem.enabled))" );
  buffs->add_action( "blood_fury,if=(buff.ascendance.up|buff.doom_winds.up|pet.surging_totem.active|(fight_remains%%action.blood_fury.cooldown<=action.blood_fury.duration)|(!talent.ascendance.enabled&!talent.doom_winds.enabled&!talent.surging_totem.enabled))" );
  buffs->add_action( "berserking,if=(buff.ascendance.up|buff.doom_winds.up|pet.surging_totem.active|(fight_remains%%action.berserking.cooldown<=action.berserking.duration)|(!talent.ascendance.enabled&!talent.doom_winds.enabled&!talent.surging_totem.enabled))" );
  buffs->add_action( "fireblood,if=(buff.ascendance.up|buff.doom_winds.up|pet.surging_totem.active|(fight_remains%%action.fireblood.cooldown<=action.fireblood.duration)|(!talent.ascendance.enabled&!talent.doom_winds.enabled&!talent.surging_totem.enabled))" );
  buffs->add_action( "ancestral_call,if=(buff.ascendance.up|buff.doom_winds.up|pet.surging_totem.active|(fight_remains%%action.ancestral_call.cooldown<=action.ancestral_call.duration)|(!talent.ascendance.enabled&!talent.doom_winds.enabled&!talent.surging_totem.enabled))" );
  buffs->add_action( "invoke_external_buff,name=power_infusion,if=((talent.deeply_rooted_elements.enabled&buff.ascendance.remains>7.5)|(!talent.deeply_rooted_elements.enabled&(buff.ascendance.up|buff.doom_winds.up|pet.surging_totem.active))|(fight_remains%%120<=20)|(!talent.ascendance.enabled&!talent.doom_winds.enabled&!talent.surging_totem.enabled))" );

  // Stormbringer Single Target
  single_sb->add_action( "primordial_storm,if=(buff.maelstrom_weapon.stack>=9|buff.primordial_storm.remains<=4&buff.maelstrom_weapon.stack>=5)" );
  single_sb->add_action( "voltaic_blaze,if=dot.flame_shock.remains=0&time<5" );
  single_sb->add_action( "flame_shock,if=!ticking" );
  single_sb->add_action( "lava_lash,if=!debuff.lashing_flames.up&time<5" );
  single_sb->add_action( "call_action_list,name=buffs" );
  single_sb->add_action( "sundering,if=talent.surging_elements.enabled|talent.feral_spirit.enabled" );
  single_sb->add_action( "doom_winds" );
  single_sb->add_action( "crash_lightning,if=!buff.crash_lightning.up|talent.storm_unleashed.enabled" );
  single_sb->add_action( "voltaic_blaze,if=(buff.doom_winds.up&buff.maelstrom_weapon.stack>=10-(1+2*talent.fire_nova.enabled)&!buff.maelstrom_weapon.stack=10)&talent.thorims_invocation.enabled" );
  single_sb->add_action( "windstrike,if=buff.maelstrom_weapon.stack>0&talent.thorims_invocation.enabled" );
  single_sb->add_action( "ascendance" );
  single_sb->add_action( "stormstrike,if=buff.doom_winds.up&talent.thorims_invocation.enabled" );
  single_sb->add_action( "crash_lightning,if=buff.doom_winds.up&talent.thorims_invocation.enabled" );
  single_sb->add_action( "tempest,if=buff.maelstrom_weapon.stack=10" );
  single_sb->add_action( "lightning_bolt,if=buff.maelstrom_weapon.stack=10" );
  single_sb->add_action( "stormstrike,if=charges_fractional>=1.8" );
  single_sb->add_action( "lava_lash" );
  single_sb->add_action( "stormstrike" );
  single_sb->add_action( "voltaic_blaze" );
  single_sb->add_action( "sundering" );
  single_sb->add_action( "lightning_bolt,if=buff.maelstrom_weapon.stack>=8" );
  single_sb->add_action( "crash_lightning" );
  single_sb->add_action( "lightning_bolt,if=buff.maelstrom_weapon.stack>=5" );
  single_sb->add_action( "flame_shock" );

  // Totemic Single Target
  single_totemic->add_action( "voltaic_blaze,if=dot.flame_shock.remains=0" );
  single_totemic->add_action( "flame_shock,if=!ticking" );
  single_totemic->add_action( "surging_totem" );
  single_totemic->add_action( "call_action_list,name=buffs" );
  single_totemic->add_action( "sundering,if=talent.surging_elements.enabled|buff.whirling_earth.up|talent.feral_spirit.enabled" );
  single_totemic->add_action( "lava_lash,if=buff.whirling_fire.up|buff.hot_hand.up" );
  single_totemic->add_action( "doom_winds" );
  single_totemic->add_action( "crash_lightning,if=!buff.crash_lightning.up|talent.storm_unleashed.enabled" );
  single_totemic->add_action( "primordial_storm,if=(buff.maelstrom_weapon.stack>=10|buff.primordial_storm.remains<3.5&buff.maelstrom_weapon.stack>=5)" );
  single_totemic->add_action( "windstrike,if=talent.thorims_invocation.enabled&buff.ascendance.up" );
  single_totemic->add_action( "ascendance,if=ti_lightning_bolt" );
  single_totemic->add_action( "crash_lightning,if=talent.thorims_invocation.enabled&buff.doom_winds.up|buff.ascendance.up" );
  single_totemic->add_action( "stormstrike,if=talent.thorims_invocation.enabled&buff.doom_winds.up" );
  single_totemic->add_action( "lightning_bolt,if=talent.elemental_tempo.enabled&(buff.maelstrom_weapon.stack>=5&(cooldown.lava_lash.remains>gcd.max)&(cooldown.lava_lash.remains<=buff.maelstrom_weapon.stack*0.3)|buff.maelstrom_weapon.stack>=10)" );
  single_totemic->add_action( "crash_lightning,if=!buff.crash_lightning.up" );
  single_totemic->add_action( "lava_lash" );
  single_totemic->add_action( "sundering,if=cooldown.surging_totem.remains>25" );
  single_totemic->add_action( "stormstrike" );
  single_totemic->add_action( "voltaic_blaze" );
  single_totemic->add_action( "crash_lightning" );
  single_totemic->add_action( "lightning_bolt,if=buff.maelstrom_weapon.stack>=5" );
  single_totemic->add_action( "flame_shock" );

// def->add_action( "call_action_list,name=opener" );
}
// shaman_t::init_action_list_restoration ===================================

void shaman_t::init_action_list_restoration_dps()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );
  action_priority_list_t* def       = get_action_priority_list( "default" );

  // Grabs whatever Elemental is using
  precombat->add_action( this, "Earth Elemental" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "potion" );

  // Default APL
  def->add_action( this, "Spiritwalker's Grace", "moving=1,if=movement.distance>6" );
  def->add_action( this, "Wind Shear", "", "Interrupt of casts." );
  def->add_action( "potion" );
  def->add_action( "use_items" );
  def->add_action( this, "Flame Shock", "if=!ticking" );
  def->add_action( this, "Earth Elemental" );

  // Racials
  def->add_action( "blood_fury" );
  def->add_action( "berserking" );
  def->add_action( "fireblood" );
  def->add_action( "ancestral_call" );
  def->add_action( "bag_of_tricks" );

  def->add_action( this, "Lava Burst", "if=dot.flame_shock.remains>cast_time&cooldown_react" );
  def->add_action( this, "Lightning Bolt", "if=spell_targets.chain_lightning<3" );
  def->add_action( this, "Chain Lightning", "if=spell_targets.chain_lightning>2" );
  def->add_action( this, "Flame Shock", "moving=1" );
  def->add_action( this, "Frost Shock", "moving=1" );
}

// shaman_t::init_blizzard_action_list ======================================

void shaman_t::init_blizzard_action_list()
{
  parse_player_effects_t::init_blizzard_action_list();

  if ( !use_cds_with_blizzard_action_list )
  {
    return;
  }

  action_priority_list_t* cooldowns = get_action_priority_list( "cooldowns" );

  cooldowns->add_action( "ascendance" );
  if ( specialization() == SHAMAN_ENHANCEMENT )
  {
    cooldowns->add_action( "doom_winds" );
  }
}


// shaman_t::init_actions ===================================================

void shaman_t::init_action_list()
{
  if ( quiet )
    return;

  if ( !action_list_str.empty() )
  {
    parse_player_effects_t::init_action_list();
    return;
  }

  clear_action_priority_lists();

  switch ( specialization() )
  {
    case SHAMAN_ENHANCEMENT:
      init_action_list_enhancement();
      break;
    case SHAMAN_ELEMENTAL:
      is_ptr() ? shaman_apl::elemental_ptr( this ) : shaman_apl::elemental( this );
      break;
    case SHAMAN_RESTORATION:
      init_action_list_restoration_dps();
      break;
    default:
      break;
  }

  use_default_action_list = true;

  parse_player_effects_t::init_action_list();
}

// shaman_t::action_names_from_spell_id ===================================================

std::vector<std::string> shaman_t::action_names_from_spell_id( unsigned int spell_id ) const
{
  return parse_player_effects_t::action_names_from_spell_id( spell_id );
}

// shaman_t::parse_assisted_combat_rule ===================================================

parsed_assisted_combat_rule_t shaman_t::parse_assisted_combat_rule( const assisted_combat_rule_data_t& rule,
                                                                    const assisted_combat_step_data_t& step ) const
{
  // 2026-02-15 Buff (Voltaic Blaze) no longer exists but it still referenced in the client data
  if ( rule.condition_value_1 == 470058 )
  {
    return { "0" };
  }

  if ( step.spell_id == 318038 && rule.condition_type == AC_AURA_ON_PLAYER && rule.condition_value_1 == 382027 )
    return { "talent.flametongue_weapon" };
  if ( rule.condition_type == AC_AURA_ON_PLAYER && rule.condition_value_1 == 384087 )
    return { "0" };
  if ( rule.condition_type == AC_AURA_MISSING_PLAYER && rule.condition_value_1 == 384087 )
    return { "1" };
  return parse_player_effects_t::parse_assisted_combat_rule( rule, step );
}

void shaman_t::parse_assisted_combat_step( const assisted_combat_step_data_t& step,
                                         action_priority_list_t* assisted_combat )
{
  if ( step.spell_id == 462854 )
    return;

  auto replace_spell = [ & ]( unsigned source_spell_id, unsigned target_spell_id ) {
    if ( step.spell_id == source_spell_id )
    {
      assisted_combat_step_data_t custom_step = step;
      custom_step.spell_id                    = target_spell_id;
      player_t::parse_assisted_combat_step( custom_step, assisted_combat );
      return true;
    }

    return false;
  };

  auto conditionally_replace_spell = [ & ]( unsigned source_spell_id, unsigned target_spell_id,
                                          assisted_combat_rule_e rule_type, unsigned rule_value ) {
    if ( step.spell_id == source_spell_id )
    {
      for ( const auto& rule : assisted_combat_rule_data_t::data( step.id, true ) )
      {
        if ( rule.condition_type == rule_type && rule.condition_value_1 == rule_value )
        {
          return replace_spell( source_spell_id, target_spell_id );
        }
      }
    }

    return false;
  };

  if ( conditionally_replace_spell( 188196, 454009, AC_AURA_ON_PLAYER, 454015 ) )
  {
    return;
  }

  if ( conditionally_replace_spell( 197214, 1218090, AC_AURA_ON_PLAYER, 1218125 ) )
  {
    return;
  }

  // Make Lightning bolt equivalent lines for tempest
  if ( step.spell_id == 188196 )
  {
    auto custom_step = step;
    custom_step.spell_id = 452201;
    player_t::parse_assisted_combat_step( custom_step, assisted_combat );
  }

  // Add Windstrike lines by copying Stormstrike lines
  if ( step.spell_id == 17364 )
  {
    auto custom_step = step;
    custom_step.spell_id = 115356;
    player_t::parse_assisted_combat_step( custom_step, assisted_combat );
  }

  player_t::parse_assisted_combat_step( step, assisted_combat );
}

// shaman_t::moving =========================================================

void shaman_t::moving()
{
  // Spiritwalker's Grace complicates things, as you can cast it while casting
  // anything. So, to model that, if a raid move event comes, we need to check
  // if we can trigger Spiritwalker's Grace. If so, conditionally execute it, to
  // allow the currently executing cast to finish.
  if ( true_level >= 85 )
  {
    action_t* swg = find_action( "spiritwalkers_grace" );

    // We need to bypass swg -> ready() check here, so whip up a special
    // readiness check that only checks for player skill, cooldown and resource
    // availability
    if ( swg && executing && swg->ready() )
    {
      // Shaman executes SWG mid-cast during a movement event, if
      // 1) The profile is casting Lava Burst (without Lava Surge)
      // 2) The profile is casting Chain Lightning
      // 3) The profile is casting Lightning Bolt
      if ( ( executing->id == 51505 ) || ( executing->id == 421 ) || ( executing->id == 403 ) )
      {
        if ( sim->log )
          sim->out_log.printf( "%s spiritwalkers_grace during spell cast, next cast (%s) should finish", name(),
                               executing->name() );
        swg->execute();
      }
    }
    else
    {
      interrupt();
    }

    if ( main_hand_attack )
      main_hand_attack->cancel();
    if ( off_hand_attack )
      off_hand_attack->cancel();
  }
  else
  {
    halt();
  }
}

double shaman_t::composite_attribute( attribute_e attr ) const
{
  auto a = player_t::composite_attribute( attr );

  if ( attr == ATTR_STR_AGI_INT )
  {
    switch ( specialization() )
    {
      case SHAMAN_ELEMENTAL:
      case SHAMAN_ENHANCEMENT:
        if ( buff.lightning_shield->check() )
          a += dbc->race_base( race ).strength +
               dbc->attribute_base( type, level() ).intellect * buff.lightning_shield->data().effectN(4).base_value();
        break;
      default:
        break;
    }
  }

  return a;
}

// shaman_t::composite_player_target_multiplier ==============================

double shaman_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = parse_player_effects_t::composite_player_target_multiplier( target, school );
  return m;
}

double shaman_t::composite_player_critical_damage_multiplier( const action_state_t* s, school_e school ) const
{
  double m = parse_player_effects_t::composite_player_critical_damage_multiplier( s, school );

  if ( talent.overcharge.ok() && dbc::is_school( school, SCHOOL_NATURE ) )
  {
    double crit_chance = 0.0;
    switch ( s->action->type )
    {
      case ACTION_ATTACK:
        crit_chance = cache.attack_crit_chance() * composite_melee_crit_chance_multiplier();
        break;
      case ACTION_SPELL:
        crit_chance = cache.spell_crit_chance() * composite_spell_crit_chance_multiplier();
        break;
      default:
        break;
    }

    // Multiply by half to get the correct value since this is applied to player damage bonus
    m *= 1.0 + talent.overcharge->effectN( 2 ).percent() * crit_chance * 0.5;
  }

  return m;
}


// shaman_t::invalidate_cache ===============================================

void shaman_t::invalidate_cache( cache_e c )
{
  parse_player_effects_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_AGILITY:
    case CACHE_STRENGTH:
    case CACHE_ATTACK_POWER:
      if ( specialization() == SHAMAN_ENHANCEMENT )
        parse_player_effects_t::invalidate_cache( CACHE_SPELL_POWER );
      break;
    case CACHE_MASTERY:
      if ( specialization() == SHAMAN_ELEMENTAL )
      {
        parse_player_effects_t::invalidate_cache( CACHE_PET_DAMAGE_MULTIPLIER );
        parse_player_effects_t::invalidate_cache( CACHE_GUARDIAN_DAMAGE_MULTIPLIER );
      }

      break;
    default:
      break;
  }
}

// shaman_t::reset ==========================================================

void shaman_t::reset()
{
  parse_player_effects_t::reset();

  lava_surge_during_lvb = false;
  sk_during_cast        = false;

  ls_counter = 0U;
  dre_attempts = 0U;
  aws_counter                    = 0U;
  lava_surge_attempts_normalized = 0.0;
  action.ti_trigger = nullptr;

  pet.all_wolves.clear();

  earthen_rage_target = nullptr;
  earthen_rage_event = nullptr;

  if ( specialization() == SHAMAN_ELEMENTAL && talent.rolling_thunder.ok() )
  {
    rt_last_trigger = -timespan_t::from_seconds( talent.rolling_thunder->effectN( 1 ).base_value() );
  }

  assert( active_flame_shock.empty() );
  assert( buff_state_lightning_rod == 0U );
  assert( buff_state_lashing_flames == 0U );

  for ( auto it : active_wolf_expr_cache )
  {
    std::get<0>( it.second ) = timespan_t::min();
    std::get<1>( it.second ) = 0.0;
  }
}

// shaman_t::merge ==========================================================

void shaman_t::merge( player_t& other )
{
  parse_player_effects_t::merge( other );

  const shaman_t& s = static_cast<shaman_t&>( other );

  if ( s.mw_source_list.size() > mw_source_list.size() )
  {
    mw_source_list.resize( s.mw_source_list.size() );
  }

  for ( auto i = 0U; i < s.mw_source_list.size(); ++i )
  {
    mw_source_list[ i ].first.merge( s.mw_source_list[ i ].first );
    mw_source_list[ i ].second.merge( s.mw_source_list[ i ].second );
  }

  if ( s.mw_spend_list.size() > mw_spend_list.size() )
  {
    mw_spend_list.resize( s.mw_spend_list.size() );
  }

  for ( auto i = 0U; i < s.mw_spend_list.size(); ++i )
  {
    for ( auto j = 0U; j < s.mw_spend_list[ i ].size(); ++j )
    {
      mw_spend_list[ i ][ j ].merge( s.mw_spend_list[ i ][ j ] );
    }
  }

  if ( talent.deeply_rooted_elements.ok() )
  {
    dre_samples.merge( s.dre_samples );
    dre_uptime_samples.merge( s.dre_uptime_samples );
  }

  lvs_samples.merge( s.lvs_samples );
}

// shaman_t::primary_role ===================================================

role_e shaman_t::primary_role() const
{
  if ( parse_player_effects_t::primary_role() == ROLE_HEAL )
    return ROLE_HYBRID;  // To prevent spawning healing_target, as there is no support for healing.

  if ( specialization() == SHAMAN_RESTORATION )
  {
    if ( parse_player_effects_t::primary_role() == ROLE_DPS || parse_player_effects_t::primary_role() == ROLE_SPELL )
      return ROLE_SPELL;

    return ROLE_SPELL;
  }

  else if ( specialization() == SHAMAN_ENHANCEMENT )
    return ROLE_ATTACK;

  else if ( specialization() == SHAMAN_ELEMENTAL )
    return ROLE_SPELL;

  return parse_player_effects_t::primary_role();
}

// shaman_t::convert_hybrid_stat ===========================================

stat_e shaman_t::convert_hybrid_stat( stat_e s ) const
{
  switch ( s )
  {
    case STAT_STR_AGI_INT:
    case STAT_AGI_INT:
      if ( specialization() == SHAMAN_ENHANCEMENT )
        return STAT_AGILITY;
      else
        return STAT_INTELLECT;
    case STAT_STR_AGI:
      // This is a guess at how AGI/STR gear will work for Resto/Elemental, TODO: confirm
      return STAT_AGILITY;
    case STAT_STR_INT:
      // This is a guess at how STR/INT gear will work for Enhance, TODO: confirm
      // this should probably never come up since shamans can't equip plate, but....
      return STAT_INTELLECT;
    case STAT_SPIRIT:
      if ( specialization() == SHAMAN_RESTORATION )
        return s;
      else
        return STAT_NONE;
    case STAT_BONUS_ARMOR:
      return STAT_NONE;
    default:
      return s;
  }
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class shaman_report_t : public player_report_extension_t
{
private:
  shaman_t& p;

public:
  shaman_report_t( shaman_t& player ) : p( player )
  { }

  void mw_consumer_stack_header( report::sc_html_stream& os )
  {
    auto columns = std::max( p.buff.maelstrom_weapon->data().max_stacks(),
      as<unsigned>( p.talent.raging_maelstrom->effectN( 1 ).base_value() ) ) + 1;

    os << "<table class=\"sc sort\" style=\"float: left;margin-right: 10px;\">\n"
       << "<thead>\n"
       << "<tr>\n";
    os << fmt::format( "<th colspan=\"{}\"><strong>Casts per Maelstrom Weapon Stack Consumed</strong></th>\n", columns + 1 )
       << "</tr>\n"
       << "<th class=\"toggle-sort\" data-sortdir=\"asc\" data-sorttype=\"alpha\">Ability</th>\n";
    for ( auto col = 0U; col < columns; ++col )
    {
       os << fmt::format( "<th>{}</th>\n", col );
    }
    os << "</tr>\n"
       << "</thead>\n";
  }

  void mw_consumer_stack_contents( report::sc_html_stream& os )
  {
    auto columns = std::max( p.buff.maelstrom_weapon->data().max_stacks(),
      as<unsigned>( p.talent.raging_maelstrom->effectN( 1 ).base_value() ) ) + 1;

    int row = 0;
    std::vector<double> row_totals( columns, 0.0 );

    for ( auto i = 0; i < as<int>( p.mw_spend_list.size() ); ++i )
    {
      const auto& ref = p.mw_spend_list[ i ];

      auto action_sum = range::accumulate( ref, 0.0, &simple_sample_data_t::sum ) - ref[ 0 ].sum();
      if ( action_sum == 0.0 )
      {
        continue;
      }

      auto action = range::find_if( p.action_list, [ i ]( const action_t* action ) {
        return action->internal_id == i;
      } );

      os << fmt::format( "<tr class=\"{}\">\n", row++ & 1 ? "odd" : "even" );
      os << fmt::format( "<td class=\"left\">{}</td>", report_decorators::decorated_action( **action ) );

      for ( auto col = 0; col < as<int>( columns ); ++col )
      {
        auto casts = ref[ col ].sum() / ( col > 1 ? as<double>( col ) : 1.0 );

        if ( ref[ col ].sum() == 0.0 )
        {
          os << "<td class=\"left\" style=\"min-width: 5ch;\">&nbsp;</td>\n";
        }
        else
        {
          os << fmt::format( "<td class=\"left\" style=\"min-width: 5ch;\">{:.2f}</td>\n", casts );
        }

        row_totals[ col ] += casts;
      }

      os << "</tr>\n";
    }

    os << fmt::format( "<tr class=\"{}\">\n", row++ & 1 ? "odd" : "even" )
       << "<td class=\"left\"><strong>Total</strong>\n";

    auto total_sum = range::accumulate( row_totals, 0.0 );
    range::for_each( row_totals, [ &os, total_sum ]( auto row_sum ) {
      if ( row_sum == 0.0 )
      {
        os << "<td class=\"left\" style=\"min-width: 5ch;\">&nbsp;</td>\n";
      }
      else
      {
        os << fmt::format( "<td class=\"left\" style=\"min-width: 5ch;\"><strong>{:.2f}</strong><br/>({:.2f}%)</td>\n",
          row_sum, 100 * row_sum / total_sum );
      }
    } );

    os << "</tr>\n";
  }

  void mw_consumer_stack_footer( report::sc_html_stream& os )
  {
    os << "</table>\n";
  }

  void mw_consumer_header( report::sc_html_stream& os )
  {
    os << "<table class=\"sc sort\" style=\"float: left;margin-right: 10px;\">\n"
       << "<thead>\n"
       << "<tr>\n"
       << "<th colspan=\"3\"><strong>Maelstrom Weapon Consumers</strong></th>\n"
       << "</tr>\n"
       << "<tr>\n"
       << "<th class=\"toggle-sort\" data-sortdir=\"asc\" data-sorttype=\"alpha\">Ability</th>\n"
       << "<th class=\"toggle-sort\">Actual</th>\n"
       << "<th class=\"toggle-sort\">% Total</th>\n"
       << "</tr>\n"
       << "</thead>\n";
  }

  void mw_consumer_contents( report::sc_html_stream& os )
  {
      int row = 0;
      double total = 0.0;

      range::for_each( p.mw_spend_list,  [ &total ]( const auto& entry ) {
        total = range::accumulate( entry, total, &simple_sample_data_t::sum ) - entry[ 0 ].sum();
      } );

      for ( auto i = 0; i < as<int>( p.mw_spend_list.size() ); ++i )
      {
        const auto& ref = p.mw_spend_list[ i ];

        auto action = range::find_if( p.action_list, [ i ]( const action_t* action ) {
          return action->internal_id == i;
        } );

        auto action_sum = range::accumulate( ref, 0.0, &simple_sample_data_t::sum ) - ref[ 0 ].sum();

        if ( action_sum == 0.0 )
        {
          continue;
        }

        os << fmt::format( "<tr class=\"{}\">\n", row++ & 1 ? "odd" : "even" );
        os << fmt::format( "<td class=\"left\">{}</td>", report_decorators::decorated_action( **action ) );
        os << fmt::format( "<td class=\"left\">{:.1f}</td>", action_sum );
        os << fmt::format( "<td class=\"left\">{:.2f}%</td>", 100.0 * action_sum / total );
        os << "</tr>\n";
      }

      os << fmt::format( "<tr class=\"{}\">\n", row++ & 1 ? "odd" : "even" );
      os << fmt::format( "<td class=\"left\"><strong>Total Spent</strong></td>" );
      os << fmt::format( "<td class=\"left\">{:.1f}</td>", total );
      os << fmt::format( "<td class=\"left\">{:.2f}%</td>", 100.0 );
  }

  void mw_consumer_piechart_contents( report::sc_html_stream& os )
  {
    highchart::pie_chart_t mw_cons( highchart::build_id( p, "mw_con" ), *p.sim );
    mw_cons.set_title( "Maelstrom Weapon Consumers" );
    mw_cons.set( "plotOptions.pie.dataLabels.format", "{point.name}: {point.y:.1f}" );

    std::vector<std::pair<action_t*, double>> processed_data;

    for ( size_t i = 0; i < p.mw_spend_list.size(); ++i )
    {
      const auto& entry = p.mw_spend_list[ i ];

      auto sum = range::accumulate( entry, 0.0, &simple_sample_data_t::sum ) - entry[ 0 ].sum();
      if ( sum == 0.0 )
      {
        continue;
      }

      auto action_it = range::find_if( p.action_list, [ i ]( const action_t* action ) {
        return action->internal_id == as<int>( i );
      } );

      processed_data.emplace_back( *action_it, sum );
    }

    range::sort( processed_data, []( const auto& left, const auto& right ) {
      if ( left.second == right.second )
      {
        return left.first->name_str < right.first->name_str;
      }

      return left.second > right.second;
    } );

    range::for_each( processed_data, [ this, &mw_cons ]( const auto& entry ) {
      color::rgb color = color::school_color( entry.first->school );

      js::sc_js_t e;
      e.set( "color", color.str() );
      e.set( "y", util::round( entry.second, p.sim->report_precision ) );
      e.set( "name", report_decorators::decorate_html_string(
          util::encode_html( entry.first->name_str ), color ) );

      mw_cons.add( "series.0.data", e );
    } );

    os << mw_cons.to_target_div();
    p.sim->add_chart_data( mw_cons );
  }

  void mw_consumer_footer( report::sc_html_stream& os )
  {
    os << "</table>\n";
  }

  void mw_generator_header( report::sc_html_stream& os )
  {
    os << "<table class=\"sc sort even\" style=\"float: left;margin-right: 10px;\">\n"
       << "<thead>\n"
       << "<tr>\n"
       << "<th colspan=\"5\"><strong>Maelstrom Weapon Sources</strong></th>\n"
       << "</tr>\n"
       << "<tr>\n"
       << "<th class=\"toggle-sort\" data-sortdir=\"asc\" data-sorttype=\"alpha\">Ability</th>\n"
       << "<th class=\"toggle-sort\">Actual</th>\n"
       << "<th class=\"toggle-sort\">Overflow</th>\n"
       << "<th class=\"toggle-sort\">% Actual</th>\n"
       << "<th class=\"toggle-sort\">% Overflow</th>\n"
       << "</tr>\n"
       << "</thead>\n";
  }

  void mw_generator_piechart_contents( report::sc_html_stream& os )
  {
    highchart::pie_chart_t mw_src( highchart::build_id( p, "mw_src" ), *p.sim );
    mw_src.set_title( "Maelstrom Weapon Sources" );
    mw_src.set( "plotOptions.pie.dataLabels.format", "{point.name}: {point.y:.1f}" );

    double overflow = 0.0;
    std::vector<std::pair<action_t*, double>> processed_data;

    for ( size_t i = 0; i < p.mw_source_list.size(); ++i )
    {
      const auto& entry = p.mw_source_list[ i ];
      overflow += entry.second.sum();

      if ( entry.first.sum() == 0.0 )
      {
        continue;
      }

      auto action_it = range::find_if( p.action_list, [ i ]( const action_t* action ) {
        return action->internal_id == as<int>( i );
      } );

      processed_data.emplace_back( *action_it, entry.first.sum() );
    }

    range::sort( processed_data, []( const auto& left, const auto& right ) {
      if ( left.second == right.second )
      {
        return left.first->name_str < right.first->name_str;
      }

      return left.second > right.second;
    } );

    range::for_each( processed_data, [ this, &mw_src ]( const auto& entry ) {
      color::rgb color = color::school_color( entry.first->school );

      js::sc_js_t e;
      e.set( "color", color.str() );
      e.set( "y", util::round( entry.second, p.sim->report_precision ) );
      e.set( "name", report_decorators::decorate_html_string(
          util::encode_html( entry.first->name_str ), color ) );

      mw_src.add( "series.0.data", e );
    } );

    if ( overflow > 0.0 )
    {
      js::sc_js_t e;
      e.set( "color", color::WHITE.str() );
      e.set( "y", util::round( overflow, p.sim->report_precision ) );
      e.set( "name", "overflow" );
      mw_src.add( "series.0.data", e );
    }

    os << mw_src.to_target_div();
    p.sim->add_chart_data( mw_src );
  }

  void mw_generator_contents( report::sc_html_stream& os )
  {
      int row = 0;
      std::string row_class_str;
      double actual = 0.0, overflow = 0.0;

      range::for_each( p.mw_source_list,  [ &actual, &overflow ]( const auto& entry ) {
        actual += entry.first.sum();
        overflow += entry.second.sum();
      } );

      for ( auto i = 0; i < as<int>( p.mw_source_list.size() ); ++i )
      {
        const auto& ref = p.mw_source_list[ i ];

        if ( ref.first.sum() == 0.0 && ref.second.sum() == 0.0 )
        {
          continue;
        }

        auto action = range::find_if( p.action_list, [ i ]( const action_t* action ) {
          return action->internal_id == i;
        } );

        os << fmt::format( "<tr class=\"{}\">\n", row++ & 1 ? "odd" : "even" );
        os << fmt::format( "<td class=\"left\">{}</td>", report_decorators::decorated_action( **action ) );
        os << fmt::format( "<td class=\"left\">{:.1f}</td>", ref.first.sum() );
        os << fmt::format( "<td class=\"left\">{:.1f}</td>", ref.second.sum() );
        os << fmt::format( "<td class=\"left\">{:.2f}%</td>",
                          100.0 * ref.first.sum() / actual );
        os << fmt::format( "<td class=\"left\">{:.2f}%</td>",
                          100.0 * ref.second.sum() / overflow );
        os << "</tr>\n";
      }

      os << fmt::format( "<tr class=\"{}\">\n", row++ & 1 ? "odd" : "even" );
      os << fmt::format( "<td class=\"left\"><strong>Overflow Stacks</strong></td>" );
      os << fmt::format( "<td class=\"left\">{:.1f}</td>", 0.0 );
      os << fmt::format( "<td class=\"left\">{:.1f}</td>", overflow );
      os << fmt::format( "<td class=\"left\">{:.2f}%</td>", 0.0 );
      os << fmt::format( "<td class=\"left\">{:.2f}%</td>", 100.0 * overflow / ( actual + overflow ) );

      os << fmt::format( "<tr class=\"{}\">\n", row++ & 1 ? "odd" : "even" );
      os << fmt::format( "<td class=\"left\"><strong>Actual Stacks</strong></td>" );
      os << fmt::format( "<td class=\"left\">{:.1f}</td>", actual );
      os << fmt::format( "<td class=\"left\">{:.1f}</td>", 0.0 );
      os << fmt::format( "<td class=\"left\">{:.2f}%</td>", 100.0 * actual / ( actual + overflow ) );
      os << fmt::format( "<td class=\"left\">{:.2f}%</td>", 0.0 );
  }

  void mw_generator_footer( report::sc_html_stream& os )
  {
    os << "</table>\n";
  }

  void dre_uptime_distribution_contents( report::sc_html_stream& os )
  {
    highchart::histogram_chart_t chart( highchart::build_id( p, "dre_uptime" ), *p.sim );

    chart.set( "plotOptions.column.color", color::GREY3.str() );
    chart.set( "plotOptions.column.pointStart", std::floor( p.dre_uptime_samples.min() ) );
    chart.set_title( fmt::format( "Ascendance Iteration Uptime% (min={:.2f}% median={:.2f}% max={:.2f}%)",
                                 p.dre_uptime_samples.min(),
                                 p.dre_uptime_samples.percentile( 0.5 ),
                                 p.dre_uptime_samples.max() ) );
    chart.set( "yAxis.title.text", "# of Iterations" );
    chart.set( "xAxis.title.text", "Uptime%" );
    chart.set( "series.0.name", "# of Iterations" );

    range::for_each( p.dre_uptime_samples.distribution, [ &chart ]( size_t n ) {
      js::sc_js_t e;

      e.set( "y", static_cast<double>( n ) );

      chart.add( "series.0.data", e );
    } );

    os << chart.to_target_div();
    p.sim->add_chart_data( chart );
  }

  void dre_proc_distribution_contents( report::sc_html_stream& os )
  {
    highchart::histogram_chart_t chart( highchart::build_id( p, "dre" ), *p.sim );

    chart.set( "plotOptions.column.color", color::RED.str() );
    chart.set( "plotOptions.column.pointStart", 1 );
    chart.set_title( fmt::format( "DRE Attempts (min={} median={} max={})", p.dre_samples.min(),
                                 p.dre_samples.percentile( 0.5 ), p.dre_samples.max() ) );
    chart.set( "yAxis.title.text", "# of Triggered Procs" );
    chart.set( "xAxis.title.text", "Proc on Attempt #" );
    chart.set( "series.0.name", "Triggered Procs" );

    range::for_each( p.dre_samples.distribution, [ &chart ]( size_t n ) {
      js::sc_js_t e;

      e.set( "y", static_cast<double>( n ) );

      chart.add( "series.0.data", e );
    } );

    os << chart.to_target_div();
    p.sim->add_chart_data( chart );
  }

  void lvs_proc_distribution_contents( report::sc_html_stream& os )
  {
    highchart::histogram_chart_t chart( highchart::build_id( p, "lvs" ), *p.sim );

    chart.set( "plotOptions.column.color", color::RED.str() );
    chart.set( "plotOptions.column.pointStart", 0 );
    chart.set_title( fmt::format( "LVS Attempts (min={} median={} max={})", p.lvs_samples.min(),
                                  p.lvs_samples.percentile( 0.5 ), p.lvs_samples.max() ) );
    chart.set( "yAxis.title.text", "# of Triggered Procs" );
    chart.set( "xAxis.title.text", "Proc on Attempt #" );
    chart.set( "series.0.name", "Triggered Procs" );

    range::for_each( p.lvs_samples.distribution, [ &chart ]( size_t n ) {
      js::sc_js_t e;

      e.set( "y", static_cast<double>( n ) );

      chart.add( "series.0.data", e );
    } );

    os << chart.to_target_div();
    p.sim->add_chart_data( chart );
  }

  void html_customsection( report::sc_html_stream& os ) override
  {
    if ( p.tracker.has_data() )
    {
      os << "\t\t\t\t<div class=\"player-section custom_section\">\n";
      os << "\t\t\t\t\t<h3 class=\"toggle open\">Proc Details</h3>\n"
          << "\t\t\t\t\t<div class=\"toggle-content\">\n";

      p.tracker.output_html( os );

      os << "\t\t\t\t\t</div>\n";

      os << "<div class=\"clear\"></div>\n";

      os << "\t\t\t\t\t</div>\n";
    }

    // Custom Class Section
    if ( p.talent.maelstrom_weapon.ok() )
    {
      os << "\t\t\t\t<div class=\"player-section custom_section\">\n";
      os << "\t\t\t\t\t<h3 class=\"toggle open\">Maelstrom Weapon Details</h3>\n"
         << "\t\t\t\t\t<div class=\"toggle-content\">\n";

      mw_generator_header( os );
      mw_generator_contents( os );
      mw_generator_piechart_contents( os );
      mw_generator_footer( os );

      os << "<div class=\"clear\"></div>\n";

      mw_consumer_header( os );
      mw_consumer_contents( os );
      mw_consumer_footer( os );

      mw_consumer_stack_header( os );
      mw_consumer_stack_contents( os );
      mw_consumer_stack_footer( os );

      os << "<div class=\"clear\"></div>\n";

      mw_consumer_piechart_contents( os );

      os << "\t\t\t\t\t</div>\n";

      os << "<div class=\"clear\"></div>\n";

      os << "\t\t\t\t\t</div>\n";
    }

    if ( p.talent.deeply_rooted_elements.ok() )
    {
      os << "\t\t\t\t<div class=\"player-section custom_section\">\n";
      os << "\t\t\t\t\t<h3 class=\"toggle open\">Deeply Rooted Elements Proc Details</h3>\n"
         << "\t\t\t\t\t<div class=\"toggle-content\">\n";

      dre_proc_distribution_contents( os );
      dre_uptime_distribution_contents( os );

      os << "\t\t\t\t\t</div>\n";

      os << "<div class=\"clear\"></div>\n";

      os << "\t\t\t\t\t</div>\n";
    }

    if ( p.spec.lava_surge->ok() )
    {
      lvs_proc_distribution_contents( os );
    }
  }
};

// SHAMAN MODULE INTERFACE ==================================================

struct shaman_module_t : public module_t
{
  shaman_module_t() : module_t( SHAMAN )
  {
  }

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p              = new shaman_t( sim, name, r );
    p->report_extension = std::unique_ptr<player_report_extension_t>( new shaman_report_t( *p ) );
    return p;
  }

  bool valid() const override
  {
    return true;
  }

  void register_actor_initializers( sim_t* sim ) const override
  {
    sim->register_actor_initializer( INIT_ACTOR_CREATE_BUFFS + offset(), []( player_t* p ) {
      p->buffs.bloodlust = make_buff( p, "bloodlust", p->find_spell( 2825 ) )
          ->set_cooldown( 0_ms )
          ->set_max_stack( 1 )
          ->set_default_value_from_effect_type( A_HASTE_ALL )
          ->add_invalidate( CACHE_HASTE );

      p->buffs.exhaustion = make_buff( p, "exhaustion", p->find_spell( 57723 ) )
          ->set_max_stack( 1 )
          ->set_quiet( true );
    }, "create_buffs_shaman" );
  }

  void register_hotfixes() const override
  {

    hotfix::register_effect( "Shaman", "2025-10-19", "Manually add Label to Enhancement 12.0 4PC set bonus", 1276472 )
      .field( "misc_value2" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 5779 )
      .verification_value( 0 );
  }
};

shaman_t::pets_t::pets_t( shaman_t* s ) :
    fire_elemental( "fire_elemental", s, []( shaman_t* s ) {
      return new pet::fire_elemental_t( s,
        s->talent.primal_elementalist.ok() ? elemental::PRIMAL_FIRE : elemental::GREATER_FIRE);
    } ),

    storm_elemental( "storm_elemental", s, []( shaman_t* s ) {
      return new pet::storm_elemental_t( s,
        s->talent.primal_elementalist.ok() ? elemental::PRIMAL_STORM : elemental::GREATER_STORM);
    } ),

    earth_elemental( "earth_elemental", s, []( shaman_t* s ) {
      return new pet::earth_elemental_t( s,
        s->talent.primal_elementalist.ok() ? elemental::PRIMAL_EARTH : elemental::GREATER_EARTH);
    } ),

    ancestor( "ancestor", s, []( shaman_t* s ) { return new pet::ancestor_t( s ); } ),

    fire_wolves( "fiery_wolf", s, []( shaman_t* s ) { return new pet::fire_wolf_t( s ); } ),
    lightning_wolves( "lightning_wolf", s, []( shaman_t* s ) { return new pet::lightning_wolf_t( s ); } ),

    healing_stream_totem( "healing_stream_totem", s, []( shaman_t* s ) { return new healing_stream_totem_t( s ); } ),
    capacitor_totem( "capacitor_totem", s, []( shaman_t* s ) { return new capacitor_totem_t( s ); } ),
    surging_totem( "surging_totem", s, []( shaman_t* s ) { return new surging_totem_t( s ); } ),
    searing_totem( "searing_totem", s, []( shaman_t* s ) { return new searing_totem_t( s ); } )
{
  fire_wolves.set_replacement_strategy( spawner::pet_replacement_strategy::REPLACE_OLDEST );
  lightning_wolves.set_replacement_strategy( spawner::pet_replacement_strategy::REPLACE_OLDEST );

  searing_totem.set_replacement_strategy( spawner::pet_replacement_strategy::REPLACE_OLDEST );
  searing_totem.set_max_pets( s->find_spell( 461242 )->max_stacks() );

  auto event_fn = [ s ]( spawner::pet_event_type t, pet::base_wolf_t* pet ) {
    auto it = range::find_if( s->pet.all_wolves, [ pet ]( const auto entry ) {
      return pet == entry;
    } );

    switch ( t )
    {
      case spawner::pet_event_type::ARISE:
      {
        assert( it == s->pet.all_wolves.end() );
        s->pet.all_wolves.emplace_back( pet );
        break;
      }
      case spawner::pet_event_type::DEMISE:
      {
        assert( it != s->pet.all_wolves.end() );
        s->pet.all_wolves.erase( it );
        break;
      }
      default:
        break;
    }
  };

  fire_wolves.set_event_callback( { spawner::pet_event_type::ARISE, spawner::pet_event_type::DEMISE }, event_fn );
  lightning_wolves.set_event_callback( { spawner::pet_event_type::ARISE, spawner::pet_event_type::DEMISE }, event_fn );

  surging_totem.set_max_pets( 1U );
  surging_totem.set_replacement_strategy( spawner::pet_replacement_strategy::REPLACE_OLDEST );
  surging_totem.set_creation_callback( []( shaman_t* owner ) {
    auto surging_totem = new surging_totem_t( owner );
    if ( owner->sets->has_set_bonus( HERO_TOTEMIC, TWW3, B2 ) &&
      owner->specialization() == SHAMAN_ENHANCEMENT &&
      owner->pet.surging_totem.n_pets() == 0 )
    {
      auto pstorm = debug_cast<primordial_storm_t*>( owner->action.tww3_primordial_storm );
      pstorm->fire->stats = surging_totem->get_stats( pstorm->fire->name_str, pstorm->fire );
      pstorm->frost->stats = surging_totem->get_stats( pstorm->frost->name_str, pstorm->frost );
      pstorm->nature->stats = surging_totem->get_stats( pstorm->nature->name_str, pstorm->nature );
    }
    return surging_totem;
  });
}

}  // namespace

const module_t* module_t::shaman()
{
  static ::shaman_module_t m;
  return &m;
}
