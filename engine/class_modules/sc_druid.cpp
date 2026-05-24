// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "config.hpp"

#include "action/parse_effects.hpp"
#include "player/pet_spawner.hpp"
#include "report/highchart.hpp"

#include "simulationcraft.hpp"

namespace
{  // UNNAMED NAMESPACE
// ==========================================================================
// Druid
// ==========================================================================

// Forward declarations
struct druid_t;
struct druid_spell_t;
struct druid_heal_t;
struct bear_attack_t;
struct cat_attack_t;

// ==========================================================================
// Hardcoded constants not found in spell data
// ==========================================================================
// hail of stars solstice duration for free casts
static constexpr timespan_t HAIL_OF_STARS_FREE_DURATION = 1000_ms;
// umbral embrace proc chance
static constexpr double UMBRAL_EMBRACE_PROC_CHANCE = 0.2;
// wild mushroom AP per hit
static constexpr double WILD_MUSHROOM_AP_PER_HIT = 5.0;
// lunar bolt reduced_aoe_targets
static constexpr double LUNAR_BOLT_REDUCED_AOE = 5;
// lunar bolt delay between bolts
static constexpr timespan_t LUNAR_BOLT_DELAY = 200_ms;
// summon+jump+fixate delay for fake summon spells like frantic frenzy & apex talent
static constexpr std::array<timespan_t, 2> FERAL_FLICKER_DELAY = { 400_ms, 500_ms };
// unseen attack # of targets for unseen attack to proc swipe instead of slash
static constexpr size_t UNSEEN_SWIPE_TARGETS = 3;
// unseen swipe reduced_aoe_targets
static constexpr double UNSEEN_SWIPE_REDUCED_AOE = 5;
// ursoc's fury & natural resilience cap as percentage of max hp
static constexpr double GUARDIAN_SHIELD_CAP_HP_PCT = 0.3;
// wild guardian echo delays
static constexpr std::array<timespan_t, 2> WILD_GUARDIAN_ECHO_DELAY = { 300_ms, 400_ms };

namespace pets
{
struct denizen_of_the_dream_t;
struct dread_shade_t;
struct force_of_nature_t;
struct grove_guardian_t;
struct sylvan_beckoning_t;
}

enum form_e : unsigned
{
  CAT_FORM       = 0x1,
  NO_FORM        = 0x2,
  TRAVEL_FORM    = 0x4,
  AQUATIC_FORM   = 0x8,  // Legacy
  BEAR_FORM      = 0x10,
  DIRE_BEAR_FORM = 0x40,  // Legacy
  MOONKIN_FORM   = 0x40000000,

  ANY_FORM = CAT_FORM | NO_FORM | TRAVEL_FORM | BEAR_FORM | MOONKIN_FORM
};

enum moon_stage_e
{
  NEW_MOON,
  HALF_MOON,
  FULL_MOON,
  MAX_MOON,
};

enum snapshot_e : uint8_t
{
  TIGERS_FURY        = 0x01,
};

enum flag_e : uint32_t
{
  NONE         = 0x00000000,
  FOREGROUND   = 0x00000001,  // action is directly cast from an APL line
  NOUNSHIFT    = 0x00000002,  // does not automatically unshift into caster form
  AUTOATTACK   = 0x00000004,  // is an autoattack
  ALLOWSTEALTH = 0x00000008,  // does not break stealth
  // free procs
  CONVOKE      = 0x00000100,  // convoke_the_spirits night_fae covenant ability
  FIRMAMENT    = 0x00000200,  // sundered firmament talent
  FLASHING     = 0x00000400,  // flashing claws talent
  GALACTIC     = 0x00000800,  // galactic guardian talent
  ORBIT        = 0x00001000,  // orbit breaker talent
  TWIN         = 0x00002000,  // twin moons talent
  TREANT       = 0x00004000,  // treants of the moon moonfire
  LIGHTOFELUNE = 0x00008000,  // light of elune talent
  CASCADE      = 0x00010000,  // star cascade talent
  SYLVAN       = 0x00020000,  // sylvan beckoning dryad starfall
  WILDGUARDIAN = 0x00040000,  // wild guardian echo
  CELESTIAL    = 0x00080000,  // bear mid1 4pc
};

template <typename T>
struct lockable_t
{
  T data = {};
  bool locked = false;

  operator const T&() const { return data; }
  operator T() { return data; }

  lockable_t& operator=( T o )
  { if ( !locked ) data = o; return *this; }
};

enum tracker_e
{
  SNAPSHOT_TRACKER,
  MAX_TRACKER,
};

struct benefit_tracker_t
{
  std::vector<simple_sample_data_t> direct;
  std::vector<simple_sample_data_t> tick;
  const spell_data_t& spell;

  benefit_tracker_t( const spell_data_t& s, size_t z ) : spell( s )
  {
    direct.resize( z );
    tick.resize( z );
  }

  void merge( const benefit_tracker_t& other )
  {
    for ( size_t i = 0; i < direct.size(); i++ )
      direct[ i ].merge( other.direct[ i ] );

    for ( size_t i = 0; i < tick.size(); i++ )
      tick[ i ].merge( other.tick[ i ] );
  }

  template <tracker_e T, typename DRUID>
  static benefit_tracker_t* get_tracker( DRUID p, action_t* action )
  {
    if ( p->sim->profileset_enabled )
      return nullptr;

    const auto& spell = action->stats->action_list.front()->data_reporting();

    auto it = range::find_if( p->trackers[ T ], [ id = spell.id() ]( const auto& data ) {
      return data->spell.id() == id;
    } );

    if ( it != p->trackers[ T ].end() )
      return ( *it ).get();

    auto size = benefit_tracker_t::get_type_list<T>().size();

    return p->trackers[ T ].emplace_back( std::make_unique<benefit_tracker_t>( spell, size ) ).get();
  }

  template <tracker_e T, typename DRUID>
  static void print_table( DRUID p, report::sc_html_stream& os, std::string_view title )
  {
    if ( p->trackers[ T ].empty() )
      return;

    bool ticks = true;
    auto type_list = benefit_tracker_t::get_type_list<T>();

    os.format( R"(<h3 class="toggle open">{}</h3>)", title );

    // write header
    os << R"(<div class="toggle-content"><table class="sc sort even"><thead><tr>)";

    if ( ticks )
    {
      os << "<th></th>";
      range::for_each( type_list, [ &os ]( const auto& t ) {
        os.format( R"(<th colspan="2">{}</th>)", t );
      } );
      os << "</tr>\n";

      os << R"(<tr><th class="toggle-sort left" data-sortdir="asc" data-sorttype="alpha">Ability</th>)";
      range::for_each( type_list, [ &os ]( const auto& ) {
        os << R"(<th class="toggle-sort">Direct</th><th class="toggle-sort">Tick</th>)";
      } );
    }
    else
    {
      os << R"(<tr><th class="toggle-sort left" data-sortdir="asc" data-sorttype="alpha">Ability</th>)";
      range::for_each( type_list, [ &os ]( const auto& t ) {
        os.format( R"(<th class="toggle-sort">{}</th>)", t );
      } );
    }

    os << "</tr></thead>\n";

    range::sort( p->trackers[ T ], []( const auto& l, const auto& r ) {
      return std::string_view( l->spell.name_cstr() ) < std::string_view( r->spell.name_cstr() );
    } );

    for ( const auto& data : p->trackers[ T ] )
    {
      os.format( R"(<tr class="right"><td class="left">{}</td>)",
                 report_decorators::decorated_spell_data( *p->sim, &data->spell ) );

      for ( size_t i = 0; i < type_list.size(); i++ )
      {
        os.format( "<td>{}</td>",
                   data->direct[ i ].sum() ? fmt::format( "{:.2f}%", data->direct[ i ].mean() * 100 ) : "-" );

        if ( ticks )
        {
          os.format( "<td>{}</td>",
                     data->tick[ i ].sum() ? fmt::format( "{:.2f}%", data->tick[ i ].mean() * 100 ) : "-" );
        }
      }

      os << "</tr>\n";
    }

    // write footer
    os << "</table></div>\n";
  }

  static constexpr std::array<std::string_view, 1> snapshot_list{
    { "Tiger's Fury" }
  };

  template <tracker_e T, typename U = void>
  static constexpr decltype( auto ) get_type_list()
  {
    if constexpr ( T == tracker_e::SNAPSHOT_TRACKER )
      return snapshot_list;
    else
      static_assert( static_false<U>, "Invalid tracker type." );
  }

  template <tracker_e T>
  void count_direct( unsigned val )
  {
    for ( size_t i = 0; i < direct.size(); i++ )
    {
      direct[ i ].add( ( val & ( 1U << i ) ) > 0 );
    }
  }

  template <tracker_e T>
  void count_tick( unsigned val )
  {
    for ( size_t i = 0; i < tick.size(); i++ )
    {
      tick[ i ].add( ( val & ( 1U << i ) ) > 0 );
    }
  }
};

struct druid_td_t final : public actor_target_data_t
{
  struct dots_t
  {
    dot_t* bloodseeker_vines;
    dot_t* dreadful_wound;
    dot_t* lunar_inspiration;
    dot_t* moonfire;
    dot_t* rake;
    dot_t* red_moon;
    dot_t* rip;
    dot_t* thrash;
  } dots;

  struct hots_t
  {
    dot_t* cultivation;
    dot_t* germination;
    dot_t* lifebloom;
    dot_t* regrowth;
    dot_t* rejuvenation;
    dot_t* wild_growth;
  } hots;

  struct debuffs_t
  {
    buff_t* atmospheric_exposure;
    buff_t* bloodseeker_vines;
    buff_t* sabertooth;
    buff_t* stellar_amplification;
  } debuff;

  struct buffs_t
  {
    buff_t* ironbark;
  } buff;

  druid_td_t( player_t& target, druid_t& source );

  template <typename Buff = buff_t, typename... Args>
  inline buff_t* make_debuff( bool b, Args&&... args );

  int hots_ticking() const;
};

struct druid_action_data_t  // variables that need to be accessed from action_t* pointer
{
  // various action flags
  uint32_t action_flags = 0;
  // form spell to automatically cast
  action_t* autoshift = nullptr;

  bool has_flag( uint32_t f ) const { return action_flags & f; }
  bool is_flag( flag_e f ) const { return ( action_flags & f ) == f; }
  bool is_free() const { return action_flags >> 8; }  // first 8 bits are not cost related
};


template <typename Data, typename Base = action_state_t>
struct druid_action_state_t : public Base, public Data
{
  static_assert( std::is_base_of_v<action_state_t, Base> );
  static_assert( std::is_default_constructible_v<Data> );  // required for initialize
  static_assert( std::is_copy_assignable_v<Data> );  // required for copy_state

  using Base::Base;

  void initialize() override
  {
    Base::initialize();
    *static_cast<Data*>( this ) = Data{};
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    Base::debug_str( s );
    if constexpr ( fmt::is_formattable<Data>::value )
      fmt::print( s, "{}", *static_cast<const Data*>( this ) );
    return s;
  }

  void copy_state( const action_state_t* o ) override
  {
    Base::copy_state( o );
    *static_cast<Data*>( this ) = *static_cast<const Data*>( static_cast<const druid_action_state_t*>( o ) );
  }

  void copy_data( const druid_action_state_t* o )
  {
    *static_cast<Data*>( this ) = *static_cast<const Data*>( o );
  }
};

// Static helper functions
template <typename V>
static const spell_data_t* resolve_spell_data( V data )
{
  if constexpr( std::is_invocable_v<decltype( &spell_data_t::ok ), V> )
    return data;
  else if constexpr( std::is_invocable_v<decltype( &buff_t::data ), V> )
    return &data->data();
  else if constexpr( std::is_invocable_v<decltype( &action_t::data ), V> )
    return &data->data();
  else
    static_assert( static_false<V>, "Could not resolve find_effect argument to spell data." );
}

// finds a spell effect
// 1) first argument can be either player_talent_t, spell_data_t*, buff_t*, action_t*
// 2) if the second argument is player_talent_t, spell_data_t*, buff_t*, or action_t* then only effects that affect it are returned
// 3) if the third (or second if the above does not apply) argument is effect subtype, then the type is assumed to be E_APPLY_AURA
// further arguments can be given to filter for full type + subtype + property
template <typename T, typename U, typename... Ts>
static const spelleffect_data_t& find_effect( T val, U type, Ts&&... args )
{
  const spell_data_t* data = resolve_spell_data<T>( val );

  if constexpr( std::is_same_v<U, effect_subtype_t> )
    return spell_data_t::find_spelleffect( *data, E_APPLY_AURA, type, std::forward<Ts>( args )... );
  else if constexpr( std::is_same_v<U, effect_type_t> )
    return spell_data_t::find_spelleffect( *data, type, std::forward<Ts>( args )... );
  else
  {
    const spell_data_t* affected = resolve_spell_data<U>( type );

    if constexpr( sizeof...( Ts ) == 0 )
      return spell_data_t::find_spelleffect( *data, *affected, E_APPLY_AURA );
    else if constexpr( std::is_same_v<std::tuple_element_t<0, std::tuple<Ts...>>, effect_subtype_t> )
      return spell_data_t::find_spelleffect( *data, *affected, E_APPLY_AURA, std::forward<Ts>( args )... );
    else if constexpr( std::is_same_v<std::tuple_element_t<0, std::tuple<Ts...>>, effect_type_t> )
      return spell_data_t::find_spelleffect( *data, *affected, std::forward<Ts>( args )... );
   else
     return spell_data_t::find_spelleffect( *data, *affected, E_APPLY_AURA );
  }
}

template <typename T, typename U, typename... Ts>
static size_t find_effect_index( T val, U type, Ts&&... args )
{
  const auto& eff = find_effect( val, type, std::forward<Ts>( args )... );

  if ( eff.ok() )
    return eff.index() + 1;
  else
    return 0;
}

// finds the first effect with a trigger spell
// argument can be either player_talent_t, spell_data_t*, buff_t*, action_t*
template <typename T>
static const spelleffect_data_t& find_trigger( T val )
{
  const spell_data_t* data = resolve_spell_data<T>( val );

  for ( const auto& eff : data->effects() )
  {
    switch ( eff.type() )
    {
      case E_TRIGGER_MISSILE:
      case E_TRIGGER_SPELL:
      case E_TRIGGER_SPELL_WITH_VALUE:
      case E_TRIGGER_MISSILE_SPELL_WITH_VALUE:
        return eff;
      case E_APPLY_AURA:
      case E_APPLY_AREA_AURA_PARTY:
        switch( eff.subtype() )
        {
          case A_DUMMY:
            if ( eff.trigger_spell_id() )
              return eff;
            break;
          case A_PROC_TRIGGER_SPELL:
          case A_PROC_TRIGGER_SPELL_WITH_VALUE:
          case A_PERIODIC_TRIGGER_SPELL:
          case A_PERIODIC_TRIGGER_SPELL_WITH_VALUE:
          case A_LINKED_SPELL_WITH_VALUE:
            return eff;
          default:
            break;
        }
        break;
      default:
        break;
    }
  }

  return spelleffect_data_t::nil();
}

static std::string get_suffix( std::string_view name, std::string_view base )
{
  return std::string( name.substr( std::min( name.size(), name.find( base ) + base.size() ) ) );
}

// utility to create target_effect_t compatible functions from druid_td_t member references
template <typename T>
static std::function<int( actor_target_data_t* )> d_fn( T d, bool stack = true )
{
  if constexpr ( std::is_invocable_v<T, druid_td_t::debuffs_t> )
  {
    if ( stack )
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast<druid_td_t*>( t )->debuff )->check();
      };
    else
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast<druid_td_t*>( t )->debuff )->check() > 0;
      };
  }
  else if constexpr ( std::is_invocable_v<T, druid_td_t::dots_t> )
  {
    if ( stack )
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast<druid_td_t*>( t )->dots )->current_stack();
      };
    else
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast<druid_td_t*>( t )->dots )->is_ticking();
      };
  }
  else
  {
    static_assert( static_false<T>, "Not a valid member of druid_td_t" );
    return nullptr;
  }
}

static void replace_stats( action_t* source, action_t* target, bool add = true )
{
  if ( add )
  {
    range::erase_remove( source->stats->action_list, target );
    source->stats->action_list.push_back( target );
  }

  if ( target->stats == source->stats )
    return;

  range::erase_remove( target->player->stats_list, target->stats );
  delete target->stats;
  target->stats = source->stats;
}

static void snapshot_and_execute( action_t* a, const action_state_t* s, bool is_dot,
                                  std::function<void( const action_state_t*, action_state_t* )> pre = nullptr )
{
  auto state = a->get_state();

  if ( s )
    a->target = state->target = s->target;

  if ( pre )
    pre( s, state );

  a->snapshot_state( state, a->amount_type( state, is_dot ) );
  a->schedule_execute( state );
}

struct druid_t final : public parse_player_effects_t
{
  form_e form = form_e::NO_FORM;  // Active druid form
  std::array<std::vector<std::unique_ptr<benefit_tracker_t>>, tracker_e::MAX_TRACKER> trackers;

  // !!!==========================================================================!!!
  // !!! Runtime variables NOTE: these MUST be properly reset in druid_t::reset() !!!
  // !!!==========================================================================!!!
  moon_stage_e moon_stage;
  std::vector<event_t*> persistent_event_delay;
  event_t* astral_power_decay;

  struct dot_list_t
  {
    std::vector<dot_t*> moonfire;
    std::vector<dot_t*> sunfire;
    std::vector<dot_t*> rake;
    std::vector<dot_t*> rip;
    std::vector<dot_t*> thrash;
    std::vector<dot_t*> dreadful_wound;
    std::vector<dot_t*> wild_growth;
    std::vector<dot_t*> regrowth;
    std::vector<dot_t*> efflorescence;
  } dot_lists;

  // delays and sequencing issues that happen if certain spells are spell queued
  struct spell_queued_t
  {
    bool blooming_infusion_damage = false;
    action_t* blooming_infusion_damage_expire = nullptr;
    player_t* star_cascade = nullptr;
  } spell_queued;
  // !!!==========================================================================!!!

  // Options
  struct options_t
  {
    // General
    bool no_cds = false;
    bool raid_combat = true;

    // Multi-Spec
    int convoke_the_spirits_deck = 5;
    double cenarius_guidance_exceptional_chance = 0.85;

    // Balance
    double initial_astral_power = 0.0;
    int initial_moon_stage = static_cast<int>( moon_stage_e::NEW_MOON );
    int initial_orbit_breaker_stacks = -1;

    // Feral
    bool disable_ready_trigger = false;

    // Guardian

    // Restoration
    double time_spend_healing = 0.0;
  } options;

  struct active_actions_t
  {
    // General
    action_t* shift_to_caster;
    action_t* shift_to_bear;
    action_t* shift_to_cat;
    action_t* shift_to_moonkin;
    action_t* hotw_cat;
    action_t* hotw_owl;

    // Balance
    action_t* ascendant_eclipses;  // placeholder action
    action_t* denizen_of_the_dream;  // placeholder action
    action_t* lunar_bolt;
    action_t* moons;  // placeholder action
    action_t* orbit_breaker;
    action_t* orbital_strike;
    action_t* shooting_stars_moonfire;
    action_t* shooting_stars_sunfire;
    action_t* shooting_stars_mid1;  // mid1 4pc, exploding shooting star
    action_t* solar_bolt;
    action_t* sundered_firmament;
    action_t* fungal_growth;  // consolidated dot
    action_t* sunseeker_mushroom;

    // Feral
    action_t* unseen_slash;
    action_t* unseen_swipe;

    // Guardian
    action_t* after_the_wildfire_heal;
    action_t* blazing_thorns;
    action_t* brambles_reflect;
    action_t* echo_of_maul;
    action_t* echo_of_ravage;
    action_t* echo_of_raze;
    action_t* elunes_favored_heal;
    action_t* galactic_guardian;
    action_t* lunar_wrath;
    action_t* lunar_wrath_heal;
    action_t* memory_of_ysera_heal;
    action_t* sundering_roar_thrash;
    action_t* thrash_flashing;
    action_t* vicious_brambles;
    action_t* waking_nightmare;  // placeholder
    action_t* waking_nightmare_pulse;

    // Restoration
    action_t* yseras_gift;

    // Hero talents
    action_t* bloodseeker_vines;
    action_t* bloodseeker_vines_implant;
    action_t* boundless_moonlight_heal;
    action_t* bursting_growth;
    action_t* dream_burst;
    action_t* rampancy;
    action_t* star_cascade;
    action_t* sylvan_beckoning;
    action_t* sylvan_beckoning_starfall_driver;
    action_t* the_light_of_elune;
    action_t* treants_of_the_moon_mf;
  } active;

  // Pets
  struct pets_t
  {
    spawner::pet_spawner_t<pets::denizen_of_the_dream_t, druid_t> denizen_of_the_dream;
    spawner::pet_spawner_t<pets::dread_shade_t, druid_t> dread_shade;
    spawner::pet_spawner_t<pets::force_of_nature_t, druid_t> force_of_nature;
    spawner::pet_spawner_t<pets::grove_guardian_t, druid_t> grove_guardian;
    spawner::pet_spawner_t<pets::sylvan_beckoning_t, druid_t> sylvan_beckoning;

    pets_t( druid_t* p )
      : denizen_of_the_dream( "denizen_of_the_dream", p ),
        dread_shade( "dread_shade", p ),
        force_of_nature( "force_of_nature", p ),
        grove_guardian( "grove_guardian", p ),
        sylvan_beckoning( "sylvan_beckoning", p )
    {}
  } pets;

  // Auto-attacks
  weapon_t caster_form_weapon;
  weapon_t cat_weapon;
  weapon_t bear_weapon;
  melee_attack_t* caster_melee_attack = nullptr;
  melee_attack_t* cat_melee_attack = nullptr;
  melee_attack_t* bear_melee_attack = nullptr;

  // Buffs
  struct buffs_t
  {
    // General
    buff_t* barkskin;
    buff_t* bear_form;
    buff_t* cat_form;
    buff_t* prowl;

    // Class
    buff_t* dash;
    buff_t* forestwalk;
    buff_t* heart_of_the_wild_bear;
    buff_t* heart_of_the_wild_owl;
    buff_t* innervate;
    buff_t* ironfur;
    buff_t* lycaras_teachings_haste;  // no form
    buff_t* lycaras_teachings_crit;   // cat form
    buff_t* lycaras_teachings_vers;   // bear form
    buff_t* lycaras_teachings_mast;   // moonkin form
    buff_t* matted_fur;
    buff_t* moonkin_form;
    buff_t* tiger_dash;
    buff_t* ursine_vigor;
    buff_t* wild_charge_movement;

    // Multi-Spec
    buff_t* survival_instincts;

    // Balance
    buff_t* ascendant_fires;
    buff_t* ascendant_stars;
    buff_t* ascendant_stars_starfall;
    buff_t* astral_communion;
    buff_t* balance_of_all_things_arcane;
    buff_t* balance_of_all_things_nature;
    buff_t* celestial_alignment;
    buff_t* denizen_of_the_dream;  // proxy buff to track stack uptime
    buff_t* dreamstate;
    buff_t* eclipse_lunar;
    buff_t* eclipse_solar;
    buff_t* elunes_challenge;
    buff_t* fury_of_elune;  // AP ticks
    buff_t* incarnation_moonkin;
    buff_t* lunar_eclipse_override;
    buff_t* natures_balance;
    buff_t* orbit_breaker;
    buff_t* owlkin_frenzy;
    buff_t* parting_skies;  // sundered firmament tracker
    buff_t* shooting_stars_moonfire;
    buff_t* shooting_stars_sunfire;
    buff_t* starweaver_starfall;  // free starfall
    buff_t* starweaver_starsurge;  // free starsurge
    buff_t* solstice;
    buff_t* starfall;
    buff_t* starlord;  // talent
    buff_t* sundered_firmament;  // AP ticks
    buff_t* touch_the_cosmos;
    buff_t* umbral_embrace;

    // Feral
    buff_t* apex_predators_craving;
    buff_t* berserk_cat;
    buff_t* chomp_enabler;
    buff_t* clearcasting_cat;
    buff_t* coiled_to_spring;
    buff_t* flash_of_clarity;  // mid1 2pc
    buff_t* frantic_momentum;
    buff_t* hunger_for_battle;
    buff_t* incarnation_cat;
    buff_t* incarnation_cat_prowl;
    buff_t* overflowing_power;
    buff_t* predator;
    buff_t* predatory_swiftness;
    buff_t* savage_fury;
    buff_t* stalking_predator;
    buff_t* sudden_ambush;
    buff_t* tigers_fury;
    buff_t* tigers_tenacity;
    buff_t* unseen_predators_craving;

    // Guardian
    buff_t* after_the_wildfire;
    buff_t* berserk_bear;
    buff_t* blood_frenzy;
    buff_t* brambles;
    buff_t* bristling_fur;
    buff_t* celestial_might;  // mid1 4pc
    buff_t* dream_guide;
    buff_t* dream_of_cenarius;
    buff_t* echo_of_ironfur;
    buff_t* elunes_favored;
    buff_t* galactic_guardian;
    buff_t* gift_of_an_ancient_guardian;
    buff_t* gift_of_frenzied_regeneration;
    buff_t* gift_of_ironfur;
    buff_t* gift_of_maul;
    buff_t* gore;
    buff_t* gory_fur;
    buff_t* guardian_of_elune;
    buff_t* incarnation_bear;
    buff_t* lunar_beam;
    buff_t* lunar_wrath;
    buff_t* natural_resilience;
    buff_t* persistence;
    buff_t* sundering_roar;
    buff_t* ursocs_fury;
    buff_t* waking_nightmare;  // proxy buff to track stack uptime
    buff_t* wild_guardian;  // enable replacement action

    // Restoration
    buff_t* abundance;
    buff_t* call_of_the_elder_druid;
    buff_t* clearcasting_tree;
    buff_t* incarnation_tree;
    buff_t* natures_swiftness;
    buff_t* oath_of_the_elder_druid;
    buff_t* soul_of_the_forest_tree;
    buff_t* yseras_gift;

    // Hero talents
    buff_t* blooming_infusion_damage;
    buff_t* blooming_infusion_damage_counter;
    buff_t* blooming_infusion_heal;
    buff_t* blooming_infusion_heal_counter;
    buff_t* boundless_moonlight_heal;
    buff_t* cenarius_might;
    buff_t* dream_burst;
    buff_t* harmony_of_the_grove;
    buff_t* implant;
    buff_t* killing_strikes;
    buff_t* killing_strikes_combat;
    buff_t* ravage_fb;
    buff_t* ravage_maul;
    buff_t* root_network;
    buff_t* ruthless_aggression;
    buff_t* strategic_infusion;
    buff_t* sylvan_beckoning;
    buff_t* treants_of_the_moon;  // treant moonfire background heartbeat
    buff_t* feline_potential;          // wildpower surge
    buff_t* feline_potential_counter;  // wildpower surge
    buff_t* ursine_potential;          // wildpower surge
    buff_t* ursine_potential_counter;  // wildpower surge
    buff_t* wildshape_mastery;

    // Helper pointers
    buff_t* clearcasting;  // clearcasting_cat or clearcasting_tree
    buff_t* b_inc_cat;     // berserk_cat or incarnation_cat
    buff_t* b_inc_bear;    // berserk_bear or incarnation_bear
    buff_t* ca_inc;        // celestial_alignment or incarnation_moonkin
  } buff;

  struct hots_t
  {
    dot_t* frenzied_regeneration;
    dot_t* echo_of_frenzied_regeneration;
  } hots;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* berserk_bear;
    cooldown_t* eclipse;
    cooldown_t* fury_of_elune;
    cooldown_t* growl;
    cooldown_t* incarnation_bear;
    cooldown_t* lunar_beam;
    cooldown_t* mangle;
    cooldown_t* moon_cd;  // New / Half / Full Moon
    cooldown_t* thrash;
  } cooldown;

  // RNGs
  struct rngs_t
  {
    proc_rng_t* convoke;
    proc_rng_t* ravage;
    proc_rng_t* bloodseeker_vines;
    proc_rng_t* symbiotic_blooms;
  } rngs;

  // Gains
  struct gains_t
  {
    gain_t* overflowing_power;
  } gain;

  // Masteries
  struct masteries_t
  {
    const spell_data_t* harmony;
    const spell_data_t* natures_guardian;
    const spell_data_t* natures_guardian_AP;
    const spell_data_t* razor_claws;
    const spell_data_t* astral_invocation;
  } mastery;

  // Procs
  struct procs_t
  {
    proc_t* clearcasting_wasted;
  } proc;

  // Talents
  struct talents_t
  {
    // Class tree
    player_talent_t aessinas_renewal;  // TODO: NYI
    player_talent_t astral_influence;
    player_talent_t circle_of_the_heavens;
    player_talent_t circle_of_the_wild;
    player_talent_t cyclone;
    player_talent_t feline_swiftness;
    player_talent_t fluid_form;
    player_talent_t forestwalk;
    player_talent_t frenzied_regeneration;
    player_talent_t heart_of_the_wild;
    player_talent_t hibernate;
    player_talent_t gale_winds;
    player_talent_t gift_of_the_wild;
    player_talent_t grievous_wounds;
    player_talent_t improved_barkskin;
    player_talent_t improved_natures_cure;
    player_talent_t improved_stampeding_roar;
    player_talent_t incapacitating_roar;
    player_talent_t incessant_tempest;
    player_talent_t innervate;
    player_talent_t instincts_of_the_claw;
    player_talent_t ironfur;
    player_talent_t killer_instinct;
    player_talent_t light_of_the_sun;
    player_talent_t lingering_healing;
    player_talent_t lore_of_the_grove;
    player_talent_t lycaras_inspiration;
    player_talent_t lycaras_teachings;
    player_talent_t maim;
    player_talent_t matted_fur;
    player_talent_t mass_entanglement;
    player_talent_t mighty_bash;
    player_talent_t moonkin_form;
    player_talent_t natural_recovery;
    player_talent_t nurturing_instinct;
    player_talent_t oakskin;
    player_talent_t perfectlyhoned_instincts;  // TODO: NYI
    player_talent_t primal_fury;
    player_talent_t rake;
    player_talent_t rejuvenation;
    player_talent_t remove_corruption;
    player_talent_t rip;
    player_talent_t skull_bash;
    player_talent_t soothe;
    player_talent_t stampeding_roar;
    player_talent_t starfire;
    player_talent_t starlight_conduit;
    player_talent_t starsurge;
    player_talent_t sunfire;
    player_talent_t swipe;
    player_talent_t symbiotic_relationship;  // TODO: NYI
    player_talent_t thick_hide;
    player_talent_t tiger_dash;
    player_talent_t typhoon;
    player_talent_t ursine_vigor;
    player_talent_t ursocs_spirit;
    player_talent_t ursols_vortex;
    player_talent_t verdant_heart;
    player_talent_t wellhoned_instincts;  // TODO: NYI
    player_talent_t wild_charge;
    player_talent_t wild_growth;

    // Multi-spec
    player_talent_t convoke_the_spirits;
    player_talent_t survival_instincts;

    // Balance
    player_talent_t ascendant_eclipses_1;  // apex
    player_talent_t ascendant_eclipses_2;  // apex
    player_talent_t ascendant_eclipses_3;  // apex
    player_talent_t aetherial_kindling;
    player_talent_t astral_communion;
    player_talent_t balance_of_all_things;
    player_talent_t celestial_alignment;
    player_talent_t celestial_fire;
    player_talent_t cosmic_rapidity;
    player_talent_t denizen_of_the_dream;
    player_talent_t eclipse;
    player_talent_t elunes_challenge;
    player_talent_t elunes_guidance;
    player_talent_t force_of_nature;
    player_talent_t fury_of_elune;
    player_talent_t harmony_of_the_heavens;
    player_talent_t hail_of_stars;
    player_talent_t improved_eclipse;
    player_talent_t incarnation_moonkin;
    player_talent_t meteor_storm;
    player_talent_t meteorites;
    player_talent_t natures_balance;
    player_talent_t natures_grace;
    player_talent_t new_moon;
    player_talent_t orbit_breaker;
    player_talent_t orbital_strike;
    player_talent_t power_of_goldrinn;
    player_talent_t radiant_moonlight;
    player_talent_t rattle_the_stars;
    player_talent_t sculpt_the_stars;
    player_talent_t shooting_stars;
    player_talent_t solar_beam;
    player_talent_t solstice;
    player_talent_t soul_of_the_forest_owl;
    player_talent_t starlord;
    player_talent_t starweaver;
    player_talent_t stellar_amplification;
    player_talent_t sundered_firmament;
    player_talent_t sunseeker_mushroom;
    player_talent_t total_eclipse;
    player_talent_t touch_the_cosmos;
    player_talent_t twin_moons;
    player_talent_t umbral_embrace;
    player_talent_t umbral_intensity;
    player_talent_t whirling_stars;
    player_talent_t wild_mushroom;
    player_talent_t wild_surges;

    // Feral
    player_talent_t apex_predators_craving;
    player_talent_t ashamanes_guidance;
    player_talent_t berserk_cat;
    player_talent_t berserk_heart_of_the_lion;
    player_talent_t blood_spattered;
    player_talent_t carnivorous_instinct;
    player_talent_t chomp;
    player_talent_t circle_of_life_and_death;
    player_talent_t coiled_to_spring;
    player_talent_t doubleclawed_rake;
    player_talent_t dreadful_bleeding;
    player_talent_t feral_frenzy;
    player_talent_t focused_frenzy;
    player_talent_t frantic_frenzy;
    player_talent_t frantic_momentum;
    player_talent_t hunger_for_battle;
    player_talent_t incarnation_cat;
    player_talent_t infected_wounds_cat;
    player_talent_t lacerating_claws;
    player_talent_t lunar_inspiration;
    player_talent_t merciless_claws;
    player_talent_t moment_of_clarity;
    player_talent_t omen_of_clarity_cat;
    player_talent_t panthers_guile;
    player_talent_t pouncing_strikes;
    player_talent_t predator;
    player_talent_t primal_wrath;
    player_talent_t raging_fury;
    player_talent_t rampant_ferocity;
    player_talent_t rip_and_tear;
    player_talent_t saber_jaws;
    player_talent_t sabertooth;
    player_talent_t savage_fury;
    player_talent_t soul_of_the_forest_cat;
    player_talent_t sudden_ambush;
    player_talent_t taste_for_blood;
    player_talent_t tigers_fury;
    player_talent_t tigers_tenacity;
    player_talent_t tireless_energy;
    player_talent_t unseen_predator_1;  // apex
    player_talent_t unseen_predator_2;  // apex
    player_talent_t unseen_predator_3;  // apex
    player_talent_t veinripper;
    player_talent_t wild_slashes;

    // Guardian
    player_talent_t after_the_wildfire;
    player_talent_t berserk_bear;
    player_talent_t blood_frenzy;
    player_talent_t brambles;
    player_talent_t bristling_fur;
    player_talent_t dream_guide;  // TODO: hp% proc NYI
    player_talent_t dream_of_cenarius_bear;
    player_talent_t elunes_favored;
    player_talent_t flashing_claws;
    player_talent_t front_of_the_pack;
    player_talent_t fury_of_nature;
    player_talent_t galactic_guardian;
    player_talent_t gift_of_an_ancient_guardian;
    player_talent_t gore;
    player_talent_t gory_fur;
    player_talent_t guardian_of_elune;
    player_talent_t harnessed_rage;
    player_talent_t incarnation_bear;
    player_talent_t infected_wounds_bear;
    player_talent_t innate_resolve;
    player_talent_t killing_blow;
    player_talent_t lunar_beam;
    player_talent_t maul;
    player_talent_t memory_of_ysera;
    player_talent_t moonless_night;
    player_talent_t natural_resilience;
    player_talent_t persistence;
    player_talent_t raze;
    player_talent_t red_moon;
    player_talent_t reinforced_fur;
    player_talent_t reinvigoration;
    player_talent_t rend_and_tear;
    player_talent_t scintillating_moonlight;
    player_talent_t soul_of_the_forest_bear;
    player_talent_t sundering_roar;
    player_talent_t survival_of_the_fittest;
    player_talent_t twin_moonfire;
    player_talent_t untamed_savagery;
    player_talent_t ursocs_endurance;
    player_talent_t ursocs_fury;
    player_talent_t ursocs_guidance;
    player_talent_t ursols_warding;  // TODO: NYI
    player_talent_t vulnerable_flesh;
    player_talent_t waking_nightmare;
    player_talent_t ward_of_the_forest;
    player_talent_t wild_guardian_1;  // apex
    player_talent_t wild_guardian_2;  // apex
    player_talent_t wild_guardian_3;  // apex

    // Restoration
    player_talent_t abundance;
    player_talent_t call_of_the_elder_druid;
    player_talent_t cenarius_guidance;
    player_talent_t cultivation;
    player_talent_t dream_of_cenarius_tree;
    player_talent_t efflorescence;
    player_talent_t embrace_of_the_dream;
    player_talent_t everbloom_1;  // apex
    player_talent_t everbloom_2;  // apex
    player_talent_t everbloom_3;  // apex
    player_talent_t flourish;
    player_talent_t germination;
    player_talent_t grove_guardians;
    player_talent_t harmonious_blooming;
    player_talent_t improved_ironbark;
    player_talent_t improved_regrowth;
    player_talent_t improved_swiftmend;
    player_talent_t improved_wild_growth;
    player_talent_t incarnation_tree;
    player_talent_t inner_peace;
    player_talent_t intensity;
    player_talent_t ironbark;
    player_talent_t lifebloom;
    player_talent_t lifetreading;
    player_talent_t liveliness;
    player_talent_t master_shapeshifter;
    player_talent_t natures_bounty;
    player_talent_t natures_splendor;
    player_talent_t natures_swiftness;
    player_talent_t nurturing_dormancy;
    player_talent_t omen_of_clarity_tree;
    player_talent_t passing_seasons;
    player_talent_t photosynthesis;
    player_talent_t power_of_the_archdruid;
    player_talent_t prosperity;
    player_talent_t rampant_growth;
    player_talent_t reforestation;
    player_talent_t regenerative_heartwood;
    player_talent_t regenesis;
    player_talent_t renewing_surge;
    player_talent_t soul_of_the_forest_tree;
    player_talent_t stonebark;
    player_talent_t swiftmend;
    player_talent_t thriving_vegetation;
    player_talent_t tranquil_mind;
    player_talent_t tranquility;
    player_talent_t unstoppable_growth;
    player_talent_t verdancy;
    player_talent_t verdant_infusion;
    player_talent_t waking_dream;
    player_talent_t wild_synthesis;
    player_talent_t yseras_gift;

    // Druid of the Claw
    player_talent_t aggravate_wounds;
    player_talent_t bestial_strength;
    player_talent_t claw_rampage;
    player_talent_t dreadful_wound;
    player_talent_t empowered_shapeshifting;
    player_talent_t exacerbating_wounds;
    player_talent_t fount_of_strength;
    player_talent_t killing_strikes;
    player_talent_t limb_from_limb;
    player_talent_t packs_endurance;
    player_talent_t ravage;
    player_talent_t ruthless_aggression;
    player_talent_t strike_for_the_heart;
    player_talent_t tear_down_the_mighty;
    player_talent_t twin_claw;
    player_talent_t wildpower_surge;
    player_talent_t wildshape_mastery;

    // Wildstalker
    player_talent_t bond_with_nature;
    player_talent_t bursting_growth;
    player_talent_t entangling_vortex;
    player_talent_t flower_walk;
    player_talent_t green_thumb;
    player_talent_t harmonious_constitution;
    player_talent_t hunt_beneath_the_open_skies;
    player_talent_t implant;
    player_talent_t lethal_preservation;
    player_talent_t patient_custodian;
    player_talent_t rampancy;
    player_talent_t resilient_flourishing;
    player_talent_t root_network;
    player_talent_t strategic_infusion;
    player_talent_t thriving_growth;
    player_talent_t twin_sprouts;
    player_talent_t vigorous_creepers;
    player_talent_t wildstalkers_power;

    // Keeper of the Grove
    player_talent_t blooming_infusion;
    player_talent_t bounteous_bloom;
    player_talent_t cenarius_might;
    player_talent_t control_of_the_dream;
    player_talent_t dream_surge;
    player_talent_t durability_of_nature;
    player_talent_t dryads_dance;
    player_talent_t early_spring;
    player_talent_t expansiveness;
    player_talent_t groves_inspiration;
    player_talent_t harmony_of_the_grove;
    player_talent_t potent_enchantments;
    player_talent_t power_of_nature;
    player_talent_t power_of_the_dream;
    player_talent_t protective_growth;
    player_talent_t spirit_of_the_thicket;
    player_talent_t sylvan_beckoning;
    player_talent_t treants_of_the_moon;

    // Elune's Chosen
    player_talent_t arcane_affinity;
    player_talent_t astral_insight;
    player_talent_t atmospheric_exposure;
    player_talent_t bask_in_moonlight;
    player_talent_t boundless_moonlight;
    player_talent_t elunes_grace;
    player_talent_t glistening_fur;
    player_talent_t lunar_calling;
    player_talent_t lunar_insight;
    player_talent_t lunation;
    player_talent_t moondust;
    player_talent_t moon_guardian;
    player_talent_t penumbral_swell;
    player_talent_t star_cascade;
    player_talent_t stellar_command;
    player_talent_t the_eternal_moon;
    player_talent_t the_light_of_elune;
  } talent;

  // Class Specializations
  struct specializations_t
  {
    // Class
    const spell_data_t* bear_form;
    const spell_data_t* bear_form_override;  // swipe
    const spell_data_t* bear_form_passive;
    const spell_data_t* bear_form_passive_2;
    const spell_data_t* cat_form;
    const spell_data_t* cat_form_override;  // swipe
    const spell_data_t* cat_form_passive;
    const spell_data_t* cat_form_passive_2;
    const spell_data_t* cat_form_speed;
    const spell_data_t* improved_prowl;  // stealth rake
    const spell_data_t* moonfire;
    const spell_data_t* moonfire_dmg;
    const spell_data_t* sunfire_dmg;
    const spell_data_t* thrash;
    const spell_data_t* thrash_bleed;
    const spell_data_t* wrath;

    // Balance
    const spell_data_t* astral_power;
    const spell_data_t* celestial_alignment;
    const spell_data_t* eclipse_lunar;
    const spell_data_t* eclipse_solar;
    const spell_data_t* full_moon;
    const spell_data_t* half_moon;
    const spell_data_t* incarnation_moonkin;
    const spell_data_t* shooting_stars_dmg;
    const spell_data_t* starfall;
    const spell_data_t* stellar_amplification;
    const spell_data_t* wild_mushroom;

    // Feral
    const spell_data_t* berserk_cat;  // berserk cast/buff spell
    const spell_data_t* chomp_controller;
    const spell_data_t* predatory_swiftness;
    const spell_data_t* sabertooth;  // sabertooth debuff

    // Guardian
    const spell_data_t* elunes_favored;
    const spell_data_t* fury_of_nature;
    const spell_data_t* lightning_reflexes;
    const spell_data_t* ursine_adept;

    // Resto

    // Hero Talent
    const spell_data_t* atmospheric_exposure;  // atmospheric exposure debuff
    const spell_data_t* bloodseeker_vines;
    const spell_data_t* dreadful_wound;
    const spell_data_t* sylvan_beckoning_sf;
  } spec;

  struct uptimes_t
  {
    uptime_t* atmospheric_exposure;
    uptime_t* stellar_amplification;
  } uptime;

  auto_dispose<std::vector<modified_spell_data_t*>> modified_spells;

  druid_t( sim_t* sim, std::string_view name, race_e r = RACE_NIGHT_ELF )
    : parse_player_effects_t( sim, DRUID, name, r ),
      options(),
      active(),
      pets( this ),
      caster_form_weapon(),
      buff(),
      hots(),
      cooldown(),
      rngs(),
      gain(),
      mastery(),
      proc(),
      talent(),
      spec(),
      uptime()
  {
    resource_regeneration = regen_type::DYNAMIC;

    regen_caches[ CACHE_HASTE ]        = true;
    regen_caches[ CACHE_ATTACK_HASTE ] = true;
  }

  // hide player_t::is_ptr()
  bool is_ptr() const { return dbc->wowv() > dbc::client_data_version( false ); }

  // Character Definition
  void activate() override;
  void init() override;
  bool validate_fight_style( fight_style_e ) const override;
  bool validate_actor() override;
  void init_absorb_priority() override;
  void init_action_list() override;
  void init_blizzard_action_list() override;
  std::vector<std::string> action_names_from_spell_id( unsigned int ) const override;
  void parse_assisted_combat_step( const assisted_combat_step_data_t&, action_priority_list_t* ) override;
  parsed_assisted_combat_rule_t parse_assisted_combat_rule( const assisted_combat_rule_data_t&,
                                                            const assisted_combat_step_data_t& ) const override;
  void init_base_stats() override;
  void init_initial_stats() override;
  void init_rng() override;
  void init_gains() override;
  void init_procs() override;
  void init_uptimes() override;
  void init_resources( bool ) override;
  void init_special_effects() override;
  void init_spells() override;
  void init_items() override;
  void init_scaling() override;
  void init_finished() override;
  void parse_player_effects();
  void create_buffs() override;
  void create_actions() override;
  void parse_action_effects( action_t* );
  void parse_action_target_effects( action_t* );
  std::string default_flask() const override;
  std::string default_potion() const override;
  std::string default_food() const override;
  std::string default_rune() const override;
  std::string default_temporary_enchant() const override;
  void invalidate_cache( cache_e ) override;
  void reset() override;
  void precombat_init() override;
  void combat_begin() override;
  void analyze( sim_t& ) override;
  timespan_t available() const override;
  double composite_armor() const override;
  double composite_block() const override { return 0; }
  double composite_dodge_rating() const override;
  double composite_parry() const override { return 0; }
  std::unique_ptr<expr_t> create_action_expression(action_t& a, std::string_view name_str) override;
  std::unique_ptr<expr_t> create_expression( std::string_view name ) override;
  action_t* create_action( std::string_view name, std::string_view options ) override;
  resource_e primary_resource() const override;
  role_e primary_role() const override;
  stat_e convert_hybrid_stat( stat_e s ) const override;
  double resource_regen_per_second( resource_e ) const override;
  double resource_gain( resource_e, double, gain_t*, action_t* a = nullptr ) override;
  void assess_damage( school_e, result_amount_type, action_state_t* ) override;
  void recalculate_resource_max( resource_e, gain_t* source = nullptr ) override;
  void create_options() override;
  const druid_td_t* find_target_data( const player_t* target ) const override;
  druid_td_t* get_target_data( player_t* target ) const override;
  void copy_from( player_t* ) override;
  void moving() override;
  action_t* execute_action() override;
  void print_custom_parsed_effects( report::sc_html_stream& ) const override;

  // utility functions
  void init_beast_weapon( weapon_t&, double );
  void adjust_health_pct( double, bool );
  const spell_data_t* apply_override( const spell_data_t*, const spell_data_t* ) const;
  bool uses_form( specialization_e, std::string_view, action_t* ) const;
  bool uses_cat_form() const;
  bool uses_bear_form() const;
  bool uses_moonkin_form() const;
  player_t* get_smart_target( const std::vector<player_t*>& tl, dot_t* druid_td_t::dots_t::*dot,
                              player_t* exclude = nullptr, double range = 0.0, bool really_smart = false );

  modified_spell_data_t* get_modified_spell( const spell_data_t* );

  // secondary actions
  std::vector<action_t*> secondary_action_list;

  template <typename T, bool PROC = true, typename... Ts>
  T* get_secondary_action( std::string_view n, Ts&&... args );

private:
  void apl_precombat();
  void apl_default();
  void apl_feral();
  void apl_feral_ptr();
  void apl_balance();
  void apl_balance_ptr();
  void apl_guardian();
  void apl_guardian_ptr();
  void apl_restoration();
  void apl_restoration_ptr();

  target_specific_t<druid_td_t> target_data;
};

namespace pets
{
// ==========================================================================
// Pets and Guardians
// ==========================================================================

// Denizen of the Dream =====================================================
struct denizen_of_the_dream_t final : public pet_t
{
  struct fey_missile_t final : public parse_action_effects_t<spell_t>
  {
    druid_t* o;

    fey_missile_t( pet_t* p )
      : parse_action_effects_t( "fey_missile", p, p->find_spell( 188046 ) ), o( static_cast<druid_t*>( p->owner ) )
    {
      name_str_reporting = "fey_missile";

      _player = p->owner;  // use owner's target data & mastery

      force_effect( o->buff.eclipse_lunar, 1, USE_CURRENT );
      force_effect( o->buff.eclipse_solar, 1, USE_CURRENT );

      force_effect( o->mastery.astral_invocation, 1 );
      force_effect( o->mastery.astral_invocation, 3 );

      o->parse_action_effects( this );
      o->parse_action_target_effects( this );
    }

    void execute() override
    {
      // Has random delay between casts. Seems to operate at 75FPS with most delays being 31, 32, or 33 frames. Delays
      // as low as 8 frames and as high as 38 frames have been observed, but for now we will only assume 31-33 with a
      // 2:3:2 distribution.
      double delay = 30;

      switch ( rng().range( 0U, 7U ) )
      {
        case 0:
        case 1: delay += 1; break;
        case 2:
        case 3:
        case 4: delay += 2; break;
        case 5:
        case 6: delay += 3; break;
        default: break;
      }

      cooldown->duration = timespan_t::from_seconds( delay / 75.0 );

      parse_action_effects_t::execute();
    }
  };

  denizen_of_the_dream_t( druid_t* p ) : pet_t( p->sim, p, "Denizen of the Dream", true, true )
  {
    owner_coeff.sp_from_sp = 1.0;

    action_list_str = "fey_missile";
  }

  action_t* create_action( std::string_view n, std::string_view opt ) override
  {
    if ( n == "fey_missile" ) return new fey_missile_t( this );

    return pet_t::create_action( n, opt );
  }
};

// Dread Shade (Waking Nightmare) ===========================================
struct dread_shade_t : public pet_t
{
  struct dire_echo_t final : public parse_action_effects_t<spell_t>
  {
    druid_t* o;

    dire_echo_t( pet_t* p )
      : parse_action_effects_t( "dire_echo", p, p->find_spell( 1253489 ) ), o( static_cast<druid_t*>( p->owner ) )
    {
      aoe = -1;

      _player = p->owner;  // use owner's target data & mastery

      o->parse_action_effects( this );
      o->parse_action_target_effects( this );
    }

    void init() override
    {
      parse_action_effects_t<spell_t>::init();

      snapshot_flags &= ~STATE_MUL_PET;  // doesn't get pet multiplier
    }
  };

  action_t* dire_echo;
  buff_t* waking_nightmare = nullptr;

  dread_shade_t( druid_t* p ) : pet_t( p->sim, p, "Dread Shade", true, true )
  {
    owner_coeff.sp_from_sp = 1.0;
  }

  druid_t* o() { return static_cast<druid_t*>( owner ); }

  void arise() override;
  void create_buffs() override;
  void create_actions() override;
};

// Sylvan Beckoning =========================================================
struct sylvan_beckoning_t final : public pet_t
{
  struct starsurge_t final : public spell_t
  {
    starsurge_t( pet_t* p ) : spell_t( "starsurge", p, p->find_spell( 1264677 ) ) {}

    sylvan_beckoning_t* p() { return static_cast<sylvan_beckoning_t*>( player ); }

    bool ready() override
    {
      if ( p()->starsurge_count >= 5 )
        return false;

      return spell_t::ready();
    }

    void execute() override
    {
      spell_t::execute();

      p()->starsurge_count++;
    }
  };

  buff_t* starfall_buff = nullptr;
  unsigned starsurge_count = 0;

  sylvan_beckoning_t( druid_t* p ) : pet_t( p->sim, p, "Sylvan Beckoning", true, true )
  {
    owner_coeff.sp_from_sp = 1.0;

    action_list_str = "starsurge";
  }

  druid_t* o() { return static_cast<druid_t*>( owner ); }

  void arise() override;
  void create_buffs() override;
  action_t* create_action( std::string_view n, std::string_view opt ) override;
};

// Treant Base ==============================================================
struct treant_base_t : public pet_t
{
  cooldown_t* mf_cd = nullptr;

  treant_base_t( druid_t* p ) : pet_t( p->sim, p, "Treant", true, true )
  {
    if ( o()->active.treants_of_the_moon_mf )
    {
      mf_cd = get_cooldown( "treants_of_the_moon" );
      mf_cd->duration = o()->find_spell( 428545 )->cooldown();
    }
  }

  druid_t* o() { return static_cast<druid_t*>( owner ); }

  // defined after foreground abilities
  void arise() override;
  void demise() override;
};

// Force of Nature ==========================================================
struct force_of_nature_t final : public treant_base_t
{
  struct fon_melee_t final : public melee_attack_t
  {
    bool first_attack = true;

    fon_melee_t( pet_t* pet ) : melee_attack_t( "melee", pet, spell_data_t::nil() )
    {
      name_str_reporting = "Melee";

      school            = SCHOOL_PHYSICAL;
      weapon            = &( pet->main_hand_weapon );
      weapon_multiplier = 1.0;
      base_execute_time = weapon->swing_time;
      may_crit = background = repeating = true;
    }

    timespan_t execute_time() const override
    {
      return first_attack ? 0_ms : melee_attack_t::execute_time();
    }

    void cancel() override
    {
      melee_attack_t::cancel();
      first_attack = true;
    }

    void schedule_execute( action_state_t* s ) override
    {
      melee_attack_t::schedule_execute( s );
      first_attack = false;
    }
  };

  struct auto_attack_t final : public melee_attack_t
  {
    auto_attack_t( pet_t* pet ) : melee_attack_t( "auto_attack", pet )
    {
      assert( pet->main_hand_weapon.type != WEAPON_NONE );
      pet->main_hand_attack = new fon_melee_t( pet );
      trigger_gcd = 0_ms;
    }

    void execute() override { player->main_hand_attack->schedule_execute(); }

    bool ready() override { return ( player->main_hand_attack->execute_event == nullptr ); }
  };

  double power_of_nature_mul = 0.0;

  force_of_nature_t( druid_t* p ) : treant_base_t( p )
  {
    // Treants have base weapon damage + ap from player's sp.
    main_hand_weapon.min_dmg = main_hand_weapon.max_dmg = 9.6;
    owner_coeff.ap_from_sp = 0.96;

    resource_regeneration = regen_type::DISABLED;
    main_hand_weapon.type = WEAPON_BEAST;

    action_list_str = "auto_attack";

    if ( o()->talent.power_of_nature.ok() )
      power_of_nature_mul = o()->find_spell( 449001 )->effectN( 1 ).percent();
  }

  void init_base_stats() override
  {
    pet_t::init_base_stats();

    resources.base[ RESOURCE_HEALTH ] =
      owner->resources.max[ RESOURCE_HEALTH ] * 0.6 *
      ( 1.0 + find_effect( o()->talent.durability_of_nature, A_MOD_PET_STAT ).percent() );
  }

  resource_e primary_resource() const override { return RESOURCE_NONE; }

  action_t* create_action( std::string_view name, std::string_view options_str ) override
  {
    if ( name == "auto_attack" ) return new auto_attack_t( this );

    return pet_t::create_action( name, options_str );
  }

  double composite_player_multiplier( school_e s ) const override
  {
    return treant_base_t::composite_player_multiplier( s ) * ( 1.0 + power_of_nature_mul );
  }
};

// Grove Guardian ===========================================================
struct grove_guardian_t final : public treant_base_t
{
  grove_guardian_t( druid_t* p ) : treant_base_t( p )
  {

  }
};

std::function<void( pet_t* )> parent_pet_action_fn( action_t* parent )
{
  return [ parent ]( pet_t* p ) {
    for ( auto a : p->action_list )
    {
      if ( auto it = range::find( parent->child_action, a->name_str, &action_t::name_str );
           it != parent->child_action.end() )
      {
        if ( a->stats != ( *it )->stats )
        {
          range::erase_remove( p->stats_list, a->stats );
          delete a->stats;
          a->stats = ( *it )->stats;
        }
      }
      else
      {
        parent->add_child( a );
      }
    }
  };
}
}  // end namespace pets

// ==========================================================================
// Base template classes
// ==========================================================================
template <typename Base = buff_t, typename = std::enable_if_t<std::is_base_of_v<buff_t, Base>>>
struct druid_buff_base_t : public Base
{
protected:
  using base_t = druid_buff_base_t<Base>;

public:
  druid_buff_base_t( druid_t* p, std::string_view n, const spell_data_t* s = spell_data_t::nil(),
                     const item_t* item = nullptr )
    : Base( p, n, s, item )
  {}

  druid_buff_base_t( druid_td_t& td, std::string_view n, const spell_data_t* s = spell_data_t::nil(),
                     const item_t* item = nullptr )
    : Base( td, n, s, item )
  {}

  druid_t* p() { return static_cast<druid_t*>( Base::source ); }

  const druid_t* p() const { return static_cast<druid_t*>( Base::source ); }
};

template <class Base>
struct druid_action_t : public parse_action_effects_t<Base>, public druid_action_data_t
{
private:
  using ab = parse_action_effects_t<Base>;

protected:
  using base_t = druid_action_t<Base>;
  std::vector<player_effect_t> persistent_multiplier_effects;

public:
  // Name to be used by get_dot() instead of action name
  std::string dot_name;
  // energize effect, used in composite_energize_amount to access parsed effect
  const modified_spelleffect_t* energize = nullptr;
  // Restricts use of a spell based on form.
  unsigned form_mask;

  druid_action_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE )
    : ab( n, p, s ), dot_name( n ), form_mask( ab::data().stance_mask() )
  {
    action_flags = f;

    if ( is_free() )
      ab::cooldown->duration = 0_ms;

    if ( ab::data().flags( spell_attribute::SX_NO_STEALTH_BREAK ) )
      action_flags |= flag_e::ALLOWSTEALTH;

    // WARNING: auto attacks will NOT get processed here since they have no spell data
    if ( ab::data().ok() )
    {
      p->parse_action_effects( this );

      if ( ab::type == action_e::ACTION_SPELL || ab::type == action_e::ACTION_ATTACK )
        p->parse_action_target_effects( this );

      if ( p->spec.ursine_adept->ok() &&
           ab::data().affected_by( find_effect( p->buff.bear_form, A_MOD_IGNORE_SHAPESHIFT ) ) )
      {
        form_mask |= form_e::BEAR_FORM;
      }
    }
  }

  druid_t* p() { return static_cast<druid_t*>( ab::player ); }

  const druid_t* p() const { return static_cast<druid_t*>( ab::player ); }

  // non-const target data accessor
  druid_td_t* td( player_t* t ) { return p()->get_target_data( t ); }

  // const target data accessor with creation, typically used to check for dots
  druid_td_t* get_td( player_t* t ) const { return p()->get_target_data( t ); }

  // const target data accessor without creation, typically used to check for debuffs
  const druid_td_t* find_td( player_t* t ) const { return p()->find_target_data( t ); }

  dot_t* get_dot( player_t* t = nullptr ) override
  {
    if ( !t )
      t = ab::target;
    if ( !t )
      return nullptr;

    dot_t*& dot = ab::target_specific_dot[ t ];
    if ( !dot )
      dot = t->get_dot( dot_name, ab::player );
    return dot;
  }

  void set_tracked_cooldown( cooldown_t*& cd )
  {
    if ( !ab::data().ok() || is_free() )
      return;

    if ( !cd )
      cd = ab::cooldown;

    assert( cd == ab::cooldown );
  }

  void set_shared_cooldown( std::string n, cooldown_t*& cd )
  {
    if ( !ab::data().ok() || is_free() )
      return;

    if ( cd )
    {
      ab::cooldown = cd;
      return;
    }

    ab::cooldown->name_str = n;
    cd = ab::cooldown;
  }

  bool ready() override
  {
    if ( !check_form_restriction() && !autoshift && ( has_flag( flag_e::NOUNSHIFT ) || !( form_mask & NO_FORM ) ) )
    {
      if ( ab::sim->debug )
      {
        ab::sim->print_debug( "{} ready() failed due to wrong form. form={:#010x} form_mask={:#010x}", ab::name(),
                              static_cast<unsigned int>( p()->form ), form_mask );
      }

      return false;
    }

    return ab::ready();
  }

  void init() override;

  void schedule_execute( action_state_t* s = nullptr ) override
  {
    check_autoshift();

    ab::schedule_execute( s );
  }

  void execute() override
  {
    // offgcd actions bypass schedule_execute so check for autoshift
    if ( ab::use_off_gcd )
      check_autoshift();

    ab::execute();

    if ( !has_flag( flag_e::ALLOWSTEALTH ) )
    {
      if ( p()->talent.strategic_infusion.ok() && p()->buff.prowl->check() )
        p()->buff.strategic_infusion->trigger();

      p()->buff.prowl->expire();
      p()->buffs.shadowmeld->expire();
    }

    if ( can_trigger_lunation() )
    {
      assert( p()->talent.lunation->effects().size() == 3 );

      static constexpr std::array<cooldown_t* druid_t::cooldowns_t::*, 3> lunation_cds = {
        &druid_t::cooldowns_t::fury_of_elune, &druid_t::cooldowns_t::moon_cd, &druid_t::cooldowns_t::lunar_beam };

      for ( auto eff : p()->talent.lunation->effects() )
        if ( auto cd = std::invoke( lunation_cds[ eff.index() ], p()->cooldown ) )
          cd->adjust( eff.time_value() );
    }
  }

  bool can_trigger_lunation() const
  {
    if ( !p()->talent.lunation.ok() || ab::background )
      return false;

    switch ( ab::id )
    {
      case 78674:   // starsurge
      case 197626:  // starsurge offspec
        return !has_flag( flag_e::CASCADE );
      case 77758:   // thrash
        return p()->talent.lunar_calling.ok();
      case 8921:    // moonfire
        return !has_flag( flag_e::GALACTIC | flag_e::TWIN );
      case 191034:  // starfall
      case 194153:  // starfire
      case 197628:  // starfire offspec
      case 383410:  // celestial alignment orbital strike
      case 390414:  // incarnation orbital strike
      case 1252871: // red moon
        return true;
      default:
        return false;
    }
  }

  // lunation & lunar amplification seem to go off the original school for temporary school changes
  school_e _get_school() const
  {
    return ab::original_school != SCHOOL_NONE ? ab::original_school : ab::get_school();
  }

  double cost() const override
  {
    if ( is_free() )
      return 0.0;

    if ( p()->specialization() == DRUID_RESTORATION && ab::current_resource() == RESOURCE_MANA &&
         p()->buff.innervate->up() )
    {
      return 0.0;
    }

    return ab::cost();
  }

  void set_energize( modified_spell_data_t* m_data )
  {
    if ( ab::energize_type == action_energize::NONE || !m_data )
      return;

    for ( const auto& e : m_data->effects )
    {
      const spelleffect_data_t& eff = e;
      if ( eff.type() == E_ENERGIZE &&
           util::power_type_to_resource( static_cast<power_e>( eff.misc_value1() ) ) == ab::energize_resource )
      {
        energize = &e;
        ab::energize_amount = energize->resource();
        ab::sim->print_debug( "{} energize {} {} from {}", *this, util::resource_type_string( ab::energize_resource ),
                              ab::energize_amount, eff );

        return;
      }
    }
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    if ( energize )
      return energize->resource( this, s );
    else
      return ab::composite_energize_amount( s );
  }

  // Override this function for temporary effects that change the normal form restrictions of the spell. eg: Predatory
  // Swiftness
  virtual bool check_form_restriction()
  {
    if ( !form_mask || ( form_mask & p()->form ) == p()->form )
      return true;

    return false;
  }

  void check_autoshift()
  {
    if ( is_free() )
      return;

    if ( !check_form_restriction() )
    {
      if ( autoshift )
      {
        autoshift->execute();
      }
      else if ( !has_flag( flag_e::NOUNSHIFT ) && form_mask & NO_FORM )
      {
        p()->active.shift_to_caster->execute();
      }
      else
      {
        throw sc_runtime_error( fmt::format( "{} executed in wrong form {:#010x} with no valid form to shift to!",
                                             *this, static_cast<unsigned>( p()->form ) ) );
      }
    }
  }

  bool verify_actor_spec() const override
  {
    if ( range::contains( p()->secondary_action_list, this ) )
      return true;
#ifndef NDEBUG
    else
      p()->sim->print_log( "{} failed verification", ab::name() );
#endif

    return ab::verify_actor_spec();
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    auto pers = ab::composite_persistent_multiplier( s );

    for ( const auto& i : persistent_multiplier_effects )
      pers *= 1.0 + ab::get_effect_value( i );

    return pers;
  }

  size_t total_effects_count() const override
  {
    return ab::total_effects_count() + persistent_multiplier_effects.size();
  }

  void print_parsed_custom_type( report::sc_html_stream& os ) const override
  {
    ab::template print_parsed_type<base_t>( os, &base_t::persistent_multiplier_effects, "Snapshots" );
  }
};

template <typename BASE>
struct use_dot_list_t : public BASE
{
protected:
  using base_t = use_dot_list_t<BASE>;
  std::vector<dot_t*>* dot_list = nullptr;

public:
  use_dot_list_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE )
    : BASE( n, p, s, f )
  {}

  void init() override
  {
    assert( dot_list );

    BASE::init();
  }

  void trigger_dot( action_state_t* s ) override
  {
    dot_t* d = BASE::get_dot( s->target );
    if ( !d->is_ticking() )
    {
      assert( !range::contains( *dot_list, d ) );
      dot_list->push_back( d );
    }

    BASE::trigger_dot( s );
  }

  void last_tick( dot_t* d ) override
  {
    assert( range::contains( *dot_list, d ) );
    range::erase_remove( *dot_list, d );

    BASE::last_tick( d );
  }
};

template <form_e FORM, typename BASE>
struct use_fluid_form_t : public BASE
{
private:
  bool delayed_shift = false;

protected:
  using base_t = use_fluid_form_t<FORM, BASE>;

public:
  use_fluid_form_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE )
    : BASE( n, p, s, f )
  {
    if ( p->talent.fluid_form.ok() && !BASE::is_free() )
    {
      if constexpr ( FORM == MOONKIN_FORM )
        delayed_shift = p->active.shift_to_moonkin ? true : false;
      else if constexpr ( FORM == CAT_FORM )
        BASE::autoshift = p->active.shift_to_cat;
      else if constexpr ( FORM == BEAR_FORM )
        BASE::autoshift = p->active.shift_to_bear;
    }
  }

  void execute() override
  {
    if constexpr ( FORM == MOONKIN_FORM )
    {
      if ( delayed_shift && BASE::p()->form != MOONKIN_FORM )
        BASE::p()->active.shift_to_moonkin->execute();
    }

    BASE::execute();
  }
};

template <specialization_e S, typename BASE>
struct trigger_aggravate_wounds_t : public BASE
{
private:
  timespan_t dot_ext = 0_ms;
  timespan_t max_ext = 0_ms;

protected:
  using base_t = trigger_aggravate_wounds_t<S, BASE>;

public:
  trigger_aggravate_wounds_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE )
    : BASE( n, p, s, f )
  {
    if ( BASE::data().ok() && p->specialization() == S && p->talent.aggravate_wounds.ok() )
    {
      if constexpr ( S == DRUID_FERAL )
      {
        assert( BASE::base_costs[ RESOURCE_ENERGY ] > 0 );
        dot_ext = p->talent.aggravate_wounds->effectN( 2 ).time_value();
      }
      else if constexpr ( S == DRUID_GUARDIAN )
      {
        dot_ext = p->talent.aggravate_wounds->effectN( 1 ).time_value();
      }

      max_ext = timespan_t::from_seconds( p->talent.aggravate_wounds->effectN( 3 ).base_value() );
    }
  }

  void execute() override
  {
    // extends even if you hit nothing
    if ( dot_ext > 0_ms )
    {
      range::for_each( BASE::p()->dot_lists.dreadful_wound, [ this ]( dot_t* d ) {
        d->adjust_duration( dot_ext, max_ext );
      } );
    }

    BASE::execute();
  }
};

template <typename BASE>
struct trigger_atmospheric_exposure_t : public BASE
{
private:
  bool has_talent;

protected:
  using base_t = trigger_atmospheric_exposure_t<BASE>;

public:
  trigger_atmospheric_exposure_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE )
    : BASE( n, p, s, f ), has_talent( p->talent.atmospheric_exposure.ok() )
  {}

  void impact( action_state_t* s ) override
  {
    BASE::impact( s );

    if ( has_talent && s->result_total )
      BASE::td( s->target )->debuff.atmospheric_exposure->trigger( this );
  }

  void tick( dot_t* d ) override
  {
    BASE::tick( d );

    if ( has_talent && d->state->result_total )
      BASE::td( d->target )->debuff.atmospheric_exposure->trigger( this );
  }
};

template <typename BASE>
struct trigger_call_of_the_elder_druid_t : public BASE
{
private:
  druid_t* p_;
  bool check;

protected:
  using base_t = trigger_call_of_the_elder_druid_t<BASE>;

public:
  trigger_call_of_the_elder_druid_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE )
    : BASE( n, p, s, f ), p_( p ), check( !p->buff.oath_of_the_elder_druid->is_fallback )
  {}

  void execute() override
  {
    // triggers before the damage and buffs it
    if ( check && !p_->buff.oath_of_the_elder_druid->check() )
    {
      p_->buff.oath_of_the_elder_druid->trigger();
      p_->buff.call_of_the_elder_druid->trigger();
    }

    BASE::execute();
  }
};

template <specialization_e S, typename BASE>
struct trigger_claw_rampage_t : public BASE
{
private:
  buff_t* berserk_buff = nullptr;
  buff_t* ravage_buff = nullptr;
  cooldown_t* icd = nullptr;
  double proc_pct = 0.0;

protected:
  using base_t = trigger_claw_rampage_t<S, BASE>;

public:
  trigger_claw_rampage_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE )
    : BASE( n, p, s, f )
  {
    if ( p->specialization() == S && p->talent.claw_rampage.ok() )
    {
      proc_pct = p->talent.claw_rampage->effectN( 1 ).percent();
      icd = p->get_cooldown( "claw_rampage_icd" );
      icd->duration = p->talent.claw_rampage->internal_cooldown();

      if constexpr ( S == DRUID_FERAL )
      {
        berserk_buff = p->buff.b_inc_cat;
        ravage_buff = p->buff.ravage_fb;
      }
      else if constexpr ( S == DRUID_GUARDIAN )
      {
        berserk_buff = p->buff.b_inc_bear;
        ravage_buff = p->buff.ravage_maul;
      }
      else
      {
        static_assert( static_false<BASE>, "Invalid specialization for trigger_claw_rampage_t." );
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    BASE::impact( s );

    if ( proc_pct && berserk_buff->check() && icd->up() && BASE::rng().roll( proc_pct ) )
    {
      ravage_buff->trigger();
      icd->start();
    }
  }
};

template <typename BASE>
struct trigger_control_of_the_dream_t : public BASE
{
private:
  timespan_t max_diff = 0_ms;

protected:
  using base_t = trigger_control_of_the_dream_t<BASE>;

public:
  trigger_control_of_the_dream_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE )
    : BASE( n, p, s, f )
  {
    if ( p->talent.control_of_the_dream.ok() )
      max_diff = timespan_t::from_seconds( p->talent.control_of_the_dream->effectN( 1 ).base_value() );
  }

  void update_ready( timespan_t cd ) override
  {
    auto _diff = 0_ms;

    if ( max_diff > 0_ms && cd == timespan_t::min() && BASE::cooldown_duration() > 0_ms &&
         ( BASE::cooldown->charges == BASE::cooldown->current_charge ) )
    {
      // cooldown starts at max diff
      auto _last = BASE::cooldown->charges > 1 ? BASE::cooldown->last_charged : BASE::cooldown->ready;
      if ( _last <= 0_ms )
        _diff = max_diff;
      else
        _diff = std::min( max_diff, BASE::sim->current_time() - _last );

      BASE::sim->print_debug( "Control of the Dream: {} last_used={} adjust={}", *this, _last, _diff );
    }

    BASE::update_ready( cd );

    if ( _diff > 0_ms )
      BASE::cooldown->adjust( -_diff, false );
  }
};

template <typename BASE>
struct trigger_gore_t : public BASE
{
private:
  double _gore_chance;

protected:
  using base_t = trigger_gore_t<BASE>;

public:
  trigger_gore_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE )
    : BASE( n, p, s, f ), _gore_chance( p->talent.gore->effectN( 1 ).percent() )
  {}

  virtual double gore_chance()
  {
    return _gore_chance;
  }

  void impact( action_state_t* s ) override
  {
    BASE::impact( s );

    if ( BASE::rng().roll( gore_chance() ) )
      BASE::p()->buff.gore->trigger( this );
  }
};

template <typename BASE, typename DOT_BASE>
struct ravage_base_t : public BASE
{
  struct dreadful_wound_t final : public DOT_BASE
  {
    dreadful_wound_t( druid_t* p, std::string_view n, flag_e f ) : DOT_BASE( n, p, p->spec.dreadful_wound, f )
    {
      DOT_BASE::background = DOT_BASE::proc = true;

      DOT_BASE::name_str_reporting = "dreadful_wound";
      DOT_BASE::dot_name = "dreadful_wound";

      DOT_BASE::dot_list = &p->dot_lists.dreadful_wound;
    }

    void snapshot_state( action_state_t* s, unsigned fl, result_amount_type rt ) override
    {
      if constexpr ( std::is_base_of_v<cat_attack_t, DOT_BASE> )
      {
        auto prev_dot = DOT_BASE::get_dot( s->target );
        auto prev_tf = false;
        auto prev_mul = 1.0;

        if ( prev_dot->is_ticking() && DOT_BASE::cast_state( prev_dot->state )->snapshots & snapshot_e::TIGERS_FURY )
        {
          prev_tf = true;
          prev_mul = prev_dot->state->persistent_multiplier;
        }

        DOT_BASE::snapshot_state( s, fl, rt );

        if ( prev_tf )
        {
          s->persistent_multiplier = prev_mul;
          DOT_BASE::cast_state( s )->snapshots |= snapshot_e::TIGERS_FURY;
        }
      }
      else
      {
        DOT_BASE::snapshot_state( s, fl, rt );
      }
    }
  };

private:
  double aoe_coeff;

protected:
  using base_t = ravage_base_t<BASE, DOT_BASE>;

public:
  ravage_base_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE )
    : BASE( n, p, s, f )
  {
    BASE::name_str_reporting = "ravage";

    // the aoe effect is parsed last and overwrites the st effect, so we need to cache the aoe coeff and re-parse the
    // st effect
    aoe_coeff = BASE::attack_power_mod.direct;
    BASE::parse_effect_direct_mods( BASE::data().effectN( 1 ), false );
    BASE::aoe = -1;

    if ( p->talent.dreadful_wound.ok() )
    {
      auto suf = get_suffix( BASE::name_str, "ravage" );
      BASE::impact_action = p->get_secondary_action<dreadful_wound_t>( "dreadful_wound" + suf, f );
      BASE::add_child( BASE::impact_action );
    }
  }

  double attack_direct_power_coefficient( const action_state_t* s ) const override
  {
    return s->chain_target == 0 ? BASE::attack_direct_power_coefficient( s ) : aoe_coeff;
  }

  void impact( action_state_t* s ) override
  {
    BASE::impact( s );

    BASE::p()->buff.killing_strikes->trigger( this );
    BASE::p()->buff.ruthless_aggression->trigger( this );
  }
};

template <typename BASE>
struct trigger_thriving_growth_t : public BASE
{
private:
  druid_t* p_;

protected:
  using base_t = trigger_thriving_growth_t<BASE>;

public:
  trigger_thriving_growth_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE )
    : BASE( n, p, s, f ), p_( p )
  {}

  void tick( dot_t* d ) override
  {
    BASE::tick( d );

    switch ( d->state->result_type )
    {
      case result_amount_type::DMG_DIRECT:
      case result_amount_type::DMG_OVER_TIME:
        if ( p_->rngs.bloodseeker_vines )
          p_->rngs.bloodseeker_vines->trigger( d->state );
        break;
      case result_amount_type::HEAL_DIRECT:
      case result_amount_type::HEAL_OVER_TIME:
        if ( p_->rngs.symbiotic_blooms )
          p_->rngs.symbiotic_blooms->trigger( d->state );
        break;
      default:
        break;
    }
  }
};

template <typename BASE>
struct trigger_wild_guardian_echo_base_t : public BASE
{
protected:
  using base_t = trigger_wild_guardian_echo_base_t<BASE>;
  action_t* echo_action = nullptr;
  buff_t* echo_buff = nullptr;
  timespan_t repeat_delay = 0_ms;
  int num_repeat;

public:
  trigger_wild_guardian_echo_base_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f )
    : BASE( n, p, s, f ), num_repeat( as<int>( p->talent.wild_guardian_3->effectN( 3 ).base_value() ) )
  {
    if ( num_repeat )
      repeat_delay = p->talent.wild_guardian_3->effectN( 4 ).time_value() / num_repeat;
  }


  virtual void trigger_echoes()
  {
    make_event( *BASE::sim, BASE::rng().range( WILD_GUARDIAN_ECHO_DELAY ), [ this ] {
      echo_action->execute();

      if ( repeat_delay > 0_ms )
        make_repeating_event( *BASE::sim, repeat_delay, [ this ] { echo_action->execute(); }, num_repeat );
    } );
  }

  void execute() override
  {
    BASE::execute();

    if ( echo_action && echo_buff->check() )
    {
      echo_buff->decrement();
      trigger_echoes();
    }
  }
};

template <specialization_e S, typename BASE>
struct trigger_wildpower_surge_t : public BASE
{
private:
  buff_t* buff = nullptr;

protected:
  using base_t = trigger_wildpower_surge_t<S, BASE>;

public:
  trigger_wildpower_surge_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE )
    : BASE( n, p, s, f )
  {
    if ( p->specialization() == S && p->talent.wildpower_surge.ok() )
    {
      if constexpr ( S == DRUID_FERAL )
        buff = p->buff.ursine_potential_counter;
      else if constexpr ( S == DRUID_GUARDIAN )
        buff = p->buff.feline_potential_counter;
    }
  }

  void execute() override
  {
    BASE::execute();

    if ( buff )
      buff->trigger( this );
  }
};

// Druid melee attack base for cat_attack_t and bear_attack_t
template <class Base>
struct druid_attack_t : public druid_action_t<Base>
{
private:
  using ab = druid_action_t<Base>;

protected:
  using base_t = druid_attack_t<Base>;

public:
  druid_attack_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f ) : ab( n, p, s, f )
  {
    ab::special = true;

    for ( const auto& e : ab::data().effects() )
    {
      if ( ( e.type() == E_SCHOOL_DAMAGE || e.type() == E_WEAPON_PERCENT_DAMAGE ) &&
           ( e.mechanic() == MECHANIC_BLEED || ab::data().mechanic() == MECHANIC_BLEED ) )
      {
        ab::ignores_armor = true;
        break;
      }
    }
  }
};

// Druid "Spell" Base for druid_spell_t, druid_heal_t ( and potentially druid_absorb_t )
template <class Base>
struct druid_spell_base_t : public druid_action_t<Base>
{
private:
  using ab = druid_action_t<Base>;

protected:
  using base_t = druid_spell_base_t<Base>;
  bool reset_melee_swing = true;  // TRUE(default) to reset swing timer on execute (as most cast time spells do)

public:
  druid_spell_base_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f ) : ab( n, p, s, f ) {}

  void execute() override
  {
    if ( ab::trigger_gcd > 0_ms && !ab::proc && !ab::background && reset_melee_swing &&
         ab::p()->main_hand_attack && ab::p()->main_hand_attack->execute_event )
    {
      ab::p()->main_hand_attack->execute_event->reschedule( ab::p()->main_hand_weapon.swing_time *
                                                            ab::p()->cache.auto_attack_speed() );
    }

    ab::execute();
  }
};

struct druid_spell_t : public druid_spell_base_t<spell_t>
{
private:
  using ab = druid_spell_base_t<spell_t>;

public:
  druid_spell_t( std::string_view n, druid_t* p, const spell_data_t* s = spell_data_t::nil(), flag_e f = flag_e::NONE )
    : ab( n, p, s, f )
  {}
};

struct druid_heal_t : public druid_spell_base_t<heal_t>
{
  double forestwalk_mul;  // % healing from forestwalk
  double imp_fr_mul;      // % healing from improved frenzied regeneration
  double photo_mul;       // tick rate multiplier when lb is on self
  double photo_pct;       // % chance to bloom when lb is on other
  double iron_mul = 0.0;  // % healing from hots with ironbark
  bool target_self = false;

  druid_heal_t( std::string_view n, druid_t* p, const spell_data_t* s = spell_data_t::nil(), flag_e f = flag_e::NONE )
    : base_t( n, p, s, f ),
      forestwalk_mul( find_effect( p->buff.forestwalk, A_MOD_HEALING_RECEIVED_PCT ).percent() ),
      imp_fr_mul( find_effect( p->talent.verdant_heart, A_ADD_FLAT_MODIFIER, P_EFFECT_2 ).percent() ),
      photo_mul( p->talent.photosynthesis->effectN( 1 ).percent() ),
      photo_pct( p->talent.photosynthesis->effectN( 2 ).percent() )
  {
    add_option( opt_bool( "target_self", target_self ) );

    if ( target_self )
      target = p;

    may_miss = harmful = false;
    ignore_false_positive = true;

    if ( p->talent.stonebark.ok() && find_effect( p->talent.ironbark, this, A_MOD_HEALING_RECEIVED_FROM_SPELL ).ok() )
      iron_mul = find_effect( p->talent.stonebark, A_ADD_FLAT_MODIFIER, P_EFFECT_2 ).percent();
  }

  virtual double harmony_multiplier( player_t* t ) const
  {
    return p()->cache.mastery_value() * get_td( t )->hots_ticking();
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double ctm = base_t::composite_target_multiplier( t );

    if ( p()->specialization() == DRUID_RESTORATION )
    {
      ctm *= 1.0 + harmony_multiplier( t );

      if ( iron_mul && get_td( t )->buff.ironbark->up() )
         ctm *= 1.0 + iron_mul;
    }

    return ctm;
  }

  double tick_time_pct_multiplier( const action_state_t* s ) const override
  {
    auto mul = base_t::tick_time_pct_multiplier( s );

    // photo effect is a positive dummy value, so divide here
    if ( photo_mul && get_td( player )->hots.lifebloom->is_ticking() )
      mul /= 1.0 + photo_mul;

    return mul;
  }

  void tick( dot_t* d ) override
  {
    base_t::tick( d );

    if ( photo_pct && d->target != player && rng().roll( photo_pct ) )
    {
      auto lb = td( d->target )->hots.lifebloom;

      if ( lb->is_ticking() )
        lb->current_action->last_tick( lb );
    }
  }
};

struct cat_attack_data_t
{
  double energy_mod = 0.0;
  int combo_points = 0;
  uint8_t snapshots = 0;
  bool free = false;

  friend void sc_format_to( const cat_attack_data_t& data, fmt::format_context::iterator out )
  {
    std::vector<std::string> str;

    if ( data.combo_points )
      str.push_back( fmt::format( "combo_points={}", data.combo_points ) );

    if ( data.energy_mod )
      str.push_back( fmt::format( "energy_mod={}", data.energy_mod ) );

    if ( data.snapshots )
    {
      std::vector<std::string_view> snap_str;

      if ( data.snapshots & snapshot_e::TIGERS_FURY )
        snap_str.push_back( "tigers_fury" );

      str.push_back( fmt::format( "snapshots={}", fmt::join( snap_str, "|" ) ) );
    }

    if ( data.free )
      str.push_back( "(free)" );

    fmt::format_to( out, "{}{}", str.empty() ? "" : " ", fmt::join( str, " " ) );
  }
};

struct cat_attack_t : public druid_attack_t<melee_attack_t>
{
  std::vector<player_effect_t> persistent_periodic_effects;
  std::vector<player_effect_t> persistent_direct_effects;
  benefit_tracker_t* snapshot_tracker = nullptr;
  gain_t* clearcasting_gain = nullptr;
  gain_t* energy_refund_gain;
  double bleed_mul = 0.0;
  bool snapshot_tigers_fury = false;

  cat_attack_t( std::string_view n, druid_t* p, const spell_data_t* s = spell_data_t::nil(), flag_e f = flag_e::NONE )
    : base_t( n, p, s, f ), energy_refund_gain( p->get_gain( "Energy Refund" ) )
  {
    if ( p->specialization() == DRUID_BALANCE || p->specialization() == DRUID_RESTORATION )
    {
      ap_type = attack_power_type::NO_WEAPON;
    }
    else if ( p->specialization() == DRUID_FERAL && data().ok() )
    {
      if ( p->talent.omen_of_clarity_cat.ok() )
      {
        clearcasting_gain = p->get_gain( "Clearcasting" );

        parse_effects( p->buff.clearcasting_cat, CONSUME_BUFF, PARSE_CALLBACK_POST_SNAPSHOT, [ this, p = p ]( auto ) {
          clearcasting_gain->add( RESOURCE_ENERGY, base_cost() * ( 1.0 + p->buff.incarnation_cat->check_value() ) );
        } );
      }

      snapshot_tigers_fury = parse_tigers_fury();
    }
  }

  using state_t = druid_action_state_t<cat_attack_data_t>;

  action_state_t* new_state() override
  { return new state_t( this, target ); }

  state_t* cast_state( action_state_t* s )
  { return static_cast<state_t*>( s ); }

  const state_t* cast_state( const action_state_t* s ) const
  { return static_cast<const state_t*>( s ); }

  bool stealthed() const  // For effects that require any form of stealth.
  {
    return p()->buff.prowl->up() || p()->buffs.shadowmeld->up();
  }

  template <typename... Ts>
  bool parse_persistent_effects( buff_t* buff, Ts... mods )
  {
    size_t cost_old = cost_effects.size();
    size_t ta_old = ta_multiplier_effects.size();
    size_t da_old = da_multiplier_effects.size();

    parse_effects( buff, mods... );

    // new entry in both tables
    if ( ta_multiplier_effects.size() > ta_old && da_multiplier_effects.size() > da_old )
    {
      if ( ta_multiplier_effects.back().value == da_multiplier_effects.back().value )  // values are same
      {
        persistent_multiplier_effects.push_back( ta_multiplier_effects.back() );
        ta_multiplier_effects.pop_back();
        da_multiplier_effects.pop_back();
        p()->sim->print_debug( "persistent-effects: {} damage modified by {}% with buff {} ({})", *this,
                               persistent_multiplier_effects.back().value * 100.0, buff->name(), buff->data().id() );
      }
      else  // values are different
      {
        persistent_direct_effects.push_back( da_multiplier_effects.back() );
        da_multiplier_effects.pop_back();
        p()->sim->print_debug( "persistent-effects: {} direct damage modified by {}% with buff {} ({})", *this,
                               persistent_direct_effects.back().value * 100.0, buff->name(), buff->data().id() );

        persistent_periodic_effects.push_back( ta_multiplier_effects.back() );
        ta_multiplier_effects.pop_back();
        p()->sim->print_debug( "persistent-effects: {} periodic damage modified by {}% with buff {} ({})", *this,
                               persistent_periodic_effects.back().value * 100.0, buff->name(), buff->data().id() );
      }

      return true;
    }

    // no persistent multiplier, but does snapshot & consume the buff
    if ( da_multiplier_effects.size() > da_old || cost_effects.size() > cost_old )
      return true;

    return false;
  }

  size_t total_effects_count() const override
  {
    return base_t::total_effects_count() + persistent_periodic_effects.size() + persistent_direct_effects.size();
  }

  void print_parsed_custom_type( report::sc_html_stream& os ) const override
  {
    base_t::print_parsed_custom_type( os );

    base_t::template print_parsed_type<cat_attack_t>( os, &cat_attack_t::persistent_periodic_effects,
                                                      "Snapshots (DOT)" );
    base_t::template print_parsed_type<cat_attack_t>( os, &cat_attack_t::persistent_direct_effects,
                                                      "Snapshots (Direct)" );
  }

  virtual bool parse_tigers_fury()
  {
    return parse_persistent_effects( p()->buff.tigers_fury, PARSE_CALLBACK_POST_SNAPSHOT,
      [ this ]( action_state_t* s ) {
        cast_state( s )->snapshots |= snapshot_e::TIGERS_FURY;
      } );
  }

  void init_finished() override
  {
    base_t::init_finished();

    if ( snapshot_tigers_fury )
      snapshot_tracker = benefit_tracker_t::get_tracker<SNAPSHOT_TRACKER>( p(), this );
  }

  void trigger_unseen_attack( int cp, double energy_mod )
  {
    assert( p()->active.unseen_slash && p()->active.unseen_swipe );

    // pick a random target
    auto _tar = rng().range( p()->active.unseen_slash->target_list() );

    // set swipe to target to check for # of targets in radius
    p()->active.unseen_swipe->set_target( _tar );

    if ( p()->active.unseen_swipe->target_list().size() > UNSEEN_SWIPE_TARGETS )
    {
      snapshot_and_execute( p()->active.unseen_swipe, nullptr, false, [ this, cp, energy_mod ]( auto, auto to ) {
        auto state = cast_state( to );
        state->combo_points = cp;
        state->energy_mod = energy_mod;
      } );
    }
    else
    {
      snapshot_and_execute( p()->active.unseen_slash, nullptr, false, [ this, cp, energy_mod, _tar ]( auto, auto to ) {
        auto state = cast_state( to );
        state->combo_points = cp;
        state->energy_mod = energy_mod;
        state->target = _tar;
        p()->active.unseen_slash->set_target( _tar );
      } );
    }
  }

  void execute() override
  {
    base_t::execute();

    if ( !hit_any_target )
      player->resource_gain( RESOURCE_ENERGY, last_resource_cost * 0.80, energy_refund_gain );

    if ( harmful )
    {
      if ( special && base_costs[ RESOURCE_ENERGY ] > 0 )
        p()->buff.incarnation_cat->up();
    }
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( snapshot_tracker && s->result_amount > 0 )
      snapshot_tracker->count_direct<SNAPSHOT_TRACKER>( cast_state( s )->snapshots );
  }

  void trigger_dot( action_state_t* s ) override
  {
    // tiger's fury can have different persistent multipliers for DMG_DIRECT vs DMG_OVER_TIME
    // because the state is released after impact, there is no downstream concerns
    if ( snapshot_tigers_fury && s->result_type != result_amount_type::DMG_OVER_TIME )
    {
      s->result_type = result_amount_type::DMG_OVER_TIME;
      s->persistent_multiplier = composite_persistent_multiplier( s );
    }

    base_t::trigger_dot( s );
  }

  void tick( dot_t* d ) override
  {
    base_t::tick( d );

    if ( snapshot_tracker && d->state->result_amount > 0 )
      snapshot_tracker->count_tick<SNAPSHOT_TRACKER>( cast_state( d->state )->snapshots );
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    auto pers = base_t::composite_persistent_multiplier( s );

    if ( s->result_type == result_amount_type::DMG_DIRECT )
    {
      for ( const auto& i : persistent_direct_effects )
        pers *= 1.0 + base_t::get_effect_value( i );
    }
    else
    {
      for ( const auto& i : persistent_periodic_effects )
        pers *= 1.0 + base_t::get_effect_value( i );
    }

    return pers;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double tm = base_t::composite_target_multiplier( t );

    if ( bleed_mul && t->debuffs.bleeding && t->debuffs.bleeding->up() )
      tm *= 1.0 + bleed_mul;

    return tm;
  }
};

struct bear_attack_t : public druid_attack_t<melee_attack_t>
{
  bear_attack_t( std::string_view n, druid_t* p, const spell_data_t* s = spell_data_t::nil(), flag_e f = flag_e::NONE )
    : base_t( n, p, s, f )
  {
    if ( p->specialization() == DRUID_BALANCE || p->specialization() == DRUID_RESTORATION )
      ap_type = attack_power_type::NO_WEAPON;
  }
};

template <class Base, bool DOT = false>
struct druid_residual_action_t : public Base
{
protected: 
  using base_t = druid_residual_action_t<Base, DOT>;
  double residual_mul = 1.0;

public:
  druid_residual_action_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE )
    : Base( n, p, s, f )
  {
    // TODO: determine if these shoule be background as well
    Base::proc = true;
    Base::round_base_dmg = false;

    Base::attack_power_mod.direct = Base::attack_power_mod.tick = 0;
    Base::spell_power_mod.direct = Base::spell_power_mod.tick = 0;
    Base::weapon_multiplier = 0;
  }

  void execute() override
  {
    if constexpr ( DOT )
    {
      if ( Base::sim->debug )
      {
        Base::sim->print_debug( "Executing periodic residual action: base_td={} residual_mul={}", Base::base_td,
                                residual_mul );
      }
    }
    else
    {
      if ( Base::sim->debug )
      {
        Base::sim->print_debug( "Executing direct residual action: base_dd_min={} residual_mul={}", Base::base_dd_min,
                                residual_mul );
      }
    }

    Base::execute();
  }

  double base_da_min( const action_state_t* ) const override
  {
    if constexpr ( !DOT )
      return Base::base_dd_min * residual_mul;
    else
      return 0.0;
  }

  double base_da_max( const action_state_t* ) const override
  {
    if constexpr ( !DOT )
      return Base::base_dd_min * residual_mul;
    else
      return 0.0;
  }

  double base_ta( const action_state_t* ) const override
  {
    if constexpr ( DOT )
      return Base::base_td * residual_mul;
    else
      return 0.0;
  }
};

namespace buffs
{
using druid_buff_t = druid_buff_base_t<>;

template <typename DATA>
struct druid_data_buff_t : public druid_buff_t
{
  DATA data;

  druid_data_buff_t( druid_t* p, std::string_view n, const spell_data_t* s = spell_data_t::nil() )
    : druid_buff_t( p, n, s ), data()
  {}
};

// data buff aliases
using treants_of_the_moon_buff_t = druid_data_buff_t<std::set<pets::treant_base_t*>>;

struct druid_absorb_buff_t : public druid_buff_base_t<absorb_buff_t>
{
protected:
  using base_t = druid_absorb_buff_t;

public:
  druid_absorb_buff_t( druid_t* p, std::string_view n, const spell_data_t* s ) : druid_buff_base_t( p, n, s )
  {
    set_absorb_gain( p->get_gain( absorb_name() ) );
    set_absorb_source( p->get_stats( absorb_name() ) );
  }

  bool trigger( int s, double v, double c, timespan_t d ) override
  {
    auto ret = druid_buff_base_t::trigger( s, v, c, d );
    if ( ret && !quiet )
      absorb_source->add_execute( 0_ms, player );

    return ret;
  }

  std::string absorb_name() const
  {
    return util::inverse_tokenize( name_str ) + " (absorb)";
  }

  double attack_power() const
  {
    return p()->composite_total_attack_power_by_type( p()->default_ap_type() );
  }
};

// Shapeshift Form Buffs ====================================================

struct swap_melee_t
{
private:
  druid_t* p_;

public:
  swap_melee_t( druid_t* p ) : p_( p ) {}

  // Used when shapeshifting to switch to a new attack & schedule it to occur
  // when the current swing timer would have ended.
  void swap_melee( attack_t* new_attack, weapon_t& new_weapon )
  {
    if ( p_->main_hand_attack && p_->main_hand_attack->execute_event )
    {
      new_attack->base_execute_time = new_weapon.swing_time;
      new_attack->execute_event =
        new_attack->start_action_execute_event( p_->main_hand_attack->execute_event->remains() );
      p_->main_hand_attack->cancel();
    }
    new_attack->weapon = &new_weapon;
    p_->main_hand_attack = new_attack;
    p_->main_hand_weapon = new_weapon;
  }
};

// Bear Form ================================================================
struct bear_form_buff_t final : public druid_buff_t, public swap_melee_t
{
  gain_t* shapeshifting_gain;
  gain_t* bear_form_gain;
  double rage_gain;

  bear_form_buff_t( druid_t* p )
    : base_t( p, "bear_form", p->spec.bear_form ),
      swap_melee_t( p ),
      shapeshifting_gain( p->get_gain( "Shapeshifting" ) ),
      bear_form_gain( p->get_gain( "Bear Form" ) ),
      rage_gain( find_effect( p->find_spell( 17057 ), E_ENERGIZE ).resource( RESOURCE_RAGE ) )
  {
    add_invalidate( CACHE_ARMOR );
    add_invalidate( CACHE_CRIT_AVOIDANCE );
    add_invalidate( CACHE_EXP );
    add_invalidate( CACHE_STAMINA );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    base_t::expire_override( expiration_stacks, remaining_duration );

    swap_melee( p()->caster_melee_attack, p()->caster_form_weapon );

    p()->resource_loss( RESOURCE_RAGE, p()->resources.current[ RESOURCE_RAGE ], shapeshifting_gain);
    p()->recalculate_resource_max( RESOURCE_HEALTH );

    p()->buff.persistence->trigger();

    make_event( *sim, [ this ] {
      if ( p()->talent.wildshape_mastery.ok() && p()->form == CAT_FORM )
      {
        p()->buff.wildshape_mastery->trigger();
      }
      else
      {
        p()->buff.ironfur->expire();
        p()->buff.echo_of_ironfur->expire();
      }
    } );
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    swap_melee( p()->bear_melee_attack, p()->bear_weapon );

    base_t::start( stacks, value, duration );

    p()->resource_gain( RESOURCE_RAGE, rage_gain, bear_form_gain );
    p()->recalculate_resource_max( RESOURCE_HEALTH );

    p()->buff.persistence->expire();
    p()->buff.ursine_vigor->trigger();

    if ( p()->buff.ursine_potential_counter->at_max_stacks() )
    {
      p()->buff.ursine_potential_counter->expire();
      p()->buff.ursine_potential->trigger();
    }
  }
};

// Cat Form =================================================================
struct cat_form_buff_t final : public druid_buff_t, public swap_melee_t
{
  cat_form_buff_t( druid_t* p ) : base_t( p, "cat_form", p->spec.cat_form ), swap_melee_t( p ) {}

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    base_t::expire_override( expiration_stacks, remaining_duration );

    swap_melee( p()->caster_melee_attack, p()->caster_form_weapon );
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    swap_melee( p()->cat_melee_attack, p()->cat_weapon );

    base_t::start( stacks, value, duration );

    if ( p()->buff.feline_potential_counter->at_max_stacks() )
    {
      p()->buff.feline_potential_counter->expire();
      p()->buff.feline_potential->trigger();
    }
  }
};

// Moonkin Form =============================================================
struct moonkin_form_buff_t final : public druid_buff_t
{
  moonkin_form_buff_t( druid_t* p ) : base_t( p, "moonkin_form", p->talent.moonkin_form )
  {
    add_invalidate( CACHE_ARMOR );
  }
};

// Brambles =================================================================
struct brambles_buff_t final : public druid_absorb_buff_t
{
  double coeff;

  brambles_buff_t( druid_t* p )
    : base_t( p, "brambles", p->talent.brambles ),
      coeff( find_effect( this, A_SCHOOL_ABSORB ).ap_coeff() )
  {
    set_quiet( true );
    set_default_value( 1 );
    set_absorb_high_priority( true );
  }

  double consume( double a, action_state_t* s ) override
  {
    auto amount = std::min( a, coeff * attack_power() );

    absorb_source->add_result( amount, 0, result_amount_type::ABSORB, RESULT_HIT, BLOCK_RESULT_UNBLOCKED, player );
    absorb_gain->add( RESOURCE_HEALTH, amount, 0 );

    sim->print_debug( "{} {} absorbs {}", *player, *this, amount );

    if ( s && s->action->player && s->action->player != player )
      p()->active.brambles_reflect->execute_on_target( s->action->player, amount );

    return amount;
  }
};

// Celestial Alignment ======================================================
struct celestial_alignment_buff_t final : public druid_buff_t
{
  celestial_alignment_buff_t( druid_t* p, std::string_view n, const spell_data_t* s ) : base_t( p, n, s )
  {
    set_cooldown( 0_ms );

    if ( p->talent.celestial_alignment.ok() )
    {
      set_default_value_from_effect_type( A_HASTE_ALL );
      set_pct_buff_type( STAT_PCT_BUFF_HASTE );
    }
  }

  bool trigger( int s, double v, double c, timespan_t d ) override
  {
    auto ret = base_t::trigger( s, v, c, d );
    if ( !ret )
      return false;

    auto dur_ = remains();

    p()->buff.eclipse_lunar->trigger( dur_ );
    p()->buff.eclipse_solar->trigger( dur_ );

    if ( p()->active.orbital_strike )
      p()->active.orbital_strike->execute_on_target( p()->target );

    return true;
  }

  void expire_override( int s, timespan_t d ) override
  {
    base_t::expire_override( s, d );

    p()->buff.eclipse_lunar->expire();
    p()->buff.eclipse_solar->expire();
  }
};

// Eclipse ==================================================================
struct eclipse_buff_base_t : public druid_buff_t
{
  buff_t* boat_buff = nullptr;
  cooldown_t* bolt_cd = nullptr;
  double challenge_ap;
  double harmony_val, harmony_cap;
  double harmony_cur = 0.0;
  unsigned num_bolts = 0;

  eclipse_buff_base_t( druid_t* p, std::string_view n, const spell_data_t* s )
    : base_t( p, n, s ),
      challenge_ap( p->talent.elunes_challenge->effectN( 1 ).base_value() ),
      harmony_val( p->talent.harmony_of_the_heavens->effectN( 1 ).percent() ),
      harmony_cap( p->talent.harmony_of_the_heavens->effectN( 2 ).percent() )
  {
    set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_GENERIC );
    set_refresh_behavior( buff_refresh_behavior::DURATION );
    set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

    if ( p->talent.ascendant_eclipses_3.ok() )
    {
      bolt_cd = p->get_cooldown( name_str + "_bolt_cd" );
      bolt_cd->duration = p->talent.ascendant_eclipses_3->internal_cooldown();
    }
  }

  double check_value() const override
  {
    return current_value + harmony_cur;
  }

  void trigger_harmony()
  {
    if ( harmony_cur >= harmony_cap )
      return;

    // harmory is queued after cast
    make_event( *sim, 1_ms, [ this ] {
      if ( sim->debug )
      {
        sim->print_debug( "{} increasing {} Harmony of the Heavens by {}% from {}% to {}%", *p(), *this,
                          harmony_val * 100.0, harmony_cur * 100.0,
                          std::min( harmony_cur + harmony_val, harmony_cap ) * 100.0 );
      }

      harmony_cur = std::min( harmony_cur + harmony_val, harmony_cap );
    } );
  }

  virtual void trigger_boat_buff() = 0;
  virtual void execute_bolt_action() = 0;

  bool trigger( int s, double v, double c, timespan_t d ) override
  {
    auto ret = base_t::trigger( s, v, c, d );
    if ( !ret )
      return false;

    harmony_cur = 0.0;

    p()->buff.starlord->expire();
    p()->buff.elunes_challenge->expire();

    if ( p()->active.sylvan_beckoning && !p()->buff.sylvan_beckoning->check() )
    {
      p()->buff.sylvan_beckoning->trigger();
      p()->active.sylvan_beckoning->execute();
    }

    trigger_boat_buff();

    if ( bolt_cd && bolt_cd->up() )
    {
      execute_bolt_action();
      bolt_cd->start();
    }

    p()->buff.astral_communion->trigger();
    p()->buff.cenarius_might->trigger();
    p()->buff.parting_skies->trigger();

    // subsequent effects are queued after eclipse application
    make_event( *sim, 1_ms, [ this ] {
      p()->buff.ascendant_fires->trigger();
      p()->buff.ascendant_stars->trigger();

      p()->buff.solstice->expire();
      if ( p()->talent.solstice.ok() )
        p()->buff.solstice->trigger();
    } );

    return true;
  }

  // override expire() since we need to indicate whether dreamstate stacks are granted or not
  void expire( timespan_t d ) override
  {
    // hijack expire delay to indicate that dreamstate should not be granted
    base_t::expire( 0_ms );

    if ( p()->resources.current[ RESOURCE_ASTRAL_POWER ] < challenge_ap )
      p()->buff.elunes_challenge->trigger();

    if ( d == 0_ms )
      p()->buff.dreamstate->trigger();  
  }
};

struct eclipse_lunar_t final : public eclipse_buff_base_t
{
  eclipse_lunar_t( druid_t* p ) : eclipse_buff_base_t( p, "eclipse_lunar", p->spec.eclipse_lunar )
  {
    num_bolts = static_cast<unsigned>( p->talent.ascendant_eclipses_3->effectN( 3 ).base_value() );
  }

  void trigger_boat_buff() override
  {
    p()->buff.balance_of_all_things_arcane->trigger();
  }

  void execute_bolt_action() override
  {
    if ( p()->active.lunar_bolt )
    {
      p()->active.lunar_bolt->execute();

      make_repeating_event( *sim, LUNAR_BOLT_DELAY, [ bolt = p()->active.lunar_bolt ] {
        bolt->execute();
      }, num_bolts - 1 );
    }
  }
};

struct eclipse_solar_t final : public eclipse_buff_base_t
{
  eclipse_solar_t( druid_t* p ) : eclipse_buff_base_t( p, "eclipse_solar", p->spec.eclipse_solar )
  {
    num_bolts = static_cast<unsigned>( p->talent.ascendant_eclipses_3->effectN( 2 ).base_value() );
  }

  void trigger_boat_buff() override
  {
    p()->buff.balance_of_all_things_nature->trigger();
  }

  void execute_bolt_action() override
  {
    if ( p()->active.solar_bolt )
      p()->active.solar_bolt->execute();
  }
};

// Fury of Elune AP =========================================================
struct fury_of_elune_buff_t final : public druid_buff_t
{
  fury_of_elune_buff_t( druid_t* p, std::string_view n, const spell_data_t* s ) : base_t( p, n, s )
  {
    set_cooldown( 0_ms );
    set_refresh_behavior( buff_refresh_behavior::DURATION );

    auto power = p->specialization() == DRUID_GUARDIAN ? POWER_RAGE : POWER_ASTRAL_POWER;
    const auto& eff = find_effect( this, A_PERIODIC_ENERGIZE, power );
    auto r_type = eff.resource_gain_type();
    auto amt = eff.resource();
    auto gain = p->get_gain( n );

    set_default_value( amt / eff.period().total_seconds() );

    if ( p->talent.dryads_dance.ok() && p->talent.sylvan_beckoning.ok() )
    {
      auto dd_mul = 1.0 + find_effect( p->find_spell( 1264618 ), this ).percent();

      set_tick_callback( [ this, r_type, amt, gain, dd_mul, p = p ]( buff_t*, int, timespan_t ) {
        player->resource_gain( r_type, p->buff.sylvan_beckoning->check() ? amt * dd_mul : amt, gain );
      } );
    }
    else
    {
      set_tick_callback( [ this, r_type, amt, gain ]( buff_t*, int, timespan_t ) {
        player->resource_gain( r_type, amt, gain );
      } );
    }
  }
};

// Matted Fur ================================================================
struct matted_fur_buff_t final : public druid_absorb_buff_t
{
  double coeff;

  matted_fur_buff_t( druid_t* p )
    : base_t( p, "matted_fur", find_trigger( p->talent.matted_fur ).trigger() ),
      coeff( find_trigger( p->talent.matted_fur ).percent() * 1.25 )
  {}

  bool trigger( int s, double, double c, timespan_t d ) override
  {
    auto amount = attack_power() * coeff;
    amount *= 1.0 + p()->composite_heal_versatility();
    amount *= 1.0 + p()->composite_player_absorb_received_multiplier();

    return base_t::trigger( s, amount, c, d );
  }
};

// Shooting Stars ============================================================
struct shooting_stars_buff_t final : public druid_buff_t
{
  std::vector<dot_t*>& dot_list;
  action_t*& shooting;
  double base_chance;

  shooting_stars_buff_t( druid_t* p, std::string_view n, std::vector<dot_t*>& d, action_t*& shs )
    : base_t( p, n, p->talent.shooting_stars ),
      dot_list( d ),
      shooting( shs ),
      base_chance( find_effect( p->talent.shooting_stars, A_PERIODIC_DUMMY ).percent() )
  {
    set_quiet( true );
    set_tick_zero( true );
    set_tick_callback( [ this ]( buff_t*, int, timespan_t ) { trigger_shooting_stars(); } );
  }

  void trigger_shooting_stars()
  {
    auto n = dot_list.size();
    if ( !n )
      return;

    double c = base_chance;
    c *= 1.0 + p()->buff.solstice->value();

    if ( n > 1 )
    {
      c *= std::sqrt( n );
      rng().shuffle( dot_list.begin(), dot_list.end() );
    }

    double procs;  // no need to initialize, modf will set it
    if ( auto f = std::modf( c, &procs ); rng().roll( f ) )
      procs++;

    assert( procs <= n );

    for ( size_t i = 0; i < procs; i++ )
        shooting->execute_on_target( dot_list[ i ]->target );
  }
};
}  // end namespace buffs

// constructor macro for foreground abilities
#define DRUID_ABILITY( _class, _base, _name, _spell ) \
  _class( druid_t* p, std::string_view n = _name, const spell_data_t* s = nullptr, flag_e f = flag_e::NONE ) \
    : _base( n, p, s ? s : _spell, f )

// base class with additional optional arguments
#define DRUID_ABILITY_B( _class, _base, _name, _spell, ... ) \
  _class( druid_t* p, std::string_view n = _name, const spell_data_t* s = nullptr, flag_e f = flag_e::NONE ) \
    : _base( n, p, s ? s : _spell, f, __VA_ARGS__ )

// child class with additional optional arguments
#define DRUID_ABILITY_C( _class, _base, _name, _spell, ... ) \
  _class( druid_t* p, std::string_view n = _name, const spell_data_t* s = nullptr, flag_e f = flag_e::NONE, __VA_ARGS__ ) \
    : _base( n, p, s ? s : _spell, f )

template <typename T, bool PROC, typename... Ts>
T* druid_t::get_secondary_action( std::string_view n, Ts&&... args )
{
  if ( auto it = range::find( secondary_action_list, n, &action_t::name_str ); it != secondary_action_list.cend() )
    return dynamic_cast<T*>( *it );

  T* a;

  if constexpr ( std::is_constructible_v<T, std::string_view, Ts...> )
    a = new T( n, std::forward<Ts>( args )... );
  else if constexpr ( std::is_constructible_v<T, druid_t*, std::string_view, Ts...> )
    a = new T( this, n, std::forward<Ts>( args )... );
  else if constexpr ( std::is_constructible_v<T, druid_t*, Ts...> )
    a = new T( this, std::forward<Ts>( args )... );
  else if constexpr ( std::is_constructible_v<T, druid_t*, std::string_view, const spell_data_t*, Ts...> )
    a = new T( this, n, nullptr, std::forward<Ts>( args )... );
  else if constexpr ( std::is_constructible_v<T, Ts...> )
    a = new T( std::forward<Ts>( args )... );
  else
    static_assert( static_false<T>, "Invalid constructor arguments to get_secondary_action" );

  secondary_action_list.push_back( a );

  if constexpr ( PROC )
  {
    a->proc = true;
  }

  return a;
}

namespace spells
{
struct druid_interrupt_t : public druid_spell_t
{
  druid_interrupt_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f ) : druid_spell_t( n, p, s, f )
  {
    ignore_false_positive = use_off_gcd = is_interrupt = true;
  }

  bool target_ready( player_t* t ) override
  {
    if ( !t->debuffs.casting->check() )
      return false;

    return druid_spell_t::target_ready( t );
  }
};

// Form Spells ==============================================================
struct druid_form_t : public druid_spell_t
{
  buff_t* form_buff = nullptr;
  buff_t* lycara_buff = nullptr;
  form_e form = NO_FORM;

  druid_form_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f ) : druid_spell_t( n, p, s, f )
  {
    harmful = reset_melee_swing = false;
    ignore_false_positive = true;
    target = p;

    action_flags |= flag_e::NOUNSHIFT;
  }

  void set_form( form_e f )
  {
    form = f;
    form_mask = ANY_FORM & ~form;
    form_buff = get_form_buff( form );

    if ( p()->talent.lycaras_teachings.ok() )
      lycara_buff = get_lycara_buff( form );
  }

  void execute() override
  {
    druid_spell_t::execute();

    shapeshift();
  }

  buff_t* get_form_buff( form_e f )
  {
    switch ( f )
    {
      case BEAR_FORM:    return p()->buff.bear_form;
      case CAT_FORM:     return p()->buff.cat_form;
      case MOONKIN_FORM: return p()->buff.moonkin_form;
      case NO_FORM:      return nullptr;
      default:           assert( false ); return nullptr;
    }
  }

  buff_t* get_lycara_buff( form_e f )
  {
    switch ( f )
    {
      case BEAR_FORM:    return p()->buff.lycaras_teachings_vers;
      case CAT_FORM:     return p()->buff.lycaras_teachings_crit;
      case MOONKIN_FORM: return p()->buff.lycaras_teachings_mast;
      case NO_FORM:      return p()->buff.lycaras_teachings_haste;
      default:           assert( false ); return nullptr;
    }
  }

  void shapeshift()
  {
    auto old_form = p()->form;
    if ( old_form == form )
      return;

    if ( auto old_buff = get_form_buff( old_form ) )
      old_buff->expire();

    if ( lycara_buff )
      get_lycara_buff( old_form )->expire();

    p()->form = form;

    if ( form_buff )
      form_buff->trigger();

    if ( lycara_buff )
      lycara_buff->trigger();
  }
};

// Bear Form Spell ==========================================================
struct bear_form_t final : public druid_form_t
{
  DRUID_ABILITY( bear_form_t, druid_form_t, "bear_form", p->spec.bear_form )
  {
    set_form( BEAR_FORM );
  }
};

// Cat Form Spell ===========================================================
struct cat_form_t final : public druid_form_t
{
  DRUID_ABILITY( cat_form_t, druid_form_t, "cat_form", p->spec.cat_form )
  {
    set_form( CAT_FORM );
  }
};

// Moonkin Form Spell =======================================================
struct moonkin_form_t final : public druid_form_t
{
  DRUID_ABILITY( moonkin_form_t, druid_form_t, "moonkin_form", p->talent.moonkin_form )
  {
    set_form( MOONKIN_FORM );
  }
};

// Cancelform (revert to caster form)========================================
struct cancel_form_t final : public druid_form_t
{
  DRUID_ABILITY( cancel_form_t, druid_form_t, "cancelform", spell_data_t::nil() )
  {
    callbacks = false;

    set_form( NO_FORM );

    trigger_gcd = 0_ms;
    use_off_gcd = true;
  }
};
}  // namespace spells

namespace cat_attacks
{
struct cp_generator_t : public trigger_aggravate_wounds_t<DRUID_FERAL, cat_attack_t>
{
protected:
  gain_t* pf_gain = nullptr;
  double pf_cp = 0.0;

public:
  cp_generator_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE )
    : base_t( n, p, s, f )
  {
    auto m_data = p->get_modified_spell( &data() );
    set_energize( m_data );

    if ( const auto& eff = p->spec.berserk_cat->effectN( 2 ); energize && !energize->modified_by( eff ) )
    {
      energize->add_parse_entry()
        .set_buff( p->buff.b_inc_cat )
        .set_func( []( const action_t* a, const action_state_t* ) { return a ? !a->proc : false; } )
        .set_flat( true )
        .set_value( eff.base_value() )
        .set_eff( &eff );
    }

    if ( p->talent.primal_fury.ok() )
    {
      pf_gain = p->get_gain( "Primal Fury" );
      pf_cp = find_effect( find_trigger( p->talent.primal_fury ).trigger(), E_ENERGIZE ).resource();
    }
  }

  virtual void trigger_primal_fury()
  {
    if ( crit_any_target && pf_cp )
    {
      // scheduled to happen after action as the energize itself is a separate action
      make_event( *sim, [ this ] { p()->resource_gain( RESOURCE_COMBO_POINT, pf_cp, pf_gain ); } );
    }
  }

  void execute() override
  {
    base_t::execute();

    if ( p()->buff.stalking_predator->consume( this ) )
      trigger_unseen_attack( as<int>( p()->resources.max[ RESOURCE_COMBO_POINT ] ), 1.0 );
  }

  double gain_energize_resource( resource_e rt, double a, gain_t* g ) override
  {
    auto ret = base_t::gain_energize_resource( rt, a, g );

    // technically requires cat form, but as currently the only way to cast cp generators outside of form is via
    // convoke, and convoke doesn't trigger as it is a proc, we are fine for now
    if ( !proc )
      trigger_primal_fury();

    return ret;
  }
};

struct cp_spender_t : public trigger_aggravate_wounds_t<DRUID_FERAL, cat_attack_t>
{
private:
  gain_t* sotf_gain = nullptr;
  double sotf_energy;

public:
  cp_spender_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE )
    : base_t( n, p, s, f ), sotf_energy( p->talent.soul_of_the_forest_cat->effectN( 1 ).resource( RESOURCE_ENERGY ) )
  {
    if ( p->talent.soul_of_the_forest_cat.ok() )
      sotf_gain = p->get_gain( "Soul of the Forest" );
  }

  // used during state snapshot
  virtual int _combo_points() const
  {
    return as<int>( p()->resources.current[ RESOURCE_COMBO_POINT ] );
  }

  // used during consume resource
  virtual int _consumed_combo_points() const
  {
    return _combo_points();
  }

  void snapshot_state( action_state_t* s, result_amount_type rt ) override
  {
    // snapshot the combo point first so composite_X calculations can correctly refer to it
    cast_state( s )->combo_points = _combo_points();

    base_t::snapshot_state( s, rt );
  }

  bool ready() override
  {
    if ( !background && p()->resources.current[ RESOURCE_COMBO_POINT ] < 1 )
      return false;

    return base_t::ready();
  }

  bool trigger_with_chance_per_cp( buff_t* buff, int cp )
  {
    return buff->trigger( this, 1, buff_t::DEFAULT_VALUE(), buff->default_chance * cp );
  }

  void consume_resource() override
  {
    base_t::consume_resource();

    if ( background || !hit_any_target )
      return;

    auto cp = _combo_points();
    auto consumed = _consumed_combo_points();

    if ( sotf_gain )
      p()->resource_gain( RESOURCE_ENERGY, cp * sotf_energy, sotf_gain );

    trigger_with_chance_per_cp( p()->buff.frantic_momentum, cp );
    trigger_with_chance_per_cp( p()->buff.predatory_swiftness, cp );
    trigger_with_chance_per_cp( p()->buff.sudden_ambush, cp );

    if ( consumed )
    {
      if ( sim->log )
        sim->print_log( "{} consumes {} combo_points for {} (0)", *player, consumed, *this );

      p()->resource_loss( RESOURCE_COMBO_POINT, consumed, nullptr, this );
      stats->consume_resource( RESOURCE_COMBO_POINT, consumed );

      p()->buff.overflowing_power->consume( this );
    }

    p()->buff.tigers_tenacity->consume( this );
  }
};

template <typename BASE>
struct trigger_panthers_guile_t : public BASE
{
private:
  cooldown_t* pg_icd = nullptr;
  gain_t* pg_gain = nullptr;
  double pg_pct = 0.0;

protected:
  using base_t = trigger_panthers_guile_t<BASE>;

public:
  trigger_panthers_guile_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE )
    : BASE( n, p, s, f )
  {
    if ( p->talent.panthers_guile.ok() )
    {
      pg_icd = p->get_cooldown( "Panther's Guile" );
      pg_icd->duration = p->talent.panthers_guile->internal_cooldown();

      pg_gain = p->get_gain( "Panther's Guile" );

      pg_pct = p->talent.panthers_guile->proc_chance();
    }
  }

  double gain_energize_resource( resource_e rt, double a, gain_t* g ) override
  {
    auto ret = BASE::gain_energize_resource( rt, a, g );

    if ( rt == RESOURCE_COMBO_POINT && ret && pg_icd && pg_icd->up() && BASE::rng().roll( pg_pct ) )
    {
      auto diff =
        BASE::p()->resources.max[ RESOURCE_COMBO_POINT ] - BASE::p()->resources.current[ RESOURCE_COMBO_POINT ];

      if ( diff > 0 )
      {
        pg_icd->start();
        BASE::p()->resource_gain( RESOURCE_COMBO_POINT, diff, pg_gain );
      }
    }

    return ret;
  }
};

// Berserk (Cat) ==============================================================
struct berserk_cat_base_t : public cat_attack_t
{
  buff_t* buff;

  berserk_cat_base_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f )
    : cat_attack_t( n, p, s, f ), buff( p->buff.berserk_cat )
  {
    harmful   = false;
    form_mask = CAT_FORM;
    autoshift = p->active.shift_to_cat;
  }

  void execute() override
  {
    cat_attack_t::execute();

    buff->extend_duration_or_trigger();
  }
};

struct berserk_cat_t final : public berserk_cat_base_t
{
  DRUID_ABILITY( berserk_cat_t, berserk_cat_base_t, "berserk_cat",
                 p->talent.incarnation_cat.ok() ? spell_data_t::not_found() : p->spec.berserk_cat )
  {
    name_str_reporting = "berserk";
  }
};

struct incarnation_cat_t final : public berserk_cat_base_t
{
  DRUID_ABILITY( incarnation_cat_t, berserk_cat_base_t, "incarnation_avatar_of_ashamane", p->talent.incarnation_cat )
  {
    buff = p->buff.incarnation_cat;
  }

  void execute() override
  {
    berserk_cat_base_t::execute();

    p()->buff.incarnation_cat_prowl->trigger();
  }
};

// Bloodseeker Vines ========================================================
struct bloodseeker_vines_t final : public cat_attack_t
{
  timespan_t orig_dur;
  double twin_pct;
  double rampancy_pct;
  int dot_stacks = 1;

  bloodseeker_vines_t( druid_t* p, std::string_view n )
    : cat_attack_t( n, p, p->spec.bloodseeker_vines ),
      twin_pct( p->talent.twin_sprouts->effectN( 1 ).percent() ),
      rampancy_pct( p->talent.rampancy->effectN( 1 ).percent() )
  {
    proc = true;
    dot_max_stack = 1;

    dot_name = "bloodseeker_vines";
    orig_dur = dot_duration;
  }

  timespan_t calculate_dot_refresh_duration( const dot_t*, timespan_t dur ) const override
  {
    return dur;
  }

  void trigger_dot( action_state_t* s ) override
  {
    cat_attack_t::trigger_dot( s );

    int stacks = dot_stacks;

    // add 1ms to ensure final tick is buffed
    if ( rng().roll( twin_pct ) && orig_dur == dot_duration )
    {
      if ( target_list().size() > 1 )
      {
        if ( auto tar = p()->get_smart_target( target_list(), &druid_td_t::dots_t::bloodseeker_vines, s->target ) )
        {
          auto state_ = get_state( s );
          state_->target = tar;
          cat_attack_t::trigger_dot( state_ );
          td( tar )->debuff.bloodseeker_vines->trigger( dot_duration + 1_ms );
          action_state_t::release( state_ );
        }
        else
        {
          stacks = 2;
        }
      }
      else
      {
        stacks = 2;
      }
    }

    td( s->target )->debuff.bloodseeker_vines->trigger( stacks, dot_duration + 1_ms );
  }

  void tick( dot_t* d ) override
  {
    cat_attack_t::tick( d );

    if ( rng().roll( rampancy_pct ) )
      p()->active.rampancy->execute_on_target( d->target );
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    auto tm = cat_attack_t::composite_target_multiplier( t );

    if ( auto _td = find_td( t ) )
      tm *= _td->debuff.bloodseeker_vines->check();

    return tm;
  }
};

// Bursting Growth ==========================================================
struct bursting_growth_t final : public cat_attack_t
{
  bursting_growth_t( druid_t* p, std::string_view n ) : cat_attack_t( n, p, p->find_spell( 440122 ) )
  {
    background = proc = true;
    aoe = -1;
    reduced_aoe_targets = 5;  // TODO: not in data, from tooltip
  }
};

// Chomp ====================================================================
struct chomp_t final : public cat_attack_t
{
  DRUID_ABILITY( chomp_t, cat_attack_t, "chomp", p->talent.chomp ) {}

  bool ready() override
  {
    return p()->buff.chomp_enabler->check() && cat_attack_t::ready();
  }
};

// Feral/Frantic Frenzy ===========================================================

struct frantic_frenzy_t final : public trigger_aggravate_wounds_t<DRUID_FERAL, cat_attack_t>
{
  struct frantic_frenzy_tick_t final : public cat_attack_t
  {
    frantic_frenzy_tick_t( druid_t* p, std::string_view n, flag_e f ) : cat_attack_t( n, p, p->find_spell( 1244079 ), f )
    {
      background = dual = proc = true;
      aoe = -1;
      ignores_armor = false;

      dot_name = "frantic_frenzy_tick";
    }

    result_amount_type report_amount_type( const action_state_t* s ) const override
    {
      return s->result_type;  // override since tick_action defaults to DMG_OVER_TIME
    }

    timespan_t travel_time() const override
    {
      return rng().range( FERAL_FLICKER_DELAY );
    }
  };

  DRUID_ABILITY( frantic_frenzy_t, base_t, "frantic_frenzy", p->talent.frantic_frenzy )
  {
    if ( data().ok() )
    {
      tick_action = p->get_secondary_action<frantic_frenzy_tick_t>( name_str + "_tick", f );
      replace_stats( this, tick_action, false );

      dynamic_tick_action = TICK_ACTION_SNAPSHOT;

      if ( !is_free() )
        track_cd_waste = true;

      // scripted so must be manually configured
      base_tick_time = 200_ms;  // wild ass guess
      dot_duration = base_tick_time * p->talent.frantic_frenzy->effectN( 1 ).base_value();

      const auto& energize_eff = find_effect( p->find_spell( 1278969 ), E_ENERGIZE );
      energize_type = action_energize::PER_TICK;
      energize_resource = energize_eff.resource_gain_type();
      energize_amount = energize_eff.resource( energize_resource );
    }
  }

  void init() override
  {
    base_t::init();

    if ( tick_action )
      tick_action->direct_tick = false;
  }
};

struct feral_frenzy_t final : public trigger_aggravate_wounds_t<DRUID_FERAL, cat_attack_t>
{
  // despite generating CP, does not actually proc cp generated related effects
  struct feral_frenzy_tick_t final : public cat_attack_t
  {
    feral_frenzy_tick_t( druid_t* p, std::string_view n, flag_e f ) : cat_attack_t( n, p, p->find_spell( 274838 ), f )
    {
      background = dual = proc = true;

      dot_name = "feral_frenzy_tick";
    }

    result_amount_type report_amount_type( const action_state_t* s ) const override
    {
      return s->result_type;  // override since tick_action defaults to DMG_OVER_TIME
    }
  };

  DRUID_ABILITY( feral_frenzy_t, base_t, "feral_frenzy", p->talent.feral_frenzy )
  {
    if ( data().ok() )
    {
      tick_action = p->get_secondary_action<feral_frenzy_tick_t>( name_str + "_tick", f );
      replace_stats( this, tick_action, false );

      dynamic_tick_action = TICK_ACTION_SNAPSHOT;

      if ( !is_free() )
        track_cd_waste = true;
    }
  }

  void init() override
  {
    base_t::init();

    if ( tick_action )
    {
      tick_action->gain = gain;
      tick_action->direct_tick = false;
    }
  }

  bool ready() override
  {
    return p()->talent.frantic_frenzy.ok() ? false : base_t::ready();
  }
};

// Ferocious Bite ===========================================================
struct ferocious_bite_base_t : public cp_spender_t
{
  struct rampant_ferocity_t final : public cat_attack_t
  {
    double energy_mod_pct;

    rampant_ferocity_t( druid_t* p, std::string_view n )
      : cat_attack_t( n, p, p->find_spell( 391710 ) ),
        energy_mod_pct( p->talent.rampant_ferocity->effectN( 2 ).percent() )
    {
      background = proc = true;
      aoe = -1;
      reduced_aoe_targets = p->talent.rampant_ferocity->effectN( 1 ).base_value();
      name_str_reporting = "rampant_ferocity";

      target_filter_callback = secondary_targets_only();
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      auto _state = cast_state( s );
      auto energy_mul = 1.0;
      if ( !_state->free )  // free casts don't get any bonus for excess energy
        energy_mul += _state->energy_mod * energy_mod_pct;

      return cat_attack_t::composite_da_multiplier( s ) * _state->combo_points * energy_mul;
    }
  };

  rampant_ferocity_t* rampant_ferocity = nullptr;
  action_t* apex_proxy = nullptr;
  stats_t* apex_stats = nullptr;
  stats_t* orig_stats;
  double excess_energy = 0.0;
  double max_excess_energy;
  double saber_jaws_mul;
  double unseen_chance;
  double spattered_mul;
  size_t spattered_cap;
  bool max_energy = false;

  ferocious_bite_base_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f )
    : cp_spender_t( n, p, s, f ),
      orig_stats( stats ),
      max_excess_energy( find_effect( this, E_POWER_BURN ).resource() ),
      saber_jaws_mul( p->talent.saber_jaws->effectN( 1 ).percent() ),
      unseen_chance( p->talent.unseen_predator_1->effectN( 1 ).percent() ),
      spattered_mul( p->talent.blood_spattered->effectN( 1 ).percent() ),
      spattered_cap( as<size_t>( p->talent.blood_spattered->effectN( 2 ).base_value() ) )
  {
    add_option( opt_bool( "max_energy", max_energy ) );

    if ( p->talent.apex_predators_craving.ok() )
    {
      auto apex_name = name_str + "_apex";

      apex_proxy = p->get_secondary_action<action_t>( apex_name, action_e::ACTION_OTHER, apex_name, p, &data() );
      apex_proxy->name_str_reporting = "Apex";

      apex_stats = p->get_stats( apex_name, apex_proxy );
      stats->add_child( apex_stats );
    }

    if ( p->talent.rampant_ferocity.ok() )
    {
      rampant_ferocity = p->get_secondary_action<rampant_ferocity_t>( "rampant_ferocity_" + name_str );
      add_child( rampant_ferocity );
    }
  }

  bool _is_free() const
  {
    return p()->buff.apex_predators_craving->check() || is_free();
  }

  void init_finished() override
  {
    cp_spender_t::init_finished();

    if ( max_energy )
      p()->resource_thresholds.push_back( maximum_energy() );
  }

  double maximum_energy() const
  {
    double req = base_costs[ RESOURCE_ENERGY ] + max_excess_energy;

    req *= 1.0 + p()->buff.incarnation_cat->check_value();

    if ( p()->buff.apex_predators_craving->check() )
      req *= 1.0 + p()->buff.apex_predators_craving->data().effectN( 1 ).percent();

    return req;
  }

  bool ready() override
  {
    if ( max_energy && p()->resources.current[ RESOURCE_ENERGY ] < maximum_energy() )
      return false;

    return cp_spender_t::ready();
  }

  double get_excess_energy() const
  {
    // Incarn does affect the additional energy consumption.
    double _max_used = max_excess_energy * ( 1.0 + p()->buff.incarnation_cat->check_value() );

    return std::min( _max_used, ( p()->resources.current[ RESOURCE_ENERGY ] - cost() ) );
  }

  void execute() override
  {
    excess_energy = get_excess_energy();

    if ( !is_free() && p()->buff.apex_predators_craving->check() )
    {
      stats = apex_stats;
      cp_spender_t::execute();
      stats = orig_stats;

      p()->buff.apex_predators_craving->consume( this );
    }
    else
    {
      cp_spender_t::execute();
    }
  }

  void impact( action_state_t* s ) override
  {
    cp_spender_t::impact( s );

    if ( s->chain_target || !result_is_hit( s->result ) )  // the rest only procs on main target
      return;

    if ( p()->talent.sabertooth.ok() )
      td( s->target )->debuff.sabertooth->trigger( this, cast_state( s )->combo_points );

    if ( p()->active.bursting_growth && td( s->target )->dots.bloodseeker_vines->is_ticking() )
      p()->active.bursting_growth->execute_on_target( s->target );

    if ( rampant_ferocity && s->result_amount > 0 )
    {
      rampant_ferocity->set_target( s->target );

      if ( !rampant_ferocity->target_list().empty() )
      {
        snapshot_and_execute( rampant_ferocity, s, false, [ this ]( auto from, auto to ) {
          cast_state( to )->copy_data( cast_state( from ) );
        } );
      }
    }

    if ( unseen_chance )
    {
      auto _state = cast_state( s );
      auto _cp = _state->combo_points;

      if ( rng().roll( unseen_chance * _cp ) )
        trigger_unseen_attack( _cp, _state->energy_mod );
    }
  }

  void record_data( action_state_t* s ) override
  {
    if ( p()->buff.apex_predators_craving->check() )
    {
      stats = apex_stats;
      cp_spender_t::record_data( s );
      stats = orig_stats;
    }
    else
    {
      cp_spender_t::record_data( s );
    }
  }

  void consume_resource() override
  {
    // Extra energy consumption happens first. In-game it happens before the skill even casts but let's not do that
    // because its dumb.
    if ( hit_any_target && !is_free() && !p()->buff.apex_predators_craving->check() )
    {
      p()->resource_loss( RESOURCE_ENERGY, excess_energy );
      stats->consume_resource( RESOURCE_ENERGY, excess_energy );
    }

    cp_spender_t::consume_resource();
  }

  int _combo_points() const override
  {
    if ( _is_free() )
      return as<int>( p()->resources.max[ RESOURCE_COMBO_POINT ] );
    else
      return cp_spender_t::_combo_points();
  }

  int _consumed_combo_points() const override
  {
    if ( _is_free() )
      return 0;
    else
      return cp_spender_t::_consumed_combo_points();
  }

  virtual double energy_modifier( const action_state_t* ) const
  {
    if ( _is_free() )
      return 1.0;
    else
      return excess_energy / max_excess_energy;
  }

  void snapshot_state( action_state_t* s, result_amount_type rt ) override
  {
    if ( _is_free() )
      cast_state( s )->free = true;

    cast_state( s )->energy_mod = energy_modifier( s );

    cp_spender_t::snapshot_state( s, rt );
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    auto da = cp_spender_t::composite_da_multiplier( s );
    auto energy_mul = 1.0 + cast_state( s )->energy_mod;

    if ( saber_jaws_mul && !cast_state( s )->free )
      energy_mul += cast_state( s )->energy_mod * saber_jaws_mul;

    // base spell coeff is for 5CP, so we reduce if lower than 5.
    da *= cast_state( s )->combo_points / p()->resources.max[ RESOURCE_COMBO_POINT ];
    da *= energy_mul;

    if ( spattered_mul && s->chain_target == 0 )
      da *= 1.0 + spattered_mul * std::min( spattered_cap, p()->dot_lists.rip.size() );

    return da;
  }
};

struct ferocious_bite_t final : public ferocious_bite_base_t
{
  struct ravage_ferocious_bite_t final : public ravage_base_t<ferocious_bite_base_t, use_dot_list_t<cat_attack_t>>
  {
    ravage_ferocious_bite_t( druid_t* p, std::string_view n, flag_e f ) : base_t( n, p, p->find_spell( 441591 ), f ) {}

    double energy_modifier( const action_state_t* s ) const override
    {
      return s->chain_target == 0 ? base_t::energy_modifier( s ) : 0.0;
    }
  };

  ravage_ferocious_bite_t* ravage = nullptr;

  DRUID_ABILITY( ferocious_bite_t, ferocious_bite_base_t, "ferocious_bite", p->find_class_spell( "Ferocious Bite" ) )
  {
    if ( !p->buff.ravage_fb->is_fallback )
    {
      ravage = p->get_secondary_action<ravage_ferocious_bite_t, false>( "ravage_" + name_str, f );
      add_child( ravage );
    }
  }

  void init() override
  {
    ferocious_bite_base_t::init();

    if ( ravage )
      ravage->max_energy = max_energy;
  }

  void execute() override
  {
    if ( ravage && p()->buff.ravage_fb->check() )
    {
      ravage->execute_on_target( target );
      p()->buff.ravage_fb->consume( this );

      return;
    }

    ferocious_bite_base_t::execute();
  }
};

// Lunar Inspiration ========================================================
struct lunar_inspiration_t final : public trigger_panthers_guile_t<cp_generator_t>
{
  DRUID_ABILITY( lunar_inspiration_t, base_t, "lunar_inspiration",
                 p->talent.lunar_inspiration.ok() ? p->find_spell( 155625 ) : spell_data_t::not_found() )
  {
    may_dodge = may_parry = may_block = false;

    s_data_reporting = p->talent.lunar_inspiration;
    dot_name = "lunar_inspiration";
  }

  void trigger_dot( action_state_t* s ) override
  {
    // If existing moonfire duration is longer, lunar inspiration dot is not applied
    if ( td( s->target )->dots.moonfire->remains() > composite_dot_duration( s ) )
      return;

    base_t::trigger_dot( s );
  }

  double composite_haste() const override
  {
    // directly call action_t::composite_haste(), as there are no intervening overrides
    return action_t::composite_haste() * p()->cache.spell_cast_speed();
  }

  double composite_crit_chance() const override
  {
    // duplicate parse_action_effects_t::composite_crit_chance() using spell_t as base
    auto cc = action_t::composite_crit_chance() + p()->cache.spell_crit_chance();

    for ( const auto& i : crit_chance_effects )
      cc += get_effect_value( i );

    return cc;
  }

  double composite_crit_chance_multiplier() const override
  {
    // duplicate parse_action_effects_t::composite_crit_chance_multiplier() using spell_t as base
    auto ccm = action_t::composite_crit_chance_multiplier() * p()->composite_spell_crit_chance_multiplier();

    for ( const auto& i : crit_chance_multiplier_effects )
      ccm *= 1.0 + get_effect_value( i );

    return ccm;
  }
};

// Maim =====================================================================
struct maim_t final : public cp_spender_t
{
  DRUID_ABILITY( maim_t, cp_spender_t, "maim", p->talent.maim ) {}

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    return cp_spender_t::composite_da_multiplier( s ) * cast_state( s )->combo_points;
  }
};

// Rake =====================================================================
struct rake_t final : public use_fluid_form_t<CAT_FORM, trigger_call_of_the_elder_druid_t<cp_generator_t>>
{
  struct rake_bleed_t final : public trigger_thriving_growth_t<use_dot_list_t<cat_attack_t>>
  {
    rake_bleed_t( druid_t* p, std::string_view n, flag_e f, rake_t* r ) : base_t( n, p, find_trigger( r ).trigger(), f )
    {
      background = dual = proc = true;
      // override for convoke. since this is only ever executed from rake_t, form checking is unnecessary.
      form_mask = 0;

      dot_name = "rake";
      dot_list = &p->dot_lists.rake;

      if ( p->talent.pouncing_strikes.ok() || p->spec.improved_prowl->ok() )
      {
        const auto& eff = r->data().effectN( 4 );
        if ( !has_parse_entry( persistent_multiplier_effects, &eff ) )
        {
          add_parse_entry( persistent_multiplier_effects )
            .set_value( eff.percent() )
            .set_func( [ this ] { return stealthed(); } )
            .set_eff( &eff )
            .print_debug( this );
        }
      }
    }
  };

  rake_bleed_t* bleed;
  bool crit_main_target = false;

  DRUID_ABILITY( rake_t, base_t, "rake", p->talent.rake )
  {
    aoe = std::max( aoe, 1 ) + as<int>( p->talent.doubleclawed_rake->effectN( 1 ).base_value() );

    if ( data().ok() )
    {
      bleed = p->get_secondary_action<rake_bleed_t>( name_str + "_bleed", f, this );
      replace_stats( this, bleed );

      if ( p->talent.pouncing_strikes.ok() || p->spec.improved_prowl->ok() )
      {
        const auto& eff = data().effectN( 4 );
        if ( !has_parse_entry( persistent_multiplier_effects, &eff ) )
        {
          add_parse_entry( persistent_multiplier_effects )
            .set_value( eff.percent() )
            .set_func( [ this ] { return stealthed(); } )
            .set_eff( &eff )
            .print_debug( this );
        }
      }
    }

    dot_name = "rake";
  }

  bool has_amount_result() const override { return bleed->has_amount_result(); }

  std::vector<player_t*>& target_list() const override
  {
    if ( !target_cache.is_valid )
      bleed->target_cache.is_valid = false;

    auto& tl = base_t::target_list();

    // When Double-Clawed Rake is active, this is an AoE action meaning it will impact onto the first 2 targets in the
    // target list. Instead, we want it to impact on the target of the action and 1 additional, so we'll override the
    // target_list to make it so.
    if ( is_aoe() && as<int>( tl.size() ) > aoe )
    {
      // always hit the target, so if it exists make sure it's first
      auto start_it = tl.begin() + ( tl[ 0 ] == target ? 1 : 0 );

      // randomize remaining targets
      rng().shuffle( start_it, tl.end() );

      // sort by remaining duration
      std::sort( start_it, tl.end(), [ this ]( player_t* a, player_t* b ) {
        auto a_td = get_td( a );
        auto b_td = get_td( b );

        return ( a_td ? a_td->dots.rake->remains() : 0_ms ) < ( b_td ? b_td->dots.rake->remains() : 0_ms );
      } );
    }

    return tl;
  }

  void trigger_primal_fury() override
  {
    // bugged so only procs when the main target is crit
    if ( crit_main_target && pf_cp )
    {
      // scheduled to happen after action as the energize itself is a separate action
      make_event( *sim, [ this ] { p()->resource_gain( RESOURCE_COMBO_POINT, pf_cp, pf_gain ); } );
    }
  }

  void execute() override
  {
    // Force invalidate target cache so that it will impact on the correct targets.
    target_cache.is_valid = false;
    crit_main_target = false;

    base_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    snapshot_and_execute( bleed, s, true );

    if ( s->chain_target == 0 && s->result == RESULT_CRIT )
      crit_main_target = true;
  }
};

// Rip ======================================================================
struct rip_t final : public trigger_thriving_growth_t<use_dot_list_t<cp_spender_t>>
{
  struct tear_t final : public druid_residual_action_t<cat_attack_t, true>
  {
    tear_t( druid_t* p, std::string_view n, flag_e f ) : base_t( n, p, p->find_spell( 391356 ), f )
    {
      name_str_reporting = "tear";

      residual_mul = p->talent.rip_and_tear->effectN( 1 ).percent() * base_tick_time / dot_duration;
    }
  };

  tear_t* tear = nullptr;
  double apex_pct;
  double apex_exp;

  DRUID_ABILITY( rip_t, base_t, "rip", p->talent.rip ),
    apex_pct( p->talent.apex_predators_craving->effectN( 1 ).percent() * 0.1 ),
    apex_exp( p->talent.apex_predators_craving->effectN( 2 ).percent() )
  {
    dot_name = "rip";
    dot_list = &p->dot_lists.rip;

    if ( p->talent.rip_and_tear.ok() )
    {
      auto suf = get_suffix( name_str, "rip" );
      tear = p->get_secondary_action<tear_t>( "tear" + suf, f );
    }

    if ( !p->buff.feline_potential->is_fallback )
    {
      const auto& eff = p->buff.feline_potential->data().effectN( 1 );
      if ( !has_parse_entry( persistent_multiplier_effects, &eff ) )
      {
        add_parse_entry( persistent_multiplier_effects )
          .set_buff( p->buff.feline_potential )
          .set_value( eff.percent() )
          .set_eff( &eff )
          .add_parse_callback( this, PARSE_CALLBACK_POST_EXECUTE,
            [ this, b = p->buff.feline_potential ]( action_state_t* ) { b->consume( this ); } )
          .print_debug( this );
       }
    }
  }

  void init_finished() override
  {
    base_t::init_finished();

    if ( tear )
      add_child( tear );
  }

  double dot_duration_pct_multiplier( const action_state_t* s ) const override
  {
    return base_t::dot_duration_pct_multiplier( s ) * ( 1.0 + cast_state( s )->combo_points );
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    // hard-cast rip is scripted to consume implant
    if ( !background && p()->active.bloodseeker_vines_implant && p()->buff.implant->check() )
    {
      p()->active.bloodseeker_vines_implant->execute_on_target( s->target );
      p()->buff.implant->expire();
    }

    if ( tear )
    {
      // target debuffs are not account for in total rip damage calculation. state is released after impact() so we can
      // safely modify the state here.
      s->target_da_multiplier = s->target_ta_multiplier = 1.0;

      auto tick_amount = calculate_tick_amount( s, 1.0 );
      auto dot_total = tick_amount * find_dot( s->target )->ticks_left_fractional();

      tear->base_td = dot_total;
      tear->execute_on_target( s->target );
    }
  }

  void tick( dot_t* d ) override
  {
    base_t::tick( d );

    auto c = apex_pct / std::pow( as<double>( p()->dot_lists.rip.size() ), apex_exp );

    if ( rng().roll( c ) )
      p()->buff.apex_predators_craving->trigger();
  }
};

// Primal Wrath =============================================================
// NOTE: must be defined AFTER rip_T
struct primal_wrath_t final : public cp_spender_t
{
  rip_t* rip;

  DRUID_ABILITY( primal_wrath_t, cp_spender_t, "primal_wrath", p->talent.primal_wrath )
  {
    aoe = -1;

    dot_name = "rip";

    if ( data().ok() )
    {
      rip = p->get_secondary_action<rip_t>( "rip_primal", p->find_spell( 1079 ), f );
      rip->dot_duration = timespan_t::from_seconds( data().effectN( 2 ).base_value() );
      rip->dual = rip->background = rip->proc = true;
      replace_stats( this, rip );
      rip->base_costs[ RESOURCE_ENERGY ] = 0;
    }
  }

  std::vector<player_t*>& target_list() const override
  {
    // target order is randomized, can be important for rip application ordering
    auto& tl = cp_spender_t::target_list();

    rng().shuffle( tl.begin(), tl.end() );

    return tl;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    return cp_spender_t::composite_da_multiplier( s ) * ( 1.0 + cast_state( s )->combo_points );
  }

  void impact( action_state_t* s ) override
  {
    snapshot_and_execute( rip, s, true );

    cp_spender_t::impact( s );
  }
};

// Shred ====================================================================
struct shred_t final : public trigger_panthers_guile_t<
                              use_fluid_form_t<CAT_FORM,
                              trigger_claw_rampage_t<DRUID_FERAL,
                              trigger_wildpower_surge_t<DRUID_FERAL,
                              trigger_call_of_the_elder_druid_t<
                              cp_generator_t>>>>>
{
  double stealth_mul = 0.0;

  DRUID_ABILITY( shred_t, base_t, "shred", p->find_class_spell( "Shred" ) )
  {
    // TODO adjust if it becomes possible to take both
    bleed_mul = p->talent.merciless_claws->effectN( 2 ).percent();

    if ( p->talent.pouncing_strikes.ok() )
    {
      stealth_mul = data().effectN( 3 ).percent();

      if ( !has_parse_entry( da_multiplier_effects, &data().effectN( 3 ) ) )
      {
        add_parse_entry( da_multiplier_effects )
          .set_value( stealth_mul )
          .set_func( [ this ] { return stealthed(); } )
          .set_eff( &data().effectN( 3 ) )
          .print_debug( this );
      }

      if ( const auto& eff = p->find_spell( 343232 )->effectN( 1 ); energize && !energize->modified_by( eff ) )
      {
        energize->add_parse_entry()
          .set_value( eff.base_value() )
          .set_func( []( const action_t* a, const action_state_t* ) {
            return a ? debug_cast<const shred_t*>( a )->stealthed() : false;
          } )
          .set_flat( true )
          .set_eff( &eff );
      }
    }
  }

  double composite_crit_chance_multiplier() const override
  {
    double cm = base_t::composite_crit_chance_multiplier();

    if ( stealth_mul && stealthed() )
      cm *= 2.0;

    return cm;
  }
};

// Swipe (Cat) ====================================================================
struct swipe_cat_t final : public trigger_claw_rampage_t<DRUID_FERAL,
                                  trigger_wildpower_surge_t<DRUID_FERAL,
                                  cp_generator_t>>
{
  DRUID_ABILITY( swipe_cat_t, base_t, "swipe_cat", p->apply_override( p->talent.swipe, p->spec.cat_form_override ) )
  {
    aoe = -1;
    reduced_aoe_targets = data().effectN( 4 ).base_value();

    if ( p->talent.merciless_claws.ok() )
      bleed_mul = p->talent.merciless_claws->effectN( 1 ).percent();

    if ( p->specialization() == DRUID_FERAL )
      name_str_reporting = "swipe";
    else
      name_str_reporting = "Cat";
  }
};

// Tiger's Fury =============================================================
struct tigers_fury_t final : public cat_attack_t
{
  DRUID_ABILITY( tigers_fury_t, cat_attack_t, "tigers_fury", p->talent.tigers_fury )
  {
    harmful = false;
    energize_type = action_energize::ON_CAST;
    track_cd_waste = true;

    form_mask = CAT_FORM;
    autoshift = p->active.shift_to_cat;
  }

  void execute() override
  {
    cat_attack_t::execute();

    p()->buff.tigers_fury->trigger();
    p()->buff.tigers_tenacity->trigger();
    p()->buff.strategic_infusion->trigger();
    p()->buff.savage_fury->trigger();

    if ( p()->buff.killing_strikes_combat->check() )
    {
      p()->buff.killing_strikes_combat->expire();
      p()->buff.ravage_fb->trigger();
    }

    p()->buff.stalking_predator->trigger();
  }
};

// Unseen Attacks ===========================================================
struct unseen_attack_t : public cat_attack_t
{
  unseen_attack_t( druid_t* p, std::string_view n, const spell_data_t* s, flag_e f = flag_e::NONE )
    : cat_attack_t( n, p, s, f )
  {
    proc = background = true;

    range = p->talent.unseen_predator_1->effectN( 2 ).base_value();
  }

  bool parse_tigers_fury() override
  {
    // unseen attacks do not snapshot tiger's fury
    parse_effects( p()->buff.tigers_fury );
    return false;
  }


  timespan_t travel_time() const override
  {
    return rng().range( FERAL_FLICKER_DELAY );
  }

  void impact( action_state_t* s ) override
  {
    cat_attack_t::impact( s );

    // Unseen Predator 2 has a 0.1s ICD so it can only proc once per execute, we can instead just have it proc only on
    // the first target hit.
    if ( s->chain_target == 0 )
    {
      auto dur = p()->buff.unseen_predators_craving->buff_duration() * cast_state( s )->combo_points;

      p()->buff.unseen_predators_craving->trigger( dur );
    }
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    auto da = cat_attack_t::composite_da_multiplier( s );
    auto state = cast_state( s );

    da *= state->combo_points / p()->resources.max[ RESOURCE_COMBO_POINT ];
    da *= state->energy_mod;

    return da;
  }
};

struct unseen_slash_t final : public unseen_attack_t
{
  struct unseen_slash_bleed_t final : public cat_attack_t
  {
    unseen_slash_bleed_t( druid_t* p ) : cat_attack_t( "unseen_slash_bleed", p, p->find_spell( 1271863 ) )
    {
      background = dual = proc = true;
    }

    bool parse_tigers_fury() override
    {
      // unseen attacks do not snapshot tiger's fury
      parse_effects( p()->buff.tigers_fury );
      return false;
    }

    double composite_rolling_ta_multiplier( const action_state_t* s ) const override
    {
      auto ta = cat_attack_t::composite_rolling_ta_multiplier( s );
      auto state = cast_state( s );

      // composite rolling ta uses a value of 1.0 to represent the base dot damage. since not having full cp & energy
      // reduces the base dot amount, we subtract penalty from the composite rolling ta.
      ta += ( state->combo_points / p()->resources.max[ RESOURCE_COMBO_POINT ] * state->energy_mod ) - 1.0;

      return ta;
    }
  };

  unseen_slash_bleed_t* bleed;

  unseen_slash_t( druid_t* p ) : unseen_attack_t( p, "unseen_slash", p->find_spell( 1263890 ) )
  {
    bleed = p->get_secondary_action<unseen_slash_bleed_t>( "unseen_slash_bleed" );
    replace_stats( this, bleed );
  }

  void impact( action_state_t* s ) override
  {
    unseen_attack_t::impact( s );

    snapshot_and_execute( bleed, s, true, [ this ]( auto from, auto to ) {
      cast_state( to )->copy_data( cast_state( from ) );
    } );
  }
};

struct unseen_swipe_t final : public unseen_attack_t
{
  unseen_swipe_t( druid_t* p ) : unseen_attack_t( p, "unseen_swipe", p->find_spell( 1263908 ) )
  {
    aoe = -1;
    reduced_aoe_targets = UNSEEN_SWIPE_REDUCED_AOE;
  }
};
}  // end namespace cat_attacks

namespace bear_attacks
{
template <typename BASE>
struct rage_spender_t : public BASE
{
private:
  druid_t* p_;
  buff_t* atw_buff;
  double moy_hp_pct_per_rage = 0.0;
  double ug_cdr;
  double lw_rage_bucket = 0.0;

protected:
  using base_t = rage_spender_t<BASE>;

public:
  rage_spender_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f )
    : BASE( n, p, s, f ),
      p_( p ),
      atw_buff( p->buff.after_the_wildfire ),
      ug_cdr( p->talent.ursocs_guidance->effectN( 5 ).base_value() )
  {
    if ( p->talent.memory_of_ysera.ok() )
    {
      moy_hp_pct_per_rage =
        p->talent.memory_of_ysera->effectN( 1 ).percent() * 0.01 / p->talent.memory_of_ysera->effectN( 2 ).base_value();
    }

    if ( p->talent.galactic_guardian.ok() && p->talent.red_moon.ok() )
      lw_rage_bucket = p->talent.galactic_guardian->effectN( 3 ).base_value();
  }

  void impact( action_state_t* s ) override
  {
    BASE::impact( s );

    if ( !BASE::proc && BASE::td( s->target )->dots.red_moon->is_ticking() )
    {
      if ( lw_rage_bucket )
      {
        auto _stacks = static_cast<int>( BASE::last_resource_cost / lw_rage_bucket );
        if ( p_->buff.lunar_wrath->consume( this, _stacks ) )
        {
          for ( int i = 0; i < _stacks; ++i )
          {
            if ( p_->bugs )  // only one proc for every 2 stacks
            {
              if ( i % 2 == 0 )
              {
                p_->active.lunar_wrath_heal->execute();
                p_->active.lunar_wrath->execute_on_target( s->target );
              }
            }
            else
            {
              p_->active.lunar_wrath_heal->execute();
              p_->active.lunar_wrath->execute_on_target( s->target );
            }
          }
        }
      }
      else
      {
        if ( p_->buff.lunar_wrath->consume( this ) )
        {
          p_->active.lunar_wrath_heal->execute();
          p_->active.lunar_wrath->execute_on_target( s->target );
        }
      }
    }
  }

  void consume_rage_after_the_wildfire( double amount )
  {
    if ( !p_->talent.after_the_wildfire.ok() )
      return;

    if ( !atw_buff->check() )
      atw_buff->trigger();

    atw_buff->current_value -= amount;

    if ( atw_buff->current_value <= 0 )
    {
      atw_buff->current_value += atw_buff->default_value;

      p_->active.after_the_wildfire_heal->execute();
    }
  }

  void consume_rage_memory_of_ysera( double amount )
  {
    if ( !p_->talent.memory_of_ysera.ok() )
      return;

    auto heal_amount = p_->resources.max[ RESOURCE_HEALTH ] * amount * moy_hp_pct_per_rage;

    p_->active.memory_of_ysera_heal->base_dd_max = heal_amount;
    p_->active.memory_of_ysera_heal->base_dd_min = heal_amount;

    p_->active.memory_of_ysera_heal->execute();
  }

  void consume_rage_ursocs_guidance( double amount )
  {
    if ( !p_->talent.ursocs_guidance.ok() )
      return;

    auto dur = timespan_t::from_seconds( amount / -ug_cdr );

    if ( p_->cooldown.incarnation_bear )
      p_->cooldown.incarnation_bear->adjust( dur );

    if ( p_->cooldown.berserk_bear )
      p_->cooldown.berserk_bear->adjust( dur );
  }

  void consume_resource() override
  {
    BASE::consume_resource();

    if ( BASE::is_free() || !BASE::last_resource_cost )
      return;

    consume_rage_after_the_wildfire( BASE::last_resource_cost );
    consume_rage_memory_of_ysera( BASE::last_resource_cost );
    consume_rage_ursocs_guidance( BASE::last_resource_cost );
  }
};

template <typename BASE>
struct trigger_ursocs_fury_t : public BASE
{
private:
  druid_t* p_;
  double mul;

protected:
  using base_t = trigger_ursocs_fury_t<BASE>;

public:
  trigger_ursocs_fury_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f )
    : BASE( n, p, s, f ),
      p_( p ),
      mul( p->talent.ursocs_fury->effectN( 1 ).percent() *
           ( 1.0 + p->talent.nurturing_instinct->effectN( 1 ).percent() ) *
           ( 1.0 + p->talent.natural_recovery->effectN( 1 ).percent() ) )
  {}

  void trigger_ursocs_fury( const action_state_t* s )
  {
    if ( !s->result_amount )
      return;

    p_->buff.ursocs_fury->trigger( 1, s->result_amount * ( 1.0 + mul ) );
    p_->buff.ursocs_fury->current_value =
      std::min( p_->buff.ursocs_fury->current_value, GUARDIAN_SHIELD_CAP_HP_PCT * p_->max_health() );
  }

  void impact( action_state_t* s ) override
  {
    BASE::impact( s );

    trigger_ursocs_fury( s );
  }

  void tick( dot_t* d ) override
  {
    BASE::tick( d );

    trigger_ursocs_fury( d->state );
  }
};

// Berserk (Bear) ===========================================================
struct berserk_bear_base_t : public bear_attack_t
{
  buff_t* buff = nullptr;

  berserk_bear_base_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f ) : bear_attack_t( n, p, s, f )
  {
    harmful   = false;
    form_mask = BEAR_FORM;
    autoshift = p->active.shift_to_bear;
  }

  void execute() override
  {
    bear_attack_t::execute();

    assert( buff );
    buff->trigger();

    if ( p()->cooldown.growl )
      p()->cooldown.growl->reset( true );

    if ( p()->cooldown.mangle )
      p()->cooldown.mangle->reset( true );

    if ( p()->cooldown.thrash )
      p()->cooldown.thrash->reset( true );

    p()->buff.wild_guardian->trigger();
  }
};

struct berserk_bear_t final : public berserk_bear_base_t
{
  DRUID_ABILITY( berserk_bear_t, berserk_bear_base_t, "berserk_bear",
                 p->talent.incarnation_bear.ok() ? spell_data_t::not_found() : p->talent.berserk_bear )
  {
    name_str_reporting = "berserk";
    buff = p->buff.berserk_bear;

    set_tracked_cooldown( p->cooldown.berserk_bear );
  }
};

struct incarnation_bear_t final : public berserk_bear_base_t
{
  DRUID_ABILITY( incarnation_bear_t, berserk_bear_base_t, "incarnation_guardian_of_ursoc", p->talent.incarnation_bear )
  {
    buff = p->buff.incarnation_bear;

    set_tracked_cooldown( p->cooldown.incarnation_bear );
  }
};

// Brambles Reflect =========================================================
struct brambles_reflect_t final : public druid_residual_action_t<bear_attack_t>
{
  brambles_reflect_t( druid_t* p ) : base_t( "brambles", p, p->find_spell( 203958 ) )
  {
    background = proc = true;
  }
};

// Bristling Fur Spell ======================================================
struct bristling_fur_t final : public bear_attack_t
{
  DRUID_ABILITY( bristling_fur_t, bear_attack_t, "bristling_fur", p->talent.bristling_fur )
  {
    harmful = false;
  }

  void execute() override
  {
    bear_attack_t::execute();

    p()->buff.bristling_fur->trigger();
  }
};

// Growl ====================================================================
struct growl_t final : public bear_attack_t
{
  DRUID_ABILITY( growl_t, bear_attack_t, "growl", p->find_class_spell( "Growl" ) )
  {
    ignore_false_positive = use_off_gcd = true;

    set_tracked_cooldown( p->cooldown.growl );
  }

  void impact( action_state_t* s ) override
  {
    if ( s->target->is_enemy() )
      target->taunt( player );

    bear_attack_t::impact( s );
  }
};

// Incapacitating Roar ======================================================
struct incapacitating_roar_t final : public bear_attack_t
{
  DRUID_ABILITY( incapacitating_roar_t, bear_attack_t, "incapacitating_roar", p->talent.incapacitating_roar )
  {
    harmful = false;

    form_mask = BEAR_FORM;
    autoshift = p->active.shift_to_bear;
  }
};

// Ironfur ==================================================================
struct ironfur_t final : public trigger_wild_guardian_echo_base_t<rage_spender_t<druid_spell_t>>
{
  struct echo_of_ironfur_t final : public druid_spell_t
  {
    echo_of_ironfur_t( druid_t* p )
      : druid_spell_t( "echo_of_ironfur", p, p->find_spell( 1269633 ), flag_e::WILDGUARDIAN )
    {
      proc = background = true;
    }

    void execute() override
    {
      druid_spell_t::execute();

      // TODO: determine if duration increased by guardian of elune
      p()->buff.echo_of_ironfur->trigger();
    }
  };

  timespan_t goe_ext;

  DRUID_ABILITY( ironfur_t, base_t, "ironfur", p->talent.ironfur ),
    goe_ext( find_effect( p->buff.guardian_of_elune, A_ADD_FLAT_MODIFIER, P_DURATION ).time_value() )
  {
    use_off_gcd = true;
    harmful = may_miss = may_dodge = may_parry = may_block = false;

    if ( p->talent.wild_guardian_1.ok() )
    {
      echo_action = p->get_secondary_action<echo_of_ironfur_t>( "echo_of_ironfur" );
      echo_buff = p->buff.gift_of_ironfur;
    }
  }

  void execute() override
  {
    base_t::execute();

    auto dur = p()->buff.ironfur->buff_duration();

    if ( p()->buff.guardian_of_elune->check() )
    {
      dur += goe_ext;
      p()->buff.guardian_of_elune->consume( this );
    }

    p()->buff.ironfur->trigger( dur );
  }
};

// Lunar Beam ===============================================================
struct lunar_beam_t final : public druid_spell_t
{
  struct lunar_beam_dot_t final : public trigger_atmospheric_exposure_t<druid_spell_t>
  {
    lunar_beam_dot_t( druid_t* p, const spell_data_t* s, flag_e f ) : base_t( "lunar_beam_dot", p, s, f )
    {
      background = dual = true;
    }
  };

  struct lunar_beam_heal_t final : public druid_heal_t
  {
    lunar_beam_heal_t( druid_t* p, flag_e f ) : druid_heal_t( "lunar_beam_heal", p, p->find_spell( 204069 ), f )
    {
      background = proc = true;
      name_str_reporting = "Heal";

      target = p;
    }
  };

  struct lunar_beam_tick_t final : public trigger_atmospheric_exposure_t<druid_spell_t>
  {
    lunar_beam_tick_t( druid_t* p, flag_e f ) : base_t( "lunar_beam_tick", p, p->find_spell( 414613 ), f )
    {
      background = proc = dual = ground_aoe = true;
      aoe = as<int>( p->talent.lunar_beam->effectN( 3 ).base_value() );

      execute_action = p->get_secondary_action<lunar_beam_heal_t>( "lunar_beam_heal", f );
    }
  };

  ground_aoe_params_t params;
  action_t* damage = nullptr;

  DRUID_ABILITY( lunar_beam_t, druid_spell_t, "lunar_beam", p->talent.lunar_beam )
  {
    if ( data().ok() )
    {
      damage = p->get_secondary_action<lunar_beam_tick_t>( "lunar_beam_tick", f );
      replace_stats( this, damage );

      params.pulse_time( 1_s )
        .duration( p->buff.lunar_beam->buff_duration() )
        .action( damage );

      execute_action = p->get_secondary_action<lunar_beam_dot_t>( "lunar_beam_dot", find_trigger( this ).trigger(), f );
      replace_stats( this, execute_action );
    }

    set_tracked_cooldown( p->cooldown.lunar_beam );
  }

  // needed to allow on-cast procs
  bool has_amount_result() const override { return damage->has_amount_result(); }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.lunar_beam->trigger();

    params.start_time( timespan_t::min() )  // reset start time
      .target( target )
      .x( p()->x_position )
      .y( p()->y_position );

    make_event<ground_aoe_event_t>( *sim, p(), params );
  }
};

// Mangle ===================================================================
struct mangle_t final : public use_fluid_form_t<BEAR_FORM,
                               trigger_claw_rampage_t<DRUID_GUARDIAN,
                               trigger_aggravate_wounds_t<DRUID_GUARDIAN,
                               trigger_wildpower_surge_t<DRUID_GUARDIAN,
                               bear_attack_t>>>>
{
  action_t* strike = nullptr;
  gain_t* red_moon_gain = nullptr;
  timespan_t red_moon_extend = 0_ms;
  double red_moon_rage = 0;
  int inc_targets = 0;

  DRUID_ABILITY( mangle_t, base_t, "mangle", p->find_class_spell( "Mangle" ) )
  {
    track_cd_waste = true;

    auto m_data = p->get_modified_spell( &data() )
      ->parse_effects( p->buff.gore );

    set_energize( m_data );

    if ( p->talent.incarnation_bear.ok() )
    {
      inc_targets =
        as<int>( find_effect( p->talent.incarnation_bear, this, A_ADD_FLAT_MODIFIER, P_CHAIN_TARGETS ).base_value() );
    }

    if ( p->talent.red_moon.ok() )
    {
      red_moon_gain = p->get_gain( "Red Moon" );
      red_moon_extend = p->talent.red_moon->effectN( 2 ).time_value();
      red_moon_rage = find_effect( p->find_spell( 1253582 ), E_ENERGIZE ).resource();
    }

    if ( p->specialization() == DRUID_GUARDIAN && p->talent.strike_for_the_heart.ok() )
    {
      strike = p->get_secondary_action<druid_heal_t>(
        "strike_for_the_heart", p, find_trigger( p->talent.strike_for_the_heart ).trigger() );
      strike->background = strike->proc = true;
    }

    set_tracked_cooldown( p->cooldown.mangle );
  }

  int n_targets() const override
  {
    auto n = base_t::n_targets();

    if ( p()->buff.incarnation_bear->check() )
      n += inc_targets;

    return n;
  }

  void execute() override
  {
    base_t::execute();

    p()->buff.gory_fur->trigger( this );
    p()->buff.guardian_of_elune->trigger( this );

    p()->buff.gore->consume( this );

    if ( p()->specialization() == DRUID_GUARDIAN && p()->buff.killing_strikes_combat->check() )
    {
      p()->buff.killing_strikes_combat->consume( this );
      p()->buff.ravage_maul->trigger();
    }

    if ( strike )
      strike->execute();
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( !proc && red_moon_extend > 0_ms )
    {
      auto _dot = td( s->target )->dots.red_moon;
      if ( _dot->is_ticking() )
      {
        _dot->adjust_duration( red_moon_extend );
        p()->resource_gain( RESOURCE_RAGE, red_moon_rage, red_moon_gain );
      }
    }
  }
};

// Maul / Raze ==============================================================
/*
maul_base_t:trigger_aggravate_wounds:trigger_ursocs_fury:trigger_gore:rage_spender
|
|->maul_ravage_base_t:trigger_celestial_might_repeat_t:trigger_wild_guardian_echo_maul_t
|  |
|  |->maul_t "maul"
|
|->echo_of_maul_t "echo_of_maul"
|
|->celestial_might_maul_t "maul_repeat"
|
|->raze_base_t
|  |
|  |->maul_ravage_base_t:trigger_celestial_might_repeat_t:trigger_wild_guardian_echo_maul_t
|  |  |
|  |  |->raze_t "raze"
|  |
|  |->celestial_might_raze_t "raze_repeat"
|  |
|  |->echo_of_maul_t "echo_of_raze"
|
|->ravage_base_t
   |
   |->ravage_maul_t:trigger_celestial_might_repeat_t:trigger_wild_guardian_echo_maul_t "ravage_maul"
   |
   |->celestial_might_maul_t "ravage_repeat"
   |
   |->echo_of_maul_t "echo_of_ravage"
*/

struct maul_data_t
{
  double rage_mod = 0.0;

  friend void sc_format_to( const maul_data_t& data, fmt::format_context::iterator out )
  {
    fmt::format_to( out, " rage_mod={}", data.rage_mod );
  }
};

template <typename BASE>
struct trigger_wild_guardian_echo_maul_t : public trigger_wild_guardian_echo_base_t<BASE>
{
private:
  using ab = trigger_wild_guardian_echo_base_t<BASE>;

public:
  trigger_wild_guardian_echo_maul_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f )
    : ab( n, p, s, f )
  {
    if ( p->talent.wild_guardian_1.ok() )
      ab::echo_buff = p->buff.gift_of_maul;
  }

  void trigger_echoes() override
  {
    // this maul and echo maul are all derived from maul_base_t so we can use ab::cast_state
    make_event( *BASE::sim, BASE::rng().range( WILD_GUARDIAN_ECHO_DELAY ),
      [ this, rage_mod = BASE::rage_modifier(), t = ab::target ] {
        snapshot_and_execute( ab::echo_action, nullptr, false, [ this, rage_mod, t ]( auto, auto to ) {
          auto state = ab::cast_state( to );
          state->rage_mod = rage_mod;
          state->target = t;
          ab::echo_action->set_target( t );
        } );

        if ( ab::repeat_delay > 0_ms )
        {
          make_repeating_event( *BASE::sim, ab::repeat_delay, [ this, rage_mod, t ] {
            snapshot_and_execute( ab::echo_action, nullptr, false, [ this, rage_mod, t ]( auto, auto to ) {
              auto state = ab::cast_state( to );
              state->rage_mod = rage_mod;
              state->target = t;
              ab::echo_action->set_target( t );
            } );
          }, ab::num_repeat );
        }
      } );
  }
};

template <typename BASE>
struct trigger_celestial_might_repeat_t : public trigger_wild_guardian_echo_maul_t<BASE>
{
private:
  using ab = trigger_wild_guardian_echo_maul_t<BASE>;
  druid_t* p_;

protected:
  using base_t = trigger_celestial_might_repeat_t<BASE>;
  action_t* repeat_action = nullptr;

public:
  trigger_celestial_might_repeat_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f )
    : ab( n, p, s, f ), p_( p )
  {}

  void execute() override
  {
    auto gift_stacks = p_->buff.gift_of_maul->check();

    ab::execute();

    if ( repeat_action && p_->buff.celestial_might->consume( this ) )
    {
      auto delay = ab::rng().range( WILD_GUARDIAN_ECHO_DELAY );

      // if gift was consumed for an echo, the repeat is delayed until after the echo
      if ( gift_stacks - p_->buff.gift_of_maul->check() > 0 )
        delay += ab::rng().range( WILD_GUARDIAN_ECHO_DELAY );

      make_event( *ab::sim, delay, [ this, rage_mod = BASE::rage_modifier(), t = ab::target ] {
        snapshot_and_execute( repeat_action, nullptr, false, [ this, rage_mod, t ]( auto, auto to ) {
          auto state = ab::cast_state( to );
          state->rage_mod = rage_mod;
          state->target = t;
          repeat_action->set_target( t );
        } );
      } );
    }
  }
};

template <typename BASE>
struct echo_of_maul_t : public BASE
{
  echo_of_maul_t( druid_t* p, std::string_view n, const spell_data_t* s ) : BASE( n, p, s, flag_e::WILDGUARDIAN )
  {
    BASE::proc = BASE::background = true;
    BASE::base_multiplier *= p->talent.wild_guardian_1->effectN( 1 ).percent();

    BASE::kb_excess_rage_mul = 0.0;  // echoes snapshot the kb rage multiplier and don't need to re-calculate
  }
};

template <typename BASE>
struct celestial_might_maul_t : public BASE
{
public:
  celestial_might_maul_t( druid_t* p, std::string_view n, const spell_data_t* s ) : BASE( n, p, s, flag_e::CELESTIAL )
  {
    BASE::proc = BASE::background = true;
    BASE::base_multiplier *= find_trigger( p->sets->set( DRUID_GUARDIAN, MID1, B4 ) ).trigger()->effectN( 1 ).percent();
    BASE::name_str_reporting = "Celestial";

    BASE::kb_excess_rage_mul = 0.0;  // repeats snapshot the kb rage multiplier and don't need to re-calculate
  }
};

struct maul_base_t : public trigger_aggravate_wounds_t<DRUID_GUARDIAN,
                            trigger_ursocs_fury_t<
                            trigger_gore_t<
                            rage_spender_t<bear_attack_t>>>>
{
  double hr_rage_threshold;
  double hr_gore_chance_pct;
  double kb_excess_rage = 0.0;
  double kb_max_excess_rage = 0.0;
  double kb_excess_rage_mul = 0.0;
  double vb_mul = 0.0;
  bool max_rage = false;

  maul_base_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f )
    : base_t( n, p, s, f ),
      hr_rage_threshold( p->talent.harnessed_rage->effectN( 1 ).base_value() ),
      hr_gore_chance_pct( p->talent.harnessed_rage->effectN( 2 ).percent() ),
      vb_mul( p->talent.wild_guardian_3->effectN( 1 ).percent() )
  {
    add_option( opt_bool( "max_rage", max_rage ) );

    if ( p->talent.killing_blow.ok() )
    {
      kb_max_excess_rage = find_effect( p->talent.killing_blow, A_MOD_MAX_RESOURCE_COST ).resource();
      kb_excess_rage_mul = p->talent.killing_blow->effectN( 2 ).percent();
    }
  }

  using state_t = druid_action_state_t<maul_data_t>;

  action_state_t* new_state() override
  { return new state_t( this, target ); }

  state_t* cast_state( action_state_t* s )
  { return static_cast<state_t*>( s ); }

  const state_t* cast_state( const action_state_t* s ) const
  { return static_cast<const state_t*>( s ); }

  bool ready() override
  {
    if ( max_rage && p()->resources.current[ RESOURCE_RAGE ] < ( cost() + kb_max_excess_rage ) )
      return false;

    return base_t::ready();
  }

  double gore_chance() override
  {
    auto g = base_t::gore_chance();

    if ( hr_rage_threshold && p()->resources.current[ RESOURCE_RAGE ] >= hr_rage_threshold )
      g *= 1.0 + hr_gore_chance_pct;

    return g;
  }

  double get_excess_rage() const
  {
    return std::min( kb_max_excess_rage, p()->resources.current[ RESOURCE_RAGE ] - cost() );
  }

  virtual double rage_modifier() const
  {
    return kb_max_excess_rage ? kb_excess_rage / kb_max_excess_rage : 0.0;
  }

  void execute() override
  {
    if ( kb_excess_rage_mul )
      kb_excess_rage = get_excess_rage();

    base_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( vb_mul && s->result_amount )
      residual_action::trigger( p()->active.vicious_brambles, s->target, s->result_amount * vb_mul );
  }

  void consume_resource() override
  {
    base_t::consume_resource();

    if ( kb_excess_rage && hit_any_target )
    {
      p()->resource_loss( RESOURCE_RAGE, kb_excess_rage );
      stats->consume_resource( RESOURCE_RAGE, kb_excess_rage );
      last_resource_cost += kb_excess_rage;

      consume_rage_memory_of_ysera( kb_excess_rage );
      consume_rage_after_the_wildfire( kb_excess_rage );
      consume_rage_ursocs_guidance( kb_excess_rage );
    }
  }

  void snapshot_state( action_state_t* s, result_amount_type rt ) override
  {
    if ( kb_excess_rage_mul )
    {
      assert( p()->resources.current[ RESOURCE_RAGE ] >= cost() );
      cast_state( s )->rage_mod = rage_modifier();
    }

    base_t::snapshot_state( s, rt );
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    return base_t::composite_da_multiplier( s ) * ( 1.0 + cast_state( s )->rage_mod );
  }
};

struct raze_base_t : public maul_base_t
{
  double aoe_coeff;

  raze_base_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f )
    : maul_base_t( n, p, s, f )
  {
    // the aoe effect is parsed last and overwrites the st effect, so we need to cache the aoe coeff and re-parse the
    // st effect
    aoe_coeff = attack_power_mod.direct;
    parse_effect_direct_mods( data().effectN( 1 ), false );

    aoe = -1;  // actually a frontal cone
    reduced_aoe_targets = data().effectN( 3 ).base_value();
  }

  double attack_direct_power_coefficient( const action_state_t* s ) const override
  {
    return s->chain_target == 0 ? maul_base_t::attack_direct_power_coefficient( s ) : aoe_coeff;
  }
};

using ravage_t = ravage_base_t<maul_base_t, use_dot_list_t<bear_attack_t>>;

template <typename BASE>
struct maul_ravage_base_t : public trigger_celestial_might_repeat_t<BASE>
{
private:
  using ab = trigger_celestial_might_repeat_t<BASE>;

protected:
  using base_t = maul_ravage_base_t<BASE>;

public:
  struct ravage_maul_t final : public trigger_celestial_might_repeat_t<ravage_t>
  {
    ravage_maul_t( druid_t* p, std::string_view n, flag_e f ) : base_t( n, p, p->find_spell( 441605 ), f )
    {
      if ( !data().ok() )
        return;

      // hasted gcd despite being 1s
      gcd_type = gcd_haste_type::ATTACK_HASTE;

      if ( p->talent.wild_guardian_1.ok() )
        echo_action = p->active.echo_of_ravage;

      if ( p->sets->has_set_bonus( DRUID_GUARDIAN, MID1, B4 ) )
      {
        repeat_action = p->get_secondary_action<celestial_might_maul_t<ravage_t>>( name_str + "_repeat", &data() );
        add_child( repeat_action );
      }
    }
  };

  ravage_maul_t* ravage = nullptr;

  maul_ravage_base_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f ) : ab( n, p, s, f )
  {
    if ( !p->buff.ravage_maul->is_fallback )
    {
      ravage = p->get_secondary_action<ravage_maul_t, false>( "ravage_" + ab::name_str, f );
      ab::add_child( ravage );
    }
  }

  void start_gcd() override
  {
    // TODO: bear ravage is bugged and has 1s gcd
    if ( ravage && ab::p()->buff.ravage_maul->check() )
      ravage->start_gcd();
    else
      ab::start_gcd();
  }

  void execute() override
  {
    if ( ravage && ab::p()->buff.ravage_maul->check() )
    {
      ravage->execute_on_target( ab::target );
      ab::p()->buff.ravage_maul->consume( this );

      return;
    }

    ab::execute();
  }
};

struct maul_t final : public maul_ravage_base_t<maul_base_t>
{
  DRUID_ABILITY( maul_t, base_t, "maul", p->talent.maul )
  {
    if ( !data().ok() )
      return;

    if ( p->talent.wild_guardian_1.ok() )
      echo_action = p->active.echo_of_maul;

    if ( p->sets->has_set_bonus( DRUID_GUARDIAN, MID1, B4 ) )
    {
      repeat_action = p->get_secondary_action<celestial_might_maul_t<maul_base_t>>( name_str + "_repeat", &data() );
      add_child( repeat_action );
    }
  }

  bool ready() override
  {
    return p()->talent.raze.ok() ? false : base_t::ready();
  }

  double rage_modifier() const override
  {
    if ( has_flag( flag_e::CONVOKE ) )
      return kb_excess_rage_mul;  // convoke maul always gets full multiplier
    else
      return base_t::rage_modifier();
  }
};

struct raze_t final : public maul_ravage_base_t<raze_base_t>
{
  DRUID_ABILITY( raze_t, base_t, "raze", p->talent.raze )
  {
    if ( !data().ok() )
      return;

    if ( p->talent.wild_guardian_1.ok() )
      echo_action = p->active.echo_of_raze;

    if ( p->sets->has_set_bonus( DRUID_GUARDIAN, MID1, B4 ) )
    {
      repeat_action = p->get_secondary_action<celestial_might_maul_t<raze_base_t>>( name_str + "_repeat", &data() );
      add_child( repeat_action );
    }
  }
};

// Sundering Roar ===========================================================
struct sundering_roar_t final : public bear_attack_t
{
  DRUID_ABILITY( sundering_roar_t, bear_attack_t, "sundering_roar", p->talent.sundering_roar )
  {
    aoe = -1;
  }

  void execute() override
  {
    bear_attack_t::execute();

    p()->buff.sundering_roar->trigger();

    if ( p()->cooldown.thrash )
      p()->cooldown.thrash->reset( true );
  }
};

// Swipe (Bear) =============================================================
struct swipe_bear_t final : public trigger_claw_rampage_t<DRUID_GUARDIAN,
                                   trigger_aggravate_wounds_t<DRUID_GUARDIAN,
                                   trigger_gore_t<bear_attack_t>>>
{
  DRUID_ABILITY( swipe_bear_t, base_t, "swipe_bear", p->apply_override( p->talent.swipe, p->spec.bear_form_override ) )
  {
    aoe = -1;
    // target hit data stored in cat swipe
    reduced_aoe_targets = p->apply_override( p->talent.swipe, p->spec.cat_form_override )->effectN( 4 ).base_value();

    if ( p->specialization() == DRUID_GUARDIAN )
      name_str_reporting = "swipe";
    else
      name_str_reporting = "Bear";
  }
};

// Thrash ===================================================================
struct thrash_t final : public trigger_claw_rampage_t<DRUID_GUARDIAN,
                               trigger_aggravate_wounds_t<DRUID_GUARDIAN,
                               trigger_ursocs_fury_t<
                               trigger_gore_t<bear_attack_t>>>>
{
  struct thrash_bleed_t final : public trigger_ursocs_fury_t<use_dot_list_t<bear_attack_t>>
  {
    int original_max_stack;

    thrash_bleed_t( druid_t* p, std::string_view n, flag_e f ) : base_t( n, p, p->spec.thrash_bleed, f )
    {
      dual = background = proc = true;

      dot_name = "thrash";
      dot_list = &p->dot_lists.thrash;

      original_max_stack = p->spec.thrash_bleed->max_stacks();
    }

    void trigger_dot( action_state_t* s )
    {
      if ( auto _stack = p()->buff.sundering_roar->check_value() )
      {
        dot_max_stack = original_max_stack + as<int>( _stack );
        base_t::trigger_dot( s );
        dot_max_stack = original_max_stack;
      }
      else
      {
        base_t::trigger_dot( s );
      }
    }
  };

  double fc_pct;
  double cascade_chance;

  DRUID_ABILITY( thrash_t, base_t, "thrash", p->spec.thrash ),
    fc_pct( p->talent.flashing_claws->effectN( 1 ).percent() ),
    cascade_chance( p->specialization() == DRUID_GUARDIAN ? p->talent.star_cascade->effectN( 2 ).percent() : 0.0 )
  {
    aoe = -1;
    impact_action = p->get_secondary_action<thrash_bleed_t>( name_str + "_dot", f );
    replace_stats( this, impact_action );
    track_cd_waste = true;

    dot_name = "thrash";

    set_tracked_cooldown( p->cooldown.thrash );
  }

  void execute() override
  {
    base_t::execute();

    if ( rng().roll( fc_pct ) )
      make_event( *sim, 500_ms, [ this ]() { p()->active.thrash_flashing->execute_on_target( target ); } );

    if ( p()->talent.waking_nightmare.ok() && p()->pets.dread_shade.n_active_pets() )
    {
      if ( p()->bugs )  // only the latest summon casts dire echo
      {
        if ( auto pet = p()->pets.dread_shade.active_pets().back() )
          pet->dire_echo->execute();
      }
      else
      {
        for ( auto pet : p()->pets.dread_shade.active_pets() )
          pet->dire_echo->execute();
      }
    }

    if ( !proc && hit_any_target && rng().roll( cascade_chance ) )
    {
      // technically thrash target order is random, and the starsurge procs on first target hit. simc does not randomize
      // order of thrash targets so we instead randomize the starsurge target
      p()->active.star_cascade->execute_on_target( rng().range( target_list() ) );
    }
  }
};

// Vicious Brambles =========================================================
struct vicious_brambles_t final : residual_action::residual_periodic_action_t<bear_attack_t>
{
  vicious_brambles_t( druid_t* p ) : residual_action_t( "vicious_brambles", p, p->find_spell( 1270065 ) )
  {
    proc = true;
  }
};
} // end namespace bear_attacks

namespace heals
{
template <typename BASE>
struct trigger_lethal_preservation_t : public BASE
{
  struct lethal_preservation_heal_t final : public druid_heal_t
  {
    double mul;

    lethal_preservation_heal_t( druid_t* p )
      : druid_heal_t( "lethal_preservation_heal", p, p->find_spell( 455470 ) ),
        mul( p->talent.lethal_preservation->effectN( 1 ).percent() )
    {
      background = proc = true;
    }

    void execute() override
    {
      base_dd_min = base_dd_max = p()->resources.max[ RESOURCE_HEALTH ] * mul;

      druid_heal_t::execute();
    }
  };

private:
  druid_t* p_;
  action_t* heal = nullptr;
  gain_t* lp_gain = nullptr;
  double lp_amt = 0.0;

protected:
  using base_t = trigger_lethal_preservation_t<BASE>;

public:
  trigger_lethal_preservation_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE )
    : BASE( n, p, s, f ), p_( p )
  {
    if ( p_->talent.lethal_preservation.ok() )
    {
      // TODO: ally heal NYI
      heal = p_->get_secondary_action<lethal_preservation_heal_t>( "lethal_preservation_heal" );
      lp_gain = p->get_gain( "Lethal Preservation" );
      lp_amt = find_effect( p->find_spell( 455466 ), E_ENERGIZE ).resource();
    }
  }

  void execute() override
  {
    BASE::execute();

    // TODO: will ALWAYS proc. assumes all soothe/decurse are sucessful
    if ( heal )
    {
      p_->resource_gain( RESOURCE_COMBO_POINT, lp_amt, lp_gain );
      heal->execute();
    }
  }
};

// Efflorescence ============================================================
struct efflorescence_t final : public druid_heal_t
{
  struct efflorescence_tick_t final : public druid_heal_t
  {
    efflorescence_tick_t( druid_t* p ) : druid_heal_t( "efflorescence_tick", p, p->find_spell( 81269 ) )
    {
      background = dual = ground_aoe = true;
      aoe = 3;  // hardcoded
      name_str_reporting = "efflorescence";
    }

    std::vector<player_t*>& target_list() const override
    {
      // get the full list
      auto& tl = druid_heal_t::target_list();

      rng().shuffle( tl.begin(), tl.end() );

      range::sort( tl, []( const player_t* l, const player_t* r ) {
        return l->health_percentage() < r->health_percentage();
      } );

      return tl;
    }

    void execute() override
    {
      // force re-evaluation of targets every tick
      target_cache.is_valid = false;

      druid_heal_t::execute();
    }
  };

  ground_aoe_params_t params;
  action_t* heal;
  timespan_t duration;
  timespan_t period;

  DRUID_ABILITY( efflorescence_t, druid_heal_t, "efflorescence", p->talent.efflorescence )
  {
    auto efflo_data = p->find_spell( 81262 );
    duration = efflo_data->duration();
    period = find_effect( efflo_data, A_PERIODIC_DUMMY ).period();

    if ( data().ok() )
    {
      heal = p->get_secondary_action<efflorescence_tick_t>( "efflorescence_tick" );
      replace_stats( this, heal );

      params.hasted( ground_aoe_params_t::hasted_with::SPELL_HASTE )
        .pulse_time( period )
        .duration( duration )
        .action( heal );
    }
  }

  void execute() override
  {
    druid_heal_t::execute();

    params.start_time( timespan_t::min() )  // reset start time
      .target( target );

    make_event<ground_aoe_event_t>( *sim, p(), params );
  }
};

// Elune's Favored ==========================================================
struct elunes_favored_heal_t final : public druid_residual_action_t<druid_heal_t>
{
  elunes_favored_heal_t( druid_t* p ) : base_t( "elunes_favored", p, p->find_spell( 370602 ) )
  {
    background = proc = true;
  }
};

// Ironbark =================================================================
struct ironbark_t final : public druid_heal_t
{
  DRUID_ABILITY( ironbark_t, druid_heal_t, "ironbark", p->talent.ironbark ) {}

  void impact( action_state_t* s ) override
  {
    druid_heal_t::impact( s );

    td( s->target )->buff.ironbark->trigger();
  }
};

// Lifebloom ================================================================
struct lifebloom_t final : public druid_heal_t
{
  struct lifebloom_bloom_t final : public druid_heal_t
  {
    lifebloom_bloom_t( druid_t* p ) : druid_heal_t( "lifebloom_bloom", p, p->find_spell( 33778 ) )
    {
      background = dual = true;
    }
  };

  action_t* bloom;

  DRUID_ABILITY( lifebloom_t, druid_heal_t, "lifebloom", p->talent.lifebloom )
  {
    if ( data().ok() )
    {
      bloom = p->get_secondary_action<lifebloom_bloom_t>( "lifebloom_bloom" );
      replace_stats( this, bloom );
    }
  }

  void impact( action_state_t* s ) override
  {
    // Cancel dot on all targets other than the one we impact on
    for ( const auto& t : sim->player_non_sleeping_list )
    {
      if ( t == target )
        continue;

      auto d = get_dot( t );

      if ( sim->debug )
        sim->print_debug( "{} fades from {}", *d, *t );

      d->reset();  // we don't want last_tick() because there is no bloom on target swap
    }

    if ( auto lb = get_dot( target ); lb->is_ticking() && lb->remains() <= dot_duration * 0.3 )
      bloom->execute_on_target( target );

    druid_heal_t::impact( s );
  }

  void trigger_dot( action_state_t* s ) override
  {
    if ( auto lb = find_dot( s->target ); lb && lb->remains() <= composite_dot_duration( lb->state ) * 0.3 )
      bloom->execute_on_target( s->target );

    druid_heal_t::trigger_dot( s );
  }

  void tick( dot_t* d ) override
  {
    druid_heal_t::tick( d );

    p()->buff.clearcasting_tree->trigger();
  }

  void last_tick( dot_t* d ) override
  {
    if ( !d->state->target->is_sleeping() )  // Prevent crash at end of simulation
      bloom->execute_on_target( d->target );

    druid_heal_t::last_tick( d );
  }
};

// Nature's Cure ============================================================
struct natures_cure_t final : public trigger_lethal_preservation_t<druid_heal_t>
{
  DRUID_ABILITY( natures_cure_t, base_t, "natures_cure", p->find_specialization_spell( "Nature's Cure" ) ) {}
};

// Nature's Swiftness =======================================================
struct natures_swiftness_t final : public trigger_control_of_the_dream_t<druid_heal_t>
{
  DRUID_ABILITY( natures_swiftness_t, base_t, "natures_swiftness", p->talent.natures_swiftness )
  {
    if ( data().ok() )
    {
      p->buff.natures_swiftness->set_expire_callback( [ this ]( buff_t*, int, timespan_t ) {
        cooldown->start();
      } );
    }
  }

  timespan_t cooldown_duration() const override
  {
    return 0_ms;  // cooldown handled by buff.natures_swiftness
  }

  void execute() override
  {
    base_t::execute();

    p()->buff.natures_swiftness->trigger();
  }

  bool ready() override
  {
    if ( p()->buff.natures_swiftness->check() )
      return false;

    return base_t::ready();
  }
};

// Regrowth =================================================================
struct regrowth_t final : public trigger_thriving_growth_t<use_dot_list_t<druid_heal_t>>
{
  buff_t* boon_of_the_oathsworn_hack = nullptr;
  timespan_t gcd_add;
  double bonus_crit;
  double sotf_mul;
  bool is_direct = false;

  DRUID_ABILITY( regrowth_t, base_t, "regrowth", p->find_class_spell( "Regrowth" ) ),
    gcd_add( find_effect( p->spec.cat_form_passive, this, A_ADD_FLAT_MODIFIER, P_GCD ).time_value() ),
    bonus_crit( p->talent.improved_regrowth->effectN( 1 ).percent() ),
    sotf_mul( p->buff.soul_of_the_forest_tree->data().effectN( 1 ).percent() )
  {
    form_mask = NO_FORM | MOONKIN_FORM;

    dot_list = &p->dot_lists.regrowth;

    // dream guide snapshots the hot
    if ( !p->buff.dream_guide->is_fallback )
    {
      const auto& eff = p->buff.dream_guide->data().effectN( 2 );
      if ( !has_parse_entry( persistent_multiplier_effects, &eff ) )
      {
        add_parse_entry( persistent_multiplier_effects )
          .set_buff( p->buff.dream_guide )
          .set_value( eff.percent() )
          .set_use_stacks( false )
          .set_eff( &eff )
          .print_debug( this );
      }
    }

    // dream of cenarius snapshots the hot
    if ( !p->buff.dream_of_cenarius->is_fallback )
    {
      auto eff = &find_effect( p->buff.dream_of_cenarius, this, A_ADD_PCT_MODIFIER, P_TICK_DAMAGE );
      if ( !has_parse_entry( persistent_multiplier_effects, eff ) )
      {
        add_parse_entry( persistent_multiplier_effects )
          .set_buff( p->buff.dream_of_cenarius )
          .set_value( eff->percent() )
          .set_eff( eff )
          .print_debug( this );
      }
    }
  }

  void init() override
  {
    base_t::init();

    if ( is_precombat && unique_gear::find_special_effect( player, 1232776 ) )
      boon_of_the_oathsworn_hack = buff_t::find( player, "boon_of_the_oathsworn" );
  }

  timespan_t gcd() const override
  {
    timespan_t g = base_t::gcd();

    if ( p()->form == CAT_FORM )
      g += gcd_add;

    return g;
  }

  timespan_t execute_time() const override
  {
    if ( p()->buff.incarnation_tree->check() )
      return 0_ms;

    return base_t::execute_time();
  }

  double composite_target_crit_chance( player_t* t ) const override
  {
    double tcc = base_t::composite_target_crit_chance( t );

    if ( is_direct && get_td( t )->hots.regrowth->is_ticking() )
      tcc += bonus_crit;

    return tcc;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double ctm = base_t::composite_target_multiplier( t );

    if ( t == player && p()->talent.harmonious_constitution.ok() )
      ctm *= 1.0 + p()->talent.harmonious_constitution->effectN( 1 ).percent();

    return ctm;
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    auto pm = base_t::composite_persistent_multiplier( s );

    if ( p()->buff.soul_of_the_forest_tree->check() )
      pm *= 1.0 + sotf_mul;

    return pm;
  }

  timespan_t dot_duration_flat_modifier( const action_state_t* s ) const override
  {
    auto mod = base_t::dot_duration_flat_modifier( s );

    if ( s->target == player )
      mod += p()->talent.lingering_healing->effectN( 2 ).time_value();

    return mod;
  }

  bool check_form_restriction() override
  {
    if ( p()->buff.predatory_swiftness->check() ||
         p()->buff.dream_of_cenarius->check() ||
         p()->buff.dream_guide->check() )
    {
      return true;
    }

    return base_t::check_form_restriction();
  }

  void execute() override
  {
    is_direct = true;
    base_t::execute();
    is_direct = false;

    p()->buff.forestwalk->trigger( this );

    p()->buff.blooming_infusion_damage_counter->trigger( this );
    if ( p()->buff.blooming_infusion_damage_counter->at_max_stacks() )
    {
      if ( !proc && !background && !is_precombat && time_to_execute > 0_ms )
        p()->spell_queued.blooming_infusion_damage = true;
      else
        p()->buff.blooming_infusion_damage->trigger();
    }

    if ( is_precombat && boon_of_the_oathsworn_hack && !boon_of_the_oathsworn_hack->check() )
      boon_of_the_oathsworn_hack->trigger();
  }
};

// Rejuvenation =============================================================
struct rejuvenation_base_t : public druid_heal_t
{
  struct cultivation_t final : public druid_heal_t
  {
    cultivation_t( druid_t* p ) : druid_heal_t( "cultivation", p, p->find_spell( 200389 ) ) {}
  };

  action_t* cult_hot = nullptr;
  double cult_pct;  // NOTE: this is base_value, NOT percent()
  double sotf_mul;

  rejuvenation_base_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f )
    : druid_heal_t( n, p, s, f ),
      cult_pct( p->talent.cultivation->effectN( 1 ).base_value() ),
      sotf_mul( p->buff.soul_of_the_forest_tree->data().effectN( 1 ).percent() )
  {
    if ( p->talent.cultivation.ok() )
      cult_hot = p->get_secondary_action<cultivation_t>( "cultivation" );
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    auto pm = druid_heal_t::composite_persistent_multiplier( s );

    if ( p()->buff.soul_of_the_forest_tree->check() )
      pm *= 1.0 + sotf_mul;

    return pm;
  }

  void trigger_dot( action_state_t* s ) override
  {
    if ( !get_dot( s->target )->is_ticking() )
      p()->buff.abundance->increment();

    druid_heal_t::trigger_dot( s );
  }

  void tick( dot_t* d ) override
  {
    druid_heal_t::tick( d );

    if ( cult_hot && d->target->health_percentage() <= cult_pct )
      cult_hot->execute_on_target( d->target );
  }

  void last_tick( dot_t* d ) override
  {
    druid_heal_t::last_tick( d );

    p()->buff.abundance->decrement();
  }
};

struct rejuvenation_t final : public rejuvenation_base_t
{
  struct germination_t final : public rejuvenation_base_t
  {
    germination_t( druid_t* p, flag_e f ) : rejuvenation_base_t( "germination", p, p->find_spell( 155777 ), f ) {}
  };

  action_t* germination = nullptr;

  DRUID_ABILITY( rejuvenation_t, rejuvenation_base_t, "rejuvenation", p->talent.rejuvenation )
  {
    if ( p->talent.germination.ok() )
      germination = p->get_secondary_action<germination_t>( "germination", f );
  }

  void execute() override
  {
    if ( p()->talent.germination.ok() )
    {
      auto hots = td( target )->hots;

      if ( ( hots.rejuvenation->is_ticking() && !hots.germination->is_ticking() ) ||
           ( hots.germination->remains() < hots.rejuvenation->remains() ) )
      {
        germination->execute_on_target( target );
        return;
      }
    }

    druid_heal_t::execute();
  }
};

// Remove Corruption ========================================================
struct remove_corruption_t final : public trigger_lethal_preservation_t<druid_heal_t>
{
  DRUID_ABILITY( remove_corruption_t, base_t, "remove_corruption", p->talent.remove_corruption ) {}
};

// Soothe ===================================================================
struct soothe_t final : public trigger_lethal_preservation_t<druid_heal_t>
{
  DRUID_ABILITY( soothe_t, base_t, "soothe", p->talent.soothe ) {}
};

// Swiftmend ================================================================
struct swiftmend_t final : public druid_heal_t
{
  DRUID_ABILITY( swiftmend_t, druid_heal_t, "swiftmend", p->talent.swiftmend ) {}

  bool target_ready( player_t* t ) override
  {
    auto hots = td( t )->hots;

    if ( hots.regrowth->is_ticking() || hots.wild_growth->is_ticking() || hots.rejuvenation->is_ticking() )
      return druid_heal_t::target_ready( t );

    return false;
  }

  void execute() override
  {
    druid_heal_t::execute();

    p()->buff.soul_of_the_forest_tree->trigger();
  }

  void impact( action_state_t* s ) override
  {
    auto t_td = td( s->target );

    if ( t_td->hots.regrowth->is_ticking() )
      t_td->hots.regrowth->cancel();
    else if ( t_td->hots.wild_growth->is_ticking() )
      t_td->hots.wild_growth->cancel();
    else if ( t_td->hots.rejuvenation->is_ticking() )
      t_td->hots.rejuvenation->cancel();
    else
      sim->error( "Swiftmend impact with no HoT ticking" );

    druid_heal_t::impact( s );
  }
};

// Tranquility ==============================================================
struct tranquility_t final : public druid_heal_t
{
  struct tranquility_tick_t final : public druid_heal_t
  {
    tranquility_tick_t( druid_t* p )
      : druid_heal_t( "tranquility_tick", p, find_trigger( p->talent.tranquility ).trigger() )
    {
      background = dual = true;
      aoe = -1;
    }
  };

  DRUID_ABILITY( tranquility_t, druid_heal_t, "tranquility", p->talent.tranquility )
  {
    channeled = true;

    tick_action = p->get_secondary_action<tranquility_tick_t>( "tranquility_tick" );
    replace_stats( this, tick_action );
  }

  void init() override
  {
    druid_heal_t::init();

    // necessary because action_t::init() sets all tick_action::direct_tick to true
    tick_action->direct_tick = false;
  }
};

// Wild Growth ==============================================================
// The spellpower coefficient c of a tick of WG is given by:
//   c(t) = 0.175 - 0.07 * t / D
// where:
//   t = time of tick = current_time - start_time
//   D = full duration of WG
struct wild_growth_t final : public druid_heal_t
{
  double decay_coeff;
  double sotf_mul;
  int inc_mod;

  DRUID_ABILITY( wild_growth_t, druid_heal_t, "wild_growth", p->talent.wild_growth ),
    decay_coeff( 0.07 * ( 1.0 - p->talent.unstoppable_growth->effectN( 1 ).percent() ) ),
    sotf_mul( p->buff.soul_of_the_forest_tree->data().effectN( 2 ).percent() ),
    inc_mod( as<int>( p->talent.incarnation_tree->effectN( 3 ).base_value() ) )
  {
    aoe = as<int>( data().effectN( 2 ).base_value() + p->talent.improved_wild_growth->effectN( 1 ).base_value() );

    // '0-tick' coeff, also unknown if this is hard set to 0.175 or based on a formula as below
    spell_power_mod.tick += decay_coeff / 2.0;
  }

  double spell_tick_power_coefficient( const action_state_t* s ) const override
  {
    auto c = druid_heal_t::spell_tick_power_coefficient( s );
    auto dot = find_dot( s->target );

    c -= decay_coeff * ( 1.0 - dot->remains() / dot->duration() );

    return c;
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    auto pm = druid_heal_t::composite_persistent_multiplier( s );

    if ( p()->buff.soul_of_the_forest_tree->check() )
      pm *= 1.0 + sotf_mul;

    return pm;
  }

  int n_targets() const override
  {
    int n = druid_heal_t::n_targets();

    if ( n && p()->buff.incarnation_tree->check() )
      n += inc_mod;

    return n;
  }
};

// Ysera's Gift =============================================================
struct yseras_gift_t final : public druid_heal_t
{
  double mul;

  yseras_gift_t( druid_t* p )
    : druid_heal_t( "yseras_gift", p, p->find_spell( 145109 ) ), mul( p->talent.yseras_gift->effectN( 1 ).percent() )
  {
    background = dual = true;
  }

  void init() override
  {
    druid_heal_t::init();

    snapshot_flags &= ~STATE_VERSATILITY;  // Is not affected by versatility.
  }

  double bonus_da( const action_state_t* ) const override
  {
    return p()->resources.max[ RESOURCE_HEALTH ] * mul;
  }

  void execute() override
  {
    if ( p()->health_percentage() < 100 )
      target = p();
    else
      target = smart_target();

    druid_heal_t::execute();
  }
};

// Frenzied Regeneration ====================================================
// NOTE: this must come after regrowth and rejuvenation due to reinvigoration
struct frenzied_regeneration_t final : public trigger_wild_guardian_echo_base_t<
                                              bear_attacks::rage_spender_t<
                                              trigger_call_of_the_elder_druid_t<
                                              druid_heal_t>>>
{
  struct echo_of_frenzied_regeneration_t final : public druid_heal_t
  {
    echo_of_frenzied_regeneration_t( druid_t* p )
      : druid_heal_t( "echo_of_frenzied_regeneration", p, p->find_spell( 1269645 ), flag_e::WILDGUARDIAN )
    {
      proc = background = true;

      // normalized based on duration, results in 17.5% less total healing
      base_td_multiplier *= 1.0 + ( p->talent.frenzied_regeneration->duration() / data().duration() );

      // scripted adjustment for increased tick time from reinvigoration, results 20% more total healing
      base_td_multiplier *=
        1.0 + ( find_effect( p->talent.reinvigoration, this, A_ADD_PCT_MODIFIER, P_TICK_TIME ).percent() );

      base_td_multiplier *= p->talent.wild_guardian_1->effectN( 1 ).percent();
    }

    dot_t* get_dot( player_t* t ) override
    {
      assert( t == player );
      return p()->hots.echo_of_frenzied_regeneration;
    }
  };

  action_t* regrowth = nullptr;
  action_t* rejuvenation = nullptr;
  double goe_mul;
  double ir_mul;
  double nr_pct;

  DRUID_ABILITY( frenzied_regeneration_t, base_t, "frenzied_regeneration", p->talent.frenzied_regeneration ),
    goe_mul( p->buff.guardian_of_elune->data().effectN( 2 ).percent() ),
    ir_mul( p->talent.innate_resolve->effectN( 1 ).percent() ),
    nr_pct( p->talent.natural_resilience->effectN( 1 ).percent() )
  {
    target = p;

    if ( p->talent.empowered_shapeshifting.ok() )
    {
      form_mask |= CAT_FORM;

      base_costs[ RESOURCE_ENERGY ] =
        find_effect( p->talent.empowered_shapeshifting, this, A_ADD_FLAT_MODIFIER, P_RESOURCE_COST_2 )
          .resource( RESOURCE_ENERGY );
    }

    if ( p->talent.reinvigoration.ok() )
    {
      regrowth = get_reinvigoration_action<regrowth_t>( "regrowth" );
      rejuvenation = get_reinvigoration_action<rejuvenation_t>( "rejuvenation" );
    }

    if ( p->talent.wild_guardian_1.ok() )
    {
      echo_action = p->get_secondary_action<echo_of_frenzied_regeneration_t>( "echo_of_frenzied_regeneration" );
      echo_buff = p->buff.gift_of_frenzied_regeneration;
    }

    // verdant heart from FR doesn't apply to FR itself
    if ( p->talent.verdant_heart.ok() )
      base_multiplier /= 1.0 + find_effect( &data(), A_MOD_HEALING_RECEIVED_PCT ).percent();
  }

  template <typename T>
  T* get_reinvigoration_action( std::string n )
  {
    auto a = p()->get_secondary_action<T>( n );
    a->name_str_reporting = n;
    a->dot_name = n;
    a->base_multiplier = p()->talent.reinvigoration->effectN( 3 ).percent();
    add_child( a );
    return a;
  }

  dot_t* get_dot( player_t* t ) override
  {
    assert( t == player );
    return p()->hots.frenzied_regeneration;
  }

  resource_e current_resource() const override
  {
    if ( p()->talent.empowered_shapeshifting.ok() && p()->form == CAT_FORM )
      return RESOURCE_ENERGY;
    else
      return base_t::current_resource();
  }

  void execute() override
  {
    base_t::execute();

    p()->buff.guardian_of_elune->consume( this );

    if ( regrowth )
      regrowth->execute();

    if ( rejuvenation )
      rejuvenation->execute();
  }

  void tick( dot_t* d ) override
  {
    base_t::tick( d );

    if ( nr_pct )
    {
      auto overheal = d->state->result_total - d->state->result_amount;
      if ( overheal > 0 )
      {
        // initial application gains absorb received bonus
        if ( !p()->buff.natural_resilience->check() )
          overheal *= 1.0 + p()->composite_player_absorb_received_multiplier();

        p()->buff.natural_resilience->trigger( 1, overheal );
        p()->buff.natural_resilience->current_value =
          std::min( p()->buff.natural_resilience->current_value, GUARDIAN_SHIELD_CAP_HP_PCT * p()->max_health() );
      }
    }
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    double pm = base_t::composite_persistent_multiplier( s );

    if ( p()->buff.guardian_of_elune->check() )
      pm *= 1.0 + goe_mul;

    // TODO: confirm the innate resolve multiplier snapshots
    pm *= 1.0 + ir_mul * ( 1.0 - p()->resources.pct( RESOURCE_HEALTH ) );

    return pm;
  }
};
}  // end namespace heals

template <class Base>
void druid_action_t<Base>::init()
{
  ab::init();

  if ( !ab::harmful && !dynamic_cast<druid_heal_t*>( this ) )
    ab::target = ab::player;

  // ensure secondary actions from convoke actions are also procs
  if ( is_free() )
    ab::proc = true;

  // some actions have both direct & periodic damage effects. we don't need to update direct multipliers.
  if ( ab::does_periodic_damage() && ab::does_direct_damage() )
    ab::update_flags &= ~( STATE_MUL_DA | STATE_TGT_MUL_DA | STATE_TGT_MITG_DA | STATE_TGT_ARMOR );
}

namespace spells
{
struct ap_spender_t : public druid_spell_t
{
protected:
  using base_t = ap_spender_t;
  buff_t* weaver_buff = nullptr;
  double mid1_4pc_chance;

public:
  timespan_t hail_dur = 0_ms;

  ap_spender_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f )
    : druid_spell_t( n, p, s, f ), mid1_4pc_chance( p->sets->set( DRUID_BALANCE, MID1, B4 )->effectN( 2 ).percent() )
  {
    if ( p->talent.hail_of_stars.ok() )
    {
      if ( is_free() )
        hail_dur = HAIL_OF_STARS_FREE_DURATION;
      else
        hail_dur = p->talent.hail_of_stars->effectN( 1 ).time_value();
    }
  }

  void consume_resource() override
  {
    druid_spell_t::consume_resource();

    p()->buff.blooming_infusion_heal_counter->trigger( this );

    if ( last_resource_cost <= 0 && hail_dur > 0_ms )
      p()->buff.solstice->extend_duration_or_trigger( hail_dur );
  }

  void execute() override
  {
    assert( weaver_buff );

    druid_spell_t::execute();

    p()->buff.starlord->trigger( this );

    if ( p()->buff.eclipse_lunar->check() )
      debug_cast<buffs::eclipse_lunar_t*>( p()->buff.eclipse_lunar )->trigger_harmony();

    if ( p()->buff.eclipse_solar->check() )
      debug_cast<buffs::eclipse_solar_t*>( p()->buff.eclipse_solar )->trigger_harmony();

    if ( !weaver_buff->consume( this ) )
      if ( !p()->buff.touch_the_cosmos->consume( this ) )
        p()->buff.astral_communion->consume( this );
  }
};

struct ap_generator_t : public druid_spell_t
{
  struct ap_generator_data_t
  {
    bool dream_burst = false;
    bool umbral_embrace = false;

    friend void sc_format_to( const ap_generator_data_t& data, fmt::format_context::iterator out )
    {
      fmt::format_to( out, " dream_burst={} umbral_embrace={}", data.dream_burst, data.umbral_embrace );
    }
  };

private:
  action_t* umbral_proxy = nullptr;
  stats_t* umbral_stats = nullptr;
  stats_t* orig_stats;
  proc_t* fake_umbral = nullptr;
  double cascade_chance;

protected:
  using base_t = ap_generator_t;
  using state_t = druid_action_state_t<ap_generator_data_t>;
  double touch_pct = 0.0;

public:
  lockable_t<bool> dreamstate;

  ap_generator_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f )
    : druid_spell_t( n, p, s, f ),
      orig_stats( stats ),
      cascade_chance( p->specialization() == DRUID_BALANCE ? p->talent.star_cascade->effectN( 1 ).percent() : 0.0 )
  {
    // damage bonus is applied at end of cast, but cast speed bonuses apply before
    parse_effects( p->buff.dreamstate, effect_mask_t( false ).enable( 3 ) );
    parse_effects( &p->buff.dreamstate->data(), effect_mask_t( true ).disable( 3 ), [ this ] { return dreamstate; } );

    base_costs[ RESOURCE_MANA ] = 0.0;  // remove mana cost so we don't need to enable mana regen

    form_mask = NO_FORM | MOONKIN_FORM;

    if ( p->talent.umbral_embrace.ok() )
    {
      // Umbral embrace is heavily scripted so we do all the auto parsing within the action itself
      const auto& eff = p->buff.umbral_embrace->data().effectN( 1 );
      if ( !has_parse_entry( da_multiplier_effects, &eff ) )
      {
        add_parse_entry( da_multiplier_effects )
          .set_value( eff.percent() )
          .set_func( [ this ] { return umbral_embrace_check(); } )
          .set_eff( &eff )
          .print_debug( this );
      }

      auto umbral_name = name_str + "_umbral";

      umbral_proxy = p->get_secondary_action<action_t>( umbral_name, action_e::ACTION_OTHER, umbral_name, p, &data() );
      umbral_proxy->name_str_reporting = "Umbral";

      umbral_stats = p->get_stats( umbral_name, umbral_proxy );
      umbral_stats->school = SCHOOL_ASTRAL;
      stats->add_child( umbral_stats );

      fake_umbral = p->get_proc( util::inverse_tokenize( name_str ) + " (False Astral)" );
    }
  }

  action_state_t* new_state() override
  { return new state_t( this, target ); }

  state_t* cast_state( action_state_t* s )
  { return static_cast<state_t*>( s ); }

  const state_t* cast_state( const action_state_t* s ) const
  { return static_cast<const state_t*>( s ); }

  bool umbral_embrace_check()
  {
    return p()->buff.umbral_embrace->check() &&
           ( p()->buff.eclipse_lunar->check() || p()->buff.eclipse_solar->check() );
  }

  // the school change effect is handled differently than damage buff effect, and will lead to 'false' casts where the
  // school is astral, but the damage bonus does not apply. for now we can treat this as purely cosmetic as all
  // currently applicable school based damage snapshots on cast.
  bool umbral_embrace_false_check()
  {
    return p()->buff.umbral_embrace->check() &&
           ( p()->buff.umbral_embrace->check_value() || p()->buff.eclipse_lunar->check() ||
             p()->buff.eclipse_solar->check() );
  }

  void umbral_embrace_trigger( action_t* a )
  {
    p()->buff.umbral_embrace->trigger( a, 1, p()->buff.eclipse_lunar->check() || p()->buff.eclipse_solar->check() );
  }

  void schedule_execute( action_state_t* s ) override
  {
    dreamstate = p()->buff.dreamstate->check() && !p()->buff.ascendant_fires->check();

    druid_spell_t::schedule_execute( s );
  }

  void snapshot_state( action_state_t* s, result_amount_type rt ) override
  {
    druid_spell_t::snapshot_state( s, rt );

    if ( umbral_embrace_check() )
      cast_state( s )->umbral_embrace = true;

    // only check & consume on actual execute with valid result amount type
    if ( rt != result_amount_type::NONE && s->chain_target == 0 && p()->buff.dream_burst->consume( this ) )
      cast_state( s )->dream_burst = true;
  }

  void execute() override
  {
    if ( rng().roll( touch_pct ) )
      p()->buff.touch_the_cosmos->trigger( this );

    if ( umbral_embrace_check() )
    {
      // preserve original school for lunar amplification/lunation
      set_school_override( SCHOOL_ASTRAL );

      stats = umbral_stats;
      druid_spell_t::execute();
      stats = orig_stats;

      clear_school_override();
      p()->buff.umbral_embrace->consume( this );
    }
    else
    {
      if ( umbral_embrace_false_check() )
        fake_umbral->occur();

      druid_spell_t::execute();
    }

    umbral_embrace_trigger( this );

    if ( dreamstate || !has_flag( flag_e::FOREGROUND ) )
      p()->buff.dreamstate->consume( this );

    dreamstate = false;

    // Dreamstate is triggered after the first harmful cast.
    if ( is_precombat && p()->talent.natures_grace.ok() && !p()->buff.dreamstate->check() )
      p()->buff.dreamstate->trigger();
  }

  void impact( action_state_t* s ) override
  {
    druid_spell_t::impact( s );

    if ( cast_state( s )->dream_burst )
      p()->active.dream_burst->execute_on_target( s->target );

    // has an icd to prevent multiple procs, but we can just check only on the main target instead
    if ( !proc && s->chain_target == 0 && rng().roll( cascade_chance ) )
    {
      if ( time_to_execute > 0_ms )
        p()->spell_queued.star_cascade = target;
      else
        p()->active.star_cascade->execute_on_target( target );
    }
  }

  void record_data( action_state_t* s ) override
  {
    // only required if there is travel time
    if ( travel_speed && cast_state( s )->umbral_embrace )
    {
      stats = umbral_stats;
      druid_spell_t::record_data( s );
      stats = orig_stats;
    }
    else
    {
      druid_spell_t::record_data( s );
    }
  }

  void reset() override { druid_spell_t::reset(); dreamstate = false; }

  void cancel() override { druid_spell_t::cancel(); dreamstate = false; }

  void interrupt_action() override { druid_spell_t::interrupt_action(); dreamstate = false; }
};

// Barkskin =================================================================
struct barkskin_t final : public druid_spell_t
{
  struct brambles_pulse_t final : public bear_attack_t
  {
    brambles_pulse_t( druid_t* p, std::string_view n ) : bear_attack_t( n, p, p->find_spell( 213709 ) )
    {
      background = proc = dual = true;
      aoe = -1;
    }
  };

  action_t* brambles = nullptr;

  DRUID_ABILITY( barkskin_t, druid_spell_t, "barkskin", p->find_class_spell( "Barkskin" ) )
  {
    harmful = false;
    use_off_gcd = true;
    dot_duration = 0_ms;

    if ( p->talent.brambles.ok() )
    {
      brambles = p->get_secondary_action<brambles_pulse_t>( name_str + "_brambles" );
      name_str += "+brambles";
      replace_stats( this, brambles );

      p->buff.barkskin->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
        brambles->execute();
      } );
    }
  }

  void init() override
  {
    druid_spell_t::init();

    if ( brambles && !name_str_reporting.empty() )
      name_str_reporting += "+brambles";
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.barkskin->trigger();
    p()->buff.matted_fur->trigger();
  }
};

// Celestial Alignment ======================================================
struct celestial_alignment_base_t : public trigger_control_of_the_dream_t<druid_spell_t>
{
  buff_t* buff;

  celestial_alignment_base_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f )
    : base_t( n, p, p->apply_override( s, p->talent.orbital_strike ), f ),
      buff( p->buff.celestial_alignment )
  {
    harmful = false;
  }

  void execute() override
  {
    base_t::execute();

    if ( p()->active.ascendant_eclipses )
      p()->active.ascendant_eclipses->stats->iteration_num_executes++;

    if ( p()->buff.eclipse_lunar->check() && p()->buff.eclipse_solar->check() )
      p()->buff.dreamstate->trigger();

    buff->trigger();
  }
};

struct celestial_alignment_t final : public celestial_alignment_base_t
{
  DRUID_ABILITY( celestial_alignment_t, celestial_alignment_base_t, "celestial_alignment",
                 p->talent.incarnation_moonkin.ok() ? spell_data_t::not_found() : p->spec.celestial_alignment )
  {}
};

struct incarnation_moonkin_t final : public celestial_alignment_base_t
{
  DRUID_ABILITY( incarnation_moonkin_t, celestial_alignment_base_t, "incarnation_chosen_of_elune",
                 p->spec.incarnation_moonkin )
  {
    form_mask = MOONKIN_FORM;
    autoshift = p->active.shift_to_moonkin;
    buff = p->buff.incarnation_moonkin;
  }
};

// Dash =====================================================================
struct dash_t final : public druid_spell_t
{
  buff_t* buff_on_cast;

  DRUID_ABILITY( dash_t, druid_spell_t, "dash",
                 p->talent.tiger_dash.ok() ? p->talent.tiger_dash : p->find_class_spell( "Dash" ) ),
    buff_on_cast( p->talent.tiger_dash.ok() ? p->buff.tiger_dash : p->buff.dash )
  {
    harmful = may_miss = false;
    ignore_false_positive = true;

    form_mask = CAT_FORM;
    autoshift = p->active.shift_to_cat;
  }

  void execute() override
  {
    druid_spell_t::execute();

    buff_on_cast->trigger();
  }
};

// Dream Burst ==============================================================
struct dream_burst_t final : public druid_spell_t
{
  dream_burst_t( druid_t* p ) : druid_spell_t( "dream_burst", p, p->find_spell( 433850 ) )
  {
    background = proc = true;
    aoe = -1;
    reduced_aoe_targets = data().effectN( 2 ).base_value();
  }
};

// Eclipse ==================================================================
struct eclipse_base_t : public druid_spell_t
{
  double total_ecl_pct;

  eclipse_base_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f )
    : druid_spell_t( n, p, s, f ), total_ecl_pct( p->talent.total_eclipse->effectN( 1 ).percent() )
  {
    set_shared_cooldown( "eclipse", p->cooldown.eclipse );
  }

  virtual buff_t* get_eclipse_buff( bool flip = false ) const = 0;

  bool ready() override
  {
    return get_eclipse_buff()->check() ? false : druid_spell_t::ready();
  }

  void start_gcd() override
  {
    druid_spell_t::start_gcd();

    if ( p()->active.ascendant_eclipses )
    {
      p()->active.ascendant_eclipses->stats->iteration_num_executes++;
      p()->active.ascendant_eclipses->stats->iteration_total_execute_time += player->gcd_ready - sim->current_time();
    }
  }

  void execute() override
  {
    druid_spell_t::execute();

    // overridden eclipse does not trigger dreamstate
    get_eclipse_buff( true )->expire( timespan_t::max() );
    get_eclipse_buff()->trigger();

    if ( rng().roll( total_ecl_pct ) )
      get_eclipse_buff( true )->trigger();
  }
};

struct lunar_eclipse_t final : public eclipse_base_t
{
  DRUID_ABILITY( lunar_eclipse_t, eclipse_base_t, "lunar_eclipse", p->find_spell( 1233272 ) ) {}

  bool ready() override
  {
    return p()->buff.lunar_eclipse_override->check() ? eclipse_base_t::ready() : false;
  }

  buff_t* get_eclipse_buff( bool flip ) const override
  {
    return flip ? p()->buff.eclipse_solar : p()->buff.eclipse_lunar;
  }
};

struct solar_eclipse_t final : public eclipse_base_t
{
  DRUID_ABILITY( solar_eclipse_t, eclipse_base_t, "solar_eclipse", p->find_spell( 1233346 ) ) {}

  bool ready() override
  {
    return p()->buff.lunar_eclipse_override->check() ? false : eclipse_base_t::ready();
  }

  buff_t* get_eclipse_buff( bool flip ) const override
  {
    return flip ? p()->buff.eclipse_lunar : p()->buff.eclipse_solar;
  }
};

struct eclipse_t final : public eclipse_base_t
{
  DRUID_ABILITY( eclipse_t, eclipse_base_t, "eclipse",
                 p->find_spell( p->talent.lunar_calling.ok() ? 1233272 : 1233346 ) )
  {}

  buff_t* get_eclipse_buff( bool flip ) const override
  {
    return static_cast<bool>( p()->buff.lunar_eclipse_override->check() ) != flip ? p()->buff.eclipse_lunar
                                                                                  : p()->buff.eclipse_solar;
  }
};

// Entangling Roots =========================================================
struct entangling_roots_t final : public druid_spell_t
{
  timespan_t gcd_add;

  DRUID_ABILITY( entangling_roots_t, druid_spell_t, "entangling_roots", p->find_class_spell( "Entangling Roots" ) ),
    gcd_add( find_effect( p->spec.cat_form_passive, this, A_ADD_FLAT_MODIFIER, P_GCD ).time_value() )
  {
    form_mask = NO_FORM | MOONKIN_FORM;
    harmful   = false;
    // workaround so that we do not need to enable mana regen
    base_costs[ RESOURCE_MANA ] = 0.0;
  }

  timespan_t gcd() const override
  {
    timespan_t g = druid_spell_t::gcd();

    if ( p()->form == CAT_FORM )
      g += gcd_add;

    return g;
  }

  bool check_form_restriction() override
  {
    return p()->buff.predatory_swiftness->check() || druid_spell_t::check_form_restriction();
  }
};

// Force of Nature ==========================================================
struct force_of_nature_t final : public trigger_control_of_the_dream_t<druid_spell_t>
{
  std::vector<timespan_t> summon_delays;
  unsigned dream_surge_num = 0;

  DRUID_ABILITY( force_of_nature_t, base_t, "force_of_nature", p->talent.force_of_nature )
  {
    if ( data().ok() )
    {
      if ( p->talent.dryads_dance.ok() && p->talent.sylvan_beckoning.ok() )
      {
        auto m_data = p->get_modified_spell( &data() )
          ->parse_effects( p->buff.sylvan_beckoning );

        set_energize( m_data );
      }

      for ( const auto& eff : data().effects() )
      {
        if ( eff.type() == E_TRIGGER_SPELL && eff.trigger()->id() == 248280 )
          summon_delays.push_back( timespan_t::from_millis( eff.misc_value1() ) );

        if ( summon_delays.size() >= as<size_t>( p->talent.force_of_nature->effectN( 1 ).base_value() ) )
          break;
      }

      p->pets.force_of_nature.set_default_duration( find_trigger( this ).trigger()->duration() + 1_ms );
      p->pets.force_of_nature.set_creation_event_callback( pets::parent_pet_action_fn( this ) );

      if ( p->active.treants_of_the_moon_mf )
        add_child( p->active.treants_of_the_moon_mf );

      if ( p->active.dream_burst )
      {
        add_child( p->active.dream_burst );

        dream_surge_num = as<unsigned>( p->talent.dream_surge->effectN( 1 ).base_value() );
      }
    }
  }

  void execute() override
  {
    base_t::execute();

    p()->buff.dream_burst->trigger( dream_surge_num );

    for ( auto t : summon_delays )
      make_event( *sim, t, [ this ] { p()->pets.force_of_nature.spawn(); } );
  }
};

// Fury of Elune =========================================================
struct fury_of_elune_t final : public druid_spell_t
{
  struct fury_of_elune_tick_t final : public trigger_atmospheric_exposure_t<druid_spell_t>
  {
    fury_of_elune_tick_t( druid_t* p, std::string_view n, const spell_data_t* s, flag_e f ) : base_t( n, p, s, f )
    {
      background = proc = dual = ground_aoe = true;
      aoe = -1;
      reduced_aoe_targets = 1.0;
      full_amount_targets = 1;
    }
  };

  struct boundless_moonlight_t final : public druid_spell_t
  {
    boundless_moonlight_t( druid_t* p, std::string_view n, flag_e f )
      : druid_spell_t( n, p, p->find_spell( 428682 ), f )
    {
      background = proc = true;
      aoe = -1;  // TODO: aoe DR?
      name_str_reporting = "boundless_moonlight";

      if ( p->talent.the_eternal_moon.ok() )
      {
        auto power = p->specialization() == DRUID_GUARDIAN ? POWER_RAGE : POWER_ASTRAL_POWER;
        energize_resource = util::power_type_to_resource( power );
        energize_amount = find_effect( this, E_ENERGIZE, A_MAX, power ).resource();
      }
      else
      {
        energize_type = action_energize::NONE;
      }
    }
  };

  ground_aoe_params_t params;
  const spell_data_t* tick_spell;
  buff_t* energize_buff;
  action_t* damage = nullptr;
  action_t* boundless = nullptr;
  timespan_t tick_period;

  DRUID_ABILITY_C( fury_of_elune_t, druid_spell_t, "fury_of_elune", p->talent.fury_of_elune, const spell_data_t* sd = nullptr,
                   buff_t* b = nullptr ),
    tick_spell( sd ? sd : p->find_spell( 211545 ) ), energize_buff( b ? b : p->buff.fury_of_elune ),
    tick_period( find_effect( energize_buff, A_PERIODIC_ENERGIZE, RESOURCE_ASTRAL_POWER ).period() )
  {
    form_mask |= NO_FORM; // can be cast without form
    dot_duration = 0_ms;  // AP gain handled via buffs

    if ( data().ok() )
    {
      damage = p->get_secondary_action<fury_of_elune_tick_t>( name_str + "_tick", tick_spell, f );
      replace_stats( this, damage );

      params.hasted( ground_aoe_params_t::hasted_with::SPELL_HASTE )
        .pulse_time( tick_period )
        .duration( data().duration() )
        .action( damage );

      if ( p->talent.boundless_moonlight.ok() )
      {
        boundless = p->get_secondary_action<boundless_moonlight_t>( name_str + "_boundless", f );
        add_child( boundless );

        params.expiration_callback( [ this ]( const action_state_t* s ) {
          boundless->execute_on_target( s->target );
        } );
      }

      if ( !is_free() )
        track_cd_waste = true;
    }

    set_tracked_cooldown( p->cooldown.fury_of_elune );
  }

  // needed to allow on-cast procs
  bool has_amount_result() const override { return damage->has_amount_result(); }

  void execute() override
  {
    druid_spell_t::execute();

    energize_buff->trigger( params.duration() );

    params.start_time( timespan_t::min() )  // reset start time
      .target( target );

    make_event<ground_aoe_event_t>( *sim, p(), params );
  }
};

// Incarnation (Tree) =========================================================
struct incarnation_tree_t final : public trigger_control_of_the_dream_t<druid_spell_t>
{
  DRUID_ABILITY( incarnation_tree_t, base_t, "incarnation_tree_of_life", p->talent.incarnation_tree )
  {
    harmful   = false;
    form_mask = NO_FORM;
    autoshift = p->active.shift_to_caster;
  }

  void execute() override
  {
    base_t::execute();

    p()->buff.incarnation_tree->trigger();
  }
};

// Innervate ================================================================
struct innervate_t final : public druid_spell_t
{
  DRUID_ABILITY( innervate_t, druid_spell_t, "innervate", p->talent.innervate )
  {
    harmful = false;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.innervate->trigger();
  }
};

// Lunar Bolt ===============================================================
struct lunar_bolt_t final : public druid_spell_t
{
  struct lunar_bolt_damage_t final : public druid_spell_t
  {
    lunar_bolt_damage_t( druid_t* p, const spell_data_t* s ) : druid_spell_t( "lunar_bolt_damage", p, s )
    {
      background = dual = true;
      aoe = -1;
      reduced_aoe_targets = LUNAR_BOLT_REDUCED_AOE;
    }

    result_e calculate_result( action_state_t* ) const override
    {
      return result_e::RESULT_CRIT;
    }
  };

  lunar_bolt_t( druid_t* p ) : druid_spell_t( "lunar_bolt", p, p->find_spell( 1261700 ) )
  {
    proc = true;
    range = p->talent.ascendant_eclipses_3->effectN( 1 ).base_value();
    impact_action = p->get_secondary_action<lunar_bolt_damage_t>( "lunar_bolt_damage", find_trigger( this ).trigger() );

    replace_stats( this, impact_action );
  }

  void execute() override
  {
    // hits random target
    set_target( rng().range( target_list() ) );

    druid_spell_t::execute();
  }
};

// Moon Spells ==============================================================
struct moon_base_t : public druid_spell_t
{
  struct minor_moon_t final : public druid_spell_t
  {
    minor_moon_t( druid_t* p, std::string_view n, flag_e f ) : druid_spell_t( n, p, p->find_spell( 424588 ), f )
    {
      background = proc = true;
      aoe = -1;
      reduced_aoe_targets = 1.0;
      full_amount_targets = 1;
      name_str_reporting = "minor_moon";
    }

    void init() override
    {
      druid_spell_t::init();

      if ( p()->active.moons && get_suffix( name_str, "minor_moon" ).empty() )
        p()->active.moons->add_child( this );
    }
  };

  moon_stage_e stage = moon_stage_e::FULL_MOON;
  action_t* minor = nullptr;
  unsigned num_minor = 0;

  moon_base_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f ) : druid_spell_t( n, p, s, f )
  {
    if ( data().ok() )
    {
      if ( p->talent.dryads_dance.ok() && p->talent.sylvan_beckoning.ok() )
      {
        auto m_data = p->get_modified_spell( &data() )
          ->parse_effects( p->buff.sylvan_beckoning );

        set_energize( m_data );
      }

      if ( p->talent.boundless_moonlight.ok() )
        minor = p->get_secondary_action<minor_moon_t>( "minor_moon", f );

      if ( !is_free() )
      {
        track_cd_waste = true;
        p->active.moons->add_child( this );
      }
    }

    set_shared_cooldown( "moon_cd", p->cooldown.moon_cd );
  }

  virtual bool check_stage() const
  {
    return p()->moon_stage == stage;
  }

  virtual void advance_stage()
  {
    p()->moon_stage++;
  }

  bool ready() override
  {
    return !check_stage() ? false : druid_spell_t::ready();
  }

  void execute() override
  {
    druid_spell_t::execute();

    if ( !is_free() && data().ok() )
      p()->active.moons->stats->add_execute( time_to_execute, target );

    // TODO: any delay/stagger?
    if ( minor && num_minor )
      for ( unsigned i = 0; i < num_minor; i++ )
        minor->execute_on_target( target );

    if ( proc )
        return;

    advance_stage();
  }
};

// New Moon Spell ===========================================================
struct new_moon_t final : public moon_base_t
{
  DRUID_ABILITY( new_moon_t, moon_base_t, "new_moon", p->talent.new_moon )
  {
    stage = moon_stage_e::NEW_MOON;

    if ( minor && p->talent.the_eternal_moon.ok() )
      num_minor = as<unsigned>( minor->data().effectN( 3 ).base_value() );
  }
};

// Half Moon Spell ==========================================================
struct half_moon_t final : public moon_base_t
{
  DRUID_ABILITY( half_moon_t, moon_base_t, "half_moon", p->spec.half_moon )
  {
    stage = moon_stage_e::HALF_MOON;

    if ( minor && p->talent.the_eternal_moon.ok() )
      num_minor = as<unsigned>( minor->data().effectN( 3 ).base_value() );
  }
};

// Full Moon Spell ==========================================================
struct full_moon_t final : public trigger_atmospheric_exposure_t<moon_base_t>
{
  DRUID_ABILITY( full_moon_t, base_t, "full_moon", p->spec.full_moon )
  {
    aoe = -1;
    reduced_aoe_targets = 1.0;
    full_amount_targets = 1;

    // Since this can be free_cast, only energize for Balance
    if ( !p->spec.astral_power->ok() )
    {
      energize_type = action_energize::NONE;
    }
    else if ( has_flag( flag_e::ORBIT ) )
    {
      if ( !energize )
       set_energize( p->get_modified_spell( &data() ) );

      const auto& eff = p->talent.orbit_breaker->effectN( 2 );
      if ( !energize->modified_by( eff ) )
      {
        energize->add_parse_entry()
          .set_value( p->talent.orbit_breaker->effectN( 2 ).percent() - 1.0 )
          .set_eff( &eff );
      }
    }

    if ( data().ok() && p->talent.boundless_moonlight.ok() )
    {
      num_minor = as<unsigned>( p->talent.boundless_moonlight->effectN( 1 ).base_value() );

      if ( !minor )
      {
        auto suf = get_suffix( name_str, "full_moon" );
        minor = p->get_secondary_action<minor_moon_t>( "minor_moon" + suf, f );
        add_child( minor );
      }
    }
  }

  bool check_stage() const override
  {
    return p()->moon_stage >= stage;
  }

  void advance_stage() override
  {
    auto max_stage = p()->talent.radiant_moonlight.ok() ? moon_stage_e::MAX_MOON : moon_stage_e::FULL_MOON;

    if ( p()->moon_stage == max_stage )
      p()->moon_stage = moon_stage_e::NEW_MOON;
    else
      base_t::advance_stage();
  }
};

// Proxy Moon Spell =========================================================
struct moon_proxy_t : public druid_spell_t
{
  new_moon_t* new_moon;
  half_moon_t* half_moon;
  full_moon_t* full_moon;

  moon_proxy_t( druid_t* p )
    : druid_spell_t( "moons", p, p->talent.new_moon.ok() ? spell_data_t::nil() : spell_data_t::not_found() ),
      new_moon( new new_moon_t( p ) ),
      half_moon( new half_moon_t( p ) ),
      full_moon( new full_moon_t( p ) )
  {
    callbacks = false;

    set_school( SCHOOL_ASTRAL );

    new_moon->action_flags |= flag_e::FOREGROUND;
    half_moon->action_flags |= flag_e::FOREGROUND;
    new_moon->action_flags |= flag_e::FOREGROUND;
  }

  void parse_options( util::string_view opt ) override
  {
    druid_spell_t::parse_options( opt );

    new_moon->parse_options( opt );
    half_moon->parse_options( opt );
    full_moon->parse_options( opt );
  }

  void schedule_execute( action_state_t* s ) override
  {
    switch ( p()->moon_stage )
    {
      case NEW_MOON:  new_moon->schedule_execute( s ); return;
      case HALF_MOON: half_moon->schedule_execute( s ); return;
      case FULL_MOON:
      case MAX_MOON:  full_moon->schedule_execute( s ); return;
      default: break;
    }

    if ( s )
      action_state_t::release( s );
  }

  void execute() override
  {
    switch ( p()->moon_stage )
    {
      case NEW_MOON:  new_moon->execute(); return;
      case HALF_MOON: half_moon->execute(); return;
      case FULL_MOON:
      case MAX_MOON:  full_moon->execute(); return;
      default: break;
    }

    if ( pre_execute_state )
      action_state_t::release( pre_execute_state );
  }

  bool action_ready() override
  {
    switch ( p()->moon_stage )
    {
      case NEW_MOON:  return new_moon->action_ready();
      case HALF_MOON: return half_moon->action_ready();
      case FULL_MOON:
      case MAX_MOON:  return full_moon->action_ready();
      default: return false;
    }
  }

  bool target_ready( player_t* t ) override
  {
    switch ( p()->moon_stage )
    {
      case NEW_MOON:  return new_moon->target_ready( t );
      case HALF_MOON: return half_moon->target_ready( t );
      case FULL_MOON:
      case MAX_MOON:  return full_moon->target_ready( t );
      default: return false;
    }
  }

  bool ready() override
  {
    switch ( p()->moon_stage )
    {
      case NEW_MOON:  return new_moon->ready();
      case HALF_MOON: return half_moon->ready();
      case FULL_MOON:
      case MAX_MOON:  return full_moon->ready();
      default: return false;
    }
  }
};

// Moonfire Spell ===========================================================
struct moonfire_t final : public druid_spell_t
{
  struct moonfire_damage_t final : public trigger_gore_t<use_dot_list_t<druid_spell_t>>
  {
    real_ppm_t* light_of_elune_rng = nullptr;
    real_ppm_t* celestial_might_rng = nullptr;  // bear mid1 4pc

    moonfire_damage_t( druid_t* p, std::string_view n, flag_e f ) : base_t( n, p, p->spec.moonfire_dmg, f )
    {
      may_miss = false;
      dual = background = proc = true;

      dot_name = "moonfire";
      dot_list = &p->dot_lists.moonfire;

      if ( p->talent.galactic_guardian.ok() && !has_flag( flag_e::GALACTIC ) )
      {
        const auto& eff = p->buff.galactic_guardian->data().effectN( 3 );
        if ( !has_parse_entry( da_multiplier_effects, &eff ) )
        {
          add_parse_entry( da_multiplier_effects )
            .set_buff( p->buff.galactic_guardian )
            .set_value( eff.percent() )
            .set_eff( &eff )
            .print_debug( this );
        }
      }

      if ( p->talent.the_light_of_elune.ok() )
      {
        light_of_elune_rng = p->get_rppm( "The Light of Elune", p->talent.the_light_of_elune );
      }

      if ( p->sets->has_set_bonus( DRUID_GUARDIAN, MID1, B4 ) )
      {
        celestial_might_rng = p->get_rppm( "Celestial Might", p->sets->set( DRUID_GUARDIAN, MID1, B4 ) );
      }
    }

    void trigger_dot( action_state_t* s ) override
    {
      base_t::trigger_dot( s );

      // moonfire will completely replace lunar inspiration if the new moonfire duration is greater
      auto li_dot = td( s->target )->dots.lunar_inspiration;

      if ( get_dot( s->target )->remains() > li_dot->remains() )
        li_dot->cancel();
    }

    void tick( dot_t* d ) override
    {
      // Moonfire damage is supressed while lunar inspiration is also on the target. Note that it is not cancelled and
      // continues to tick down it's duration. If there is any duration remaining after lunar inspiration expires,
      // moonfire will resume ticking for damage.
      // Note that moonfire CAN still proc shooting stars while suppressed
      if ( td( d->target )->dots.lunar_inspiration->is_ticking() )
        return;

      base_t::tick( d );

      if ( light_of_elune_rng && light_of_elune_rng->trigger() )
        p()->active.the_light_of_elune->execute_on_target( d->target );

      if ( celestial_might_rng && celestial_might_rng->trigger() )
        p()->buff.celestial_might->trigger();
    }

    void impact( action_state_t* s ) override
    {
      base_t::impact( s );

      if ( light_of_elune_rng && light_of_elune_rng->trigger() )
        p()->active.the_light_of_elune->execute_on_target( s->target );
    }
  };

  moonfire_damage_t* damage;  // Add damage modifiers in moonfire_damage_t, not moonfire_t
  moonfire_t* twin = nullptr;
  double twin_range = 0.0;

  DRUID_ABILITY( moonfire_t, druid_spell_t, "moonfire", p->spec.moonfire )
  {
    damage = p->get_secondary_action<moonfire_damage_t>( name_str + "_dmg", f );
    replace_stats( this, damage );

    auto m_data = p->get_modified_spell( &data() )
      ->parse_effects( p->buff.galactic_guardian );

    if ( p->spec.astral_power->ok() && !has_flag( flag_e::TWIN | flag_e::TREANT ) )
    {
      energize_type = action_energize::ON_CAST;
      energize_resource = RESOURCE_ASTRAL_POWER;
    }
    else if ( p->talent.galactic_guardian.ok() && !has_flag( flag_e::GALACTIC ) )
    {
      energize_type = action_energize::ON_CAST;
      energize_resource = RESOURCE_RAGE;
    }
    else if ( p->talent.moon_guardian.ok() && has_flag( flag_e::GALACTIC ) )
    {
      energize_type = action_energize::ON_CAST;
      energize_resource = find_effect( p->find_spell( 430581 ), E_ENERGIZE ).resource_gain_type();
      gain = p->get_gain( "Moon Guardian" );
    }
    else
    {
      energize_type = action_energize::NONE;
    }

    if ( energize_type != action_energize::NONE )
      set_energize( m_data );

    if ( ( p->talent.twin_moonfire.ok() || p->talent.twin_moons.ok() ) && !has_flag( flag_e::TWIN | flag_e::TREANT ) )
    {
      twin = p->get_secondary_action<moonfire_t>( name_str + "_twin", flag_e::TWIN );
      twin->background = twin->dual = twin->proc = true;
      replace_stats( this, twin->damage, false );
      twin->stats = stats;

      // radius is only found in balance twin moons talent data
      twin_range = p->find_spell( 279620 )->effectN( 1 ).base_value();
    }
  }

  // needed to allow on-cast procs
  bool has_amount_result() const override
  {
    return damage->has_amount_result();
  }

  bool ready() override
  {
    return p()->talent.red_moon.ok() ? false : druid_spell_t::ready();
  }

  void execute() override
  {
    // 2nd moonfire executes first for normal moonfire, but 2nd for galactic guardian
    if ( twin && !has_flag( flag_e::GALACTIC ) )
    {
      const auto& tl = target_list();
      if ( auto twin_target = p()->get_smart_target( tl, &druid_td_t::dots_t::moonfire, target, twin_range, true ) )
        twin->execute_on_target( twin_target );
    }

    druid_spell_t::execute();

    auto state = damage->get_state();
    damage->target = state->target = target;
    damage->snapshot_state( state, result_amount_type::DMG_DIRECT );
    damage->schedule_execute( state );

    // 2nd moonfire executes first for normal moonfire, but 2nd for galactic guardian
    if ( twin && has_flag( flag_e::GALACTIC ) )
    {
      const auto& tl = target_list();
      if ( auto twin_target = p()->get_smart_target( tl, &druid_td_t::dots_t::moonfire, target, twin_range ) )
        twin->execute_on_target( twin_target );
    }

    p()->buff.galactic_guardian->consume( this );
  }
};

// Prowl ====================================================================
struct prowl_t final : public druid_spell_t
{
  DRUID_ABILITY( prowl_t, druid_spell_t, "prowl", p->find_class_spell( "Prowl" ) )
  {
    use_off_gcd = ignore_false_positive = true;
    harmful = false;

    action_flags |= flag_e::ALLOWSTEALTH;

    form_mask = CAT_FORM;
    autoshift = p->active.shift_to_cat;
  }

  void execute() override
  {
    if ( sim->log )
      sim->print_log( "{} performs {}", player->name(), name() );

    p()->buff.incarnation_cat_prowl->expire();
    p()->buff.prowl->trigger();

    druid_spell_t::execute();

    p()->cancel_auto_attacks();

    if ( !p()->in_boss_encounter )
      p()->leave_combat();
  }

  bool ready() override
  {
    if ( p()->buff.prowl->check() || ( p()->in_combat && !p()->buff.incarnation_cat_prowl->check() ) )
      return false;

    return druid_spell_t::ready();
  }
};

// Red Moon =================================================================
struct red_moon_t final : public druid_spell_t
{
  DRUID_ABILITY( red_moon_t, druid_spell_t, "red_moon", p->talent.red_moon )
  {
    dot_name = "red_moon";
  }
};

// Proxy Swipe Spell ========================================================
struct swipe_proxy_t final : public druid_spell_t
{
  cat_attacks::swipe_cat_t* swipe_cat;
  bear_attacks::swipe_bear_t* swipe_bear;

  swipe_proxy_t( druid_t* p )
    : druid_spell_t( "swipe", p, p->talent.swipe ),
      swipe_cat( new cat_attacks::swipe_cat_t( p ) ),
      swipe_bear( new bear_attacks::swipe_bear_t( p ) )
  {
    callbacks = false;

    set_school( SCHOOL_PHYSICAL );

    swipe_cat->action_flags |= flag_e::FOREGROUND;
    swipe_bear->action_flags |= flag_e::FOREGROUND;
  }

  void parse_options( util::string_view opt ) override
  {
    druid_spell_t::parse_options( opt );

    swipe_cat->parse_options( opt );
    swipe_bear->parse_options( opt );
  }

  timespan_t gcd() const override
  {
    switch ( p()->form )
    {
      case BEAR_FORM: return swipe_bear->gcd();
      case CAT_FORM:  return swipe_cat->gcd();
      default:        return druid_spell_t::gcd();
    }
  }

  void execute() override
  {
    switch ( p()->form )
    {
      case BEAR_FORM: swipe_bear->execute(); break;
      case CAT_FORM:  swipe_cat->execute();  break;
      default:        break;
    }

    if ( pre_execute_state )
      action_state_t::release( pre_execute_state );
  }

  bool action_ready() override
  {
    switch ( p()->form )
    {
      case BEAR_FORM: return swipe_bear->action_ready();
      case CAT_FORM:  return swipe_cat->action_ready();
      default:        return false;
    }
  }

  bool target_ready( player_t* t ) override
  {
    switch ( p()->form )
    {
      case BEAR_FORM: return swipe_bear->target_ready( t );
      case CAT_FORM:  return swipe_cat->target_ready( t );
      default:        return false;
    }
  }

  bool ready() override
  {
    switch ( p()->form )
    {
      case BEAR_FORM: return swipe_bear->ready();
      case CAT_FORM:  return swipe_cat->ready();
      default:        return false;
    }
  }

  double cost() const override
  {
    switch ( p()->form )
    {
      case BEAR_FORM: return swipe_bear->cost();
      case CAT_FORM:  return swipe_cat->cost();
      default:        return 0.0;
    }
  }
};

// Shooting Stars ===========================================================
struct shooting_stars_t : public druid_spell_t
{
  shooting_stars_t( druid_t* p, std::string_view n, const spell_data_t* s ) : druid_spell_t( n, p, s )
  {
    background = proc = true;

    auto m_data = p->get_modified_spell( &data() );
    set_energize( m_data );

    if ( const auto& eff = p->sets->set( DRUID_BALANCE, MID1, B2 )->effectN( 2 ); !energize->modified_by( eff ) )
    {
      energize->add_parse_entry()
        .set_func( [ p ]( const action_t*, const action_state_t* ) {
          return !p->buff.eclipse_solar->check() && !p->buff.eclipse_lunar->check();
        } )
        .set_flat( true )
        .set_value( eff.base_value() )
        .set_eff( &eff );
    }
  }

  void impact( action_state_t* s ) override
  {
    druid_spell_t::impact( s );

    if ( p()->buff.orbit_breaker->trigger() )
    {
      if ( p()->buff.orbit_breaker->at_max_stacks() && p()->active.orbit_breaker )
      {
        p()->active.orbit_breaker->execute_on_target( target );
        p()->buff.orbit_breaker->expire();
      }
    }
  }
};

struct shooting_stars_mid1_t final : public shooting_stars_t
{
  double aoe_coeff;

  shooting_stars_mid1_t( druid_t* p ) : shooting_stars_t( p, "shooting_stars_exploding", p->find_spell( 1272339 ) )
  {
    name_str_reporting = "Exploding";
    aoe = -1;

    // the aoe effect is parsed last and overwrites the st effect, so we need to cache the aoe coeff and re-parse the
    // st effect
    aoe_coeff = spell_power_mod.direct;
    parse_effect_direct_mods( data().effectN( 1 ), false );
  }

  double spell_direct_power_coefficient( const action_state_t* s ) const override
  {
    return s->chain_target == 0 ? shooting_stars_t::spell_direct_power_coefficient( s ) : aoe_coeff;
  }
};

// Skull Bash ===============================================================
struct skull_bash_t final : public use_fluid_form_t<CAT_FORM, druid_interrupt_t>
{
  DRUID_ABILITY( skull_bash_t, base_t, "skull_bash", p->talent.skull_bash ) {}

  bool target_ready( player_t* t ) override
  {
    // bypass druid_interrupt_t to allow usage with fluid form for offgcd shifting
    return druid_spell_t::target_ready( t );
  }
};

// Solar Beam ===============================================================
struct solar_beam_t final : public druid_interrupt_t
{
  DRUID_ABILITY( solar_beam_t, druid_interrupt_t, "solar_beam", p->talent.solar_beam )
  {
    base_costs[ RESOURCE_MANA ] = 0.0;  // remove mana cost so we don't need to enable mana regen

    // since simc interrupts only happen when the target is casting, it will always count as successful
    if ( p->talent.light_of_the_sun.ok() )
      cooldown->duration -= timespan_t::from_seconds( p->talent.light_of_the_sun->effectN( 1 ).base_value() );
  }
};

// Solar Bolt ===============================================================
struct solar_bolt_t final : public druid_spell_t
{
  solar_bolt_t( druid_t* p ) : druid_spell_t( "solar_bolt", p, p->find_spell( 1261573 ) )
  {
    proc = true;
    range = p->talent.ascendant_eclipses_3->effectN ( 1 ).base_value();
  }

  result_e calculate_result( action_state_t* ) const override
  {
    return result_e::RESULT_CRIT;
  }

  void execute() override
  {
    set_target( p()->target );

    druid_spell_t::execute();
  }
};

// Stampeding Roar ==========================================================
struct stampeding_roar_t final : public druid_spell_t
{
  DRUID_ABILITY( stampeding_roar_t, druid_spell_t, "stampeding_roar", p->talent.stampeding_roar )
  {
    harmful = false;

    form_mask = BEAR_FORM | CAT_FORM;
    autoshift = p->active.shift_to_bear;
  }

  void execute() override
  {
    druid_spell_t::execute();

    // TODO: add target eligibility via radius
    for ( const auto& actor : sim->player_non_sleeping_list )
      actor->buffs.stampeding_roar->trigger();
  }
};

// Starfall Spell ===========================================================
struct starfall_t final : public ap_spender_t
{
  struct starfall_damage_t final : public druid_spell_t
  {
    double ascendant_mul;

    starfall_damage_t( druid_t* p, std::string_view n, const spell_data_t* s, flag_e f )
      : druid_spell_t( n, p, s ? s : p->find_spell( 191037 ), f ),
        ascendant_mul( p->buff.ascendant_stars_starfall->data().effectN( 1 ).percent() )
    {
      background = proc = dual = true;
    }

    double action_multiplier() const override
    {
      auto am = druid_spell_t::action_multiplier();

      if ( !has_flag( flag_e::SYLVAN ) )
      {
        auto sf_mod = std::max( 1.0, as<double>( p()->buff.starfall->check() ) );
        auto asc_mod = ascendant_mul * p()->buff.ascendant_stars_starfall->check();

        am *= sf_mod + asc_mod;
      }

      return am;
    }
  };

  struct meteorites_t final : public druid_spell_t
  {
    meteorites_t( druid_t* p, std::string_view n, flag_e f ) : druid_spell_t( n, p, p->find_spell( 1240913 ), f )
    {
      proc = true;

      name_str_reporting = "meteorites";
    }

    // add fake travel time from starfall driver
    timespan_t travel_time() const override
    {
      // seems to have random discrete intervals. guesstimating at 66ms.
      return druid_spell_t::travel_time() + ( rng().range<int>( 14 ) + 1 ) * 66_ms;
    }
  };

  struct starfall_driver_t final : public druid_spell_t
  {
    starfall_damage_t* damage;
    meteorites_t* meteorites = nullptr;
    size_t num_meteorites;

    starfall_driver_t( druid_t* p, std::string_view n, const spell_data_t* s, const spell_data_t* damage_spell,
                       flag_e f )
      : druid_spell_t( n, p, s, f ), num_meteorites( as<size_t>( p->talent.meteorites->effectN( 1 ).base_value() ) )
    {
      background = proc = dual = true;

      auto pre = name_str.substr( 0, name_str.find_last_of( '_' ) );
      damage = p->get_secondary_action<starfall_damage_t>( pre + "_damage", damage_spell, f );

      if ( p->talent.meteorites.ok() && !has_flag( flag_e::SYLVAN ) )
        meteorites = p->get_secondary_action<meteorites_t>( pre + "_meteorite", f );
    }

    std::vector<player_t*>& target_list() const override
    {
      auto& tl = druid_spell_t::target_list();

      // randomize every tick
      rng().shuffle( tl.begin(), tl.end() );

      return tl;
    }

    util::span<player_t*> random_affected_target( size_t num ) const
    {
      auto& tl = druid_spell_t::target_list();

      if ( tl.empty() )
        return {};

      // list is already shuffled, just randomly rotate and take the first num targets
      std::rotate( tl.begin(), tl.begin() + rng().range( tl.size() ), tl.end() );

      return util::make_span( tl ).subspan( 0, std::min( num, tl.size() ) );
    }

    void execute() override
    {
      druid_spell_t::execute();

      if ( meteorites )
      {
        for ( auto _tar : random_affected_target( num_meteorites ) )
           meteorites->execute_on_target( _tar );
      }
    }

    // fake travel time to simulate execution delay for individual stars
    timespan_t travel_time() const override
    {
      // seems to have random discrete intervals. guesstimating at 66ms.
      return ( rng().range<int>( 14 ) + 1 ) * 66_ms;
    }

    void impact( action_state_t* s ) override
    {
      druid_spell_t::impact( s );

      snapshot_and_execute( damage, s, false, []( auto from, auto to ) { to->copy_state( from ); } );
    }
  };

  starfall_driver_t* driver;
  buff_t* buff;
  timespan_t dot_ext = 0_ms;
  timespan_t max_ext = 0_ms;

  DRUID_ABILITY_C( starfall_t, base_t, "starfall", p->spec.starfall, const spell_data_t* damage_spell = nullptr ),
    buff( p->buff.starfall ),
    dot_ext( timespan_t::from_seconds( p->talent.aetherial_kindling->effectN( 1 ).base_value() ) ),
    max_ext( timespan_t::from_seconds( p->talent.aetherial_kindling->effectN( 2 ).base_value() ) )
  {
    may_miss = false;
    queue_failed_proc = p->get_proc( "starfall queue failed" );
    dot_duration = 0_ms;
    form_mask |= NO_FORM;

    if ( data().ok() )
    {
      driver = p->get_secondary_action<starfall_driver_t>( name_str + "_driver", find_trigger( this ).trigger(),
                                                           damage_spell, f );
      assert( driver->damage );
      replace_stats( this, driver, false );
      replace_stats( this, driver->damage );
      if ( driver->meteorites )
        add_child( driver->meteorites );
    }

    weaver_buff = p->buff.starweaver_starfall;
  }

  void execute() override
  {
    if ( p()->talent.aetherial_kindling.ok() )
    {
      range::for_each( p()->dot_lists.moonfire, [ this ]( dot_t* d ) { d->adjust_duration( dot_ext, max_ext ); } );
      range::for_each( p()->dot_lists.sunfire, [ this ]( dot_t* d ) { d->adjust_duration( dot_ext, max_ext ); } );
    }

    base_t::execute();

    buff->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) { driver->execute(); } );
    buff->trigger();

    // technically triggered by buff application, do it in action so we can easily grab the driver targets
    if ( rng().roll( mid1_4pc_chance ) )
      p()->active.shooting_stars_mid1->execute_on_target( driver->random_affected_target( 1 ).front() );

    p()->buff.starweaver_starsurge->trigger( this );

    if ( p()->buff.ascendant_stars->consume( this ) )
      p()->buff.ascendant_stars_starfall->trigger();
  }

  void html_customsection( report::sc_html_stream& os ) override
  {
    // TODO: hack to not report DISABLED
    snapshot_flags |= STATE_TGT_MUL_DA;
    base_t::html_customsection( os );
  }
};

// Starfire =============================================================
struct starfire_t : public use_fluid_form_t<MOONKIN_FORM, ap_generator_t>
{
  DRUID_ABILITY( starfire_t, base_t, "starfire", p->talent.starfire )
  {
    aoe = -1;
    reduced_aoe_targets = data().effectN( p->specialization() == DRUID_BALANCE ? 5 : 3 ).base_value();

    // offspec starfire seems to do same splash as balance starfire
    if ( p->bugs )
      base_aoe_multiplier *= p->find_spell( 194153 )->effectN( 3 ).percent();
    else
      base_aoe_multiplier *= data().effectN( p->specialization() == DRUID_BALANCE ? 3 : 2 ).percent();

    base_aoe_multiplier /= 1.0 + find_effect( p->talent.lunar_calling, &data() ).percent();

    auto m_data = p->get_modified_spell( &data() );
    set_energize( m_data );

    // parse these last as they're percent bonus
    m_data->parse_effects( p->buff.eclipse_lunar );
    if ( p->talent.dryads_dance.ok() && p->talent.sylvan_beckoning.ok() )
      m_data->parse_effects( p->buff.sylvan_beckoning );

    touch_pct = p->talent.touch_the_cosmos->effectN( 2 ).percent();

    if ( p->talent.umbral_embrace.ok() )
    {
      // apply bonus from solar eclipse
      force_effect( p->spec.eclipse_solar, 1, [ this ] { return umbral_embrace_check(); } );
      // apply bonus from solar mastery
      force_effect( p->mastery.astral_invocation, 3, [ this ] { return umbral_embrace_check(); } );
    }

    if ( p->talent.master_shapeshifter.ok() )
    {
      const auto& eff = p->talent.master_shapeshifter->effectN( 2 );
      if ( !has_parse_entry( da_multiplier_effects, &eff ) )
      {
        add_parse_entry( da_multiplier_effects )
          .set_value( eff.percent() )
          .set_eff( &eff )
          .print_debug( this );
      }
    }
  }

  void init() override
  {
    base_t::init();

    // for precombat we hack it to manually energize 100ms later to get around AP capping on combat start
    if ( is_precombat && energize_resource_() == RESOURCE_ASTRAL_POWER )
      energize_type = action_energize::NONE;
  }

  void schedule_travel( action_state_t* s ) override
  {
    // for precombat we hack it to manually energize 100ms later to get around AP capping on combat start.
    if ( is_precombat && energize_resource_() == RESOURCE_ASTRAL_POWER && s->chain_target == 0 )
    {
      base_t::schedule_travel( s );

      make_event( *sim, 100_ms, [ this ]() {
        p()->resource_gain( RESOURCE_ASTRAL_POWER, composite_energize_amount( execute_state ),
                            energize_gain( execute_state ) );
      } );

      return;
    }

    base_t::schedule_travel( s );
  }

  void execute() override
  {
    base_t::execute();

    if ( !p()->buff.ascendant_fires->consume( this ) )
      if ( p()->buff.owlkin_frenzy->up() )
        p()->buff.owlkin_frenzy->expire();

    p()->buff.lunar_eclipse_override->trigger();
  }
};

// Starsurge ================================================================
struct starsurge_offspec_t final : public trigger_call_of_the_elder_druid_t<druid_spell_t>
{
  DRUID_ABILITY( starsurge_offspec_t, base_t, "starsurge", p->talent.starsurge )
  {
    form_mask = MOONKIN_FORM | NO_FORM;
    base_costs[ RESOURCE_MANA ] = 0.0;  // so we don't need to enable mana regen

    if ( p->talent.master_shapeshifter.ok() )
    {
      const auto& eff = p->talent.master_shapeshifter->effectN( 2 );
      if ( !has_parse_entry( da_multiplier_effects, &eff ) )
      {
        add_parse_entry( da_multiplier_effects )
          .set_value( eff.percent() )
          .set_eff( &eff )
          .print_debug( this );
      }
    }
  }
};

struct starsurge_cascade_t final : public druid_spell_t
{
  // hardcoded id as talent isn't necessary
  starsurge_cascade_t( druid_t* p )
    : druid_spell_t( "starsurge_cascade", p, p->find_spell( p->specialization() == DRUID_BALANCE ? 1271222 : 197626 ),
                     flag_e::CASCADE )
  {
    proc = true;
    name_str_reporting = "Cascade";

    base_costs[ RESOURCE_MANA ] = 0.0;
    internal_cooldown->duration = p->talent.star_cascade->internal_cooldown();

    base_multiplier = p->talent.star_cascade->effectN( p->specialization() == DRUID_BALANCE ? 3 : 4 ).percent();
  }

  void execute() override
  {
    druid_spell_t::execute();

    if ( p()->talent.hail_of_stars.ok() )
      p()->buff.solstice->extend_duration_or_trigger( HAIL_OF_STARS_FREE_DURATION );

    if ( p()->buff.eclipse_lunar->check() )
      debug_cast<buffs::eclipse_lunar_t*>( p()->buff.eclipse_lunar )->trigger_harmony();

    if ( p()->buff.eclipse_solar->check() )
      debug_cast<buffs::eclipse_solar_t*>( p()->buff.eclipse_solar )->trigger_harmony();

  }
};

struct starsurge_t final : public trigger_call_of_the_elder_druid_t<ap_spender_t>
{
  struct goldrinns_fang_t final : public druid_spell_t
  {
    goldrinns_fang_t( druid_t* p, std::string_view n, flag_e f )
      : druid_spell_t( n, p, find_trigger( p->talent.power_of_goldrinn ).trigger(), f )
    {
      background = proc = true;
      name_str_reporting = "goldrinns_fang";
    }
  };

  action_t* goldrinn = nullptr;
  bool moonkin_form_in_precombat = false;

  DRUID_ABILITY( starsurge_t, base_t, "starsurge", p->talent.starsurge )
  {
    form_mask |= NO_FORM; // spec version can be cast with no form despite spell data form mask

    if ( p->talent.power_of_goldrinn.ok() )
    {
      auto suf = get_suffix( name_str, "starsurge" );
      goldrinn = p->get_secondary_action<goldrinns_fang_t>( "goldrinns_fang" + suf, f );
      add_child( goldrinn );
    }

    if ( p->talent.master_shapeshifter.ok() )
    {
      const auto& eff = p->talent.master_shapeshifter->effectN( 2 );
      if ( !has_parse_entry( da_multiplier_effects, &eff ) )
      {
        add_parse_entry( da_multiplier_effects )
          .set_value( eff.percent() )
          .set_eff( &eff )
          .print_debug( this );
      }
    }

    weaver_buff = p->buff.starweaver_starsurge;
  }

  void init() override
  {
    base_t::init();

    if ( is_precombat )
    {
      moonkin_form_in_precombat = range::any_of( p()->precombat_action_list, []( action_t* a ) {
        return util::str_compare_ci( a->name(), "moonkin_form" );
      } );

      // hardcode travel time to 100ms
      travel_speed = 0.0;
      min_travel_time = 0.1;
    }
  }

  bool ready() override
  {
    if ( !is_precombat )
      return base_t::ready();

    // in precombat, so hijack standard ready() procedure
    // emulate performing check_form_restriction()
    if ( !moonkin_form_in_precombat )
      return false;

    // emulate performing resource_available( current_resource(), cost() )
    if ( !p()->talent.natures_balance.ok() && p()->options.initial_astral_power < cost() )
      return false;

    return true;
  }

  void execute() override
  {
    base_t::execute();

    if ( goldrinn && rng().roll( p()->talent.power_of_goldrinn->proc_chance() ) )
      goldrinn->execute_on_target( target );

    p()->buff.starweaver_starfall->trigger( this );
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( p()->talent.stellar_amplification.ok() )
      td( s->target )->debuff.stellar_amplification->trigger( this );

    if ( rng().roll( mid1_4pc_chance ) )
      p()->active.shooting_stars_mid1->execute_on_target( s->target );
  }
};

// Sunfire Spell ============================================================
struct sunfire_t final : public druid_spell_t
{
  struct sunfire_damage_t final : public use_dot_list_t<druid_spell_t>
  {
    real_ppm_t* shroom_rng = nullptr;

    sunfire_damage_t( druid_t* p, flag_e f ) : base_t( "sunfire_dmg", p, p->spec.sunfire_dmg, f )
    {
      dual = background = proc = true;
      aoe = -1;
      base_aoe_multiplier = 0;

      dot_name = "sunfire";
      dot_list = &p->dot_lists.sunfire;

      if ( p->talent.sunseeker_mushroom.ok() )
      {
        shroom_rng = p->get_rppm( "sunseeker_mushroom", p->talent.sunseeker_mushroom );
      }
    }

    void tick( dot_t* d ) override
    {
      base_t::tick( d );

      if ( shroom_rng && shroom_rng->trigger() )
        p()->active.sunseeker_mushroom->execute_on_target( d->target );
    }
  };

  action_t* damage;  // Add damage modifiers in sunfire_damage_t, not sunfire_t

  DRUID_ABILITY( sunfire_t, druid_spell_t, "sunfire", p->talent.sunfire )
  {
    if ( data().ok() )
    {
      damage = p->get_secondary_action<sunfire_damage_t>( "sunfire_dmg", f );
      replace_stats( this, damage );
    }
  }

  // needed to allow on-cast procs
  bool has_amount_result() const override { return damage->has_amount_result(); }

  void execute() override
  {
    druid_spell_t::execute();

    // as we use a single secondary action for all instances of sunfire invalidate the damage action cache whenever it
    // doesn't match the current proxy action target.
    if ( damage->target != target )
      damage->target_cache.is_valid = false;

    auto state = damage->get_state();
    damage->target = state->target = target;
    damage->snapshot_state( state, result_amount_type::DMG_DIRECT );
    damage->schedule_execute( state );
  }
};

// Survival Instincts =======================================================
struct survival_instincts_t final : public druid_spell_t
{
  DRUID_ABILITY( survival_instincts_t, druid_spell_t, "survival_instincts", p->talent.survival_instincts )
  {
    harmful     = false;
    use_off_gcd = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.survival_instincts->trigger();

    p()->buff.matted_fur->trigger();
  }
};

// Orbital Strike ===========================================================
struct orbital_strike_t final : public druid_spell_t
{
  struct stellar_flare_t final : public druid_spell_t
  {
    stellar_flare_t( druid_t* p ) : druid_spell_t( "stellar_flare", p, p->find_spell( 202347 ) )
    {
      dot_name = "stellar_flare";
    }
  };

  action_t* flare;

  orbital_strike_t( druid_t* p ) : druid_spell_t( "orbital_strike", p, p->find_spell( 361237 ) )
  {
    proc = true;
    aoe = -1;
    travel_speed = 75.0;  // guesstimate

    flare = p->get_secondary_action<stellar_flare_t>( "stellar_flare" );
    flare->proc = true;
    add_child( flare );
  }

  void impact( action_state_t* s ) override
  {
    flare->execute_on_target( s->target );  // flare is applied before impact damage

    druid_spell_t::impact( s );
  }
};

// Wild Charge ==============================================================
struct wild_charge_t final : public druid_spell_t
{
  double movement_speed_increase = 5.0;

  DRUID_ABILITY( wild_charge_t, druid_spell_t, "wild_charge", p->talent.wild_charge )
  {
    harmful = may_miss = false;
    ignore_false_positive = true;
    range = data().max_range();
    movement_directionality = movement_direction_type::OMNI;
    trigger_gcd = 0_ms;
  }

  void execute() override
  {
    auto dur = timespan_t::from_seconds( p()->current.distance_to_move / p()->composite_movement_speed() );

    p()->buff.wild_charge_movement->trigger( 1, movement_speed_increase, 1, dur );

    druid_spell_t::execute();
  }

  bool ready() override
  {
    if ( p()->current.distance_to_move < data().min_range() )  // Cannot charge if the target is too close.
      return false;

    return druid_spell_t::ready();
  }

  void update_ready( timespan_t cd ) override
  {
    if ( p()->talent.elunes_grace.ok() && ( p()->form == BEAR_FORM || p()->form == MOONKIN_FORM ) )
      cd = cooldown_duration() - 3_s;

    druid_spell_t::update_ready( cd );
  }
};

// Wild Guardian ============================================================
struct wild_guardian_t final : public druid_spell_t
{
  int dream_charges;

  DRUID_ABILITY( wild_guardian_t, druid_spell_t, "wild_guardian",
                 p->talent.wild_guardian_1.ok() ? p->find_spell( 1269658 ) : spell_data_t::not_found() ),
    dream_charges( as<int>( p->talent.wild_guardian_2->effectN( 2 ).base_value() ) )
  {}

  bool ready() override
  {
    if ( !p()->buff.wild_guardian->check() )
      return false;

    return druid_spell_t::ready();
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.wild_guardian->expire();
    p()->buff.gift_of_frenzied_regeneration->trigger();
    p()->buff.gift_of_ironfur->trigger();
    p()->buff.gift_of_maul->trigger();

    if ( dream_charges )
    {
      p()->buff.dream_guide->trigger( dream_charges );
      p()->buff.dream_of_cenarius->trigger( dream_charges );
    }
  }
};

// Wild Mushroom ============================================================
struct wild_mushroom_t final : public druid_spell_t
{
  struct fungal_growth_t final : public druid_spell_t
  {
    uptime_t* uptime;

    fungal_growth_t( druid_t* p, std::string_view n, const spell_data_t* s )
      : druid_spell_t( n, p, s ), uptime( p->get_uptime( "Fungal Growth" ) )
    {
      background = proc = true;

      name_str_reporting = "fungal_growth";
      dot_name = "fungal_growth";
    }

    void trigger_dot( action_state_t* s ) override
    {
      druid_spell_t::trigger_dot( s );

      uptime->update( true, sim->current_time() );
    }

    void last_tick( dot_t* d ) override
    {
      druid_spell_t::last_tick( d );

      uptime->update( false, sim->current_time() );
    }
  };

  struct wild_mushroom_damage_t final : public druid_spell_t
  {
    double ap_max;

    wild_mushroom_damage_t( druid_t* p, std::string_view n, const spell_data_t* s, const spell_data_t* sd, flag_e f )
      : druid_spell_t( n, p, s, f ), ap_max( data().effectN( 2 ).base_value() )
    {
      background = proc = dual = true;
      aoe = -1;

      // TODO: determine if AP per hit is affected by dryad's dance + sylvan beckoning

      if ( !p->active.fungal_growth )
        p->active.fungal_growth = p->get_secondary_action<fungal_growth_t>( "fungal_growth", sd );
    }

    double ap_gain( int targets_hit ) const
    {
      return std::min( ap_max, ( targets_hit + 1 ) * WILD_MUSHROOM_AP_PER_HIT );
    }

    void execute() override
    {
      druid_spell_t::execute();

      gain_energize_resource( RESOURCE_ASTRAL_POWER, ap_gain( num_targets_hit ), gain );
    }

    void impact( action_state_t* s ) override
    {
      druid_spell_t::impact( s );

      p()->active.fungal_growth->execute_on_target( s->target );
    }
  };

  ground_aoe_params_t params;
  const spell_data_t* dot_spell;
  wild_mushroom_damage_t* damage = nullptr;
  timespan_t delay;

  DRUID_ABILITY_C( wild_mushroom_t, druid_spell_t, "wild_mushroom", p->talent.wild_mushroom,
                   const spell_data_t* sd = nullptr ),
    dot_spell( sd ? sd : p->find_spell( 81281 ) ),
    delay( timespan_t::from_millis( find_trigger( this ).misc_value1() ) )
  {
    harmful = false;

    if ( data().ok() )
    {
      damage = p->get_secondary_action<wild_mushroom_damage_t>( name_str + "_damage", find_trigger( this ).trigger(),
                                                                dot_spell, f );
      replace_stats( this, damage );
      damage->gain = gain;

      params.pulse_time( delay )
        .duration( delay )
        .action( damage );
    }
  }

  void execute() override
  {
    druid_spell_t::execute();

    params.start_time( timespan_t::min() )  // reset start time
      .target( target )
      .x( target->x_position )
      .y( target->y_position );

    make_event<ground_aoe_event_t>( *sim, p(), params );
  }

  std::unique_ptr<expr_t> create_expression( std::string_view name ) override
  {
    if ( name == "energize_amount" )
    {
      if ( !damage )
        return expr_t::create_constant( name, 0.0 );

      auto spell_targets = damage->create_expression( "spell_targets" );

      return make_fn_expr( name, [ this, spell_targets = std::move( spell_targets ) ] {
        return damage->ap_gain( spell_targets->evaluate() );
      } );
    }

    return druid_spell_t::create_expression( name );
  }
};

// Wrath ====================================================================
struct wrath_t : public use_fluid_form_t<MOONKIN_FORM, ap_generator_t>
{
  unsigned count = 0;

  DRUID_ABILITY( wrath_t, base_t, "wrath", p->spec.wrath )
  {
    auto m_data = p->get_modified_spell( &data() );
    set_energize( m_data );

    if ( energize && p->talent.dryads_dance.ok() && p->talent.sylvan_beckoning.ok() )
    {
      // dryad's dance causes sylvan beckoning to affect astral power (197911#2) which sets wrath energize value
      // (190984#2) so we need to adjust
      if ( const auto& eff = find_effect( p->buff.sylvan_beckoning, p->spec.astral_power );
           !energize->modified_by( eff ) )
      {
        energize->add_parse_entry()
          .set_buff( p->buff.sylvan_beckoning )
          .set_flat( true )
          .set_value( p->spec.astral_power->effectN( 2 ).base_value() * eff.percent() )
          .set_eff( &eff );
      }
    }

    // parse these last as they're percent bonus
    m_data->parse_effects( p->buff.eclipse_solar );
    if ( p->talent.dryads_dance.ok() && p->talent.sylvan_beckoning.ok() )
      m_data->parse_effects( p->buff.sylvan_beckoning );

    touch_pct = p->talent.touch_the_cosmos->effectN( 1 ).percent();

    if ( p->talent.umbral_embrace.ok() )
    {
      // apply bonus from lunar eclipse
      force_effect( p->spec.eclipse_lunar, 1, [ this ] { return umbral_embrace_check(); } );
      // apply bonus from lunra mastery
      force_effect( p->mastery.astral_invocation, 1, [ this ] { return umbral_embrace_check(); } );
    }

    if ( p->talent.master_shapeshifter.ok() )
    {
      const auto& eff = p->talent.master_shapeshifter->effectN( 2 );
      if ( !has_parse_entry( da_multiplier_effects, &eff ) )
      {
        add_parse_entry( da_multiplier_effects )
          .set_value( eff.percent() )
          .set_eff( &eff )
          .print_debug( this );
      }
    }

    if ( p->talent.arcane_affinity.ok() )
    {
      // TODO: arcane affinity seems to be bugged and applies only to the maximum damage of non-UE wrath.
      // as simc only looks at the average, apply a half strength modifier.
      parse_effects( p->talent.arcane_affinity, p->talent.arcane_affinity->effectN( 1 ).percent() * 0.5,
        [ this ] { return !umbral_embrace_check(); } );

      // arcane affinity applies fully to umbral embrace'd wrath
      if ( p->talent.umbral_embrace.ok() )
        parse_effects( p->talent.arcane_affinity, [ this ] { return umbral_embrace_check(); } );
    }
  }

  timespan_t travel_time() const override
  {
    if ( !count )
      return base_t::travel_time();

    // for each additional wrath in precombat apl, reduce the travel time by the cast time
    player->invalidate_cache( CACHE_SPELL_HASTE );
    return std::max( 1_ms, base_t::travel_time() - base_execute_time * composite_haste() * count );
  }

  void schedule_travel( action_state_t* s ) override
  {
    // in druid_t::init_finished(), we set the final wrath of the precombat to have energize type of NONE, so that
    // we can handle the delayed enerigze & eclipse stack triggering here.
    if ( is_precombat && energize_resource_() == RESOURCE_ASTRAL_POWER && energize_type == action_energize::NONE )
    {
      base_t::schedule_travel( s );

      make_event( *sim, 100_ms, [ this ]() {
        p()->resource_gain( RESOURCE_ASTRAL_POWER, composite_energize_amount( execute_state ),
                            energize_gain( execute_state ) );
      } );

      return;
    }

    base_t::schedule_travel( s );
  }

  void execute() override
  {
    base_t::execute();

    p()->buff.ascendant_fires->consume( this );

    if ( !p()->talent.lunar_calling.ok() )
      p()->buff.lunar_eclipse_override->expire();
  }
};

// Heart of the Wild ========================================================
struct heart_of_the_wild_t final : public druid_spell_t
{
  action_t* hotw_cat;
  action_t* hotw_owl;

  DRUID_ABILITY( heart_of_the_wild_t, druid_spell_t, "heart_of_the_wild", p->talent.heart_of_the_wild )
  {
    harmful = may_miss = reset_melee_swing = callbacks = false;

    // validate all version of hotw have the same cd
    assert( p->buff.heart_of_the_wild_bear->is_fallback ||
            cooldown->duration == p->buff.heart_of_the_wild_bear->data().cooldown() );
    assert( !p->active.hotw_cat || cooldown->duration == p->active.hotw_cat->data().cooldown() );
    assert( !p->active.hotw_owl || cooldown->duration == p->active.hotw_owl->data().cooldown() );
  }

  void init() override
  {
    druid_spell_t::init();

    if ( p()->active.hotw_cat )
      p()->active.hotw_cat->tick_action->gain = p()->active.hotw_cat->gain;
  }

  void execute() override
  {
    druid_spell_t::execute();

    switch( p()->form )
    {
      case BEAR_FORM:
        if( !p()->buff.heart_of_the_wild_bear->is_fallback )
          p()->buff.heart_of_the_wild_bear->trigger();
        break;

      case CAT_FORM:
        if ( p()->active.hotw_cat )
          p()->active.hotw_cat->execute_on_target( target );
        break;

      case MOONKIN_FORM:
        if ( p()->active.hotw_owl )
        {
          p()->active.hotw_owl->stats->add_execute( 0_ms, target );
          p()->buff.heart_of_the_wild_owl->trigger();
        }
        break;

      default: break;
    }
  }
};

// Convoke the Spirits ======================================================
// NOTE must be defined after all other spells
struct convoke_the_spirits_t final : public trigger_control_of_the_dream_t<druid_spell_t>
{
  enum convoke_cast_e
  {
    CAST_NONE = 0,
    CAST_OFFSPEC,
    CAST_SPEC,
    CAST_EXCEPTIONAL,
    CAST_HEAL,
    CAST_MAIN,
    CAST_WRATH,
    CAST_MOONFIRE,
    CAST_RAKE,
    CAST_THRASH,
    CAST_FULL_MOON,
    CAST_STARSURGE,
    CAST_STARFALL,
    CAST_MAUL,
    CAST_IRONFUR,
    CAST_MANGLE,
    CAST_FERAL_FRENZY,
    CAST_FEROCIOUS_BITE,
    CAST_SHRED
  };

  struct
  {
    // Multi-spec
    action_t* conv_wrath;
    action_t* conv_moonfire;
    action_t* conv_rake;
    action_t* conv_thrash;
    // Moonkin Form
    action_t* conv_full_moon;
    action_t* conv_starsurge;
    action_t* conv_starfall;
    // Bear Form
    action_t* conv_ironfur;
    action_t* conv_mangle;
    action_t* conv_maul;
    // Cat Form
    action_t* conv_feral_frenzy;
    action_t* conv_ferocious_bite;
    action_t* conv_shred;
    action_t* conv_lunar_inspiration;
    // Heals
    action_t* conv_regrowth;
    action_t* conv_rejuvenation;
  } actions;

  std::vector<convoke_cast_e> cast_list;
  std::vector<std::pair<convoke_cast_e, double>> chances;
  std::vector<std::pair<convoke_cast_e, double>> offspec;

  int max_ticks = 0;
  unsigned main_count = 0;
  unsigned filler_count = 0;
  unsigned dot_count = 0;
  unsigned off_count = 0;
  bool guidance;

  DRUID_ABILITY( convoke_the_spirits_t, base_t, "convoke_the_spirits", p->talent.convoke_the_spirits ),
    actions(),
    guidance( p->talent.elunes_guidance.ok() || p->talent.ursocs_guidance.ok() ||
              p->talent.ashamanes_guidance.ok() || p->talent.cenarius_guidance.ok() )
  {
    if ( !p->talent.convoke_the_spirits.ok() )
      return;

    channeled = true;
    harmful = may_miss = false;

    max_ticks = as<int>( util::floor( dot_duration / base_tick_time ) );
  }

  void init() override
  {
    base_t::init();

    if ( !data().ok() )
      return;

    using namespace bear_attacks;
    using namespace cat_attacks;
    using namespace heals;

    // Create actions used by all specs
    actions.conv_wrath        = get_convoke_action<wrath_t>( "wrath" );
    actions.conv_rake         = get_convoke_action<rake_t>( "rake", p()->find_spell( 1822 ) );
    actions.conv_thrash       = get_convoke_action<thrash_t>( "thrash" );
    actions.conv_regrowth     = get_convoke_action<regrowth_t>( "regrowth" );
    actions.conv_rejuvenation = get_convoke_action<rejuvenation_t>( "rejuvenation", p()->find_spell( 774 ) );

    if ( p()->talent.red_moon.ok() )
      actions.conv_moonfire   = get_convoke_action<red_moon_t>( "red_moon" );
    else
      actions.conv_moonfire   = get_convoke_action<moonfire_t>( "moonfire" );

    // Call form-specific initialization to create necessary actions & setup variables
    if ( p()->uses_moonkin_form() )
      _init_moonkin();

    if ( p()->uses_bear_form() )
      _init_bear();

    if ( p()->uses_cat_form() )
      _init_cat();
  }

  template <typename T>
  T* get_convoke_action( std::string n, const spell_data_t* s = nullptr )
  {
    auto a = p()->get_secondary_action<T>( n + "_convoke", s, flag_e::CONVOKE );
    if ( a->name_str_reporting.empty() )
      a->name_str_reporting = n;

    stats->add_child( a->stats );
    a->gain = gain;
    a->proc = true;
    a->trigger_gcd = 0_ms;  // prevent schedule_ready() fuzziness being added to execute time stat
    // get_convoke_action is called in init() so newly created actions need to be init'd
    a->init();
    return a;
  }

  action_t* convoke_action_from_type( convoke_cast_e conv_type )
  {
    switch ( conv_type )
    {
      case CAST_MOONFIRE:
        if ( p()->form == CAT_FORM )
          return actions.conv_lunar_inspiration;
        else
          return actions.conv_moonfire;
      case CAST_WRATH:          return actions.conv_wrath;
      case CAST_RAKE:           return actions.conv_rake;
      case CAST_THRASH:         return actions.conv_thrash;
      case CAST_FULL_MOON:      return actions.conv_full_moon;
      case CAST_STARSURGE:      return actions.conv_starsurge;
      case CAST_STARFALL:       return actions.conv_starfall;
      case CAST_MAUL:           return actions.conv_maul;
      case CAST_IRONFUR:        return actions.conv_ironfur;
      case CAST_MANGLE:         return actions.conv_mangle;
      case CAST_FERAL_FRENZY:   return actions.conv_feral_frenzy;
      case CAST_FEROCIOUS_BITE: return actions.conv_ferocious_bite;
      case CAST_SHRED:          return actions.conv_shred;
      case CAST_HEAL:           return rng().roll( 0.5 ) ? actions.conv_regrowth : actions.conv_rejuvenation;
      default: return nullptr;
    }
  }

  convoke_cast_e get_cast_from_dist( std::vector<std::pair<convoke_cast_e, double>> dist )
  {
    auto _sum = []( double a, std::pair<convoke_cast_e, double> b ) { return a + b.second; };

    auto roll = rng().range( 0.0, std::accumulate( dist.begin(), dist.end(), 0.0, _sum ) );

    for ( auto it = dist.begin(); it != dist.end(); it++ )
      if ( roll < std::accumulate( dist.begin(), it + 1, 0.0, _sum ) )
        return it->first;

    return CAST_NONE;
  }

  double composite_haste() const override { return 1.0; }

  void _init_cat()
  {
    using namespace cat_attacks;

    if ( p()->talent.frantic_frenzy.ok() )
      actions.conv_feral_frenzy    = get_convoke_action<frantic_frenzy_t>( "frantic_frenzy", p()->find_spell( 1243807 ) );
    else
      actions.conv_feral_frenzy    = get_convoke_action<feral_frenzy_t>( "feral_frenzy", p()->find_spell( 274837 ) );

    actions.conv_ferocious_bite    = get_convoke_action<ferocious_bite_t>( "ferocious_bite" );
    actions.conv_shred             = get_convoke_action<shred_t>( "shred" );
    actions.conv_lunar_inspiration = get_convoke_action<lunar_inspiration_t>( "lunar_inspiration", p()->find_spell( 155625 ) );
    // find by id since talent is not required
    actions.conv_lunar_inspiration->s_data_reporting = p()->find_spell( 155580 );

  }

  void _init_moonkin()
  {
    actions.conv_full_moon = get_convoke_action<full_moon_t>( "full_moon", p()->find_spell( 274283 ) );
    actions.conv_starfall  = get_convoke_action<starfall_t>( "starfall", p()->find_spell( 191034 ) );
    actions.conv_starsurge = get_convoke_action<starsurge_t>( "starsurge", p()->find_spell( 78674 ) );
  }

  void _init_bear()
  {
    using namespace bear_attacks;

    actions.conv_ironfur   = get_convoke_action<ironfur_t>( "ironfur", p()->find_spell( 192081 ) );
    actions.conv_mangle    = get_convoke_action<mangle_t>( "mangle" );

    if ( p()->talent.raze.ok() )
      actions.conv_maul    = get_convoke_action<raze_t>( "raze", p()->find_spell( 400254 ) );
    else
      actions.conv_maul    = get_convoke_action<maul_t>( "maul", p()->find_spell( 6807 ) );
  }

  void insert_exceptional()
  {
    assert( p()->rngs.convoke );

    if ( !p()->rngs.convoke->trigger() )
      return;

    // Restoration with CS (only 9 ticks) seem to only succeed ~85% of the time
    if ( max_ticks <= 9 && !rng().roll( p()->options.cenarius_guidance_exceptional_chance ) )
      return;

    cast_list.push_back( CAST_EXCEPTIONAL );
  }

  void _execute_bear()
  {
    main_count   = 0;

    chances = { { CAST_THRASH, guidance ? 0.85 : 0.95 },
                { CAST_IRONFUR, 1.0 },
                { CAST_MANGLE, 1.0 } };
    offspec = { { CAST_HEAL, 0.5 },
                { CAST_RAKE, 0.25 },
                { CAST_WRATH, 0.25 } };

    cast_list.insert( cast_list.end(),
                      static_cast<int>( rng().range( guidance ? 3.5 : 5, guidance ? 6 : 7 ) ),
                      CAST_OFFSPEC );

    insert_exceptional();
  }

  convoke_cast_e _tick_bear( convoke_cast_e base_type, const std::vector<player_t*>& tl, player_t*& conv_tar )
  {
    convoke_cast_e type_ = base_type == CAST_EXCEPTIONAL ? CAST_MAUL : base_type;

    if ( type_ == CAST_OFFSPEC )
    {
      type_ = get_cast_from_dist( offspec );
    }
    else if ( type_ == CAST_SPEC )
    {
      auto dist = chances;  // local copy of distribution

      std::vector<player_t*> mf_tl;  // separate list for mf targets without a dot;
      for ( const auto& t : tl )
        if ( !td( t )->dots.moonfire->is_ticking() )
          mf_tl.push_back( t );

      if ( !mf_tl.empty() )
        dist.emplace_back( CAST_MOONFIRE, ( main_count ? 0.25 : 1.0 ) / ( guidance ? 2.0 : 1.0 ) );

      type_ = get_cast_from_dist( dist );

      if ( type_ == CAST_MOONFIRE )
        conv_tar = rng().range( mf_tl );  // mf has it's own target list
    }

    if ( !conv_tar )
      conv_tar = rng().range( tl );

    if ( type_ == CAST_RAKE && td( conv_tar )->dots.rake->is_ticking() )
      type_ = CAST_WRATH;

    if ( type_ == CAST_MOONFIRE )
      main_count++;

    return type_;
  }

  void _execute_cat()
  {
    chances = { { CAST_SHRED, 0.10 },
                { CAST_RAKE,  0.22 } };
    offspec = { { CAST_HEAL, 0.35 },
                { CAST_MOONFIRE, 0.5 },
                { CAST_WRATH, 0.15 } };

    cast_list.insert( cast_list.end(),
                      static_cast<size_t>( rng().range( guidance ? 2.5 : 4, guidance ? 7.5 : 9 ) ),
                      CAST_OFFSPEC );

    insert_exceptional();

    auto bites = rng().gauss_ab( guidance ? 3 : 4.2, guidance ? 0.8 : 0.9360890055, 0.0,
                                 as<double>( max_ticks - cast_list.size() ) );

    cast_list.insert( cast_list.end(), static_cast<size_t>( bites ), CAST_MAIN );
  }

  convoke_cast_e _tick_cat( convoke_cast_e base_type, const std::vector<player_t*>& tl, player_t*& conv_tar )
  {
    convoke_cast_e type_ = base_type == CAST_EXCEPTIONAL ? CAST_FERAL_FRENZY : base_type;

    if ( type_ == CAST_OFFSPEC )
    {
      type_ = get_cast_from_dist( offspec );
    }
    else if ( type_ == CAST_MAIN )
    {
      type_ = CAST_FEROCIOUS_BITE;
      conv_tar = p()->target;
    }
    else if ( type_ == CAST_SPEC )
    {
      type_ = get_cast_from_dist( chances );
    }
    else if ( type_ == CAST_FERAL_FRENZY )
    {
      conv_tar = p()->target;
    }

    if ( !conv_tar )
      conv_tar = rng().range( tl );

    auto target_data = td( conv_tar );

    if ( type_ == CAST_MOONFIRE && target_data->dots.lunar_inspiration->is_ticking() )
      type_ = CAST_WRATH;
    else if ( type_ == CAST_RAKE && target_data->dots.rake->is_ticking() )
      type_ = CAST_SHRED;

    return type_;
  }

  void _execute_moonkin()
  {
    cast_list.insert( cast_list.end(), 5 - ( guidance ? 1 : 0 ), CAST_HEAL );
    off_count    = 0;
    main_count   = 0;
    dot_count    = 0;
    filler_count = 0;

    insert_exceptional();
  }

  convoke_cast_e _tick_moonkin( convoke_cast_e base_type, const std::vector<player_t*>& tl, player_t*& conv_tar )
  {
    convoke_cast_e type_ = base_type == CAST_EXCEPTIONAL ? CAST_FULL_MOON : base_type;
    std::vector<std::pair<convoke_cast_e, double>> dist;
    unsigned adjust = guidance ? 1 : 0;

    if ( type_ == CAST_SPEC )
    {
      bool add_more = true;

      if ( !p()->buff.starfall->check() )
      {
        dist.emplace_back( CAST_STARFALL, 3 + adjust );
        add_more = false;
      }

      std::vector<player_t*> mf_tl;  // separate list for mf targets without a dot;
      for ( const auto& t : tl )
        if ( !td( t )->dots.moonfire->is_ticking() )
          mf_tl.push_back( t );

      if ( !mf_tl.empty() )
      {
        if      ( dot_count < ( 4 - adjust ) ) dist.emplace_back( CAST_MOONFIRE, 5.5 );
        else if ( dot_count < ( 5 - adjust ) ) dist.emplace_back( CAST_MOONFIRE, 1.0 );
      }

      if ( add_more )
      {
        if      ( filler_count < ( 3 - adjust ) ) dist.emplace_back( CAST_WRATH, 5.5 );
        else if ( filler_count < ( 4 - adjust ) ) dist.emplace_back( CAST_WRATH, 3.5 );
        else if ( filler_count < ( 5 - adjust ) ) dist.emplace_back( CAST_WRATH, 1.0 );
      }

      if      ( main_count < 3 - adjust ) dist.emplace_back( CAST_STARSURGE, 6.0 );
      else if ( main_count < 4 - adjust ) dist.emplace_back( CAST_STARSURGE, 3.0 );
      else if ( main_count < 5 - adjust ) dist.emplace_back( CAST_STARSURGE, 1.0 );
      else if ( main_count < 6 - adjust ) dist.emplace_back( CAST_STARSURGE, 0.5 );

      if      ( filler_count < ( 4 - adjust ) ) dist.emplace_back( CAST_WRATH, 4.0 );
      else if ( filler_count < ( 5 - adjust ) ) dist.emplace_back( CAST_WRATH, 2.0 );
      else if ( filler_count < ( 6 - adjust ) ) dist.emplace_back( CAST_WRATH, 1.0 );
      else if ( filler_count < ( 7 - adjust ) ) dist.emplace_back( CAST_WRATH, 0.2 );

      if      ( off_count < ( 6 - adjust ) ) dist.emplace_back( CAST_HEAL, 0.8 );
      else if ( off_count < ( 7 - adjust ) ) dist.emplace_back( CAST_HEAL, 0.4 );

      type_ = get_cast_from_dist( dist );

      if ( type_ == CAST_STARSURGE )
        conv_tar = p()->target;
      else if ( type_ == CAST_MOONFIRE )
        conv_tar = rng().range( mf_tl );
    }
    else if ( type_ == CAST_FULL_MOON )
    {
      conv_tar = p()->target;
    }

    if ( !conv_tar )
      conv_tar = rng().range( tl );

    if ( type_ == CAST_STARSURGE )
      main_count++;
    else if ( type_ == CAST_WRATH )
      filler_count++;
    else if ( type_ == CAST_MOONFIRE )
      dot_count++;
    else if ( type_ == CAST_HEAL )
      off_count++;

    return type_;
  }

  void execute() override
  {
    // Generic routine
    base_t::execute();

    cast_list.clear();
    main_count = 0;

    // form-specific execute setup
    switch ( p()->form )
    {
      case BEAR_FORM:    _execute_bear();    break;
      case CAT_FORM:     _execute_cat();     break;
      case MOONKIN_FORM: _execute_moonkin(); break;
      default:           break;
    }

    cast_list.insert( cast_list.end(), max_ticks - cast_list.size(), CAST_SPEC );
  }

  void tick( dot_t* d ) override
  {
    base_t::tick( d );

    // The last partial tick does nothing
    if ( d->time_to_tick() < base_tick_time )
      return;

    // form-agnostic
    action_t* conv_cast = nullptr;
    player_t* conv_tar  = nullptr;

    // pick random spell and remove it
    std::swap( rng().range( cast_list ), cast_list.back() );
    auto conv_type = cast_list.back();
    cast_list.pop_back();

    std::vector<player_t*> tl = target_list();
    if ( tl.empty() )
      return;

    // Do form-specific spell selection
    switch ( p()->form )
    {
      case BEAR_FORM:
        conv_type = _tick_bear( conv_type, tl, conv_tar );
        break;
      case CAT_FORM:
        conv_type = _tick_cat( conv_type, tl, conv_tar );
        break;
      case MOONKIN_FORM:
        conv_type = _tick_moonkin( conv_type, tl, conv_tar );
        break;
      default: break;
    }

    conv_cast = convoke_action_from_type( conv_type );
    if ( !conv_cast )
      return;

    if ( conv_type == convoke_cast_e::CAST_HEAL )
    {
      const auto& heal_tl = conv_cast->target_list();
      conv_tar = rng().range( heal_tl );
    }

    conv_cast->execute_on_target( conv_tar );
  }

  bool usable_moving() const override { return true; }
};
}  // end namespace spells

#undef DRUID_ABILITY
#undef DRUID_ABILITY_B
#undef DRUID_ABILITY_C
#undef DRUID_ABILITY_D

namespace pets
{
void dread_shade_t::arise()
{
  pet_t::arise();

  waking_nightmare->trigger();
}

void dread_shade_t::create_buffs()
{
  pet_t::create_buffs();

  waking_nightmare = make_buff( actor_pair_t( this, owner ), "waking_nightmare", find_spell( 1253462 ) )
    ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
      o()->active.waking_nightmare_pulse->execute();
    } );
}

void dread_shade_t::create_actions()
{
  pet_t::create_actions();

  dire_echo = new dire_echo_t( this );
}

void sylvan_beckoning_t::arise()
{
  pet_t::arise();

  starsurge_count = 0;
  starfall_buff->trigger();
}

void sylvan_beckoning_t::create_buffs()
{
  pet_t::create_buffs();

  starfall_buff = make_buff( actor_pair_t( this, owner ), "starfall", o()->spec.sylvan_beckoning_sf )
    ->set_freeze_stacks( true )
    ->set_tick_callback( [ this ]( auto, auto, auto ) {
      o()->active.sylvan_beckoning_starfall_driver->execute();
    } );
}

action_t* sylvan_beckoning_t::create_action( std::string_view n, std::string_view opt )
{
  {
    if ( n == "starsurge" )
      return new starsurge_t( this );

    return pet_t::create_action( n, opt );
  }
}

void treant_base_t::arise()
{
  pet_t::arise();

  o()->buff.harmony_of_the_grove->trigger();

  if ( !o()->buff.treants_of_the_moon->is_fallback )
    static_cast<buffs::treants_of_the_moon_buff_t*>( o()->buff.treants_of_the_moon )->data.insert( this );
}

void treant_base_t::demise()
{
  pet_t::demise();

  o()->buff.harmony_of_the_grove->decrement();

  if ( !o()->buff.treants_of_the_moon->is_fallback )
    static_cast<buffs::treants_of_the_moon_buff_t*>( o()->buff.treants_of_the_moon )->data.erase( this );
}
}  // namespace pets

namespace auto_attacks
{
template <typename Base>
struct druid_melee_t : public Base
{
private:
  using ab = Base;
  buff_t* ravage_buff = nullptr;
  proc_t* ravage_proc = nullptr;
  double ooc_chance = 0.0;

protected:
  using base_t = druid_melee_t<Base>;

public:
  druid_melee_t( std::string_view n, druid_t* p ) : Base( n, p, spell_data_t::nil(), flag_e::AUTOATTACK )
  {
    ab::may_crit = ab::background = ab::repeating = true;
    ab::allow_class_ability_procs = ab::not_a_proc = true;
    ab::school = SCHOOL_PHYSICAL;
    ab::trigger_gcd = 0_ms;
    ab::special = false;
    ab::weapon_multiplier = 1.0;

    p->parse_action_effects( this );
    p->parse_action_target_effects( this );

    // Auto attack mods
    ab::parse_effects( p->buff.tigers_fury );

    // 7.00 PPM via community testing (~368k auto attacks)
    // https://docs.google.com/spreadsheets/d/1vMvlq1k3aAuwC1iHyDjqAneojPZusdwkZGmatuWWZWs/edit#gid=1097586165
    if ( p->talent.omen_of_clarity_cat.ok() )
      ooc_chance = 7.00;

    if ( p->talent.moment_of_clarity.ok() )
      ooc_chance *= 1.0 + p->talent.moment_of_clarity->effectN( 2 ).percent();

    if ( p->talent.ravage.ok() )
    {
      ravage_buff = p->specialization() == DRUID_FERAL ? p->buff.ravage_fb : p->buff.ravage_maul;
      ravage_proc = p->get_proc( "Ravage" )->collect_interval()->collect_count();
    }
  }

  timespan_t execute_time() const override
  {
    if ( !ab::player->in_combat )
      return 10_ms;

    return ab::execute_time();
  }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );

    if ( ab::result_is_hit( s->result ) )
    {
      if ( ooc_chance )
      {
        bool active = ab::p()->buff.clearcasting_cat->check();
        double chance = ab::weapon->proc_chance_on_swing( ooc_chance );

        // Internal cooldown is handled by buff.
        if ( ab::p()->buff.clearcasting_cat->trigger( 1, buff_t::DEFAULT_VALUE(), chance ) )
        {
          if ( active )
            ab::p()->proc.clearcasting_wasted->occur();

          ab::p()->buff.flash_of_clarity->trigger();
        }
      }

      if ( ab::p()->rngs.ravage && ab::p()->rngs.ravage->trigger() )
      {
        ravage_proc->occur();
        ravage_buff->trigger();
      }
    }
  }
};

// Caster Melee Attack ======================================================
struct caster_melee_t final : public druid_melee_t<druid_attack_t<melee_attack_t>>
{
  caster_melee_t( druid_t* p ) : base_t( "caster_melee", p )
  {
    name_str_reporting = "Caster Melee";
  }
};

// Bear Melee Attack ========================================================
struct bear_melee_t final : public druid_melee_t<bear_attack_t>
{
  bear_melee_t( druid_t* p ) : base_t( "bear_melee", p )
  {
    name_str_reporting = "Bear Melee";

    form_mask = form_e::BEAR_FORM;

    energize_type = action_energize::ON_HIT;
    energize_resource = resource_e::RESOURCE_RAGE;
    energize_amount = util::round( 1.75 *
      p->bear_weapon.swing_time.total_seconds() *
      ( 1.0 + find_effect( p->spec.bear_form_passive, A_MOD_RAGE_FROM_DAMAGE_DEALT ).percent() ), 1 );
  }
};

// Cat Melee Attack =========================================================
struct cat_melee_t final : public druid_melee_t<cat_attack_t>
{
  cat_melee_t( druid_t* p ) : base_t( "cat_melee", p )
  {
    name_str_reporting = "Cat Melee";

    form_mask = form_e::CAT_FORM;
  }
};

// Auto Attack (Action)========================================================
struct auto_attack_t final : public melee_attack_t
{
  auto_attack_t( druid_t* p ) : melee_attack_t( "auto_attack", p, spell_data_t::nil() )
  {
    trigger_gcd = 0_ms;
    ignore_false_positive = use_off_gcd = true;
  }

  void execute() override
  {
    player->main_hand_attack->weapon = &( player->main_hand_weapon );
    player->main_hand_attack->base_execute_time = player->main_hand_weapon.swing_time;
    player->main_hand_attack->schedule_execute();
  }

  bool ready() override
  {
    if ( player->is_moving() )
      return false;

    if ( !player->main_hand_attack )
      return false;

    return ( player->main_hand_attack->execute_event == nullptr );  // not swinging
  }
};
}  // namespace auto_attacks

// Persistent Delay Event ===================================================
// Delay triggering the event a random amount. This prevents fixed-period drivers from ticking at the exact same times
// on every iteration. Buffs that use the event to activate should implement tick_zero-like behavior.
struct persistent_delay_event_t final : public event_t
{
  std::function<void()> exec_fn;

  persistent_delay_event_t( druid_t* p, buff_t* b )
    : persistent_delay_event_t( p, [ b ]() { b->execute(); }, b->buff_period )
  {}

  persistent_delay_event_t( druid_t* p, std::function<void()> fn, timespan_t d )
    : event_t( *p ), exec_fn( std::move( fn ) )
  {
    schedule( rng().real() * d );
  }

  const char* name() const override { return "persistent_event_delay"; }

  void execute() override { exec_fn(); }
};

struct astral_power_decay_event_t final : public event_t
{
private:
  druid_t* p_;
  double nb_cap;

public:
  astral_power_decay_event_t( druid_t* p )
    : event_t( *p, 500_ms ),
      p_( p ),
      nb_cap( p->talent.natures_balance->effectN( 2 ).base_value() )
  {}

  const char* name() const override { return "astral_power_decay"; }

  void execute() override
  {
    if ( !p_->in_combat )
    {
      if ( auto cur = p_->resources.current[ RESOURCE_ASTRAL_POWER ]; cur > nb_cap )
      {
        p_->resource_loss( RESOURCE_ASTRAL_POWER, std::min( 5.0, cur - nb_cap ) );
        make_event<astral_power_decay_event_t>( sim(), p_ );
      }
    }
  }
};

// Denizen of the Dream Proxy Action ========================================
struct denizen_of_the_dream_t final : public action_t
{
  druid_t* druid;

  denizen_of_the_dream_t( druid_t* p )
    : action_t( action_e::ACTION_OTHER, "denizen_of_the_dream", p, p->talent.denizen_of_the_dream ), druid( p )
  {
    callbacks = false;

    p->pets.denizen_of_the_dream.set_default_duration( p->find_spell( 394076 )->duration() );
    p->pets.denizen_of_the_dream.set_creation_event_callback( pets::parent_pet_action_fn( this ) );
  }

  result_e calculate_result( action_state_t* ) const override
  {
    return result_e::RESULT_NONE;
  }

  void execute() override
  {
    action_t::execute();

    druid->pets.denizen_of_the_dream.spawn();
  }
};

// Sylvan Beckoning Proxy Action ============================================
struct sylvan_beckoning_t final : public action_t
{
  druid_t* druid;

  sylvan_beckoning_t( druid_t* p )
    : action_t( action_e::ACTION_OTHER, "sylvan_beckoning", p, p->talent.sylvan_beckoning ), druid( p )
  {
    callbacks = false;

    p->pets.sylvan_beckoning.set_default_duration( p->buff.sylvan_beckoning->buff_duration() );
    p->pets.sylvan_beckoning.set_creation_event_callback( pets::parent_pet_action_fn( this ) );
  }

  result_e calculate_result( action_state_t* ) const override
  {
    return result_e::RESULT_NONE;
  }

  void execute() override
  {
    action_t::execute();

    druid->pets.sylvan_beckoning.spawn();
  }
};

// Waking Nightmare Proxy Action ============================================
struct waking_nightmare_t final : public action_t
{
  druid_t* druid;

  waking_nightmare_t( druid_t* p )
    : action_t( action_e::ACTION_OTHER, "waking_nightmare", p, p->talent.waking_nightmare ), druid( p )
  {
    callbacks = false;

    p->pets.dread_shade.set_default_duration( find_trigger( this ).trigger()->duration() + 1_ms );
    p->pets.dread_shade.set_creation_event_callback( pets::parent_pet_action_fn( this ) );
  }

  result_e calculate_result( action_state_t* ) const override
  {
    return result_e::RESULT_NONE;
  }

  void execute() override
  {
    action_t::execute();

    druid->pets.dread_shade.spawn();
  }
};

// ==========================================================================
// Druid Character Definition
// ==========================================================================

// druid_t::activate ========================================================
void druid_t::activate()
{
  if ( spec.astral_power->ok() )
  {
    // Create repeating resource_loss event once OOC for 20s
    register_on_combat_state_callback( [ this ]( player_t*, bool c ) {
      if ( !c )
      {
        astral_power_decay = make_event( *sim, 20_s, [ this ]() {
          astral_power_decay = nullptr;
          make_event<astral_power_decay_event_t>( *sim, this );
        } );
      }
      else
      {
        event_t::cancel( astral_power_decay );
      }
    } );
  }

  if ( talent.chomp.ok() )
  {
    auto chomp_pct = spec.chomp_controller->effectN( 1 ).base_value() + 1;
    auto chomp_grace = buff.chomp_enabler->data().effectN( 1 ).time_value();

    register_resource_callback( RESOURCE_ENERGY, chomp_pct, [ this, chomp_grace ]( bool inc ) {
      if ( inc && buff.chomp_enabler->check() )
        buff.chomp_enabler->trigger( chomp_grace );
      else if ( !inc )
        buff.chomp_enabler->trigger();
    }, true, false );
  }

  if ( talent.hunger_for_battle.ok() && talent.rip.ok() )
  {
    auto hfb_gain = get_gain( "Hunger for Battle" );
    auto hfb_amt = find_effect( find_spell( 1244550 ), E_ENERGIZE ).resource( RESOURCE_ENERGY );

    register_on_kill_callback( [ this, hfb_gain, hfb_amt ]( player_t* t ) {
      if ( get_target_data( t )->dots.rip->is_ticking() )
      {
        buff.hunger_for_battle->trigger();
        resource_gain( RESOURCE_ENERGY, hfb_amt, hfb_gain );
      }
    } );
  }

  if ( talent.resilient_flourishing.ok() && talent.thriving_growth.ok() )
  {
    register_on_kill_callback( [ this ]( player_t* t ) {
      auto td = get_target_data( t );
      auto stacks = td->debuff.bloodseeker_vines->check();
      auto dur = td->dots.bloodseeker_vines->remains();

      if ( stacks && dur > 0_ms )
      {
        auto vines = debug_cast<cat_attacks::bloodseeker_vines_t*>( active.bloodseeker_vines );
        const auto& tl = vines->target_list();
        if ( auto tar = get_smart_target( tl, &druid_td_t::dots_t::bloodseeker_vines, t ) )
        {
          // TODO: ugly hack. possibly use custom action_state
          auto orig_dur = vines->dot_duration;

          vines->dot_duration = dur;
          vines->dual = true;
          vines->dot_stacks = stacks;

          vines->execute_on_target( tar );

          vines->dot_duration = orig_dur;
          vines->dual = false;
          vines->dot_stacks = 1;
        }
      }
    } );
  }

  if ( talent.killing_strikes.ok() )
  {
    register_on_combat_state_callback( [ this ]( player_t*, bool c ) {
      if ( c )
        buff.killing_strikes_combat->trigger();
    } );
  }

  player_t::activate();
}

// druid_t::create_action  ==================================================
action_t* druid_t::create_action( std::string_view name, std::string_view opt )
{
  using namespace auto_attacks;
  using namespace cat_attacks;
  using namespace bear_attacks;
  using namespace heals;
  using namespace spells;

  action_t* a = nullptr;

  // Baseline Abilities
  if      ( name == "auto_attack"                   ) a =                  new auto_attack_t( this );
  else if ( name == "bear_form"                     ) a =                    new bear_form_t( this );
  else if ( name == "cat_form"                      ) a =                     new cat_form_t( this );
  else if ( name == "cancelform"                    ) a =                  new cancel_form_t( this );

  // Incarnation
  else if ( name == "incarnation" )
  {
    if      ( specialization() == DRUID_BALANCE     ) a =          new incarnation_moonkin_t( this );
    else if ( specialization() == DRUID_FERAL       ) a =              new incarnation_cat_t( this );
    else if ( specialization() == DRUID_GUARDIAN    ) a =             new incarnation_bear_t( this );
    else if ( specialization() == DRUID_RESTORATION ) a =             new incarnation_tree_t( this );
  }
  else if ( name == "celestial_alignment" &&
            talent.incarnation_moonkin.ok()         ) a =          new incarnation_moonkin_t( this );
  else if ( name == "berserk" &&
            talent.incarnation_cat.ok()             ) a =              new incarnation_cat_t( this );
  else if ( name == "berserk" &&
            talent.incarnation_bear.ok()            ) a =             new incarnation_bear_t( this );
  else if ( name == "celestial_alignment_only" ||
            name == "celestial_alignment"           ) a =          new celestial_alignment_t( this );
  else if ( name == "berserk_only" || name == "berserk" )
  {
    if      ( specialization() == DRUID_FERAL       ) a =                  new berserk_cat_t( this );
    else if ( specialization() == DRUID_GUARDIAN    ) a =                 new berserk_bear_t( this );
  }

  // Balance
  else if ( name == "eclipse"                       ) a =                      new eclipse_t( this );
  else if ( name == "lunar_eclipse"                 ) a =                new lunar_eclipse_t( this );
  else if ( name == "solar_eclipse"                 ) a =                new solar_eclipse_t( this );
  else if ( name == "force_of_nature"               ) a =              new force_of_nature_t( this );
  else if ( name == "fury_of_elune"                 ) a =                new fury_of_elune_t( this );
  else if ( name == "new_moon"                      ) a =                     new new_moon_t( this );
  else if ( name == "half_moon"                     ) a =                    new half_moon_t( this );
  else if ( name == "full_moon"                     ) a =                    new full_moon_t( this );
  else if ( name == "moons"                         ) a =                   new moon_proxy_t( this );
  else if ( name == "solar_beam"                    ) a =                   new solar_beam_t( this );
  else if ( name == "starfall"                      ) a =                     new starfall_t( this );
  else if ( name == "starsurge" &&
            specialization() == DRUID_BALANCE       ) a =                    new starsurge_t( this );
  else if ( name == "wild_mushroom"                 ) a =                new wild_mushroom_t( this );

  // Feral
  else if ( name == "chomp"                         ) a =                        new chomp_t( this );
  else if ( name == "feral_frenzy"                  ) a =                 new feral_frenzy_t( this );
  else if ( name == "frantic_frenzy"                ) a =               new frantic_frenzy_t( this );
  else if ( name == "moonfire_cat" ||
            name == "lunar_inspiration"             ) a =            new lunar_inspiration_t( this );
  else if ( name == "primal_wrath"                  ) a =                 new primal_wrath_t( this );
  else if ( name == "tigers_fury"                   ) a =                  new tigers_fury_t( this );

  // Guardian
  else if ( name == "bristling_fur"                 ) a =                new bristling_fur_t( this );
  else if ( name == "lunar_beam"                    ) a =                   new lunar_beam_t( this );
  else if ( ( name == "maul" && talent.raze.ok() ) ||
            name == "raze"                          ) a =                         new raze_t( this );
  else if ( name == "maul_only" || name == "maul"   ) a =                         new maul_t( this );
  else if ( ( name == "moonfire" && talent.red_moon.ok() ) ||
            name == "red_moon"                      ) a =                     new red_moon_t( this );
  else if ( name == "moonfire_only"                 ) a =                     new moonfire_t( this );
  else if ( name == "sundering_roar"                ) a =               new sundering_roar_t( this );
  else if ( name == "wild_guardian"                 ) a =                new wild_guardian_t( this );

  // Restoration
  else if ( name == "efflorescence"                 ) a =                new efflorescence_t( this );
  else if ( name == "lifebloom"                     ) a =                    new lifebloom_t( this );
  else if ( name == "natures_cure"                  ) a =                 new natures_cure_t( this );
  else if ( name == "swiftmend"                     ) a =                    new swiftmend_t( this );
  else if ( name == "tranquility"                   ) a =                  new tranquility_t( this );

  // Multi-spec Talents
  else if ( name == "convoke_the_spirits"           ) a =          new convoke_the_spirits_t( this );
  else if ( name == "survival_instincts"            ) a =           new survival_instincts_t( this );

  // Class Talents
  else if ( name == "barkskin"                      ) a =                     new barkskin_t( this );
  else if ( name == "dash" || name == "tiger_dash"  ) a =                         new dash_t( this );
  else if ( name == "frenzied_regeneration"         ) a =        new frenzied_regeneration_t( this );
  else if ( name == "heart_of_the_wild"             ) a =            new heart_of_the_wild_t( this );
  else if ( name == "incapacitating_roar"           ) a =          new incapacitating_roar_t( this );
  else if ( name == "innervate"                     ) a =                    new innervate_t( this );
  else if ( name == "ironfur"                       ) a =                      new ironfur_t( this );
  else if ( name == "maim"                          ) a =                         new maim_t( this );
  else if ( name == "moonkin_form"                  ) a =                 new moonkin_form_t( this );
  else if ( name == "rake"                          ) a =                         new rake_t( this );
  else if ( name == "rejuvenation"                  ) a =                 new rejuvenation_t( this );
  else if ( name == "remove_corruption"             ) a =            new remove_corruption_t( this );
  else if ( name == "rip"                           ) a =                          new rip_t( this );
  else if ( name == "skull_bash"                    ) a =                   new skull_bash_t( this );
  else if ( name == "soothe"                        ) a =                       new soothe_t( this );
  else if ( name == "stampeding_roar"               ) a =              new stampeding_roar_t( this );
  else if ( name == "starfire"                      ) a =                     new starfire_t( this );
  else if ( name == "starsurge"                     )a =            new starsurge_offspec_t( this );
  else if ( name == "sunfire"                       ) a =                      new sunfire_t( this );
  else if ( name == "swipe"                         ) a =                  new swipe_proxy_t( this );
  else if ( name == "swipe_bear"                    ) a =                   new swipe_bear_t( this );
  else if ( name == "swipe_cat"                     ) a =                    new swipe_cat_t( this );
  else if ( name == "thrash"                        ) a =                       new thrash_t( this );
  else if ( name == "wild_charge"                   ) a =                  new wild_charge_t( this );
  else if ( name == "wild_growth"                   ) a =                  new wild_growth_t( this );

  // Class Abilities
  else if ( name == "entangling_roots"              ) a =             new entangling_roots_t( this );
  else if ( name == "ferocious_bite"                ) a =               new ferocious_bite_t( this );
  else if ( name == "growl"                         ) a =                        new growl_t( this );
  else if ( name == "mangle"                        ) a =                       new mangle_t( this );
  else if ( name == "moonfire"                      ) a =                     new moonfire_t( this );
  else if ( name == "prowl"                         ) a =                        new prowl_t( this );
  else if ( name == "regrowth"                      ) a =                     new regrowth_t( this );
  else if ( name == "shred"                         ) a =                        new shred_t( this );
  else if ( name == "wrath"                         ) a =                        new wrath_t( this );

  if ( a )
  {
    a->parse_options( opt );

    if ( auto tmp = dynamic_cast<druid_action_data_t*>( a ) )
      tmp->action_flags |= flag_e::FOREGROUND;

    return a;
  }

  return player_t::create_action( name, opt );
}

// druid_t::init_spells =====================================================
void druid_t::init_spells()
{
  // Carnivorous Instinct is missing trait definitions for effect#2 and effect#3, so we manually add it into
  // dbc_override here, before any talent pointers are initialized.
  if ( auto ci = find_talent_spell( talent_tree::SPECIALIZATION, "Carnivorous Instinct" ); ci.ok() )
  {
    register_passive_effect_override( ci->effectN( 2 ), ci->effectN( 2 ).base_value() * ci.rank() );
    register_passive_effect_override( ci->effectN( 3 ), ci->effectN( 3 ).base_value() * ci.rank() );
  }

  auto check = [ this ]( auto check, auto arg ) {
    bool b;

    if constexpr ( std::is_invocable_v<decltype( &player_talent_t::ok ), decltype( check )> )
      b = check.ok();
    else if constexpr ( std::is_invocable_v<decltype( &spell_data_t::ok ), decltype( check )> )
      b = check->ok();
    else b = check;

    if ( b )
    {
      if constexpr ( std::is_same_v<decltype( arg ), int> )
        return find_spell( arg );
      else if constexpr ( std::is_same_v<decltype( arg ), const spell_data_t*> )
        return arg;
    }

    return spell_data_t::not_found();
  };

  player_t::init_spells();

  // Talents ================================================================

  auto CT = [ this ]( std::string_view n, auto... s ) {
    return find_talent_spell( talent_tree::CLASS, n, s... );
  };
  auto ST = [ this ]( std::string_view n, auto... s ) {
    return find_talent_spell( talent_tree::SPECIALIZATION, n, s... );
  };
  auto HT = [ this ]( std::string_view n, auto... s ) {
    return find_talent_spell( talent_tree::HERO, n, s... );
  };

  // Class tree
  sim->print_debug( "Initializing class talents..." );
  talent.aessinas_renewal               = CT( "Aessina's Renewal" );  // TODO: NYI
  talent.astral_influence               = CT( "Astral Influence" );
  talent.circle_of_the_heavens          = CT( "Circle of the Heavens" );
  talent.circle_of_the_wild             = CT( "Circle of the Wild" );
  talent.cyclone                        = CT( "Cyclone" );
  talent.feline_swiftness               = CT( "Feline Swiftness" );
  talent.fluid_form                     = CT( "Fluid Form" );
  talent.forestwalk                     = CT( "Forestwalk" );
  talent.frenzied_regeneration          = CT( "Frenzied Regeneration" );
  talent.heart_of_the_wild              = CT( "Heart of the Wild" );
  talent.hibernate                      = CT( "Hibernate" );
  talent.gale_winds                     = CT( "Gale Winds" );
  talent.gift_of_the_wild               = CT( "Gift of the Wild" );
  talent.grievous_wounds                = CT( "Grievous Wounds" );
  talent.improved_barkskin              = CT( "Improved Barkskin" );
  talent.improved_natures_cure          = CT( "Improved Nature's Cure" );
  talent.improved_stampeding_roar       = CT( "Improved Stampeding Roar");
  talent.incapacitating_roar            = CT( "Incapacitating Roar" );
  talent.incessant_tempest              = CT( "Incessant Tempest" );
  talent.innervate                      = CT( "Innervate" );
  talent.instincts_of_the_claw          = CT( "Instincts of the Claw" );
  talent.ironfur                        = CT( "Ironfur" );
  talent.killer_instinct                = CT( "Killer Instinct" );
  talent.light_of_the_sun               = CT( "Light of the Sun" );
  talent.lingering_healing              = CT( "Lingering Healing" );
  talent.lore_of_the_grove              = CT( "Lore of the Grove" );
  talent.lycaras_inspiration            = CT( "Lycara's Inspiration" );
  talent.lycaras_teachings              = CT( "Lycara's Teachings" );
  talent.maim                           = CT( "Maim" );
  talent.mass_entanglement              = CT( "Mass Entanglement" );
  talent.matted_fur                     = CT( "Matted Fur" );
  talent.mighty_bash                    = CT( "Mighty Bash" );
  talent.moonkin_form                   = CT( "Moonkin Form" );
  talent.natural_recovery               = CT( "Natural Recovery" );
  talent.nurturing_instinct             = CT( "Nurturing Instinct" );
  talent.oakskin                        = CT( "Oakskin" );
  talent.perfectlyhoned_instincts       = CT( "Perfectly-Honed Instincts" );  // TODO: NYI
  talent.primal_fury                    = CT( "Primal Fury" );
  talent.rake                           = CT( "Rake" );
  talent.rejuvenation                   = CT( "Rejuvenation" );
  talent.remove_corruption              = CT( "Remove Corruption" );
  talent.rip                            = CT( "Rip" );
  talent.skull_bash                     = CT( "Skull Bash" );
  talent.soothe                         = CT( "Soothe" );
  talent.stampeding_roar                = CT( "Stampeding Roar" );
  talent.starfire                       = CT( "Starfire" );
  talent.starlight_conduit              = CT( "Starlight Conduit" );
  talent.starsurge                      = CT( "Starsurge" );
  talent.sunfire                        = CT( "Sunfire" );
  talent.swipe                          = CT( "Swipe" );
  talent.symbiotic_relationship         = CT( "Symbiotic Relationship" );  // TODO: NYI
  talent.thick_hide                     = CT( "Thick Hide" );
  talent.typhoon                        = CT( "Typhoon" );
  talent.ursine_vigor                   = CT( "Ursine Vigor" );
  talent.ursocs_spirit                  = CT( "Ursoc's Spirit" );
  talent.ursols_vortex                  = CT( "Ursol's Vortex" );
  talent.verdant_heart                  = CT( "Verdant Heart" );
  talent.wellhoned_instincts            = CT( "Well-Honed Instincts" );
  talent.wild_charge                    = CT( "Wild Charge" );
  talent.wild_growth                    = CT( "Wild Growth" );

  // Multi-Spec
  sim->print_debug( "Initializing multi-spec talents..." );
  talent.convoke_the_spirits            = ST( "Convoke the Spirits" );
  talent.survival_instincts             = ST( "Survival Instincts" );

  // Balance
  sim->print_debug( "Initializing balance talents..." );
  talent.ascendant_eclipses_1           = ST( "Ascendant Eclipses", 1 );
  talent.ascendant_eclipses_2           = ST( "Ascendant Eclipses", 2 );
  talent.ascendant_eclipses_3           = ST( "Ascendant Eclipses", 3 );
  talent.aetherial_kindling             = ST( "Aetherial Kindling" );
  talent.astral_communion               = ST( "Astral Communion" );
  talent.balance_of_all_things          = ST( "Balance of All Things" );
  talent.celestial_alignment            = ST( "Celestial Alignment" );
  talent.celestial_fire                 = ST( "Celestial Fire" );
  talent.cosmic_rapidity                = ST( "Cosmic Rapidity" );
  talent.denizen_of_the_dream           = ST( "Denizen of the Dream" );
  talent.eclipse                        = ST( "Eclipse" );
  talent.elunes_challenge               = ST( "Elune's Challenge" );
  talent.elunes_guidance                = ST( "Elune's Guidance" );
  talent.force_of_nature                = ST( "Force of Nature" );
  talent.fury_of_elune                  = ST( "Fury of Elune" );
  talent.harmony_of_the_heavens         = ST( "Harmony of the Heavens" );
  talent.hail_of_stars                  = ST( "Hail of Stars" );
  talent.improved_eclipse               = ST( "Improved Eclipse" );
  talent.incarnation_moonkin            = ST( "Incarnation: Chosen of Elune" );
  talent.light_of_the_sun               = ST( "Light of the Sun" );
  talent.meteor_storm                   = ST( "Meteor Storm" );
  talent.meteorites                     = ST( "Meteorites" );
  talent.natures_balance                = ST( "Nature's Balance" );
  talent.natures_grace                  = ST( "Nature's Grace" );
  talent.new_moon                       = ST( "New Moon" );
  talent.orbit_breaker                  = ST( "Orbit Breaker" );
  talent.orbital_strike                 = ST( "Orbital Strike" );
  talent.power_of_goldrinn              = ST( "Power of Goldrinn" );
  talent.radiant_moonlight              = ST( "Radiant Moonlight" );
  talent.rattle_the_stars               = ST( "Rattle the Stars" );
  talent.sculpt_the_stars               = ST( "Sculpt the Stars" );
  talent.shooting_stars                 = ST( "Shooting Stars" );
  talent.solar_beam                     = ST( "Solar Beam" );
  talent.solstice                       = ST( "Solstice" );
  talent.soul_of_the_forest_owl         = ST( "Soul of the Forest", DRUID_BALANCE );
  talent.starlord                       = ST( "Starlord" );
  talent.starweaver                     = ST( "Starweaver" );
  talent.stellar_amplification          = ST( "Stellar Amplification" );
  talent.sundered_firmament             = ST( "Sundered Firmament" );
  talent.sunseeker_mushroom             = ST( "Sunseeker Mushroom" );
  talent.total_eclipse                  = ST( "Total Eclipse" );
  talent.touch_the_cosmos               = ST( "Touch the Cosmos" );
  talent.twin_moons                     = ST( "Twin Moons" );
  talent.umbral_embrace                 = ST( "Umbral Embrace" );
  talent.umbral_intensity               = ST( "Umbral Intensity" );
  talent.whirling_stars                 = ST( "Whirling Stars" );
  talent.wild_mushroom                  = ST( "Wild Mushroom" );
  talent.wild_surges                    = ST( "Wild Surges" );

  // Feral
  sim->print_debug( "Initializing feral talents..." );
  talent.apex_predators_craving         = ST( "Apex Predator's Craving" );
  talent.ashamanes_guidance             = ST( "Ashamane's Guidance" );
  talent.berserk_cat                    = ST( "Berserk", DRUID_FERAL );
  talent.berserk_heart_of_the_lion      = ST( "Berserk: Heart of the Lion" );
  talent.blood_spattered                = ST( "Blood Spattered" );
  talent.carnivorous_instinct           = ST( "Carnivorous Instinct" );
  talent.chomp                          = ST( "Chomp" );
  talent.circle_of_life_and_death       = ST( "Circle of Life and Death" );
  talent.coiled_to_spring               = ST( "Coiled to Spring" );
  talent.doubleclawed_rake              = ST( "Double-Clawed Rake" );
  talent.dreadful_bleeding              = ST( "Dreadful Bleeding" );
  talent.feral_frenzy                   = ST( "Feral Frenzy" );
  talent.focused_frenzy                 = ST( "Focused Frenzy" );
  talent.frantic_frenzy                 = ST( "Frantic Frenzy" );
  talent.frantic_momentum               = ST( "Frantic Momentum" );
  talent.hunger_for_battle              = ST( "Hunger for Battle" );
  talent.incarnation_cat                = ST( "Incarnation: Avatar of Ashamane" );
  talent.infected_wounds_cat            = ST( "Infected Wounds", DRUID_FERAL );
  talent.lacerating_claws               = ST( "Lacerating Claws" );
  talent.lunar_inspiration              = ST( "Lunar Inspiration" );
  talent.merciless_claws                = ST( "Merciless Claws" );
  talent.moment_of_clarity              = ST( "Moment of Clarity" );
  talent.omen_of_clarity_cat            = ST( "Omen of Clarity", DRUID_FERAL );
  talent.panthers_guile                 = ST( "Panther's Guile" );
  talent.pouncing_strikes               = ST( "Pouncing Strikes" );
  talent.primal_wrath                   = ST( "Primal Wrath" );
  talent.predator                       = ST( "Predator" );
  talent.raging_fury                    = ST( "Raging Fury" );
  talent.rampant_ferocity               = ST( "Rampant Ferocity" );
  talent.rip_and_tear                   = ST( "Rip and Tear" );
  talent.saber_jaws                     = ST( "Saber Jaws" );
  talent.sabertooth                     = ST( "Sabertooth" );
  talent.savage_fury                    = ST( "Savage Fury" );
  talent.soul_of_the_forest_cat         = ST( "Soul of the Forest", DRUID_FERAL );
  talent.sudden_ambush                  = ST( "Sudden Ambush" );
  talent.taste_for_blood                = ST( "Taste for Blood" );
  talent.tigers_fury                    = ST( "Tiger's Fury" );
  talent.tigers_tenacity                = ST( "Tiger's Tenacity" );
  talent.tireless_energy                = ST( "Tireless Energy" );
  talent.unseen_predator_1              = ST( "Unseen Predator", 1 );
  talent.unseen_predator_2              = ST( "Unseen Predator", 2 );
  talent.unseen_predator_3              = ST( "Unseen Predator", 3 );
  talent.veinripper                     = ST( "Veinripper" );
  talent.wild_slashes                   = ST( "Wild Slashes" );

  // Guardian
  sim->print_debug( "Initializing guardian talents..." );
  talent.after_the_wildfire             = ST( "After the Wildfire" );
  talent.berserk_bear                   = ST( "Berserk", DRUID_GUARDIAN );
  talent.blood_frenzy                   = ST( "Blood Frenzy" );
  talent.brambles                       = ST( "Brambles" );
  talent.bristling_fur                  = ST( "Bristling Fur" );
  talent.dream_guide                    = ST( "Dream Guide" );
  talent.dream_of_cenarius_bear         = ST( "Dream of Cenarius", DRUID_GUARDIAN );
  talent.elunes_favored                 = ST( "Elune's Favored" );
  talent.flashing_claws                 = ST( "Flashing Claws" );
  talent.front_of_the_pack              = ST( "Front of the Pack" );
  talent.fury_of_nature                 = ST( "Fury of Nature" );
  talent.galactic_guardian              = ST( "Galactic Guardian" );
  talent.gift_of_an_ancient_guardian    = ST( "Gift of an Ancient Guardian" );
  talent.gore                           = ST( "Gore" );
  talent.gory_fur                       = ST( "Gory Fur" );
  talent.guardian_of_elune              = ST( "Guardian of Elune" );
  talent.harnessed_rage                 = ST( "Harnessed Rage" );
  talent.incarnation_bear               = ST( "Incarnation: Guardian of Ursoc" );
  talent.infected_wounds_bear           = ST( "Infected Wounds", DRUID_GUARDIAN );
  talent.innate_resolve                 = ST( "Innate Resolve" );
  talent.killing_blow                   = ST( "Killing Blow" );
  talent.lunar_beam                     = ST( "Lunar Beam" );
  talent.maul                           = ST( "Maul" );
  talent.memory_of_ysera                = ST( "Memory of Ysera" );
  talent.moonless_night                 = ST( "Moonless Night" );
  talent.natural_resilience             = ST( "Natural Resilience" );
  talent.persistence                    = ST( "Persistence" );
  talent.raze                           = ST( "Raze" );
  talent.red_moon                       = ST( "Red Moon" );
  talent.reinforced_fur                 = ST( "Reinforced Fur" );
  talent.reinvigoration                 = ST( "Reinvigoration" );
  talent.rend_and_tear                  = ST( "Rend and Tear" );
  talent.scintillating_moonlight        = ST( "Scintillating Moonlight" );
  talent.soul_of_the_forest_bear        = ST( "Soul of the Forest", DRUID_GUARDIAN );
  talent.sundering_roar                 = ST( "Sundering Roar" );
  talent.survival_of_the_fittest        = ST( "Survival of the Fittest" );
  talent.twin_moonfire                  = ST( "Twin Moonfire" );
  talent.untamed_savagery               = ST( "Untamed Savagery" );
  talent.ursocs_endurance               = ST( "Ursoc's Endurance" );
  talent.ursocs_fury                    = ST( "Ursoc's Fury" );
  talent.ursocs_guidance                = ST( "Ursoc's Guidance" );
  talent.ursols_warding                 = ST( "Ursol's Warding" );  // TODO: NYI
  talent.vulnerable_flesh               = ST( "Vulnerable Flesh" );
  talent.waking_nightmare               = ST( "Waking Nightmare" );
  talent.ward_of_the_forest             = ST( "Ward of the Forest" );
  talent.wild_guardian_1                = ST( "Wild Guardian", 1 );
  talent.wild_guardian_2                = ST( "Wild Guardian", 2 );
  talent.wild_guardian_3                = ST( "Wild Guardian", 3 );

  // Restoration
  sim->print_debug( "Initializing restoration talents..." );
  talent.abundance                      = ST( "Abundance" );
  talent.call_of_the_elder_druid        = ST( "Call of the Elder Druid" );
  talent.cenarius_guidance              = ST( "Cenarius' Guidance" );  // TODO: Incarn bonus NYI
  talent.cultivation                    = ST( "Cultivation" );
  talent.dream_of_cenarius_tree         = ST( "Dream of Cenarius", DRUID_RESTORATION );  // TODO: NYI
  talent.efflorescence                  = ST( "Efflorescence" );
  talent.embrace_of_the_dream           = ST( "Embrace of the Dream" );  // TODO: NYI
  talent.everbloom_1                    = ST( "Everbloom", 1 );
  talent.everbloom_2                    = ST( "Everbloom", 2 );
  talent.everbloom_3                    = ST( "Everbloom", 3 );
  talent.flourish                       = ST( "Flourish" );
  talent.germination                    = ST( "Germination" );
  talent.grove_guardians                = ST( "Grove Guardians" );  // TODO: NYI
  talent.harmonious_blooming            = ST( "Harmonious Blooming" );
  talent.improved_ironbark              = ST( "Improved Ironbark" );
  talent.improved_regrowth              = ST( "Improved Regrowth" );
  talent.improved_swiftmend             = ST( "Improved Swiftmend" );
  talent.improved_wild_growth           = ST( "Improved Wild Growth" );
  talent.incarnation_tree               = ST( "Incarnation: Tree of Life" );
  talent.inner_peace                    = ST( "Inner Peace" );
  talent.intensity                      = ST( "Intensity" );
  talent.ironbark                       = ST( "Ironbark" );
  talent.lifebloom                      = ST( "Lifebloom" );
  talent.lifetreading                   = ST( "Lifetreading" );
  talent.liveliness                     = ST( "Liveliness" );
  talent.master_shapeshifter            = ST( "Master Shapeshifter" );
  talent.natures_bounty                 = ST( "Nature's Bounty" );
  talent.natures_splendor               = ST( "Nature's Splendor" );
  talent.natures_swiftness              = ST( "Nature's Swiftness" );
  talent.nurturing_dormancy             = ST( "Nurturing Dormancy" );  // TODO: NYI
  talent.omen_of_clarity_tree           = ST( "Omen of Clarity", DRUID_RESTORATION );
  talent.passing_seasons                = ST( "Passing Seasons" );
  talent.photosynthesis                 = ST( "Photosynthesis" );
  talent.power_of_the_archdruid         = ST( "Power of the Archdruid" );  // TODO: NYI
  talent.prosperity                     = ST( "Prosperity" );  // TODO: NYI
  talent.rampant_growth                 = ST( "Rampant Growth" );  // TODO: copy on lb target NYI
  talent.reforestation                  = ST( "Reforestation" );  // TODO: NYI
  talent.regenesis                      = ST( "Regenesis" );  // TODO: NYI
  talent.regenerative_heartwood         = ST( "Regenerative Heartwood" );  // TODO: NYI
  talent.renewing_surge                 = ST( "Renewing Surge" );
  talent.soul_of_the_forest_tree        = ST( "Soul of the Forest", DRUID_RESTORATION );
  talent.stonebark                      = ST( "Stonebark" );
  talent.swiftmend                      = ST( "Swiftmend" );
  talent.thriving_vegetation            = ST( "Thriving Vegetation" );  // TODO: NYI
  talent.tranquil_mind                  = ST( "Tranquil Mind" );
  talent.tranquility                    = ST( "Tranquility" );
  talent.unstoppable_growth             = ST( "Unstoppable Growth" );
  talent.verdancy                       = ST( "Verdancy" );  // TODO: NYI
  talent.verdant_infusion               = ST( "Verdant Infusion" );  // TODO: NYI
  talent.waking_dream                   = ST( "Waking Dream" );  // TODO: increased healing per rejuv NYI
  talent.wild_synthesis                 = ST( "Wild Synthesis" );  // TODO: NYI
  talent.yseras_gift                    = ST( "Ysera's Gift" );

  sim->print_debug( "Initializing hero talents..." );
  // Druid of the Claw
  talent.aggravate_wounds               = HT( "Aggravate Wounds" );
  talent.bestial_strength               = HT( "Bestial Strength" );
  talent.claw_rampage                   = HT( "Claw Rampage" );
  talent.dreadful_wound                 = HT( "Dreadful Wound" );
  talent.empowered_shapeshifting        = HT( "Empowered Shapeshifting" );
  talent.exacerbating_wounds            = HT( "Exacerbating Wounds" );
  talent.fount_of_strength              = HT( "Fount of Strength" );  // TODO: FR hp buff NYI
  talent.killing_strikes                = HT( "Killing Strikes" );
  talent.limb_from_limb                 = HT( "Limb from Limb" );
  talent.packs_endurance                = HT( "Pack's Endurance" );
  talent.ravage                         = HT( "Ravage" );
  talent.ruthless_aggression            = HT( "Ruthless Aggression" );
  talent.strike_for_the_heart           = HT( "Strike for the Heart" );
  talent.tear_down_the_mighty           = HT( "Tear Down the Mighty" );
  talent.twin_claw                      = HT( "Twin Claw" );
  talent.wildpower_surge                = HT( "Wildpower Surge" );
  talent.wildshape_mastery              = HT( "Wildshape Mastery" );  // TODO:: heal NYI

  // Wildstalker
  talent.bond_with_nature               = HT( "Bond with Nature" );
  talent.bursting_growth                = HT( "Bursting Growth" );
  talent.entangling_vortex              = HT( "Entangling Vortex" );  // NYI
  talent.flower_walk                    = HT( "Flower Walk" );  // TODO: heal NYI
  talent.green_thumb                    = HT( "Green Thumb" );
  talent.harmonious_constitution        = HT( "Harmonious Constitution" );
  talent.hunt_beneath_the_open_skies    = HT( "Hunt Beneath the Open Skies" );
  talent.implant                        = HT( "Implant" );
  talent.lethal_preservation            = HT( "Lethal Preservation" );
  talent.patient_custodian              = HT( "Patient Custodian" );
  talent.rampancy                       = HT( "Rampancy" );
  talent.resilient_flourishing          = HT( "Resilient Flourishing" );
  talent.root_network                   = HT( "Root Network" );  // TODO: symbiotic bloom buff NYI
  talent.strategic_infusion             = HT( "Strategic Infusion" );  // TODO: heal buff NYI
  talent.thriving_growth                = HT( "Thriving Growth" );  // TODO: heal NYI
  talent.twin_sprouts                   = HT( "Twin Sprouts" );
  talent.vigorous_creepers              = HT( "Vigorous Creepers" );
  talent.wildstalkers_power             = HT( "Wildstalker's Power" );

  // Keeper of the Grove
  talent.blooming_infusion              = HT( "Blooming Infusion" );
  talent.bounteous_bloom                = HT( "Bounteous Bloom" );
  talent.cenarius_might                 = HT( "Cenarius' Might" );
  talent.control_of_the_dream           = HT( "Control of the Dream" );
  talent.dream_surge                    = HT( "Dream Surge" );  // TODO: heal NYI
  talent.durability_of_nature           = HT( "Durability of Nature" );  // TODO: NYI
  talent.dryads_dance                   = HT( "Dryad's Dance" );
  talent.early_spring                   = HT( "Early Spring" );
  talent.expansiveness                  = HT( "Expansiveness" );
  talent.groves_inspiration             = HT( "Grove's Inspiration" );
  talent.harmony_of_the_grove           = HT( "Harmony of the Grove" );
  talent.potent_enchantments            = HT( "Potent Enchantments" );
  talent.power_of_nature                = HT( "Power of Nature" );  // TODO: grove guardian buff NYI
  talent.power_of_the_dream             = HT( "Power of the Dream" );
  talent.protective_growth              = HT( "Protective Growth" );  // TODO: NYI
  talent.spirit_of_the_thicket          = HT( "Spirit of the Thicket" );
  talent.sylvan_beckoning               = HT( "Sylvan Beckoning" );
  talent.treants_of_the_moon            = HT( "Treants of the Moon" );

  // Elune's Chosen
  talent.arcane_affinity                = HT( "Arcane Affinity" );
  talent.astral_insight                 = HT( "Astral Insight" );
  talent.atmospheric_exposure           = HT( "Atmospheric Exposure" );
  talent.bask_in_moonlight              = HT( "Bask in Moonlight" );
  talent.boundless_moonlight            = HT( "Boundless Moonlight" );
  talent.elunes_grace                   = HT( "Elune's Grace" );
  talent.glistening_fur                 = HT( "Glistening Fur" );
  talent.lunar_calling                  = HT( "Lunar Calling" );
  talent.lunar_insight                  = HT( "Lunar Insight" );
  talent.lunation                       = HT( "Lunation" );
  talent.moondust                       = HT( "Moondust" );  // TODO: NYI
  talent.moon_guardian                  = HT( "Moon Guardian" );
  talent.penumbral_swell                = HT( "Penumbral Swell" );
  talent.star_cascade                   = HT( "Star Cascade" );
  talent.stellar_command                = HT( "Stellar Command" );
  talent.the_eternal_moon               = HT( "The Eternal Moon" );
  talent.the_light_of_elune             = HT( "The Light of Elune" );

  // Baseline
  spec.bear_form                = find_class_spell( "Bear Form" );
  spec.bear_form_override       = find_spell( 106829 );
  spec.bear_form_passive        = find_spell( 1178 );
  spec.bear_form_passive_2      = find_spell( 21178 );
  spec.cat_form                 = find_class_spell( "Cat Form" );
  spec.cat_form_override        = find_spell( 48629 );
  spec.cat_form_passive         = find_spell( 3025 );
  spec.cat_form_passive_2       = find_spell( 106840 );
  spec.cat_form_speed           = find_spell( 113636 );
  spec.improved_prowl           = find_specialization_spell( "Improved Prowl" );
  // hardcoded since red moon replaces
  spec.moonfire                 = talent.red_moon.ok() ? find_spell( 8921 ) : find_class_spell( "Moonfire" );
  spec.moonfire_dmg             = find_spell( 164812 );
  spec.sunfire_dmg              = check( talent.sunfire, 164815 );
  spec.thrash                   = find_class_spell( "Thrash" );
  spec.thrash_bleed             = find_spell( 192090 );
  spec.wrath                    = find_specialization_spell( "Wrath" );
  if ( !spec.wrath->ok() )
    spec.wrath                  = find_class_spell( "Wrath" );

  // Balance Abilities
  spec.astral_power             = find_specialization_spell( "Astral Power" );
  spec.celestial_alignment      = talent.celestial_alignment.find_override_spell();
  spec.eclipse_lunar            = check( talent.eclipse, 48518 );
  spec.eclipse_solar            = check( talent.eclipse, 48517 );
  spec.full_moon                = check( talent.new_moon, 274283 );
  spec.half_moon                = check( talent.new_moon, 274282 );
  spec.incarnation_moonkin      = check( talent.incarnation_moonkin, 102560 );
  spec.shooting_stars_dmg       = check( talent.shooting_stars, 202497 );  // shooting stars damage
  spec.starfall                 = find_specialization_spell( "Starfall" );
  spec.stellar_amplification    = check( talent.stellar_amplification, 450214 );

  // Feral Abilities
  spec.berserk_cat              = talent.berserk_cat.find_override_spell();
  spec.chomp_controller         = check( talent.chomp, 1244292 );
  spec.predatory_swiftness      = find_specialization_spell( "Predatory Swiftness" );
  spec.sabertooth               = check( talent.sabertooth, 391722 );

  // Guardian Abilities
  spec.elunes_favored           = check( talent.elunes_favored, 370588 );
  spec.fury_of_nature           = check( talent.fury_of_nature, 370701 );
  spec.lightning_reflexes       = find_specialization_spell( "Lightning Reflexes" );
  spec.ursine_adept             = find_specialization_spell( "Ursine Adept" );

  // Restoration Abilities

  // Hero Talents
  spec.atmospheric_exposure     = check( talent.atmospheric_exposure, 430589 );
  spec.bloodseeker_vines        = check( talent.thriving_growth, 439531 );
  spec.dreadful_wound           = check( talent.dreadful_wound, specialization() == DRUID_FERAL ? 441812 : 451177 );
  spec.sylvan_beckoning_sf      = check( talent.sylvan_beckoning, 1264671 );

  // Masteries ==============================================================
  mastery.harmony             = find_mastery_spell( DRUID_RESTORATION );
  mastery.natures_guardian    = find_mastery_spell( DRUID_GUARDIAN );
  mastery.natures_guardian_AP = check( mastery.natures_guardian, 159195 );
  mastery.razor_claws         = find_mastery_spell( DRUID_FERAL );
  mastery.astral_invocation   = find_mastery_spell( DRUID_BALANCE );

  if ( talent.frenzied_regeneration.ok() )
  {
    hots.frenzied_regeneration = get_dot( "frenzied_regeneration", this );

    if ( talent.wild_guardian_1.ok() )
      hots.echo_of_frenzied_regeneration = get_dot( "echo_of_frenzied_regeneration", this );
  }

  // Arcane affinity is bugged with wrath and manually handled in wrath_t
  register_passive_affect_list( talent.arcane_affinity, affect_list_t( 1 ).remove_family_flag( 0 ) );

  // Circle of the Heavens/Wild have different values for restoration
  auto circle_mask = specialization() == DRUID_RESTORATION
    ? effect_mask_t( false ).enable( 3, 4 ) : effect_mask_t( false ).enable( 1, 2 );
  register_passive_effect_mask( talent.circle_of_the_heavens, circle_mask );
  register_passive_effect_mask( talent.circle_of_the_wild, circle_mask );

  register_passive_effect_mask( talent.patient_custodian, specialization() == DRUID_FERAL
    ? effect_mask_t( false ).enable( 1, 2 ) : effect_mask_t( false ).enable( 3, 4 ) );
  register_passive_effect_mask( talent.strike_for_the_heart, specialization() == DRUID_FERAL
    ? effect_mask_t( false ).enable( 1, 2, 3 ) : effect_mask_t( false ).enable( 4, 5 ) );

  if ( specialization() == DRUID_GUARDIAN )
    register_passive_effect_mask( talent.tear_down_the_mighty, effect_mask_t( false ).enable( 1 ) );

  if ( specialization() == DRUID_RESTORATION )
    deregister_passive_spell( talent.spirit_of_the_thicket );

  // Appears to be some kind of normalization factor but in reverse, disabled via script
  deregister_passive_effect( talent.rattle_the_stars->effectN( 3 ) );

  if ( !bugs )
  {
    // Bask in Moonlight is bugged and doesn't disable other spec's effects
    register_passive_effect_mask( talent.bask_in_moonlight, specialization() == DRUID_BALANCE
      ? effect_mask_t( false ).enable( 1, 2 ) : effect_mask_t( false ).enable( 3, 4 ) );
  }

  parse_all_class_passives();
  parse_all_passive_talents();
  parse_all_passive_sets();

  if ( talent.gift_of_the_wild.ok() )
  {
    auto eff = sim->auras.mark_of_the_wild->data().effectN( 1 );
    auto val = eff.base_value() * ( 1.0 + talent.gift_of_the_wild->effectN( 1 ).percent() );
    register_passive_effect_override( eff, val );
  }

  parse_raid_buffs();

  if ( talent.ashamanes_guidance.ok() )
    parse_passive_effects( find_spell( talent.convoke_the_spirits.ok() ? 391538 : 1244546 ) );

  if ( talent.chomp.ok() )
    parse_passive_effects( find_spell( 1244292 ) );  // chomp controller

  // Fury of Nature talent applies value to the passive via script
  if ( talent.fury_of_nature.ok() )
  {
    auto fon_base_value = talent.fury_of_nature->effectN( 1 ).base_value();
    register_passive_effect_override( spec.fury_of_nature->effectN( 1 ), fon_base_value );
    register_passive_effect_override( spec.fury_of_nature->effectN( 4 ), fon_base_value );

    if ( talent.lunar_calling.ok() )
    {
      register_passive_effect_override( spec.fury_of_nature->effectN( 2 ), fon_base_value );
      register_passive_effect_override( spec.fury_of_nature->effectN( 3 ), fon_base_value );
    }
  }

  parse_passive_effects( spec.fury_of_nature, true );
}

// druid_t::init_items ======================================================
void druid_t::init_items()
{
  player_t::init_items();
}

// druid_t::init_base =======================================================
void druid_t::init_base_stats()
{
  // Set base distance based on spec
  if ( base.distance < 1 )
    base.distance = specialization() == DRUID_BALANCE ? 40 : 5;

  switch ( specialization() )
  {
    case DRUID_BALANCE:
    case DRUID_RESTORATION: base.spell_power_per_intellect = 1.0; break;
    case DRUID_FERAL:
    case DRUID_GUARDIAN:    base.attack_power_per_agility = 1.0;  break;
    default: break;
  }

  player_t::init_base_stats();

  // only intially activate required resources. others will be dynamically activated depending on apl
  resources.active_resource[ RESOURCE_ASTRAL_POWER ] = specialization() == DRUID_BALANCE;
  resources.active_resource[ RESOURCE_ENERGY ]       = specialization() == DRUID_FERAL;
  resources.active_resource[ RESOURCE_COMBO_POINT ]  = specialization() == DRUID_FERAL;
  resources.active_resource[ RESOURCE_HEALTH ]       = specialization() == DRUID_GUARDIAN;
  resources.active_resource[ RESOURCE_RAGE ]         = specialization() == DRUID_GUARDIAN;
  resources.active_resource[ RESOURCE_MANA ]         = specialization() == DRUID_RESTORATION;

  if ( options.disable_ready_trigger )
    ready_type = ready_e::READY_POLL;
  else if ( specialization() == DRUID_FERAL )
    ready_type = ready_e::READY_TRIGGER;
}

void druid_t::init_initial_stats()
{
  player_t::init_initial_stats();

  // enable CP & energy for cat form
  if ( uses_cat_form() )
  {
    resources.active_resource[ RESOURCE_COMBO_POINT ] = true;
    resources.active_resource[ RESOURCE_ENERGY ] = true;
  }

  // enable rage for bear form
  if ( uses_bear_form() )
  {
    resources.active_resource[ RESOURCE_HEALTH ] = true;
    resources.active_resource[ RESOURCE_RAGE ] = true;
  }

  if ( unique_gear::find_special_effect( this, 1234996 ) )
    resources.active_resource[ RESOURCE_MANA ] = true;
}

void druid_t::init_finished()
{
  for ( auto a : action_list )
  {
    if ( a->name_str == "cancel_buff" && !a->signature_str.empty() &&
         ( util::str_in_str_ci( a->signature_str, "name=bear_form" ) ||
           util::str_in_str_ci( a->signature_str, "name=cat_form" ) ||
           util::str_in_str_ci( a->signature_str, "name=moonkin_form" ) ) )
    {
      throw sc_invalid_apl_argument( fmt::format( "Use 'cancelform' instead of 'cancel_buff,{}'.", a->signature_str ) );
    }
  }

  player_t::init_finished();

  if ( ready_type == READY_TRIGGER && resource_thresholds.empty() )
    resource_thresholds.push_back( 0 );

  // PRECOMBAT SHENANIGANS
  // we do this here so all precombat actions have gone throught init() and init_finished() so if-expr are properly
  // parsed and we can adjust wrath travel times accordingly based on subsequent precombat actions that will sucessfully
  // cast. we also set and lock dreamstate after determining which casts will benefit from the cast time reduction.
  auto first_wrath_itr = precombat_action_list.end();

  for ( auto pre = precombat_action_list.begin(); pre != precombat_action_list.end(); pre++ )
  {
    if ( auto wr = dynamic_cast<spells::wrath_t*>( *pre ) )
    {
      if ( first_wrath_itr == precombat_action_list.end() )
        first_wrath_itr = pre;
      else if ( first_wrath_itr != pre - 1 )
        wr->dreamstate = true;

      wr->dreamstate.locked = true;

      std::for_each( pre + 1, precombat_action_list.end(), [ wr ]( action_t* a ) {
        // unnecessary offspec resources are disabled by default, so evaluate any if-expr on the candidate action first
        // so we don't call action_ready() on possible offspec actions that will require off-spec resources to be
        // enabled
        // * don't check harmful: we assume that any action that follows waits for wrath to finish, meaning wrath happens
        // entirely precombat
        if ( ( !a->if_expr || a->if_expr->success() ) && a->action_ready() )
          wr->harmful = false;  // more actions exist, set current wrath to non-harmful so we can keep casting

        if ( a->name_str == wr->name_str )
          wr->count++;  // see how many wrath casts are left, so we can adjust travel time when combat begins
      } );

      // if wrath is still harmful then it is the final precast spell, so we set the energize type to NONE, which will
      // then be accounted for in wrath_t::execute()
      if ( wr->harmful )
        wr->energize_type = action_energize::NONE;
    }
    else if ( auto sf = dynamic_cast<spells::starfire_t*>( *pre ) )
    {
      if ( first_wrath_itr != precombat_action_list.end() && first_wrath_itr != pre - 1 )
        sf->dreamstate = true;

      sf->dreamstate.locked = true;
    }
  }
}

// druid_t::create_buffs ====================================================
template <typename Buff = buffs::druid_buff_t, typename... Args>
static inline buff_t* make_fallback( Args&&... args )
{
  return buff_t::make_buff_fallback<Buff>( std::forward<Args>( args )... );
}

void druid_t::create_buffs()
{
  player_t::create_buffs();

  using namespace buffs;

  // Baseline
  buff.barkskin = make_buff<druid_buff_t>( this, "barkskin", find_class_spell( "Barkskin" ) )
    ->set_cooldown( 0_ms )
    ->set_refresh_behavior( buff_refresh_behavior::DURATION )
    ->set_tick_behavior( buff_tick_behavior::NONE );
  if ( talent.brambles.ok() )
    buff.barkskin->set_tick_behavior( buff_tick_behavior::REFRESH );
  if ( talent.ward_of_the_forest.ok() )
  {
    buff.barkskin->set_stack_change_callback(
      [ this, mul = talent.ward_of_the_forest->effectN( 2 ).percent() ]( buff_t*, int old_, int new_ ) {
        if ( !old_ )
          adjust_health_pct( mul, true );
        else if ( !new_ )
          adjust_health_pct( mul, false );
      } );
  }

  buff.bear_form = make_buff<bear_form_buff_t>( this );

  buff.cat_form = make_buff<cat_form_buff_t>( this );

  buff.dash = make_buff<druid_buff_t>( this, "dash", find_class_spell( "Dash" ) )
    ->set_cooldown( 0_ms )
    ->set_default_value_from_effect_type( A_MOD_INCREASE_SPEED );

  buff.prowl = make_buff<druid_buff_t>( this, "prowl", find_class_spell( "Prowl" ) );

  // Class
  buff.forestwalk =
    make_fallback( talent.forestwalk.ok(), this, "forestwalk", find_trigger( talent.forestwalk ).trigger() )
      ->set_default_value( find_trigger( talent.forestwalk ).percent() );

  buff.heart_of_the_wild_bear = make_fallback( talent.heart_of_the_wild.ok() && specialization() != DRUID_GUARDIAN,
    this, "heart_of_the_wild", apply_override( talent.heart_of_the_wild, spec.bear_form ) )
      ->set_cooldown( 0_ms );
  buff.heart_of_the_wild_bear->set_stack_change_callback(
    [ this, mul = 1.0 + buff.heart_of_the_wild_bear->data().effectN( 1 ).percent() ]
    ( buff_t*, int old_, int new_ ) {
      if ( !old_ )
        adjust_health_pct( mul, true );
      else if ( !new_ )
        adjust_health_pct( mul, false );
    } );

  buff.heart_of_the_wild_owl = make_fallback( talent.heart_of_the_wild.ok(),
    this, "heart_of_the_wild_owl", apply_override( talent.heart_of_the_wild, talent.moonkin_form ) )
      ->set_cooldown( 0_ms );

  buff.ironfur = make_fallback( talent.ironfur.ok(), this, "ironfur", talent.ironfur )
    ->set_default_value_from_effect_type( A_MOD_ARMOR_BY_PRIMARY_STAT_PCT )
    ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
    ->set_cooldown( 0_ms )
    ->add_invalidate( CACHE_AGILITY )
    ->add_invalidate( CACHE_ARMOR );

  buff.innervate = make_fallback( talent.innervate.ok(), this, "innervate", talent.innervate );

  buff.lycaras_teachings_haste =
    make_fallback( talent.lycaras_teachings.ok(), this, "lycaras_teachings_haste", find_spell( 378989 ) )
      ->set_default_value( talent.lycaras_teachings->effectN( 1 ).percent() )
      ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
      ->set_name_reporting( "Haste" );

  buff.lycaras_teachings_crit =
    make_fallback( talent.lycaras_teachings.ok(), this, "lycaras_teachings_crit", find_spell( 378990 ) )
      ->set_default_value( talent.lycaras_teachings->effectN( 1 ).percent() )
      ->set_pct_buff_type( STAT_PCT_BUFF_CRIT )
      ->set_name_reporting( "Crit" );

  buff.lycaras_teachings_vers =
    make_fallback( talent.lycaras_teachings.ok(), this, "lycaras_teachings_vers", find_spell( 378991 ) )
      ->set_default_value( talent.lycaras_teachings->effectN( 1 ).percent() )
      ->set_pct_buff_type( STAT_PCT_BUFF_VERSATILITY )
      ->set_name_reporting( "Vers" );

  buff.lycaras_teachings_mast =
    make_fallback( talent.lycaras_teachings.ok(), this, "lycaras_teachings_mast", find_spell( 378992 ) )
      ->set_default_value( talent.lycaras_teachings->effectN( 1 ).base_value() )
      ->set_pct_buff_type( STAT_PCT_BUFF_MASTERY )
      ->set_name_reporting( "Mastery" );

  buff.matted_fur = make_fallback<matted_fur_buff_t>( talent.matted_fur.ok(), this, "matted_fur" );

  buff.moonkin_form = make_fallback<moonkin_form_buff_t>( talent.moonkin_form.ok(), this, "moonkin_form" );

  buff.tiger_dash = make_fallback( talent.tiger_dash.ok(), this, "tiger_dash", talent.tiger_dash )
    ->set_cooldown( 0_ms )
    ->set_freeze_stacks( true )
    ->set_default_value_from_effect_type( A_MOD_INCREASE_SPEED )
    ->set_tick_callback( []( buff_t* b, int, timespan_t ) {
      b->current_value -= b->data().effectN( 2 ).percent();
    } );

  buff.ursine_vigor =
    make_fallback( talent.ursine_vigor.ok(), this, "ursine_vigor", find_trigger( talent.ursine_vigor ).trigger() )
      ->set_stack_change_callback(
        [ this, mul = 1.0 + find_trigger( talent.ursine_vigor ).percent() ]
        ( buff_t*, int old_, int new_ ) {
          if ( !old_ )
            adjust_health_pct( mul, true );
          else if ( !new_ )
            adjust_health_pct( mul, false );
        } );

  buff.wild_charge_movement = make_fallback( talent.wild_charge.ok(), this, "wild_charge_movement" )
    ->set_proc_callbacks( false );

  // Multi-spec
  // The buff ID in-game is same as the talent, 61336, but the buff effect is in the hidden buff 50322
  buff.survival_instincts =
    make_fallback( talent.survival_instincts.ok(), this, "survival_instincts", talent.survival_instincts )
      ->set_cooldown( 0_ms );

  // Balance buffs
  buff.ascendant_fires = make_fallback( talent.ascendant_eclipses_1.ok(),
    this, "ascendant_fires", find_trigger( talent.ascendant_eclipses_1 ).trigger() );

  buff.ascendant_stars =
    make_fallback( talent.ascendant_eclipses_1.ok(), this, "ascendant_stars", find_spell( 1263382 ) )
      ->set_initial_stack_to_max_stack()
      ->set_consume_all_stacks( false );

  buff.ascendant_stars_starfall =
    make_fallback( talent.ascendant_eclipses_1.ok(), this, "ascendant_stars_starfall", find_spell( 1263401 ) )
      ->set_quiet( true );

  buff.astral_communion = make_fallback( talent.astral_communion.ok(), this, "astral_communion", find_spell( 450599 ) );

  buff.balance_of_all_things_arcane =
    make_fallback( talent.balance_of_all_things.ok(), this, "balance_of_all_things_arcane", find_spell( 394050 ) )
      ->set_reverse( true )
      ->set_refresh_behavior( buff_refresh_behavior::DURATION )
      ->set_name_reporting( "Arcane" );

  buff.balance_of_all_things_nature =
    make_fallback( talent.balance_of_all_things.ok(), this, "balance_of_all_things_nature", find_spell( 394049 ) )
      ->set_reverse( true )
      ->set_refresh_behavior( buff_refresh_behavior::DURATION )
      ->set_name_reporting( "Nature" );

  buff.celestial_alignment =
    make_fallback<celestial_alignment_buff_t>( talent.celestial_alignment.ok() && !talent.incarnation_moonkin.ok(),
      this, "celestial_alignment", spec.celestial_alignment );

  buff.incarnation_moonkin =
    make_fallback<celestial_alignment_buff_t>( talent.incarnation_moonkin.ok(),
      this, "incarnation_chosen_of_elune", spec.incarnation_moonkin );

  buff.ca_inc = talent.incarnation_moonkin.ok() ? buff.incarnation_moonkin : buff.celestial_alignment;

  buff.denizen_of_the_dream =
    make_fallback( talent.denizen_of_the_dream.ok(), this, "denizen_of_the_dream", find_spell( 394076 ) )
      ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
      ->set_max_stack( 10 )
      ->set_proc_callbacks( false );

  buff.dreamstate = make_fallback( talent.natures_grace.ok(), this, "dreamstate", find_spell( 450346 ) )
    ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT )
    ->set_consume_all_stacks( false )
    ->set_initial_stack_to_max_stack();

  buff.eclipse_lunar = make_fallback<eclipse_lunar_t>( talent.eclipse.ok(), this, "eclipse_lunar" );

  buff.eclipse_solar = make_fallback<eclipse_solar_t>( talent.eclipse.ok(), this, "eclipse_solar" );

  buff.elunes_challenge =
    make_fallback( talent.elunes_challenge.ok(), this, "elunes_challenge", find_spell( 1240285 ) );

  buff.fury_of_elune = make_fallback<fury_of_elune_buff_t>( talent.fury_of_elune.ok() || talent.the_light_of_elune.ok(),
    this, "fury_of_elune", find_spell( 202770 ) );  // hardcoded for the light of elune

  buff.lunar_eclipse_override =
    make_fallback( talent.eclipse.ok(), this, "lunar_eclipse_override", find_spell( 1233353 ) )
      ->set_quiet( true );

  buff.sundered_firmament = make_fallback<fury_of_elune_buff_t>( talent.sundered_firmament.ok(),
    this, "sundered_firmament", find_spell( 394108 ) )
      ->set_refresh_behavior( buff_refresh_behavior::EXTEND );

  buff.parting_skies = make_fallback( talent.sundered_firmament.ok(), this, "parting_skies", find_spell( 395110 ) )
    ->set_reverse( true )
    ->set_expire_callback( [ this ]( buff_t*, int, timespan_t ) {
      active.sundered_firmament->execute_on_target( target );
    } );

  buff.natures_balance = make_fallback( talent.natures_balance.ok(), this, "natures_balance", talent.natures_balance )
    ->set_quiet( true )
    ->set_freeze_stacks( true );
  if ( talent.natures_balance.ok() )
  {
    const auto& nb_eff = find_effect( buff.natures_balance, A_PERIODIC_ENERGIZE );
    buff.natures_balance->set_default_value( nb_eff.resource() / nb_eff.period().total_seconds() )
      ->set_tick_callback(
        [ ap = nb_eff.resource(),
          cap = talent.natures_balance->effectN( 2 ).base_value(),
          g = get_gain( "Natures Balance" ),
          this ]
        ( buff_t*, int, timespan_t ) mutable {
          if ( !in_combat )
          {
            if ( resources.current[ RESOURCE_ASTRAL_POWER ] < cap )
              ap *= 3.0;
            else
              ap = 0;
          }
          resource_gain( RESOURCE_ASTRAL_POWER, ap, g );
        } );
  }

  buff.orbit_breaker = make_fallback( talent.orbit_breaker.ok(), this, "orbit_breaker" )
    ->set_quiet( true )
    ->set_proc_callbacks( false )
    ->set_cooldown( talent.orbit_breaker->internal_cooldown() )
    ->set_max_stack( std::max( 1, as<int>( talent.orbit_breaker->effectN( 1 ).base_value() ) ) );

  buff.owlkin_frenzy = make_fallback( specialization() == DRUID_BALANCE && talent.moonkin_form.ok(),
    this, "owlkin_frenzy", find_spell( 157228 ) );

  buff.shooting_stars_moonfire = make_fallback<shooting_stars_buff_t>( talent.shooting_stars.ok(),
    this, "shooting_stars_moonfire", dot_lists.moonfire, active.shooting_stars_moonfire );

  buff.shooting_stars_sunfire = make_fallback<shooting_stars_buff_t>( talent.shooting_stars.ok() && talent.sunfire.ok(),
    this, "shooting_stars_sunfire", dot_lists.sunfire, active.shooting_stars_sunfire );

  // lookup via spell_id since hail of stars can proc solstice without solstice talented
  buff.solstice =
    make_fallback( talent.solstice.ok() || talent.hail_of_stars.ok(), this, "solstice", find_spell( 343648 ) )
      ->set_default_value( find_spell( 343647 )->effectN( 1 ).percent() );

  // lookup via spell_id for convoke
  buff.starfall = make_fallback( spec.starfall->ok() || ( talent.convoke_the_spirits.ok() && talent.moonkin_form.ok() ),
    this, "starfall", find_spell( 191034 ) )
      ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
      ->set_freeze_stacks( true )
      ->set_partial_tick( true )                           // TODO: confirm true?
      ->set_tick_behavior( buff_tick_behavior::REFRESH );  // TODO: confirm true?

  buff.starlord = make_fallback( talent.starlord.ok(), this, "starlord", find_spell( 279709 ) )
    ->set_default_value( talent.starlord->effectN( 1 ).percent() )
    ->set_refresh_behavior( buff_refresh_behavior::DISABLED )
    ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
    ->set_trigger_spell( talent.starlord );

  buff.starweaver_starfall = make_fallback( talent.starweaver.ok(), this, "starweavers_warp", find_spell( 393942 ) )
    ->set_chance( talent.starweaver->effectN( 1 ).percent() )
    ->set_trigger_spell( talent.starweaver );

  buff.starweaver_starsurge = make_fallback( talent.starweaver.ok(), this, "starweavers_weft", find_spell( 393944 ) )
    ->set_chance( talent.starweaver->effectN( 2 ).percent() )
    ->set_trigger_spell( talent.starweaver );

  buff.touch_the_cosmos = make_fallback( talent.touch_the_cosmos.ok(), this, "touch_the_cosmos", find_spell( 450360 ) )
    ->set_trigger_spell( talent.touch_the_cosmos );

  buff.umbral_embrace =
    make_fallback( talent.umbral_embrace.ok(), this, "umbral_embrace", find_trigger( talent.umbral_embrace ).trigger() )
      ->set_trigger_spell( talent.umbral_embrace )
      ->set_chance( UMBRAL_EMBRACE_PROC_CHANCE )
      ->set_default_value( 0 );  // value used to indicate if the proc happened during eclipse (1) or not (0)

  // Feral buffs
  buff.apex_predators_craving = make_fallback( talent.apex_predators_craving.ok(),
    this, "apex_predators_craving", find_trigger( talent.apex_predators_craving ).trigger() );

  buff.berserk_cat = make_fallback( talent.berserk_cat.ok(), this, "berserk_cat", spec.berserk_cat )
    ->set_name_reporting( "berserk" )
    ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
      if ( !new_ )
      {
        gain.overflowing_power->overflow[ RESOURCE_COMBO_POINT ] += buff.overflowing_power->check();
        buff.overflowing_power->expire();
      }
    } );

  buff.hunger_for_battle =
    make_fallback( talent.hunger_for_battle.ok(), this, "hunger_for_battle", find_spell( 1244553 ) );

  buff.incarnation_cat =
    make_fallback( talent.incarnation_cat.ok(), this, "incarnation_avatar_of_ashamane", talent.incarnation_cat )
      ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_RESOURCE_COST_1 )
      ->set_stack_change_callback(
        [ this, ag_dur = timespan_t::from_seconds( find_spell( 421440 )->effectN( 1 ).base_value() ) ]
        ( buff_t*, int, int new_ ) {
          if ( !new_ )
          {
            gain.overflowing_power->overflow[ RESOURCE_COMBO_POINT ] += buff.overflowing_power->check();
            buff.overflowing_power->expire();
          }
        } );

  buff.b_inc_cat = talent.incarnation_cat.ok() ? buff.incarnation_cat : buff.berserk_cat;
  buff.b_inc_cat->set_cooldown( 0_ms )
    ->set_tick_callback(
      [ this,
        cp = find_effect( find_trigger( buff.b_inc_cat ).trigger(), E_ENERGIZE ).resource( RESOURCE_COMBO_POINT ),
        gain = get_gain( buff.b_inc_cat->name() ) ]
      ( buff_t*, int, timespan_t ) {
        resource_gain( RESOURCE_COMBO_POINT, cp, gain );
      } );

  buff.chomp_enabler =
    make_fallback( talent.chomp.ok(), this, "chomp_enabler", spec.chomp_controller->effectN( 1 ).trigger() );

  buff.clearcasting_cat = make_fallback( talent.omen_of_clarity_cat.ok(),
    this, "clearcasting_cat", find_trigger( talent.omen_of_clarity_cat ).trigger() )
      ->set_name_reporting( "clearcasting" )
      ->set_consume_all_stacks( false );

  buff.coiled_to_spring = make_fallback( talent.coiled_to_spring.ok(), this, "coiled_to_spring", find_spell( 449538 ) );

  buff.flash_of_clarity =
    make_fallback( sets->has_set_bonus( DRUID_FERAL, MID1, B2 ), this, "flash_of_clarity", find_spell( 1272262 ) )
      ->set_default_value_from_effect_type( A_MOD_ALL_CRIT_CHANCE )
      ->set_pct_buff_type( STAT_PCT_BUFF_CRIT );

  buff.frantic_momentum = make_fallback( talent.frantic_momentum.ok(),
    this, "frantic_momentum", find_trigger( talent.frantic_momentum ).trigger() )
      ->set_cooldown( talent.frantic_momentum->internal_cooldown() )
      ->set_chance( find_trigger( talent.frantic_momentum ).percent() )
      ->set_trigger_spell( talent.frantic_momentum )
      ->set_default_value_from_effect_type( A_HASTE_ALL )
      ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );

  buff.incarnation_cat_prowl = make_fallback( talent.incarnation_cat.ok(),
    this, "incarnation_avatar_of_ashamane_prowl", find_effect( talent.incarnation_cat, E_TRIGGER_SPELL ).trigger() )
      ->set_name_reporting( "Prowl" );

  buff.overflowing_power = make_fallback( talent.berserk_cat.ok(), this, "overflowing_power", find_spell( 405189 ) )
    ->set_expire_callback( [ this ]( buff_t*, int s, timespan_t ) {
      resource_gain( RESOURCE_COMBO_POINT, s, get_gain( "Overflowing Power" ) );
    } );

  // TODO: confirm this is how the value is calculated
  auto predator_buff = find_trigger( talent.predator ).trigger();
  buff.predator = make_fallback( talent.predator.ok(), this, "predator", predator_buff )
    ->set_quiet( true )
    ->set_tick_zero( true )
    ->set_freeze_stacks( true )  // prevent buff_t::bump it buff_t::tick_t overwriting current value
    ->set_chance( 1.0 )  // avoid assert
    ->set_tick_callback( [ this, mul = predator_buff->effectN( 2 ).percent() ]( buff_t* b, int, timespan_t ) {
      b->current_value = ( 1.0 / composite_melee_haste() - 1.0 ) * mul;
    } );

  buff.predatory_swiftness =
    make_fallback( spec.predatory_swiftness->ok(), this, "predatory_swiftness", find_spell( 69369 ) )
      ->set_chance( spec.predatory_swiftness->effectN( 3 ).percent() )
      ->set_trigger_spell( spec.predatory_swiftness );

  buff.savage_fury =
    make_fallback( talent.savage_fury.ok(), this, "savage_fury", find_trigger( talent.savage_fury ).trigger() )
      ->set_default_value_from_effect_type( A_HASTE_ALL )
      ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );

  buff.stalking_predator = make_fallback( talent.unseen_predator_3.ok(),
    this, "stalking_predator", find_trigger( talent.unseen_predator_3 ).trigger() )
      ->set_initial_stack_to_max_stack()
      ->set_consume_all_stacks( false );

  buff.sudden_ambush =
    make_fallback( talent.sudden_ambush.ok(), this, "sudden_ambush", find_trigger( talent.sudden_ambush ).trigger() )
      ->set_chance( talent.sudden_ambush->effectN( 1 ).percent() )
      ->set_trigger_spell( talent.sudden_ambush );

  buff.tigers_fury = make_fallback( talent.tigers_fury.ok(), this, "tigers_fury", talent.tigers_fury )
    ->set_cooldown( 0_ms );

  buff.tigers_tenacity = make_fallback( talent.tigers_tenacity.ok(),
    this, "tigers_tenacity", find_trigger( talent.tigers_tenacity ).trigger() )
      ->set_reverse( true )
      ->set_consume_all_stacks( false );
  buff.tigers_tenacity->set_stack_change_callback(
    [ this, g = get_gain( "Tiger's Tenacity" ),
      amt = find_effect( find_trigger( buff.tigers_tenacity ).trigger(), E_ENERGIZE ).resource() ]
    ( buff_t*, int old_, int new_ ) {
      if ( old_ > new_ )
        resource_gain( RESOURCE_COMBO_POINT, amt, g );
    } );

  buff.unseen_predators_craving =
    make_fallback( talent.unseen_predator_2.ok(), this, "unseen_predators_craving", find_spell( 1263939 ) )
      ->set_trigger_spell( talent.unseen_predator_2 )
      ->set_refresh_behavior( buff_refresh_behavior::EXTEND );

  // Guardian buffs
  buff.after_the_wildfire = make_fallback( talent.after_the_wildfire.ok(),
    this, "after_the_wildfire", find_trigger( talent.after_the_wildfire ).trigger() )
      // ignore the imprecise 30 stack @ 10 rage / stack tracking done in game and use current value to track
      ->set_default_value( talent.after_the_wildfire->effectN( 2 ).base_value() )
      ->set_max_stack( 1 );

  buff.berserk_bear = make_fallback( talent.berserk_bear.ok(), this, "berserk_bear", talent.berserk_bear )
    ->set_name_reporting( "berserk" )
    ->set_cooldown( 0_ms )
    ->set_refresh_behavior( buff_refresh_behavior::EXTEND );

  buff.incarnation_bear =
    make_fallback( talent.incarnation_bear.ok(), this, "incarnation_guardian_of_ursoc", talent.incarnation_bear )
      ->set_cooldown( 0_ms )
      ->set_refresh_behavior( buff_refresh_behavior::EXTEND )
      ->set_stack_change_callback(
        [ this, mul = 1.0 + find_effect( talent.incarnation_bear, A_INCREASE_HEALTH_PCT ).percent() ]
        ( buff_t*, int old_, int new_ ) {
          if ( !old_ )
            adjust_health_pct( mul, true );
          else if ( !new_ )
            adjust_health_pct( mul, false );
        } );

  buff.b_inc_bear = talent.incarnation_bear.ok() ? buff.incarnation_bear : buff.berserk_bear;

  buff.blood_frenzy =
    make_fallback( talent.blood_frenzy.ok(), this, "blood_frenzy_buff", talent.blood_frenzy )
      ->set_quiet( true )
      ->set_tick_zero( true )
      ->set_tick_callback(
        [ this,
          g = get_gain( "Blood Frenzy" ),
          cap = talent.blood_frenzy->effectN( 1 ).base_value(),
          rage = find_effect( find_spell( 203961 ), E_ENERGIZE ).resource( RESOURCE_RAGE ) ]
        ( buff_t*, int, timespan_t ) {
          if ( auto n = as<double>( dot_lists.thrash.size() ) )
            resource_gain( RESOURCE_RAGE, std::min( cap, n ) * rage, g );
        } );

  buff.brambles = make_fallback<brambles_buff_t>( talent.brambles.ok(), this, "brambles" );

  buff.bristling_fur = make_fallback( talent.bristling_fur.ok(), this, "bristling_fur", talent.bristling_fur )
    ->set_cooldown( 0_ms );

  buff.celestial_might = make_fallback( sets->has_set_bonus( DRUID_GUARDIAN, MID1, B4 ),
    this, "celestial_might", find_trigger( sets->set( DRUID_GUARDIAN, MID1, B4 ) ).trigger() );

  buff.dream_guide =
    make_fallback( talent.dream_guide.ok(), this, "dream_guide", find_trigger( talent.dream_guide ).trigger() )
      ->set_consume_all_stacks( false );

  buff.dream_of_cenarius = make_fallback( talent.dream_of_cenarius_bear.ok(),
    this, "dream_of_cenarius", talent.dream_of_cenarius_bear->effectN( 1 ).trigger() );

  buff.echo_of_ironfur =
    make_fallback( talent.wild_guardian_1.ok(), this, "echo_of_ironfur", find_spell( 1269633 ) )
      ->set_default_value( find_effect( talent.ironfur, A_MOD_ARMOR_BY_PRIMARY_STAT_PCT ).percent() *
                           talent.wild_guardian_1->effectN( 1 ).percent() )
      ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
      ->add_invalidate( CACHE_AGILITY )
      ->add_invalidate( CACHE_ARMOR );

  buff.elunes_favored = make_fallback( talent.elunes_favored.ok(), this, "elunes_favored", spec.elunes_favored )
    ->set_quiet( true )
    ->set_freeze_stacks( true )
    ->set_default_value( 0 )
    ->set_tick_callback( [ this ]( buff_t* b, int, timespan_t ) {
      if ( b->check_value() )
      {
        active.elunes_favored_heal->execute_on_target( this, b->check_value() );
        b->current_value = 0;
      }
    } );

  buff.galactic_guardian = make_fallback( talent.galactic_guardian.ok() && !talent.red_moon.ok(),
    this, "galactic_guardian", find_spell( 213708 ) );

  buff.gift_of_an_ancient_guardian =
    make_fallback( talent.gift_of_an_ancient_guardian.ok(), this, "gift_of_an_ancient_guardian", find_spell( 1251877 ) )
      ->set_default_value_from_effect_type( A_MOD_MASTERY_PCT )
      ->set_pct_buff_type( STAT_PCT_BUFF_MASTERY );
  if ( talent.gift_of_an_ancient_guardian.ok() )
  {
    buff.ironfur->set_stack_change_callback( [ this ]( auto, int old_, int new_ ) {
      if ( !old_ )
        buff.gift_of_an_ancient_guardian->trigger();
      else if ( !new_ )
        buff.gift_of_an_ancient_guardian->expire();
    } );
  }

  buff.gift_of_frenzied_regeneration =
    make_fallback( talent.wild_guardian_1.ok() && ( talent.berserk_bear.ok() || talent.incarnation_bear.ok() ),
      this,"gift_of_frenzied_regeneration", find_spell( 1269661 ) )
        ->set_initial_stack_to_max_stack()
        ->set_consume_all_stacks( false );

  buff.gift_of_ironfur =
    make_fallback( talent.wild_guardian_1.ok() && ( talent.berserk_bear.ok() || talent.incarnation_bear.ok() ),
      this, "gift_of_ironfur", find_spell( 1269659 ) )
        ->set_initial_stack_to_max_stack()
        ->set_consume_all_stacks( false );

  buff.gift_of_maul =
    make_fallback( talent.wild_guardian_1.ok() && ( talent.berserk_bear.ok() || talent.incarnation_bear.ok() ),
      this, "gift_of_maul", find_spell( 1269660 ) )
        ->set_initial_stack_to_max_stack()
        ->set_consume_all_stacks( false );

  buff.gore = make_fallback( talent.gore.ok(), this, "gore", find_spell( 93622 ) )
    ->set_trigger_spell( talent.gore );

  buff.gory_fur = make_fallback( talent.gory_fur.ok(), this, "gory_fur", find_trigger( talent.gory_fur ).trigger() )
    ->set_trigger_spell( talent.gory_fur );

  buff.guardian_of_elune = make_fallback( talent.guardian_of_elune.ok(),
    this, "guardian_of_elune", find_trigger( talent.guardian_of_elune ).trigger() )
      ->set_trigger_spell( talent.guardian_of_elune );

  buff.lunar_beam = make_fallback( talent.lunar_beam.ok(), this, "lunar_beam", talent.lunar_beam )
    ->set_cooldown( 0_ms )
    ->set_default_value_from_effect_type( A_MOD_MASTERY_PCT )
    ->set_pct_buff_type( STAT_PCT_BUFF_MASTERY );
  if ( bugs && talent.lunar_beam.ok() && talent.the_eternal_moon.ok() )
    buff.lunar_beam->modify_default_value( 0.5 );  // modify mastery% (318) seems to round

  buff.lunar_wrath =
    make_fallback( talent.galactic_guardian.ok() && talent.red_moon.ok(), this, "lunar_wrath", find_spell( 1253600 ) )
      ->set_consume_all_stacks( false );

  buff.natural_resilience = make_fallback<druid_absorb_buff_t>( talent.natural_resilience.ok(),
    this, "natural_resilience", find_spell( 1278800 ) );
  if ( talent.natural_resilience.ok() )
    debug_cast<druid_absorb_buff_t*>( buff.natural_resilience )->set_cumulative( true );

  buff.persistence = make_fallback( talent.persistence.ok(), this, "persistence", find_spell( 1251407 ) )
    ->set_reverse( true );

  buff.sundering_roar = make_fallback( talent.sundering_roar.ok(), this, "sundering_roar", talent.sundering_roar )
    ->set_cooldown( 0_ms )
    ->set_default_value_from_effect_type( A_ADD_FLAT_MODIFIER, P_MAX_STACKS )
    ->set_expire_callback( [ this, orig_max_stack = as<int>( spec.thrash_bleed->max_stacks() ) ]( auto, auto, auto ) {
      for ( auto dot : dot_lists.thrash )
      {
        if ( auto excess = dot->current_stack() - orig_max_stack; excess > 0 )
        {
          auto damage = dot->tick_damage_over_remaining_time( excess );
          active.sundering_roar_thrash->execute_on_target( dot->target, damage );
          dot->decrement( excess );
          dot->max_stack = orig_max_stack;
        }
      }
    } );

  buff.ursocs_fury =
    make_fallback<druid_absorb_buff_t>( talent.ursocs_fury.ok(), this, "ursocs_fury", find_spell( 372505 ) );
  if ( talent.ursocs_fury.ok() )
    debug_cast<druid_absorb_buff_t*>( buff.ursocs_fury )->set_cumulative( true );

  buff.waking_nightmare = make_fallback( talent.waking_nightmare.ok(),
    this, "waking_nightmare", find_trigger( talent.waking_nightmare ).trigger() )
      ->set_proc_callbacks( false );

  buff.wild_guardian = make_fallback( talent.wild_guardian_1.ok(), this, "wild_guardian", find_spell( 1269616 ) );

  // Restoration buffs
  buff.abundance = make_fallback( talent.abundance.ok(), this, "abundance", find_spell( 207640 ) )
    ->set_duration( 0_ms );

  buff.call_of_the_elder_druid =
    make_fallback( talent.call_of_the_elder_druid.ok(), this, "call_of_the_elder_druid", find_spell( 319454 ) )
      ->set_cooldown( 0_ms )
      ->set_duration( timespan_t::from_seconds( talent.call_of_the_elder_druid->effectN( 1 ).base_value() ) );
  buff.call_of_the_elder_druid->set_tick_callback(
    [ this,
      a = buff.call_of_the_elder_druid->data().effectN( 14 ).base_value(),
      g = get_gain( "Call of the Elder Druid" ) ]
    ( auto, auto, auto ) {
      if ( form == CAT_FORM )
        resource_gain( RESOURCE_COMBO_POINT, a, g );
    } );

  buff.clearcasting_tree = make_fallback( talent.omen_of_clarity_tree.ok(),
    this, "clearcasting_tree", find_trigger( talent.omen_of_clarity_tree ).trigger() )
      ->set_chance( find_trigger( talent.omen_of_clarity_tree ).percent() )
      ->set_name_reporting( "clearcasting" );

  buff.incarnation_tree =
    make_fallback( talent.incarnation_tree.ok(), this, "incarnation_tree_of_life", find_spell( 5420 ) )
      ->set_duration( find_spell( 117679 )->duration() );  // 117679 is the generic incarnation shift proxy spell

  buff.natures_swiftness =
    make_fallback( talent.natures_swiftness.ok(), this, "natures_swiftness", talent.natures_swiftness )
      ->set_cooldown( 0_ms );

  buff.oath_of_the_elder_druid =
    make_fallback( talent.call_of_the_elder_druid.ok(), this, "oath_of_the_elder_druid", find_spell( 338643 ) );

  buff.soul_of_the_forest_tree =
    make_fallback( talent.soul_of_the_forest_tree.ok(), this, "soul_of_the_forest_tree", find_spell( 114108 ) )
      ->set_name_reporting( "soul_of_the_forest" );

  buff.yseras_gift = make_fallback( talent.yseras_gift.ok(), this, "yseras_gift_driver", talent.yseras_gift )
    ->set_quiet( true )
    ->set_tick_zero( true )
    ->set_tick_callback( [this]( buff_t*, int, timespan_t ) {
        active.yseras_gift->schedule_execute();
      } );

  // Hero talents
  buff.blooming_infusion_damage =
    make_fallback( talent.blooming_infusion.ok(), this, "blooming_infusion_damage", find_spell( 429474 ) )
      ->set_name_reporting( "Damage" );

  buff.blooming_infusion_damage_counter =
    make_fallback( talent.blooming_infusion.ok(), this, "blooming_infusion_damage_counter" )
      //->set_quiet( true )
      ->set_proc_callbacks( false )
      ->set_name_reporting( "Blooming Infusion Damage Counter" )
      ->set_max_stack( as<int>( talent.blooming_infusion->effectN( 1 ).base_value() ) )
      ->set_expire_at_max_stack( true )
      ->set_trigger_spell( talent.blooming_infusion );

  buff.blooming_infusion_heal =
    make_fallback( talent.blooming_infusion.ok(), this, "blooming_infusion_heal", find_spell( 429438 ) )
      ->set_name_reporting( "Heal" );

  buff.blooming_infusion_heal_counter =
    make_fallback( talent.blooming_infusion.ok(), this, "blooming_infusion_heal_counter" )
      //->set_quiet( true )
      ->set_proc_callbacks( false )
      ->set_name_reporting( "Blooming Infusion Heal Counter" )
      ->set_max_stack( as<int>( talent.blooming_infusion->effectN( 1 ).base_value() ) )
      ->set_expire_at_max_stack( true )
      ->set_trigger_spell( talent.blooming_infusion )
      ->set_expire_callback( [ this ]( buff_t*, int, timespan_t ) {
        buff.blooming_infusion_heal->trigger();
      } );

  buff.boundless_moonlight_heal = make_fallback( talent.boundless_moonlight.ok() && talent.lunar_beam.ok(),
    this, "boundless_moonlight_heal", find_spell( 425217 ) )
      ->set_quiet( true )
      ->set_freeze_stacks( true )
      ->set_default_value( 0 )
      ->set_tick_callback( [ this ]( buff_t* b, int, timespan_t ) {
        if ( b->check_value() )
        {
          active.boundless_moonlight_heal->execute_on_target( this, b->check_value() );
          b->current_value = 0;
        }
      } );

  buff.cenarius_might =
    make_fallback( talent.cenarius_might.ok(), this, "cenarius_might", find_trigger( talent.cenarius_might ).trigger() )
      ->set_default_value_from_effect_type( A_HASTE_ALL )
      ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );

  buff.dream_burst = make_fallback( talent.dream_surge.ok(), this, "dream_burst", find_spell( 433832 ) )
    ->set_consume_all_stacks( false );

  buff.harmony_of_the_grove = make_fallback( talent.harmony_of_the_grove.ok(),
    this, "harmony_of_the_grove", find_spell( specialization() == DRUID_RESTORATION ? 428737 : 428735 ) )
      ->set_cooldown( 0_ms );

  buff.implant = make_fallback( talent.implant.ok(), this, "implant", find_spell( 455496 ) )
    ->set_cooldown( talent.implant->internal_cooldown() );
  if ( talent.implant.ok() )
  {
    buff.tigers_fury->set_stack_change_callback( [ this ]( auto, int old_, int new_ ) {
      if ( !old_ || !new_ )
        buff.implant->trigger();
    } );
  }

  buff.killing_strikes = make_fallback( talent.killing_strikes.ok(),
    this, "killing_strikes", find_trigger( talent.killing_strikes ).trigger() )
      ->set_trigger_spell( talent.killing_strikes )
      ->set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE )
      ->add_invalidate( CACHE_ARMOR );

  buff.killing_strikes_combat =
    make_fallback( talent.killing_strikes.ok(), this, "killing_strikes_combat", find_spell( 441827 ) )
      ->set_quiet( true );

  buff.ravage_fb = make_fallback( talent.ravage.ok() && specialization() == DRUID_FERAL,
    this, "ravage", find_spell( 441585 ) )
      ->set_trigger_spell( talent.ravage );

  buff.ravage_maul = make_fallback( talent.ravage.ok() && specialization() == DRUID_GUARDIAN,
    this, "ravage", find_spell( 441602 ) )
      ->set_trigger_spell( talent.ravage );

  buff.root_network = make_fallback( talent.root_network.ok(), this, "root_network", find_spell( 439887 ) )
    // TODO: confirm updating behavior where all stacks are decreased at once then recalibrated on tick
    ->set_period( 0_ms )
    ->set_tick_behavior( buff_tick_behavior::NONE );

  buff.ruthless_aggression = make_fallback( talent.ruthless_aggression.ok(),
    this, "ruthless_aggression", find_trigger( talent.ruthless_aggression ).trigger() )
      ->set_trigger_spell( talent.ruthless_aggression )
      ->add_invalidate( CACHE_AUTO_ATTACK_SPEED );

  buff.strategic_infusion = make_fallback( talent.strategic_infusion.ok() && talent.tigers_fury.ok(),
    this, "strategic_infusion", find_spell( 439891 ) );

  buff.sylvan_beckoning =
    make_fallback( talent.sylvan_beckoning.ok(), this, "sylvan_beckoning", find_spell( 1264618 ) );

  buff.treants_of_the_moon = make_fallback<treants_of_the_moon_buff_t>(
    talent.treants_of_the_moon.ok() && ( talent.force_of_nature.ok() || talent.grove_guardians.ok() ),
      this, "treants_of_the_moon" )
        ->set_quiet( true )
        ->set_period( 1.5_s )
        ->set_freeze_stacks( true )
        ->set_tick_callback( [ this ]( buff_t* b, int, timespan_t ) {
          for ( auto pet : static_cast<treants_of_the_moon_buff_t*>( b )->data )
          {
            if ( pet->mf_cd->up() )
            {
              pet->mf_cd->start();

              auto mf = active.treants_of_the_moon_mf;
              if ( auto tar = get_smart_target( mf->target_list(), &druid_td_t::dots_t::moonfire, nullptr, 0.0, true ) )
                mf->execute_on_target( tar );
            }
          }
        } );

  buff.feline_potential = make_fallback( talent.wildpower_surge.ok() && specialization() == DRUID_GUARDIAN,
    this, "feline_potential", find_spell( 441702 ) )
      ->set_stack_change_callback(
        [ this, g = get_gain( "Wildpower Surge" ), amt = find_effect( find_spell( 441704 ), E_ENERGIZE ).resource() ]
        ( buff_t*, int, int new_ ) {
          if ( new_ )
            resource_gain( RESOURCE_COMBO_POINT, amt, g );
        } );

  buff.feline_potential_counter = make_fallback( talent.wildpower_surge.ok() && specialization() == DRUID_GUARDIAN,
    this, "feline_potential_counter", find_spell( 441701 ) )
      ->set_trigger_spell( talent.wildpower_surge )
      ->set_cooldown( talent.wildpower_surge->internal_cooldown() );

  buff.ursine_potential = make_fallback( talent.wildpower_surge.ok() && specialization() == DRUID_FERAL,
    this, "ursine_potential", find_spell( 441698 ) )
      ->set_stack_change_callback(
        [ this, g = get_gain( "Wildpower Surge" ), amt = find_effect( find_spell( 442562 ), E_ENERGIZE ).resource() ]
        ( buff_t*, int, int new_ ) {
          if ( !new_ )
            resource_gain( RESOURCE_RAGE, amt, g );
        } );

  buff.ursine_potential_counter = make_fallback( talent.wildpower_surge.ok() && specialization() == DRUID_FERAL,
    this, "ursine_potential_counter", find_spell( 441695 ) )
      ->set_trigger_spell( talent.wildpower_surge )
      ->set_cooldown( talent.wildpower_surge->internal_cooldown() );

  buff.wildshape_mastery = make_fallback( talent.wildshape_mastery.ok() && specialization() == DRUID_FERAL,
    this, "wildshape_mastery", find_spell( 441685 ) );

  // call this here to ensure all buffs have been created
  parse_player_effects();
}

// Create active actions ====================================================
void druid_t::create_actions()
{
  using namespace cat_attacks;
  using namespace bear_attacks;
  using namespace spells;
  using namespace heals;
  using namespace auto_attacks;

  // Melee Attacks
  if ( !caster_melee_attack )
    caster_melee_attack = new caster_melee_t( this );

  if ( !cat_melee_attack )
  {
    init_beast_weapon( cat_weapon, 1.0 );
    cat_melee_attack = new cat_melee_t( this );
  }

  if ( !bear_melee_attack )
  {
    init_beast_weapon( bear_weapon, 2.5 );
    bear_melee_attack = new bear_melee_t( this );
  }

  // General
  active.shift_to_caster = get_secondary_action<cancel_form_t>( "cancel_form_shift" );
  active.shift_to_caster->dual = true;

  active.shift_to_bear = get_secondary_action<bear_form_t>( "bear_form_shift" );
  active.shift_to_bear->dual = true;

  active.shift_to_cat = get_secondary_action<cat_form_t>( "cat_form_shift" );
  active.shift_to_cat->dual = true;

  if ( talent.moonkin_form.ok() )
  {
    active.shift_to_moonkin = get_secondary_action<moonkin_form_t>( "moonkin_form_shift" );
    active.shift_to_moonkin->dual = true;
  }

  if ( talent.heart_of_the_wild.ok() )
  {
    if ( specialization() == DRUID_GUARDIAN || specialization() == DRUID_RESTORATION )
    {
      // set up cat
      auto _cat = get_secondary_action<druid_attack_t<melee_attack_t>, false>(
        "heart_of_the_wild_cat", this, apply_override( talent.heart_of_the_wild, spec.cat_form ), flag_e::NONE );
      _cat->name_str_reporting = "Cat";

      _cat->tick_action =
        get_secondary_action<feral_frenzy_t::feral_frenzy_tick_t>( "heart_of_the_wild_cat_tick", flag_e::NONE );
      _cat->tick_action->base_multiplier *= talent.heart_of_the_wild->effectN( 2 ).percent();

      active.hotw_cat = _cat;

      // set up owl
      auto _owl = get_secondary_action<action_t, false>( "heart_of_the_wild_owl", action_e::ACTION_OTHER,
        "heart_of_the_wild_owl", this, &buff.heart_of_the_wild_owl->data() );
      _owl->name_str_reporting = "Moonkin";

      auto _owl_driver = get_secondary_action<starfall_t::starfall_driver_t>(
        "heart_of_the_wild_owl_driver", find_trigger( _owl ).trigger(), find_spell( 1286243 ), flag_e::NONE );
      _owl_driver->damage->name_str_reporting = "HotW";

      replace_stats( _owl, _owl_driver, false );
      replace_stats( _owl, _owl_driver->damage );

      buff.heart_of_the_wild_owl->set_tick_callback( [ _owl_driver ]( buff_t*, int, timespan_t ) {
        _owl_driver->execute();
      } );

      active.hotw_owl = _owl;
    }
  }

  // Balance
  if ( talent.ascendant_eclipses_3.ok() )
  {
    active.ascendant_eclipses =
      new action_t( action_e::ACTION_OTHER, "ascendant_eclipses", this, talent.ascendant_eclipses_3 );

    auto lunar = get_secondary_action<lunar_bolt_t>( "lunar_bolt" );
    active.ascendant_eclipses->add_child( lunar );
    active.lunar_bolt = lunar;

    auto solar = get_secondary_action<solar_bolt_t>( "solar_bolt" );
    active.ascendant_eclipses->add_child( solar );
    active.solar_bolt = solar;
  }

  if ( talent.denizen_of_the_dream.ok() )
    active.denizen_of_the_dream = new denizen_of_the_dream_t( this );

  if ( talent.new_moon.ok() )
  {
    active.moons = new action_t( action_e::ACTION_OTHER, "moons_talent", this, talent.new_moon );
    active.moons->name_str_reporting = "Talent";
  }

  if ( talent.orbital_strike.ok() )
    active.orbital_strike = get_secondary_action<orbital_strike_t>( "orbital_strike" );

  if ( talent.shooting_stars.ok() )
  {
    auto shs_proxy = new action_t( action_e::ACTION_OTHER, "shooting_stars", this, talent.shooting_stars );

    auto mf = get_secondary_action<shooting_stars_t>( "shooting_stars_moonfire", spec.shooting_stars_dmg );
    mf->name_str_reporting = "Moonfire";
    shs_proxy->add_child( mf );
    active.shooting_stars_moonfire = mf;

    auto sf = get_secondary_action<shooting_stars_t>( "shooting_stars_sunfire", spec.shooting_stars_dmg );
    sf->name_str_reporting = "Sunfire";
    shs_proxy->add_child( sf );
    active.shooting_stars_sunfire = sf;

    if ( talent.orbit_breaker.ok() )
    {
      auto fm = get_secondary_action<full_moon_t>( "full_moon_orbit", find_spell( 274283 ), flag_e::ORBIT );
      fm->s_data_reporting = talent.orbit_breaker;
      fm->name_str_reporting = "orbit_breaker";
      fm->base_multiplier = talent.orbit_breaker->effectN( 2 ).percent();
      fm->energize_amount *= talent.orbit_breaker->effectN( 2 ).percent();
      fm->background = true;
      fm->proc = true;
      shs_proxy->add_child(fm );
      active.orbit_breaker = fm;
    }
  }

  if ( talent.sundered_firmament.ok() )
  {
    auto firmament = get_secondary_action<fury_of_elune_t>(
      "sundered_firmament", find_spell( 394106 ), flag_e::FIRMAMENT, find_spell( 394111 ), buff.sundered_firmament );
    firmament->damage->base_multiplier = talent.sundered_firmament->effectN( 1 ).percent();
    firmament->s_data_reporting = talent.sundered_firmament;
    firmament->background = true;
    firmament->proc = true;
    active.sundered_firmament = firmament;
  }

  if ( talent.sunseeker_mushroom.ok() )
  {
    auto sunseeker = get_secondary_action<wild_mushroom_t>(
      "sunseeker_mushroom", find_trigger( talent.sunseeker_mushroom ).trigger(), flag_e::NONE, find_spell( 1280213 ) );
    sunseeker->background = true;
    sunseeker->proc = true;
    active.sunseeker_mushroom = sunseeker;
  }

  if ( sets->has_set_bonus( DRUID_BALANCE, MID1, B4 ) )
    active.shooting_stars_mid1 = get_secondary_action<shooting_stars_mid1_t>( "shooting_stars_exploding" );

  // Feral
  if ( talent.unseen_predator_1.ok() )
  {
    active.unseen_slash = get_secondary_action<unseen_slash_t>( "unseen_slash" );
    active.unseen_swipe = get_secondary_action<unseen_swipe_t>( "unseen_swipe" );
  }

  // Guardian
  if ( talent.after_the_wildfire.ok() )
  {
    auto heal = get_secondary_action<druid_heal_t>( "after_the_wildfire", this, find_spell( 371982 ) );
    heal->proc = true;
    heal->aoe = as<int>( talent.after_the_wildfire->effectN( 3 ).base_value() );
    active.after_the_wildfire_heal = heal;
  }

  if ( talent.brambles.ok() )
    active.brambles_reflect = get_secondary_action<brambles_reflect_t>( "brambles" );

  if ( talent.elunes_favored.ok() )
    active.elunes_favored_heal = get_secondary_action<elunes_favored_heal_t>( "elunes_favored" );

  if ( talent.flashing_claws.ok() )
  {
    auto flash = get_secondary_action<thrash_t>( "flashing_claws", flag_e::FLASHING );
    flash->proc = true;
    flash->s_data_reporting = talent.flashing_claws;
    flash->name_str_reporting = "flashing_claws";
    active.thrash_flashing = flash;
  }

  if ( talent.galactic_guardian.ok() )
  {
    auto gg = get_secondary_action<moonfire_t>( "galactic_guardian", flag_e::GALACTIC );
    gg->s_data_reporting = talent.galactic_guardian;
    gg->proc = true;
    active.galactic_guardian = gg;

    if ( talent.red_moon.ok() )
    {
      auto s_data = find_spell( 1253609 );  // hardcoded

      auto lw = get_secondary_action<druid_spell_t>( "lunar_wrath", this, s_data );
      lw->proc = true;
      active.lunar_wrath = lw;
      active.galactic_guardian->add_child( lw );

      auto lwh = get_secondary_action<druid_heal_t>( "lunar_wrath_heal", this, s_data );
      lwh->proc = true;
      lwh->name_str_reporting = "Heal";
      active.lunar_wrath_heal = lwh;
    }
  }

  if ( talent.memory_of_ysera.ok() )
  {
    active.memory_of_ysera_heal = get_secondary_action<druid_heal_t>( "memory_of_ysera", this, find_spell( 1250913 ) );
    active.memory_of_ysera_heal->proc = true;
  }

  if ( talent.sundering_roar.ok() )
  {
    auto sr_thrash = get_secondary_action<bear_attack_t>( "sundering_roar_thrash", this, find_spell( 1253982 ) );
    sr_thrash->proc = true;
    sr_thrash->name_str_reporting = "Thrash";
    active.sundering_roar_thrash = sr_thrash;
  }

  if ( talent.waking_nightmare.ok() )
  {
    active.waking_nightmare = new waking_nightmare_t( this );

    auto pulse = get_secondary_action<druid_spell_t>( "waking_nightmare_pulse", this, find_spell( 1253488 ) );
    pulse->proc = true;
    pulse->name_str_reporting = "Pulse";
    active.waking_nightmare_pulse = pulse;
    active.waking_nightmare->add_child( pulse );
  }

  if ( talent.wild_guardian_1.ok() )
  {
    active.echo_of_maul = get_secondary_action<echo_of_maul_t<maul_base_t>>( "echo_of_maul", find_spell( 1269648 ) );
    active.echo_of_raze = get_secondary_action<echo_of_maul_t<raze_base_t>>( "echo_of_raze", find_spell( 1269972 ) );
    active.echo_of_ravage = get_secondary_action<echo_of_maul_t<ravage_t>>( "echo_of_ravage", find_spell( 1269973 ) );
    active.echo_of_ravage->name_str_reporting.clear();
  }

  if ( talent.wild_guardian_3.ok() )
    active.vicious_brambles = get_secondary_action<vicious_brambles_t>( "vicious_brambles", this ); 

  // Restoration
  if ( talent.yseras_gift.ok() )
    active.yseras_gift = get_secondary_action<yseras_gift_t>( "yseras_gift" );

  // Hero talents
  if ( talent.boundless_moonlight.ok() && talent.lunar_beam.ok() )
  {
    auto heal = get_secondary_action<druid_residual_action_t<druid_heal_t>>( "boundless_moonlight_heal", this,
                                                                             find_spell( 425206 ) );
    heal->background = heal->proc = true;
    active.boundless_moonlight_heal = heal;
  }

  if ( talent.bursting_growth.ok() && talent.thriving_growth.ok() )
  {
    auto burst = get_secondary_action<bursting_growth_t>( "bursting_growth" );

    if ( talent.rampancy.ok() )
    {
      auto ramp = get_secondary_action<bursting_growth_t>( "rampancy" );
      ramp->s_data_reporting = talent.rampancy;
      ramp->base_multiplier = talent.rampancy->effectN( 3 ).percent();
      burst->add_child( ramp );
      active.rampancy = ramp;
    }

    active.bursting_growth = burst;
  }

  if ( talent.dream_surge.ok() && talent.force_of_nature.ok() )
    active.dream_burst = get_secondary_action<dream_burst_t>( "dream_burst" );

  if ( talent.the_light_of_elune.ok() )
  {
    auto foe = get_secondary_action<fury_of_elune_t>(
      "the_light_of_elune", find_spell( 202770 ), flag_e::LIGHTOFELUNE, find_spell( 211545 ), buff.fury_of_elune );
    foe->s_data_reporting = talent.the_light_of_elune;
    foe->background = true;
    foe->proc = true;
    foe->params.duration( talent.the_light_of_elune->effectN( 2 ).time_value() );
    active.the_light_of_elune = foe;
  }

  if ( talent.thriving_growth.ok() )
  {
    auto vines = get_secondary_action<bloodseeker_vines_t>( "bloodseeker_vines" );

    if ( talent.implant.ok() )
    {
      auto implant = get_secondary_action<bloodseeker_vines_t>( "bloodseeker_vines_implant" );
      implant->name_str_reporting = "bloodseeker_vines";
      implant->dot_duration = talent.implant->effectN( specialization() == DRUID_FERAL ? 1 : 2 ).time_value();
      active.bloodseeker_vines_implant = implant;

      replace_stats( vines, implant );
    }

    active.bloodseeker_vines = vines;
  }

  if ( talent.treants_of_the_moon.ok() )
  {
    auto mf = get_secondary_action<moonfire_t>( "moonfire_treants", flag_e::TREANT );
    mf->proc = true;
    mf->name_str_reporting = "moonfire";
    active.treants_of_the_moon_mf = mf;
  }

  if ( talent.star_cascade.ok() )
    active.star_cascade = get_secondary_action<starsurge_cascade_t>( "starsurge_cascade" );

  if ( talent.sylvan_beckoning.ok() )
  {
    active.sylvan_beckoning = new sylvan_beckoning_t( this );

    auto driver = get_secondary_action<starfall_t::starfall_driver_t>(
      "sylvan_beckoning_starfall_driver", find_trigger( spec.sylvan_beckoning_sf ).trigger(), find_spell( 1264676 ),
      flag_e::SYLVAN );
    driver->name_str_reporting = "starfall";
    driver->callbacks = false;
    driver->damage->callbacks = false;
    replace_stats( driver, driver->damage );
    active.sylvan_beckoning->add_child( driver );
    active.sylvan_beckoning_starfall_driver = driver;
  }

  player_t::create_actions();

  // stat parent/child hookups
  auto find_parent = [ this ]( action_t* action, std::string_view n ) {
    if ( action && !action->stats->parent )
    {
      if ( auto stat = find_stats( n ) )
      {
        for ( const auto& a : stat->action_list )
        {
          if ( a->data().ok() )
          {
            stat->add_child( action->stats );
            return;
          }
        }
      }
    }
  };

  find_parent( active.bursting_growth, "bloodseeker_vines" );
  find_parent( active.echo_of_maul, "wild_guardian" );
  find_parent( active.echo_of_ravage, "wild_guardian" );
  find_parent( active.echo_of_raze, "wild_guardian" );
  find_parent( active.shooting_stars_mid1, "shooting_stars" );
  find_parent( active.sundering_roar_thrash, "sundering_roar" );
  find_parent( active.the_light_of_elune, "moonfire" );
  find_parent( active.thrash_flashing, "thrash" );
  find_parent( active.vicious_brambles, "wild_guardian" );

  // shroom madness
  find_parent( active.fungal_growth, "wild_mushroom" );
  find_parent( active.fungal_growth, "sunseeker_mushroom" );
  find_parent( active.sunseeker_mushroom, "wild_mushroom" );

  if ( specialization() == DRUID_GUARDIAN )
    find_parent( active.star_cascade, "thrash" );
}

// Default Consumables ======================================================
std::string druid_t::default_flask() const
{
  switch ( specialization() )
  {
    case DRUID_BALANCE:     return "magisters_2";
    case DRUID_FERAL:       return "magisters_2";
    case DRUID_GUARDIAN:    return "blood_knights_2";
    case DRUID_RESTORATION: return "blood_knights_2";
    default:                return "disabled";
  }
}

std::string druid_t::default_potion() const
{
  switch ( specialization() )
  {
    case DRUID_BALANCE:     return "lights_potential_2";
    case DRUID_FERAL:       return "lights_potential_2";
    case DRUID_GUARDIAN:    return "lights_potential_2";
    case DRUID_RESTORATION: return "lights_potential_2";
    default:                return "disabled";
  }
}

std::string druid_t::default_food() const
{
  switch ( specialization() )
  {
    case DRUID_BALANCE:     return "harandar_celebration";
    case DRUID_FERAL:       return "harandar_celebration";
    case DRUID_GUARDIAN:    return "harandar_celebration";
    case DRUID_RESTORATION: return "harandar_celebration";
    default:                return "harandar_celebration";
  }
}

std::string druid_t::default_rune() const
{
  return "void_touched";
}

std::string druid_t::default_temporary_enchant() const
{
  std::string str = "main_hand:";

  switch ( specialization() )
  {
    case DRUID_BALANCE:     return str + "thalassian_phoenix_oil_2";
    case DRUID_FERAL:       return str + "thalassian_phoenix_oil_2";
    case DRUID_GUARDIAN:    return str + "thalassian_phoenix_oil_2";
    case DRUID_RESTORATION: return str + "thalassian_phoenix_oil_2";
    default:                return "disabled";
  }
}

// ALL Spec Pre-Combat Action Priority List =================================
void druid_t::apl_precombat()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  // Consumables
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
}

// NO Spec Combat Action Priority List ======================================
void druid_t::apl_default()
{
  action_priority_list_t* def = get_action_priority_list( "default" );

  // Assemble Racials / On-Use Items / Professions
  for ( const auto& action_str : get_racial_actions() )
    def->add_action( action_str );

  for ( const auto& action_str : get_item_actions() )
    def->add_action( action_str );

  for ( const auto& action_str : get_profession_actions() )
    def->add_action( action_str );
}

// Action Priority Lists ========================================
void druid_t::apl_feral()
{
#include "class_modules/apl/druid/feral_apl.inc"
}

void druid_t::apl_feral_ptr()
{
#include "class_modules/apl/druid/feral_apl_ptr.inc"
}

void druid_t::apl_balance()
{
#include "class_modules/apl/druid/balance_apl.inc"
}

void druid_t::apl_balance_ptr()
{
#include "class_modules/apl/druid/balance_apl_ptr.inc"
}

void druid_t::apl_guardian()
{
#include "class_modules/apl/druid/guardian_apl.inc"
}

void druid_t::apl_guardian_ptr()
{
#include "class_modules/apl/druid/guardian_apl_ptr.inc"
}

void druid_t::apl_restoration()
{
#include "class_modules/apl/druid/restoration_druid_apl.inc"
}

void druid_t::apl_restoration_ptr()
{
#include "class_modules/apl/druid/restoration_druid_apl_ptr.inc"
}

// druid_t::init_scaling ====================================================
void druid_t::init_scaling()
{
  player_t::init_scaling();

  scaling->disable( STAT_STRENGTH );

  // workaround for resto dps scaling
  if ( specialization() == DRUID_RESTORATION )
  {
    scaling->disable( STAT_AGILITY );
    scaling->disable( STAT_MASTERY_RATING );
    scaling->disable( STAT_WEAPON_DPS );
    scaling->enable( STAT_INTELLECT );
  }

  // Save a copy of the weapon
  caster_form_weapon = main_hand_weapon;

  // Bear/Cat form weapons need to be scaled up if we are calculating scale factors for the weapon
  // dps. The actual cached cat/bear form weapons are created before init_scaling is called, so the
  // adjusted values for the "main hand weapon" have not yet been added.
  if ( sim->scaling->scale_stat == STAT_WEAPON_DPS )
  {
    if ( cat_weapon.damage > 0 )
    {
      auto coeff = sim->scaling->scale_value * cat_weapon.swing_time.total_seconds();
      cat_weapon.damage += coeff;
      cat_weapon.min_dmg += coeff;
      cat_weapon.max_dmg += coeff;
      cat_weapon.dps += sim->scaling->scale_value;
    }

    if ( bear_weapon.damage > 0 )
    {
      auto coeff = sim->scaling->scale_value * bear_weapon.swing_time.total_seconds();
      bear_weapon.damage += coeff;
      bear_weapon.min_dmg += coeff;
      bear_weapon.max_dmg += coeff;
      bear_weapon.dps += sim->scaling->scale_value;
    }
  }
}

// druid_t::init ============================================================
void druid_t::init()
{
  player_t::init();

  switch ( specialization() )
  {
    case DRUID_BALANCE:
      action_list_information +=
        "\n# Balance APL can be found at https://github.com/dreamgrove/dreamgrove/blob/master/sims/owl/balance.txt\n";
      break;
    case DRUID_FERAL:
      action_list_information +=
        "\n# Feral APL can be found at https://github.com/dreamgrove/dreamgrove/blob/master/sims/cat/feral.txt\n";
      break;
    case DRUID_GUARDIAN:
      action_list_information +=
        "\n# Guardian APL can be found at https://github.com/dreamgrove/dreamgrove/blob/master/sims/bear/guardian.txt\n";
      break;
    case DRUID_RESTORATION:
      action_list_information +=
        "\n# Restoration DPS APL can be found at https://github.com/dreamgrove/dreamgrove/blob/master/sims/tree/restoration.txt\n";
      break;
    default:
      break;
  }
}

bool druid_t::validate_fight_style( fight_style_e style ) const
{
  switch ( specialization() )
  {
    case DRUID_BALANCE:
      if ( style == FIGHT_STYLE_PATCHWERK || style == FIGHT_STYLE_DUNGEON_ROUTE )
        return true;
      else
        return false;

    case DRUID_FERAL:
      break;

    case DRUID_GUARDIAN:
      if ( style == FIGHT_STYLE_DUNGEON_SLICE || style == FIGHT_STYLE_DUNGEON_ROUTE )
      {
        sim->error( error_level_e::MODERATE,
                    "DungeonSlice and DungeonRoute have no incoming tank damage which may result in incorrect "
                    "evaluation of some talents & items." );
      }
      break;

    case DRUID_RESTORATION:
      break;

    default:
      break;
  }

  return true;
}

bool druid_t::validate_actor()
{
  return true;
}

// Thriving Growth RNG ======================================================
// community testing (~257k ticks) memorial
// https://docs.google.com/spreadsheets/d/1lPDhmfqe03G_eFetGJEbSLbXMcfkzjhzyTaQ8mdxADM/edit?gid=385734241

/* Dev Notes:
Whenever an event occurs that can trigger Bloodseeker Vines or Symbiotic Blooms occurs, an amount is added to an
accumulator. That amount is initialized by a value in the Thriving Growth spell, modified by the number of DoTs or HoTs
that are active so chance doesn't grow linearly with number of targets in AOE, and randomized. When the accumulator is
greater than 1000, an instance of Bloodseeker Vines or Symbiotic Blooms is generated and the accumulator is reduced by
1000. Thriving Growth Parameters in SpellID 439528:

100 - Rip tick accumulator initial value
135 - Rake tick accumulator initial value
85 - Wild Growth tick accumulator initial value
155 - Regrowth tick accumulator initial value
155 - Efflorescence tick accumulator initial value
62 - Bloodseeker Vines inverse growth coefficient * 100
75 - Symbiotic Blooms inverse growth coefficient * 100
The formula for the amount added by a triggering event is:

Aura Accumulator Initial Value / (Number of Active Auras) ^ (Inverse Coefficient)

Efflorescence works slightly differently - its accumulator value is just divided by the number of targets healed.

These formulas causes it to behave similarly to the chance for Apex Predator’s Craving to proc, where each additional
affected target provides a smaller increase than the one before. For example, if you have 4 Rips active, the amount
accumulated per damage tick is 100 / (4 ^ 0.62), or 42. Since you have 4 Rips ticking, you’ll accumulate 168 in the time
you’d accumulate 100 with just one Rip active.

The calculated amount of every event is then randomized by multiplying its value by a random value between 0 and 2.
Finally, it is added to the accumulator. There is no bad luck protection or delay before another growth can occur.

The 11.2 Set Bonus works by multiplying the accumulator initial values by 1 + the set bonus chance.
*/
struct thriving_growth_rng_t : public proc_rng_t
{
  static constexpr rng_type_e rng_type = RNG_CUSTOM;
  static constexpr double threshold = 1000.0;
  static constexpr double rng_mult = 2.0;

  struct coeffs_t
  {
    std::vector<dot_t*>* dot_list = nullptr;
    action_t* action = nullptr;
    double scale = 0.0;
    double scale_mul = 1.0;
    double dot_exp = 1.0;
  };

  coeffs_t c_rip;
  coeffs_t c_rake;
  coeffs_t c_wild_growth;
  coeffs_t c_regrowth;
  coeffs_t c_efflorescence;
  druid_t*p;
  double count = 0.0;

  thriving_growth_rng_t( std::string_view n, player_t* p_ )
    : proc_rng_t( rng_type, n, p_ ), p( static_cast<druid_t*>( p_ ) )
  {
    auto vine_mul  = p->specialization() == DRUID_FERAL ? 1.0 + p->talent.green_thumb->effectN( 1 ).percent() : 1.0;
    auto bloom_mul = p->specialization() == DRUID_FERAL ? 1.0 : 1.0 + p->talent.green_thumb->effectN( 2 ).percent();

    auto vine_exp  = p->talent.thriving_growth->effectN( 6 ).percent();
    auto bloom_exp = p->talent.thriving_growth->effectN( 7 ).percent();

    c_rip  = { &p->dot_lists.rip, p->active.bloodseeker_vines, p->talent.thriving_growth->effectN( 1 ).base_value(),
               vine_mul, vine_exp };
    c_rake = { &p->dot_lists.rake, p->active.bloodseeker_vines, p->talent.thriving_growth->effectN( 2 ).base_value(),
               vine_mul, vine_exp };

    c_wild_growth   = { &p->dot_lists.wild_growth, nullptr, p->talent.thriving_growth->effectN( 3 ).base_value(),
                        bloom_mul, bloom_exp };
    c_regrowth      = { &p->dot_lists.regrowth, nullptr, p->talent.thriving_growth->effectN( 4 ).base_value(),
                        bloom_mul, bloom_exp };
    c_efflorescence = { &p->dot_lists.efflorescence, nullptr, p->talent.thriving_growth->effectN( 5 ).base_value(),
                        bloom_mul, bloom_exp };
  }

  void reset( reset_type_e r_type ) override
  {
    if ( r_type == reset_type_e::COMBAT )
      count -= threshold;
    else
      count = player->rng().range( threshold );  // accumulator seems to never reset, so randomize each start
  }

  int _calculate( const coeffs_t& c, action_state_t* s )
  {
    auto raw = c.scale * c.scale_mul / std::pow( as<double>( c.dot_list->size() ), c.dot_exp );
    auto val = player->rng().range( 0.0, raw * rng_mult );

    count += val;

    auto result = count >= threshold;

    if ( player->sim->debug )
    {
      player->sim->print_debug(
        "{} from {}: scale={} scale_mul={} num_dots={} dot_exp={} raw={} value={} count={} result={}",
        name(), *s->action, c.scale, c.scale_mul, c.dot_list->size(), c.dot_exp, raw, val, count, result );
    }

    if ( result )
    {
      if ( c.action )
        c.action->execute_on_target( s->target );

      reset( reset_type_e::COMBAT );
    }

    return result;
  }

  int trigger( action_state_t* s ) override
  {
    switch ( s->action->id )
    {
      case 1079:  return _calculate( c_rip, s );            // rip
      case 155722: return _calculate( c_rake, s );           // rake
      case 48438:  return _calculate( c_wild_growth, s );    // wild growth
      case 8936:   return _calculate( c_regrowth, s );     // regrowth
      case 81269:  return _calculate( c_efflorescence, s );  // efflorescence
      default:     return 0;
    }
  }
};

// druid_t::init_rng ========================================================
void druid_t::init_rng()
{
  player_t::init_rng();

  if ( talent.convoke_the_spirits.ok() )
  {
    bool guidance = talent.elunes_guidance.ok() || talent.ashamanes_guidance.ok() ||
                    talent.ursocs_guidance.ok() || talent.cenarius_guidance.ok();

    rngs.convoke = get_shuffled_rng( "convoke_the_spirits", 1, guidance ? 2 : options.convoke_the_spirits_deck );
  }

  if ( talent.ravage.ok() )
  {
    // Assuming pseudo random distribution with the following nominal probabilities:
    // Feral baseline:           5.0%
    // Feral limb from limb:     6.5%
    // Guardian baseline:       10.0%
    // Guardian limb from limb: 13.0%

    // *** Outdated Info Below **
    // Feral: 0.286% via community testing (~197k auto attacks)
    // Guardian: 1.144% via community testing (9921 auto attacks), 4x feral
    // https://docs.google.com/spreadsheets/d/1lPDhmfqe03G_eFetGJEbSLbXMcfkzjhzyTaQ8mdxADM/edit?gid=385734241

    auto c = specialization() == DRUID_FERAL ? 0.05 : 0.10;
    c *= 1.0 + talent.limb_from_limb->effectN( 1 ).percent();

    rngs.ravage = get_accumulated_rng( "ravage", prd::find_constant( c ) );
  }

  if ( talent.thriving_growth.ok() )
  {
    rngs.bloodseeker_vines = get_rng<thriving_growth_rng_t>( "bloodseeker_vines" );
    // rngs.symbiotic_blooms = get_rng<thriving_growth_rng_t>( "symbiotic_blooms" );  // NYI
  }
}

// druid_t::init_gains ======================================================
void druid_t::init_gains()
{
  player_t::init_gains();

  if ( talent.berserk_cat.ok() )
    gain.overflowing_power = get_gain( "Overflowing Power" );
}

// druid_t::init_procs ======================================================
void druid_t::init_procs()
{
  player_t::init_procs();

  if ( talent.omen_of_clarity_cat.ok() || talent.omen_of_clarity_tree.ok() )
    proc.clearcasting_wasted = get_proc( "Clearcasting (Wasted)" );
}

// druid_t::init_uptimes ====================================================
void druid_t::init_uptimes()
{
  player_t::init_uptimes();

  if ( talent.atmospheric_exposure.ok() )
    uptime.atmospheric_exposure = get_uptime( "Atmospheric Exposure" );

  if ( talent.stellar_amplification.ok() )
    uptime.stellar_amplification = get_uptime( "Stellar Amplification" );
}

// druid_t::init_resources ==================================================
void druid_t::init_resources( bool force )
{
  player_t::init_resources( force );

  if ( options.initial_astral_power == 0.0 && talent.natures_balance.ok() )
    resources.current[ RESOURCE_ASTRAL_POWER ] = talent.natures_balance->effectN( 2 ).base_value();
  else if ( options.initial_astral_power > 0.0 )
    resources.current[ RESOURCE_ASTRAL_POWER ] = options.initial_astral_power;
}


// druid_t::init_special_effects ============================================
void druid_t::init_special_effects()
{
  using trigger_type = dbc_proc_callback_t::trigger_fn_type;

  struct druid_cb_t : public dbc_proc_callback_t
  {
    druid_cb_t( druid_t* p, const special_effect_t& e ) : dbc_proc_callback_t( p, e ) {}

    druid_t* p() { return static_cast<druid_t*>( listener ); }
  };

  // General
  // bear form rage from being attacked
  if ( uses_bear_form() )
  {
    const auto driver = new special_effect_t( this );
    driver->name_str = "rage_from_being_attacked";
    driver->spell_id = spec.bear_form_passive->id();
    special_effects.push_back( driver );

    auto _gain = get_gain( "Rage from being attacked" );
    auto _rage = spec.bear_form_passive->effectN( 3 ).base_value();

    callbacks.register_callback_execute_function( driver->spell_id, [ this, _gain, _rage ]( auto, auto, auto, auto ) {
      resource_gain( RESOURCE_RAGE, _rage, _gain );
    } );

    auto cb = new dbc_proc_callback_t( this, *driver );
    cb->activate_with_buff( buff.bear_form );
  }

  // Balance
  if ( talent.ascendant_eclipses_2.ok() )
  {
    const auto driver = new special_effect_t( this );
    driver->name_str = talent.ascendant_eclipses_2->name_cstr();
    driver->spell_id = talent.ascendant_eclipses_2->id();
    driver->proc_flags2_ = PF2_CRIT;
    special_effects.push_back( driver );

    auto _damage = get_secondary_action<residual_action::residual_periodic_action_t<druid_spell_t>>(
      "astral_smolder", this, find_spell( 1263250 ) );
    _damage->proc = true;
    auto _mul = talent.ascendant_eclipses_2->effectN( 1 ).percent();

    callbacks.register_callback_execute_function(
      driver->spell_id, [ _damage, _mul ]( auto, auto, player_t* t, const action_state_t* s ) {
        if ( s->result_amount )
          residual_action::trigger( _damage, t, s->result_amount * _mul );
      } );

    auto cb = new dbc_proc_callback_t( this, *driver );
    cb->deactivate();

    buff.eclipse_lunar->add_stack_change_callback( [ this, cb ]( buff_t* b, int, int ) {
      cb->active = b->check() || buff.eclipse_solar->check();
    } );
    buff.eclipse_solar->add_stack_change_callback( [ this, cb ]( buff_t* b, int, int ) {
      cb->active = b->check() || buff.eclipse_lunar->check();
    } );
  }

  if ( talent.denizen_of_the_dream.ok() )
  {
    const auto driver = new special_effect_t( this );
    driver->name_str = talent.denizen_of_the_dream->name_cstr();
    driver->spell_id = talent.denizen_of_the_dream->id();
    special_effects.push_back( driver );

    callbacks.register_callback_trigger_function(
      driver->spell_id, trigger_type::CONDITION, [ this ]( auto, const proc_data_t& data, auto, auto, auto ) {
        return data->id() == spec.moonfire_dmg->id() || data->id() == spec.sunfire_dmg->id();
      } );

    auto _proc = get_proc( "Denizen of the Dream" )->collect_count();

    callbacks.register_callback_execute_function( driver->spell_id, [ this, _proc ]( auto, auto, auto, auto ) {
      active.denizen_of_the_dream->execute();
      buff.denizen_of_the_dream->trigger();
      _proc->occur();
    } );

    new dbc_proc_callback_t( this, *driver );
  }

  if ( specialization() == DRUID_BALANCE && talent.moonkin_form.ok() )
  {
    const auto driver = new special_effect_t( this );
    driver->name_str = buff.owlkin_frenzy->name();
    driver->spell_id = talent.moonkin_form->id();
    driver->proc_chance_ =
      find_effect( find_specialization_spell( "Owlkin Frenzy" ), A_ADD_FLAT_MODIFIER, P_PROC_CHANCE ).percent();
    driver->custom_buff = buff.owlkin_frenzy;
    special_effects.push_back( driver );

    callbacks.register_callback_trigger_function(
      driver->spell_id, trigger_type::CONDITION, []( auto, const auto&, auto, const action_state_t* s, auto ) {
        return s->n_targets == 1;
      } );

    auto cb = new dbc_proc_callback_t( this, *driver );
    cb->activate_with_buff( buff.moonkin_form );
  }

  // Feral

  // Guardian
  if ( mastery.natures_guardian->ok() )
  {
    const auto driver = new special_effect_t( this );
    driver->name_str = "natures_guardian";
    driver->spell_id = mastery.natures_guardian->id();
    driver->proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_HEAL;
    special_effects.push_back( driver );

    callbacks.register_callback_trigger_function(
      driver->spell_id, trigger_type::CONDITION,
      []( auto, const proc_data_t& data, auto, const action_state_t* s, auto ) {
        if ( data->id() <= 0 || s->result_total <= 0 || s->action->harmful )
          return false;

        auto pct_heal = debug_cast<heal_t*>( s->action );
        assert( pct_heal && "Non-heal action attempting to trigger Nature's Guardian." );

        return !pct_heal->base_pct_heal && !pct_heal->tick_pct_heal;
      } );

    auto _heal = get_secondary_action<druid_heal_t>( "natures_guardian", this, find_spell( 227034 ) );
    _heal->background = _heal->proc = true;
    _heal->callbacks = false;
    _heal->target = this;

    callbacks.register_callback_execute_function(
      driver->spell_id, [ this, _heal ]( auto, auto, auto, const action_state_t* s ) {
        _heal->base_dd_min = _heal->base_dd_max = s->result_total * cache.mastery_value();
        _heal->schedule_execute();
      } );

    new dbc_proc_callback_t( this, *driver );
  }

  if ( talent.bristling_fur.ok() )
  {
    const auto driver = new special_effect_t( this );
    driver->name_str = talent.bristling_fur->name_cstr();
    driver->spell_id = talent.bristling_fur->id();
    driver->cooldown_ = 0_ms;
    special_effects.push_back( driver );

    auto _gain = get_gain( "Bristling Fur" );
    auto _action = find_action( "bristling_fur" );

    callbacks.register_callback_execute_function(
      driver->spell_id, [ this, _gain, _action ]( auto, auto, auto, const action_state_t* s ) {
        // 1 rage per 1% of maximum health taken
        auto pct = std::min( 1.0, s->result_amount / resources.max[ RESOURCE_HEALTH ] );

        resource_gain( RESOURCE_RAGE, pct * 100, _gain, _action );
      } );

    auto cb = new dbc_proc_callback_t( this, *driver );
    cb->activate_with_buff( buff.bristling_fur );
  }

  if ( talent.dream_guide.ok() )
  {
    const auto driver = new special_effect_t( this );
    driver->name_str = talent.dream_guide->name_cstr();
    driver->spell_id = talent.dream_guide->id();
    driver->custom_buff = buff.dream_guide;
    special_effects.push_back( driver );

    new dbc_proc_callback_t( this, *driver );
  }

  if ( talent.dream_of_cenarius_bear.ok() )
  {
    // derived class needed as registered trigger_fn passes const dbc_proc_callback_t* and we need to adjust proc_chance
    struct dream_of_cenarius_cb_t final : public druid_cb_t
    {
      dream_of_cenarius_cb_t( druid_t* p, const special_effect_t& e ) : druid_cb_t( p, e ) {}

      void trigger( const proc_data_t& data, player_t* t, action_state_t* s, proc_trigger_type_e type ) override
      {
        proc_chance = p()->cache.attack_crit_chance();

        druid_cb_t::trigger( data, t, s, type );
      }
    };

    const auto driver = new special_effect_t( this );
    driver->name_str = talent.dream_of_cenarius_bear->name_cstr();
    driver->spell_id = talent.dream_of_cenarius_bear->id();
    driver->cooldown_ = find_spell( 372523 )->duration();
    driver->custom_buff = buff.dream_of_cenarius;
    special_effects.push_back( driver );

    new dream_of_cenarius_cb_t( this, *driver );
  }

  if ( talent.elunes_favored.ok() )
  {
    const auto driver = new special_effect_t( this );
    driver->name_str = talent.elunes_favored->name_cstr();
    driver->spell_id = spec.elunes_favored->id();
    special_effects.push_back( driver );

    callbacks.register_callback_trigger_function(
      driver->spell_id, trigger_type::CONDITION, []( auto, const auto&, auto, const action_state_t* s, auto ) {
        return s && s->result_amount && dbc::has_common_school( s->action->get_school(), SCHOOL_ASTRAL );
      } );

    auto _mul = talent.elunes_favored->effectN( 1 ).percent();
    callbacks.register_callback_execute_function(
      driver->spell_id, [ this, _mul ]( auto, auto, auto, const action_state_t* s ) {
          buff.elunes_favored->current_value += s->result_amount * _mul;
      } );

    auto cb = new dbc_proc_callback_t( this, *driver );
    cb->activate_with_buff( buff.bear_form );
  }

  if ( talent.galactic_guardian.ok() )
  {
    // derived class needed as registered trigger_fn passes const dbc_proc_callback_t* and we need to adjust proc_chance
    struct galactic_guardian_cb_t final : public druid_cb_t
    {
      proc_t* gg_proc;
      double orig_proc_chance;
      double mid1_proc_chance;
      int lw_charges;

      galactic_guardian_cb_t( druid_t* p, const special_effect_t& e )
        : druid_cb_t( p, e ),
          gg_proc( p->get_proc( "Galactic Guardian" )->collect_count() ),
          orig_proc_chance( e.proc_chance() ),
          mid1_proc_chance( p->sets->set( DRUID_GUARDIAN, MID1, B4 )->effectN( 1 ).percent() ),
          lw_charges( as<int>( p->talent.galactic_guardian->effectN( 2 ).base_value() ) )
      {}

      void trigger( const proc_data_t& data, player_t* t, action_state_t* s, proc_trigger_type_e pt ) override
      {
        assert( proc_chance == orig_proc_chance );

        switch ( data->id() )
        {
          case 8921:    // moonfire
          case 164812:  // moonfire damage
            return;
          case 6807:    // maul
          case 400254:  // raze
          case 441605:  // ravage
            proc_chance += mid1_proc_chance;
            break;
          default: break;
        }

        druid_cb_t::trigger( data, t, s, pt );

        proc_chance = orig_proc_chance;
      }

      void execute( const spell_data_t*, player_t* t, action_state_t* s ) override
      {
        p()->active.galactic_guardian->execute_on_target( get_target( t, s, p()->active.galactic_guardian ) );
        p()->buff.galactic_guardian->trigger();
        p()->buff.lunar_wrath->trigger( lw_charges );
        gg_proc->occur();
      }
    };

    const auto driver = new special_effect_t( this );
    driver->name_str = talent.galactic_guardian->name_cstr();
    driver->spell_id = talent.galactic_guardian->id();
    driver->proc_chance_ = talent.galactic_guardian->effectN( 1 ).percent();
    special_effects.push_back( driver );

    new galactic_guardian_cb_t( this, *driver );
  }

  if ( talent.moonless_night.ok() )
  {
    struct moonless_night_cb_t final : public druid_cb_t
    {
      struct moonless_night_t final : public druid_residual_action_t<bear_attack_t>
      {
        moonless_night_t( druid_t* p ) : base_t( "moonless_night", p, p->find_spell( 400360 ) )
        {
          proc = true;

          residual_mul = p->talent.moonless_night->effectN( 1 ).percent();
        }
      };

      action_t* moonless;

      moonless_night_cb_t( druid_t* p, const special_effect_t& e ) : druid_cb_t( p, e )
      {
        moonless = p->get_secondary_action<moonless_night_t>( "moonless_night" );
      }

      void trigger( const proc_data_t& data, player_t* t, action_state_t* s, proc_trigger_type_e pt ) override
      {
        if ( !s->target->is_enemy() )
        {
          throw sc_runtime_error( fmt::format( "{} triggering Moonless Night on {}.", *s->action, *t ) );
        }

        if ( !s->result_amount )
          return;

        // raze (400254) & ravage (441605) trigger moonless night despite being an aoe spell
        // moonfire (164812) & sunfire (164815) do not trigger
        switch ( data->id() )
        {
          case 164812:  // moonfire
          case 164815:  // sunfire
          case 400360:  // moonless night
            return;     // end
          case 400254:  // raze
          case 441605:  // ravage
            break;      // continue
          default:
            if ( s->action->aoe < 0 || s->action->aoe > 1 )
              return;   // end
            else
              break;    // continue
        }

        if ( !p()->get_target_data( s->target )->dots.moonfire->is_ticking() )
          return;

        druid_cb_t::trigger( data, t, s, pt );
      }

      void execute( const spell_data_t*, player_t* t, action_state_t* s ) override
      {
        moonless->execute_on_target( t, s->result_amount );
      }
    };

    const auto driver = new special_effect_t( this );
    driver->name_str = talent.moonless_night->name_cstr();
    driver->spell_id = talent.moonless_night->id();
    driver->proc_flags2_ = PF2_ALL_HIT;
    driver->set_can_only_proc_from_class_abilities( true );
    special_effects.push_back( driver );

    new moonless_night_cb_t( this, *driver );
  }

  if ( talent.waking_nightmare.ok() )
  {
    const auto driver = new special_effect_t( this );
    driver->name_str = talent.waking_nightmare->name_cstr();
    driver->spell_id = talent.waking_nightmare->id();
    special_effects.push_back( driver );

    callbacks.register_callback_execute_function( driver->spell_id, [ this ]( auto, auto, auto, auto ) {
      active.waking_nightmare->execute();
      buff.waking_nightmare->trigger();
    } );

    auto cb = new dbc_proc_callback_t( this, * driver );
    cb->deactivate_with_buff( buff.waking_nightmare );
  }

  // Hero talents
  if ( talent.boundless_moonlight.ok() && talent.lunar_beam.ok() )
  {
    const auto driver = new special_effect_t( this );
    driver->name_str = talent.boundless_moonlight->name_cstr();
    // TODO: confirm if driver lasts for 12s as per spell data
    driver->spell_id = 425217;
    special_effects.push_back( driver );

    auto _mul = buff.boundless_moonlight_heal->data().effectN( 1 ).percent();
    callbacks.register_callback_execute_function(
      driver->spell_id, [ this, _mul ]( auto, auto, auto, const action_state_t* s ) {
        buff.boundless_moonlight_heal->current_value += s->result_amount * _mul;
      } );

    auto cb = new dbc_proc_callback_t( this, *driver );
    cb->activate_with_buff( buff.lunar_beam );

    buff.lunar_beam->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
      if ( new_ )
        buff.boundless_moonlight_heal->trigger();
      else
        buff.boundless_moonlight_heal->expire();
    } );
  }

  if ( talent.implant.ok() && active.bloodseeker_vines_implant )
  {
    const auto driver = new special_effect_t( this );
    driver->name_str = talent.implant->name_cstr();
    driver->spell_id = buff.implant->data().id();
    driver->proc_flags2_ = PF2_CAST_DAMAGE;
    special_effects.push_back( driver );

    // TODO: whitelist aoe spells as necessary if they can trigger
    callbacks.register_callback_trigger_function(
      driver->spell_id, trigger_type::CONDITION, []( auto, const auto&, auto, const action_state_t* s, auto ) {
        return ( s->result_amount > 0 && ( s->action->aoe == 0 || s->action->aoe == 1 ) );
      } );

    callbacks.register_callback_execute_function(
      driver->spell_id, [ this ]( const dbc_proc_callback_t* cb, auto, player_t* t, action_state_t* s ) {
        active.bloodseeker_vines_implant->execute_on_target( cb->get_target( t, s, active.bloodseeker_vines_implant ) );
        buff.implant->consume( s->action );
      } );

    auto cb = new dbc_proc_callback_t( this, *driver );
    cb->activate_with_buff( buff.implant );
  };

  if ( talent.twin_claw.ok() )
  {
    const auto driver = new special_effect_t( this );
    driver->name_str = talent.twin_claw->name_cstr();
    driver->spell_id = talent.twin_claw->id();
    driver->proc_flags_ = PF_MELEE_ABILITY;  // only melee abilities trigger
    driver->set_can_only_proc_from_class_abilities( true );
  
    if ( specialization() == DRUID_FERAL )
    {
      auto claw = get_secondary_action<cat_attack_t>( "twin_claw", this, find_spell( 1271636 ) );
      claw->background = claw->proc = true;
      driver->execute_action = claw;
      driver->proc_chance_ = talent.twin_claw->effectN( 1 ).percent();
    }
    else
    {
      auto claw = get_secondary_action<bear_attack_t>( "twin_claw", this, find_spell( 1271657 ) );
      claw->background = claw->proc = true;
      driver->execute_action = claw;
      driver->proc_chance_ = talent.twin_claw->effectN( 2 ).percent();
    }

    special_effects.push_back( driver );

    auto _form = specialization() == DRUID_FERAL ? CAT_FORM : BEAR_FORM;

    callbacks.register_callback_trigger_function(
      driver->spell_id, trigger_type::CONDITION,
      [ this, _form ]( auto, const proc_data_t& data, auto, const action_state_t* s, auto ) {
        // raze can trigger despite being aoe
        return form == _form && s->result_amount &&
               ( data->id() == talent.raze->id() || s->action->aoe == 0 || s->action->aoe == 1 );
      } );

    new dbc_proc_callback_t( this, *driver );
  }

  // NOTE: this must come after all dbc_proc_callback creation in order to properly initialize them all
  player_t::init_special_effects();
}

// druid_t::init_actions ====================================================
void druid_t::init_action_list()
{
  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }

  clear_action_priority_lists();

  apl_precombat();  // PRE-COMBAT

  switch ( specialization() )
  {
    case DRUID_FERAL:       is_ptr() ? apl_feral_ptr() : apl_feral();             break;
    case DRUID_BALANCE:     is_ptr() ? apl_balance_ptr() : apl_balance();         break;
    case DRUID_GUARDIAN:    is_ptr() ? apl_guardian_ptr() : apl_guardian();       break;
    case DRUID_RESTORATION: is_ptr() ? apl_restoration_ptr() : apl_restoration(); break;
    default:                apl_default();                                        break;
  }

  use_default_action_list = true;

  player_t::init_action_list();
}

// druid_t::init_blizzard_action_list =======================================
void druid_t::init_blizzard_action_list()
{
  auto pre = get_action_priority_list( "precombat" );
  auto def = get_action_priority_list( "default" );

  if ( specialization() == DRUID_FERAL )
    def->add_action( "auto_attack,if=!buff.prowl.up" );
  else if ( specialization() == DRUID_GUARDIAN )
    def->add_action( "auto_attack" );

  player_t::init_blizzard_action_list();

  if ( specialization() == DRUID_FERAL )
    pre->add_action( "prowl" );
}

// druid_t::action_names_from_spell_id ======================================
std::vector<std::string> druid_t::action_names_from_spell_id( unsigned int spell_id ) const
{
  if ( spell_id == 1126 )  // mark of the wild
    return {};

  if ( specialization() == DRUID_FERAL && spell_id == 8921 )  // moonfire
    return { "lunar_inspiration" };

  if ( spell_id == 274281 )  // new moon
    return { "new_moon", "half_moon", "full_moon" };

  if ( spell_id == 1249752 )  // waiting for energy
    return { "pool_resource" };

  return player_t::action_names_from_spell_id( spell_id );
}

// druid_t::parse_assisted_combat_step ======================================
void druid_t::parse_assisted_combat_step( const assisted_combat_step_data_t& step, action_priority_list_t* apl )
{
  switch ( step.spell_id )
  {
    case 768:    // cat form
    case 5487:   // bear form
    case 24858:  // moonkin form
      if ( step.order_index <= 1 )
      {
        player_t::parse_assisted_combat_step( step, get_action_priority_list( "precombat" ) );
        return;
      }
      break;
    default: break;
  }

  player_t::parse_assisted_combat_step( step, apl );
}

// druid_t::parse_assisted_combat_rule ======================================
parsed_assisted_combat_rule_t druid_t::parse_assisted_combat_rule( const assisted_combat_rule_data_t& rule,
                                                                   const assisted_combat_step_data_t& step ) const
{
  // adjust for unused obsolete spell id for apex predator buff
  if ( rule.condition_type == AC_AURA_ON_PLAYER && rule.condition_value_2 == 252752 )
  {
    auto new_rule = rule;  // make a copy
    new_rule.condition_value_2 = rule.condition_value_1;
    return { player_t::parse_assisted_combat_rule( new_rule, step ), true };
  }

  // adjust to check dream burst being overridden
  if ( rule.condition_type == AC_AURA_ON_TARGET && rule.condition_value_1 == 433831 )
  {
    auto new_rule = rule;  // make a copy
    new_rule.condition_type = AC_AURA_MISSING_PLAYER;
    new_rule.condition_value_1 = 433832;
    return { player_t::parse_assisted_combat_rule( new_rule, step ), true };
  }

  return player_t::parse_assisted_combat_rule( rule, step );
}

// druid_t::reset ===========================================================
void druid_t::reset()
{
  player_t::reset();

  // Reset druid_t variables to their original state.
  form = NO_FORM;
  base_gcd = 1.5_s;

  // Restore main hand attack / weapon to normal state
  main_hand_attack = caster_melee_attack;
  main_hand_weapon = caster_form_weapon;

  if ( mastery.natures_guardian->ok() )
    recalculate_resource_max( RESOURCE_HEALTH );

  // Reset runtime variables
  moon_stage = static_cast<moon_stage_e>( options.initial_moon_stage );
  persistent_event_delay.clear();
  astral_power_decay = nullptr;
  dot_lists.moonfire.clear();
  dot_lists.sunfire.clear();
  dot_lists.rake.clear();
  dot_lists.rip.clear();
  dot_lists.thrash.clear();
  dot_lists.dreadful_wound.clear();
  spell_queued = {};
}

/*
// druid_t::merge ===========================================================
void druid_t::merge( player_t& other )
{
  player_t::merge( other );

  druid_t& od = static_cast<druid_t&>( other );

  for ( size_t i = 0; i < counters.size(); i++ )
    counters[ i ]->merge( *od.counters[ i ] );
}
*/

void druid_t::analyze( sim_t& s )
{
  player_t::analyze( s );

  // GG is a major portion of guardian druid damage but skews moonfire reporting because gg has no execute time. We
  // adjust by removing the gg amount from mf stat and re-calculating dpe and dpet for moonfire.
  if ( talent.galactic_guardian.ok() )
  {
    if ( auto mf = find_action( "moonfire" ), gg = find_action( "galactic_guardian" ); mf && gg )
    {
      auto mf_amt = mf->stats->compound_amount;
      auto gg_amt = gg->stats->compound_amount;
      auto mod = ( mf_amt - gg_amt ) / mf_amt;

      mf->stats->ape *= mod;
      mf->stats->apet *= mod;
    }
  }
}

// druid_t::mana_regen_per_second ===========================================
double druid_t::resource_regen_per_second( resource_e r ) const
{
  double reg = player_t::resource_regen_per_second( r );

  if ( r == RESOURCE_ENERGY && buff.savage_fury->check() )
    reg *= 1.0 + buff.savage_fury->data().effectN( 2 ).percent();

  return reg;
}

double druid_t::resource_gain( resource_e r, double amount, gain_t* g, action_t* a )
{
  auto actual = player_t::resource_gain( r, amount, g, a );

  if ( r == RESOURCE_COMBO_POINT )
  {
    auto over = amount - actual;

    if ( g != gain.overflowing_power && over > 0 && buff.b_inc_cat->check() )
    {
      auto avail =
        std::min( over, as<double>( buff.overflowing_power->max_stack() - buff.overflowing_power->check() ) );

      if ( avail > 0 )
        g->overflow[ r ] -= avail;

      buff.overflowing_power->trigger( as<int>( over ) );
      over -= avail;
    }

    if ( talent.coiled_to_spring.ok() && over > 0 )
      buff.coiled_to_spring->trigger();
  }

  return actual;
}

// druid_t::available =======================================================
timespan_t druid_t::available() const
{
  if ( ready_type != ready_e::READY_TRIGGER )
    return player_t::available();

  double energy = resources.current[ RESOURCE_ENERGY ];

  if ( energy >= resource_thresholds.front() )
    return player_t::available();

  return std::max( player_t::available(), timespan_t::from_seconds( ( resource_thresholds.front() - energy ) /
                                                                    resource_regen_per_second( RESOURCE_ENERGY ) ) );
}

// druid_t::precombat_init (called before precombat apl)=======================
void druid_t::precombat_init()
{
  player_t::precombat_init();

  if ( talent.brambles.ok() )
    buff.brambles->trigger();

  if ( talent.lunar_calling.ok() )
    buff.lunar_eclipse_override->trigger();

  if ( talent.lycaras_teachings.ok() )
    buff.lycaras_teachings_haste->trigger();

  if ( talent.orbit_breaker.ok() )
  {
    auto stacks = options.initial_orbit_breaker_stacks >= 0
                    ? options.initial_orbit_breaker_stacks
                    : rng().range( 0, as<int>( talent.orbit_breaker->effectN( 1 ).base_value() ) );

    if ( stacks )
      buff.orbit_breaker->trigger( stacks );
  }

  auto start_buff = [ this ]( buff_t* b ) {
    if ( !b->is_fallback )
      persistent_event_delay.push_back( make_event<persistent_delay_event_t>( *sim, this, b ) );
  };

  start_buff( buff.blood_frenzy );
  start_buff( buff.elunes_favored );
  start_buff( buff.natures_balance );
  start_buff( buff.predator );
  start_buff( buff.shooting_stars_moonfire );
  start_buff( buff.shooting_stars_sunfire );
  start_buff( buff.treants_of_the_moon );
  start_buff( buff.yseras_gift );
}

// druid_t::combat_begin (called after precombat apl before default apl)=======
void druid_t::combat_begin()
{
  player_t::combat_begin();

  if ( specialization() == DRUID_BALANCE )
  {
    if ( !options.raid_combat )
      in_boss_encounter = 0;

    if ( in_boss_encounter )
    {
      double cap = std::max( talent.natures_balance->effectN( 2 ).base_value(), 20.0 );
      double curr = resources.current[ RESOURCE_ASTRAL_POWER ];

      resources.current[ RESOURCE_ASTRAL_POWER ] = std::min( cap, curr );
      if ( curr > cap )
        sim->print_debug( "Astral Power capped at combat start to {} (was {})", cap, curr );
    }

    // Fallthrough if there wasn't any precombat Starfire cast to trigger Dreamstate
    if ( !buff.dreamstate->check() )
        buff.dreamstate->trigger();
  }

  buff.blooming_infusion_damage_counter->expire();
  buff.blooming_infusion_heal_counter->expire();
}

// druid_t::recalculate_resource_max ========================================
void druid_t::recalculate_resource_max( resource_e rt, gain_t* source )
{
  double pct_health = 0;
  double current_health = 0;
  bool adjust_natures_guardian_health = mastery.natures_guardian->ok() && rt == RESOURCE_HEALTH;

  if ( adjust_natures_guardian_health )
  {
    current_health = resources.current[ rt ];
    pct_health     = resources.pct( rt );
  }

  player_t::recalculate_resource_max( rt, source );

  if ( adjust_natures_guardian_health )
  {
    resources.max[ rt ] *= 1.0 + cache.mastery_value();
    // Maintain current health pct.
    resources.current[ rt ] = resources.max[ rt ] * pct_health;

    if ( sim->log )
    {
      sim->print_log( "{} recalculates maximum health. old_current={:.0f} new_current={:.0f} net_health={:.0f}", name(),
                      current_health, resources.current[ rt ], resources.current[ rt ] - current_health );
    }
  }
}

// druid_t::invalidate_cache ================================================
void druid_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_ATTACK_POWER:
      if ( current.spell_power_per_attack_power > 0 )
        invalidate_cache( CACHE_SPELL_POWER );
      break;

    case CACHE_SPELL_POWER:
      if ( current.attack_power_per_spell_power > 0 )
        invalidate_cache( CACHE_ATTACK_POWER );
      break;

    case CACHE_MASTERY:
      if ( mastery.natures_guardian->ok() )
      {
        invalidate_cache( CACHE_ATTACK_POWER );
        recalculate_resource_max( RESOURCE_HEALTH );
      }
      break;

    case CACHE_CRIT_CHANCE:
      if ( spec.lightning_reflexes->ok() )
        invalidate_cache( CACHE_DODGE );
      break;

    case CACHE_AGILITY:
      if ( buff.ironfur->check() || buff.echo_of_ironfur->check() )
        invalidate_cache( CACHE_ARMOR );
      break;

    case CACHE_STAMINA:
      recalculate_resource_max( RESOURCE_HEALTH );
      break;

    default: break;
  }
}

// Composite combat stat override functions =================================

// Defense ==================================================================
double druid_t::composite_armor() const
{
  double a = player_t::composite_armor();

  if ( buff.ironfur->check() || buff.echo_of_ironfur->check() )
  {
    auto if_val = buff.ironfur->check_stack_value() + buff.echo_of_ironfur->check_stack_value();

    // TODO: confirm this is dynamic
    if ( buff.killing_strikes->check() )
      if_val *= 1.0 + buff.killing_strikes->data().effectN( 2 ).percent();

    a += if_val * cache.agility();
  }

  return a;
}

double druid_t::composite_dodge_rating() const
{
  double dr = player_t::composite_dodge_rating();

  if ( spec.lightning_reflexes->ok() )
    dr += composite_rating( RATING_MELEE_CRIT ) * spec.lightning_reflexes->effectN( 1 ).percent();

  return dr;
}

// Expressions ==============================================================
std::unique_ptr<expr_t> druid_t::create_action_expression( action_t& a, std::string_view name )
{
  auto splits = util::string_split<std::string_view>( name, "." );

  if ( splits[ 0 ] == "ticks_gained_on_refresh" ||
       ( splits.size() > 2 && ( splits[ 0 ] == "druid" || splits[ 0 ] == "dot" ) &&
         splits[ 2 ] == "ticks_gained_on_refresh" ) )
  {
    bool pmul = false;
    if ( ( splits.size() > 1 && splits[ 1 ] == "pmult" ) || ( splits.size() > 4 && splits[ 3 ] == "pmult" ) )
      pmul = true;

    action_t* dot_action = nullptr;

    if ( splits.size() > 2 )
    {
      if ( splits[ 1 ] == "moonfire_cat" )
        dot_action = find_action( "lunar_inspiration" );
      else if ( splits[ 1 ] == "rake" )
        dot_action = find_action( "rake_bleed" );
      else
        dot_action = find_action( splits[ 1 ] );

      if ( !dot_action )
        throw sc_invalid_apl_argument( fmt::format( "Invalid action in 'ticks_gained_on_refresh.{}'.", splits[ 1 ] ) );
    }
    else
      dot_action = &a;

    action_t* source_action = &a;
    double multiplier = 1.0;

    if ( dot_action->name_str == "primal_wrath" )
    {
      dot_action = find_action( "rip" );
      source_action = find_action( "primal_wrath" );
      multiplier = 0.5;
    }

    return make_fn_expr( name, [ dot_action, source_action, multiplier, pmul ]() -> double {
      auto ticks_gained_func = []( double mod, action_t* dot_action, player_t* t, bool pmul ) -> double {
        action_state_t* state = dot_action->get_state();
        state->target = t;
        dot_action->snapshot_state( state, result_amount_type::DMG_OVER_TIME );

        dot_t* dot = dot_action->get_dot( t );
        timespan_t ttd = t->time_to_percent( 0 );
        timespan_t duration = dot_action->composite_dot_duration( state ) * mod;

        double remaining_ticks = std::min( dot->remains(), ttd ) / dot_action->tick_time( state ) *
                                 ( ( pmul && dot->state ) ? dot->state->persistent_multiplier : 1.0 );
        double new_ticks = std::min( dot_action->calculate_dot_refresh_duration( dot, duration ), ttd ) /
                           dot_action->tick_time( state ) * ( pmul ? state->persistent_multiplier : 1.0 );

        action_state_t::release( state );
        return new_ticks - remaining_ticks;
      };

      if ( source_action->aoe == -1 )
      {
        double accum = 0.0;
        for ( player_t* t : source_action->targets_in_range_list( source_action->target_list() ) )
          accum += ticks_gained_func( multiplier, dot_action, t, pmul );

        return accum;
      }

      return ticks_gained_func( multiplier, dot_action, source_action->target, pmul );
    } );
  }

  return player_t::create_action_expression( a, name );
}

std::unique_ptr<expr_t> druid_t::create_expression( std::string_view name )
{
  auto splits = util::string_split<std::string_view>( name, "." );

  if ( util::str_compare_ci( splits[ 0 ], "druid" ) && splits.size() == 2 )
  {
    if ( util::str_compare_ci( splits[ 1 ], "no_cds" ) )
      return expr_t::create_constant( "no_cds", options.no_cds );
    if ( util::str_compare_ci( splits[ 1 ], "time_spend_healing" ) )
      return expr_t::create_constant( "time_spend_healing", options.time_spend_healing );
    if ( util::str_compare_ci( splits[ 1 ], "initial_orbit_breaker_stacks" ) )
      return expr_t::create_constant( "initial_orbit_breaker_stacks", options.initial_orbit_breaker_stacks );
  }

  if ( util::str_compare_ci( name, "combo_points" ) )
    return make_ref_expr( "combo_points", resources.current[ RESOURCE_COMBO_POINT ] );

  if ( ( util::str_compare_ci( splits[ 0 ], "active_dot" ) || util::str_compare_ci( splits[ 0 ], "active_dots" ) ) &&
       splits.size() == 2 )
  {
    if ( util::str_compare_ci( splits[ 1 ], "moonfire" ) )
      return make_fn_expr( name, [ this ]() { return dot_lists.moonfire.size(); } );
    else if ( util::str_compare_ci( splits[ 1 ], "sunfire" ) )
      return make_fn_expr( name, [ this ]() { return dot_lists.sunfire.size(); } );
    else if ( util::str_compare_ci( splits[ 1 ], "rake" ) )
      return make_fn_expr( name, [ this ]() { return dot_lists.rake.size(); } );
    else if ( util::str_compare_ci( splits[ 1 ], "rip" ) )
      return make_fn_expr( name, [ this ]() { return dot_lists.rip.size(); } );
    else if ( util::str_compare_ci( splits[ 1 ], "thrash" ) )
      return make_fn_expr( name, [ this ]() { return dot_lists.thrash.size(); } );
    else if ( util::str_compare_ci( splits[ 1 ], "dreadful_wound" ) )
      return make_fn_expr( name, [ this ]() { return dot_lists.dreadful_wound.size(); } );
  }

  if ( specialization() == DRUID_BALANCE )
  {
    if ( util::str_compare_ci( name, "astral_power" ) )
      return make_ref_expr( name, resources.current[ RESOURCE_ASTRAL_POWER ] );

    if ( splits.size() >= 2 && util::str_compare_ci( splits[ 0 ], "buff" ) &&
         util::str_compare_ci( splits[ 1 ], "eclipse" ) )
    {
      splits[ 1 ] = "eclipse_solar";
      auto solar = druid_t::create_expression( util::string_join( splits, "." ) );
      splits[ 1 ] = "eclipse_lunar";
      auto lunar = druid_t::create_expression( util::string_join( splits, "." ) );

      return make_fn_expr( name, [ solar = std::move( solar ), lunar = std::move( lunar ), this ] {
        return buff.eclipse_lunar->check() ? lunar->evaluate() : solar->evaluate();
      } );
    }

    if ( splits.size() >= 2 && util::str_compare_ci( splits[ 0 ], "cooldown" ) &&
         ( util::str_compare_ci( splits[ 1 ], "solar_eclipse" ) ||
           util::str_compare_ci( splits[ 1 ], "lunar_eclipse" ) ) )
    {
      splits[ 1 ] = "eclipse";

      return druid_t::create_expression( util::string_join( splits, "." ) );
    }

    if ( splits.size() == 2 && util::str_compare_ci( splits[ 0 ], "eclipse" ) )
    {
      if ( util::str_compare_ci( splits[ 1 ], "solar" ) )
        return make_fn_expr( name, [ this ] { return !buff.lunar_eclipse_override->check(); } );
      else if ( util::str_compare_ci( splits[ 1 ], "lunar" ) )
        return make_fn_expr( name, [ this ] { return buff.lunar_eclipse_override->check(); } );
    }

    // New Moon stage related expressions
    if ( util::str_compare_ci( name, "new_moon" ) )
      return make_fn_expr( name, [ this ] { return moon_stage == NEW_MOON; } );
    else if ( util::str_compare_ci( name, "half_moon" ) )
      return make_fn_expr( name, [ this ] { return moon_stage == HALF_MOON; } );
    else if ( util::str_compare_ci( name, "full_moon" ) )
      return make_fn_expr( name, [ this ] { return moon_stage >= FULL_MOON; } );

    // automatic resolution of Celestial Alignment vs talented Incarnation
    if ( splits.size() >= 2 && util::str_compare_ci( splits[ 1 ], "ca_inc" ) )
    {
      splits[ 1 ] = talent.incarnation_moonkin.ok() ? "incarnation_chosen_of_elune" : "celestial_alignment";

      return druid_t::create_expression( util::string_join( splits, "." ) );
    }

    if ( splits.size() == 3 && util::str_compare_ci( splits[ 0 ], "buff" ) &&
         util::str_compare_ci( splits[ 1 ], "fury_of_elune" ) )
    {
      if ( util::str_compare_ci( splits[ 2 ], "stack" ) )
      {
        return make_fn_expr( name, [ this ] {
          return buff.fury_of_elune->check() + buff.sundered_firmament->check();
        } );
      }
      else if ( util::str_compare_ci( splits[ 2 ], "remains" ) )
      {
        return make_fn_expr( name, [ this ] {
          return std::max( buff.fury_of_elune->remains(), buff.sundered_firmament->remains() );
        } );
      }
    }

    static constexpr std::array<std::string_view, 4> control_cooldowns{
      { "force_of_nature", "incarnation_chosen_of_elune", "celestial_alignment", "convoke_the_spirits" }
    };

    if ( talent.control_of_the_dream.ok() && splits.size() == 3 &&
         util::str_compare_ci( splits[ 0 ], "cooldown" ) &&
         util::str_compare_ci( splits[ 2 ], "duration" ) &&
         range::any_of( control_cooldowns, [ s = splits[ 1 ] ]( std::string_view cd ) {
           return util::str_compare_ci( s, cd );
         } ) )
    {
      if ( auto cd = get_cooldown( splits[ 1 ] ) )
      {
        if ( cd->duration == 0_ms )
          return expr_t::create_constant( name, 0 );

        timespan_t max_diff = timespan_t::from_seconds( talent.control_of_the_dream->effectN( 1 ).base_value() );

        return make_fn_expr( name, [ cd, max_diff, this ] {
          auto dur = cooldown_t::cooldown_duration( cd );

          if ( cd->charges != cd->current_charge )
            return dur;

          auto last = cd->charges > 1 ? cd->last_charged : cd->ready;
          auto diff = 0_ms;

          if ( last <= 0_ms )
            diff = max_diff;
          else if ( auto cur = sim->current_time(); cur > last )
            diff = std::min( max_diff, cur - last );

          assert( dur - diff > 0_ms );
          return dur - diff;
        } );
      }
    }
  }
  else if ( specialization() == DRUID_FERAL )
  {
    if ( splits.size() >= 2 && util::str_compare_ci( splits[ 1 ], "moonfire_cat" ) )
    {
      splits[ 1 ] = "lunar_inspiration";
      return druid_t::create_expression( util::string_join( splits, "." ) );
    }
  }

  // Convert [talent/buff/cooldown].incarnation.* to spec-based incarnations
  if ( splits.size() >= 2 && util::str_compare_ci( splits[ 1 ], "incarnation" ) &&
       ( util::str_compare_ci( splits[ 0 ], "buff" ) || util::str_compare_ci( splits[ 0 ], "talent" ) ||
         util::str_compare_ci( splits[ 0 ], "cooldown" ) ) )
  {
    switch ( specialization() )
    {
      case DRUID_BALANCE:     splits[ 1 ] = "incarnation_chosen_of_elune";    break;
      case DRUID_FERAL:       splits[ 1 ] = "incarnation_avatar_of_ashamane"; break;
      case DRUID_GUARDIAN:    splits[ 1 ] = "incarnation_guardian_of_ursoc";  break;
      case DRUID_RESTORATION: splits[ 1 ] = "incarnation_tree_of_life";       break;
      default: break;
    }

    return druid_t::create_expression( util::string_join( splits, "." ) );
  }

  if ( splits.size() >= 2 && util::str_compare_ci( splits[ 1 ], "berserk" ) )
  {
    if ( specialization() == DRUID_FERAL )
      splits[ 1 ] = "berserk_cat";
    else if ( specialization() == DRUID_GUARDIAN )
      splits[ 1 ] = "berserk_bear";
    else
      splits[ 1 ] = "berserk";

    return druid_t::create_expression( util::string_join( splits, "." ) );
  }

  if ( splits.size() >= 2 && util::str_compare_ci( splits[ 1 ], "bs_inc" ) )
  {
    if ( specialization() == DRUID_FERAL )
    {
      if ( talent.incarnation_cat.ok() )
        splits[ 1 ] = "incarnation_avatar_of_ashamane";
      else
        splits[ 1 ] = "berserk_cat";
    }
    else if ( specialization() == DRUID_GUARDIAN )
    {
      if ( talent.incarnation_bear.ok() )
        splits[ 1 ] = "incarnation_guardian_of_ursoc";
      else
        splits[ 1 ] = "berserk_bear";
    }
    else
    {
      splits[ 1 ] = "berserk";
    }

    return druid_t::create_expression( util::string_join( splits, "." ) );
  }

  if ( splits.size() >= 2 && util::str_compare_ci( splits[ 1 ], "clearcasting" ) )
  {
    if ( specialization() == DRUID_FERAL )
      splits[ 1 ] = "clearcasting_cat";
    else
      splits[ 1 ] = "clearcasting_tree";

    return druid_t::create_expression( util::string_join( splits, "." ) );
  }

  return player_t::create_expression( name );
}

void druid_t::create_options()
{
  player_t::create_options();

  // General
  add_option( opt_bool( "druid.no_cds", options.no_cds ) );
  add_option( opt_bool( "druid.raid_combat", options.raid_combat ) );

  // Balance
  add_option( opt_float( "druid.initial_astral_power", options.initial_astral_power ) );
  add_option( opt_int( "druid.initial_moon_stage", options.initial_moon_stage ) );
  add_option( opt_int( "druid.initial_orbit_breaker_stacks", options.initial_orbit_breaker_stacks ) );

  // Feral
  add_option( opt_bool( "druid.disable_ready_trigger", options.disable_ready_trigger ) );

  // Guardian

  // Restoration
  add_option( opt_float( "druid.time_spend_healing", options.time_spend_healing ) );
}

role_e druid_t::primary_role() const
{
  // First, check for the user-specified role
  switch ( player_t::primary_role() )
  {
    case ROLE_TANK:
    case ROLE_ATTACK:
    case ROLE_SPELL:
      return player_t::primary_role();
      break;
    default:
      break;
  }

  // Else, fall back to spec
  switch ( specialization() )
  {
    case DRUID_BALANCE:
      return ROLE_SPELL; break;
    case DRUID_GUARDIAN:
      return ROLE_TANK; break;
    default:
      return ROLE_ATTACK;
      break;
  }
}

stat_e druid_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work for certain specs into the appropriate
  // "basic" stats
  switch ( s )
  {
    case STAT_STR_AGI_INT:
      switch ( specialization() )
      {
        case DRUID_BALANCE:
        case DRUID_RESTORATION: return STAT_INTELLECT;
        case DRUID_FERAL:
        case DRUID_GUARDIAN: return STAT_AGILITY;
        default: return STAT_NONE;
      }
    case STAT_AGI_INT:
      if ( specialization() == DRUID_BALANCE || specialization() == DRUID_RESTORATION )
        return STAT_INTELLECT;
      else
        return STAT_AGILITY;
    case STAT_STR_AGI: return STAT_AGILITY;
    case STAT_STR_INT: return STAT_INTELLECT;
    case STAT_SPIRIT:
      if ( specialization() == DRUID_RESTORATION )
        return s;
      else
        return STAT_NONE;
    case STAT_BONUS_ARMOR:
      if ( specialization() == DRUID_GUARDIAN )
        return s;
      else
        return STAT_NONE;
    default: return s;
  }
}

resource_e druid_t::primary_resource() const
{
  if ( specialization() == DRUID_BALANCE )
    return RESOURCE_ASTRAL_POWER;

  if ( specialization() == DRUID_GUARDIAN )
    return RESOURCE_RAGE;

  if ( primary_role() == ROLE_HEAL || primary_role() == ROLE_SPELL )
    return RESOURCE_MANA;

  return RESOURCE_ENERGY;
}

void druid_t::init_absorb_priority()
{
  auto add_absorb = [ this ]( buff_t* b ) {
    if ( b->data().id() )
      absorb_priority.push_back( b->data().id() );
  };

  add_absorb( buff.brambles );      // brambles always goes first
}

void druid_t::assess_damage( school_e school, result_amount_type dtype, action_state_t* s )
{
  if ( dbc::is_school( school, SCHOOL_PHYSICAL ) && dtype == result_amount_type::DMG_DIRECT && s->result == RESULT_HIT )
    buff.ironfur->up();

  player_t::assess_damage( school, dtype, s );
}

// Target Data ==============================================================
template <typename Buff, typename... Args>
inline buff_t* druid_td_t::make_debuff( bool b, Args&&... args )
{
  return buff_t::make_buff_fallback<Buff>( target->is_enemy() && b, std::forward<Args>( args )... );
}

struct bloodseeker_vines_debuff_t : public buffs::druid_buff_t
{
  druid_td_t* target_data;
  bool initial_twin_sprouts = false;

  bloodseeker_vines_debuff_t( druid_td_t& td, druid_t* p )
    : buffs::druid_buff_t( td, "bloodseeker_vines", p->spec.bloodseeker_vines ), target_data( &td )
  {
    set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );
    set_activated( true );

    // bursting growth needs to be executed first so it benefits from the expiring root network. note that in-game logs
    // show root network expiring first, but bursting growth damage is calculated before buff expiration.
    if ( p->talent.bursting_growth.ok() )
    {
      add_stack_change_callback( [ p = p ]( buff_t* b, int, int new_ ) {
        // NOTE: we only account for the expiration burst here, which doesn't benefit from vine-debuff multipliers.
        // burst from scripted "expiration" which doesn't fully expire the actual dot is handled in decrement()
        if ( !new_ )
          p->active.bursting_growth->execute_on_target( b->player );
      } );
    }
    if ( p->talent.root_network.ok() )
    {
      add_stack_change_callback( [ p = p ]( buff_t*, int old_, int new_ ) {
        auto diff = new_ - old_;
        if ( diff > 0 )
          p->buff.root_network->trigger( diff );
        else if ( diff < 0 )
          p->buff.root_network->decrement( -diff );
      } );
    }
  }

  // if the initial dot is doubled via twin sprouts, only one is decremented after 6s. track an initial twin sprouts and
  // since this will be the first decrement event, decrement by one when tracked
  void start( int s, double v, timespan_t d ) override
  {
    initial_twin_sprouts = s == 2 && !check();

    buffs::druid_buff_t::start( s, v, d );
  }

  void decrement( int s, double v ) override
  {
    // if the initial dot is doubled via twin sprouts, only one is decremented after 6s. it's possible to have remaining
    // stacks upon dot expiration so handle this by completely removing all stacks if the dot is no longer ticking.
    if ( !target_data->dots.bloodseeker_vines->is_ticking() )
    {
      s = current_stack;
    }
    else if ( s == 2 && initial_twin_sprouts )
    {
      s = 1;
      initial_twin_sprouts = false;
    }

    // if this is not a real expiration but scripted decrement, burst happens before the decrement gaining bonuses of
    // the pre-decrement number of stacks
    if ( p()->active.bursting_growth && current_stack - s > 0 )
    {
      auto dec = s;  // make a copy
      while ( dec-- )
        p()->active.bursting_growth->execute_on_target( player );
    }

    buffs::druid_buff_t::decrement( s, v );
  }
};

druid_td_t::druid_td_t( player_t& target, druid_t& source )
  : actor_target_data_t( &target, &source ), dots(), hots(), debuff(), buff()
{
  if ( target.is_enemy() )
  {
    dots.bloodseeker_vines     = target.get_dot( "bloodseeker_vines", &source );
    dots.dreadful_wound        = target.get_dot( "dreadful_wound", &source );
    dots.lunar_inspiration     = target.get_dot( "lunar_inspiration", &source );
    dots.moonfire              = target.get_dot( "moonfire", &source );
    dots.red_moon              = target.get_dot( "red_moon", &source );
    dots.rake                  = target.get_dot( "rake", &source );
    dots.rip                   = target.get_dot( "rip", &source );
    dots.thrash                = target.get_dot( "thrash", &source );

    debuff.atmospheric_exposure = make_debuff( source.talent.atmospheric_exposure.ok(),
      *this, "atmospheric_exposure", source.spec.atmospheric_exposure )
        ->set_trigger_spell( source.talent.atmospheric_exposure )
        ->set_stack_change_callback( [ p = &source ]( auto, int old_, int new_ ) {
          if ( new_ )
            p->uptime.atmospheric_exposure->update( true, p->sim->current_time() );
          else if ( old_ )
            p->uptime.atmospheric_exposure->update( false, p->sim->current_time() );
        } );

    debuff.bloodseeker_vines = make_debuff<bloodseeker_vines_debuff_t>( source.talent.thriving_growth.ok(),
      *this, "bloodseeker_vines", &source );

    debuff.sabertooth = make_debuff( source.talent.sabertooth.ok(), *this, "sabertooth", source.spec.sabertooth )
      ->set_trigger_spell( source.talent.sabertooth )
      ->set_max_stack( as<int>( source.resources.base[ RESOURCE_COMBO_POINT ] ) );

    debuff.stellar_amplification = make_debuff( source.talent.stellar_amplification.ok(),
      *this, "stellar_amplification", source.spec.stellar_amplification )
        ->set_trigger_spell( source.talent.stellar_amplification )
        ->set_refresh_duration_callback(
          [ dur = source.talent.stellar_amplification->effectN( 1 ).time_value() ]( const buff_t* b, timespan_t d ) {
            return std::min( dur, b->remains() + d );
          } )
        ->set_stack_change_callback( [ p = &source ]( auto, int old_, int new_ ) {
          if ( new_ )
            p->uptime.stellar_amplification->update( true, p->sim->current_time() );
          else if ( old_ )
            p->uptime.stellar_amplification->update( false, p->sim->current_time() );
        } );
  }
  else
  {
    hots.cultivation           = target.get_dot( "cultivation", &source );
    hots.germination           = target.get_dot( "germination", &source );
    hots.lifebloom             = target.get_dot( "lifebloom", &source );
    hots.regrowth              = target.get_dot( "regrowth", &source );
    hots.rejuvenation          = target.get_dot( "rejuvenation", &source );
    hots.wild_growth           = target.get_dot( "wild_growth", &source );

    buff.ironbark =
      make_buff_fallback( source.talent.ironbark.ok() && !target.is_enemy(), *this, "ironbark", source.talent.ironbark )
        ->set_cooldown( 0_ms );
  }
}

int druid_td_t::hots_ticking() const
{
  unsigned count = 0;

  static constexpr auto hots_list = {
    &druid_td_t::hots_t::cultivation,
    &druid_td_t::hots_t::germination,
    &druid_td_t::hots_t::regrowth,
    &druid_td_t::hots_t::rejuvenation,
    &druid_td_t::hots_t::wild_growth
  };

  for ( const auto& hot_ref : hots_list )
    if ( hots.*hot_ref && ( hots.*hot_ref )->is_ticking() )
      count++;

  auto lb_mul = 1 + as<int>( static_cast<druid_t*>( source )->talent.harmonious_blooming->effectN( 1 ).base_value() );
  count += lb_mul * hots.lifebloom->is_ticking();

  return count;
}

const druid_td_t* druid_t::find_target_data( const player_t* t ) const
{
  assert( t );
  return target_data[ t ];
}

druid_td_t* druid_t::get_target_data( player_t* t ) const
{
  assert( t );
  druid_td_t*& td = target_data[ t ];
  if ( !td )
    td = new druid_td_t( *t, const_cast<druid_t&>( *this ) );

  return td;
}

// ==========================================================================
// druid_t utility functions
// ==========================================================================
void druid_t::init_beast_weapon( weapon_t& w, double swing_time )
{
  // use main hand weapon as base
  w = main_hand_weapon;

  if ( w.type == WEAPON_NONE )
  {
    // if main hand weapon is empty, use unarmed damage unarmed base beast weapon damage range is 1-1 Jul 25 2018
    w.min_dmg = w.max_dmg = w.damage = 1;
  }
  else
  {
    // Otherwise normalize the main hand weapon's damage to the beast weapon's speed.
    double normalizing_factor = swing_time / w.swing_time.total_seconds();
    w.min_dmg *= normalizing_factor;
    w.max_dmg *= normalizing_factor;
    w.damage *= normalizing_factor;
  }

  w.type       = WEAPON_BEAST;
  w.school     = SCHOOL_PHYSICAL;
  w.swing_time = timespan_t::from_seconds( swing_time );
}

void druid_t::adjust_health_pct( double mul, bool increase )
{
  if ( increase )
  {
    resources.max[ RESOURCE_HEALTH ] *= mul;
    resources.current[ RESOURCE_HEALTH ] *= mul;
  }
  else
  {
    resources.max[ RESOURCE_HEALTH ] /= mul;
    resources.current[ RESOURCE_HEALTH ] /= mul;
  }

  recalculate_resource_max( RESOURCE_HEALTH );
}

const spell_data_t* druid_t::apply_override( const spell_data_t* base_spell, const spell_data_t* passive_spell ) const
{
  if ( !passive_spell->ok() )
    return base_spell;

  for ( const auto& eff : passive_spell->effects() )
    if ( eff.type() == E_APPLY_AURA && eff.subtype() == A_OVERRIDE_ACTION_SPELL )
      if ( eff.misc_value1() == as<int>( base_spell->id() ) || base_spell->affected_by( eff ) )
        return find_spell( as<unsigned>( eff.base_value() ) );

  return spell_data_t::not_found();
}

bool druid_t::uses_form( specialization_e s, std::string_view name, action_t* action ) const
{
  if ( specialization() == s )
    return true;

  for ( auto a : action_list )
  {
    if ( a->name_str == name && !a->dual )
      return true;

    if ( auto tmp = dynamic_cast<druid_action_data_t*>( a ) )
    {
      if ( tmp->autoshift == action )
        return true;
    }
  }

  return false;
}

bool druid_t::uses_cat_form() const
{
  return uses_form( DRUID_FERAL, "cat_form", active.shift_to_cat );
}

bool druid_t::uses_bear_form() const
{
  return uses_form( DRUID_GUARDIAN, "bear_form", active.shift_to_bear );
}

bool druid_t::uses_moonkin_form() const
{
  return uses_form( DRUID_BALANCE, "moonkin_form", active.shift_to_moonkin );
}

player_t* druid_t::get_smart_target( const std::vector<player_t*>& _tl, dot_t* druid_td_t::dots_t::*dot,
                                     player_t* exclude, double dis, bool really_smart )
{
  if ( !_tl.size() )
    return nullptr;

  auto tl = _tl;  // make a copy

  if ( exclude )
  {
    if ( dis && sim->distance_targeting_enabled )
    {
      // remove out of range
      range::erase_remove( tl, [ exclude, dis ]( player_t* t ) {
        return t == exclude || t->get_player_distance( *exclude ) > dis;
      } );
    }
    else
    {
      range::erase_remove( tl, exclude );
    }
  }

  if ( tl.size() > 1 )
  {
    // randomize remaining targets
    rng().shuffle( tl.begin(), tl.end() );

    if ( really_smart )
    {
      // sort by time remaining
      range::sort( tl, [ this, &dot ]( player_t* a, player_t* b ) {
        auto td_a = get_target_data( a );
        auto td_b = get_target_data( b );

        if ( td_a && td_b )
          return std::invoke( dot, td_a->dots )->remains() < std::invoke( dot, td_b->dots )->remains();
        else
          return ( td_a != nullptr ) < ( td_b != nullptr );
      } );
    }
    else
    {
      // prioritize undotted over dotted
      std::partition( tl.begin(), tl.end(), [ this, &dot ]( player_t* t ) {
        return !std::invoke( dot, get_target_data( t )->dots )->is_ticking();
      } );
    }
  }

  if ( tl.size() )
    return tl[ 0 ];

  return nullptr;
}

modified_spell_data_t* druid_t::get_modified_spell( const spell_data_t* s )
{
  if ( s && s->ok() )
  {
    for ( auto m : modified_spells )
      if ( m->_spell.id() == s->id() )
        return m;

    return modified_spells.emplace_back( new modified_spell_data_t( s ) );
  }

  return modified_spell_data_t::nil();
}

void druid_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  options = static_cast<druid_t*>( source )->options;
}

void druid_t::moving()
{
  if ( ( executing && !executing->usable_moving() ) || ( channeling && !channeling->usable_moving() ) )
    player_t::interrupt();
}

action_t* druid_t::execute_action()
{
  auto a = player_t::execute_action();

  if ( a && a->type != ACTION_OTHER && a->type != ACTION_CALL && a->type != ACTION_SEQUENCE )
  {
    if ( spell_queued.blooming_infusion_damage )
    {
      spell_queued.blooming_infusion_damage = false;

      if ( a->id == 190984 || a->id == 194153 )  // wrath, starfire
        make_event( *sim, [ this ] { buff.blooming_infusion_damage->trigger(); } );
      else
        buff.blooming_infusion_damage->trigger();
    }

    if ( spell_queued.blooming_infusion_damage_expire )
    {
      auto prev_action = spell_queued.blooming_infusion_damage_expire;
      spell_queued.blooming_infusion_damage_expire = nullptr;

      if ( a->id == 190984 || a->id == 194153 )  // wrath, starfire
      {
        make_event( *sim, [ this, a ] {
          spell_queued.blooming_infusion_damage_expire = nullptr;
          buff.blooming_infusion_damage->consume( a );
        } );
      }
      else
      {
        buff.blooming_infusion_damage->consume( prev_action );
      }
    }

    if ( spell_queued.star_cascade )
    {
      if ( a->id == 1271222 )  // starsurge (cascade)
        make_event( *sim, [ this, t = spell_queued.star_cascade ] { active.star_cascade->execute_on_target( t ); } );
      else
        active.star_cascade->execute_on_target( spell_queued.star_cascade );

      spell_queued.star_cascade = nullptr;
    }
  }

  return a;
}

void druid_t::print_custom_parsed_effects( report::sc_html_stream& os ) const
{
  parse_player_effects_t::print_custom_parsed_effects( os );
  modified_spell_data_t::parsed_effects_html( os, *sim, modified_spells );
}

// ==========================================================================
// DBC/Spell data based auto-parsing
// ==========================================================================

void druid_t::parse_action_effects( action_t* action )
{
  auto _a = dynamic_cast<parse_action_base_t*>( action );
  assert( _a );

  // Class
  _a->parse_effects( buff.bear_form );
  _a->parse_effects( spec.bear_form_passive_2, [ this ] { return form == BEAR_FORM; } );
  _a->parse_effects( buff.cat_form );
  _a->parse_effects( spec.cat_form_passive_2, [ this ] { return form == CAT_FORM; } );
  _a->parse_effects( buff.moonkin_form );

  if ( talent.lycaras_inspiration.ok() )
    _a->parse_effects( buff.lycaras_teachings_haste );

  // Balance
  _a->parse_effects( mastery.astral_invocation,
                     // arcane passive mastery (eff#1) applies to orbital strike, goldrinn's fang (label 2391)
                     // lunar bolt (1263137), meteorites (1240913)
                     affect_list_t( 1 ).add_label( 2391 ).add_spell( 1263137, 1240913 ),
                     // nature passive mastery (eff#3) applies to orbital strike, goldrinn's fang (label 2391)
                     // and dream burst (433850), solar bolt (1261573), and meteorites (1240913)
                     affect_list_t( 3 ).add_label( 2391 ).add_spell( 433850, 1261573, 1240913 ) );

  _a->parse_effects( buff.ascendant_fires );  // consumption handled within starfire_base_t/wrath_base_t
  _a->parse_effects( buff.ascendant_stars, CONSUME_BUFF );
  _a->parse_effects( buff.astral_communion );

  // talent data for balance of all things only modifies effect#1 of the buff, and is missing modification to effect#3
  // which is done via hidden script. hack around this by overriding the value instead of normally parsing the talent.
  _a->parse_effects( buff.balance_of_all_things_arcane );
  _a->parse_effects( buff.balance_of_all_things_arcane, effect_mask_t( false ).enable( 3 ),
                     talent.balance_of_all_things->effectN( 1 ).percent() );
  // nature boat applies to dream burst (433850) via hidden script
  _a->parse_effects( buff.balance_of_all_things_nature, affect_list_t( 1 ).add_spell( 433850 ) );
  _a->parse_effects( buff.balance_of_all_things_nature, effect_mask_t( false ).enable( 3 ),
                     talent.balance_of_all_things->effectN( 1 ).percent() );

  // due to harmony of the heavens, we parse the damage effects (#1/#2) separately and use the current buff value
  // instead of data value
  _a->parse_effects( buff.eclipse_lunar, effect_mask_t( true ).disable( 1, 2 ) );
  _a->parse_effects( buff.eclipse_lunar, effect_mask_t( false ).enable( 1, 2 ), USE_CURRENT,
                     // damage (eff#1) applies to orbital strike, goldrinn's fang (label 2391),
                     // lunar bolt (1263137), and meteorites (1240913)
                     affect_list_t( 1 ).add_label( 2391 ).add_spell( 1263137, 1240913 ) );

  // due to harmony of the heavens, we parse the damage effects (#1/#2) separately and use the current buff value
  // instead of data value
  _a->parse_effects( buff.eclipse_solar, effect_mask_t( true ).disable( 1, 2 ) );
  _a->parse_effects( buff.eclipse_solar, effect_mask_t( false ).enable( 1, 2 ), USE_CURRENT,
                     // damage (eff#1) applies to orbital strike, goldrinn's fang (label 2391)
                     // and dream burst (433850), solar bolt (1261573), and meteorites (1240913)
                     affect_list_t( 1 ).add_label( 2391 ).add_spell( 433850, 1261573, 1240913 ) );

  _a->parse_effects( buff.elunes_challenge );

  _a->parse_effects( buff.incarnation_moonkin, effect_mask_t( false ).enable( 1, 2, 3, 4 ) );
  // additional effects if astral_insight is talented
  if ( talent.astral_insight.ok() )
  {
    _a->parse_effects( buff.incarnation_moonkin, effect_mask_t( false ).enable( 6, 7 ) );
    // further additional effects if lunar calling is also talented
    if ( talent.lunar_calling.ok() )
      _a->parse_effects( buff.incarnation_moonkin, effect_mask_t( false ).enable( 8, 9 ) );
  }

  _a->parse_effects( buff.owlkin_frenzy );
  _a->parse_effects( buff.starweaver_starfall );
  _a->parse_effects( buff.starweaver_starsurge );
  _a->parse_effects( buff.touch_the_cosmos );

  // Feral
  _a->parse_effects( mastery.razor_claws );
  _a->parse_effects( buff.apex_predators_craving );
  _a->parse_effects( buff.berserk_cat );
  _a->parse_effects( buff.coiled_to_spring, CONSUME_BUFF );
  _a->parse_effects( buff.hunger_for_battle );
  _a->parse_effects( buff.incarnation_cat );
  _a->parse_effects( buff.predator, USE_CURRENT );
  _a->parse_effects( buff.predatory_swiftness, CONSUME_BUFF );
  _a->parse_effects( buff.sudden_ambush, CONSUME_BUFF );
  _a->parse_effects( talent.taste_for_blood, [ this ] { return buff.tigers_fury->check();},
                     talent.taste_for_blood->effectN( 2 ).percent() );
  _a->parse_effects( buff.unseen_predators_craving, talent.unseen_predator_2->effectN( 1 ).percent() );

  // Guardian
  _a->parse_effects( buff.berserk_bear, effect_mask_t( true ).disable( 6, 7, 8, 9 ) );
  _a->parse_effects( buff.incarnation_bear, effect_mask_t( true ).disable( 6, 7, 8, 9, 14, 15, 16, 17 ) );
  // additional effects if astral insight is talented
  if ( talent.astral_insight.ok() )
  {
    _a->parse_effects( buff.incarnation_bear, effect_mask_t( false ).enable( 14, 15 ) );
    // further additional effects if lunar calling is also talented
    if ( talent.lunar_calling.ok() )
      _a->parse_effects( buff.incarnation_bear, effect_mask_t( false ).enable( 16, 17 ) );
  }

  // snapshots regrowth hot, so disable the effect
  _a->parse_effects( buff.dream_of_cenarius, effect_mask_t( true ).disable( 5 ), CONSUME_BUFF );

  _a->parse_effects( buff.gory_fur, CONSUME_BUFF );

  if ( talent.penumbral_swell.ok() )
    _a->parse_effects( buff.lunar_beam );

  // Restoration
  _a->parse_effects( buff.abundance );
  _a->parse_effects( buff.call_of_the_elder_druid, effect_mask_t( true ).disable( 10, 11, 12 ) );
  _a->parse_effects( buff.clearcasting_tree );
  _a->parse_effects( buff.incarnation_tree );
  _a->parse_effects( buff.natures_swiftness, CONSUME_BUFF );

  // Hero talents
  _a->parse_effects( buff.blooming_infusion_damage, [ action, this ]( action_state_t* ) {
    if ( !action->proc && !action->background && !action->is_precombat && action->time_to_execute > 0_ms )
      spell_queued.blooming_infusion_damage_expire = action;
    else
      buff.blooming_infusion_damage->consume( action );
  } );
  _a->parse_effects( buff.blooming_infusion_heal, CONSUME_BUFF );
  _a->parse_effects( buff.feline_potential, CONSUME_BUFF );
  _a->parse_effects( buff.harmony_of_the_grove );
  _a->parse_effects( buff.root_network );
  _a->parse_effects( buff.strategic_infusion );
  _a->parse_effects( buff.ursine_potential, CONSUME_BUFF );
}

void druid_t::parse_action_target_effects( action_t* action )
{
  auto _a = dynamic_cast<parse_action_base_t*>( action );
  assert( _a );

  _a->parse_target_effects( d_fn( &druid_td_t::debuffs_t::atmospheric_exposure ), spec.atmospheric_exposure );
  _a->parse_target_effects( d_fn( &druid_td_t::debuffs_t::sabertooth ), spec.sabertooth,
                            talent.sabertooth->effectN( 2 ).percent() );
  _a->parse_target_effects( d_fn( &druid_td_t::debuffs_t::stellar_amplification ), spec.stellar_amplification );
  _a->parse_target_effects( d_fn( &druid_td_t::dots_t::thrash ), spec.thrash_bleed );

  if ( talent.exacerbating_wounds.ok() )
    _a->parse_target_effects( d_fn( &druid_td_t::dots_t::dreadful_wound ), spec.dreadful_wound );
}

void druid_t::parse_player_effects()
{
  parse_effects( mastery.natures_guardian );
  parse_effects( mastery.natures_guardian_AP );
  parse_effects( talent.thick_hide, PARSE_PASSIVE );

  parse_effects( buff.bear_form, effect_mask_t( true ).disable( 13, 14 ) );
  parse_effects( spec.bear_form_passive, [ this ] { return form == BEAR_FORM; } );
  parse_effects( spec.bear_form_passive_2, [ this ] { return form == BEAR_FORM; } );
  parse_effects( spec.cat_form_speed, [ this ] { return form == CAT_FORM; } );
  parse_effects( buff.moonkin_form, effect_mask_t( true ).disable( 12, 13 ) );

  if ( talent.glistening_fur.ok() )
  {
    parse_effects( buff.bear_form, effect_mask_t( false ).enable( 13, 14 ) );
    parse_effects( buff.moonkin_form, effect_mask_t( false ).enable( 12, 13 ) );
  }

  parse_effects( buff.barkskin );
  parse_effects( buff.dash, [ this ] { return form == CAT_FORM; } );
  parse_effects( buff.tiger_dash, [ this ] { return form == CAT_FORM; } );
  parse_effects( buff.forestwalk );
  parse_effects( buff.killing_strikes );

  if ( talent.lycaras_inspiration.ok() )
  {
    parse_effects( buff.lycaras_teachings_crit );
    parse_effects( buff.lycaras_teachings_vers );
  }

  parse_effects( buff.persistence, effect_mask_t( false ).enable( 1 ),  // stamina
                 find_effect( spec.bear_form_passive, A_MOD_TOTAL_STAT_PERCENTAGE ).percent() /
                   buff.persistence->max_stack() );
  parse_effects( buff.persistence, effect_mask_t( false ).enable( 2 ),  // armor
                 find_effect( buff.bear_form, A_MOD_BASE_RESISTANCE_PCT ).percent() /
                   buff.persistence->max_stack() );
  parse_effects( buff.prowl, effect_mask_t( false ).enable( 3 ), racials.elusiveness->effectN( 1 ).percent() );
  parse_effects( buff.ruthless_aggression );
  parse_effects( buff.survival_instincts );
  parse_effects( buff.ursine_vigor, talent.ursine_vigor );
  parse_effects( buff.wildshape_mastery, effect_mask_t( false ).enable( 2 ),  // base armor
                 find_effect( buff.bear_form, A_MOD_BASE_RESISTANCE_PCT ).percent() *
                 buff.wildshape_mastery->data().effectN( 1 ).percent() );
  parse_effects( buff.wildshape_mastery, effect_mask_t( false ).enable( 3 ),  // stamina
                 find_effect( spec.bear_form_passive, A_MOD_TOTAL_STAT_PERCENTAGE ).percent() *
                 buff.wildshape_mastery->data().effectN( 1 ).percent() );
  parse_effects( buff.wildshape_mastery, effect_mask_t( false ).enable( 4, 5 ) );  // crit avoidance & expertise

  parse_target_effects( d_fn( &druid_td_t::debuffs_t::bloodseeker_vines ), spec.bloodseeker_vines );
  parse_target_effects( d_fn( &druid_td_t::dots_t::dreadful_wound ), spec.dreadful_wound );
  parse_target_effects( [ this ]( auto ) { return hots.frenzied_regeneration->is_ticking(); },
                        talent.frenzied_regeneration );

  if ( talent.scintillating_moonlight.ok() )
  {
    parse_target_effects( d_fn( &druid_td_t::dots_t::moonfire ), spec.moonfire_dmg, effect_mask_t( false ).enable( 6 ),
                          -talent.scintillating_moonlight->effectN( 1 ).percent() );
  }

  parse_target_effects( d_fn( &druid_td_t::dots_t::thrash ), spec.thrash_bleed );

  if ( talent.wild_charge.ok() )
  {
    add_parse_entry( non_stacking_movement_effects )
      .set_buff( buff.wild_charge_movement )
      .set_type( USE_CURRENT )
      .set_eff( &talent.wild_charge->effectN( 2 ) )
      .print_debug( this );
  }
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class druid_report_t final : public player_report_extension_t
{
  druid_t& p;

public:
  druid_report_t( druid_t& player ) : p( player ) {}

  void html_customsection( report::sc_html_stream& os ) override
  {
    os << R"(<div class="player-section custom_section">)";

    benefit_tracker_t::print_table<SNAPSHOT_TRACKER>( &p, os, "Snapshot Tracker" );

    os << "</div>\n";
  }
};

// DRUID MODULE INTERFACE ===================================================
struct druid_module_t final : public module_t
{
  druid_module_t() : module_t( DRUID ) {}

  player_t* create_player( sim_t* sim, std::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p = new druid_t( sim, name, r );
    p->report_extension = std::make_unique<druid_report_t>( *p );
    return p;
  }
  bool valid() const override { return true; }

  void init( player_t* p ) const override
  {
    p->buffs.stampeding_roar = make_buff( p, "stampeding_roar", p->find_spell( 106898 ) )
      ->set_cooldown( 0_ms )
      ->set_default_value_from_effect_type( A_MOD_INCREASE_SPEED );
  }

  void static_init() const override {}

  void register_hotfixes() const override {}

  void combat_begin( sim_t* ) const override {}
  void combat_end( sim_t* ) const override {}
};
}  // UNNAMED NAMESPACE

const module_t* module_t::druid()
{
  static druid_module_t m;
  return &m;
}
