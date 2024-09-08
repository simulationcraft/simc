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

namespace pets
{
struct denizen_of_the_dream_t;
struct force_of_nature_t;
struct grove_guardian_t;
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

enum eclipse_e : uint8_t
{
  SOLAR        = 0x01,
  LUNAR        = 0x02,
};

enum flag_e : uint32_t
{
  NONE         = 0x00000000,
  FOREGROUND   = 0x00000001,  // action is directly cast from an APL line
  NOUNSHIFT    = 0x00000002,  // does not automatically unshift into caster form
  AUTOATTACK   = 0x00000004,  // is an autoattack
  ALLOWSTEALTH = 0x00000008,  // does not break stealth
  // free procs
  CONVOKE      = 0x00001000,  // convoke_the_spirits night_fae covenant ability
  FIRMAMENT    = 0x00002000,  // sundered firmament talent
  FLASHING     = 0x00004000,  // flashing claws talent
  GALACTIC     = 0x00008000,  // galactic guardian talent
  ORBIT        = 0x00010000,  // orbit breaker talent
  TWIN         = 0x00020000,  // twin moons talent
  TREANT       = 0x00040000,  // treants of the moon moonfire
  LIGHTOFELUNE = 0x00080000,  // light of elune talent
  THRASHING    = 0x00100000,  // thrashing claws talent
  // free casts
  APEX         = 0x01000000,  // apex predators's craving
  TOOTHANDCLAW = 0x02000000,  // tooth and claw talent
  // misc
  UMBRAL       = 0x10000000,  // umbral embrace talent

  FREE_PROCS = CONVOKE | FIRMAMENT | FLASHING | GALACTIC | ORBIT | TWIN | TREANT | LIGHTOFELUNE,
  FREE_CASTS = APEX | TOOTHANDCLAW
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

struct druid_td_t final : public actor_target_data_t
{
  struct dots_t
  {
    dot_t* adaptive_swarm_damage;
    dot_t* astral_smolder;
    dot_t* bloodseeker_vines;
    dot_t* dreadful_wound;
    dot_t* feral_frenzy;
    dot_t* frenzied_assault;
    dot_t* fungal_growth;
    dot_t* lunar_inspiration;
    dot_t* moonfire;
    dot_t* rake;
    dot_t* rip;
    dot_t* stellar_flare;
    dot_t* sunfire;
    dot_t* tear;
    dot_t* thrash_bear;
    dot_t* thrash_cat;
  } dots;

  struct hots_t
  {
    dot_t* adaptive_swarm_heal;
    dot_t* cenarion_ward;
    dot_t* cultivation;
    dot_t* frenzied_regeneration;
    dot_t* germination;
    dot_t* lifebloom;
    dot_t* regrowth;
    dot_t* rejuvenation;
    dot_t* spring_blossoms;
    dot_t* wild_growth;
  } hots;

  struct debuffs_t
  {
    buff_t* atmospheric_exposure;
    buff_t* bloodseeker_vines;
    buff_t* pulverize;
    buff_t* sabertooth;
    buff_t* stellar_amplification;
    buff_t* tooth_and_claw;
    buff_t* waning_twilight;
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

struct snapshot_counter_t
{
  simple_sample_data_t execute;
  simple_sample_data_t tick;
  simple_sample_data_t waste;
  std::vector<buff_t*> buffs;
  const sim_t* sim;
  stats_t* stats;
  bool is_snapped = false;

  snapshot_counter_t( player_t* p, stats_t* s, buff_t* b )
    : execute(), tick(), waste(), sim( p->sim ), stats( s )
  {
    buffs.push_back( b );
  }

  bool check_all()
  {
    double n_up = 0;

    for ( const auto& b : buffs )
      if ( b->check() )
        n_up++;

    if ( n_up == 0 )
    {
      is_snapped = false;
      return false;
    }

    waste.add( n_up - 1 );
    is_snapped = true;
    return true;
  }

  void add_buff( buff_t* b ) { buffs.push_back( b ); }

  void count_execute()
  {
    // Skip iteration 0 for non-debug, non-log sims
    if ( sim->current_iteration == 0 && sim->iterations > sim->threads && !sim->debug && !sim->log )
      return;

    execute.add( check_all() );
  }

  void count_tick()
  {
    // Skip iteration 0 for non-debug, non-log sims
    if ( sim->current_iteration == 0 && sim->iterations > sim->threads && !sim->debug && !sim->log )
      return;

    tick.add( is_snapped );
  }

  void merge( const snapshot_counter_t& other )
  {
    execute.merge( other.execute );
    tick.merge( other.tick );
    waste.merge( other.waste );
  }
};

struct druid_action_data_t  // variables that need to be accessed from action_t* pointer
{
  // various action flags
  uint32_t action_flags = 0;
  // form spell to automatically cast
  action_t* autoshift = nullptr;

  bool has_flag( uint32_t f ) const { return action_flags & f; }
  bool is_flag( flag_e f ) const { return ( action_flags & f ) == f; }
  bool is_free() const { return action_flags >> 12; }  // first 12 bits are not cost related
};

struct eclipse_handler_t
{
  // final entry in data_array holds # of iterations
  using data_array = std::array<double, 5>;
  using iter_array = std::array<unsigned, 4>;

  struct
  {
    std::vector<data_array> arrays;
    data_array* wrath;
    data_array* starfire;
    data_array* starsurge;
    data_array* starfall;
    data_array* fury_of_elune;
    data_array* new_moon;
    data_array* half_moon;
    data_array* full_moon;
  } data;

  struct
  {
    std::vector<iter_array> arrays;
    iter_array* wrath;
    iter_array* starfire;
    iter_array* starsurge;
    iter_array* starfall;
    iter_array* fury_of_elune;
    iter_array* new_moon;
    iter_array* half_moon;
    iter_array* full_moon;
  } iter;

  druid_t* p;
  unsigned wrath_counter;
  unsigned wrath_counter_base;
  unsigned starfire_counter;
  unsigned starfire_counter_base;
  uint8_t state = 0;

  std::array<uptime_t*, 4> uptimes;

  gain_t* ac_gain = nullptr;
  double ac_ap = 0.0;
  double ga_mod = 0.0;

  eclipse_handler_t( druid_t* player ) : data(), iter(), p( player ) {}

  void init();
  bool enabled() const { return p != nullptr; }

  void cast_wrath();
  void cast_starfire();
  void cast_starsurge();
  void cast_moon( moon_stage_e );
  void tick_starfall();
  void tick_fury_of_elune();

  template <eclipse_e E> buff_t* get_boat() const;
  template <eclipse_e E> buff_t* get_harmony() const;
  template <eclipse_e E> buff_t* get_eclipse() const;
  template <eclipse_e E> void advance_eclipse( bool active );
  template <eclipse_e E> void update_eclipse();

  bool in_none( uint8_t state ) const { return state == 0; }
  bool in_none() const { return state == 0; }
  bool in_eclipse( uint8_t state ) const { return state & ( eclipse_e::LUNAR | eclipse_e::SOLAR ); }
  bool in_eclipse() const { return state & ( eclipse_e::LUNAR | eclipse_e::SOLAR ); }
  bool in_lunar() const { return state & eclipse_e::LUNAR; }
  bool in_solar() const { return state & eclipse_e::SOLAR; }
  bool in_both() const { return in_lunar() && in_solar(); }

  void reset_stacks();
  void reset_state();

  void datacollection_begin();
  void datacollection_end();
  void merge( const eclipse_handler_t& );

  void print_table( report::sc_html_stream& );
  void print_line( report::sc_html_stream&, const spell_data_t*, const data_array& );
};

struct convoke_counter_t
{
  std::unordered_map<action_t*, extended_sample_data_t> data;
  player_t& p;

  convoke_counter_t( player_t& player ) : p( player ) {}

  void analyze()
  {
    for ( auto& d : data )
      d.second.analyze();
  }

  void print_table( report::sc_html_stream& os )
  {
    os << R"(<h3 class="toggle open">Convoke Counter</h3><div class="toggle-content"><table class="sc sort even">)"
       << R"(<thead><tr><th class="toggle-sort left" data-sorttype="alpha">Ability</th>)"
       << R"(<th class="toggle-sort">Avg</th>)"
       << R"(<th class="toggle-sort">Min</th>)"
       << R"(<th class="toggle-sort">Max</th>)"
       << R"(<th class="toggle-sort">StdDev</th>)"
       << R"(<th class="toggle-sort">Var</th></tr></thead>)";

    std::vector<action_t*> _list;

    for ( const auto& [ a, sample ] : data )
      if ( sample.mean() )
        _list.push_back( a );

    range::sort( _list, [ this ]( auto l, auto r ) {
      return data.at( l ).mean() > data.at( r ).mean();
    } );

    for ( auto a : _list )
    {
      auto& sample = data.at( a );
      auto token = highchart::build_id( p, "_" + a->name_str + "_conv_counter" );

      os.format(
        R"(<tbody><tr class="right"><td class="left"><span id="{}_toggle" class="toggle-details">{}</span></td>)",
        token, report_decorators::decorated_action( *a ) );

      os.format( "<td>{:.2f}</td><td>{:.2f}</td><td>{:.2f}</td><td>{:.2f}</td><td>{:.2f}</td>\n",
                 sample.mean(), sample.min(), sample.max(), sample.std_dev, sample.variance );

      os << R"(<tr class="details hide"><td colspan="6">)";

      report_helper::print_distribution_chart( os, p, &sample, a->name_str, token, "_count" );

      os << "</td></tr></tbody>\n";
    }

    os << "</table></div>\n";
  }
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
      fmt::print( s, " {}", *static_cast<const Data*>( this ) );
    return s;
  }

  void copy_state( const action_state_t* o ) override
  {
    Base::copy_state( o );
    *static_cast<Data*>( this ) = *static_cast<const Data*>( static_cast<const druid_action_state_t*>( o ) );
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

  return nullptr;
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

  return spelleffect_data_t::nil();
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
      case E_TRIGGER_SPELL:
      case E_TRIGGER_SPELL_WITH_VALUE:
        return eff;
      case E_APPLY_AURA:
      case E_APPLY_AREA_AURA_PARTY:
        switch( eff.subtype() )
        {
          case A_PROC_TRIGGER_SPELL:
          case A_PROC_TRIGGER_SPELL_WITH_VALUE:
          case A_PERIODIC_TRIGGER_SPELL:
          case A_PERIODIC_TRIGGER_SPELL_WITH_VALUE:
          case A_285:
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

struct druid_t final : public parse_player_effects_t
{
private:
  form_e form = form_e::NO_FORM;  // Active druid form
public:
  eclipse_handler_t eclipse_handler;
  std::vector<std::unique_ptr<snapshot_counter_t>> counters;  // counters for snapshot tracking
  std::unique_ptr<convoke_counter_t> convoke_counter;
  std::vector<std::tuple<unsigned, unsigned, timespan_t, timespan_t, double>> prepull_swarm;
  std::vector<player_t*> swarm_targets;

  // !!!==========================================================================!!!
  // !!! Runtime variables NOTE: these MUST be properly reset in druid_t::reset() !!!
  // !!!==========================================================================!!!
  moon_stage_e moon_stage;
  bool orbital_bug;
  std::vector<event_t*> persistent_event_delay;
  event_t* astral_power_decay;
  struct dot_list_t
  {
    std::vector<dot_t*> moonfire;
    std::vector<dot_t*> sunfire;
    std::vector<dot_t*> thrash_bear;
    std::vector<dot_t*> dreadful_wound;
  } dot_lists;
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
    double adaptive_swarm_jump_distance_melee = 5.0;
    double adaptive_swarm_jump_distance_ranged = 25.0;
    double adaptive_swarm_jump_distance_stddev = 1.0;
    unsigned adaptive_swarm_melee_targets = 7;
    unsigned adaptive_swarm_ranged_targets = 12;
    std::string adaptive_swarm_prepull_setup = "";

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

    // Balance
    action_t* astral_smolder;
    action_t* denizen_of_the_dream;  // placeholder action
    action_t* moons;                 // placeholder action
    action_t* orbit_breaker;
    action_t* orbital_strike;
    action_t* shooting_stars;        // placeholder action
    action_t* shooting_stars_moonfire;
    action_t* shooting_stars_sunfire;
    action_t* crashing_star_moonfire;
    action_t* crashing_star_sunfire;
    action_t* sundered_firmament;
    action_t* sunseeker_mushroom;

    // Feral
    action_t* ferocious_bite_apex;  // free bite from apex predator's crazing
    action_t* frenzied_assault;
    action_t* thrashing_claws;

    // Guardian
    action_t* after_the_wildfire_heal;
    action_t* blazing_thorns;
    action_t* brambles_reflect;
    action_t* elunes_favored_heal;
    action_t* galactic_guardian;
    action_t* maul_tooth_and_claw;
    action_t* raze_tooth_and_claw;
    action_t* thrash_bear_flashing;

    // Restoration
    action_t* yseras_gift;

    // Hero talents
    action_t* bloodseeker_vines;
    action_t* bloodseeker_vines_implant;
    action_t* boundless_moonlight_heal;
    action_t* bursting_growth;
    action_t* dream_burst;
    action_t* the_light_of_elune;
    action_t* treants_of_the_moon_mf;
  } active;

  // Pets
  struct pets_t
  {
    spawner::pet_spawner_t<pets::denizen_of_the_dream_t, druid_t> denizen_of_the_dream;
    spawner::pet_spawner_t<pets::force_of_nature_t, druid_t> force_of_nature;
    spawner::pet_spawner_t<pets::grove_guardian_t, druid_t> grove_guardian;

    pets_t( druid_t* p )
      : denizen_of_the_dream( "denizen_of_the_dream", p ),
        force_of_nature( "force_of_nature", p ),
        grove_guardian( "grove_guardian", p )
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
    buff_t* heart_of_the_wild;
    buff_t* innervate;
    buff_t* ironfur;
    buff_t* lycaras_teachings_haste;  // no form
    buff_t* lycaras_teachings_crit;   // cat form
    buff_t* lycaras_teachings_vers;   // bear form
    buff_t* lycaras_teachings_mast;   // moonkin form
    buff_t* matted_fur;
    buff_t* moonkin_form;
    buff_t* natures_vigil;
    buff_t* rising_light_falling_night_day;
    buff_t* rising_light_falling_night_night;
    buff_t* tiger_dash;
    buff_t* ursine_vigor;
    buff_t* wild_charge_movement;

    // Multi-Spec
    buff_t* survival_instincts;

    // Balance
    buff_t* astral_communion;
    buff_t* balance_of_all_things_arcane;
    buff_t* balance_of_all_things_nature;
    buff_t* celestial_alignment;
    buff_t* denizen_of_the_dream;  // proxy buff to track stack uptime
    buff_t* dreamstate;
    buff_t* eclipse_lunar;
    buff_t* eclipse_solar;
    buff_t* fury_of_elune;  // AP ticks
    buff_t* harmony_of_the_heavens_lunar;  // proxy tracker buff
    buff_t* harmony_of_the_heavens_solar;  // proxy tracker buff
    buff_t* incarnation_moonkin;
    buff_t* natures_balance;
    buff_t* orbit_breaker;
    buff_t* owlkin_frenzy;
    buff_t* parting_skies;  // sundered firmament tracker
    buff_t* shooting_stars_moonfire;
    buff_t* shooting_stars_sunfire;
    buff_t* shooting_stars_stellar_flare;
    buff_t* starweaver_starfall;  // free starfall
    buff_t* starweaver_starsurge;  // free starsurge
    buff_t* solstice;
    buff_t* starfall;
    buff_t* starlord;  // talent
    buff_t* sundered_firmament;  // AP ticks
    buff_t* touch_the_cosmos;
    buff_t* touch_the_cosmos_starfall;   // remove in 11.0.5
    buff_t* touch_the_cosmos_starsurge;  // remove in 11.0.5
    buff_t* umbral_embrace;
    buff_t* umbral_inspiration;
    buff_t* warrior_of_elune;

    // Feral
    buff_t* apex_predators_craving;
    buff_t* ashamanes_guidance;  // buff to track incarnation ashamane's guidance buff 10.2 ptr
    buff_t* berserk_cat;
    buff_t* bloodtalons;
    buff_t* bt_rake;          // dummy buff
    buff_t* bt_shred;         // dummy buff
    buff_t* bt_swipe;         // dummy buff
    buff_t* bt_thrash;        // dummy buff
    buff_t* bt_moonfire;      // dummy buff
    buff_t* bt_feral_frenzy;  // dummy buff
    buff_t* clearcasting_cat;
    buff_t* coiled_to_spring;
    buff_t* frantic_momentum;
    buff_t* incarnation_cat;
    buff_t* incarnation_cat_prowl;
    buff_t* overflowing_power;
    buff_t* predator;
    buff_t* predatory_swiftness;
    buff_t* savage_fury;
    buff_t* sudden_ambush;
    buff_t* tigers_fury;
    buff_t* tigers_tenacity;
    buff_t* tigers_strength;  // TWW1 2pc
    buff_t* fell_prey;        // TWW1 4pc

    // Guardian
    buff_t* after_the_wildfire;
    buff_t* berserk_bear;
    buff_t* blood_frenzy;
    buff_t* brambles;
    buff_t* bristling_fur;
    buff_t* dream_of_cenarius;
    buff_t* earthwarden;
    buff_t* elunes_favored;
    buff_t* galactic_guardian;
    buff_t* gore;
    buff_t* gory_fur;
    buff_t* guardian_of_elune;
    buff_t* incarnation_bear;
    buff_t* lunar_beam;
    buff_t* rage_of_the_sleeper;
    buff_t* tooth_and_claw;
    buff_t* ursocs_fury;
    buff_t* vicious_cycle_mangle;
    buff_t* vicious_cycle_maul;
    buff_t* guardians_tenacity;  // TWW1 2pc

    // Restoration
    buff_t* abundance;
    buff_t* call_of_the_elder_druid;
    buff_t* cenarion_ward;
    buff_t* clearcasting_tree;
    buff_t* flourish;
    buff_t* incarnation_tree;
    buff_t* natures_swiftness;
    buff_t* soul_of_the_forest_tree;
    buff_t* yseras_gift;

    // Hero talents
    buff_t* blooming_infusion_damage;
    buff_t* blooming_infusion_damage_counter;
    buff_t* blooming_infusion_heal;
    buff_t* blooming_infusion_heal_counter;
    buff_t* boundless_moonlight_heal;
    buff_t* bounteous_bloom;
    buff_t* cenarius_might;
    buff_t* dream_burst;
    buff_t* harmony_of_the_grove;
    buff_t* implant;
    buff_t* killing_strikes;
    buff_t* killing_strikes_combat;
    buff_t* lunar_amplification;
    buff_t* lunar_amplification_starfall;
    buff_t* protective_growth;
    buff_t* ravage_fb;
    buff_t* ravage_maul;
    buff_t* root_network;
    buff_t* ruthless_aggression;
    buff_t* strategic_infusion;
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

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* berserk_bear;
    cooldown_t* frenzied_regeneration;
    cooldown_t* fury_of_elune;
    cooldown_t* growl;
    cooldown_t* incarnation_bear;
    cooldown_t* lunar_beam;
    cooldown_t* mangle;
    cooldown_t* moon_cd;  // New / Half / Full Moon
    cooldown_t* natures_swiftness;
    cooldown_t* thrash_bear;
  } cooldown;

  // Gains
  struct gains_t
  {
    // Multiple Specs / Forms
    gain_t* clearcasting;        // Feral & Restoration

    // Feral (Cat)
    gain_t* energy_refund;
    gain_t* overflowing_power;
    gain_t* soul_of_the_forest;

    // Guardian (Bear)
    gain_t* bear_form;
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
    player_talent_t astral_influence;
    player_talent_t cyclone;
    player_talent_t feline_swiftness;
    player_talent_t fluid_form;
    player_talent_t forestwalk;
    player_talent_t frenzied_regeneration;
    player_talent_t heart_of_the_wild;
    player_talent_t hibernate;
    player_talent_t improved_barkskin;
    player_talent_t improved_rejuvenation;
    player_talent_t improved_stampeding_roar;
    player_talent_t improved_sunfire;
    player_talent_t incapacitating_roar;
    player_talent_t innervate;
    player_talent_t instincts_of_the_claw;
    player_talent_t ironfur;
    player_talent_t killer_instinct;
    player_talent_t lore_of_the_grove;
    player_talent_t lycaras_teachings;
    player_talent_t maim;
    player_talent_t matted_fur;
    player_talent_t mass_entanglement;
    player_talent_t mighty_bash;
    player_talent_t natural_recovery;
    player_talent_t natures_vigil;
    player_talent_t nurturing_instinct;
    player_talent_t oakskin;
    player_talent_t primal_fury;
    player_talent_t rake;
    player_talent_t rejuvenation;
    player_talent_t remove_corruption;
    player_talent_t renewal;
    player_talent_t rising_light_falling_night;
    player_talent_t rip;
    player_talent_t skull_bash;
    player_talent_t soothe;
    player_talent_t stampeding_roar;
    player_talent_t starfire;
    player_talent_t starlight_conduit;
    player_talent_t starsurge;
    player_talent_t sunfire;
    player_talent_t thick_hide;
    player_talent_t thrash;
    player_talent_t tiger_dash;
    player_talent_t typhoon;
    player_talent_t ursine_vigor;
    player_talent_t ursocs_spirit;
    player_talent_t ursols_vortex;
    player_talent_t verdant_heart;
    player_talent_t wellhoned_instincts;
    player_talent_t wild_charge;
    player_talent_t wild_growth;

    // Multi-spec
    player_talent_t convoke_the_spirits;
    player_talent_t survival_instincts;

    // Balance
    player_talent_t aetherial_kindling;
    player_talent_t astral_communion;
    player_talent_t astral_smolder;
    player_talent_t astronomical_impact;
    player_talent_t balance_of_all_things;
    player_talent_t celestial_alignment;
    player_talent_t circle_of_life_and_death_owl;
    player_talent_t cosmic_rapidity;
    player_talent_t crashing_star;
    player_talent_t denizen_of_the_dream;
    player_talent_t eclipse;
    player_talent_t elunes_guidance;
    player_talent_t force_of_nature;
    player_talent_t fury_of_elune;
    player_talent_t greater_alignment;  // TODO: remove in 11.0.5
    player_talent_t harmony_of_the_heavens;
    player_talent_t hail_of_stars;
    player_talent_t incarnation_moonkin;
    player_talent_t light_of_the_sun;
    player_talent_t lunar_shrapnel;  // TODO: remove in 11.0.5
    player_talent_t natures_balance;
    player_talent_t natures_grace;
    player_talent_t new_moon;
    player_talent_t orbit_breaker;
    player_talent_t orbital_strike;
    player_talent_t power_of_goldrinn;
    player_talent_t radiant_moonlight;
    player_talent_t rattle_the_stars;
    player_talent_t shooting_stars;
    player_talent_t solar_beam;
    player_talent_t solstice;
    player_talent_t soul_of_the_forest_moonkin;
    player_talent_t starfall;  // TODO: remove in 11.0.5
    player_talent_t starlord;
    player_talent_t starweaver;
    player_talent_t stellar_amplification;
    player_talent_t stellar_flare;
    player_talent_t sundered_firmament;
    player_talent_t sunseeker_mushroom;
    player_talent_t touch_the_cosmos;
    player_talent_t twin_moons;
    player_talent_t umbral_embrace;
    player_talent_t umbral_inspiration;
    player_talent_t umbral_intensity;
    player_talent_t waning_twilight;
    player_talent_t warrior_of_elune;
    player_talent_t whirling_stars;
    player_talent_t wild_mushroom;
    player_talent_t wild_surges;

    // Feral
    player_talent_t adaptive_swarm;
    player_talent_t apex_predators_craving;
    player_talent_t ashamanes_guidance;
    player_talent_t berserk;
    player_talent_t berserk_frenzy;
    player_talent_t berserk_heart_of_the_lion;
    player_talent_t bloodtalons;
    player_talent_t brutal_slash;
    player_talent_t carnivorous_instinct;
    player_talent_t circle_of_life_and_death_cat;
    player_talent_t coiled_to_spring;
    player_talent_t doubleclawed_rake;
    player_talent_t dreadful_bleeding;
    player_talent_t feral_frenzy;
    player_talent_t frantic_momentum;
    player_talent_t incarnation_cat;
    player_talent_t infected_wounds_cat;
    player_talent_t lions_strength;
    player_talent_t lunar_inspiration;
    player_talent_t merciless_claws;
    player_talent_t moment_of_clarity;
    player_talent_t omen_of_clarity_cat;
    player_talent_t pouncing_strikes;
    player_talent_t predator;
    player_talent_t predatory_swiftness;
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
    player_talent_t thrashing_claws;
    player_talent_t tigers_fury;
    player_talent_t tigers_tenacity;
    player_talent_t tireless_energy;
    player_talent_t unbridled_swarm;
    player_talent_t veinripper;
    player_talent_t wild_slashes;

    // Guardian
    player_talent_t after_the_wildfire;
    player_talent_t berserk_persistence;
    player_talent_t berserk_ravage;
    player_talent_t berserk_unchecked_aggression;
    player_talent_t blood_frenzy;
    player_talent_t brambles;
    player_talent_t bristling_fur;
    player_talent_t circle_of_life_and_death_bear;
    player_talent_t dream_of_cenarius_bear;
    player_talent_t earthwarden;
    player_talent_t elunes_favored;
    player_talent_t flashing_claws;
    player_talent_t fury_of_nature;
    player_talent_t galactic_guardian;
    player_talent_t gore;
    player_talent_t gory_fur;
    player_talent_t guardian_of_elune;
    player_talent_t improved_survival_instincts;
    player_talent_t incarnation_bear;
    player_talent_t infected_wounds_bear;
    player_talent_t innate_resolve;
    player_talent_t layered_mane;
    player_talent_t lunar_beam;
    player_talent_t mangle;
    player_talent_t maul;
    player_talent_t moonless_night;
    player_talent_t pulverize;
    player_talent_t rage_of_the_sleeper;
    player_talent_t raze;
    player_talent_t reinforced_fur;
    player_talent_t reinvigoration;
    player_talent_t rend_and_tear;
    player_talent_t scintillating_moonlight;
    player_talent_t soul_of_the_forest_bear;
    player_talent_t survival_of_the_fittest;
    player_talent_t thorns_of_iron;
    player_talent_t tooth_and_claw;
    player_talent_t twin_moonfire;
    player_talent_t ursocs_endurance;
    player_talent_t ursocs_fury;
    player_talent_t vicious_cycle;
    player_talent_t vulnerable_flesh;
    player_talent_t untamed_savagery;
    player_talent_t ursocs_guidance;

    // Restoration
    player_talent_t abundance;
    player_talent_t budding_leaves;
    player_talent_t call_of_the_elder_druid;
    player_talent_t cenarion_ward;
    player_talent_t cenarius_guidance;
    player_talent_t cultivation;
    player_talent_t dream_of_cenarius_tree;
    player_talent_t dreamstate;
    player_talent_t efflorescence;
    player_talent_t embrace_of_the_dream;
    player_talent_t flash_of_clarity;
    player_talent_t flourish;
    player_talent_t germination;
    player_talent_t grove_guardians;
    player_talent_t grove_tending;
    player_talent_t harmonious_blooming;
    player_talent_t improved_ironbark;
    player_talent_t improved_regrowth;
    player_talent_t improved_wild_growth;
    player_talent_t incarnation_tree;
    player_talent_t inner_peace;
    player_talent_t invigorate;
    player_talent_t ironbark;
    player_talent_t lifebloom;
    player_talent_t liveliness;
    player_talent_t master_shapeshifter;
    player_talent_t natures_splendor;
    player_talent_t natures_swiftness;
    player_talent_t nourish;
    player_talent_t nurturing_dormancy;
    player_talent_t omen_of_clarity_tree;
    player_talent_t overgrowth;
    player_talent_t passing_seasons;
    player_talent_t photosynthesis;
    player_talent_t power_of_the_archdruid;
    player_talent_t prosperity;
    player_talent_t rampant_growth;
    player_talent_t reforestation;
    player_talent_t regenerative_heartwood;
    player_talent_t regenesis;
    player_talent_t soul_of_the_forest_tree;
    player_talent_t spring_blossoms;
    player_talent_t stonebark;
    player_talent_t thriving_vegetation;
    player_talent_t tranquil_mind;
    player_talent_t tranquility;
    player_talent_t undergrowth;
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
    player_talent_t fount_of_strength;
    player_talent_t killing_strikes;
    player_talent_t packs_endurance;
    player_talent_t ravage;
    player_talent_t ruthless_aggression;
    player_talent_t strike_for_the_heart;
    player_talent_t tear_down_the_mighty;
    player_talent_t wildpower_surge;
    player_talent_t wildshape_mastery;

    // Wildstalker
    player_talent_t bond_with_nature;
    player_talent_t bursting_growth;
    player_talent_t entangling_vortex;
    player_talent_t flower_walk;
    player_talent_t harmonious_constitution;
    player_talent_t hunt_beneath_the_open_skies;
    player_talent_t implant;
    player_talent_t lethal_preservation;
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
    player_talent_t early_spring;
    player_talent_t expansiveness;
    player_talent_t groves_inspiration;
    player_talent_t harmony_of_the_grove;
    player_talent_t potent_enchantments;
    player_talent_t power_of_nature;
    player_talent_t power_of_the_dream;
    player_talent_t protective_growth;
    player_talent_t treants_of_the_moon;

    // Elune's Chosen
    player_talent_t arcane_affinity;
    player_talent_t astral_insight;
    player_talent_t atmospheric_exposure;
    player_talent_t boundless_moonlight;
    player_talent_t elunes_grace;
    player_talent_t glistening_fur;
    player_talent_t lunar_amplification;
    player_talent_t lunar_calling;
    player_talent_t lunar_insight;
    player_talent_t lunation;
    player_talent_t moondust;
    player_talent_t moon_guardian;
    player_talent_t stellar_command;
    player_talent_t the_eternal_moon;
    player_talent_t the_light_of_elune;
  } talent;

  // Class Specializations
  struct specializations_t
  {
    // Passive Auras
    const spell_data_t* druid;

    // Baseline
    const spell_data_t* bear_form_override;  // swipe/thrash
    const spell_data_t* bear_form_passive;
    const spell_data_t* bear_form_passive_2;
    const spell_data_t* cat_form_override;  // swipe/thrash
    const spell_data_t* cat_form_passive;
    const spell_data_t* cat_form_passive_2;
    const spell_data_t* cat_form_speed;
    const spell_data_t* feral_affinity;
    const spell_data_t* improved_prowl;  // stealth rake
    const spell_data_t* moonfire_2;
    const spell_data_t* moonfire_dmg;
    const spell_data_t* swipe;
    const spell_data_t* wrath;

    // Class
    const spell_data_t* moonfire;
    const spell_data_t* sunfire_dmg;
    const spell_data_t* thrash_bear_bleed;
    const spell_data_t* thrash_cat_bleed;

    // Balance
    const spell_data_t* astral_power;
    const spell_data_t* celestial_alignment;
    const spell_data_t* crashing_star_dmg;
    const spell_data_t* eclipse_lunar;
    const spell_data_t* eclipse_solar;
    const spell_data_t* full_moon;
    const spell_data_t* half_moon;
    const spell_data_t* incarnation_moonkin;
    const spell_data_t* moonkin_form;
    const spell_data_t* shooting_stars_dmg;
    const spell_data_t* starfall;
    const spell_data_t* stellar_amplification;
    const spell_data_t* waning_twilight;

    // Feral
    const spell_data_t* adaptive_swarm_damage;
    const spell_data_t* adaptive_swarm_heal;
    const spell_data_t* feral_overrides;
    const spell_data_t* ashamanes_guidance;
    const spell_data_t* ashamanes_guidance_buff;  // buff spell for ashamanes guidance 421442
    const spell_data_t* berserk_cat;  // berserk cast/buff spell
    const spell_data_t* sabertooth;  // sabertooth debuff

    // Guardian
    const spell_data_t* bear_form_2;
    const spell_data_t* berserk_bear;  // berserk cast/buff spell
    const spell_data_t* elunes_favored;
    const spell_data_t* fury_of_nature;
    const spell_data_t* incarnation_bear;
    const spell_data_t* lightning_reflexes;
    const spell_data_t* tooth_and_claw_debuff;
    const spell_data_t* ursine_adept;

    // Resto
    const spell_data_t* cenarius_guidance;

    // Hero Talent
    const spell_data_t* atmospheric_exposure;  // atmospheric exposure debuff
    const spell_data_t* bloodseeker_vines;
    const spell_data_t* dreadful_wound;
  } spec;

  struct uptimes_t
  {
    uptime_t* tooth_and_claw_debuff;
  } uptime;

  auto_dispose<std::vector<modified_spell_data_t*>> modified_spells;

  druid_t( sim_t* sim, std::string_view name, race_e r = RACE_NIGHT_ELF )
    : parse_player_effects_t( sim, DRUID, name, r ),
      eclipse_handler( this ),
      options(),
      active(),
      pets( this ),
      caster_form_weapon(),
      buff(),
      cooldown(),
      gain(),
      mastery(),
      proc(),
      talent(),
      spec(),
      uptime()
  {
    cooldown.berserk_bear          = get_cooldown( "berserk_bear" );
    cooldown.frenzied_regeneration = get_cooldown( "frenzied_regeneration" );
    cooldown.fury_of_elune         = get_cooldown( "fury_of_elune" );
    cooldown.growl                 = get_cooldown( "growl" );
    cooldown.incarnation_bear      = get_cooldown( "incarnation_guardian_of_ursoc" );
    cooldown.lunar_beam            = get_cooldown( "lunar_beam" );
    cooldown.mangle                = get_cooldown( "mangle" );
    cooldown.moon_cd               = get_cooldown( "moon_cd" );
    cooldown.natures_swiftness     = get_cooldown( "natures_swiftness" );
    cooldown.thrash_bear           = get_cooldown( "thrash_bear" );

    resource_regeneration = regen_type::DYNAMIC;

    regen_caches[ CACHE_HASTE ]        = true;
    regen_caches[ CACHE_ATTACK_HASTE ] = true;
  }

  // Character Definition
  void activate() override;
  void init() override;
  bool validate_fight_style( fight_style_e ) const override;
  void init_absorb_priority() override;
  void init_action_list() override;
  void init_base_stats() override;
  void init_stats() override;
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
  void apply_affecting_auras( buff_t& );
  void create_actions() override;
  void apply_affecting_auras( action_t& ) override;
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
  void merge( player_t& other ) override;
  void datacollection_begin() override;
  void datacollection_end() override;
  void analyze( sim_t& ) override;
  timespan_t available() const override;
  double composite_armor() const override;
  double composite_block() const override { return 0; }
  double composite_dodge_rating() const override;
  double composite_parry() const override { return 0; }
  double non_stacking_movement_modifier() const override;
  double stacking_movement_modifier() const override;
  std::unique_ptr<expr_t> create_action_expression(action_t& a, std::string_view name_str) override;
  std::unique_ptr<expr_t> create_expression( std::string_view name ) override;
  action_t* create_action( std::string_view name, std::string_view options ) override;
  void create_pets() override;
  resource_e primary_resource() const override;
  role_e primary_role() const override;
  stat_e convert_hybrid_stat( stat_e s ) const override;
  double resource_regen_per_second( resource_e ) const override;
  double resource_gain( resource_e, double, gain_t*, action_t* a = nullptr ) override;
  void target_mitigation( school_e, result_amount_type, action_state_t* ) override;
  void assess_damage( school_e, result_amount_type, action_state_t* ) override;
  void assess_damage_imminent_pre_absorb( school_e, result_amount_type, action_state_t* ) override;
  void recalculate_resource_max( resource_e, gain_t* source = nullptr ) override;
  void create_options() override;
  std::string create_profile( save_e type ) override;
  const druid_td_t* find_target_data( const player_t* target ) const override;
  druid_td_t* get_target_data( player_t* target ) const override;
  void copy_from( player_t* ) override;
  void moving() override;

  // utility functions
  form_e get_form() const { return form; }
  void shapeshift( form_e );
  void init_beast_weapon( weapon_t&, double );
  void adjust_health_pct( double, bool );
  const spell_data_t* apply_override( const spell_data_t* base, const spell_data_t* passive ) const;
  bool uses_form( specialization_e spec, std::string_view name, action_t* action ) const;
  bool uses_cat_form() const;
  bool uses_bear_form() const;
  bool uses_moonkin_form() const;
  player_t* get_smart_target( const std::vector<player_t*>& tl, dot_t* druid_td_t::dots_t::*dot,
                              player_t* exclude = nullptr, double range = 0.0, bool really_smart = false );

  modified_spell_data_t* get_modified_spell( const spell_data_t* );

  // secondary actions
  std::vector<action_t*> secondary_action_list;

  template <typename T, typename... Ts>
  T* get_secondary_action( std::string_view n, Ts&&... args );

private:
  void apl_precombat();
  void apl_default();
  void apl_feral();
  void apl_feral_ptr();
  void apl_balance();
  void apl_balance_ptr();
  void apl_guardian();
  void apl_restoration();

  target_specific_t<druid_td_t> target_data;
};

namespace pets
{
// ==========================================================================
// Pets and Guardians
// ==========================================================================

// Denizen of the Dream =============================================
struct denizen_of_the_dream_t final : public pet_t
{
  struct fey_missile_t final : public parse_action_effects_t<spell_t>
  {
    druid_t* o;
    double mastery_passive;
    double mastery_dot;

    fey_missile_t( pet_t* p )
      : parse_action_effects_t( "fey_missile", p, p->find_spell( 188046 ) ),
        o( static_cast<druid_t*>( p->owner ) ),
        mastery_passive( o->mastery.astral_invocation->effectN( 1 ).mastery_value() ),
        mastery_dot( o->mastery.astral_invocation->effectN( 5 ).mastery_value() )
    {
      name_str_reporting = "fey_missile";

      _player = p->owner;  // use owner's target data & mastery

      force_effect( o->buff.eclipse_lunar, 1, USE_CURRENT );
      force_effect( o->buff.eclipse_solar, 1, USE_CURRENT );

      force_effect( o->mastery.astral_invocation, 1 );
      force_effect( o->mastery.astral_invocation, 3 );

      force_target_effect( d_fn( &druid_td_t::dots_t::moonfire ), o->spec.moonfire_dmg, 4,
                           o->mastery.astral_invocation );
      force_target_effect( d_fn( &druid_td_t::dots_t::sunfire ), o->spec.sunfire_dmg, 4,
                           o->mastery.astral_invocation );

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

// Treant Base ======================================================
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

  // NOTE: defined after namespace buffs
  void arise() override;
  void demise() override;

  druid_t* o() { return static_cast<druid_t*>( owner ); }
};

// Force of Nature ==================================================
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
    // TODO: confirm this
    owner_coeff.ap_from_sp = 0.935;

    double base_dps = o()->dbc->expected_stat( o()->true_level ).creature_auto_attack_dps;

    main_hand_weapon.min_dmg = main_hand_weapon.max_dmg = base_dps * main_hand_weapon.swing_time.total_seconds() / 1000;

    resource_regeneration = regen_type::DISABLED;
    main_hand_weapon.type = WEAPON_BEAST;

    action_list_str = "auto_attack";

    if ( o()->talent.power_of_nature.ok() )
      power_of_nature_mul = o()->find_spell( 449001 )->effectN( 1 ).percent();
  }

  void init_base_stats() override
  {
    pet_t::init_base_stats();

    // TODO: confirm these values
    resources.base[ RESOURCE_HEALTH ] = owner->resources.max[ RESOURCE_HEALTH ] * 0.4;
    resources.base[ RESOURCE_MANA ]   = 0;

    initial.stats.attribute[ ATTR_INTELLECT ] = 0;
    initial.spell_power_per_intellect         = 0;
    intellect_per_owner                       = 0;
    stamina_per_owner                         = 0;
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

  bool can_trigger( action_t* a ) const override
  {
    assert( dynamic_cast<druid_action_data_t*>( a ) && "Non Druid action passed to Druid buff can_trigger." );

    if ( Base::is_fallback || !a->data().ok() || !Base::get_trigger_data()->ok() )
      return false;

    if ( Base::get_trigger_data()->flags( spell_attribute::SX_ONLY_PROC_FROM_CLASS_ABILITIES ) &&
         !a->allow_class_ability_procs )
    {
      return false;
    }

    if ( a->proc )
    {
      // allow if the driver can proc from procs
      if ( Base::get_trigger_data()->flags( spell_attribute::SX_CAN_PROC_FROM_PROCS ) )
      {
        return true;
      }

      // allow if the action is from convoke and the driver procs from cast successful
      if ( ( Base::get_trigger_data()->proc_flags() & PF_CAST_SUCCESSFUL ) &&
           dynamic_cast<druid_action_data_t*>( a )->has_flag( flag_e::CONVOKE ) )
      {
        return true;
      }

      // by default procs cannot trigger
      return false;
    }

    return true;
  }

  bool can_expire( action_t* a ) const override
  {
    assert( dynamic_cast<druid_action_data_t*>( a ) && "Non Druid action passed to Druid buff can_expire." );

    if ( Base::is_fallback || !a->data().ok() || !Base::data().ok() )
      return false;

    if ( Base::data().flags( spell_attribute::SX_ONLY_PROC_FROM_CLASS_ABILITIES ) && !a->allow_class_ability_procs )
      return false;

    if ( a->proc )
    {
      // allow if either the buff or the driver can proc from procs
      if ( Base::data().flags( spell_attribute::SX_CAN_PROC_FROM_PROCS ) ||
           Base::get_trigger_data()->flags( spell_attribute::SX_CAN_PROC_FROM_PROCS ) )
      {
        return true;
      }

      // allow if the action is from convoke and the buff procs from cast successful
      if ( ( Base::data().proc_flags() & PF_CAST_SUCCESSFUL ) &&
           dynamic_cast<druid_action_data_t*>( a )->has_flag( flag_e::CONVOKE ) )
      {
        return true;
      }

      // by default procs cannot expire
      return false;
    }

    return true;
  }
};

template <class Base>
struct druid_action_t : public parse_action_effects_t<Base>, public druid_action_data_t
{
private:
  using ab = parse_action_effects_t<Base>;

public:
  using base_t = druid_action_t<Base>;

  std::vector<player_effect_t> persistent_multiplier_effects;

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

      if ( ab::data().flags( spell_attribute::SX_ABILITY ) || ab::trigger_gcd > 0_ms )
        ab::not_a_proc = true;

      if ( p->spec.ursine_adept->ok() &&
           ab::data().affected_by( find_effect( p->buff.bear_form, A_MOD_IGNORE_SHAPESHIFT ) ) )
      {
        form_mask |= form_e::BEAR_FORM;
      }
    }
  }

  druid_t* p() { return static_cast<druid_t*>( ab::player ); }

  const druid_t* p() const { return static_cast<druid_t*>( ab::player ); }

  druid_td_t* td( player_t* t ) const { return p()->get_target_data( t ); }

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

  void replace_stats( action_t* a, bool add = true )
  {
    if ( add )
    {
      range::erase_remove( ab::stats->action_list, a );
      ab::stats->action_list.push_back( a );
    }

    if ( a->stats == ab::stats )
      return;

    range::erase_remove( ab::player->stats_list, a->stats );
    delete a->stats;
    a->stats = ab::stats;
  }

  bool ready() override
  {
    if ( !check_form_restriction() && !autoshift && ( has_flag( flag_e::NOUNSHIFT ) || !( form_mask & NO_FORM ) ) )
    {
      if ( ab::sim->debug )
      {
        ab::sim->print_debug( "{} ready() failed due to wrong form. form={:#010x} form_mask={:#010x}", ab::name(),
                              static_cast<unsigned int>( p()->get_form() ), form_mask );
      }

      return false;
    }

    return ab::ready();
  }

  void snapshot_and_execute( const action_state_t* s, bool is_dot,
                             std::function<void( const action_state_t*, action_state_t* )> pre = nullptr,
                             std::function<void( const action_state_t*, action_state_t* )> post = nullptr )
  {
    auto state = this->get_state();
    this->target = state->target = s->target;

    if ( pre )
      pre( s, state );

    this->snapshot_state( state, this->amount_type( state, is_dot ) );

    if ( post )
      post( s, state );

    this->schedule_execute( state );
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

    if ( ab::harmful )
    {
      // TODO: confirm what can and cannot trigger or consume lunar amplificiation
      if ( p()->talent.lunar_amplification.ok() && !ab::background )
      {
        if ( dbc::has_common_school( _get_school(), SCHOOL_ARCANE ) )
        {
          // schedule expiration to wait for complete action resolution
          if ( p()->buff.lunar_amplification->check() && p()->buff.lunar_amplification->can_expire( this ) )
            make_event( *ab::sim, [ this ] { p()->buff.lunar_amplification->expire(); } );
        }
        else
        {
          p()->buff.lunar_amplification->trigger( this );
        }
      }

      // TODO: confirm what can and cannot trigger lunation
      if ( p()->talent.lunation.ok() && has_flag( flag_e::FOREGROUND ) &&
           dbc::has_common_school( _get_school(), SCHOOL_ARCANE ) )
      {
        assert( p()->talent.lunation->effects().size() == 3 );
        auto eff = p()->talent.lunation->effects().begin();

        for ( auto cd : { p()->cooldown.fury_of_elune, p()->cooldown.moon_cd, p()->cooldown.lunar_beam } )
          cd->adjust( ( *eff++ ).time_value() );
      }
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
           util::translate_power_type( static_cast<power_e>( eff.misc_value1() ) ) == ab::energize_resource )
      {
        energize = &e;
        ab::energize_amount = energize->resource();
        ab::sim->print_debug( "{} energize {} {} from {}#{}", *this,
                              util::resource_type_string( ab::energize_resource ), ab::energize_amount,
                              m_data->_spell.id(), eff.index() + 1 );

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
    if ( !form_mask || ( form_mask & p()->get_form() ) == p()->get_form() )
      return true;

    return false;
  }

  void check_autoshift()
  {
    if ( !check_form_restriction() )
    {
      if ( autoshift )
        autoshift->execute();
      else if ( !has_flag( flag_e::NOUNSHIFT ) && form_mask & NO_FORM )
        p()->active.shift_to_caster->execute();
      else
        assert( false && "Action executed in wrong form with no valid form to shift to!" );
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

  void init_finished() override
  {
    ab::init_finished();
    ab::initialize_buff_list_on_vector( persistent_multiplier_effects );
  }

  size_t total_effects_count() override
  {
    return ab::total_effects_count() + persistent_multiplier_effects.size();
  }

  void print_parsed_custom_type( report::sc_html_stream& os ) override
  {
    ab::template print_parsed_type<base_t>( os, &base_t::persistent_multiplier_effects, "Snapshots" );
  }
};

template <typename BASE>
struct use_dot_list_t : public BASE
{
  using base_t = use_dot_list_t<BASE>;

  std::vector<dot_t*>* dot_list = nullptr;

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

template <specialization_e S, typename BASE>
struct use_fluid_form_t : public BASE
{
  using base_t = use_fluid_form_t<S, BASE>;

  use_fluid_form_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE )
    : BASE( n, p, s, f )
  {
    if ( p->talent.fluid_form.ok() && !BASE::has_flag( flag_e::CONVOKE ) )
    {
      if constexpr ( S == DRUID_BALANCE )
      {
        if ( p->specialization() == DRUID_BALANCE )
          BASE::autoshift = p->active.shift_to_moonkin;
      }
      else if constexpr ( S == DRUID_FERAL )
      {
        BASE::autoshift = p->active.shift_to_cat;
      }
      else if constexpr ( S == DRUID_GUARDIAN )
      {
        BASE::autoshift = p->active.shift_to_bear;
      }
    }
  }
};

template <specialization_e S, typename BASE>
struct trigger_aggravate_wounds_t : public BASE
{
private:
  timespan_t dot_ext = 0_ms;
  timespan_t max_ext = 0_ms;

public:
  using base_t = trigger_aggravate_wounds_t<S, BASE>;

  trigger_aggravate_wounds_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE )
    : BASE( n, p, s, f )
  {
    if ( p->specialization() == S && p->talent.aggravate_wounds.ok() )
    {
      if constexpr ( S == DRUID_FERAL )
        dot_ext = p->talent.aggravate_wounds->effectN( 2 ).time_value();
      else if constexpr ( S == DRUID_GUARDIAN )
        dot_ext = p->talent.aggravate_wounds->effectN( 1 ).time_value();

      max_ext = timespan_t::from_seconds( p->talent.aggravate_wounds->effectN( 3 ).base_value() );
    }
  }

  void execute() override
  {
    BASE::execute();

    if ( dot_ext > 0_ms )
    {
      range::for_each( BASE::p()->dot_lists.dreadful_wound, [ this ]( dot_t* d ) {
        d->adjust_duration( dot_ext, max_ext );
      } );
    }
  }
};

template <typename BASE>
struct trigger_atmospheric_exposure_t : public BASE
{
  using base_t = trigger_atmospheric_exposure_t<BASE>;

  trigger_atmospheric_exposure_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE )
    : BASE( n, p, s, f )
  {}

  void impact( action_state_t* s ) override
  {
    BASE::impact( s );

    if ( action_t::result_is_hit( s->result ) )
      BASE::td( s->target )->debuff.atmospheric_exposure->trigger( this );
  }
};

template <typename BASE>
struct trigger_call_of_the_elder_druid_t : public BASE
{
private:
  timespan_t dur;

public:
  using base_t = trigger_call_of_the_elder_druid_t<BASE>;

  trigger_call_of_the_elder_druid_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE )
    : BASE( n, p, s, f ),
      dur( timespan_t::from_seconds( p->talent.call_of_the_elder_druid->effectN( 1 ).base_value() ) )
  {}

  void execute() override
  {
    BASE::execute();

    if ( dur > 0_ms && BASE::p()->in_combat && !BASE::p()->buff.call_of_the_elder_druid->check() )
    {
      BASE::p()->buff.call_of_the_elder_druid->trigger();
      BASE::p()->buff.heart_of_the_wild->extend_duration_or_trigger( dur );
    }
  }
};

template <specialization_e S, typename BASE>
struct trigger_claw_rampage_t : public BASE
{
private:
  cooldown_t* icd = nullptr;
  buff_t* berserk_buff = nullptr;
  buff_t* ravage_buff = nullptr;
  double proc_pct = 0.0;

public:
  using base_t = trigger_claw_rampage_t<S, BASE>;

  trigger_claw_rampage_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE )
    : BASE( n, p, s, f )
  {
    if ( p->specialization() == S && p->talent.claw_rampage.ok() )
    {
      proc_pct = p->talent.claw_rampage->proc_chance();
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

public:
  using base_t = trigger_control_of_the_dream_t<BASE>;

  trigger_control_of_the_dream_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE )
    : BASE( n, p, s, f )
  {
    if ( p->talent.control_of_the_dream.ok() )
      max_diff = timespan_t::from_seconds( p->talent.control_of_the_dream->effectN( 1 ).base_value() );
  }

  void update_ready( timespan_t cd ) override
  {
    auto diff = 0_ms;

    // cooldown starts at max diff
    if ( max_diff > 0_ms && cd == timespan_t::min() && BASE::cooldown_duration() > 0_ms &&
         ( BASE::cooldown->charges == BASE::cooldown->current_charge ) )
    {
      diff = std::min( max_diff, BASE::sim->current_time() - BASE::cooldown->ready );
    }

    BASE::update_ready( cd );

    if ( diff > 0_ms )
      BASE::cooldown->adjust( -diff, false );
  }
};

template <typename BASE>
struct trigger_gore_t : public BASE
{
private:
  proc_t* gore_proc = nullptr;
  cooldown_t* icd = nullptr;

public:
  using base_t = trigger_gore_t<BASE>;

  trigger_gore_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE ) : BASE( n, p, s, f )
  {
    if ( p->talent.gore.ok() )
    {
      gore_proc = p->get_proc( "Gore" )->collect_interval();
      icd = p->get_cooldown( "gore_icd" );
      icd->duration = p->talent.gore->internal_cooldown();
    }
  }

  void impact( action_state_t* s ) override
  {
    BASE::impact( s );

    if ( gore_proc && icd->up() && BASE::p()->buff.gore->trigger( this ) )
    {
      BASE::p()->cooldown.mangle->reset( true );
      gore_proc->occur();
      icd->start();
    }
  }
};

template <typename BASE>
struct trigger_guardians_tenacity_t : public BASE
{
  using base_t = trigger_guardians_tenacity_t<BASE>;

  trigger_guardians_tenacity_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE )
    : BASE( n, p, s, f )  {}

  void execute() override
  {
    BASE::execute();

    BASE::p()->buff.guardians_tenacity->trigger( this );
  }
};

template <typename BASE, typename DOT_BASE>
struct ravage_base_t : public BASE
{
  struct dreadful_wound_t final : public DOT_BASE
  {
    dreadful_wound_t( druid_t* p, std::string_view n, flag_e f ) : DOT_BASE( n, p, p->spec.dreadful_wound, f )
    {
      DOT_BASE::background = true;

      DOT_BASE::name_str_reporting = "dreadful_wound";
      DOT_BASE::dot_name = "dreadful_wound";

      DOT_BASE::dot_list = &p->dot_lists.dreadful_wound;
    }

    // TODO: use custom action state if non-TF persistent multipliers are applied
    void snapshot_state( action_state_t* s, unsigned fl, result_amount_type rt ) override
    {
      auto prev_mul = 1.0;
      auto prev_dot = DOT_BASE::get_dot( s->target );

      if ( prev_dot->is_ticking() )
        prev_mul = prev_dot->state->persistent_multiplier;

      DOT_BASE::snapshot_state( s, fl, rt );

      if ( s->persistent_multiplier < prev_mul )
        s->persistent_multiplier = prev_mul;
    }
  };

  using base_t = ravage_base_t<BASE, DOT_BASE>;

  double aoe_coeff;

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

  void execute() override
  {
    BASE::execute();

    BASE::p()->buff.killing_strikes->trigger();
    BASE::p()->buff.ruthless_aggression->trigger();
  }
};

// Proc chance after X failures:
//   0.6 - 1.13 ^ ( -A * ( X - B ) )
// where:
//   A = percent value of corresponding effect index on the talent spell data
//   B = 3 - base_tick_time in seconds
//
// via community testing (~257k ticks)
// https://docs.google.com/spreadsheets/d/1lPDhmfqe03G_eFetGJEbSLbXMcfkzjhzyTaQ8mdxADM/edit?gid=385734241
//
// TODO: confirm any AOE diminishing returns
// TODO: add manual adjustment for early checks with <0 value:
//   Rake: 3 ticks = 1.25%, 4 ticks = 7%
//   Rip: 5 ticks = 1.75%
template <size_t IDX, typename BASE>
struct trigger_thriving_growth_t : public BASE
{
private:
  target_specific_t<accumulated_rng_t> vine_rng;
  double scale = 0.0;
  double shift = 0.0;

public:
  using base_t = trigger_thriving_growth_t<IDX, BASE>;

  trigger_thriving_growth_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE )
    : BASE( n, p, s, f ), vine_rng( false )
  {
    if ( p->talent.thriving_growth.ok() )
    {
      scale = p->talent.thriving_growth->effectN( IDX ).percent();
      shift = 3 - BASE::base_tick_time.total_seconds();
    }
  }

  double rng_fn( unsigned c ) const
  {
    return std::max( 0.0, 0.6 - std::pow( 1.13, -scale * ( c - shift ) ) );
  }

  void tick( dot_t* d ) override
  {
    BASE::tick( d );

    if ( !scale )
      return;

    auto& _rng = vine_rng[ d->target ];
    if ( !_rng )
    {
      _rng = BASE::p()->get_accumulated_rng( fmt::format( "{}_vines_{}", BASE::name(), d->target->actor_index ), scale,
                                             [ this ]( double, unsigned c ) { return rng_fn( c ); } );

      d->target->register_on_demise_callback( d->target, [ _rng ]( player_t* ) { _rng->reset(); } );
    }

    if ( _rng->trigger() )
    {
      BASE::p()->active.bloodseeker_vines->execute_on_target( d->target );

      for ( auto e : vine_rng.get_entries() )
        if ( e )
          e->reset();
    }
  }
};

template <typename BASE>
struct trigger_waning_twilight_t : public BASE
{
private:
  int num_dots;

public:
  using base_t = trigger_waning_twilight_t<BASE>;

  trigger_waning_twilight_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE )
    : BASE( n, p, s, f ), num_dots( as<int>( p->talent.waning_twilight->effectN( 3 ).base_value() ) )
  {}

  void update_waning_twilight( player_t* t )
  {
    if ( !BASE::p()->talent.waning_twilight.ok() )
      return;

    auto td_ = BASE::td( t );
    int count = td_->dots.astral_smolder->is_ticking() +
                td_->dots.feral_frenzy->is_ticking() +
                td_->dots.fungal_growth->is_ticking() +
                td_->dots.moonfire->is_ticking() +
                td_->dots.rake->is_ticking() +
                td_->dots.rip->is_ticking() +
                td_->dots.stellar_flare->is_ticking() +
                td_->dots.sunfire->is_ticking() +
                td_->dots.thrash_bear->is_ticking() +
                td_->dots.thrash_cat->is_ticking();

    if ( count < num_dots )
      td_->debuff.waning_twilight->expire();
    else if ( !td_->debuff.waning_twilight->check() )
      td_->debuff.waning_twilight->trigger();
  }

  void trigger_dot( action_state_t* s ) override
  {
    BASE::trigger_dot( s );

    update_waning_twilight( s->target );
  }

  void last_tick( dot_t* d ) override
  {
    BASE::last_tick( d );

    update_waning_twilight( d->target );
  }
};

template <specialization_e S, typename BASE>
struct trigger_wildpower_surge_t : public BASE
{
private:
  buff_t* buff = nullptr;

public:
  using base_t = trigger_wildpower_surge_t<S, BASE>;

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

public:
  using base_t = druid_attack_t<Base>;

  double bleed_mul = 0.0;
  bool direct_bleed = false;

  druid_attack_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f ) : ab( n, p, s, f )
  {
    ab::special = true;

    for ( const auto& e : ab::data().effects() )
    {
      if ( ( e.type() == E_SCHOOL_DAMAGE || e.type() == E_WEAPON_PERCENT_DAMAGE ) &&
           ( e.mechanic() == MECHANIC_BLEED || ab::data().mechanic() == MECHANIC_BLEED ) )
      {
        direct_bleed = true;
        break;
      }
    }
  }

  double composite_target_armor( player_t* t ) const override
  {
    return direct_bleed ? 0.0 : ab::composite_target_armor( t );
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double tm = ab::composite_target_multiplier( t );

    if ( bleed_mul && t->debuffs.bleeding && t->debuffs.bleeding->up() )
      tm *= 1.0 + bleed_mul;

    return tm;
  }
};

// Druid "Spell" Base for druid_spell_t, druid_heal_t ( and potentially druid_absorb_t )
template <class Base>
struct druid_spell_base_t : public druid_action_t<Base>
{
private:
  using ab = druid_action_t<Base>;

public:
  using base_t = druid_spell_base_t<Base>;

  bool reset_melee_swing = true;  // TRUE(default) to reset swing timer on execute (as most cast time spells do)

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
  struct
  {
    bool flourish;  // true if tick_rate is affected by flourish, NOTE: extension is handled in flourish_t
    bool soul_of_the_forest;
  } affected_by;

  double forestwalk_mul;  // % healing from forestwalk
  double imp_fr_mul;      // % healing from improved frenzied regeneration
  double photo_mul;       // tick rate multiplier when lb is on self
  double photo_pct;       // % chance to bloom when lb is on other
  double iron_mul = 0.0;  // % healing from hots with ironbark
  bool target_self = false;

  druid_heal_t( std::string_view n, druid_t* p, const spell_data_t* s = spell_data_t::nil(), flag_e f = flag_e::NONE )
    : base_t( n, p, s, f ),
      affected_by(),
      forestwalk_mul( find_effect( p->buff.forestwalk, A_MOD_HEALING_PCT ).percent() ),
      imp_fr_mul( find_effect( p->talent.verdant_heart, A_ADD_FLAT_MODIFIER, P_EFFECT_2 ).percent() ),
      photo_mul( p->talent.photosynthesis->effectN( 1 ).percent() ),
      photo_pct( p->talent.photosynthesis->effectN( 2 ).percent() )
  {
    add_option( opt_bool( "target_self", target_self ) );

    if ( target_self )
      target = p;

    may_miss = harmful = false;
    ignore_false_positive = true;

    if ( p->talent.stonebark.ok() && find_effect( p->talent.ironbark, this, A_MOD_HEALING_RECEIVED ).ok() )
      iron_mul = find_effect( p->talent.stonebark, A_ADD_FLAT_MODIFIER, P_EFFECT_2 ).percent();

    if ( p->talent.flourish.ok() )
      affected_by.flourish = find_effect( p->talent.flourish, this, A_ADD_PCT_MODIFIER, P_TICK_TIME ).ok();
  }

  virtual double harmony_multiplier( player_t* t ) const
  {
    return p()->cache.mastery_value() * td( t )->hots_ticking();
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double ctm = base_t::composite_target_multiplier( t );

    if ( p()->mastery.harmony->ok() )
      ctm *= 1.0 + harmony_multiplier( t );

    if ( iron_mul && td( t )->buff.ironbark->up() )
      ctm *= 1.0 + iron_mul;

    if ( t == player )
    {
      if ( p()->buff.forestwalk->check() )
        ctm *= 1.0 + forestwalk_mul;

      if ( p()->buff.barkskin->check() || td( t )->hots.frenzied_regeneration->is_ticking() )
        ctm *= 1.0 + imp_fr_mul;

      ctm *= 1.0 + p()->talent.natural_recovery->effectN( 1 ).percent();

      ctm *= 1.0 + p()->talent.bond_with_nature->effectN( 1 ).percent();
    }

    return ctm;
  }

  double tick_time_pct_multiplier( const action_state_t* s ) const override
  {
    auto mul = base_t::tick_time_pct_multiplier( s );

    // flourish effect is a negative percent modifier, so multiply here
    if ( affected_by.flourish && p()->buff.flourish->check() )
      mul *= 1.0 + p()->buff.flourish->default_value;

    // photo effect is a positive dummy value, so divide here
    if ( photo_mul && td( player )->hots.lifebloom->is_ticking() )
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

  void execute() override
  {
    base_t::execute();

    if ( affected_by.soul_of_the_forest && p()->buff.soul_of_the_forest_tree->up() )
      p()->buff.soul_of_the_forest_tree->expire( this );
  }
};

struct cat_attack_t : public druid_attack_t<melee_attack_t>
{
  struct
  {
    bool tigers_fury;
    bool bloodtalons;
    bool clearcasting;
  } snapshots;

  std::vector<player_effect_t> persistent_periodic_effects;
  std::vector<player_effect_t> persistent_direct_effects;
  snapshot_counter_t* bt_counter = nullptr;
  snapshot_counter_t* tf_counter = nullptr;

  cat_attack_t( std::string_view n, druid_t* p, const spell_data_t* s = spell_data_t::nil(), flag_e f = flag_e::NONE )
    : base_t( n, p, s, f ), snapshots()
  {
    if ( p->specialization() == DRUID_BALANCE || p->specialization() == DRUID_RESTORATION )
      ap_type = attack_power_type::NO_WEAPON;

    if ( data().ok() )
    {
      // effect data missing stack suppress flag for effect #2, manually override
      snapshots.bloodtalons =  parse_persistent_effects( p->buff.bloodtalons, IGNORE_STACKS );
      snapshots.tigers_fury =  parse_persistent_effects( p->buff.tigers_fury,
                                                         p->talent.carnivorous_instinct,
                                                         p->talent.tigers_tenacity );
      // NOTE: thrash dot snapshot data is missing, it must be manually added in cat_thrash_t
      snapshots.clearcasting = parse_persistent_effects( p->buff.clearcasting_cat,
                                                         p->talent.moment_of_clarity );
    }
  }

  bool stealthed() const  // For effects that require any form of stealth.
  {
    return p()->buff.prowl->up() || p()->buffs.shadowmeld->up();
  }

  bool stealthed_any() const
  {
    return stealthed() || p()->buff.sudden_ambush->up();
  }

  void consume_resource() override
  {
    double eff_cost = base_cost();

    base_t::consume_resource();

    // Treat Omen of Clarity energy savings like an energy gain for tracking purposes.
    if ( !is_free() && snapshots.clearcasting && current_resource() == RESOURCE_ENERGY &&
         p()->buff.clearcasting_cat->up() )
    {
      p()->buff.clearcasting_cat->decrement();

      // Base cost doesn't factor in but Omen of Clarity does net us less energy during it, so account for that here.
      eff_cost *= 1.0 + p()->buff.incarnation_cat->check_value();

      p()->gain.clearcasting->add( RESOURCE_ENERGY, eff_cost );
    }
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
        p()->sim->print_debug( "persistent-effects: {} ({}) damage modified by {}% with buff {} ({})", name(), id,
                               persistent_multiplier_effects.back().value * 100.0, buff->name(), buff->data().id() );
      }
      else  // values are different
      {
        persistent_direct_effects.push_back( da_multiplier_effects.back() );
        da_multiplier_effects.pop_back();
        p()->sim->print_debug( "persistent-effects: {} ({}) direct damage modified by {}% with buff {} ({})", name(),
                               id, persistent_direct_effects.back().value * 100.0, buff->name(), buff->data().id() );

        persistent_periodic_effects.push_back( ta_multiplier_effects.back() );
        ta_multiplier_effects.pop_back();
        p()->sim->print_debug( "persistent-effects: {} ({}) periodic damage modified by {}% with buff {} ({})", name(),
                               id, persistent_periodic_effects.back().value * 100.0, buff->name(), buff->data().id() );
      }

      return true;
    }

    // no persistent multiplier, but does snapshot & consume the buff
    if ( da_multiplier_effects.size() > da_old || cost_effects.size() > cost_old )
      return true;

    return false;
  }

  snapshot_counter_t* get_counter( buff_t* buff )
  {
    auto st = stats;
    while ( st->parent )
    {
      if ( !st->parent->action_list.front()->harmful )
        return nullptr;
      st = st->parent;
    }

    for ( const auto& c : p()->counters )
      if ( c->stats == st)
        for ( const auto& b : c->buffs )
          if ( b == buff )
            return c.get();

    return p()->counters.emplace_back( std::make_unique<snapshot_counter_t>( p(), st, buff ) ).get();
  }

  void init() override
  {
    base_t::init();

    if ( data().ok() || has_flag( flag_e::AUTOATTACK ) )
    {
      if ( snapshots.bloodtalons && p()->talent.bloodtalons.ok() )
        bt_counter = get_counter( p()->buff.bloodtalons );

      if ( snapshots.tigers_fury && p()->talent.tigers_fury.ok() )
        tf_counter = get_counter( p()->buff.tigers_fury );
    }
  }

  void init_finished() override
  {
    base_t::init_finished();
    base_t::initialize_buff_list_on_vector( persistent_periodic_effects );
    base_t::initialize_buff_list_on_vector( persistent_direct_effects );
  }

  size_t total_effects_count() override
  {
    return base_t::total_effects_count() + persistent_periodic_effects.size() + persistent_direct_effects.size();
  }

  void print_parsed_custom_type( report::sc_html_stream& os ) override
  {
    base_t::print_parsed_custom_type( os );

    base_t::template print_parsed_type<cat_attack_t>( os, &cat_attack_t::persistent_periodic_effects,
                                                      "Snapshots (DOT)" );
    base_t::template print_parsed_type<cat_attack_t>( os, &cat_attack_t::persistent_direct_effects,
                                                      "Snapshots (Direct)" );
  }

  void trigger_dot( action_state_t* s ) override
  {
    // tiger's fury can have different persistent multipliers for DMG_DIRECT vs DMG_OVER_TIME
    // because the state is released after impact, there is no downstream concerns
    if ( snapshots.tigers_fury && s->result_type != result_amount_type::DMG_OVER_TIME )
    {
      s->result_type = result_amount_type::DMG_OVER_TIME;
      s->persistent_multiplier = composite_persistent_multiplier( s );
    }

    base_t::trigger_dot( s );
  }

  void tick( dot_t* d ) override
  {
    base_t::tick( d );

    if ( snapshots.bloodtalons && bt_counter )
      bt_counter->count_tick();

    if ( snapshots.tigers_fury && tf_counter )
      tf_counter->count_tick();
  }

  void execute() override
  {
    base_t::execute();

    if ( snapshots.bloodtalons )
    {
      if ( bt_counter && hit_any_target )
        bt_counter->count_execute();

      p()->buff.bloodtalons->decrement();
    }

    if ( snapshots.tigers_fury && tf_counter && hit_any_target )
      tf_counter->count_execute();

    if ( !hit_any_target )
      player->resource_gain( RESOURCE_ENERGY, last_resource_cost * 0.80, p()->gain.energy_refund );

    if ( harmful )
    {
      if ( special && base_costs[ RESOURCE_ENERGY ] > 0 )
        p()->buff.incarnation_cat->up();
    }
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    auto pers = base_t::composite_persistent_multiplier( s );

    if ( s->result_type == result_amount_type::DMG_DIRECT )
    {
      for ( const auto& i : persistent_direct_effects )
        pers *= 1.0 + base_t::get_effect_value( i );
    }
    else if ( s->result_type == result_amount_type::DMG_OVER_TIME )
    {
      for ( const auto& i : persistent_periodic_effects )
        pers *= 1.0 + base_t::get_effect_value( i );
    }

    return pers;
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
  using base_t = druid_residual_action_t<Base, DOT>;

  double residual_mul = 1.0;

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
  double rage_gain;

  bear_form_buff_t( druid_t* p )
    : base_t( p, "bear_form", p->find_class_spell( "Bear Form" ) ),
      swap_melee_t( p ),
      rage_gain( find_effect( p->find_spell( 17057 ), E_ENERGIZE ).resource( RESOURCE_RAGE ) )
  {
    add_invalidate( CACHE_ARMOR );
    add_invalidate( CACHE_ATTACK_POWER );
    add_invalidate( CACHE_CRIT_AVOIDANCE );
    add_invalidate( CACHE_EXP );
    add_invalidate( CACHE_HIT );
    add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    add_invalidate( CACHE_STAMINA );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    base_t::expire_override( expiration_stacks, remaining_duration );

    swap_melee( p()->caster_melee_attack, p()->caster_form_weapon );

    p()->resource_loss( RESOURCE_RAGE, p()->resources.current[ RESOURCE_RAGE ] );
    p()->recalculate_resource_max( RESOURCE_HEALTH );

    p()->buff.rage_of_the_sleeper->expire();

    make_event( *sim, [ this ] {
      if ( p()->talent.wildshape_mastery.ok() && p()->get_form() == CAT_FORM )
        p()->buff.wildshape_mastery->trigger();
      else
        p()->buff.ironfur->expire();
    } );
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    swap_melee( p()->bear_melee_attack, p()->bear_weapon );

    base_t::start( stacks, value, duration );

    p()->resource_gain( RESOURCE_RAGE, rage_gain, p()->gain.bear_form );
    p()->recalculate_resource_max( RESOURCE_HEALTH );

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
  cat_form_buff_t( druid_t* p ) : base_t( p, "cat_form", p->find_class_spell( "Cat Form" ) ), swap_melee_t( p )
  {
    add_invalidate( CACHE_ATTACK_POWER );
    add_invalidate( CACHE_EXP );
    add_invalidate( CACHE_HIT );
  }

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
  moonkin_form_buff_t( druid_t* p ) : base_t( p, "moonkin_form", p->spec.moonkin_form )
  {
    add_invalidate( CACHE_ARMOR );
    add_invalidate( CACHE_EXP );
    add_invalidate( CACHE_HIT );
  }
};

// Berserk / Incarnation (Bear) Buff ========================================
struct berserk_bear_buff_base_t : public druid_buff_t
{
  berserk_bear_buff_base_t( druid_t* p, std::string_view n, const spell_data_t* s ) : base_t( p, n, s )
  {
    set_cooldown( 0_ms );
    set_refresh_behavior( buff_refresh_behavior::EXTEND );

    if ( p->talent.berserk_unchecked_aggression.ok() )
    {
      set_default_value_from_effect_type( A_HASTE_ALL );
      set_pct_buff_type( STAT_PCT_BUFF_HASTE );
    }
  }

  void start( int s, double v, timespan_t d ) override
  {
    if ( p()->talent.berserk_persistence.ok() )
      p()->cooldown.frenzied_regeneration->reset( true, p()->cooldown.frenzied_regeneration->charges );

    if ( p()->talent.berserk_ravage.ok() )
    {
      p()->cooldown.growl->reset( true );
      p()->cooldown.mangle->reset( true );
      p()->cooldown.thrash_bear->reset( true );
    }

    base_t::start( s, v, d );
  }
};

struct berserk_bear_buff_t final : public berserk_bear_buff_base_t
{
  berserk_bear_buff_t( druid_t* p ) : berserk_bear_buff_base_t( p, "berserk_bear", p->spec.berserk_bear )
  {
    set_name_reporting( "berserk" );
  }
};

struct incarnation_bear_buff_t final : public berserk_bear_buff_base_t
{
  double inc_mul;

  incarnation_bear_buff_t( druid_t* p )
    : berserk_bear_buff_base_t( p, "incarnation_guardian_of_ursoc", p->spec.incarnation_bear ),
      inc_mul( 1.0 + find_effect( p->spec.incarnation_bear, A_MOD_INCREASE_HEALTH_PERCENT ).percent() )
  {}

  void start( int s, double v, timespan_t d ) override
  {
    berserk_bear_buff_base_t::start( s, v, d );

    p()->adjust_health_pct( inc_mul, true );
  }

  void expire_override( int s, timespan_t d ) override
  {
    berserk_bear_buff_base_t::expire_override( s, d );

    p()->adjust_health_pct( inc_mul, false );
  }
};

// Bloodtalons Tracking Buff ================================================
struct bt_dummy_buff_t final : public druid_buff_t
{
  int count;

  bt_dummy_buff_t( druid_t* p, std::string_view n )
    : base_t( p, n ), count( as<int>( p->talent.bloodtalons->effectN( 2 ).base_value() ) )
  {
    // The counting starts from the end of the triggering ability gcd.
    set_duration( timespan_t::from_seconds( p->talent.bloodtalons->effectN( 1 ).base_value() ) );
    set_quiet( true );
    set_refresh_behavior( buff_refresh_behavior::DURATION );
    set_activated( false );
  }

  bool trigger( int s, double v, double c, timespan_t d ) override
  {
    if ( !p()->talent.bloodtalons.ok() )
      return false;

    if ( ( p()->buff.bt_rake->check() + p()->buff.bt_shred->check() + p()->buff.bt_swipe->check() +
           p()->buff.bt_thrash->check() + p()->buff.bt_moonfire->check() + p()->buff.bt_feral_frenzy->check() +
           !this->check() ) < count )
    {
      return base_t::trigger( s, v, c, d );
    }

    p()->buff.bt_rake->expire();
    p()->buff.bt_shred->expire();
    p()->buff.bt_swipe->expire();
    p()->buff.bt_thrash->expire();
    p()->buff.bt_moonfire->expire();
    p()->buff.bt_feral_frenzy->expire();

    p()->buff.bloodtalons->trigger();

    return true;
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

// Earthwarden ==============================================================
struct earthwarden_buff_t final : public druid_absorb_buff_t
{
  double absorb_pct;

  earthwarden_buff_t( druid_t* p )
    : base_t( p, "earthwarden", p->find_spell( 203975 ) ), absorb_pct( p->talent.earthwarden->effectN( 1 ).percent() )
  {
    set_default_value( 1 );
    set_absorb_high_priority( true );
    set_trigger_spell( p->talent.earthwarden );
  }

  double consume( double a, action_state_t* s ) override
  {
    if ( s->action->special )
      return 0;

    auto amount = a * absorb_pct;

    absorb_source->add_result( amount, 0, result_amount_type::ABSORB, RESULT_HIT, BLOCK_RESULT_UNBLOCKED, player );
    absorb_gain->add( RESOURCE_HEALTH, amount, 0 );

    sim->print_debug( "{} {} absorbs {}", *player, *this, amount );

    decrement();

    return amount;
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
    auto amt = eff.resource();

    set_default_value( amt / eff.period().total_seconds() );
    set_tick_callback( [ this, amt, r = eff.resource_gain_type(), g = p->get_gain( n ) ]( buff_t*, int, timespan_t ) {
      player->resource_gain( r, amt, g );
    } );
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
    return base_t::trigger( s, attack_power() * coeff * ( 1.0 + p()->composite_heal_versatility() ), c, d );
  }
};

// Rage of the Sleeper =======================================================
struct rage_of_the_sleeper_buff_t final : public druid_absorb_buff_t
{
  action_t* damage = nullptr;
  double absorb_pct;

  rage_of_the_sleeper_buff_t( druid_t* p )
    : base_t( p, "rage_of_the_sleeper", p->talent.rage_of_the_sleeper ), absorb_pct( data().effectN( 3 ).percent() )
  {
    set_cooldown( 0_ms );
    set_default_value( 1 );
    set_absorb_high_priority( true );
    add_invalidate( CACHE_LEECH );
  }

  double consume( double a, action_state_t* s ) override
  {
    auto amount = a * absorb_pct;

    absorb_source->add_result( amount, 0, result_amount_type::ABSORB, RESULT_HIT, BLOCK_RESULT_UNBLOCKED, player );
    absorb_gain->add( RESOURCE_HEALTH, amount, 0 );

    sim->print_debug( "{} {} absorbs {}", *player, *this, amount );

    if ( damage && s && s->action->player && s->action->player != player )
      damage->execute_on_target( s->action->player );

    return amount;
  }
};

// Shooting Stars ============================================================
struct shooting_stars_buff_t final : public druid_buff_t
{
  std::vector<dot_t*>& dot_list;
  action_t*& shooting;
  action_t*& crashing;
  double base_chance;
  double crashing_chance;

  shooting_stars_buff_t( druid_t* p, std::string_view n, std::vector<dot_t*>& d, action_t*& shs, action_t*& cs )
    : base_t( p, n, p->talent.shooting_stars ),
      dot_list( d ),
      shooting( shs ),
      crashing( cs ),
      base_chance( find_effect( p->talent.shooting_stars, A_PERIODIC_DUMMY ).percent() ),
      crashing_chance( p->talent.crashing_star->effectN( 1 ).percent() )
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
    {
      if ( rng().roll( crashing_chance ) )
        crashing->execute_on_target( dot_list[ i ]->target );
      else
        shooting->execute_on_target( dot_list[ i ]->target );
    }
  }
};
}  // end namespace buffs

namespace pets
{
void treant_base_t::arise()
{
  pet_t::arise();

  o()->buff.bounteous_bloom->trigger();
  o()->buff.harmony_of_the_grove->trigger();

  if ( !o()->buff.treants_of_the_moon->is_fallback )
    static_cast<buffs::treants_of_the_moon_buff_t*>( o()->buff.treants_of_the_moon )->data.insert( this );
}

void treant_base_t::demise()
{
  pet_t::demise();

  o()->buff.bounteous_bloom->decrement();
  o()->buff.harmony_of_the_grove->decrement();

  if ( !o()->buff.treants_of_the_moon->is_fallback )
    static_cast<buffs::treants_of_the_moon_buff_t*>( o()->buff.treants_of_the_moon )->data.erase( this );
}
}  // namespace pets

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

template <typename T, typename... Ts>
T* druid_t::get_secondary_action( std::string_view n, Ts&&... args )
{
  if ( auto it = range::find( secondary_action_list, n, &action_t::name_str ); it != secondary_action_list.cend() )
    return dynamic_cast<T*>( *it );

  T* a;

  if constexpr ( std::is_constructible_v<T, druid_t*, std::string_view, Ts...> )
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
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->shapeshift( form );
  }
};

// Bear Form Spell ==========================================================
struct bear_form_t final : public trigger_call_of_the_elder_druid_t<druid_form_t>
{
  DRUID_ABILITY( bear_form_t, base_t, "bear_form", p->find_class_spell( "Bear Form" ) )
  {
    set_form( BEAR_FORM );
  }

  void execute() override
  {
    base_t::execute();

    p()->buff.ursine_vigor->trigger();
  }
};

// Cat Form Spell ===========================================================
struct cat_form_t final : public trigger_call_of_the_elder_druid_t<druid_form_t>
{
  DRUID_ABILITY( cat_form_t, base_t, "cat_form", p->find_class_spell( "Cat Form" ) )
  {
    set_form( CAT_FORM );
  }
};

// Moonkin Form Spell =======================================================
struct moonkin_form_t final : public druid_form_t
{
  DRUID_ABILITY( moonkin_form_t, druid_form_t, "moonkin_form", p->spec.moonkin_form )
  {
    set_form( MOONKIN_FORM );
  }
};

// Cancelform (revert to caster form)========================================
struct cancel_form_t final : public druid_form_t
{
  DRUID_ABILITY( cancel_form_t, druid_form_t, "cancelform", spell_data_t::nil() )
  {
    set_form( NO_FORM );

    trigger_gcd = 0_ms;
    use_off_gcd = true;
  }
};
}  // namespace spells

namespace cat_attacks
{
struct cat_finisher_data_t
{
  int combo_points = 0;

  void sc_format_to( const cat_finisher_data_t& data, fmt::format_context::iterator out )
  {
    fmt::format_to( out, "combo_points={}", data.combo_points );
  }
};

struct cp_generator_t : public trigger_aggravate_wounds_t<DRUID_FERAL, cat_attack_t>
{
  bool attack_critical = false;
  buff_t* bt_buff = nullptr;
  double berserk_frenzy_mul;

  cp_generator_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE )
    : base_t( n, p, s, f ), berserk_frenzy_mul( p->talent.berserk_frenzy->effectN( 1 ).percent() )
  {
    auto m_data = p->get_modified_spell( &data() );

    set_energize( m_data );

    if ( const auto& eff = find_effect( find_trigger( p->talent.primal_fury ).trigger(), E_ENERGIZE );
         energize && !energize->modified_by( eff ) )
    {
      // technically requires cat form, but as currently the only way to cast cp generators outside of form is via
      // convoke, and convoke doesn't trigger as it is a proc, we are fine for now
      energize->add_parse_entry()
        .set_func( []( const action_t* a, const action_state_t* ) {
          return a ? !a->proc && debug_cast<const cp_generator_t*>( a )->attack_critical : false;
        } )
        .set_flat( true )
        .set_value( eff.resource() )
        .set_eff( &eff );
    }

    if ( const auto& eff = p->spec.berserk_cat->effectN( 2 ); energize && !energize->modified_by( eff ) )
    {
      energize->add_parse_entry()
        .set_buff( p->buff.berserk_cat )
        .set_func( []( const action_t* a, const action_state_t* ) {
          return a ? !a->proc : false;
        } )
        .set_flat( true )
        .set_value( eff.base_value() )
        .set_eff( &eff );
    }
  }

  void execute() override
  {
    attack_critical = false;

    base_t::execute();

    if ( bt_buff )
      bt_buff->trigger();

    if ( attack_critical )
      p()->buff.fell_prey->trigger( this );
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      if ( s->result == RESULT_CRIT )
        attack_critical = true;

      if ( berserk_frenzy_mul && p()->buff.b_inc_cat->check() )
        residual_action::trigger( p()->active.frenzied_assault, s->target, s->result_amount * berserk_frenzy_mul );
    }
  }

  void gain_energize_resource( resource_e rt, double a, gain_t* g ) override
  {
    base_t::gain_energize_resource( rt, a, g );

    // TODO: coiled to spring procs when you end up at 5 CP from a crit while not in berserk
    if ( p()->bugs && attack_critical &&
         rt == RESOURCE_COMBO_POINT && p()->resources.current[ RESOURCE_COMBO_POINT ] == 5 &&
         p()->talent.coiled_to_spring.ok() && !p()->buff.coiled_to_spring->check() &&
         !p()->buff.b_inc_cat->check() )
    {
      p()->buff.coiled_to_spring->trigger();
    }
  }
};

template <class Data = cat_finisher_data_t>
struct cp_spender_t : public trigger_aggravate_wounds_t<DRUID_FERAL, cat_attack_t>
{
protected:
  using state_t = druid_action_state_t<Data>;

public:
  cp_spender_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE )
    : base_t( n, p, s, f )
  {}

  action_state_t* new_state() override
  {
    return new state_t( this, target );
  }

  state_t* cast_state( action_state_t* s )
  {
    return static_cast<state_t*>( s );
  }

  const state_t* cast_state( const action_state_t* s ) const
  {
    return static_cast<const state_t*>( s );
  }

  int cp( const action_state_t* s ) const
  {
    return cast_state( s )->combo_points;
  }

  // used during state snapshot
  virtual int _combo_points() const
  {
    if ( is_free() )
      return as<int>( p()->resources.max[ RESOURCE_COMBO_POINT ] );
    else
      return as<int>( p()->resources.current[ RESOURCE_COMBO_POINT ] );
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

  void execute() override
  {
    base_t::execute();

    if ( !has_flag( flag_e::CONVOKE ) )
      p()->buff.overflowing_power->expire( this );
  }

  void consume_resource() override
  {
    base_t::consume_resource();

    if ( background || !hit_any_target )
      return;

    auto consumed = _combo_points();

    if ( p()->talent.soul_of_the_forest_cat.ok() )
    {
      p()->resource_gain( RESOURCE_ENERGY,
                          consumed * p()->talent.soul_of_the_forest_cat->effectN( 1 ).resource( RESOURCE_ENERGY ),
                          p()->gain.soul_of_the_forest );
    }

    trigger_with_chance_per_cp( p()->buff.frantic_momentum, consumed );
    trigger_with_chance_per_cp( p()->buff.predatory_swiftness, consumed );
    trigger_with_chance_per_cp( p()->buff.sudden_ambush, consumed );

    if ( !is_free() )
    {
      p()->resource_loss( RESOURCE_COMBO_POINT, consumed, nullptr, this );

      if ( sim->log )
      {
        sim->print_log( "{} consumes {} {} for {} (0)", player->name(), consumed,
                        util::resource_type_string( RESOURCE_COMBO_POINT ), name() );
      }

      stats->consume_resource( RESOURCE_COMBO_POINT, consumed );
    }

    if ( p()->buff.tigers_tenacity->check() && p()->buff.tigers_tenacity->can_expire( this ) )
      p()->buff.tigers_tenacity->decrement();
  }
};

using cat_finisher_t = cp_spender_t<>;

template <typename BASE>
struct trigger_thrashing_claws_t : public BASE
{
  using base_t = trigger_thrashing_claws_t<BASE>;

  trigger_thrashing_claws_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f = flag_e::NONE )
    : BASE( n, p, s, f )
  {
    if ( p->active.thrashing_claws )
      BASE::impact_action = p->active.thrashing_claws;
  }
};

// Adaptive Swarm ===========================================================
struct adaptive_swarm_t final : public cat_attack_t
{
  struct adaptive_swarm_state_t final : public action_state_t
  {
    double range = 0.0;
    int stacks = 0;
    bool jump = false;

    adaptive_swarm_state_t( action_t* a, player_t* t ) : action_state_t( a, t ) {}

    void initialize() override
    {
      action_state_t::initialize();

      range = 0.0;
      stacks = as<int>( static_cast<druid_t*>( action->player )->talent.adaptive_swarm->effectN( 1 ).base_value() );
      jump = false;
    }

    void copy_state( const action_state_t* s ) override
    {
      action_state_t::copy_state( s );
      auto s_ = static_cast<const adaptive_swarm_state_t*>( s );

      range = s_->range;
      stacks = s_->stacks;
      jump = s_->jump;
    }

    std::ostringstream& debug_str( std::ostringstream& s ) override
    {
      action_state_t::debug_str( s );

      s << " range=" << range;
      s << " swarm_stacks=" << stacks;

      if ( jump )
        s << " (jumped)";

      return s;
    }
  };

  template <typename BASE, typename OTHER>
  struct adaptive_swarm_base_t : public BASE
  {
    adaptive_swarm_base_t<OTHER, BASE>* other = nullptr;
    double tf_mul;
    bool heal = false;

    adaptive_swarm_base_t( druid_t* p, std::string_view n, const spell_data_t* s )
      : BASE( n, p, s ),
        tf_mul( find_effect( p->talent.tigers_fury, this, A_ADD_PCT_MODIFIER, P_TICK_DAMAGE ).percent() )
    {
      BASE::dual = BASE::background = true;
      BASE::dot_behavior = dot_behavior_e::DOT_CLIP;
    }

    action_state_t* new_state() override
    { return new adaptive_swarm_state_t( this, BASE::target ); }

    adaptive_swarm_state_t* state( action_state_t* s )
    { return static_cast<adaptive_swarm_state_t*>( s ); }

    const adaptive_swarm_state_t* state( const action_state_t* s ) const
    { return static_cast<const adaptive_swarm_state_t*>( s ); }

    timespan_t travel_time() const override
    {
      auto s_ = state( BASE::execute_state );

      if ( s_->jump )
        return timespan_t::from_seconds( s_->range / BASE::travel_speed );

      if ( BASE::target == BASE::player )
        return 0_ms;

      return BASE::travel_time();
    }

    virtual player_t* new_swarm_target( player_t* = nullptr ) const = 0;

    virtual double new_swarm_range( const action_state_t*, player_t* ) const = 0;

    void send_swarm( player_t* t, dot_t* d )
    {
      auto new_state = state( BASE::get_state() );

      BASE::target = new_state->target = t;
      new_state->range = new_swarm_range( d->state, t );
      new_state->stacks = d->current_stack() - 1;
      new_state->jump = true;

      if ( BASE::sim->log )
      {
        BASE::sim->print_log( "ADAPTIVE_SWARM: jumping {} {} stacks to {} ({} yd)", new_state->stacks,
                              heal ? "heal" : "damage", new_state->target->name(), new_state->range );
      }

      BASE::schedule_execute( new_state );
    }

    void jump_swarm( dot_t* d )
    {
      auto new_target = other->new_swarm_target();
      if ( !new_target )
      {
        new_target = new_swarm_target();
        assert( new_target );
        send_swarm( new_target, d );
      }
      else
      {
        other->send_swarm( new_target, d );
      }

      if ( BASE::p()->talent.unbridled_swarm.ok() &&
           BASE::rng().roll( BASE::p()->talent.unbridled_swarm->effectN( 1 ).percent() ) )
      {
        auto second_target = other->new_swarm_target( new_target );
        if ( !second_target )
        {
          second_target = new_swarm_target( new_target );
          if ( !second_target )
            return;

          if ( BASE::sim->log )
            BASE::sim->print_log( "ADAPTIVE_SWARM: splitting off {} swarm", heal ? "heal" : "damage" );

          send_swarm( second_target, d );
        }
        else
        {
          if ( BASE::sim->log )
            BASE::sim->print_log( "ADAPTIVE_SWARM: splitting off {} swarm", other->heal ? "heal" : "damage" );

          other->send_swarm( second_target, d );
        }
      }
    }

    void impact( action_state_t* s ) override
    {
      auto incoming = state( s )->stacks;
      auto existing = BASE::get_dot( s->target )->current_stack();

      BASE::impact( s );

      auto d = BASE::get_dot( s->target );
      d->increment( incoming + existing - 1 );

      if ( BASE::sim->log )
      {
        BASE::sim->print_log( "ADAPTIVE_SWARM: {} incoming:{} existing:{} final:{}", heal ? "heal" : "damage", incoming,
                              existing, d->current_stack() );
      }
    }

    void last_tick( dot_t* d ) override
    {
      BASE::last_tick( d );

      // last_tick() is called via dot_t::cancel() when a new swarm CLIPs an existing swarm. As dot_t::last_tick() is
      // called before dot_t::reset(), check the remaining time on the dot to see if we're dealing with a true last_tick
      // (and thus need to check for a new jump) or just a refresh.

      // NOTE: if demise() is called on the target d->remains() has time left, so assume that a target that is sleeping
      // with time remaining has been demised()'d and jump to a new target
      if ( d->remains() > 0_ms && !d->target->is_sleeping() )
        return;

      if ( d->current_stack() > 1 )
        jump_swarm( d );
      else if ( BASE::sim->log )
        BASE::sim->print_log( "ADAPTIVE_SWARM: {} expiring on final stack", heal ? "heal" : "damage" );
    }
  };

  using damage_swarm_t = adaptive_swarm_base_t<cat_attack_t, druid_heal_t>;
  using healing_swarm_t = adaptive_swarm_base_t<druid_heal_t, cat_attack_t>;

  struct adaptive_swarm_damage_t final : public damage_swarm_t
  {
    adaptive_swarm_damage_t( druid_t* p ) : damage_swarm_t( p, "adaptive_swarm_damage", p->spec.adaptive_swarm_damage )
    {}

    bool dots_ticking( const druid_td_t* td ) const
    {
      return td->dots.moonfire->is_ticking() || td->dots.rake->is_ticking() || td->dots.rip->is_ticking() ||
             td->dots.stellar_flare->is_ticking() || td->dots.sunfire->is_ticking() ||
             td->dots.thrash_bear->is_ticking() || td->dots.thrash_cat->is_ticking();
    }

    player_t* new_swarm_target( player_t* exclude ) const override
    {
      auto tl = target_list();  // make a copy

      // because action_t::available_targets() explicitly adds the current action_t::target to the target_cache, we need
      // to explicitly remove it here as swarm should not pick an invulnerable target whenignore_invulnerable_targets is
      // enabled.
      if ( sim->ignore_invulnerable_targets && target->debuffs.invulnerable->check() )
        range::erase_remove( tl, target );

      if ( exclude )
        range::erase_remove( tl, exclude );

      if ( tl.empty() )
        return nullptr;

      if ( tl.size() > 1 )
      {
        rng().shuffle( tl.begin(), tl.end() );

        // prioritize unswarmed over swarmed
        auto it = range::partition( tl, [ this ]( player_t* t ) {
          return !p()->get_target_data( t )->dots.adaptive_swarm_damage->is_ticking();
        } );

        // if unswarmed exists, prioritize dotted over undotted
        if ( it != tl.begin() )
        {
          std::partition( tl.begin(), it, [ this ]( player_t* t ) {
            return dots_ticking( p()->get_target_data( t ) );
          } );
        }
        // otherwise if swarmed exists, prioritize dotted over undotted
        else if ( it != tl.end() )
        {
          std::partition( it, tl.end(), [ this ]( player_t* t ) {
            return dots_ticking( p()->get_target_data( t ) );
          } );
        }
      }

      return tl.front();
    }

    double new_swarm_range( const action_state_t* s, player_t* ) const override
    {
      return state( s )->range;
    }

    double composite_persistent_multiplier( const action_state_t* s ) const override
    {
      double pm = adaptive_swarm_base_t::composite_persistent_multiplier( s );

      // inherits from druid_spell_t so does not get automatic persistent multiplier parsing
      if ( !state( s )->jump && p()->buff.tigers_fury->up() )
        pm *= 1.0 + tf_mul;

      return pm;
    }
  };

  struct adaptive_swarm_heal_t final : public healing_swarm_t
  {
    adaptive_swarm_heal_t( druid_t* p ) : healing_swarm_t( p, "adaptive_swarm_heal", p->spec.adaptive_swarm_heal )
    {
      quiet = heal = true;

      parse_effect_period( data().effectN( 1 ) );
    }

    player_t* new_swarm_target( player_t* exclude ) const override
    {
      auto tl = target_list();  // make a copy

      if ( exclude )
        range::erase_remove( tl, exclude );

      if ( tl.empty() )
        return nullptr;

      if ( tl.size() > 1 )
      {
        rng().shuffle( tl.begin(), tl.end() );

        // prioritize unswarmed over swarmed
        range::partition( tl, [ this ]( player_t* t ) {
          return !p()->get_target_data( t )->hots.adaptive_swarm_heal->is_ticking();
        } );
      }

      return tl.front();
    }

    double new_swarm_range( const action_state_t*, player_t* t ) const override
    {
      double dis = 0.0;

      if ( t->role == ROLE_ATTACK )
        dis = p()->options.adaptive_swarm_jump_distance_melee;
      else if ( t->role == ROLE_SPELL )
        dis = p()->options.adaptive_swarm_jump_distance_ranged;

      return rng().gauss_a( dis, p()->options.adaptive_swarm_jump_distance_stddev, 0.0 );
    }
  };

  adaptive_swarm_damage_t* damage;
  adaptive_swarm_heal_t* heal;
  timespan_t gcd_add;
  bool target_self = false;

  DRUID_ABILITY( adaptive_swarm_t, cat_attack_t, "adaptive_swarm", p->talent.adaptive_swarm ),
    gcd_add( find_effect( p->spec.cat_form_passive, this, A_ADD_FLAT_LABEL_MODIFIER, P_GCD ).time_value() )
  {
    add_option( opt_bool( "target_self", target_self ) );

    // These are always necessary to allow APL parsing of dot.adaptive_swarm expressions
    damage = p->get_secondary_action<adaptive_swarm_damage_t>( "adaptive_swarm_damage" );
    heal = p->get_secondary_action<adaptive_swarm_heal_t>( "adaptive_swarm_heal" );

    if ( !p->talent.adaptive_swarm.ok() )
      return;

    may_miss = harmful = false;
    base_costs[ RESOURCE_MANA ] = 0.0;  // remove mana cost so we don't need to enable mana regen

    damage->other = heal;
    replace_stats( damage );
    heal->other = damage;
    add_child( damage );
  }

  timespan_t gcd() const override
  {
    timespan_t g = cat_attack_t::gcd();

    if ( p()->buff.cat_form->check() )
      g += gcd_add;

    return g;
  }

  void execute() override
  {
    cat_attack_t::execute();

    if ( target_self )
    {
      heal->set_target( player );
      heal->schedule_execute();
    }
    else
    {
      damage->set_target( target );
      damage->schedule_execute();
    }
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

  bloodseeker_vines_t( druid_t* p, std::string_view n )
    : cat_attack_t( n, p, p->spec.bloodseeker_vines ), twin_pct( p->talent.twin_sprouts->effectN( 1 ).percent() )
  {
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

    // execute() instead of trigger() to avoid proc delay, and add 1ms to ensure final tick is buffed
    td( s->target )->debuff.bloodseeker_vines->execute( 1, buff_t::DEFAULT_VALUE(), dot_duration + 1_ms );

    // TODO: can this trigger itself? what about spread from killed target?
    if ( rng().roll( twin_pct ) && orig_dur == dot_duration )
    {
      const auto& tl = target_list();
      // TODO: determine if this can hit same target if other targets are available
      auto exclude = target_list().size() > 1 ? s->target : nullptr;

      if ( auto tar = p()->get_smart_target( tl, &druid_td_t::dots_t::bloodseeker_vines, exclude ) )
        execute_on_target( tar );
    }
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    return cat_attack_t::composite_target_multiplier( t ) * td( t )->debuff.bloodseeker_vines->check();
  }
};

// Brutal Slash =============================================================
struct brutal_slash_t final : public trigger_claw_rampage_t<DRUID_FERAL,
                                       trigger_wildpower_surge_t<DRUID_FERAL,
                                         trigger_thrashing_claws_t<cp_generator_t>>>
{
  DRUID_ABILITY( brutal_slash_t, base_t, "brutal_slash", p->talent.brutal_slash )
  {
    aoe = -1;
    reduced_aoe_targets = data().effectN( 3 ).base_value();
    track_cd_waste = true;

    if ( p->talent.merciless_claws.ok() )
      bleed_mul = p->talent.merciless_claws->effectN( 1 ).percent();

    bt_buff = p->buff.bt_swipe;
  }

  resource_e current_resource() const override
  {
    return p()->buff.cat_form->check() ? RESOURCE_ENERGY : RESOURCE_NONE;
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    return p()->buff.cat_form->check() ? base_t::composite_energize_amount( s ) : 0.0;
  }
};

// Bursting Growth ==========================================================
struct bursting_growth_t final : public cat_attack_t
{
  bursting_growth_t( druid_t* p ) : cat_attack_t( "bursting_growth", p, p->find_spell( 440122 ) )
  {
    background = true;
    aoe = -1;
    reduced_aoe_targets = 5;  // TODO: not in data, from tooltip
  }
};

// Feral Frenzy =============================================================
struct feral_frenzy_t final : public cat_attack_t
{
  struct feral_frenzy_tick_t final : public cp_generator_t
  {
    bool is_direct_damage = false;

    feral_frenzy_tick_t( druid_t* p, std::string_view n, flag_e f ) : cp_generator_t( n, p, p->find_spell( 274838 ), f )
    {
      background = dual = proc = true;
      direct_bleed = false;

      dot_name = "feral_frenzy_tick";
    }

    // Small hack to properly report instant ticks from the driver, from actual periodic ticks from the bleed
    result_amount_type report_amount_type( const action_state_t* s ) const override
    {
      return is_direct_damage ? result_amount_type::DMG_DIRECT : s->result_type;
    }

    void execute() override
    {
      is_direct_damage = true;
      cp_generator_t::execute();
      is_direct_damage = false;
    }
  };

  DRUID_ABILITY( feral_frenzy_t, cat_attack_t, "feral_frenzy", p->talent.feral_frenzy )
  {
    track_cd_waste = true;

    if ( data().ok() )
    {
      tick_action = p->get_secondary_action<feral_frenzy_tick_t>( name_str + "_tick", f );
      replace_stats( tick_action, false );
    }
  }

  void init() override
  {
    cat_attack_t::init();

    if ( tick_action )
      tick_action->gain = gain;
  }

  void execute() override
  {
    cat_attack_t::execute();

    p()->buff.bt_feral_frenzy->trigger();
  }
};

// Ferocious Bite ===========================================================
struct ferocious_bite_base_t : public cat_finisher_t
{
  struct rampant_ferocity_data_t
  {
    int combo_points = 0;
    double energy_mul = 0.0;

    void sc_format_to( const rampant_ferocity_data_t& data, fmt::format_context::iterator out )
    {
      fmt::format_to( out, "combo_points={} energy_mul={}", data.combo_points, data.energy_mul );
    }
  };

  struct rampant_ferocity_t final : public cat_attack_t
  {
    using state_t = druid_action_state_t<rampant_ferocity_data_t>;

    rampant_ferocity_t( druid_t* p, std::string_view n ) : cat_attack_t( n, p, p->find_spell( 391710 ) )
    {
      aoe = -1;
      reduced_aoe_targets = p->talent.rampant_ferocity->effectN( 1 ).base_value();
      name_str_reporting = "rampant_ferocity";

      // bloodtalons is applied via script
      force_effect( p->buff.bloodtalons, 2, IGNORE_STACKS );
    }

    action_state_t* new_state() override
    {
      return new state_t( this, target );
    }

    state_t* cast_state( action_state_t* s )
    {
      return static_cast<state_t*>( s );
    }

    const state_t* cast_state( const action_state_t* s ) const
    {
      return static_cast<const state_t*>( s );
    }

    std::vector<player_t*>& target_list() const override
    {
      target_cache.is_valid = false;

      auto& tl = cat_attack_t::target_list();

      range::erase_remove( tl, [ this ]( player_t* t ) { return !td( t )->dots.rip->is_ticking() || t == target; } );

      return tl;
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      auto state = cast_state( s );

      return cat_attack_t::composite_da_multiplier( s ) * state->combo_points * ( 1.0 + state->energy_mul );
    }
  };

  cat_attack_t* rampant_ferocity = nullptr;
  double excess_energy = 0.0;
  double max_excess_energy;
  double saber_jaws_mul;
  double rf_energy_mod_pct;
  bool max_energy = false;

  ferocious_bite_base_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f )
    : cat_finisher_t( n, p, s, f ),
      max_excess_energy( find_effect( this, E_POWER_BURN ).resource() ),
      saber_jaws_mul( p->talent.saber_jaws->effectN( 1 ).percent() ),
      rf_energy_mod_pct( p->talent.rampant_ferocity->effectN( 2 ).percent() )
  {
    add_option( opt_bool( "max_energy", max_energy ) );

    if ( p->talent.rampant_ferocity.ok() )
    {
      rampant_ferocity = p->get_secondary_action<rampant_ferocity_t>( "rampant_ferocity_" + name_str );
      rampant_ferocity->background = true;
      add_child( rampant_ferocity );
    }
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

    return cat_finisher_t::ready();
  }

  void execute() override
  {
    // Incarn does affect the additional energy consumption.
    double _max_used = max_excess_energy * ( 1.0 + p()->buff.incarnation_cat->check_value() );

    excess_energy = std::min( _max_used, ( p()->resources.current[ RESOURCE_ENERGY ] - cost() ) );
    
    cat_finisher_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    cat_finisher_t::impact( s );

    if ( s->chain_target || !result_is_hit( s->result ) )  // the rest only procs on main target
      return;

    if ( p()->talent.sabertooth.ok() )
      td( s->target )->debuff.sabertooth->trigger( this, cp( s ) );

    if ( p()->talent.bursting_growth.ok() )
      p()->active.bursting_growth->execute_on_target( s->target );

    if ( rampant_ferocity && s->result_amount > 0 && !rampant_ferocity->target_list().empty() )
    {
      rampant_ferocity->snapshot_and_execute( s, false, [ this ]( const action_state_t* from, action_state_t* to ) {
        auto state = debug_cast<rampant_ferocity_t*>( rampant_ferocity )->cast_state( to );
        state->combo_points = cp( from );

        // TODO: RF from apex/convoke currently does not scale with excess energy, unlike hardcasted FB
        if ( p()->bugs && is_free() )
          state->energy_mul = 1.0;
        else
          state->energy_mul = 1.0 + ( energy_modifier( from ) * rf_energy_mod_pct );
      } );
    }
  }

  void consume_resource() override
  {
    // Extra energy consumption happens first. In-game it happens before the skill even casts but let's not do that
    // because its dumb.
    if ( hit_any_target && !is_free() )
    {
      player->resource_loss( current_resource(), excess_energy );
      stats->consume_resource( current_resource(), excess_energy );
    }

    cat_finisher_t::consume_resource();
  }

  virtual double energy_modifier( const action_state_t* ) const
  {
    return is_free() ? 1.0 : ( excess_energy / max_excess_energy * ( 1.0 + saber_jaws_mul ) );
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    auto dam = cat_finisher_t::composite_da_multiplier( s );
    auto energy_mul = 1.0 + energy_modifier( s );
    // base spell coeff is for 5CP, so we reduce if lower than 5.
    auto combo_mul = cp( s ) / p()->resources.max[ RESOURCE_COMBO_POINT ];

    return dam * energy_mul * combo_mul;
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
      ravage = p->get_secondary_action<ravage_ferocious_bite_t>( "ravage_" + name_str, f );
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
    if ( !has_flag( flag_e::APEX ) && p()->buff.apex_predators_craving->check() &&
         p()->buff.apex_predators_craving->can_expire( this ) )
    {
      p()->last_foreground_action = p()->active.ferocious_bite_apex;
      p()->active.ferocious_bite_apex->execute_on_target( target );
      p()->buff.apex_predators_craving->expire();
      return;
    }

    if ( ravage && p()->buff.ravage_fb->check() )
    {
      ravage->execute_on_target( target );
      p()->buff.ravage_fb->expire( this );
      return;
    }

    ferocious_bite_base_t::execute();
  }
};

// Frenzied Assault =========================================================
struct frenzied_assault_t final : public residual_action::residual_periodic_action_t<cat_attack_t>
{
  frenzied_assault_t( druid_t* p ) : residual_action_t( "frenzied_assault", p, p->find_spell( 391140 ) )
  {
    proc = true;
  }
};

// Lunar Inspiration ========================================================
struct lunar_inspiration_t final : public cp_generator_t
{
  DRUID_ABILITY( lunar_inspiration_t, cp_generator_t, "lunar_inspiration",
                 p->talent.lunar_inspiration.ok() ? p->find_spell( 155625 ) : spell_data_t::not_found() )
  {
    may_dodge = may_parry = may_block = false;
    // LI is a spell, but we parent to cp_generator_t to get all the proper cat attack methods.
    gcd_type = gcd_haste_type::SPELL_CAST_SPEED;

    s_data_reporting = p->talent.lunar_inspiration;
    dot_name = "lunar_inspiration";
    bt_buff = p->buff.bt_moonfire;
  }

  void trigger_dot( action_state_t* s ) override
  {
    // If existing moonfire duration is longer, lunar inspiration dot is not applied
    if ( td( s->target )->dots.moonfire->remains() > composite_dot_duration( s ) )
      return;

    cp_generator_t::trigger_dot( s );
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
struct maim_t final : public cat_finisher_t
{
  DRUID_ABILITY( maim_t, cat_finisher_t, "maim", p->talent.maim ) {}

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    return cat_finisher_t::composite_da_multiplier( s ) * cp( s );
  }
};

// Rake =====================================================================
struct rake_t final : public use_fluid_form_t<DRUID_FERAL, cp_generator_t>
{
  struct rake_bleed_t final : public trigger_thriving_growth_t<2, trigger_waning_twilight_t<cat_attack_t>>
  {
    rake_bleed_t( druid_t* p, std::string_view n, flag_e f, rake_t* r ) : base_t( n, p, find_trigger( r ).trigger(), f )
    {
      background = dual = true;
      // override for convoke. since this is only ever executed from rake_t, form checking is unnecessary.
      form_mask = 0;

      dot_name = "rake";
    }
  };

  rake_bleed_t* bleed;

  DRUID_ABILITY( rake_t, base_t, "rake", p->talent.rake )
  {
    aoe = std::max( aoe, 1 ) + as<int>( p->talent.doubleclawed_rake->effectN( 1 ).base_value() );

    if ( data().ok() )
    {
      bleed = p->get_secondary_action<rake_bleed_t>( name_str + "_bleed", f, this );
      replace_stats( bleed );

      if ( p->talent.pouncing_strikes.ok() || p->spec.improved_prowl->ok() )
      {
        const auto& eff = data().effectN( 4 );
        add_parse_entry( persistent_multiplier_effects )
          .set_value( eff.percent() )
          .set_func( [ this ] { return stealthed_any(); } )
          .set_eff( &eff );

        // check first since bleed is a secondary action with only one instance
        if ( !has_parse_entry( bleed->persistent_multiplier_effects, &eff ) )
        {
          add_parse_entry( bleed->persistent_multiplier_effects )
            .set_value( eff.percent() )
            .set_func( [ this ] { return stealthed_any(); } )
            .set_eff( &eff );
        }
      }
    }

    dot_name = "rake";
    bt_buff = p->buff.bt_rake;
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
        return td( a )->dots.rake->remains() < td( b )->dots.rake->remains();
      } );
    }

    return tl;    
  }

  void execute() override
  {
    // Force invalidate target cache so that it will impact on the correct targets.
    target_cache.is_valid = false;

    base_t::execute();

    if ( !stealthed() )
    	p()->buff.sudden_ambush->expire( this );
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    bleed->snapshot_and_execute( s, true );
  }
};

// Rip ======================================================================
struct rip_t final : public trigger_thriving_growth_t<1, trigger_waning_twilight_t<cat_finisher_t>>
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

  DRUID_ABILITY( rip_t, base_t, "rip", p->talent.rip ),
    apex_pct( find_trigger( p->talent.apex_predators_craving ).percent() * 0.1 )
  {
    dot_name = "rip";

    if ( p->talent.rip_and_tear.ok() )
    {
      auto suf = get_suffix( name_str, "rip" );
      tear = p->get_secondary_action<tear_t>( "tear" + suf, f );
    }

    if ( !p->buff.feline_potential->is_fallback )
    {
      const auto& eff = p->buff.feline_potential->data().effectN( 1 );
      add_parse_entry( persistent_multiplier_effects )
        .set_buff( p->buff.feline_potential )
        .set_value( eff.percent() )
        .set_eff( &eff )
        .set_idx( UINT32_MAX );
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
    return base_t::dot_duration_pct_multiplier( s ) * cp( s ) + 1;
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    if ( tear )
    {
      auto tick_amount = calculate_tick_amount( s, 1.0 );
      auto dot_total = tick_amount * find_dot( s->target )->ticks_left_fractional();

      tear->base_td = dot_total;
      tear->execute_on_target( s->target );
    }

    // hard-cast rip is scripted to consume implant
    if ( !background && p()->active.bloodseeker_vines_implant && p()->buff.implant->check() )
    {
      p()->active.bloodseeker_vines_implant->execute_on_target( s->target );
      p()->buff.implant->expire();
    }
  }

  void tick( dot_t* d ) override
  {
    base_t::tick( d );

    auto c = apex_pct / std::pow( p()->get_active_dots( d ), 0.3 );

    if ( rng().roll( c ) )
      p()->buff.apex_predators_craving->trigger();
  }
};

// Primal Wrath =============================================================
// NOTE: must be defined AFTER rip_T
struct primal_wrath_t final : public cat_finisher_t
{
  rip_t* rip;

  DRUID_ABILITY( primal_wrath_t, cat_finisher_t, "primal_wrath", p->talent.primal_wrath )
  {
    aoe = -1;

    dot_name = "rip";
    // Manually set true so bloodtalons is decremented and we get proper snapshot reporting
    snapshots.bloodtalons = true;

    auto m_data = p->get_modified_spell( &data() )
      ->parse_effects( p->talent.circle_of_life_and_death_cat )
      ->parse_effects( p->talent.veinripper );

    if ( data().ok() )
    {
      rip = p->get_secondary_action<rip_t>( "rip_primal", p->find_spell( 1079 ), f );
      rip->dot_duration = timespan_t::from_seconds( m_data->effectN( 2 ).base_value() );
      rip->dual = rip->background = true;
      replace_stats( rip );
      rip->base_costs[ RESOURCE_ENERGY ] = 0;
      // mods are parsed on construction so set to false so the rip execute doesn't decrement
      rip->snapshots.bloodtalons = false;

      if ( p->talent.adaptive_swarm.ok() )
      {
        const auto& eff = p->spec.adaptive_swarm_damage->effectN( 2 );
        add_parse_entry( target_multiplier_effects )
          .set_func( d_fn( &druid_td_t::dots_t::adaptive_swarm_damage ) )
          .set_value( eff.percent() )
          .set_eff( &eff );
      }
    }
  }

  std::vector<player_t*>& target_list() const override
  {
    // target order is randomized, can be important for rip application ordering
    auto& tl = cat_finisher_t::target_list();

    rng().shuffle( tl.begin(), tl.end() );

    return tl;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    return cat_finisher_t::composite_da_multiplier( s ) * ( 1.0 + cp( s ) );
  }

  void impact( action_state_t* s ) override
  {
    rip->snapshot_and_execute( s, true );

    cat_finisher_t::impact( s );
  }
};

// Shred ====================================================================
struct shred_t final : public use_fluid_form_t<DRUID_FERAL,
                                trigger_claw_rampage_t<DRUID_FERAL,
                                  trigger_wildpower_surge_t<DRUID_FERAL,
                                    trigger_thrashing_claws_t<cp_generator_t>>>>
{
  double stealth_mul = 0.0;

  DRUID_ABILITY( shred_t, base_t, "shred", p->find_class_spell( "Shred" ) )
  {
    // TODO adjust if it becomes possible to take both
    bleed_mul = std::max( p->talent.merciless_claws->effectN( 2 ).percent(),
                          p->talent.thrashing_claws->effectN( 1 ).percent() );

    if ( p->talent.pouncing_strikes.ok() )
    {
      stealth_mul = data().effectN( 3 ).percent();

      add_parse_entry( da_multiplier_effects )
        .set_value( stealth_mul )
        .set_func( [ this ] { return stealthed_any(); } )
        .set_eff( &data().effectN( 3 ) );

      if ( const auto& eff = p->find_spell( 343232 )->effectN( 1 ); energize && !energize->modified_by( eff ) )
      {
        energize->add_parse_entry()
          .set_value( eff.base_value() )
          .set_func( []( const action_t* a, const action_state_t* ) {
            return a ? debug_cast<const shred_t*>( a )->stealthed_any() : false;
          } )
          .set_flat( true )
          .set_eff( &eff );
      }
    }

    bt_buff = p->buff.bt_shred;
  }

  void execute() override
  {
    base_t::execute();

    if ( !stealthed() )
      p()->buff.sudden_ambush->expire( this );
  }

  double composite_crit_chance_multiplier() const override
  {
    double cm = base_t::composite_crit_chance_multiplier();

    if ( stealth_mul && stealthed_any() )
      cm *= 2.0;

    return cm;
  }
};

// Swipe (Cat) ====================================================================
struct swipe_cat_t final : public trigger_claw_rampage_t<DRUID_FERAL,
                                    trigger_wildpower_surge_t<DRUID_FERAL,
                                      trigger_thrashing_claws_t<cp_generator_t>>>
{
  DRUID_ABILITY( swipe_cat_t, base_t, "swipe_cat", p->apply_override( p->spec.swipe, p->spec.cat_form_override ) )
  {
    aoe = -1;
    reduced_aoe_targets = data().effectN( 4 ).base_value();

    if ( p->talent.merciless_claws.ok() )
      bleed_mul = p->talent.merciless_claws->effectN( 1 ).percent();

    if ( p->specialization() == DRUID_FERAL )
      name_str_reporting = "swipe";

    bt_buff = p->buff.bt_swipe;
  }

  bool ready() override
  {
    return p()->talent.brutal_slash.ok() ? false : base_t::ready();
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
    p()->buff.tigers_strength->trigger();

    if ( p()->buff.killing_strikes_combat->check() )
    {
      p()->buff.killing_strikes_combat->expire();
      p()->buff.ravage_fb->trigger();
    }
  }
};

// Thrash (Cat) =============================================================
struct thrash_cat_t final : public trigger_claw_rampage_t<DRUID_FERAL, cp_generator_t>
{
  struct thrash_cat_bleed_t final : public trigger_waning_twilight_t<cat_attack_t>
  {
    thrash_cat_bleed_t( druid_t* p, std::string_view n, flag_e f ) : base_t( n, p, p->spec.thrash_cat_bleed, f )
    {
      dual = background = true;

      dot_name = "thrash_cat";

      // NOTE: thrash dot snapshot data is missing, so manually add here
      if ( !p->buff.clearcasting_cat->is_fallback && p->talent.moment_of_clarity.ok() )
      {
        add_parse_entry( persistent_periodic_effects )
          .set_buff( p->buff.clearcasting_cat )
          .set_use_stacks( false )
          .set_value( p->talent.moment_of_clarity->effectN( 4 ).percent() )
          .set_eff( &p->buff.clearcasting_cat->data().effectN( 4 ) );
      }
    }

    void trigger_dot( action_state_t* s ) override
    {
      // cat thrash is not applied if existing bear thrash has longer duration
      if ( auto thrash_bear = td( s->target )->dots.thrash_bear; thrash_bear->remains() <= composite_dot_duration( s ) )
      {
        thrash_bear->cancel();
        base_t::trigger_dot( s );
      }
    }
  };

  DRUID_ABILITY( thrash_cat_t, base_t, "thrash_cat", p->apply_override( p->talent.thrash, p->spec.cat_form_override ) )
  {
    aoe = -1;

    impact_action = p->get_secondary_action<thrash_cat_bleed_t>( name_str + "_bleed", f );
    replace_stats( impact_action );

    dot_name = "thrash_cat";

    if ( p->specialization() == DRUID_FERAL )
      name_str_reporting = "thrash";

    bt_buff = p->buff.bt_thrash;
  }
};
}  // end namespace cat_attacks

namespace bear_attacks
{
template <typename BASE = bear_attack_t>
struct rage_spender_t : public BASE
{
private:
  druid_t* p_;

protected:
  using base_t = rage_spender_t<BASE>;

public:
  buff_t* atw_buff;
  double ug_cdr;

  rage_spender_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f )
    : BASE( n, p, s, f ),
      p_( p ),
      atw_buff( p->buff.after_the_wildfire ),
      ug_cdr( p->talent.ursocs_guidance->effectN( 5 ).base_value() )
  {}

  void consume_resource() override
  {
    BASE::consume_resource();

    if ( BASE::is_free() || !BASE::last_resource_cost )
      return;

    if ( p_->talent.after_the_wildfire.ok() )
    {
      if ( !atw_buff->check() )
        atw_buff->trigger();

      atw_buff->current_value -= BASE::last_resource_cost;

      if ( atw_buff->current_value <= 0 )
      {
        atw_buff->current_value += atw_buff->default_value;

        p_->active.after_the_wildfire_heal->execute();
      }
    }

    if ( p_->talent.ursocs_guidance.ok() && p_->talent.incarnation_bear.ok() )
    {
      p_->cooldown.incarnation_bear->adjust( timespan_t::from_seconds( BASE::last_resource_cost / -ug_cdr ) );
      p_->cooldown.berserk_bear->adjust( timespan_t::from_seconds( BASE::last_resource_cost / -ug_cdr ) );
    }
  }
};

template <typename BASE>
struct trigger_ursocs_fury_t : public BASE
{
private:
  druid_t* p_;
  double cap = 0.3;  // not in spell data
  double mul;

public:
  using base_t = trigger_ursocs_fury_t<BASE>;

  trigger_ursocs_fury_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f )
    : BASE( n, p, s, f ),
      p_( p ),
      mul( p->talent.ursocs_fury->effectN( 1 ).percent() *
           ( 1.0 + p->talent.nurturing_instinct->effectN( 1 ).percent() ) )
  {}

  void trigger_ursocs_fury( const action_state_t* s )
  {
    if ( !s->result_amount )
      return;

    p_->buff.ursocs_fury->trigger( 1, s->result_amount * ( 1.0 + mul ) );
    p_->buff.ursocs_fury->current_value = std::min( p_->buff.ursocs_fury->current_value, cap * p_->max_health() );
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
  buff_t* buff;

  berserk_bear_base_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f )
    : bear_attack_t( n, p, s, f ), buff( p->buff.berserk_bear )
  {
    harmful   = false;
    form_mask = BEAR_FORM;
    autoshift = p->active.shift_to_bear;
  }

  void execute() override
  {
    bear_attack_t::execute();

    buff->trigger();

    // Always resets mangle & thrash CDs, even without berserk: ravage
    p()->cooldown.mangle->reset( true );
    p()->cooldown.thrash_bear->reset( true );

    if ( p()->talent.berserk_persistence.ok() )
      p()->cooldown.frenzied_regeneration->reset( true );
  }
};

struct berserk_bear_t final : public berserk_bear_base_t
{
  DRUID_ABILITY( berserk_bear_t, berserk_bear_base_t, "berserk_bear",
                 p->talent.incarnation_bear.ok() ? spell_data_t::not_found() : p->spec.berserk_bear )
  {
    name_str_reporting = "berserk";
  }
};

struct incarnation_bear_t final : public berserk_bear_base_t
{
  DRUID_ABILITY( incarnation_bear_t, berserk_bear_base_t, "incarnation_guardian_of_ursoc", p->spec.incarnation_bear )
  {
    buff = p->buff.incarnation_bear;
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
    gcd_type = gcd_haste_type::ATTACK_HASTE;
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
struct ironfur_t final : public rage_spender_t<>
{
  struct thorns_of_iron_t final : public bear_attack_t
  {
    double mul;

    thorns_of_iron_t( druid_t* p, flag_e f )
      : bear_attack_t( "thorns_of_iron", p, find_trigger( p->talent.thorns_of_iron ).trigger(), f ),
        mul( find_trigger( p->talent.thorns_of_iron ).percent() )
    {
      background = proc = split_aoe_damage = true;
      aoe = -1;
    }

    double base_da_min( const action_state_t* ) const override
    {
      return p()->cache.armor() * mul;
    }

    double base_da_max( const action_state_t* ) const override
    {
      return p()->cache.armor() * mul;
    }

    double action_multiplier() const override
    {
      auto am = bear_attack_t::action_multiplier();
      auto lm = p()->buff.ironfur->check();

      if ( lm > 4 )
        am *= std::pow( 0.955, lm - 4 );  // approx. from testing

      return am;
    }
  };

  action_t* thorns = nullptr;
  timespan_t goe_ext;
  double lm_chance;

  DRUID_ABILITY( ironfur_t, base_t, "ironfur", p->talent.ironfur ),
    goe_ext( find_effect( p->buff.guardian_of_elune, A_ADD_FLAT_MODIFIER, P_DURATION ).time_value() ),
    lm_chance( p->talent.layered_mane->effectN( 1 ).percent() )
  {
    use_off_gcd = true;
    harmful = may_miss = may_dodge = may_parry = may_block = false;

    if ( p->talent.thorns_of_iron.ok() )
      thorns = p->get_secondary_action<thorns_of_iron_t>( "thorns_of_iron", f );
  }

  void execute() override
  {
    base_t::execute();

    int stack = 1;

    // TODO: does guardian of elune also apply to the extra application from layered mane?
    if ( p()->talent.layered_mane.ok() && rng().roll( lm_chance ) )
      stack++;

    auto dur = p()->buff.ironfur->buff_duration();

    if ( p()->buff.guardian_of_elune->check() )
    {
      dur += goe_ext;
      p()->buff.guardian_of_elune->expire( this );
    }

    p()->buff.ironfur->trigger( stack, dur );

    if ( thorns && !proc )
      thorns->execute();
  }
};

// Lunar Beam ===============================================================
struct lunar_beam_t final : public bear_attack_t
{
  struct lunar_beam_heal_t final : public druid_heal_t
  {
    lunar_beam_heal_t( druid_t* p, flag_e f ) : druid_heal_t( "lunar_beam_heal", p, p->find_spell( 204069 ), f )
    {
      background = true;
      name_str_reporting = "Heal";

      target = p;
    }
  };

  struct lunar_beam_tick_t final : public trigger_atmospheric_exposure_t<bear_attack_t>
  {
    lunar_beam_tick_t( druid_t* p, flag_e f ) : base_t( "lunar_beam_tick", p, p->find_spell( 414613 ), f )
    {
      background = dual = ground_aoe = true;
      aoe = -1;

      execute_action = p->get_secondary_action<lunar_beam_heal_t>( "lunar_beam_heal", f );
    }
  };

  ground_aoe_params_t params;
  action_t* damage = nullptr;

  DRUID_ABILITY( lunar_beam_t, bear_attack_t, "lunar_beam", p->talent.lunar_beam )
  {
    if ( data().ok() )
    {
      damage = p->get_secondary_action<lunar_beam_tick_t>( "lunar_beam_tick", f );
      replace_stats( damage );

      params.pulse_time( 1_s )
        .duration( p->buff.lunar_beam->buff_duration() )
        .action( damage );
    }
  }

  // needed to allow on-cast procs
  bool has_amount_result() const override { return damage->has_amount_result(); }

  void execute() override
  {
    bear_attack_t::execute();

    p()->buff.lunar_beam->trigger();

    params.start_time( timespan_t::min() )  // reset start time
      .target( target )
      .x( p()->x_position )
      .y( p()->y_position );

    make_event<ground_aoe_event_t>( *sim, p(), params );
  }
};

// Mangle ===================================================================
struct mangle_t final : public use_fluid_form_t<DRUID_GUARDIAN,
                                 trigger_claw_rampage_t<DRUID_GUARDIAN,
                                   trigger_aggravate_wounds_t<DRUID_GUARDIAN,
                                     trigger_wildpower_surge_t<DRUID_GUARDIAN, bear_attack_t>>>>
{
  int inc_targets = 0;

  DRUID_ABILITY( mangle_t, base_t, "mangle", p->find_class_spell( "Mangle" ) )
  {
    track_cd_waste = true;

    auto m_data = p->get_modified_spell( &data() )
      ->parse_effects( p->talent.soul_of_the_forest_bear )
      ->parse_effects( p->buff.gore );

    set_energize( m_data );

    if ( p->talent.mangle.ok() )
      bleed_mul = data().effectN( 3 ).percent();

    if ( p->talent.incarnation_bear.ok() )
    {
      inc_targets =
        as<int>( find_effect( p->spec.incarnation_bear, this, A_ADD_FLAT_MODIFIER, P_TARGET ).base_value() );
    }

    if ( p->talent.strike_for_the_heart.ok() )
    {
      impact_action = p->get_secondary_action<druid_heal_t>(
        "strike_for_the_heart", "strike_for_the_heart", p, find_trigger( p->talent.strike_for_the_heart ).trigger() );
      impact_action->background = true;
    }
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
    p()->buff.vicious_cycle_maul->trigger( this, num_targets_hit );

    p()->buff.gore->expire( this );

    if ( p()->specialization() == DRUID_GUARDIAN && p()->buff.killing_strikes_combat->check() )
    {
      p()->buff.killing_strikes_combat->expire( this );
      p()->buff.ravage_maul->trigger();
    }
  }
};

// Maul =====================================================================
struct maul_base_t : public trigger_aggravate_wounds_t<DRUID_GUARDIAN,
                              trigger_ursocs_fury_t<trigger_gore_t<rage_spender_t<>>>>
{
  maul_base_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f ) : base_t( n, p, s, f ) {}

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    if ( p()->buff.tooth_and_claw->check() )
      td( s->target )->debuff.tooth_and_claw->trigger();
  }

  void execute() override
  {
    base_t::execute();

    p()->buff.vicious_cycle_mangle->trigger( this );
  }
};

struct maul_t final : public maul_base_t
{
  struct ravage_maul_t final : public ravage_base_t<maul_base_t, use_dot_list_t<bear_attack_t>>
  {
    ravage_maul_t( druid_t* p, std::string_view n, flag_e f ) : base_t( n, p, p->find_spell( 441605 ), f ) {}
  };

  action_t* ravage = nullptr;

  DRUID_ABILITY( maul_t, maul_base_t, "maul", p->talent.maul )
  {
    if ( !p->buff.ravage_maul->is_fallback )
    {
      ravage = p->get_secondary_action<ravage_maul_t>( "ravage_" + name_str, f );
      add_child( ravage );
    }
  }

  void execute() override
  {
    if ( !has_flag( flag_e::TOOTHANDCLAW ) && p()->buff.tooth_and_claw->check() &&
         p()->buff.tooth_and_claw->can_expire( this ) )
    {
      p()->last_foreground_action = p()->active.maul_tooth_and_claw;
      p()->active.maul_tooth_and_claw->execute_on_target( target );
      p()->buff.tooth_and_claw->decrement();  // TODO: adjust if cases arise where it doesn't consume
      return;
    }

    if ( ravage && p()->buff.ravage_maul->check() )
    {
      ravage->execute_on_target( target );
      p()->buff.ravage_maul->expire( this );
      return;
    }

    maul_base_t::execute();
  }
};

// Pulverize ================================================================
struct pulverize_t final : public bear_attack_t
{
  int consume;

  DRUID_ABILITY( pulverize_t, bear_attack_t, "pulverize", p->talent.pulverize ),
    consume( as<int>( data().effectN( 3 ).base_value() ) )
  {}

  void impact( action_state_t* s ) override
  {
    bear_attack_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      if ( !proc )
        td( s->target )->dots.thrash_bear->decrement( consume );

      td( s->target )->debuff.pulverize->trigger();
    }
  }

  bool target_ready( player_t* t ) override
  {
    // Call bear_attack_t::ready() first for proper targeting support.
    // Hardcode stack max since it may not be set if this code runs before Thrash is cast.
    return bear_attack_t::target_ready( t ) && ( is_free() || td( t )->dots.thrash_bear->current_stack() >= consume );
  }
};

// Rage of the Sleeper ======================================================
struct rage_of_the_sleeper_t final : public bear_attack_t
{
  struct rage_of_the_sleeper_reflect_t final : public bear_attack_t
  {
    rage_of_the_sleeper_reflect_t( druid_t* p )
      : bear_attack_t( "rage_of_the_sleeper_reflect", p, p->find_spell( 219432 ) )
    {
      background = proc = dual = true;
    }
  };

  action_t* damage = nullptr;

  DRUID_ABILITY( rage_of_the_sleeper_t, bear_attack_t, "rage_of_the_sleeper", p->talent.rage_of_the_sleeper )
  {
    harmful = may_miss = may_dodge = may_parry = may_block = false;

    if ( data().ok() )
    {
      damage = p->get_secondary_action<rage_of_the_sleeper_reflect_t>( "rage_of_the_sleeper_reflect" );
      replace_stats( damage );

      debug_cast<buffs::rage_of_the_sleeper_buff_t*>( p->buff.rage_of_the_sleeper )->damage = damage;
    }
  }

  void execute() override
  {
    bear_attack_t::execute();

    p()->buff.rage_of_the_sleeper->trigger();
  }
};

// Raze =====================================================================
struct raze_t final : public trigger_aggravate_wounds_t<DRUID_GUARDIAN,
                               trigger_ursocs_fury_t<trigger_gore_t<rage_spender_t<>>>>
{
  DRUID_ABILITY( raze_t, base_t, "raze", p->talent.raze )
  {
    aoe = -1;  // actually a frontal cone
    reduced_aoe_targets = 5.0;  // PTR not in spell data
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    if ( p()->buff.tooth_and_claw->check() )
      td( s->target )->debuff.tooth_and_claw->trigger();
  }

  void execute() override
  {
    if ( !has_flag( flag_e::TOOTHANDCLAW ) && p()->buff.tooth_and_claw->check() &&
         p()->buff.tooth_and_claw->can_expire( this ) )
    {
      p()->last_foreground_action = p()->active.raze_tooth_and_claw;
      p()->active.raze_tooth_and_claw->execute_on_target( target );
      p()->buff.tooth_and_claw->decrement();  // TODO: adjust if cases arise where it doesn't consume
      return;
    }

    base_t::execute();

    // raze triggers vicious cycle once per target hit
    if ( hit_any_target )
      p()->buff.vicious_cycle_mangle->trigger( this, num_targets_hit );
  }
};

// Swipe (Bear) =============================================================
struct swipe_bear_t final : public trigger_claw_rampage_t<DRUID_GUARDIAN,
                                     trigger_aggravate_wounds_t<DRUID_GUARDIAN,
                                       trigger_gore_t<bear_attack_t>>>
{
  DRUID_ABILITY( swipe_bear_t, base_t, "swipe_bear",
                 p->apply_override( p->spec.swipe, p->spec.bear_form_override ) )
  {
    aoe = -1;
    // target hit data stored in cat swipe
    reduced_aoe_targets = p->apply_override( p->spec.swipe, p->spec.cat_form_override )->effectN( 4 ).base_value();

    if ( p->specialization() == DRUID_GUARDIAN )
      name_str_reporting = "swipe";
  }
};

// Thrash (Bear) ============================================================
struct thrash_bear_t final : public trigger_claw_rampage_t<DRUID_GUARDIAN,
                                      trigger_aggravate_wounds_t<DRUID_GUARDIAN,
                                        trigger_guardians_tenacity_t<
                                          trigger_ursocs_fury_t<
                                            trigger_gore_t<bear_attack_t>>>>>
{
  struct thrash_bear_bleed_t final
    : public trigger_ursocs_fury_t<use_dot_list_t<trigger_waning_twilight_t<bear_attack_t>>>
  {
    thrash_bear_bleed_t( druid_t* p, std::string_view n, flag_e f ) : base_t( n, p, p->spec.thrash_bear_bleed, f )
    {
      dual = background = true;

      dot_name = "thrash_bear";
      dot_list = &p->dot_lists.thrash_bear;
    }

    void trigger_dot( action_state_t* s ) override
    {
      // existing cat thrash is cancelled only if it's shorter than bear thrash duration
      if ( auto thrash_cat = td( s->target )->dots.thrash_cat; thrash_cat->remains() < composite_dot_duration( s ) )
        thrash_cat->cancel();

      base_t::trigger_dot( s );
    }

    void tick( dot_t* d ) override
    {
      // if both cat thrash and bear thrash is up (due to cat thrash having a longer duration) then bear thrash damage
      // is suppressed
      if ( td( d->target )->dots.thrash_cat->is_ticking() )
        return;

      base_t::tick( d );
    }
  };

  double fc_pct;

  DRUID_ABILITY( thrash_bear_t, base_t, "thrash_bear",
                 p->apply_override( p->talent.thrash, p->spec.bear_form_override ) ),
      fc_pct( p->talent.flashing_claws->effectN( 1 ).percent() )
  {
    aoe = -1;
    impact_action = p->get_secondary_action<thrash_bear_bleed_t>( name_str + "_dot", f );
    replace_stats( impact_action );
    track_cd_waste = true;

    dot_name = "thrash_bear";

    if ( p->specialization() == DRUID_GUARDIAN )
      name_str_reporting = "thrash";
  }

  void execute() override
  {
    base_t::execute();

    if ( rng().roll( fc_pct ) )
      make_event( *sim, 500_ms, [ this ]() { p()->active.thrash_bear_flashing->execute_on_target( target ); } );
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( result_is_hit( s->result ) )
      p()->buff.earthwarden->trigger( this );
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
      background = true;
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

public:
  using base_t = trigger_lethal_preservation_t<BASE>;

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

// After the Wildfire =======================================================
struct after_the_wildfire_heal_t final : public druid_heal_t
{
  after_the_wildfire_heal_t( druid_t* p ) : druid_heal_t( "after_the_wildfire", p, p->find_spell( 371982 ) )
  {
    proc = true;
    aoe = 5;  // TODO: not in known spell data
  }
};

// Boundless Moonlight Heal =================================================
struct boundless_moonlight_heal_t final : public druid_residual_action_t<druid_heal_t>
{
  boundless_moonlight_heal_t( druid_t* p ) : base_t( "boundless_moonlight", p, p->find_spell( 425206 ) )
  {
    background = proc = true;
  }
};

// Cenarion Ward ============================================================
struct cenarion_ward_t final : public druid_heal_t
{
  struct cenarion_ward_hot_t final : public druid_heal_t
  {
    cenarion_ward_hot_t( druid_t* p )
      : druid_heal_t( "cenarion_ward_hot", p, p->talent.cenarion_ward->effectN( 1 ).trigger() )
    {
      background = true;

      dot_name = "cenarion_ward";
    }
  };

  DRUID_ABILITY( cenarion_ward_t, druid_heal_t, "cenarion_ward", p->talent.cenarion_ward )
  {
    if ( data().ok() )
    {
      auto hot = p->get_secondary_action<cenarion_ward_hot_t>( "cenarion_ward_hot" );

      p->buff.cenarion_ward->set_expire_callback( [ hot ]( buff_t* b, int, timespan_t ) {
        hot->execute_on_target( b->player );
      } );
    }
  }

  void execute() override
  {
    druid_heal_t::execute();

    p()->buff.cenarion_ward->trigger();
  }
};

// Efflorescence ============================================================
struct efflorescence_t final : public druid_heal_t
{
  struct spring_blossoms_t final : public druid_heal_t
  {
    spring_blossoms_t( druid_t* p ) : druid_heal_t( "spring_blossoms", p, p->find_spell( 207386 ) ) {}
  };

  struct efflorescence_tick_t final : public druid_heal_t
  {
    action_t* spring = nullptr;

    efflorescence_tick_t( druid_t* p ) : druid_heal_t( "efflorescence_tick", p, p->find_spell( 81269 ) )
    {
      background = dual = ground_aoe = true;
      aoe = 3;  // hardcoded
      name_str_reporting = "efflorescence";

      if ( p->talent.spring_blossoms.ok() )
        spring = p->get_secondary_action<spring_blossoms_t>( "spring_blossoms" );
    }

    std::vector<player_t*>& target_list() const override
    {
      // get the full list
      auto& tl = druid_heal_t::target_list();

      rng().shuffle( tl.begin(), tl.end() );

      // sort by targets without spring blossoms
      if ( spring )
      {
        range::sort( tl, [ this ]( player_t* l, player_t* r ) {
          return td( l )->hots.spring_blossoms->is_ticking() < td( r )->hots.spring_blossoms->is_ticking();
        } );
      }
      else
      {
        range::sort( tl, []( const player_t* l, const player_t* r ) {
          return l->health_percentage() < r->health_percentage();
        } );
      }

      return tl;
    }

    void execute() override
    {
      // force re-evaluation of targets every tick
      target_cache.is_valid = false;

      druid_heal_t::execute();
    }


    void impact( action_state_t* s ) override
    {
      druid_heal_t::impact( s );

      if ( spring )
        spring->execute_on_target( s->target );
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
      replace_stats( heal );

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

// Frenzied Regeneration ====================================================
struct frenzied_regeneration_t final : public bear_attacks::rage_spender_t<druid_heal_t>
{
  cooldown_t* dummy_cd;
  cooldown_t* orig_cd;
  double goe_mul = 0.0;
  double ir_mul;
  double lm_pct;

  DRUID_ABILITY( frenzied_regeneration_t, base_t, "frenzied_regeneration", p->talent.frenzied_regeneration ),
    dummy_cd( p->get_cooldown( "dummy_cd" ) ),
    orig_cd( cooldown ),
    ir_mul( p->talent.innate_resolve->effectN( 1 ).percent() ),
    lm_pct( p->talent.layered_mane->effectN( 2 ).percent() )
  {
    target = p;

    if ( p->talent.guardian_of_elune.ok() )
      goe_mul = p->buff.guardian_of_elune->data().effectN( 2 ).percent();

    if ( p->talent.empowered_shapeshifting.ok() )
    {
      form_mask |= CAT_FORM;

      base_costs[ RESOURCE_ENERGY ] =
        find_effect( p->talent.empowered_shapeshifting, this, A_ADD_FLAT_MODIFIER, P_RESOURCE_COST_1 )
          .resource( RESOURCE_ENERGY );
    }
  }

  resource_e current_resource() const override
  {
    if ( p()->talent.empowered_shapeshifting.ok() && p()->buff.cat_form->check() )
      return RESOURCE_ENERGY;
    else
      return base_t::current_resource();
  }

  void execute() override
  {
    if ( rng().roll( lm_pct ) )
      cooldown = dummy_cd;

    base_t::execute();

    cooldown = orig_cd;

    p()->buff.guardian_of_elune->expire( this );
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

// Flourish =================================================================
struct flourish_t final : public druid_heal_t
{
  timespan_t ext;

  DRUID_ABILITY( flourish_t, druid_heal_t, "flourish", p->talent.flourish ),
    ext( timespan_t::from_seconds( data().effectN( 1 ).base_value() ) )
  {
    dot_duration = 0_ms;
  }

  void execute() override
  {
    druid_heal_t::execute();

    p()->buff.flourish->trigger();

    for ( const auto& t : sim->player_non_sleeping_list )
    {
      auto hots = td( t )->hots;
      hots.cenarion_ward->adjust_duration( ext );
      hots.cultivation->adjust_duration( ext );
      hots.germination->adjust_duration( ext );
      hots.lifebloom->adjust_duration( ext );
      hots.regrowth->adjust_duration( ext );
      hots.rejuvenation->adjust_duration( ext );
      hots.spring_blossoms->adjust_duration( ext );
      hots.wild_growth->adjust_duration( ext );
    }
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
      replace_stats( bloom );
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
  DRUID_ABILITY( natures_swiftness_t, base_t, "natures_swiftness", p->talent.natures_swiftness ) {}

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

// Nourish ==================================================================
struct nourish_t final : public druid_heal_t
{
  DRUID_ABILITY( nourish_t, druid_heal_t, "nourish", p->talent.nourish ) {}

  double harmony_multiplier( player_t* t ) const override
  {
    return druid_heal_t::harmony_multiplier( t ) * 3.0;  // multiplier not in any spell data
  }
};

// Regrowth =================================================================
struct regrowth_t final : public druid_heal_t
{
  timespan_t gcd_add;
  double bonus_crit;
  double sotf_mul;
  bool is_direct = false;

  DRUID_ABILITY( regrowth_t, druid_heal_t, "regrowth", p->find_class_spell( "Regrowth" ) ),
    gcd_add( find_effect( p->spec.cat_form_passive, this, A_ADD_FLAT_MODIFIER, P_GCD ).time_value() ),
    bonus_crit( p->talent.improved_regrowth->effectN( 1 ).percent() ),
    sotf_mul( p->buff.soul_of_the_forest_tree->data().effectN( 1 ).percent() )
  {
    form_mask = NO_FORM | MOONKIN_FORM;

    affected_by.soul_of_the_forest = true;

    if ( !p->buff.dream_of_cenarius->is_fallback )
    {
      const auto& eff = find_effect( p->buff.dream_of_cenarius, this, A_ADD_PCT_MODIFIER, P_TICK_DAMAGE );
      add_parse_entry( persistent_multiplier_effects )
        .set_buff( p->buff.dream_of_cenarius )
        .set_value( eff.percent() )
        .set_eff( &eff );
    }
  }

  timespan_t gcd() const override
  {
    timespan_t g = druid_heal_t::gcd();

    if ( p()->buff.cat_form->check() )
      g += gcd_add;

    return g;
  }

  timespan_t execute_time() const override
  {
    if ( p()->buff.incarnation_tree->check() )
      return 0_ms;

    return druid_heal_t::execute_time();
  }

  double composite_target_crit_chance( player_t* t ) const override
  {
    double tcc = druid_heal_t::composite_target_crit_chance( t );

    if ( is_direct && td( t )->hots.regrowth->is_ticking() )
      tcc += bonus_crit;

    return tcc;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double ctm = druid_heal_t::composite_target_multiplier( t );

    if ( t == player && p()->talent.harmonious_constitution.ok() )
    {
      auto hc_mul = p()->talent.harmonious_constitution->effectN( 1 ).percent();
      if ( p()->specialization() == DRUID_FERAL )
        hc_mul += p()->spec_spell->effectN( 16 ).percent();

      ctm *= 1.0 + hc_mul;
    }

    return ctm;
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    auto pm = druid_heal_t::composite_persistent_multiplier( s );

    if ( p()->buff.soul_of_the_forest_tree->check() )
      pm *= 1.0 + sotf_mul;

    return pm;
  }

  bool check_form_restriction() override
  {
    if ( p()->buff.predatory_swiftness->check() || p()->buff.dream_of_cenarius->check() )
      return true;

    return druid_heal_t::check_form_restriction();
  }

  void execute() override
  {
    is_direct = true;
    druid_heal_t::execute();
    is_direct = false;

    p()->buff.forestwalk->trigger( this );

    if ( target == p() )
      p()->buff.protective_growth->trigger();

    p()->buff.blooming_infusion_damage_counter->trigger( this );
  }

  void last_tick( dot_t* d ) override
  {
    druid_heal_t::last_tick( d );

    if ( d->target == p() )
      p()->buff.protective_growth->expire();
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
    affected_by.soul_of_the_forest = true;

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

// Renewal ==================================================================
struct renewal_t final : public druid_heal_t
{
  double mul;

  DRUID_ABILITY( renewal_t, druid_heal_t, "renewal", p->talent.renewal ),
    mul( find_effect( this, E_HEAL_PCT ).percent() )
  {}

  void execute() override
  {
    base_dd_min = base_dd_max = p()->resources.max[ RESOURCE_HEALTH ] * mul;

    druid_heal_t::execute();
  }
};

// Soothe ===================================================================
struct soothe_t final : public trigger_lethal_preservation_t<druid_heal_t>
{
  DRUID_ABILITY( soothe_t, base_t, "soothe", p->talent.soothe ) {}
};

// Swiftmend ================================================================
struct swiftmend_t final : public druid_heal_t
{
  DRUID_ABILITY( swiftmend_t, druid_heal_t, "swiftmend", p->find_specialization_spell( "Swiftmend" ) ) {}

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
    replace_stats( tick_action );
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

    affected_by.soul_of_the_forest = true;
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

// Overgrowth ===============================================================
// NOTE: this must come last since it requires other spell definitions
struct overgrowth_t final : public druid_heal_t
{
  std::vector<action_t*> spell_list;

  DRUID_ABILITY( overgrowth_t, druid_heal_t, "overgrowth", p->talent.overgrowth )
  {
    get_overgrowth_action<lifebloom_t>( "lifebloom" );
    get_overgrowth_action<rejuvenation_t>( "rejuvenation" );
    get_overgrowth_action<regrowth_t>( "regrowth" )
      ->base_dd_multiplier = 0.0;
    get_overgrowth_action<wild_growth_t>( "wild_growth" )
      ->aoe = 0;
  }

  template <typename T>
  T* get_overgrowth_action( std::string n )
  {
    auto a = p()->get_secondary_action<T>( n + "_overgrowth" );
    a->name_str_reporting = n;
    a->dot_name = n;
    add_child( a );
    spell_list.push_back( a );
    return a;
  }

  void execute() override
  {
    druid_heal_t::execute();

    for ( const auto& a : spell_list )
      a->execute_on_target( target );
  }
};
}  // end namespace heals

template <class Base>
void druid_action_t<Base>::init()
{
  ab::init();

  if ( !ab::harmful && !dynamic_cast<druid_heal_t*>( this ) )
    ab::target = ab::player;

  if ( dbc::is_school( ab::school, SCHOOL_ARCANE ) && p()->buff.lunar_amplification->can_expire( this ) )
  {
    // allow ground_aoe for snapshots
    if ( !ab::background || ab::ground_aoe )
    {
      const auto& eff = p()->buff.lunar_amplification->data().effectN( 1 );
      add_parse_entry( persistent_multiplier_effects )
        .set_buff( p()->buff.lunar_amplification )
        .set_value( eff.percent() )
        .set_eff( &eff );
    }
  }
}

namespace spells
{
struct ap_spender_t : public druid_spell_t
{
protected:
  using base_t = ap_spender_t;

public:
  timespan_t hail_dur = 0_ms;

  ap_spender_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f ) : druid_spell_t( n, p, s, f )
  {
    if ( p->talent.hail_of_stars.ok() )
    {
      if ( has_flag( flag_e::CONVOKE ) )
        hail_dur = 1_s;  // not in spell data, from blue post
      else
        hail_dur = p->talent.hail_of_stars->effectN( 1 ).time_value();
    }
  }

  void consume_resource() override
  {
    druid_spell_t::consume_resource();

    p()->buff.blooming_infusion_heal_counter->trigger( this );

    if ( last_resource_cost <= 0 && hail_dur > 0_ms )
      p()->buff.solstice->trigger( hail_dur );
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.starlord->trigger( this );

    if ( p()->eclipse_handler.in_lunar() )
      p()->buff.harmony_of_the_heavens_lunar->trigger( this );

    if ( p()->eclipse_handler.in_solar() )
      p()->buff.harmony_of_the_heavens_solar->trigger( this );
  }
};

struct ap_generator_t : public druid_spell_t
{
protected:
  using base_t = ap_generator_t;

public:
  double smolder_mul;
  double smolder_pct;
  double touch_pct = 0.0;
  lockable_t<bool> dreamstate;

  ap_generator_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f )
    : druid_spell_t( n, p, s, f ),
      smolder_mul( p->talent.astral_smolder->effectN( 1 ).percent() ),
      smolder_pct( p->talent.astral_smolder->proc_chance() )
  {
    // damage bonus is applied at end of cast, but cast speed bonuses apply before
    parse_effects( p->buff.dreamstate, effect_mask_t( false ).enable( 3 ) );
    parse_effects( &p->buff.dreamstate->data(), effect_mask_t( true ).disable( 3 ), [ this ] { return dreamstate; } );
  }

  void schedule_execute( action_state_t* s ) override
  {
    dreamstate = p()->buff.dreamstate->check();

    druid_spell_t::schedule_execute( s );
  }

  void execute() override
  {
    druid_spell_t::execute();

    if ( ( dreamstate || !has_flag( flag_e::FOREGROUND ) ) && p()->buff.dreamstate->can_expire( this ) )
      p()->buff.dreamstate->decrement();

    dreamstate = false;

    // Dreamstate is triggered after the first harmful cast.
    if ( is_precombat && p()->talent.natures_grace.ok() && !p()->buff.dreamstate->check() )
      p()->buff.dreamstate->trigger();

    if ( p()->is_ptr() && p()->eclipse_handler.in_eclipse() && rng().roll( touch_pct ) )
      p()->buff.touch_the_cosmos->trigger( this );
  }

  void impact( action_state_t* s ) override
  {
    druid_spell_t::impact( s );

    if ( p()->active.astral_smolder && s->result_amount && rng().roll( smolder_pct ) )
    {
      residual_action::trigger( p()->active.astral_smolder, s->target, s->result_amount * smolder_mul );
    }

    if ( p()->buff.dream_burst->check() && p()->buff.dream_burst->can_expire( this ) && s->chain_target == 0 )
    {
      p()->active.dream_burst->execute_on_target( s->target );
      p()->buff.dream_burst->decrement();
    }  
  }

  void reset() override { druid_spell_t::reset(); dreamstate = false; }

  void cancel() override { druid_spell_t::cancel(); dreamstate = false; }

  void interrupt_action() override { druid_spell_t::interrupt_action(); dreamstate = false; }
};

template <eclipse_e E, typename BASE>
struct umbral_embrace_t : public BASE
{
protected:
  using base_t = umbral_embrace_t<E, BASE>;

public:
  struct _umbral_t final : public BASE
  {
    _umbral_t( druid_t* p, std::string_view n, const spell_data_t* s ) : BASE( n, p, s, flag_e::UMBRAL )
    {
      BASE::set_school_override( SCHOOL_ASTRAL );  // preserve original school for lunar amplification/lunation
      BASE::name_str_reporting = "Umbral";

      const spell_data_t* other_ecl;
      dot_t* druid_td_t::dots_t::*other_dot;
      const spell_data_t* other_dmg;
      unsigned other_idx = 0;

      if constexpr ( E == eclipse_e::LUNAR )
      {
        other_ecl = p->spec.eclipse_solar;
        other_dot = &druid_td_t::dots_t::sunfire;
        other_dmg = p->spec.sunfire_dmg;
        other_idx = 3;
      }
      else if constexpr ( E == eclipse_e::SOLAR )
      {
        other_ecl = p->spec.eclipse_lunar;
        other_dot = &druid_td_t::dots_t::moonfire;
        other_dmg = p->spec.moonfire_dmg;
        other_idx = 1;
      }
      else
      {
        static_assert( static_false<BASE>, "Invalid eclipse type for _umbral_t." );
      }

      // Umbral embrace is heavily scripted so we do all the auto parsing within the action itself
      add_parse_entry( BASE::da_multiplier_effects )
        .set_value( find_effect( p->talent.umbral_embrace, p->buff.umbral_embrace ).percent() )
        .set_func( [ this ] { return umbral_embrace_check(); } )
        .set_eff( &p->buff.umbral_embrace->data().effectN( 1 ) );

      // apply bonus from opposite eclipse
      BASE::force_effect( other_ecl, 1, [ this ] { return umbral_embrace_check(); } );

      // apply bonus from opposite dot mastery
      BASE::force_target_effect( [ this, other_dot ]( actor_target_data_t* t ) {
        return umbral_embrace_check() && std::invoke( other_dot, static_cast<druid_td_t*>( t )->dots )->is_ticking();
      }, other_dmg, as<unsigned>( other_dmg->effect_count() ), p->mastery.astral_invocation );

      // apply bonus from opposite passive mastery
      BASE::force_effect( p->mastery.astral_invocation, other_idx, [ this ] { return umbral_embrace_check(); } );
    }

    bool umbral_embrace_check()
    {
      return BASE::p()->buff.umbral_embrace->check() && BASE::p()->eclipse_handler.in_eclipse();
    }

    // the school change effect is handled differently than damage buff effect, and will lead to 'false' casts where the
    // school is astral, but the damage bonus does not apply. for now we can treat this as purely cosmetic as all
    // currently applicable school based damage snapshots on cast.
    bool umbral_embrace_false_check()
    {
      return BASE::p()->buff.umbral_embrace->check() &&
             ( BASE::p()->buff.umbral_embrace->check_value() || BASE::p()->eclipse_handler.in_eclipse() );
    }

    void umbral_embrace_trigger( action_t* a )
    {
      BASE::p()->buff.umbral_embrace->trigger( a, 1, BASE::p()->eclipse_handler.in_eclipse() );
    }
  };

  _umbral_t* umbral = nullptr;
  proc_t* fake_umbral = nullptr;

  umbral_embrace_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f ) : BASE( n, p, s, f )
  {
    if ( p->talent.umbral_embrace.ok() )
    {
      umbral = p->get_secondary_action<_umbral_t>( BASE::name_str + "_umbral", s );
      BASE::add_child( umbral );

      fake_umbral = BASE::p()->get_proc( util::inverse_tokenize( BASE::name_str ) + " (False Astral)" );
    }
  }

  void init() override
  {
    BASE::init();

    // copy generator characteristic to umbral version, specifically for convoke as get_convoke_action will set
    // variables post construction.
    //
    // note that init() is not required as player_t::init_actions() does not use range based for loops, thus actions
    // that are added during initialization (as happens with convoke) will still be init()'d.
    if ( umbral )
    {
      umbral->gain = BASE::gain;
      umbral->proc = BASE::proc;
      umbral->trigger_gcd = BASE::trigger_gcd;
      umbral->action_flags |= BASE::action_flags;
    }
  }

  void execute() override
  {
    if ( umbral )
    {
      if ( umbral->umbral_embrace_check() )
      {
        BASE::p()->last_foreground_action = umbral;
        umbral->time_to_execute = BASE::time_to_execute;
        umbral->dreamstate = BASE::dreamstate;
        umbral->execute_on_target( BASE::target );

        if ( BASE::p()->buff.umbral_embrace->can_expire( this ) )
        {
          BASE::p()->buff.umbral_embrace->expire( this );
          BASE::p()->buff.umbral_inspiration->trigger();
        }

        umbral->umbral_embrace_trigger( this );
        return;
      }
      else if ( umbral->umbral_embrace_false_check() )
      {
        fake_umbral->occur();
      }

      BASE::execute();

      umbral->umbral_embrace_trigger( this );
    }
    else
    {
      BASE::execute();
    }
  }
};

// Astral Smolder ===========================================================
struct astral_smolder_t
  : public residual_action::residual_periodic_action_t<trigger_waning_twilight_t<druid_spell_t>>
{
  uptime_t* uptime;

  astral_smolder_t( druid_t* p )
    : residual_action_t( "astral_smolder", p, p->find_spell( 394061 ) ),
      uptime( p->get_uptime( "Astral Smolder" )->collect_uptime( *p->sim ) )
  {
    proc = true;

    // eclipse snapshot script seems to be overriding all damage modifications including standard whitelists
    if ( p->bugs )
    {
      da_multiplier_effects.clear();
      ta_multiplier_effects.clear();
      target_multiplier_effects.clear();
      persistent_multiplier_effects.clear();
    }

    // double dips and snapshots eclipse via script
    add_parse_entry( persistent_multiplier_effects )
      .set_buff( p->buff.eclipse_lunar )
      .set_type( USE_CURRENT )
      .set_eff( &p->spec.eclipse_lunar->effectN( 7 ) );

    add_parse_entry( persistent_multiplier_effects )
      .set_buff( p->buff.eclipse_solar )
      .set_type( USE_CURRENT )
      .set_eff( &p->spec.eclipse_solar->effectN( 8 ) );
  }

  void trigger_dot( action_state_t* s ) override
  {
    residual_action_t::trigger_dot( s );

    uptime->update( true, sim->current_time() );
  }

  void last_tick( dot_t* d ) override
  {
    residual_action_t::last_tick( d );

    uptime->update( false, sim->current_time() );
  }
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
      replace_stats( brambles );

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

    // TODO: do this manually until we can get redirection working on apply_affecting_aura
    if ( p->talent.whirling_stars.ok() )
    {
      apply_affecting_aura( p->talent.whirling_stars );

      if ( p->talent.potent_enchantments.ok() )
      {
        auto mod = find_effect( p->talent.potent_enchantments, p->talent.whirling_stars ).time_value();
        cooldown->duration += mod;
        sim->print_debug( "{} cooldown recharge time modified by {}", *this, mod );
      }
    }
  }

  void execute() override
  {
    base_t::execute();

    buff->trigger();

    p()->buff.harmony_of_the_heavens_lunar->expire();
    p()->buff.harmony_of_the_heavens_solar->expire();
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
  double gcd_mul;

  DRUID_ABILITY( dash_t, druid_spell_t, "dash",
                 p->talent.tiger_dash.ok() ? p->talent.tiger_dash : p->find_class_spell( "Dash" ) ),
    buff_on_cast( p->talent.tiger_dash.ok() ? p->buff.tiger_dash : p->buff.dash ),
    gcd_mul( find_effect( p->buff.cat_form, this, A_ADD_PCT_MODIFIER, P_GCD ).percent() )
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
    background = true;
    aoe = -1;
    reduced_aoe_targets = data().effectN( 2 ).base_value();
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

    if ( p()->buff.cat_form->check() )
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
  unsigned num;
  unsigned dream_surge_num = 0;

  DRUID_ABILITY( force_of_nature_t, base_t, "force_of_nature", p->talent.force_of_nature ),
    num( as<unsigned>( p->talent.force_of_nature->effectN( 1 ).base_value() ) )
  {
    harmful = false;

    if ( data().ok() )
    {
      p->pets.force_of_nature.set_default_duration( find_trigger( p->talent.force_of_nature ).trigger()->duration() + 1_ms );
      p->pets.force_of_nature.set_creation_event_callback( pets::parent_pet_action_fn( this ) );

      if ( p->active.treants_of_the_moon_mf )
        add_child( p->active.treants_of_the_moon_mf );

      if ( p->active.dream_burst )
      {
        add_child( p->active.dream_burst );

        auto m_data = p->get_modified_spell( p->talent.dream_surge )
          ->parse_effects( p->talent.power_of_the_dream );

        dream_surge_num = as<unsigned>( m_data->effectN( 1 ).base_value() );
      }
    }
  }

  void execute() override
  {
    base_t::execute();

    p()->pets.force_of_nature.spawn( num );
    p()->buff.dream_burst->trigger( dream_surge_num );
  }
};

// Fury of Elune =========================================================
struct fury_of_elune_t final : public druid_spell_t
{
  struct fury_of_elune_tick_t final : public trigger_atmospheric_exposure_t<druid_spell_t>
  {
    fury_of_elune_tick_t( druid_t* p, std::string_view n, const spell_data_t* s, flag_e f ) : base_t( n, p, s, f )
    {
      background = dual = ground_aoe = true;
      aoe = -1;
      reduced_aoe_targets = 1.0;
      full_amount_targets = 1;
    }

    void impact( action_state_t* s ) override
    {
      base_t::impact( s );

      p()->eclipse_handler.tick_fury_of_elune();
    }
  };

  struct boundless_moonlight_t final : public druid_spell_t
  {
    boundless_moonlight_t( druid_t* p, std::string_view n, flag_e f )
      : druid_spell_t( n, p, p->find_spell( 428682 ), f )
    {
      background = true;
      aoe = -1;  // TODO: aoe DR?
      name_str_reporting = "boundless_moonlight";

      if ( p->talent.the_eternal_moon.ok() )
      {
        auto power = p->specialization() == DRUID_GUARDIAN ? POWER_RAGE : POWER_ASTRAL_POWER;
        const auto& eff = find_effect( this, E_ENERGIZE, A_MAX, power );

        energize_amount = eff.resource();
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

  DRUID_ABILITY_C( fury_of_elune_t, druid_spell_t, "fury_of_elune", p->talent.fury_of_elune,
                   const spell_data_t* sd = nullptr, buff_t* b = nullptr ),
    tick_spell( sd ? sd : p->find_spell( 211545 ) ),
    energize_buff( b ? b : p->buff.fury_of_elune ),
    tick_period( find_effect( energize_buff, A_PERIODIC_ENERGIZE, RESOURCE_ASTRAL_POWER ).period() )
  {
    track_cd_waste = true;

    form_mask |= NO_FORM; // can be cast without form
    dot_duration = 0_ms;  // AP gain handled via buffs

    if ( data().ok() )
    {
      damage = p->get_secondary_action<fury_of_elune_tick_t>( name_str + "_tick", tick_spell, f );
      replace_stats( damage );

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
    }
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

// Heart of the Wild ========================================================
struct heart_of_the_wild_t final : public druid_spell_t
{
  DRUID_ABILITY( heart_of_the_wild_t, druid_spell_t, "heart_of_the_wild", p->talent.heart_of_the_wild )
  {
    harmful = may_miss = reset_melee_swing = false;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.heart_of_the_wild->trigger();
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

// Mark of the Wild =========================================================
struct mark_of_the_wild_t final : public druid_spell_t
{
  DRUID_ABILITY( mark_of_the_wild_t, druid_spell_t, "mark_of_the_wild", p->find_class_spell( "Mark of the Wild" ) )
  {
    harmful = false;
    ignore_false_positive = true;

    if ( sim->overrides.mark_of_the_wild )
      background = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    if ( !sim->overrides.mark_of_the_wild )
      sim->auras.mark_of_the_wild->trigger();
  }
};

// Moon Spells ==============================================================
struct moon_base_t : public druid_spell_t
{
  struct minor_moon_t final : public druid_spell_t
  {
    minor_moon_t( druid_t* p, std::string_view n, flag_e f ) : druid_spell_t( n, p, p->find_spell( 424588 ), f )
    {
      background = true;
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
    if ( data().ok() && p->talent.boundless_moonlight.ok() && p->active.moons )
      minor = p->get_secondary_action<minor_moon_t>( "minor_moon", f );
  }

  void init() override
  {
    if ( has_flag( flag_e::FOREGROUND ) )
    {
      cooldown = p()->cooldown.moon_cd;
      track_cd_waste = true;
    }

    druid_spell_t::init();

    if ( !is_free() && data().ok() )
    {
      if ( cooldown->duration != 0_ms && cooldown->duration != data().charge_cooldown() )
      {
        p()->sim->error( "Moon CD: {} ({}) cooldown of {} doesn't match existing cooldown of {}", name_str, data().id(),
                         data().charge_cooldown(), cooldown->duration );
      }
      cooldown->duration = data().charge_cooldown();

      if ( cooldown->charges != 1 && cooldown->charges != as<int>( data().charges() ) )
      {
        p()->sim->error( "Moon CD: {} ({}) charges of {} doesn't match existing charges of {}", name_str, data().id(),
                         data().charges(), cooldown->charges );
      }
      cooldown->charges = data().charges();

      p()->active.moons->add_child( this );
    }
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

    p()->eclipse_handler.cast_moon( stage );

    // TODO: any delay/stagger?
    if ( minor && num_minor )
      for ( unsigned i = 0; i < num_minor; i++ )
        minor->execute_on_target( target );

    if ( proc )
    {
      if ( p()->moon_stage == moon_stage_e::MAX_MOON && p()->orbital_bug && p()->bugs )
        p()->orbital_bug = false;
      else
        return;
    }

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
      energize_type = action_energize::NONE;

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

    if ( p()->moon_stage == moon_stage_e::MAX_MOON )
      p()->orbital_bug = false;

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
  struct moonfire_damage_t final : public trigger_guardians_tenacity_t<
                                            trigger_gore_t<
                                              use_dot_list_t<
                                                trigger_waning_twilight_t<druid_spell_t>>>>
  {
    real_ppm_t* light_of_elune_rng = nullptr;

    moonfire_damage_t( druid_t* p, std::string_view n, flag_e f ) : base_t( n, p, p->spec.moonfire_dmg, f )
    {
      may_miss = false;
      dual = background = true;

      dot_name = "moonfire";
      dot_list = &p->dot_lists.moonfire;

      if ( p->talent.galactic_guardian.ok() && !has_flag( flag_e::GALACTIC ) )
      {
        const auto& eff = p->buff.galactic_guardian->data().effectN( 3 );
        add_parse_entry( da_multiplier_effects )
          .set_buff( p->buff.galactic_guardian )
          .set_value( eff.percent() )
          .set_eff( &eff );
      }

      if ( p->talent.the_light_of_elune.ok() )
      {
        light_of_elune_rng = p->get_rppm( "The Light of Elune", p->talent.the_light_of_elune );
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
    replace_stats( damage );

    auto m_data = p->get_modified_spell( &data() )
      ->parse_effects( p->spec.astral_power )
      ->parse_effects( p->talent.moon_guardian )
      ->parse_effects( p->buff.galactic_guardian );

    if ( p->spec.astral_power->ok() && !has_flag( flag_e::TWIN ) )
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

    if ( ( p->talent.twin_moonfire.ok() || p->talent.twin_moons.ok() ) && !has_flag( flag_e::TWIN ) )
    {
      twin = p->get_secondary_action<moonfire_t>( name_str + "_twin", flag_e::TWIN );
      twin->background = twin->dual = twin->proc = true;
      replace_stats( twin->damage, false );
      twin->stats = stats;

      // radius is only found in balance twin moons talent data
      twin_range = p->find_spell( 279620 )->effectN( 1 ).base_value();
    }
  }

  // needed to allow on-cast procs
  bool has_amount_result() const override { return damage->has_amount_result(); }

  void execute() override
  {
    druid_spell_t::execute();

    auto state = damage->get_state();
    damage->target = state->target = target;
    damage->snapshot_state( state, result_amount_type::DMG_DIRECT );
    damage->schedule_execute( state );

    if ( twin )
    {
      const auto& tl = target_list();
      if ( auto twin_target = p()->get_smart_target( tl, &druid_td_t::dots_t::moonfire, target, twin_range ) )
        twin->execute_on_target( twin_target );
    }

    p()->buff.galactic_guardian->expire( this );
  }
};

// Nature's Vigil =============================================================
template <typename Base>
struct natures_vigil_tick_t final : public Base
{
  natures_vigil_tick_t( druid_t* p )
    : Base( "natures_vigil_tick", p, p->find_spell( p->specialization() == DRUID_RESTORATION ? 124988 : 124991 ) )
  {
    Base::dual = Base::proc = true;
  }

  player_t* _get_target( buff_t* b )
  {
    if constexpr ( std::is_base_of_v<druid_spell_t, Base> )
      return b->player->target;
    else
      return b->player;
  }
};

template <typename Base>
struct natures_vigil_t final : public Base
{

  DRUID_ABILITY( natures_vigil_t, Base, "natures_vigil", p->talent.natures_vigil )
  {
    Base::dot_duration = 0_ms;
    Base::reset_melee_swing = false;

    if ( p->talent.natures_vigil.ok() )
    {
      auto tick = p->get_secondary_action<natures_vigil_tick_t<druid_residual_action_t<Base>>>( "natures_vigil_tick" );
      Base::replace_stats( tick );

      p->buff.natures_vigil->set_tick_callback( [ tick ]( buff_t* b, int, timespan_t ) {
        if ( b->check_value() )
        {
          tick->execute_on_target( tick->_get_target( b ), b->check_value() );
          b->current_value = 0;
        }
      } );
    }
  }

  void execute() override
  {
    Base::execute();

    Base::p()->buff.natures_vigil->trigger();
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

// Proxy Swipe Spell ========================================================
struct swipe_proxy_t final : public druid_spell_t
{
  cat_attacks::swipe_cat_t* swipe_cat;
  bear_attacks::swipe_bear_t* swipe_bear;

  swipe_proxy_t( druid_t* p )
    : druid_spell_t( "swipe", p, p->spec.swipe ),
      swipe_cat( new cat_attacks::swipe_cat_t( p ) ),
      swipe_bear( new bear_attacks::swipe_bear_t( p ) )
  {
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
    if ( p()->buff.cat_form->check() )
      return swipe_cat->gcd();
    else if ( p()->buff.bear_form->check() )
      return swipe_bear->gcd();

    return druid_spell_t::gcd();
  }

  void execute() override
  {
    if ( p()->buff.cat_form->check() )
      swipe_cat->execute();
    else if ( p()->buff.bear_form->check() )
      swipe_bear->execute();

    if ( pre_execute_state )
      action_state_t::release( pre_execute_state );
  }

  bool action_ready() override
  {
    if ( p()->buff.cat_form->check() )
      return swipe_cat->action_ready();
    else if ( p()->buff.bear_form->check() )
      return swipe_bear->action_ready();

    return false;
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( p()->buff.cat_form->check() )
      return swipe_cat->target_ready( candidate_target );
    else if ( p()->buff.bear_form->check() )
      return swipe_bear->target_ready( candidate_target );

    return false;
  }

  bool ready() override
  {
    if ( p()->buff.cat_form->check() )
      return swipe_cat->ready();
    else if ( p()->buff.bear_form->check() )
      return swipe_bear->ready();

    return false;
  }

  double cost() const override
  {
    if ( p()->buff.cat_form->check() )
      return swipe_cat->cost();
    else if ( p()->buff.bear_form->check() )
      return swipe_bear->cost();

    return 0;
  }
};

// Proxy Thrash =============================================================
struct thrash_proxy_t final : public druid_spell_t
{
  bear_attacks::thrash_bear_t* thrash_bear;
  cat_attacks::thrash_cat_t* thrash_cat;

  thrash_proxy_t( druid_t* p )
    : druid_spell_t( "thrash", p, p->talent.thrash ),
      thrash_bear( new bear_attacks::thrash_bear_t( p ) ),
      thrash_cat( new cat_attacks::thrash_cat_t( p ) )
  {
    // At the moment, both versions of these spells are the same (and only the cat version has a talent that changes
    // this) so we use this spell data here. If this changes we will have to think of something more robust. These two
    // are important for the "ticks_gained_on_refresh" expression to work
    dot_duration = thrash_cat->dot_duration;
    base_tick_time = thrash_cat->base_tick_time;

    set_school( p->talent.lunar_calling.ok() ? SCHOOL_ARCANE : SCHOOL_PHYSICAL );

    thrash_bear->action_flags |= flag_e::FOREGROUND;
    thrash_cat->action_flags |= flag_e::FOREGROUND;
  }

  void parse_options( util::string_view opt ) override
  {
    druid_spell_t::parse_options( opt );

    thrash_bear->parse_options( opt );
    thrash_cat->parse_options( opt );
  }

  timespan_t gcd() const override
  {
    if ( p()->buff.cat_form->check() )
      return thrash_cat->gcd();
    else if ( p()->buff.bear_form->check() )
      return thrash_bear->gcd();

    return druid_spell_t::gcd();
  }

  void execute() override
  {
    if ( p()->buff.cat_form->check() )
      thrash_cat->execute();
    else if ( p()->buff.bear_form->check() )
      thrash_bear->execute();

    if ( pre_execute_state )
      action_state_t::release( pre_execute_state );
  }

  bool action_ready() override
  {
    if ( p()->buff.cat_form->check() )
      return thrash_cat->action_ready();
    else if ( p()->buff.bear_form->check() )
      return thrash_bear->action_ready();

    return false;
  }

  dot_t* get_dot( player_t* t ) override
  {
    if ( p()->buff.cat_form->check() )
      return thrash_cat->get_dot( t );
    else if ( p()->buff.bear_form->check() )
      return thrash_bear->get_dot( t );

    return nullptr;
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( p()->buff.cat_form->check() )
      return thrash_cat->target_ready( candidate_target );
    else if ( p()->buff.bear_form->check() )
      return thrash_bear->target_ready( candidate_target );

    return false;
  }

  bool ready() override
  {
    if ( p()->buff.cat_form->check() )
      return thrash_cat->ready();
    else if ( p()->buff.bear_form->check() )
      return thrash_bear->ready();

    return false;
  }

  double cost() const override
  {
    if ( p()->buff.cat_form->check() )
      return thrash_cat->cost();
    else if ( p()->buff.bear_form->check() )
      return thrash_bear->cost();

    return 0;
  }
};

// Shooting Stars ===========================================================
struct shooting_stars_t final : public druid_spell_t
{
  shooting_stars_t( druid_t* p, std::string_view n, const spell_data_t* s ) : druid_spell_t( n, p, s )
  {
    background = proc = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.orbit_breaker->trigger();

    if ( p()->buff.orbit_breaker->at_max_stacks() && p()->active.orbit_breaker )
    {
      p()->active.orbit_breaker->execute_on_target( target );
      p()->buff.orbit_breaker->expire();
    }
  }
};

// Skull Bash ===============================================================
struct skull_bash_t final : public druid_interrupt_t
{
  DRUID_ABILITY( skull_bash_t, druid_interrupt_t, "skull_bash", p->talent.skull_bash ) {}
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
    starfall_damage_t( druid_t* p, std::string_view n, flag_e f ) : druid_spell_t( n, p, p->find_spell( 191037 ), f )
    {
      background = dual = true;

      if ( !p->buff.lunar_amplification->is_fallback )
      {
        range::erase_remove( persistent_multiplier_effects, [ & ]( const auto& e ) {
          return e.buff == p->buff.lunar_amplification;
        } );

        const auto& eff = p->buff.lunar_amplification->data().effectN( 1 );
        add_parse_entry( da_multiplier_effects )
          .set_buff( p->buff.lunar_amplification_starfall )
          .set_value( eff.percent() )
          .set_eff( &eff );
      }
    }

    double action_multiplier() const override
    {
      return druid_spell_t::action_multiplier() * std::max( 1.0, as<double>( p()->buff.starfall->check() ) );
    }
  };

  struct starfall_driver_t final : public druid_spell_t
  {
    starfall_damage_t* damage;

    starfall_driver_t( druid_t* p, std::string_view n, flag_e f )
      : druid_spell_t( n, p, find_trigger( p->buff.starfall ).trigger(), f )
    {
      background = dual = true;

      auto pre = name_str.substr( 0, name_str.find_last_of( '_' ) );
      damage = p->get_secondary_action<starfall_damage_t>( pre + "_damage", f );
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

      damage->snapshot_and_execute( s, false, []( const action_state_t* from, action_state_t* to ) {
        to->copy_state( from );
      } );
    }
  };

  starfall_driver_t* driver;
  timespan_t dot_ext = 0_ms;
  timespan_t max_ext = 0_ms;

  DRUID_ABILITY( starfall_t, base_t, "starfall", p->spec.starfall ),
    dot_ext( timespan_t::from_seconds( p->talent.aetherial_kindling->effectN( 1 ).base_value() ) ),
    max_ext( timespan_t::from_seconds( p->talent.aetherial_kindling->effectN( 2 ).base_value() ) )
  {
    may_miss = false;
    queue_failed_proc = p->get_proc( "starfall queue failed" );
    dot_duration = 0_ms;
    form_mask |= NO_FORM;

    if ( data().ok() )
    {
      driver = p->get_secondary_action<starfall_driver_t>( name_str + "_driver", f );
      assert( driver->damage );
      replace_stats( driver, false );
      replace_stats( driver->damage );

      if ( p->talent.aetherial_kindling.ok() )
      {
        auto m_data = p->get_modified_spell( p->talent.aetherial_kindling )
          ->parse_effects( p->talent.circle_of_life_and_death_owl );

        dot_ext = timespan_t::from_seconds( m_data->effectN( 1 ).base_value() );
        max_ext = timespan_t::from_seconds( m_data->effectN( 2 ).base_value() );
      }
    }
  }

  void execute() override
  {
    if ( p()->talent.aetherial_kindling.ok() )
    {
      range::for_each( p()->dot_lists.moonfire, [ this ]( dot_t* d ) { d->adjust_duration( dot_ext, max_ext ); } );
      range::for_each( p()->dot_lists.sunfire, [ this ]( dot_t* d ) { d->adjust_duration( dot_ext, max_ext ); } );
    }

    base_t::execute();

    if ( p()->buff.starweaver_starfall->check() )
      p()->buff.starweaver_starfall->expire( this );
    else if ( p()->buff.touch_the_cosmos_starfall->check() )
      p()->buff.touch_the_cosmos_starfall->expire( this );
    else if ( p()->buff.touch_the_cosmos->check() )
      p()->buff.touch_the_cosmos->expire( this );
    else
      p()->buff.astral_communion->expire( this );

    p()->buff.starfall->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
      driver->execute();
      p()->eclipse_handler.tick_starfall();
    } );

    p()->buff.starfall->trigger();
    p()->buff.starweaver_starsurge->trigger( this );
  }
};

// Starfire =============================================================
struct starfire_base_t : public use_fluid_form_t<DRUID_BALANCE, ap_generator_t>
{
  const modified_spelleffect_t* aoe_eff;
  double smolder_mul;
  double sotf_mul;
  unsigned sotf_cap;

  starfire_base_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f )
    : base_t( n, p, s, f ),
    smolder_mul( p->talent.astral_smolder->effectN( 1 ).percent() ),
    sotf_mul( p->talent.soul_of_the_forest_moonkin->effectN( 2 ).percent() ),
    sotf_cap( as<unsigned>( p->talent.soul_of_the_forest_moonkin->effectN( 3 ).base_value() ) )
  {
    aoe = -1;
    reduced_aoe_targets = data().effectN( p->specialization() == DRUID_BALANCE ? 5 : 3 ).base_value();

    auto m_data = p->get_modified_spell( &data() )
      ->parse_effects( p->talent.wild_surges )
      ->parse_effects( p->buff.eclipse_lunar, p->talent.umbral_intensity )
      ->parse_effects( p->talent.moon_guardian );

    set_energize( m_data );

    if ( energize && p->sets->has_set_bonus( DRUID_BALANCE, TWW1, B4 ) )
    {
      if ( const auto& eff = p->sets->set( DRUID_BALANCE, TWW1, B4 )->effectN( 2 ); !energize->modified_by( eff ) )
      {
        energize->add_parse_entry()
          .set_buff( p->buff.eclipse_lunar )
          .set_flat( true )
          .set_value( eff.base_value() )
          .set_eff( &eff );

        energize->add_parse_entry()
          .set_buff( p->buff.eclipse_solar )
          .set_flat( true )
          .set_value( eff.base_value() )
          .set_eff( &eff );
      }

      touch_pct = p->talent.touch_the_cosmos->effectN( 2 ).percent();
    }

    // parse this last as it's percent bonus
    m_data->parse_effects( p->buff.warrior_of_elune );

    if ( p->specialization() == DRUID_BALANCE )
      aoe_eff = &m_data->effectN( 3 );
    else
      aoe_eff = &m_data->effectN( 2 );

    if ( p->talent.master_shapeshifter.ok() )
    {
      const auto& eff = p->talent.master_shapeshifter->effectN( 2 );
      add_parse_entry( da_multiplier_effects )
        .set_func( [ p = p ] { return p->get_form() == NO_FORM; } )
        .set_value( eff.percent() )
        .set_eff( &eff );
    }
  }

  void init() override
  {
    base_t::init();

    // for precombat we hack it to manually energize 100ms later to get around AP capping on combat start
    if ( is_precombat && energize_resource_() == RESOURCE_ASTRAL_POWER )
      energize_type = action_energize::NONE;
  }

  double sotf_multiplier( const action_state_t* s ) const
  {
    auto mul = 1.0;

    if ( sotf_mul && p()->eclipse_handler.in_lunar() && s->n_targets > 1 )
      mul += sotf_mul * std::min( sotf_cap, s->n_targets - 1 );

    return mul;
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    return base_t::composite_energize_amount( s ) * sotf_multiplier( s );
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    return base_t::composite_da_multiplier( s ) * sotf_multiplier( s );
  }

  void execute() override
  {
    base_t::execute();

    if ( p()->buff.owlkin_frenzy->up() )
      p()->buff.owlkin_frenzy->expire();
    else if ( p()->buff.warrior_of_elune->up() )
      p()->buff.warrior_of_elune->decrement();

    if ( p()->eclipse_handler.in_eclipse() && !p()->is_ptr() )
      p()->buff.touch_the_cosmos_starfall->trigger( this );
  }

  void schedule_travel( action_state_t* s ) override
  {
    // for precombat we hack it to advance eclipse and manually energize 100ms later to get around the eclipse stack
    // reset & AP capping on combat start
    if ( is_precombat && energize_resource_() == RESOURCE_ASTRAL_POWER )
    {
      base_t::schedule_travel( s );

      make_event( *sim, 100_ms, [ this ]() {
        p()->eclipse_handler.cast_starfire();
        p()->resource_gain( RESOURCE_ASTRAL_POWER, composite_energize_amount( execute_state ),
                            energize_gain( execute_state ) );
      } );

      return;
    }

    // eclipse is handled after cast but before impact
    if ( s->chain_target == 0 )
      p()->eclipse_handler.cast_starfire();

    base_t::schedule_travel( s );
  }

  // TODO: we do this all in composite_aoe_multiplier() as base_aoe_multiplier is not a virtual function. If necessary
  // in the future, base_aoe_multiplier may need to be made into one.
  double composite_aoe_multiplier( const action_state_t* s ) const override
  {
    double cam = base_t::composite_aoe_multiplier( s );

    if ( s->chain_target )
      cam *= aoe_eff->percent();

    return cam;
  }
};

struct starfire_t final : public umbral_embrace_t<eclipse_e::LUNAR, starfire_base_t>
{
  DRUID_ABILITY( starfire_t, base_t, "starfire", p->talent.starfire ) {}
};

// Starsurge Spell ==========================================================
struct starsurge_offspec_t final : public trigger_call_of_the_elder_druid_t<druid_spell_t>
{
  DRUID_ABILITY( starsurge_offspec_t, base_t, "starsurge", p->talent.starsurge )
  {
    form_mask = NO_FORM;
    base_costs[ RESOURCE_MANA ] = 0.0;  // so we don't need to enable mana regen

    if ( p->talent.master_shapeshifter.ok() )
    {
      const auto& eff = p->talent.master_shapeshifter->effectN( 2 );
      add_parse_entry( da_multiplier_effects )
        .set_func( [ p = p ] { return p->get_form() == NO_FORM; } )
        .set_value( eff.percent() )
        .set_eff( &eff );
    }
  }
};

struct starsurge_t final : public ap_spender_t
{
  struct goldrinns_fang_t final : public druid_spell_t
  {
    goldrinns_fang_t( druid_t* p, std::string_view n, flag_e f )
      : druid_spell_t( n, p, find_trigger( p->talent.power_of_goldrinn ).trigger(), f )
    {
      background = true;
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

    if ( p()->buff.starweaver_starsurge->check() )
      p()->buff.starweaver_starsurge->expire( this );
    else if ( p()->buff.touch_the_cosmos_starsurge->check() )
      p()->buff.touch_the_cosmos_starsurge->expire( this );
    else
      p()->buff.astral_communion->expire( this );

    if ( goldrinn && rng().roll( p()->talent.power_of_goldrinn->proc_chance() ) )
      goldrinn->execute_on_target( target );

    p()->buff.starweaver_starfall->trigger( this );

    p()->eclipse_handler.cast_starsurge();
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( p()->talent.stellar_amplification.ok() )
      td( s->target )->debuff.stellar_amplification->trigger( this );
  }
};

// Stellar Flare ============================================================
struct stellar_flare_t final : public trigger_waning_twilight_t<druid_spell_t>
{
  DRUID_ABILITY( stellar_flare_t, base_t, "stellar_flare", p->talent.stellar_flare )
  {
    dot_name = "stellar_flare";
  }
};

// Sunfire Spell ============================================================
struct sunfire_t final : public druid_spell_t
{
  struct sunfire_damage_t final : public use_dot_list_t<trigger_waning_twilight_t<druid_spell_t>>
  {
    real_ppm_t* shroom_rng = nullptr;

    sunfire_damage_t( druid_t* p, flag_e f ) : base_t( "sunfire_dmg", p, p->spec.sunfire_dmg, f )
    {
      dual = background = true;
      aoe = p->talent.improved_sunfire.ok() ? -1 : 0;
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
    auto m_data = p->get_modified_spell( &data() )
      ->parse_effects( p->spec.astral_power );

    set_energize( m_data );

    if ( data().ok() )
    {
      damage = p->get_secondary_action<sunfire_damage_t>( "sunfire_dmg", f );
      replace_stats( damage );
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
  action_t* flare;

  orbital_strike_t( druid_t* p ) : druid_spell_t( "orbital_strike", p, p->find_spell( 361237 ) )
  {
    proc = true;
    aoe = -1;
    travel_speed = 75.0;  // guesstimate

    flare = p->get_secondary_action<stellar_flare_t>( "stellar_flare_orbital_strike", p->find_spell( 202347 ) );
    flare->name_str_reporting = "stellar_flare";
    flare->dot_duration += p->talent.potent_enchantments->effectN( 1 ).time_value();
    add_child( flare );
  }

  void impact( action_state_t* s ) override
  {
    flare->execute_on_target( s->target );  // flare is applied before impact damage

    druid_spell_t::impact( s );
  }
};

// Warrior of Elune =========================================================
struct warrior_of_elune_t final : public druid_spell_t
{
  DRUID_ABILITY( warrior_of_elune_t, druid_spell_t, "warrior_of_elune", p->talent.warrior_of_elune )
  {
    harmful = may_miss = false;
    track_cd_waste = true;

    form_mask |= NO_FORM;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.warrior_of_elune->trigger();
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

  void schedule_execute( action_state_t* execute_state ) override
  {
    druid_spell_t::schedule_execute( execute_state );

    // Since Cat/Bear charge is limited to moving towards a target, cancel form if the druid wants to move away. Other
    // forms can already move in any direction they want so they're fine.
    if ( p()->current.movement_direction == movement_direction_type::AWAY )
      p()->shapeshift( NO_FORM );
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
    if ( p()->talent.elunes_grace.ok() && ( p()->get_form() == BEAR_FORM || p()->get_form() == MOONKIN_FORM ) )
      cd = cooldown_duration() - 3_s;

    druid_spell_t::update_ready( cd );
  }
};

// Wild Mushroom ============================================================
struct wild_mushroom_t final : public druid_spell_t
{
  struct fungal_growth_t final : public trigger_waning_twilight_t<druid_spell_t>
  {
    fungal_growth_t( druid_t* p, std::string_view n, flag_e f ) : base_t( n, p, p->find_spell( 81281 ), f )
    {
      name_str_reporting = "fungal_growth";
    }
  };

  struct wild_mushroom_damage_t final : public druid_spell_t
  {
    action_t* fungal = nullptr;
    double ap_max;

    wild_mushroom_damage_t( druid_t* p, std::string_view n, const spell_data_t* s, flag_e f )
      : druid_spell_t( n, p, s, f ), ap_max( data().effectN( 2 ).base_value() )
    {
      background = dual = true;
      aoe = -1;
    }

    double ap_gain() const
    {
      return std::min( ap_max, ( ( p()->is_ptr() ? 1 : 0 ) + num_targets_hit ) * 5.0 );

    }

    void execute() override
    {
      druid_spell_t::execute();

      gain_energize_resource( RESOURCE_ASTRAL_POWER, ap_gain(), gain );
    }

    void impact( action_state_t* s ) override
    {
      druid_spell_t::impact( s );

      fungal->execute_on_target( s->target );
    }
  };

  ground_aoe_params_t params;
  wild_mushroom_damage_t* damage = nullptr;
  timespan_t delay;

  DRUID_ABILITY( wild_mushroom_t, druid_spell_t, "wild_mushroom", p->talent.wild_mushroom ),
    delay( timespan_t::from_millis( find_trigger( this ).misc_value1() ) )
  {
    harmful = false;

    if ( data().ok() )
    {
      damage =
        p->get_secondary_action<wild_mushroom_damage_t>( name_str + "_damage", find_trigger( &data() ).trigger(), f );
      replace_stats( damage );
      damage->gain = gain;

      damage->fungal = p->get_secondary_action<fungal_growth_t>( name_str + "_fungal", f );
      add_child( damage->fungal );

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
};

// Wrath ====================================================================
struct wrath_base_t : public use_fluid_form_t<DRUID_BALANCE, ap_generator_t>
{
  double smolder_mul;
  unsigned count = 0;

  wrath_base_t( std::string_view n, druid_t* p, const spell_data_t* s, flag_e f )
    : base_t( n, p, s, f ), smolder_mul( p->talent.astral_smolder->effectN( 1 ).percent() )
  {
    form_mask = NO_FORM | MOONKIN_FORM;

    auto m_data = p->get_modified_spell( &data() )
      ->parse_effects( p->spec.astral_power )
      ->parse_effects( p->talent.wild_surges );

    set_energize( m_data );

    if ( energize && p->sets->has_set_bonus( DRUID_BALANCE, TWW1, B4 ) )
    {
      if ( const auto& eff = p->sets->set( DRUID_BALANCE, TWW1, B4 )->effectN( 1 ); !energize->modified_by( eff ) )
      {
        energize->add_parse_entry()
         .set_buff( p->buff.eclipse_lunar )
         .set_flat( true )
         .set_value( eff.base_value() )
         .set_eff( &eff );

        energize->add_parse_entry()
         .set_buff( p->buff.eclipse_solar )
         .set_flat( true )
         .set_value( eff.base_value() )
         .set_eff( &eff );
      }
    }

    // parse this last as it's percent bonus
    m_data->parse_effects( p->buff.eclipse_solar, p->talent.soul_of_the_forest_moonkin );

    if ( p->talent.master_shapeshifter.ok() )
    {
      const auto& eff = p->talent.master_shapeshifter->effectN( 2 );
      add_parse_entry( da_multiplier_effects )
        .set_func( [ p = p ] { return p->get_form() == NO_FORM; } )
        .set_value( eff.percent() )
        .set_eff( &eff );
    }

    touch_pct = p->talent.touch_the_cosmos->effectN( 1 ).percent();
  }

  timespan_t travel_time() const override
  {
    if ( !count )
      return base_t::travel_time();

    // for each additional wrath in precombat apl, reduce the travel time by the cast time
    player->invalidate_cache( CACHE_SPELL_HASTE );
    return std::max( 1_ms, base_t::travel_time() - base_execute_time * composite_haste() * count );
  }

  // return false on invulnerable targets as this action is !harmful to allow for self-healing, thus will pass the
  // invulnerable check in action_t::target_ready()
  bool target_ready( player_t* t ) override
  {
    if ( t->debuffs.invulnerable->check() && sim->ignore_invulnerable_targets )
      return false;

    return base_t::target_ready( t );
  }

  void execute() override
  {
    base_t::execute();

    //if ( p()->eclipse_handler.in_eclipse() && !p()->is_ptr() )
    if ( p()->eclipse_handler.in_eclipse() )
      p()->buff.touch_the_cosmos_starsurge->trigger( this );
  }

  void schedule_travel( action_state_t* s ) override
  {
    // in druid_t::init_finished(), we set the final wrath of the precombat to have energize type of NONE, so that
    // we can handle the delayed enerigze & eclipse stack triggering here.
    if ( is_precombat && energize_resource_() == RESOURCE_ASTRAL_POWER && energize_type == action_energize::NONE )
    {
      base_t::schedule_travel( s );

      make_event( *sim, 100_ms, [ this ]() {
        p()->eclipse_handler.cast_wrath();
        p()->resource_gain( RESOURCE_ASTRAL_POWER, composite_energize_amount( execute_state ),
                            energize_gain( execute_state ) );
      } );

      return;
    }

    // eclipse is handled after cast but before travel
    p()->eclipse_handler.cast_wrath();

    base_t::schedule_travel( s );
  }
};

struct wrath_t final : public umbral_embrace_t<eclipse_e::SOLAR, wrath_base_t>
{
  DRUID_ABILITY( wrath_t, base_t, "wrath", p->spec.wrath )
  {
    if ( p->talent.arcane_affinity.ok() )
    {
      // TODO: arcane affinity seems to be bugged and applies only to the maximum damage of non-UE wrath.
      // as simc only looks at the average, apply a half strength modifier.
      auto val = p->talent.arcane_affinity->effectN( 1 ).percent() * 0.5;
      base_dd_multiplier *= 1.0 + val;
      sim->print_debug( "{} base_dd_multiplier modified by {}%", *this, val * 100 );
      affecting_list.emplace_back( &p->talent.arcane_affinity->effectN( 1 ), val );

      // arcane affinity applies fully to umbral embrace'd wrath
      if ( umbral )
        umbral->apply_affecting_aura( p->talent.arcane_affinity );
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
    CAST_HEAL,
    CAST_MAIN,
    CAST_WRATH,
    CAST_MOONFIRE,
    CAST_RAKE,
    CAST_THRASH_BEAR,
    CAST_FULL_MOON,
    CAST_STARSURGE,
    CAST_STARFALL,
    CAST_PULVERIZE,
    CAST_IRONFUR,
    CAST_MANGLE,
    CAST_FERAL_FRENZY,
    CAST_FEROCIOUS_BITE,
    CAST_THRASH_CAT,
    CAST_SHRED
  };

  struct
  {
    // Multi-spec
    action_t* conv_wrath;
    action_t* conv_moonfire;
    action_t* conv_rake;
    action_t* conv_thrash_bear;
    // Moonkin Form
    action_t* conv_full_moon;
    action_t* conv_starsurge;
    action_t* conv_starfall;
    // Bear Form
    action_t* conv_ironfur;
    action_t* conv_mangle;
    action_t* conv_pulverize;
    // Cat Form
    action_t* conv_feral_frenzy;
    action_t* conv_ferocious_bite;
    action_t* conv_thrash_cat;
    action_t* conv_shred;
    action_t* conv_lunar_inspiration;
    // Heals
    action_t* conv_regrowth;
    action_t* conv_rejuvenation;
  } actions;

  std::vector<convoke_cast_e> cast_list;
  std::unordered_map<action_t*, unsigned> cast_count;
  std::vector<convoke_cast_e> offspec_list;
  std::vector<std::pair<convoke_cast_e, double>> chances;

  shuffled_rng_t* deck = nullptr;

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

    if ( !p->convoke_counter )
      p->convoke_counter = std::make_unique<convoke_counter_t>( *p );

    channeled = true;
    harmful = may_miss = false;

    max_ticks = as<int>( util::floor( dot_duration / base_tick_time ) );

    // create deck for exceptional spell cast
    deck = p->get_shuffled_rng( "convoke_the_spirits", 1, guidance ? 2 : p->options.convoke_the_spirits_deck );
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
    actions.conv_moonfire     = get_convoke_action<moonfire_t>( "moonfire" );
    actions.conv_rake         = get_convoke_action<rake_t>( "rake", p()->find_spell( 1822 ) );
    actions.conv_thrash_bear  = get_convoke_action<thrash_bear_t>( "thrash_bear", p()->find_spell( 77758 ) );
    actions.conv_regrowth     = get_convoke_action<regrowth_t>( "regrowth" );
    actions.conv_rejuvenation = get_convoke_action<rejuvenation_t>( "rejuvenation", p()->find_spell( 774 ) );

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
    p()->convoke_counter->data.emplace( a, a->name_str ).first->second.change_mode( false );
    return a;
  }

  action_t* convoke_action_from_type( convoke_cast_e type )
  {
    switch ( type )
    {
      case CAST_MOONFIRE:
        if ( p()->buff.cat_form->check() )
          return actions.conv_lunar_inspiration;
        else
          return actions.conv_moonfire;
      case CAST_WRATH:          return actions.conv_wrath;
      case CAST_RAKE:           return actions.conv_rake;
      case CAST_THRASH_BEAR:    return actions.conv_thrash_bear;
      case CAST_FULL_MOON:      return actions.conv_full_moon;
      case CAST_STARSURGE:      return actions.conv_starsurge;
      case CAST_STARFALL:       return actions.conv_starfall;
      case CAST_PULVERIZE:      return actions.conv_pulverize;
      case CAST_IRONFUR:        return actions.conv_ironfur;
      case CAST_MANGLE:         return actions.conv_mangle;
      case CAST_FERAL_FRENZY:   return actions.conv_feral_frenzy;
      case CAST_FEROCIOUS_BITE: return actions.conv_ferocious_bite;
      case CAST_THRASH_CAT:     return actions.conv_thrash_cat;
      case CAST_SHRED:          return actions.conv_shred;
      case CAST_HEAL:           return rng().roll( 0.5 ) ? actions.conv_regrowth : actions.conv_rejuvenation;
      default: return nullptr;
    }
  }

  convoke_cast_e get_cast_from_dist( std::vector<std::pair<convoke_cast_e, double>> chances )
  {
    auto _sum = []( double a, std::pair<convoke_cast_e, double> b ) { return a + b.second; };

    auto roll = rng().range( 0.0, std::accumulate( chances.begin(), chances.end(), 0.0, _sum ) );

    for ( auto it = chances.begin(); it != chances.end(); it++ )
      if ( roll < std::accumulate( chances.begin(), it + 1, 0.0, _sum ) )
        return it->first;

    return CAST_NONE;
  }

  double composite_haste() const override { return 1.0; }

  void _init_cat()
  {
    using namespace cat_attacks;

    actions.conv_feral_frenzy      = get_convoke_action<feral_frenzy_t>( "feral_frenzy", p()->find_spell( 274837 ) );
    actions.conv_ferocious_bite    = get_convoke_action<ferocious_bite_t>( "ferocious_bite" );
    actions.conv_thrash_cat        = get_convoke_action<thrash_cat_t>( "thrash_cat", p()->find_spell( 106830 ) );
    actions.conv_shred             = get_convoke_action<shred_t>( "shred" );
    actions.conv_lunar_inspiration = get_convoke_action<lunar_inspiration_t>( "lunar_inspiration", p()->find_spell( 155625 ) );
    // find by id since talent is not required
    actions.conv_lunar_inspiration->s_data_reporting = p()->find_spell( 155580 );

  }

  void _init_moonkin()
  {
    actions.conv_full_moon = get_convoke_action<full_moon_t>( "full_moon", p()->find_spell( 274283 ) );
    actions.conv_starfall  = get_convoke_action<starfall_t>( "starfall", p()->find_spell( 191034 ) );
    actions.conv_starsurge = get_convoke_action<starsurge_t>( "starsurge" );
  }

  void _init_bear()
  {
    using namespace bear_attacks;

    actions.conv_ironfur   = get_convoke_action<ironfur_t>( "ironfur", p()->find_spell( 192081 ) );
    actions.conv_mangle    = get_convoke_action<mangle_t>( "mangle" );
    actions.conv_pulverize = get_convoke_action<pulverize_t>( "pulverize", p()->find_spell( 80313 ) );
  }

  void insert_exceptional( convoke_cast_e cast )
  {
    if ( !deck->trigger() )
      return;

    // Restoration with CS (only 9 ticks) seem to only succeed ~85% of the time
    if ( max_ticks <= 9 && !rng().roll( p()->options.cenarius_guidance_exceptional_chance ) )
      return;

    cast_list.push_back( cast );
  }

  void _execute_bear()
  {
    main_count   = 0;
    offspec_list = { CAST_HEAL, CAST_HEAL, CAST_RAKE, CAST_WRATH };
    chances      = { { CAST_THRASH_BEAR, guidance ? 0.85 : 0.95 },
                     { CAST_IRONFUR, 1.0 },
                     { CAST_MANGLE, 1.0 }
                   };

    cast_list.insert( cast_list.end(),
                      static_cast<int>( rng().range( guidance ? 3.5 : 5, guidance ? 6 : 7 ) ),
                      CAST_OFFSPEC );

    insert_exceptional( CAST_PULVERIZE );
  }

  convoke_cast_e _tick_bear( convoke_cast_e base_type, const std::vector<player_t*>& tl, player_t*& conv_tar )
  {
    convoke_cast_e type_ = base_type;

    // convoke will not cast pulverize if it's already up on the target
    if ( type_ == CAST_PULVERIZE && td( target )->debuff.pulverize->check() )
      type_ = CAST_SPEC;

    if ( type_ == CAST_OFFSPEC && !offspec_list.empty() )
      type_ = offspec_list.at( rng().range( offspec_list.size() ) );
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
        conv_tar = mf_tl.at( rng().range( mf_tl.size() ) );  // mf has it's own target list
    }

    if ( !conv_tar )
      conv_tar = tl.at( rng().range( tl.size() ) );

    if ( type_ == CAST_RAKE && td( conv_tar )->dots.rake->is_ticking() )
      type_ = CAST_WRATH;

    if ( type_ == CAST_MOONFIRE )
      main_count++;

    return type_;
  }

  void _execute_cat()
  {
    offspec_list = { CAST_HEAL, CAST_HEAL, CAST_WRATH, CAST_MOONFIRE };
    chances      = { { CAST_SHRED, 0.10 },
                     { CAST_THRASH_CAT, 0.0588 },
                     { CAST_RAKE, 0.22 }
                   };

    cast_list.insert( cast_list.end(),
                      static_cast<size_t>( rng().range( guidance ? 2.5 : 4, guidance ? 7.5 : 9 ) ),
                      CAST_OFFSPEC );

    insert_exceptional( CAST_FERAL_FRENZY );

    cast_list.insert(
        cast_list.end(),
        static_cast<size_t>( rng().gauss_ab( guidance ? 3 : 4.2, guidance ? 0.8 : 0.9360890055, as<double>( 0 ),
                                             as<double>( max_ticks - cast_list.size() ) ) ),
        CAST_MAIN );
  }

  convoke_cast_e _tick_cat( convoke_cast_e base_type, const std::vector<player_t*>& tl, player_t*& conv_tar )
  {
    convoke_cast_e type_ = base_type;

    if ( base_type == CAST_OFFSPEC && !offspec_list.empty() )
    {
      type_ = offspec_list.at( rng().range( offspec_list.size() ) );
    }
    else if ( base_type == CAST_MAIN )
    {
      type_ = CAST_FEROCIOUS_BITE;
      conv_tar = p()->target;
    }
    else if ( base_type == CAST_SPEC )
    {
      auto dist = chances;
      type_ = get_cast_from_dist( dist );
    }
    else if ( base_type == CAST_FERAL_FRENZY )
    {
      conv_tar = p()->target;
    }

    if ( !conv_tar )
      conv_tar = tl.at( rng().range( tl.size() ) );

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

    insert_exceptional( CAST_FULL_MOON );
  }

  convoke_cast_e _tick_moonkin( convoke_cast_e base_type, const std::vector<player_t*>& tl, player_t*& conv_tar )
  {
    convoke_cast_e type_ = base_type;
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
        conv_tar = mf_tl.at( rng().range( mf_tl.size() ) );
    }
    else if ( type_ == CAST_FULL_MOON )
    {
      conv_tar = p()->target;
    }

    if ( !conv_tar )
      conv_tar = tl.at( rng().range( tl.size() ) );

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
    cast_count.clear();
    main_count = 0;

    // form-specific execute setup
    if ( p()->buff.bear_form->check() )
      _execute_bear();
    else if ( p()->buff.moonkin_form->check() )
      _execute_moonkin();
    else if ( p()->buff.cat_form->check() )
      _execute_cat();

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
    std::swap( cast_list.at( rng().range( cast_list.size() ) ), cast_list.back() );
    auto type = cast_list.back();
    cast_list.pop_back();

    std::vector<player_t*> tl = target_list();
    if ( tl.empty() )
      return;

    // Do form-specific spell selection
    if ( p()->buff.moonkin_form->check() )
      type = _tick_moonkin( type, tl, conv_tar );
    else if ( p()->buff.bear_form->check() )
      type = _tick_bear( type, tl, conv_tar );
    else if ( p()->buff.cat_form->check() )
      type = _tick_cat( type, tl, conv_tar );

    conv_cast = convoke_action_from_type( type );
    if ( !conv_cast )
      return;

    cast_count[ conv_cast ]++;

    if ( type == convoke_cast_e::CAST_HEAL )
    {
      const auto& heal_tl = conv_cast->target_list();
      conv_tar = heal_tl.at( rng().range( heal_tl.size() ) );
    }

    conv_cast->execute_on_target( conv_tar );
  }

  void last_tick( dot_t* d ) override
  {
    base_t::last_tick( d );

    for ( auto& [ a, sample ] : p()->convoke_counter->data )
    {
      if ( auto it = cast_count.find( a ); it != cast_count.end() )
        sample.add( it->second );
      else
        sample.add( 0 );
    }
  }

  bool usable_moving() const override { return true; }
};
}  // end namespace spells

#undef DRUID_ABILITY
#undef DRUID_ABILITY_B
#undef DRUID_ABILITY_C
#undef DRUID_ABILITY_D

namespace auto_attacks
{
template <typename Base>
struct druid_melee_t : public Base
{
  using ab = Base;
  using base_t = druid_melee_t<Base>;

  accumulated_rng_t* ravage_rng = nullptr;
  proc_t* ravage_proc = nullptr;
  buff_t* ravage_buff = nullptr;
  double ooc_chance = 0.0;

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
    ab::parse_effects( p->spec_spell );
    ab::parse_effects( p->talent.killer_instinct );
    ab::parse_effects( p->buff.tigers_fury,
                       p->talent.carnivorous_instinct,
                       p->talent.tigers_tenacity );

    // 7.00 PPM via community testing (~368k auto attacks)
    // https://docs.google.com/spreadsheets/d/1vMvlq1k3aAuwC1iHyDjqAneojPZusdwkZGmatuWWZWs/edit#gid=1097586165
    if ( p->talent.omen_of_clarity_cat.ok() )
      ooc_chance = 7.00;

    if ( p->talent.moment_of_clarity.ok() )
      ooc_chance *= 1.0 + p->talent.moment_of_clarity->effectN( 2 ).percent();

    // Feral: 0.286% via community testing (~197k auto attacks)
    // Guardian: 1.144% via community testing (9921 auto attacks), 4x feral
    // https://docs.google.com/spreadsheets/d/1lPDhmfqe03G_eFetGJEbSLbXMcfkzjhzyTaQ8mdxADM/edit?gid=385734241
    if ( p->talent.ravage.ok() )
    {
      auto c = 0.0;
      if ( p->specialization() == DRUID_FERAL )
      {
        c = 0.00286;
        ravage_buff = p->buff.ravage_fb;
      }
      else if ( p->specialization() == DRUID_GUARDIAN )
      {
        c = 0.01144;
        ravage_buff = p->buff.ravage_maul;
      }

      ravage_rng = p->get_accumulated_rng( "ravage", c );
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
        }
      }

      if ( ravage_rng && ravage_rng->trigger() )
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
  proc_t* tnc_proc = nullptr;

  bear_melee_t( druid_t* p ) : base_t( "bear_melee", p )
  {
    name_str_reporting = "Bear Melee";

    form_mask = form_e::BEAR_FORM;

    energize_type = action_energize::ON_HIT;
    energize_resource = resource_e::RESOURCE_RAGE;
    energize_amount = util::round( 1.75 *
      p->bear_weapon.swing_time.total_seconds() *
      ( 1.0 + find_effect( p->spec.bear_form_passive, A_MOD_RAGE_FROM_DAMAGE_DEALT ).percent() ), 1 );

    if ( p->talent.gore.ok() )
      tnc_proc = p->get_proc( "Tooth and Claw" )->collect_interval();
  }

  void execute() override
  {
    base_t::execute();

    if ( hit_any_target && p()->buff.tooth_and_claw->trigger() && tnc_proc )
        tnc_proc->occur();
  }
};

// Cat Melee Attack =========================================================
struct cat_melee_t final : public druid_melee_t<cat_attack_t>
{
  cat_melee_t( druid_t* p ) : base_t( "cat_melee", p )
  {
    name_str_reporting = "Cat Melee";

    form_mask = form_e::CAT_FORM;
    snapshots.tigers_fury = true;
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
      nb_cap( p->resources.base[ RESOURCE_ASTRAL_POWER ] * p->talent.natures_balance->effectN( 2 ).percent() )
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

  if ( talent.lycaras_teachings.ok() )
  {
    sim->register_heartbeat_event_callback( [ this ]( sim_t* ) {
      buff_t* new_buff;

      switch ( get_form() )
      {
        case NO_FORM:      new_buff = buff.lycaras_teachings_haste; break;
        case CAT_FORM:     new_buff = buff.lycaras_teachings_crit; break;
        case BEAR_FORM:    new_buff = buff.lycaras_teachings_vers; break;
        case MOONKIN_FORM: new_buff = buff.lycaras_teachings_mast; break;
        default:           return;
      }

      if ( !new_buff->check() )
      {
        buff.lycaras_teachings_haste->expire();
        buff.lycaras_teachings_crit->expire();
        buff.lycaras_teachings_vers->expire();
        buff.lycaras_teachings_mast->expire();

        new_buff->trigger();
      }
    } );
  }

  if ( talent.resilient_flourishing.ok() && talent.thriving_growth.ok() )
  {
    register_on_kill_callback( [ this ]( player_t* t ) {
      if ( auto dur = get_target_data( t )->dots.bloodseeker_vines->remains(); dur > 0_ms )
      {
        const auto& tl = active.bloodseeker_vines->target_list();
        if ( auto tar = get_smart_target( tl, &druid_td_t::dots_t::bloodseeker_vines, t ) )
        {
          // TODO: ugly hack. possibly use custom action_state
          auto orig_dur = active.bloodseeker_vines->dot_duration;

          active.bloodseeker_vines->dot_duration = dur;
          active.bloodseeker_vines->dual = true;

          active.bloodseeker_vines->execute_on_target( tar );

          active.bloodseeker_vines->dot_duration = orig_dur;
          active.bloodseeker_vines->dual = false;
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
  else if ( name == "entangling_roots"              ) a =             new entangling_roots_t( this );
  else if ( name == "ferocious_bite"                ) a =               new ferocious_bite_t( this );
  else if ( name == "growl"                         ) a =                        new growl_t( this );
  else if ( name == "mangle"                        ) a =                       new mangle_t( this );
  else if ( name == "mark_of_the_wild"              ) a =             new mark_of_the_wild_t( this );
  else if ( name == "moonfire"                      ) a =                     new moonfire_t( this );
  else if ( name == "prowl"                         ) a =                        new prowl_t( this );
  else if ( name == "regrowth"                      ) a =                     new regrowth_t( this );
  else if ( name == "shred"                         ) a =                        new shred_t( this );
  else if ( name == "wrath"                         ) a =                        new wrath_t( this );

  // Class Talents
  else if ( name == "barkskin"                      ) a =                     new barkskin_t( this );
  else if ( name == "dash" ||
            name == "tiger_dash"                    ) a =                         new dash_t( this );
  else if ( name == "frenzied_regeneration"         ) a =        new frenzied_regeneration_t( this );
  else if ( name == "heart_of_the_wild"             ) a =            new heart_of_the_wild_t( this );
  else if ( name == "incapacitating_roar"           ) a =          new incapacitating_roar_t( this );
  else if ( name == "innervate"                     ) a =                    new innervate_t( this );
  else if ( name == "ironfur"                       ) a =                      new ironfur_t( this );
  else if ( name == "maim"                          ) a =                         new maim_t( this );
  else if ( name == "moonkin_form"                  ) a =                 new moonkin_form_t( this );
  else if ( name == "natures_vigil" )
  {
    if      ( specialization() == DRUID_RESTORATION ) a = new natures_vigil_t<druid_spell_t>( this );
    else                                              a =  new natures_vigil_t<druid_heal_t>( this );
  }
  else if ( name == "rake"                          ) a =                         new rake_t( this );
  else if ( name == "rejuvenation"                  ) a =                 new rejuvenation_t( this );
  else if ( name == "remove_corruption"             ) a =            new remove_corruption_t( this );
  else if ( name == "renewal"                       ) a =                      new renewal_t( this );
  else if ( name == "rip"                           ) a =                          new rip_t( this );
  else if ( name == "skull_bash"                    ) a =                   new skull_bash_t( this );
  else if ( name == "soothe"                        ) a =                       new soothe_t( this );
  else if ( name == "stampeding_roar"               ) a =              new stampeding_roar_t( this );
  else if ( name == "starfire"                      ) a =                     new starfire_t( this );
  else if ( name == "starsurge" )
  {
    if      ( specialization() == DRUID_BALANCE     ) a =                    new starsurge_t( this );
    else                                              a =            new starsurge_offspec_t( this );
  }
  else if ( name == "sunfire"                       ) a =                      new sunfire_t( this );
  else if ( name == "swipe"                         ) a =                  new swipe_proxy_t( this );
  else if ( name == "swipe_bear"                    ) a =                   new swipe_bear_t( this );
  else if ( name == "swipe_cat"                     ) a =                    new swipe_cat_t( this );
  else if ( name == "thrash"                        ) a =                 new thrash_proxy_t( this );
  else if ( name == "thrash_bear"                   ) a =                  new thrash_bear_t( this );
  else if ( name == "thrash_cat"                    ) a =                   new thrash_cat_t( this );
  else if ( name == "wild_charge"                   ) a =                  new wild_charge_t( this );
  else if ( name == "wild_growth"                   ) a =                  new wild_growth_t( this );

  // Multispec Talents
  else if ( name == "berserk" )
  {
    if      ( specialization() == DRUID_GUARDIAN    ) a =                 new berserk_bear_t( this );
    else if ( specialization() == DRUID_FERAL       ) a =                  new berserk_cat_t( this );
  }
  else if ( name == "convoke_the_spirits"           ) a =          new convoke_the_spirits_t( this );
  else if ( name == "incarnation" )
  {
    if      ( specialization() == DRUID_BALANCE     ) a =          new incarnation_moonkin_t( this );
    else if ( specialization() == DRUID_FERAL       ) a =              new incarnation_cat_t( this );
    else if ( specialization() == DRUID_GUARDIAN    ) a =             new incarnation_bear_t( this );
    else if ( specialization() == DRUID_RESTORATION ) a =             new incarnation_tree_t( this );
  }
  else if ( name == "survival_instincts"            ) a =           new survival_instincts_t( this );

  // Balance
  else if ( name == "celestial_alignment"           ) a =          new celestial_alignment_t( this );
  else if ( name == "force_of_nature"               ) a =              new force_of_nature_t( this );
  else if ( name == "fury_of_elune"                 ) a =                new fury_of_elune_t( this );
  else if ( name == "new_moon"                      ) a =                     new new_moon_t( this );
  else if ( name == "half_moon"                     ) a =                    new half_moon_t( this );
  else if ( name == "full_moon"                     ) a =                    new full_moon_t( this );
  else if ( name == "moons"                         ) a =                   new moon_proxy_t( this );
  else if ( name == "solar_beam"                    ) a =                   new solar_beam_t( this );
  else if ( name == "starfall"                      ) a =                     new starfall_t( this );
  else if ( name == "stellar_flare"                 ) a =                new stellar_flare_t( this );
  else if ( name == "warrior_of_elune"              ) a =             new warrior_of_elune_t( this );
  else if ( name == "wild_mushroom"                 ) a =                new wild_mushroom_t( this );

  // Feral
  else if ( name == "adaptive_swarm"                ) a =               new adaptive_swarm_t( this );
  else if ( name == "berserk_cat"                   ) a =                  new berserk_cat_t( this );
  else if ( name == "brutal_slash"                  ) a =                 new brutal_slash_t( this );
  else if ( name == "feral_frenzy"                  ) a =                 new feral_frenzy_t( this );
  else if ( name == "moonfire_cat" ||
            name == "lunar_inspiration"             ) a =            new lunar_inspiration_t( this );
  else if ( name == "primal_wrath"                  ) a =                 new primal_wrath_t( this );
  else if ( name == "tigers_fury"                   ) a =                  new tigers_fury_t( this );

  // Guardian
  else if ( name == "berserk_bear"                  ) a =                 new berserk_bear_t( this );
  else if ( name == "bristling_fur"                 ) a =                new bristling_fur_t( this );
  else if ( name == "lunar_beam"                    ) a =                   new lunar_beam_t( this );
  else if ( name == "maul"                          ) a =                         new maul_t( this );
  else if ( name == "pulverize"                     ) a =                    new pulverize_t( this );
  else if ( name == "raze"                          ) a =                         new raze_t( this );
  else if ( name == "rage_of_the_sleeper"           ) a =          new rage_of_the_sleeper_t( this );

  // Restoration
  else if ( name == "cenarion_ward"                 ) a =                new cenarion_ward_t( this );
  else if ( name == "efflorescence"                 ) a =                new efflorescence_t( this );
  else if ( name == "flourish"                      ) a =                     new flourish_t( this );
  else if ( name == "lifebloom"                     ) a =                    new lifebloom_t( this );
  else if ( name == "natures_cure"                  ) a =                 new natures_cure_t( this );
  else if ( name == "nourish"                       ) a =                      new nourish_t( this );
  else if ( name == "overgrowth"                    ) a =                   new overgrowth_t( this );
  else if ( name == "swiftmend"                     ) a =                    new swiftmend_t( this );
  else if ( name == "tranquility"                   ) a =                  new tranquility_t( this );

  if ( a )
  {
    a->parse_options( opt );

    if ( auto tmp = dynamic_cast<druid_action_data_t*>( a ) )
      tmp->action_flags |= flag_e::FOREGROUND;

    return a;
  }

  return player_t::create_action( name, opt );
}

// druid_t::create_pets =====================================================
void druid_t::create_pets()
{
  player_t::create_pets();

  // not actually pets, but this stage is a good place to create these as all spells & actions have been created
  if ( talent.adaptive_swarm.ok() )
  {
    swarm_targets.push_back( this );

    size_t i = 0;
    while ( i < options.adaptive_swarm_melee_targets )
    {
      auto t = swarm_targets.emplace_back(
        module_t::heal_enemy()->create_player( sim, "swarm_melee_target_" + util::to_string( ++i ), RACE_NONE ) );
      t->quiet = true;
      t->role = ROLE_ATTACK;
      sim->init_actor( t );
    }

    while ( i < options.adaptive_swarm_melee_targets + options.adaptive_swarm_ranged_targets )
    {
      auto t = swarm_targets.emplace_back(
        module_t::heal_enemy()->create_player( sim, "swarm_ranged_target_" + util::to_string( ++i ), RACE_NONE ) );
      t->quiet = true;
      t->role = ROLE_SPELL;
      sim->init_actor( t );
    }
  }
}

// druid_t::init_spells =====================================================
void druid_t::init_spells()
{
  // Carnivorous Instinct is missing trait definitions for effect#2 and effect#3, so we manually add it into
  // dbc_override here, before any talent pointers are initialized.
  if ( auto ci = find_talent_spell( talent_tree::SPECIALIZATION, "Carnivorous Instinct" ).spell(); ci->ok() )
  {
    auto val = ci->effectN( 1 ).base_value();
    // manual validation
    assert( ci->effectN( 2 ).base_value() == 6.0 );
    assert( ci->effectN( 3 ).base_value() == 6.0 );
    const_cast<dbc_override_t*>( dbc_override )->register_effect( *dbc, ci->effectN( 2 ).id(), "base_value", val );
    const_cast<dbc_override_t*>( dbc_override )->register_effect( *dbc, ci->effectN( 3 ).id(), "base_value", val );
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

  auto CT  = [ this ]( std::string_view n ) { return find_talent_spell( talent_tree::CLASS, n ); };
  auto ST  = [ this ]( std::string_view n ) { return find_talent_spell( talent_tree::SPECIALIZATION, n ); };
  auto STS = [ this ]( std::string_view n, specialization_e s ) {
    return find_talent_spell( talent_tree::SPECIALIZATION, n, s );
  };
  auto HT = [ this ]( std::string_view n ) { return find_talent_spell( talent_tree::HERO, n ); };

  // Class tree
  sim->print_debug( "Initializing class talents..." );
  talent.astral_influence               = CT( "Astral Influence" );
  talent.cyclone                        = CT( "Cyclone" );
  talent.feline_swiftness               = CT( "Feline Swiftness" );
  talent.fluid_form                     = CT( "Fluid Form" );
  talent.forestwalk                     = CT( "Forestwalk" );
  talent.frenzied_regeneration          = CT( "Frenzied Regeneration" );
  talent.heart_of_the_wild              = CT( "Heart of the Wild" );
  talent.hibernate                      = CT( "Hibernate" );
  talent.improved_barkskin              = CT( "Improved Barkskin" );
  talent.improved_rejuvenation          = CT( "Improved Rejuvenation" );
  talent.improved_stampeding_roar       = CT( "Improved Stampeding Roar");
  talent.improved_sunfire               = CT( "Improved Sunfire" );
  talent.incapacitating_roar            = CT( "Incapacitating Roar" );
  talent.innervate                      = CT( "Innervate" );
  talent.instincts_of_the_claw          = CT( "Instincts of the Claw" );
  talent.ironfur                        = CT( "Ironfur" );
  talent.killer_instinct                = CT( "Killer Instinct" );
  talent.lore_of_the_grove              = CT( "Lore of the Grove" );
  talent.lycaras_teachings              = CT( "Lycara's Teachings" );
  talent.maim                           = CT( "Maim" );
  talent.mass_entanglement              = CT( "Mass Entanglement" );
  talent.matted_fur                     = CT( "Matted Fur" );
  talent.mighty_bash                    = CT( "Mighty Bash" );
  talent.natural_recovery               = CT( "Natural Recovery" );
  talent.natures_vigil                  = CT( "Nature's Vigil" );
  talent.nurturing_instinct             = CT( "Nurturing Instinct" );
  talent.oakskin                        = CT( "Oakskin" );
  talent.primal_fury                    = CT( "Primal Fury" );
  talent.rake                           = CT( "Rake" );
  talent.rejuvenation                   = CT( "Rejuvenation" );
  talent.remove_corruption              = CT( "Remove Corruption" );
  talent.renewal                        = CT( "Renewal" );
  talent.rising_light_falling_night     = CT( "Rising Light, Falling Night" );
  talent.rip                            = CT( "Rip" );
  talent.skull_bash                     = CT( "Skull Bash" );
  talent.soothe                         = CT( "Soothe" );
  talent.stampeding_roar                = CT( "Stampeding Roar" );
  talent.starfire                       = CT( "Starfire" );
  talent.starlight_conduit              = CT( "Starlight Conduit" );
  talent.starsurge                      = CT( "Starsurge" );
  talent.sunfire                        = CT( "Sunfire" );
  talent.thick_hide                     = CT( "Thick Hide" );
  talent.thrash                         = CT( "Thrash" );
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
  talent.aetherial_kindling             = ST( "Aetherial Kindling" );
  talent.astral_communion               = ST( "Astral Communion" );
  talent.astral_smolder                 = ST( "Astral Smolder" );
  talent.astronomical_impact            = ST( "Astronomical Impact" );
  talent.balance_of_all_things          = ST( "Balance of All Things" );
  talent.celestial_alignment            = ST( "Celestial Alignment" );
  talent.circle_of_life_and_death_owl   = STS( "Circle of Life and Death", DRUID_BALANCE );
  talent.cosmic_rapidity                = ST( "Cosmic Rapidity" );
  talent.crashing_star                  = ST( "Crashing Star" );
  talent.denizen_of_the_dream           = ST( "Denizen of the Dream" );
  talent.eclipse                        = ST( "Eclipse" );
  talent.elunes_guidance                = ST( "Elune's Guidance" );
  talent.force_of_nature                = ST( "Force of Nature" );
  talent.fury_of_elune                  = ST( "Fury of Elune" );
  talent.greater_alignment              = ST( "Greater Alignment" );
  talent.harmony_of_the_heavens         = ST( "Harmony of the Heavens" );
  talent.hail_of_stars                  = ST( "Hail of Stars" );
  talent.incarnation_moonkin            = ST( "Incarnation: Chosen of Elune" );
  talent.light_of_the_sun               = ST( "Light of the Sun" );
  talent.lunar_shrapnel                 = ST( "Lunar Shrapnel" );
  talent.natures_balance                = ST( "Nature's Balance" );
  talent.natures_grace                  = ST( "Nature's Grace" );
  talent.new_moon                       = ST( "New Moon" );
  talent.orbit_breaker                  = ST( "Orbit Breaker" );
  talent.orbital_strike                 = ST( "Orbital Strike" );
  talent.power_of_goldrinn              = ST( "Power of Goldrinn" );
  talent.radiant_moonlight              = ST( "Radiant Moonlight" );
  talent.rattle_the_stars               = ST( "Rattle the Stars" );
  talent.shooting_stars                 = ST( "Shooting Stars" );
  talent.solar_beam                     = ST( "Solar Beam" );
  talent.solstice                       = ST( "Solstice" );
  talent.soul_of_the_forest_moonkin     = STS( "Soul of the Forest", DRUID_BALANCE );
  talent.starfall                       = ST( "Starfall" );
  talent.starlord                       = ST( "Starlord" );
  talent.starweaver                     = ST( "Starweaver" );
  talent.stellar_amplification          = ST( "Stellar Amplification" );
  talent.stellar_flare                  = ST( "Stellar Flare" );
  talent.sundered_firmament             = ST( "Sundered Firmament" );
  talent.sunseeker_mushroom             = ST( "Sunseeker Mushroom" );
  talent.touch_the_cosmos               = ST( "Touch the Cosmos" );
  talent.twin_moons                     = ST( "Twin Moons" );
  talent.umbral_embrace                 = ST( "Umbral Embrace" );
  talent.umbral_inspiration             = ST( "Umbral Inspiration" );
  talent.umbral_intensity               = ST( "Umbral Intensity" );
  talent.waning_twilight                = ST( "Waning Twilight" );
  talent.warrior_of_elune               = ST( "Warrior of Elune" );
  talent.whirling_stars                 = ST( "Whirling Stars" );
  talent.wild_mushroom                  = ST( "Wild Mushroom" );
  talent.wild_surges                    = ST( "Wild Surges" );

  // Feral
  sim->print_debug( "Initializing feral talents..." );
  talent.adaptive_swarm                 = ST( "Adaptive Swarm" );
  talent.apex_predators_craving         = ST( "Apex Predator's Craving" );
  talent.ashamanes_guidance             = ST( "Ashamane's Guidance" );
  talent.berserk                        = ST( "Berserk" );
  talent.berserk_frenzy                 = ST( "Berserk: Frenzy" );
  talent.berserk_heart_of_the_lion      = ST( "Berserk: Heart of the Lion" );
  talent.bloodtalons                    = ST( "Bloodtalons" );
  talent.brutal_slash                   = ST( "Brutal Slash" );
  talent.carnivorous_instinct           = ST( "Carnivorous Instinct" );
  talent.circle_of_life_and_death_cat   = STS( "Circle of Life and Death", DRUID_FERAL );
  talent.coiled_to_spring               = ST( "Coiled to Spring" );
  talent.doubleclawed_rake              = ST( "Double-Clawed Rake" );
  talent.dreadful_bleeding              = ST( "Dreadful Bleeding" );
  talent.feral_frenzy                   = ST( "Feral Frenzy" );
  talent.frantic_momentum               = ST( "Frantic Momentum" );
  talent.incarnation_cat                = ST( "Incarnation: Avatar of Ashamane" );
  talent.infected_wounds_cat            = STS( "Infected Wounds", DRUID_FERAL );
  talent.lions_strength                 = ST( "Lion's Strength" );
  talent.lunar_inspiration              = ST( "Lunar Inspiration" );
  talent.merciless_claws                = ST( "Merciless Claws" );
  talent.moment_of_clarity              = ST( "Moment of Clarity" );
  talent.omen_of_clarity_cat            = STS( "Omen of Clarity", DRUID_FERAL );
  talent.pouncing_strikes               = ST( "Pouncing Strikes" );
  talent.predatory_swiftness            = ST( "Predatory Swiftness" );
  talent.primal_wrath                   = ST( "Primal Wrath" );
  talent.predator                       = ST( "Predator" );
  talent.raging_fury                    = ST( "Raging Fury" );
  talent.rampant_ferocity               = ST( "Rampant Ferocity" );
  talent.rip_and_tear                   = ST( "Rip and Tear" );
  talent.saber_jaws                     = ST( "Saber Jaws" );
  talent.sabertooth                     = ST( "Sabertooth" );
  talent.savage_fury                    = ST( "Savage Fury" );
  talent.soul_of_the_forest_cat         = STS( "Soul of the Forest", DRUID_FERAL );
  talent.sudden_ambush                  = ST( "Sudden Ambush" );
  talent.taste_for_blood                = ST( "Taste for Blood" );
  talent.thrashing_claws                = ST( "Thrashing Claws" );
  talent.tigers_fury                    = ST( "Tiger's Fury" );
  talent.tigers_tenacity                = ST( "Tiger's Tenacity" );
  talent.tireless_energy                = ST( "Tireless Energy" );
  talent.unbridled_swarm                = ST( "Unbridled Swarm" );
  talent.veinripper                     = ST( "Veinripper" );
  talent.wild_slashes                   = ST( "Wild Slashes" );

  // Guardian
  sim->print_debug( "Initializing guardian talents..." );
  talent.after_the_wildfire             = ST( "After the Wildfire" );
  talent.berserk_persistence            = ST( "Berserk: Persistence" );
  talent.berserk_ravage                 = ST( "Berserk: Ravage" );
  talent.berserk_unchecked_aggression   = ST( "Berserk: Unchecked Aggression" );
  talent.blood_frenzy                   = ST( "Blood Frenzy" );
  talent.brambles                       = ST( "Brambles" );
  talent.bristling_fur                  = ST( "Bristling Fur" );
  talent.circle_of_life_and_death_bear  = STS( "Circle of Life and Death", DRUID_GUARDIAN );
  talent.dream_of_cenarius_bear         = STS( "Dream of Cenarius", DRUID_GUARDIAN );
  talent.earthwarden                    = ST( "Earthwarden" );
  talent.elunes_favored                 = ST( "Elune's Favored" );
  talent.flashing_claws                 = ST( "Flashing Claws" );
  talent.fury_of_nature                 = ST( "Fury of Nature" );
  talent.galactic_guardian              = ST( "Galactic Guardian" );
  talent.gore                           = ST( "Gore" );
  talent.gory_fur                       = ST( "Gory Fur" );
  talent.guardian_of_elune              = ST( "Guardian of Elune" );
  talent.improved_survival_instincts    = ST( "Improved Survival Instincts" );
  talent.incarnation_bear               = ST( "Incarnation: Guardian of Ursoc" );
  talent.infected_wounds_bear           = STS( "Infected Wounds", DRUID_GUARDIAN );
  talent.innate_resolve                 = ST( "Innate Resolve" );
  talent.layered_mane                   = ST( "Layered Mane" );
  talent.lunar_beam                     = ST( "Lunar Beam" );
  talent.mangle                         = ST( "Mangle" );
  talent.maul                           = ST( "Maul" );
  talent.moonless_night                 = ST( "Moonless Night" );
  talent.pulverize                      = ST( "Pulverize" );
  talent.rage_of_the_sleeper            = ST( "Rage of the Sleeper" );
  talent.raze                           = ST( "Raze" );
  talent.reinforced_fur                 = ST( "Reinforced Fur" );
  talent.reinvigoration                 = ST( "Reinvigoration" );
  talent.rend_and_tear                  = ST( "Rend and Tear" );
  talent.scintillating_moonlight        = ST( "Scintillating Moonlight" );
  talent.soul_of_the_forest_bear        = STS( "Soul of the Forest", DRUID_GUARDIAN );
  talent.survival_of_the_fittest        = ST( "Survival of the Fittest" );
  talent.thorns_of_iron                 = ST( "Thorns of Iron" );
  talent.tooth_and_claw                 = ST( "Tooth and Claw" );
  talent.twin_moonfire                  = ST( "Twin Moonfire" );
  talent.ursocs_guidance                = ST( "Ursoc's Guidance" );
  talent.ursocs_endurance               = ST( "Ursoc's Endurance" );
  talent.vicious_cycle                  = ST( "Vicious Cycle" );
  talent.vulnerable_flesh               = ST( "Vulnerable Flesh" );
  talent.untamed_savagery               = ST( "Untamed Savagery" );
  talent.ursocs_fury                    = ST( "Ursoc's Fury" );

  // Restoration
  sim->print_debug( "Initializing restoration talents..." );
  talent.abundance                      = ST( "Abundance" );
  talent.budding_leaves                 = ST( "Budding Leaves" );  // TODO: NYI
  talent.call_of_the_elder_druid        = ST( "Call of the Elder Druid" );  // TODO: NYI
  talent.cenarion_ward                  = ST( "Cenarion Ward" );
  talent.cenarius_guidance              = ST( "Cenarius' Guidance" );  // TODO: Incarn bonus NYI
  talent.cultivation                    = ST( "Cultivation" );
  talent.dream_of_cenarius_tree         = STS( "Dream of Cenarius", DRUID_RESTORATION );  // TODO: NYI
  talent.dreamstate                     = ST( "Dreamstate" );  // TODO: NYI
  talent.efflorescence                  = ST( "Efflorescence" );
  talent.embrace_of_the_dream           = ST( "Embrace of the Dream" );  // TODO: NYI
  talent.flash_of_clarity               = ST( "Flash of Clarity" );
  talent.flourish                       = ST( "Flourish" );
  talent.germination                    = ST( "Germination" );
  talent.grove_guardians                = ST( "Grove Guardians" );  // TODO: NYI
  talent.grove_tending                  = ST( "Grove Tending" );
  talent.harmonious_blooming            = ST( "Harmonious Blooming" );
  talent.improved_ironbark              = ST( "Improved Ironbark" );
  talent.improved_regrowth              = ST( "Improved Regrowth" );
  talent.improved_wild_growth           = ST( "Improved Wild Growth" );
  talent.incarnation_tree               = ST( "Incarnation: Tree of Life" );
  talent.inner_peace                    = ST( "Inner Peace" );
  talent.invigorate                     = ST( "Invigorate" );  // TODO: NYI
  talent.ironbark                       = ST( "Ironbark" );
  talent.lifebloom                      = ST( "Lifebloom" );
  talent.liveliness                     = ST( "Liveliness" );
  talent.master_shapeshifter            = ST( "Master Shapeshifter" );
  talent.natures_splendor               = ST( "Nature's Splendor" );
  talent.natures_swiftness              = ST( "Nature's Swiftness" );
  talent.nourish                        = ST( "Nourish" );
  talent.nurturing_dormancy             = ST( "Nurturing Dormancy" );  // TODO: NYI
  talent.omen_of_clarity_tree           = STS( "Omen of Clarity", DRUID_RESTORATION );
  talent.overgrowth                     = ST( "Overgrowth" );
  talent.passing_seasons                = ST( "Passing Seasons" );
  talent.photosynthesis                 = ST( "Photosynthesis" );
  talent.power_of_the_archdruid         = ST( "Power of the Archdruid" );  // TODO: NYI
  talent.prosperity                     = ST( "Prosperity" );  // TODO: NYI
  talent.rampant_growth                 = ST( "Rampant Growth" );  // TODO: copy on lb target NYI
  talent.reforestation                  = ST( "Reforestation" );  // TODO: NYI
  talent.regenesis                      = ST( "Regenesis" );  // TODO: NYI
  talent.regenerative_heartwood         = ST( "Regenerative Heartwood" );  // TODO: NYI
  talent.soul_of_the_forest_tree        = STS( "Soul of the Forest", DRUID_RESTORATION );
  talent.spring_blossoms                = ST( "Spring Blossoms" );
  talent.stonebark                      = ST( "Stonebark" );
  talent.thriving_vegetation            = ST( "Thriving Vegetation" );  // TODO: NYI
  talent.tranquil_mind                  = ST( "Tranquil Mind" );
  talent.tranquility                    = ST( "Tranquility" );
  talent.undergrowth                    = ST( "Undergrowth" );  // TODO: NYI
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
  talent.fount_of_strength              = HT( "Fount of Strength" );  // TODO: FR hp buff NYI
  talent.killing_strikes                = HT( "Killing Strikes" );
  talent.packs_endurance                = HT( "Pack's Endurance" );
  talent.ravage                         = HT( "Ravage" );
  talent.ruthless_aggression            = HT( "Ruthless Aggression" );
  talent.strike_for_the_heart           = HT( "Strike for the Heart" );
  talent.tear_down_the_mighty           = HT( "Tear Down the Mighty" );
  talent.wildpower_surge                = HT( "Wildpower Surge" );
  talent.wildshape_mastery              = HT( "Wildshape Mastery" );  // TODO:: heal NYI

  // Wildstalker
  talent.bond_with_nature               = HT( "Bond with Nature" );
  talent.bursting_growth                = HT( "Bursting Growth" );
  talent.entangling_vortex              = HT( "Entangling Vortex" );  // NYI
  talent.flower_walk                    = HT( "Flower Walk" );  // TODO: heal NYI
  talent.harmonious_constitution        = HT( "Harmonious Constitution" );
  talent.hunt_beneath_the_open_skies    = HT( "Hunt Beneath the Open Skies" );
  talent.implant                        = HT( "Implant" );
  talent.lethal_preservation            = HT( "Lethal Preservation" );
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
  talent.early_spring                   = HT( "Early Spring" );
  talent.expansiveness                  = HT( "Expansiveness" );
  talent.groves_inspiration             = HT( "Grove's Inspiration" );
  talent.harmony_of_the_grove           = HT( "Harmony of the Grove" );
  talent.potent_enchantments            = HT( "Potent Enchantments" );
  talent.power_of_nature                = HT( "Power of Nature" );  // TODO: grove guardian buff NYI
  talent.power_of_the_dream             = HT( "Power of the Dream" );
  talent.protective_growth              = HT( "Protective Growth" );
  talent.treants_of_the_moon            = HT( "Treants of the Moon" );

  // Elune's Chosen
  talent.arcane_affinity                = HT( "Arcane Affinity" );
  talent.astral_insight                 = HT( "Astral Insight" );
  talent.atmospheric_exposure           = HT( "Atmospheric Exposure" );
  talent.boundless_moonlight            = HT( "Boundless Moonlight" );
  talent.elunes_grace                   = HT( "Elune's Grace" );
  talent.glistening_fur                 = HT( "Glistening Fur" );
  talent.lunar_amplification            = HT( "Lunar Amplification" );
  talent.lunar_calling                  = HT( "Lunar Calling" );
  talent.lunar_insight                  = HT( "Lunar Insight" );
  talent.lunation                       = HT( "Lunation" );
  talent.moondust                       = HT( "Moondust" );  // TODO: NYI
  talent.moon_guardian                  = HT( "Moon Guardian" );
  talent.stellar_command                = HT( "Stellar Command" );
  talent.the_eternal_moon               = HT( "The Eternal Moon" );
  talent.the_light_of_elune             = HT( "The Light of Elune" );

  // Passive Auras
  spec.druid                    = dbc::get_class_passive( *this, SPEC_NONE );

  // Baseline
  spec.bear_form_override       = find_spell( 106829 );
  spec.bear_form_passive        = find_spell( 1178 );
  spec.bear_form_passive_2      = find_spell( 21178 );
  spec.cat_form_override        = find_spell( 48629 );
  spec.cat_form_passive         = find_spell( 3025 );
  spec.cat_form_passive_2       = find_spell( 106840 );
  spec.cat_form_speed           = find_spell( 113636 );
  spec.feral_affinity           = find_specialization_spell( "Feral Affinity" );
  spec.improved_prowl           = find_specialization_spell( "Improved Prowl" );
  spec.moonfire_2               = find_rank_spell( "Moonfire", "Rank 2" );
  spec.moonfire_dmg             = find_spell( 164812 );
  spec.swipe                    = find_class_spell( "Swipe" );
  spec.wrath                    = find_specialization_spell( "Wrath" );
  if ( !spec.wrath->ok() )
    spec.wrath                  = find_class_spell( "Wrath" );

  // Class Abilities
  spec.moonfire                 = find_class_spell( "Moonfire" );
  spec.sunfire_dmg              = check( talent.sunfire, 164815 );
  spec.thrash_bear_bleed        = find_spell( 192090 );
  spec.thrash_cat_bleed         = find_spell( 405233 );

  // Balance Abilities
  spec.astral_power             = find_specialization_spell( "Astral Power" );
  spec.celestial_alignment      = talent.celestial_alignment.find_override_spell();
  spec.crashing_star_dmg        = check( talent.crashing_star.ok() && talent.shooting_stars.ok(), 468981 );
  spec.eclipse_lunar            = check( talent.eclipse, 48518 );
  spec.eclipse_solar            = check( talent.eclipse, 48517 );
  spec.full_moon                = check( talent.new_moon, 274283 );
  spec.half_moon                = check( talent.new_moon, 274282 );
  spec.incarnation_moonkin      = check( talent.incarnation_moonkin, 102560 );
  spec.moonkin_form             = find_specialization_spell( "Moonkin Form" );
  spec.shooting_stars_dmg       = check( talent.shooting_stars, 202497 );  // shooting stars damage
  spec.stellar_amplification    = check( talent.stellar_amplification, 450214 );
  spec.waning_twilight          = check( talent.waning_twilight, 393957 );
  spec.starfall                 = !is_ptr() ? check( talent.starfall, 191034 ) : find_specialization_spell( "Starfall" );

  // Feral Abilities
  spec.adaptive_swarm_damage    = check( talent.adaptive_swarm, 391889 );
  spec.adaptive_swarm_heal      = check( talent.adaptive_swarm, 391891 );
  spec.feral_overrides          = find_specialization_spell( "Feral Overrides Passive" );
  spec.ashamanes_guidance       = check( talent.ashamanes_guidance, talent.convoke_the_spirits.ok() ? 391538 : 421440 );
  spec.ashamanes_guidance_buff  = check( talent.ashamanes_guidance, 421442 );
  spec.berserk_cat              = talent.berserk.find_override_spell();
  spec.sabertooth               = check( talent.sabertooth, 391722 );

  // Guardian Abilities
  spec.bear_form_2              = find_rank_spell( "Bear Form", "Rank 2" );
  spec.berserk_bear             = check( talent.berserk_ravage.ok() ||
                                         talent.berserk_unchecked_aggression.ok() ||
                                         talent.berserk_persistence.ok(), 50334 );
  spec.elunes_favored           = check( talent.elunes_favored, 370588 );
  spec.fury_of_nature           = check( talent.fury_of_nature, 370701 );
  spec.incarnation_bear         = check( talent.incarnation_bear, 102558 );
  spec.lightning_reflexes       = find_specialization_spell( "Lightning Reflexes" );
  spec.tooth_and_claw_debuff    = find_trigger( find_trigger( talent.tooth_and_claw ).trigger() ).trigger();
  spec.ursine_adept             = find_specialization_spell( "Ursine Adept" );

  // Restoration Abilities
  spec.cenarius_guidance        = check( talent.cenarius_guidance, talent.convoke_the_spirits.ok() ? 393374 : 393381 );

  // Hero Talents
  spec.atmospheric_exposure     = check( talent.atmospheric_exposure, 430589 );
  spec.bloodseeker_vines        = check( talent.thriving_growth, 439531 );
  spec.dreadful_wound           = check( talent.dreadful_wound, specialization() == DRUID_FERAL ? 441812 : 451177 );

  // Masteries ==============================================================
  mastery.harmony             = find_mastery_spell( DRUID_RESTORATION );
  mastery.natures_guardian    = find_mastery_spell( DRUID_GUARDIAN );
  mastery.natures_guardian_AP = check( mastery.natures_guardian, 159195 );
  mastery.razor_claws         = find_mastery_spell( DRUID_FERAL );
  mastery.astral_invocation   = find_mastery_spell( DRUID_BALANCE );

  eclipse_handler.init();  // initialize this here since we need talent info to properly init
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

  player_t::init_base_stats();

  switch ( specialization() )
  {
    case DRUID_BALANCE:
    case DRUID_RESTORATION: base.spell_power_per_intellect = 1.0; break;
    case DRUID_FERAL:
    case DRUID_GUARDIAN:    base.attack_power_per_agility = 1.0;  break;
    default: break;
  }

  // Passive Talents & Spells
  auto crit = find_effect( find_specialization_spell( "Critical Strike" ), A_MOD_ALL_CRIT_CHANCE ).percent();
  base.spell_crit_chance  += crit;
  base.attack_crit_chance += crit;
  base.armor_multiplier   *= 1.0 + find_effect( talent.killer_instinct, A_MOD_BASE_RESISTANCE_PCT ).percent();

  // Resources
  resources.base[ RESOURCE_RAGE ] = 100 +
    find_effect( talent.fount_of_strength, A_MOD_INCREASE_RESOURCE, POWER_RAGE ).resource();
  resources.base[ RESOURCE_COMBO_POINT ] = 5;
  resources.base[ RESOURCE_ASTRAL_POWER ] = 100 +
    find_effect( talent.astral_communion, A_MOD_MAX_RESOURCE, POWER_ASTRAL_POWER ).resource() +
    find_effect( talent.expansiveness, A_MOD_MAX_RESOURCE, POWER_ASTRAL_POWER ).resource();
  resources.base[ RESOURCE_ENERGY ] = 100 +
    find_effect( talent.tireless_energy, A_MOD_INCREASE_RESOURCE, POWER_ENERGY ).resource() +
    find_effect( talent.fount_of_strength, A_MOD_MAX_RESOURCE, POWER_ENERGY ).resource();

  resources.base_multiplier[ RESOURCE_MANA ] = 1.0 +
    find_effect( talent.expansiveness, A_MOD_MANA_POOL_PCT ).percent();

  // only intially activate required resources. others will be dynamically activated depending on apl
  for ( auto r = RESOURCE_HEALTH; r < RESOURCE_MAX; r++ )
  {
    switch ( r )
    {
      case RESOURCE_ASTRAL_POWER: resources.active_resource[ r ] = specialization() == DRUID_BALANCE;     break;
      case RESOURCE_COMBO_POINT:
      case RESOURCE_ENERGY:       resources.active_resource[ r ] = specialization() == DRUID_FERAL;       break;
      case RESOURCE_HEALTH:
      case RESOURCE_RAGE:         resources.active_resource[ r ] = specialization() == DRUID_GUARDIAN;    break;
      case RESOURCE_MANA:         resources.active_resource[ r ] = specialization() == DRUID_RESTORATION; break;
      default:                    resources.active_resource[ r ] = false;                                 break;
    }
  }

  // Energy Regen
  resources.base_regen_per_second[ RESOURCE_ENERGY ] = 10;
  resources.base_regen_per_second[ RESOURCE_ENERGY ] *=
    1.0 + find_effect( spec.feral_affinity, A_MOD_POWER_REGEN_PERCENT ).percent();
  resources.base_regen_per_second[ RESOURCE_ENERGY ] *=
    1.0 + find_effect( spec_spell, A_MOD_POWER_REGEN_PERCENT ).percent();
  resources.base_regen_per_second[ RESOURCE_ENERGY ] *=
    1.0 + find_effect( talent.tireless_energy, A_MOD_POWER_REGEN_PERCENT ).percent();

  base_gcd = 1.5_s;
}

void druid_t::init_stats()
{
  player_t::init_stats();

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
}

void druid_t::init_finished()
{
  player_t::init_finished();

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
        if ( a->harmful && ( !a->if_expr || a->if_expr->success() ) && a->action_ready() )
          wr->harmful = false;  // more harmful actions exist, set current wrath to non-harmful so we can keep casting

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
    ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_TAKEN )
    ->set_refresh_behavior( buff_refresh_behavior::DURATION )
    ->set_tick_behavior( buff_tick_behavior::NONE );
  if ( talent.brambles.ok() )
    buff.barkskin->set_tick_behavior( buff_tick_behavior::REFRESH );

  buff.bear_form = make_buff<bear_form_buff_t>( this );

  buff.cat_form = make_buff<cat_form_buff_t>( this );

  buff.dash = make_buff<druid_buff_t>( this, "dash", find_class_spell( "Dash" ) )
    ->set_cooldown( 0_ms )
    ->set_default_value_from_effect_type( A_MOD_INCREASE_SPEED );

  buff.prowl = make_buff<druid_buff_t>( this, "prowl", find_class_spell( "Prowl" ) );

  // Class
  buff.forestwalk =
    make_fallback( talent.forestwalk.ok(), this, "forestwalk", find_trigger( talent.forestwalk ).trigger() )
      ->set_default_value_from_effect_type( A_MOD_SPEED_ALWAYS, P_MAX, 0.0, E_APPLY_AREA_AURA_PARTY );

  buff.heart_of_the_wild =
    make_fallback( talent.heart_of_the_wild.ok(), this, "heart_of_the_wild", talent.heart_of_the_wild )
      ->set_cooldown( 0_ms );
  if ( specialization() == DRUID_FERAL )
  {
    buff.heart_of_the_wild->set_period( 0_ms );
  }
  else
  {
    buff.heart_of_the_wild->set_tick_callback(
      [ g = get_gain( "Heart of the Wild" ), this ]( buff_t*, int, timespan_t ) {
        if ( get_form() == CAT_FORM )
          resource_gain( RESOURCE_COMBO_POINT, 1, g );
      } );
  }

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

  buff.moonkin_form = make_fallback<moonkin_form_buff_t>( spec.moonkin_form->ok(), this, "moonkin_form" );

  buff.natures_vigil = make_fallback( talent.natures_vigil.ok(), this, "natures_vigil", talent.natures_vigil )
    ->set_default_value( 0 )
    ->set_cooldown( 0_ms )
    ->set_freeze_stacks( true );

  buff.rising_light_falling_night_day = make_fallback( talent.rising_light_falling_night.ok(),
    this, "rising_light_falling_night__day", find_spell( 417714 ) );

  buff.rising_light_falling_night_night = make_fallback( talent.rising_light_falling_night.ok(),
    this, "rising_light_falling_night__night", find_spell( 417715 ) )
      ->set_default_value_from_effect_type( A_MOD_VERSATILITY_PCT )
      ->set_pct_buff_type( STAT_PCT_BUFF_VERSATILITY );

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
        [ this,
          mul = 1.0 + find_trigger( talent.ursine_vigor ).percent() ]
        ( buff_t*, int old_, int new_ ) {
          if ( !old_ )
            adjust_health_pct( mul, true );
          else if ( !new_ )
            adjust_health_pct( mul, false );
        } );

  buff.wild_charge_movement = make_fallback( talent.wild_charge.ok(), this, "wild_charge_movement" );

  // Multi-spec
  // The buff ID in-game is same as the talent, 61336, but the buff effects (as well as tooltip reference) is in 50322
  buff.survival_instincts =
    make_fallback( talent.survival_instincts.ok(), this, "survival_instincts", talent.survival_instincts )
      ->set_cooldown( 0_ms )
      ->set_default_value( find_effect( find_spell( 50322 ), A_MOD_DAMAGE_PERCENT_TAKEN ).percent() +
                           find_effect( talent.oakskin, find_spell( 50322 ), A_ADD_FLAT_MODIFIER ).percent() );

  // Balance buffs
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
    make_fallback( talent.celestial_alignment.ok(), this, "celestial_alignment", spec.celestial_alignment );

  buff.incarnation_moonkin =
    make_fallback( talent.incarnation_moonkin.ok(), this, "incarnation_chosen_of_elune", spec.incarnation_moonkin );

  buff.ca_inc = talent.incarnation_moonkin.ok() ? buff.incarnation_moonkin : buff.celestial_alignment;
  buff.ca_inc->set_cooldown( 0_ms )
    ->set_stack_change_callback( [ this ]( buff_t* b, int old_, int new_ ) {
      if ( !old_ )
      {
        // cannot use remains() here as expiration is not yet set when stack change callbacks are run
        auto d = b->buff_duration();

        // advance eclipse manually if refreshing as stack_change_callback is not called
        auto in_lunar = eclipse_handler.in_lunar();
        auto in_solar = eclipse_handler.in_solar();

        buff.eclipse_lunar->trigger( d );
        if ( in_lunar )
          eclipse_handler.advance_eclipse<eclipse_e::LUNAR>( true );
        eclipse_handler.update_eclipse<eclipse_e::LUNAR>();

        buff.eclipse_solar->trigger( d );
        if ( in_solar )
          eclipse_handler.advance_eclipse<eclipse_e::SOLAR>( true );
        eclipse_handler.update_eclipse<eclipse_e::SOLAR>();

        if ( active.orbital_strike )
          active.orbital_strike->execute_on_target( target );
      }
      else if ( !new_ )
      {
        buff.eclipse_lunar->expire();
        buff.eclipse_solar->expire();
      }
    } );

  if ( talent.celestial_alignment.ok() )
  {
    buff.ca_inc->set_default_value_from_effect_type( A_HASTE_ALL )
      ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );
  }

  buff.denizen_of_the_dream =
    make_fallback( talent.denizen_of_the_dream.ok(), this, "denizen_of_the_dream", find_spell( 394076 ) )
      ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
      ->set_max_stack( 10 )
      ->set_rppm( rppm_scale_e::RPPM_DISABLE );

  buff.dreamstate = make_fallback( talent.natures_grace.ok(), this, "dreamstate", find_spell( 450346 ) )
    ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
  buff.dreamstate->set_initial_stack( buff.dreamstate->max_stack() );

  buff.eclipse_lunar = make_fallback( talent.eclipse.ok(), this, "eclipse_lunar", spec.eclipse_lunar )
    ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_GENERIC )
    ->set_refresh_behavior( buff_refresh_behavior::DURATION )
    ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT )
    ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
      eclipse_handler.advance_eclipse<eclipse_e::LUNAR>( new_ );
    } );

  buff.eclipse_solar = make_fallback( talent.eclipse.ok(), this, "eclipse_solar", spec.eclipse_solar )
    ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_GENERIC )
    ->set_refresh_behavior( buff_refresh_behavior::DURATION )
    ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT )
    ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
      eclipse_handler.advance_eclipse<eclipse_e::SOLAR>( new_ );
    } );

  buff.fury_of_elune =
    make_fallback<fury_of_elune_buff_t>( talent.fury_of_elune.ok() || talent.the_light_of_elune.ok(),
      this, "fury_of_elune", find_spell( 202770 ) );  // hardcoded for the light of elune

  buff.sundered_firmament =
    make_fallback<fury_of_elune_buff_t>( talent.sundered_firmament.ok(),
      this, "sundered_firmament", find_spell( 394108 ) )
        ->set_refresh_behavior( buff_refresh_behavior::EXTEND );

  buff.parting_skies = make_fallback( talent.sundered_firmament.ok(), this, "parting_skies", find_spell( 395110 ) )
    ->set_reverse( true )
    ->set_expire_callback( [ this ]( buff_t*, int, timespan_t ) {
      active.sundered_firmament->execute_on_target( target );
    } );

  int harmony_of_the_heavens_max_stacks =
    !talent.harmony_of_the_heavens.ok() ? 1 : ( as<int>( talent.harmony_of_the_heavens->effectN( 2 ).base_value() /
                                                         talent.harmony_of_the_heavens->effectN( 1 ).base_value() ) );

  buff.harmony_of_the_heavens_lunar = make_fallback( talent.harmony_of_the_heavens.ok(),
    this, "harmony_of_the_heavens_lunar", talent.harmony_of_the_heavens )
      ->set_default_value_from_effect( 1 )
      ->set_max_stack( harmony_of_the_heavens_max_stacks )
      ->set_name_reporting( "Lunar" )
      ->set_stack_change_callback( [ this ]( buff_t*, int, int ) {
        eclipse_handler.update_eclipse<eclipse_e::LUNAR>();
      } );

  buff.harmony_of_the_heavens_solar = make_fallback( talent.harmony_of_the_heavens.ok(),
    this, "harmony_of_the_heavens_solar", talent.harmony_of_the_heavens )
      ->set_default_value_from_effect( 1 )
      ->set_max_stack( harmony_of_the_heavens_max_stacks )
      ->set_name_reporting( "Solar" )
      ->set_stack_change_callback( [ this ]( buff_t*, int, int ) {
        eclipse_handler.update_eclipse<eclipse_e::SOLAR>();
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
          cap = talent.natures_balance->effectN( 2 ).percent(),
          g = get_gain( "Natures Balance" ),
          this ]
        ( buff_t*, int, timespan_t ) mutable {
          if ( !in_combat )
          {
            if ( resources.current[ RESOURCE_ASTRAL_POWER ] < resources.base[ RESOURCE_ASTRAL_POWER ] * cap )
              ap *= 3.0;
            else
              ap = 0;
          }
          resource_gain( RESOURCE_ASTRAL_POWER, ap, g );
        } );
  }

  buff.orbit_breaker = make_fallback( talent.orbit_breaker.ok(), this, "orbit_breaker" )
    ->set_quiet( true )
    ->set_max_stack( std::max( 1, as<int>( talent.orbit_breaker->effectN( 1 ).base_value() ) ) );

  buff.owlkin_frenzy = make_fallback( spec.moonkin_form->ok(), this, "owlkin_frenzy", find_spell( 157228 ) );

  buff.shooting_stars_moonfire = make_fallback<shooting_stars_buff_t>( talent.shooting_stars.ok(),
    this, "shooting_stars_moonfire", dot_lists.moonfire, active.shooting_stars_moonfire, active.crashing_star_moonfire );

  buff.shooting_stars_sunfire = make_fallback<shooting_stars_buff_t>( talent.shooting_stars.ok() && talent.sunfire.ok(),
    this, "shooting_stars_sunfire", dot_lists.sunfire, active.shooting_stars_sunfire, active.crashing_star_sunfire );

  buff.solstice = make_fallback( talent.solstice.ok(), this, "solstice", find_trigger( talent.solstice ).trigger() )
    ->set_default_value( find_trigger( talent.solstice ).percent() );

  bool make_starfall = talent.starfall.ok() || ( talent.convoke_the_spirits.ok() && spec.moonkin_form->ok() );
  // lookup via spellid for convoke
  buff.starfall = make_fallback( !is_ptr() ? make_starfall : spec.starfall->ok(), this, "starfall", find_spell( 191034 ) )
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

  buff.touch_the_cosmos =
    make_fallback( is_ptr() && talent.touch_the_cosmos.ok(), this, "touch_the_cosmos", find_spell( 450360 ) )
      ->set_trigger_spell( talent.touch_the_cosmos );

  buff.touch_the_cosmos_starfall = make_fallback( talent.touch_the_cosmos.ok() && talent.starfall.ok(),
    this, "touch_the_cosmos_starfall", find_spell( 450361 ) )
      ->set_chance( talent.touch_the_cosmos->effectN( 2 ).percent() )
      ->set_name_reporting( "Starfall" )
      ->set_trigger_spell( talent.touch_the_cosmos );

  buff.touch_the_cosmos_starsurge = make_fallback( talent.touch_the_cosmos.ok(),
    this, "touch_the_cosmos_starsurge", find_spell( 450360 ) )
      ->set_chance( talent.touch_the_cosmos->effectN( 1 ).percent() )
      ->set_name_reporting( "Starsurge" )
      ->set_trigger_spell( talent.touch_the_cosmos );

  buff.umbral_embrace =
    make_fallback( talent.umbral_embrace.ok(), this, "umbral_embrace", find_trigger( talent.umbral_embrace ).trigger() )
      ->set_trigger_spell( talent.umbral_embrace )
      ->set_chance( 0.2 )        // TODO: harcoded value
      ->set_default_value( 0 );  // value used to indicate if the proc happened during eclipse (1) or not (0)

  buff.umbral_inspiration =
    make_fallback( talent.umbral_inspiration.ok(), this, "umbral_inspiration", find_spell( 450419 ) );

  buff.warrior_of_elune =
    make_fallback( talent.warrior_of_elune.ok(), this, "warrior_of_elune", talent.warrior_of_elune )
      ->set_reverse( true );

  // Feral buffs
  buff.ashamanes_guidance = make_fallback( talent.ashamanes_guidance.ok() && talent.incarnation_cat.ok(),
    this, "ashamanes_guidance", spec.ashamanes_guidance_buff );

  buff.apex_predators_craving = make_fallback( talent.apex_predators_craving.ok(),
    this, "apex_predators_craving", find_trigger( talent.apex_predators_craving ).trigger() );

  buff.berserk_cat =
    make_fallback( talent.berserk.ok(), this, "berserk_cat", spec.berserk_cat )
      ->set_name_reporting( "berserk" )
      ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
        if ( !new_ )
        {
          gain.overflowing_power->overflow[ RESOURCE_COMBO_POINT ] += buff.overflowing_power->check();
          buff.overflowing_power->expire();
        }
      } );

  buff.incarnation_cat =
    make_fallback( talent.incarnation_cat.ok(), this, "incarnation_avatar_of_ashamane", talent.incarnation_cat )
      ->set_default_value_from_effect_type( A_ADD_PCT_MODIFIER, P_RESOURCE_COST )
      ->set_stack_change_callback(
        [ this,
          ag_dur = timespan_t::from_seconds( find_spell( 421440 )->effectN( 1 ).base_value() ) ]
        ( buff_t*, int old_, int new_ ) {
          if ( !old_ )
          {
            buff.ashamanes_guidance->trigger();
          }
          else if ( !new_ )
          {
            gain.overflowing_power->overflow[ RESOURCE_COMBO_POINT ] += buff.overflowing_power->check();
            buff.overflowing_power->expire();

            if ( talent.ashamanes_guidance.ok() )
            {
              // ashamanes guidance buff in spell-data is infinite, so we manually expire after 30 seconds here
              // unfortunately, if incarn procs or is casted during this 30 seconds so check back if this becomes
              // relevant
              make_event( *sim, ag_dur, [ this ]() { buff.ashamanes_guidance->expire(); } );
            }
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

  buff.bloodtalons     = make_fallback( talent.bloodtalons.ok(), this, "bloodtalons", find_spell( 145152 ) );
  buff.bt_rake         = make_fallback<bt_dummy_buff_t>( talent.bloodtalons.ok(), this, "bt_rake" );
  buff.bt_shred        = make_fallback<bt_dummy_buff_t>( talent.bloodtalons.ok(), this, "bt_shred" );
  buff.bt_swipe        = make_fallback<bt_dummy_buff_t>( talent.bloodtalons.ok(), this, "bt_swipe" );
  buff.bt_thrash       = make_fallback<bt_dummy_buff_t>( talent.bloodtalons.ok(), this, "bt_thrash" );
  buff.bt_moonfire     = make_fallback<bt_dummy_buff_t>( talent.bloodtalons.ok(), this, "bt_moonfire" );
  buff.bt_feral_frenzy = make_fallback<bt_dummy_buff_t>( talent.bloodtalons.ok(), this, "bt_feral_frenzy" );

  // 1.05s ICD per https://github.com/simulationcraft/simc/commit/b06d0685895adecc94e294f4e3fcdd57ac909a10
  buff.clearcasting_cat = make_fallback( talent.omen_of_clarity_cat.ok(),
    this, "clearcasting_cat", find_trigger( talent.omen_of_clarity_cat ).trigger() )
      ->set_cooldown( 1.05_s )
      ->set_name_reporting( "clearcasting" );

  buff.coiled_to_spring = make_fallback( talent.coiled_to_spring.ok(), this, "coiled_to_spring", find_spell( 449538 ) );

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

  buff.overflowing_power = make_fallback( talent.berserk.ok(), this, "overflowing_power", find_spell( 405189 ) )
    ->set_expire_callback( [ this ]( buff_t*, int s, timespan_t ) {
      resource_gain( RESOURCE_COMBO_POINT, s, get_gain( "Overflowing Power" ) );
    } );

  // TODO: confirm this is how the value is calculated
  auto predator_buff = find_trigger( talent.predator ).trigger();
  buff.predator = make_fallback( talent.predator.ok(), this, "predator", predator_buff )
    ->set_quiet( true )
    ->set_tick_zero( true )
    ->set_freeze_stacks( true )  // prevent buff_t::bump it buff_t::tick_t overwriting current value
    ->set_tick_callback( [ this, mul = predator_buff->effectN( 2 ).percent() ]( buff_t* b, int, timespan_t ) {
      b->current_value = ( 1.0 / composite_melee_haste() - 1.0 ) * mul;
    } );

  buff.predatory_swiftness =
    make_fallback( talent.predatory_swiftness.ok(), this, "predatory_swiftness", find_spell( 69369 ) )
      ->set_chance( talent.predatory_swiftness->effectN( 3 ).percent() )
      ->set_trigger_spell( talent.predatory_swiftness );

  buff.savage_fury =
    make_fallback( talent.savage_fury.ok(), this, "savage_fury", find_trigger( talent.savage_fury ).trigger() )
      ->set_default_value_from_effect_type( A_HASTE_ALL )
      ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );

  buff.sudden_ambush =
    make_fallback( talent.sudden_ambush.ok(), this, "sudden_ambush", find_trigger( talent.sudden_ambush ).trigger() )
      ->set_cooldown( talent.sudden_ambush->internal_cooldown() )
      ->set_chance( talent.sudden_ambush->effectN( 1 ).percent() )
      ->set_trigger_spell( talent.sudden_ambush );

  buff.tigers_fury = make_fallback( talent.tigers_fury.ok(), this, "tigers_fury", talent.tigers_fury )
    ->set_cooldown( 0_ms );

  buff.tigers_tenacity = make_fallback( talent.tigers_tenacity.ok(),
    this, "tigers_tenacity", find_trigger( talent.tigers_tenacity ).trigger() )
      ->set_reverse( true );
  buff.tigers_tenacity->set_stack_change_callback(
    [ this,
      g = get_gain( "Tiger's Tenacity" ),
      amt = find_effect( find_trigger( buff.tigers_tenacity ).trigger(), E_ENERGIZE ).resource() ]
    ( buff_t*, int old_, int new_ ) {
      if ( old_ > new_ )
        resource_gain( RESOURCE_COMBO_POINT, amt, g );
    } );

  auto cat_tww1_2pc = sets->set( DRUID_FERAL, TWW1, B2 );
  buff.tigers_strength = make_fallback( sets->has_set_bonus( DRUID_FERAL, TWW1, B2 ),
    this, "tigers_strength", find_trigger( cat_tww1_2pc ).trigger() )
      ->set_trigger_spell( cat_tww1_2pc )
      ->set_freeze_stacks( true )  // prevent buff_t::bump it buff_t::tick_t overwriting current value
      ->set_default_value_from_effect_type( A_MOD_ALL_CRIT_CHANCE )
      ->set_pct_buff_type( STAT_PCT_BUFF_CRIT )
      ->set_tick_callback( [ mod = cat_tww1_2pc->effectN( 1 ).percent() ]( buff_t* b, int, timespan_t ) {
        b->current_value += mod;
        b->invalidate_cache();
      } );

  auto cat_tww1_4pc = sets->set( DRUID_FERAL, TWW1, B4 );
  buff.fell_prey = make_fallback( sets->has_set_bonus( DRUID_FERAL, TWW1, B4 ),
    this, "fell_prey", find_trigger( cat_tww1_4pc ).trigger() )
      ->set_trigger_spell( cat_tww1_4pc )
      ->set_cooldown( cat_tww1_4pc->internal_cooldown() );

  // Guardian buffs
  buff.after_the_wildfire = make_fallback( talent.after_the_wildfire.ok(), this, "after_the_wildfire",
                                           talent.after_the_wildfire->effectN( 1 ).trigger() )
                              ->set_default_value( talent.after_the_wildfire->effectN( 2 ).base_value() );

  buff.berserk_bear =
    make_fallback<berserk_bear_buff_t>( talent.berserk_ravage.ok(), this, "berserk_bear" );

  buff.incarnation_bear =
    make_fallback<incarnation_bear_buff_t>( talent.incarnation_bear.ok(), this, "incarnation_guardian_of_ursoc" );

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
          if ( auto n = as<double>( dot_lists.thrash_bear.size() ) )
            resource_gain( RESOURCE_RAGE, std::min( cap, n ) * rage, g );
        } );

  buff.brambles = make_fallback<brambles_buff_t>( talent.brambles.ok(), this, "brambles" );

  buff.bristling_fur = make_fallback( talent.bristling_fur.ok(), this, "bristling_fur", talent.bristling_fur )
    ->set_cooldown( 0_ms );

  buff.dream_of_cenarius = make_fallback( talent.dream_of_cenarius_bear.ok(),
    this, "dream_of_cenarius", talent.dream_of_cenarius_bear->effectN( 1 ).trigger() );

  buff.earthwarden = make_fallback<earthwarden_buff_t>( talent.earthwarden.ok(), this, "earthwarden" );

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

  buff.galactic_guardian =
    make_fallback( talent.galactic_guardian.ok(), this, "galactic_guardian", find_spell( 213708 ) );

  buff.gore = make_fallback( talent.gore.ok(), this, "gore", find_spell( 93622 ) )
    ->set_trigger_spell( talent.gore )
    ->set_cooldown( talent.gore->internal_cooldown() )
    ->set_chance( talent.gore->effectN( 1 ).percent() );

  buff.gory_fur = make_fallback( talent.gory_fur.ok(), this, "gory_fur", find_trigger( talent.gory_fur ).trigger() )
    ->set_trigger_spell( talent.gory_fur );

  buff.guardian_of_elune = make_fallback( talent.guardian_of_elune.ok(),
    this, "guardian_of_elune", find_trigger( talent.guardian_of_elune ).trigger() )
      ->set_trigger_spell( talent.guardian_of_elune );

  buff.lunar_beam = make_fallback( talent.lunar_beam.ok(), this, "lunar_beam", talent.lunar_beam )
    ->set_cooldown( 0_ms )
    ->set_default_value_from_effect_type( A_MOD_MASTERY_PCT )
    ->set_pct_buff_type( STAT_PCT_BUFF_MASTERY );

  buff.rage_of_the_sleeper =
    make_fallback<rage_of_the_sleeper_buff_t>( talent.rage_of_the_sleeper.ok(), this, "rage_of_the_sleeper" );

  buff.tooth_and_claw =
    make_fallback( talent.tooth_and_claw.ok(), this, "tooth_and_claw", find_trigger( talent.tooth_and_claw ).trigger() )
      ->set_chance( find_trigger( talent.tooth_and_claw ).percent() );

  // TODO: figure out more elegant fallback method
  buff.ursocs_fury =
    make_fallback<druid_absorb_buff_t>( talent.ursocs_fury.ok(), this, "ursocs_fury", find_spell( 372505 ) );
  if ( talent.ursocs_fury.ok() )
    debug_cast<druid_absorb_buff_t*>( buff.ursocs_fury )->set_cumulative( true );

  buff.vicious_cycle_mangle =
    make_fallback( talent.vicious_cycle.ok(), this, "vicious_cycle_mangle", find_spell( 372019 ) )
      ->set_default_value( talent.vicious_cycle->effectN( 1 ).percent() )
      ->set_name_reporting( "Mangle" )
      ->set_trigger_spell( talent.vicious_cycle );

  buff.vicious_cycle_maul =
    make_fallback( talent.vicious_cycle.ok(), this, "vicious_cycle_maul", find_spell( 372015 ) )
      ->set_default_value( talent.vicious_cycle->effectN( 1 ).percent() )
      ->set_name_reporting( "Maul" )
      ->set_trigger_spell( talent.vicious_cycle );

  auto bear_tww1_2pc = sets->set( DRUID_GUARDIAN, TWW1, B2 );
  buff.guardians_tenacity = make_fallback( sets->has_set_bonus( DRUID_GUARDIAN, TWW1, B2 ),
    this, "guardians_tenacity", find_trigger( bear_tww1_2pc ).trigger() )
      ->set_trigger_spell( bear_tww1_2pc )
      ->set_cooldown( bear_tww1_2pc->internal_cooldown() )
      ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_TAKEN );

  // Restoration buffs
  buff.abundance = make_fallback( talent.abundance.ok(), this, "abundance", find_spell( 207640 ) )
    ->set_duration( 0_ms );

  buff.call_of_the_elder_druid = make_fallback( talent.call_of_the_elder_druid.ok(),
    this, "call_of_the_elder_druid", find_spell( 338643 ) );

  buff.cenarion_ward = make_fallback( talent.cenarion_ward.ok(), this, "cenarion_ward", talent.cenarion_ward );

  buff.clearcasting_tree = make_fallback( talent.omen_of_clarity_tree.ok(),
    this, "clearcasting_tree", find_trigger( talent.omen_of_clarity_tree ).trigger() )
      ->set_chance( find_trigger( talent.omen_of_clarity_tree ).percent() )
      ->set_name_reporting( "clearcasting" );

  buff.flourish = make_fallback( talent.flourish.ok(), this, "flourish", talent.flourish )
    ->set_default_value_from_effect( 2 );

  buff.incarnation_tree =
    make_fallback( talent.incarnation_tree.ok(), this, "incarnation_tree_of_life", find_spell( 5420 ) )
      ->set_duration( find_spell( 117679 )->duration() )  // 117679 is the generic incarnation shift proxy spell
      ->add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );

  buff.natures_swiftness =
    make_fallback( talent.natures_swiftness.ok(), this, "natures_swiftness", talent.natures_swiftness )
      ->set_cooldown( 0_ms )
      ->set_expire_callback( [ this ]( buff_t*, int, timespan_t ) {
        cooldown.natures_swiftness->start();
      } );

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
      ->set_name_reporting( "Blooming Infusion Damage Counter" )
      ->set_max_stack( as<int>( talent.blooming_infusion->effectN( 1 ).base_value() ) )
      ->set_expire_at_max_stack( true )
      ->set_trigger_spell( talent.blooming_infusion )
      ->set_expire_callback( [ this ]( buff_t*, int, timespan_t ) {
        buff.blooming_infusion_damage->trigger();
      } );

  buff.blooming_infusion_heal =
    make_fallback( talent.blooming_infusion.ok(), this, "blooming_infusion_heal", find_spell( 429438 ) )
      ->set_name_reporting( "Heal" );

  buff.blooming_infusion_heal_counter =
    make_fallback( talent.blooming_infusion.ok(), this, "blooming_infusion_heal_counter" )
      //->set_quiet( true )
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

  buff.bounteous_bloom = make_fallback( talent.bounteous_bloom.ok(), this, "bounteous_bloom", find_spell( 429217 ) )
    ->set_freeze_stacks( true )
    ->set_cooldown( 0_ms );
  if ( talent.bounteous_bloom.ok() )
  {
    const auto& bb_eff = find_effect( buff.bounteous_bloom, E_APPLY_AREA_AURA_PET, A_PERIODIC_ENERGIZE );
    buff.bounteous_bloom->set_default_value( bb_eff.resource() / bb_eff.period().total_seconds() )
      ->set_tick_callback(
        [ ap = bb_eff.resource(),
          g = get_gain( "Bounteous Bloom" ),
          this ]
        ( buff_t* b, int, timespan_t ) {
          resource_gain( RESOURCE_ASTRAL_POWER, ap * b->check(), g );
        } );
  }

  buff.cenarius_might =
    make_fallback( talent.cenarius_might.ok(), this, "cenarius_might", find_trigger( talent.cenarius_might ).trigger() )
      ->set_default_value_from_effect_type( A_HASTE_ALL )
      ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );

  buff.dream_burst = make_fallback( talent.dream_surge.ok(), this, "dream_burst", find_spell( 433832 ) );

  buff.harmony_of_the_grove = make_fallback( talent.harmony_of_the_grove.ok(),
    this, "harmony_of_the_grove", find_spell( specialization() == DRUID_RESTORATION ? 428737 : 428735 ) )
      ->set_cooldown( 0_ms );

  buff.implant = make_fallback( talent.implant.ok(), this, "implant", find_spell( 455496 ) );
  if ( talent.implant.ok() )
  {
    buff.tigers_fury->set_stack_change_callback( [ this ]( buff_t*, int old_, int new_ ) {
      if ( !old_ || !new_ )
        buff.implant->trigger();
    } );
  }

  buff.killing_strikes = make_fallback( talent.killing_strikes.ok(),
    this, "killing_strikes", find_trigger( talent.killing_strikes ).trigger() )
      ->set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE )
      ->add_invalidate( CACHE_ARMOR );

  buff.killing_strikes_combat =
    make_fallback( talent.killing_strikes.ok(), this, "killing_strikes_combat", find_spell( 441827 ) )
      ->set_quiet( true );

  buff.lunar_amplification =
    make_fallback( talent.lunar_amplification.ok(), this, "lunar_amplification", find_spell( 431250 ) )
      // TODO: currently uses trigger flags for can_trigger, but this should be checked in a callback
      ->set_trigger_spell( talent.lunar_amplification );

  buff.lunar_amplification_starfall = make_fallback( talent.lunar_amplification.ok() && spec.starfall->ok(),
    this, "lunar_amplification_starfall", find_spell( 432846 ) )
      ->set_quiet( true );

  buff.protective_growth =
    make_fallback( talent.protective_growth.ok(), this, "protective_growth", find_spell( 433749 ) )
      ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_TAKEN );

  buff.ravage_fb = make_fallback( talent.ravage.ok() && specialization() == DRUID_FERAL,
    this, "ravage", find_spell( 441585 ) )
      ->set_trigger_spell( talent.ravage );

  buff.ravage_maul = make_fallback( talent.ravage.ok() && specialization() == DRUID_GUARDIAN,
    this, "ravage", find_spell( 441602 ) )
      ->set_trigger_spell( talent.ravage );

  buff.root_network = make_fallback( talent.root_network.ok(), this, "root_network", find_spell( 439887 ) )
    // TODO: confirm updating behavior where all stacks are decreased at once then recalibrated on tick
    ->set_period( 0_ms );

  buff.ruthless_aggression = make_fallback( talent.ruthless_aggression.ok(),
    this, "ruthless_aggression", find_trigger( talent.ruthless_aggression ).trigger() )
      ->add_invalidate( CACHE_AUTO_ATTACK_SPEED );

  buff.strategic_infusion = make_fallback( talent.strategic_infusion.ok() && talent.tigers_fury.ok(),
    this, "strategic_infusion", find_spell( 439891 ) );

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
        [ this,
          g = get_gain( "Wildpower Surge" ),
          amt = find_effect( find_spell( 441704 ), E_ENERGIZE ).resource() ]
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
        [ this,
          g = get_gain( "Wildpower Surge" ),
          amt = find_effect( find_spell( 442562 ), E_ENERGIZE ).resource() ]
        ( buff_t*, int, int new_ ) {
          if ( !new_ )
            resource_gain( RESOURCE_RAGE, amt, g );
        } );

  buff.ursine_potential_counter = make_fallback( talent.wildpower_surge.ok() && specialization() == DRUID_FERAL,
    this, "ursine_potential_counter", find_spell( 441695 ) )
      ->set_trigger_spell( talent.wildpower_surge )
      ->set_cooldown( talent.wildpower_surge->internal_cooldown() );

  buff.wildshape_mastery =
    make_fallback( talent.wildshape_mastery.ok(), this, "wildshape_mastery", find_spell( 441685 ) );

  for ( auto b : buff_list )
    if ( b->data().ok() )
      apply_affecting_auras( *b );

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
  active.shift_to_caster->background = true;

  active.shift_to_bear = get_secondary_action<bear_form_t>( "bear_form_shift" );
  active.shift_to_bear->dual = true;

  active.shift_to_cat = get_secondary_action<cat_form_t>( "cat_form_shift" );
  active.shift_to_cat->dual = true;

  if ( spec.moonkin_form->ok() )
  {
    active.shift_to_moonkin = get_secondary_action<moonkin_form_t>( "moonkin_form_shift" );
    active.shift_to_moonkin->dual = true;
  }

  // Balance
  if ( talent.astral_smolder.ok() )
    active.astral_smolder = new astral_smolder_t( this );

  if ( talent.denizen_of_the_dream.ok() )
    active.denizen_of_the_dream = new denizen_of_the_dream_t( this );

  if ( talent.new_moon.ok() )
  {
    active.moons = new action_t( action_e::ACTION_OTHER, "moons_talent", this, talent.new_moon );
    active.moons->name_str_reporting = "Talent";
  }

  if ( talent.shooting_stars.ok() )
  {
    active.shooting_stars = new action_t( action_e::ACTION_OTHER, "shooting_stars", this, talent.shooting_stars );

    auto mf = get_secondary_action<shooting_stars_t>( "shooting_stars_moonfire", spec.shooting_stars_dmg );
    mf->name_str_reporting = "Moonfire";
    active.shooting_stars->add_child( mf );
    active.shooting_stars_moonfire = mf;

    auto sf = get_secondary_action<shooting_stars_t>( "shooting_stars_sunfire", spec.shooting_stars_dmg );
    sf->name_str_reporting = "Sunfire";
    active.shooting_stars->add_child( sf );
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
      active.shooting_stars->add_child(fm );
      active.orbit_breaker = fm;
    }

    if ( talent.crashing_star.ok() )
    {
      auto cs_mf = get_secondary_action<shooting_stars_t>( "crashing_star_moonfire", spec.crashing_star_dmg );
      cs_mf->name_str_reporting = "Moonfire";
      active.shooting_stars_moonfire->add_child( cs_mf );
      active.crashing_star_moonfire = cs_mf;

      auto cs_sf = get_secondary_action<shooting_stars_t>( "crashing_star_sunfire", spec.crashing_star_dmg );
      cs_sf->name_str_reporting = "Sunfire";
      active.shooting_stars_sunfire->add_child( cs_sf );
      active.crashing_star_sunfire = cs_sf;
    }
  }

  if ( talent.orbital_strike.ok() )
    active.orbital_strike = get_secondary_action<orbital_strike_t>( "orbital_strike" );

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
    auto shroom = get_secondary_action<wild_mushroom_t>(
      "sunseeker_mushroom", find_trigger( talent.sunseeker_mushroom ).trigger() );
    shroom->background = true;
    shroom->proc = true;
    active.sunseeker_mushroom = shroom;
  }

  // Feral
  if ( talent.apex_predators_craving.ok() )
  {
    auto apex = get_secondary_action<ferocious_bite_t>( "apex_predators_craving", flag_e::APEX );
    apex->s_data_reporting = talent.apex_predators_craving;
    active.ferocious_bite_apex = apex;
  }

  if ( talent.berserk_frenzy.ok() )
    active.frenzied_assault = get_secondary_action<frenzied_assault_t>( "frenzied_assault" );

  if ( talent.thrash.ok() && talent.thrashing_claws.ok() )
  {
    auto tc = get_secondary_action<thrash_cat_t::thrash_cat_bleed_t>( "thrashing_claws", flag_e::THRASHING );
    tc->s_data_reporting = talent.thrashing_claws;
    active.thrashing_claws = tc;
  }

  // Guardian
  if ( talent.after_the_wildfire.ok() )
    active.after_the_wildfire_heal = get_secondary_action<after_the_wildfire_heal_t>( "after_the_wildfire" );

  if ( talent.brambles.ok() )
    active.brambles_reflect = get_secondary_action<brambles_reflect_t>( "brambles" );

  if ( talent.elunes_favored.ok() )
    active.elunes_favored_heal = get_secondary_action<elunes_favored_heal_t>( "elunes_favored" );

  if ( talent.galactic_guardian.ok() )
  {
    auto gg = get_secondary_action<moonfire_t>( "galactic_guardian", flag_e::GALACTIC );
    gg->s_data_reporting = talent.galactic_guardian;
    gg->proc = true;
    active.galactic_guardian = gg;
  }

  if ( talent.tooth_and_claw.ok() )
  {
    if ( talent.maul.ok() )
    {
      auto tnc = get_secondary_action<maul_t>( "maul_tooth_and_claw", flag_e::TOOTHANDCLAW );
      tnc->s_data_reporting = talent.tooth_and_claw;
      tnc->name_str_reporting = "tooth_and_claw";
      active.maul_tooth_and_claw = tnc;
    }

    if ( talent.raze.ok() )
    {
      auto tnc = get_secondary_action<raze_t>( "raze_tooth_and_claw", flag_e::TOOTHANDCLAW );
      tnc->s_data_reporting = talent.tooth_and_claw;
      tnc->name_str_reporting = "tooth_and_claw";
      active.raze_tooth_and_claw = tnc;
    }
  }

  if ( talent.flashing_claws.ok() )
  {
    auto flash = get_secondary_action<thrash_bear_t>( "flashing_claws", flag_e::FLASHING );
    flash->s_data_reporting = talent.flashing_claws;
    flash->name_str_reporting = "flashing_claws";
    active.thrash_bear_flashing = flash;
  }

  // Restoration
  if ( talent.yseras_gift.ok() )
    active.yseras_gift = get_secondary_action<yseras_gift_t>( "yseras_gift" );

  // Hero talents
  if ( talent.boundless_moonlight.ok() && talent.lunar_beam.ok() )
    active.boundless_moonlight_heal = get_secondary_action<boundless_moonlight_heal_t>( "boundless_moonlight_heal" );

  if ( talent.bursting_growth.ok() && talent.thriving_growth.ok() )
    active.bursting_growth = get_secondary_action<bursting_growth_t>( "bursting_growth" );

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
      auto m_data = get_modified_spell( talent.implant )
        ->parse_effects( talent.resilient_flourishing );

      auto implant = get_secondary_action<bloodseeker_vines_t>( "bloodseeker_vines_implant" );
      implant->name_str_reporting = "bloodseeker_vines";
      implant->dot_duration = m_data->effectN( specialization() == DRUID_FERAL ? 1 : 2 ).time_value();
      active.bloodseeker_vines_implant = implant;

      vines->replace_stats( implant );
    }

    active.bloodseeker_vines = vines;
  }

  if ( talent.treants_of_the_moon.ok() )
  {
    auto mf = get_secondary_action<moonfire_t>( "moonfire_treants", flag_e::TREANT );
    mf->name_str_reporting = "moonfire";
    active.treants_of_the_moon_mf = mf;
  }

  player_t::create_actions();

  // stat parent/child hookups
  auto find_parent = [ this ]( action_t* action, std::string_view n ) {
    if ( action )
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
  find_parent( active.ferocious_bite_apex, "ferocious_bite" );
  find_parent( active.galactic_guardian, "moonfire" );
  find_parent( active.maul_tooth_and_claw, "maul" );
  find_parent( active.raze_tooth_and_claw, "raze" );
  find_parent( active.the_light_of_elune, "moonfire" );
  find_parent( active.thrash_bear_flashing, "thrash_bear" );
}

// Default Consumables ======================================================
std::string druid_t::default_flask() const
{
  switch ( specialization() )
  {
    case DRUID_BALANCE:     return "tempered_mastery_3";
    case DRUID_FERAL:       return "tempered_aggression_3";
    case DRUID_GUARDIAN:    return "tempered_swiftness_3";
    case DRUID_RESTORATION: return "tempered_swiftness_3";
    default:                return "disabled";
  }
}

std::string druid_t::default_potion() const
{
  switch ( specialization() )
  {
    case DRUID_BALANCE:     return "tempered_potion_3";
    case DRUID_FERAL:       return "tempered_potion_3";
    case DRUID_GUARDIAN:    return "tempered_potion_3";
    case DRUID_RESTORATION: return "tempered_potion_3";
    default:                return "disabled";
  }
}

std::string druid_t::default_food() const
{
  switch ( specialization() )
  {
    case DRUID_BALANCE:     return "stuffed_cave_peppers";
    case DRUID_FERAL:       return "mycobloom_risotto";
    case DRUID_GUARDIAN:    return "mycobloom_risotto";
    case DRUID_RESTORATION: return "stuffed_cave_peppers";
    default:                return "disabled";
  }
}

std::string druid_t::default_rune() const
{
  return "crystallized";
}

std::string druid_t::default_temporary_enchant() const
{
  std::string str = "main_hand:";

  switch ( specialization() )
  {
    case DRUID_BALANCE:     return str + "algari_mana_oil_3";
    case DRUID_FERAL:       return str + "algari_mana_oil_3";
    case DRUID_GUARDIAN:    return str + "algari_mana_oil_3";
    case DRUID_RESTORATION: return str + "algari_mana_oil_3";
    default:                return "disabled";
  }
}

// ALL Spec Pre-Combat Action Priority List =================================
void druid_t::apl_precombat()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  // Consumables
  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
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
#include "class_modules/apl/feral_apl.inc"
}

void druid_t::apl_feral_ptr()
{
#include "class_modules/apl/feral_apl_ptr.inc"
}

void druid_t::apl_balance()
{
#include "class_modules/apl/balance_apl.inc"
}

void druid_t::apl_balance_ptr()
{
#include "class_modules/apl/balance_apl_ptr.inc"
}

void druid_t::apl_guardian()
{
#include "class_modules/apl/guardian_apl.inc"
}

void druid_t::apl_restoration()
{
#include "class_modules/apl/restoration_druid_apl.inc"
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
        "\n# Balance APL can be found at https://www.dreamgrove.gg/sims/owl/balance.txt\n";
      break;
    case DRUID_FERAL:
      action_list_information +=
        "\n# Feral APL can be found at https://www.dreamgrove.gg/sims/cat/feral.txt\n";
      break;
    case DRUID_GUARDIAN:
      action_list_information +=
        "\n# Guardian APL can be found at https://www.dreamgrove.gg/sims/bear/guardian.txt\n";
      break;
    case DRUID_RESTORATION:
      action_list_information +=
        "\n# Restoration DPS APL can be found at https://www.dreamgrove.gg/sims/tree/restoration.txt\n";
      break;
    default:
      break;
  }

  if ( sim->fight_style == FIGHT_STYLE_DUNGEON_SLICE || sim->fight_style == FIGHT_STYLE_DUNGEON_ROUTE )
  {
    if ( options.adaptive_swarm_melee_targets == 7 )
      options.adaptive_swarm_melee_targets = 2;
    if ( options.adaptive_swarm_ranged_targets == 12 )
      options.adaptive_swarm_ranged_targets = 2;
  }
}

bool druid_t::validate_fight_style( fight_style_e style ) const
{
  switch ( specialization() )
  {
    case DRUID_BALANCE:
      if ( style != FIGHT_STYLE_PATCHWERK )
        return false;

      break;
    case DRUID_FERAL:
      break;

    case DRUID_GUARDIAN:
      break;

    case DRUID_RESTORATION:
      sim->error( "Restoration Druid does not yet have an Action Priority List (APL)." );
      return false;

    default:
      break;
  }

  return true;
}

// druid_t::init_gains ======================================================
void druid_t::init_gains()
{
  player_t::init_gains();

  if ( uses_bear_form() )
    gain.bear_form = get_gain( "Bear Form" );

  if ( uses_cat_form() )
    gain.energy_refund = get_gain( "Energy Refund" );

  if ( talent.berserk.ok() )
    gain.overflowing_power = get_gain( "Overflowing Power" );

  if ( talent.soul_of_the_forest_cat.ok() )
    gain.soul_of_the_forest = get_gain( "Soul of the Forest" );

  if ( talent.omen_of_clarity_cat.ok() || talent.omen_of_clarity_tree.ok() )
    gain.clearcasting = get_gain( "Clearcasting" );
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

  if ( talent.tooth_and_claw.ok() )
    uptime.tooth_and_claw_debuff = get_uptime( "Tooth and Claw Debuff" )->collect_uptime( *sim );
}

// druid_t::init_resources ==================================================
void druid_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[ RESOURCE_RAGE ] = 0;
  resources.current[ RESOURCE_COMBO_POINT ] = 0;

  if ( options.initial_astral_power == 0.0 && talent.natures_balance.ok() )
  {
    resources.current[ RESOURCE_ASTRAL_POWER ] =
      resources.base[ RESOURCE_ASTRAL_POWER ] * talent.natures_balance->effectN( 2 ).percent();
  }
  else
  {
    resources.current[ RESOURCE_ASTRAL_POWER ] = options.initial_astral_power;
  }
}


// druid_t::init_special_effects ============================================
void druid_t::init_special_effects()
{
  struct druid_cb_t : public dbc_proc_callback_t
  {
    druid_cb_t( druid_t* p, const special_effect_t& e ) : dbc_proc_callback_t( p, e ) {}

    druid_t* p() { return static_cast<druid_t*>( listener ); }
  };

  // General
  // bear form rage from being attacked
  if ( uses_bear_form() )
  {
    struct rage_from_being_attacked_cb_t final : public druid_cb_t
    {
      gain_t* gain;
      double rage;

      rage_from_being_attacked_cb_t( druid_t* p, const special_effect_t& e )
        : druid_cb_t( p, e ),
          gain( p->get_gain( "Rage from being attacked" ) ),
          rage( p->spec.bear_form_passive->effectN( 3 ).base_value() )
      {}

      void execute( action_t*, action_state_t* ) override
      {
        p()->resource_gain( RESOURCE_RAGE, rage, gain );
      }
    };

    const auto driver = new special_effect_t( this );
    driver->name_str = "rage_from_being_attacked";
    driver->spell_id = spec.bear_form_passive->id();
    special_effects.push_back( driver );

    auto cb = new rage_from_being_attacked_cb_t( this, *driver );
    cb->activate_with_buff( buff.bear_form );
  }

  if ( talent.natures_vigil.ok() )
  {
    struct natures_vigil_cb_t final : public druid_cb_t
    {
      double mul;

      natures_vigil_cb_t( druid_t* p, const special_effect_t& e )
        : druid_cb_t( p, e ), mul( p->talent.natures_vigil->effectN( 3 ).percent() )
      {}

      void trigger( action_t* a, action_state_t* s ) override
      {
        // raze procs despite being an aoe
        if ( s->result_total && ( s->n_targets == 1 || a->id == 400254 ) )
          druid_cb_t::trigger( a, s );
      }

      void execute( action_t*, action_state_t* s ) override
      {
        // counts overheal
        p()->buff.natures_vigil->current_value += s->result_total * mul;
      }
    };

    const auto driver = new special_effect_t( this );
    driver->name_str = talent.natures_vigil->name_cstr();
    driver->spell_id = talent.natures_vigil->id();
    driver->cooldown_ = 0_ms;
    driver->proc_flags2_ = PF2_ALL_HIT;

    if ( specialization() == DRUID_RESTORATION )
    {
      driver->proc_flags_ = ( PF_MAGIC_HEAL | PF_HELPFUL_PERIODIC );
      driver->proc_flags2_ |= PF2_PERIODIC_HEAL;
    }
    else
    {
      driver->proc_flags_ = talent.natures_vigil->proc_flags() & ~( PF_MAGIC_HEAL | PF_HELPFUL_PERIODIC );
      driver->proc_flags2_ |= PF2_PERIODIC_DAMAGE;
    }

    special_effects.push_back( driver );

    auto cb = new natures_vigil_cb_t( this, *driver );
    cb->activate_with_buff( buff.natures_vigil );
  }

  // Balance
  if ( talent.denizen_of_the_dream.ok() )
  {
    struct denizen_of_the_dream_cb_t final : public druid_cb_t
    {
      proc_t* catto_proc;

      denizen_of_the_dream_cb_t( druid_t* p, const special_effect_t& e )
        : druid_cb_t( p, e ), catto_proc( p->get_proc( "Denizen of the Dream" )->collect_count() )
      {}

      void trigger( action_t* a, action_state_t* s ) override
      {
        if ( a->id == p()->spec.moonfire_dmg->id() || a->id == p()->spec.sunfire_dmg->id() )
          dbc_proc_callback_t::trigger( a, s );
      }

      void execute( action_t*, action_state_t* ) override
      {
        p()->active.denizen_of_the_dream->execute();
        p()->buff.denizen_of_the_dream->trigger();
        catto_proc->occur();
      }
    };

    const auto driver = new special_effect_t( this );
    driver->name_str = talent.denizen_of_the_dream->name_cstr();
    driver->spell_id = talent.denizen_of_the_dream->id();
    special_effects.push_back( driver );

    new denizen_of_the_dream_cb_t( this, *driver );
  }

  if ( spec.moonkin_form->ok() )
  {
    struct owlkin_frenzy_cb_t final : public druid_cb_t
    {
      owlkin_frenzy_cb_t( druid_t* p, const special_effect_t& e ) : druid_cb_t( p, e ) {}

      void trigger( action_t* a, action_state_t* s ) override
      {
        if ( s->n_targets == 1 )
          druid_cb_t::trigger( a, s );
      }
    };

    const auto driver = new special_effect_t( this );
    driver->name_str = buff.owlkin_frenzy->name();
    driver->spell_id = spec.moonkin_form->id();
    driver->proc_chance_ =
      find_effect( find_specialization_spell( "Owlkin Frenzy" ), A_ADD_FLAT_MODIFIER, P_PROC_CHANCE ).percent();
    driver->custom_buff = buff.owlkin_frenzy;
    special_effects.push_back( driver );

    auto cb = new owlkin_frenzy_cb_t( this, *driver );
    cb->activate_with_buff( buff.moonkin_form );
  }

  // Feral

  // Guardian
  if ( mastery.natures_guardian->ok() )
  {
    struct natures_guardian_cb_t final : public druid_cb_t
    {
      struct natures_guardian_t final : public druid_heal_t
      {
        natures_guardian_t( druid_t* p ) : druid_heal_t( "natures_guardian", p, p->find_spell( 227034 ) )
        {
          background = true;
          callbacks = false;
          target = p;
        }
      };

      action_t* heal;

      natures_guardian_cb_t( druid_t* p, const special_effect_t& e ) : druid_cb_t( p, e )
      {
        heal = p->get_secondary_action<natures_guardian_t>( "natures_guardian" );
      }

      void trigger( action_t* a, action_state_t* s ) override
      {
        if ( a->id <= 0 || s->result_total <= 0 || a->harmful )
          return;

        if ( auto heal = debug_cast<heal_t*>( a ); heal->base_pct_heal || heal->tick_pct_heal )
          return;

        druid_cb_t::trigger( a, s );
      }

      void execute( action_t*, action_state_t* s ) override
      {
        heal->base_dd_min = heal->base_dd_max = s->result_total * p()->cache.mastery_value();
        heal->schedule_execute();
      }
    };

    const auto driver = new special_effect_t( this );
    driver->name_str = "natures_guardian";
    driver->spell_id = mastery.natures_guardian->id();
    driver->proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_HEAL;
    special_effects.push_back( driver );

    new natures_guardian_cb_t( this, *driver );
  }

  if ( talent.bristling_fur.ok() )
  {
    struct bristling_fur_cb_t final : public druid_cb_t
    {
      gain_t* gain;
      action_t* action;

      bristling_fur_cb_t( druid_t* p, const special_effect_t& e )
        : druid_cb_t( p, e ), gain( p->get_gain( "Bristling Fur" ) ), action( p->find_action( "bristling_fur" ) )
      {}

      void execute( action_t*, action_state_t* s ) override
      {
        // 1 rage per 1% of maximum health taken
        auto pct = s->result_amount / p()->resources.max[ RESOURCE_HEALTH ];

        p()->resource_gain( RESOURCE_RAGE, pct * 100, gain, action );
      }
    };

    const auto driver = new special_effect_t( this );
    driver->name_str = talent.bristling_fur->name_cstr();
    driver->spell_id = talent.bristling_fur->id();
    driver->cooldown_ = 0_ms;
    special_effects.push_back( driver );

    auto cb = new bristling_fur_cb_t( this, *driver );
    cb->activate_with_buff( buff.bristling_fur );
  }

  if ( talent.dream_of_cenarius_bear.ok() )
  {
    struct dream_of_cenarius_cb_t final : public druid_cb_t
    {
      dream_of_cenarius_cb_t( druid_t* p, const special_effect_t& e ) : druid_cb_t( p, e ) {}

      void trigger( action_t* a, action_state_t* s ) override
      {
        proc_chance = p()->cache.attack_crit_chance();

        druid_cb_t::trigger( a, s );
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
    struct elunes_favored_cb_t final : public druid_cb_t
    {
      double mul;

      elunes_favored_cb_t( druid_t* p, const special_effect_t& e )
        : druid_cb_t( p, e ), mul( p->talent.elunes_favored->effectN( 1 ).percent() )
      {}

      void trigger( action_t* a, action_state_t* s ) override
      {
        if ( p()->get_form() != BEAR_FORM || !s->result_amount )
          return;

        // Elune's Favored heals off both arcane & nature damage
        if ( !dbc::has_common_school( a->get_school(), SCHOOL_ASTRAL ) )
          return;

        druid_cb_t::trigger( a, s );
      }

      void execute( action_t*, action_state_t* s ) override
      {
        p()->buff.elunes_favored->current_value += s->result_amount * mul;
      }
    };

    const auto driver = new special_effect_t( this );
    driver->name_str = talent.elunes_favored->name_cstr();
    driver->spell_id = spec.elunes_favored->id();
    special_effects.push_back( driver );

    new elunes_favored_cb_t( this, *driver );
  }

  if ( talent.galactic_guardian.ok() )
  {
    struct galactic_guardian_cb_t final : public druid_cb_t
    {
      proc_t* gg_proc;

      galactic_guardian_cb_t( druid_t* p, const special_effect_t& e )
        : druid_cb_t( p, e ), gg_proc( p->get_proc( "Galactic Guardian" )->collect_count() )
      {}

      void trigger( action_t* a, action_state_t* s ) override
      {
        if ( a->id != p()->spec.moonfire_dmg->id() && a->id != p()->spec.moonfire->id() )
          druid_cb_t::trigger( a, s );
      }

      void execute( action_t*, action_state_t* s ) override
      {
        p()->active.galactic_guardian->execute_on_target( target( s, p()->active.galactic_guardian ) );
        p()->buff.galactic_guardian->trigger();
        gg_proc->occur();
      }
    };

    const auto driver = new special_effect_t( this );
    driver->name_str = talent.galactic_guardian->name_cstr();
    driver->spell_id = talent.galactic_guardian->id();
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

      void trigger( action_t* a, action_state_t* s ) override
      {
        if ( !s->result_amount || !p()->get_target_data( s->target )->dots.moonfire->is_ticking() )
          return;

        // raze (400254) & ravage (441605) trigger moonless night despite being an aoe spell
        // moonfire (164812) & sunfire (164815) do not trigger
        switch ( a->id )
        {
          case 164812:  // moonfire
          case 164815:  // sunfire
            return;     // end

          case 400254:  // raze
          case 441605:  // ravage
            break;      // continue

          default:
            if ( a->aoe < 0 || a->aoe > 1 )
              return;   // end
            else
              break;    // continue
        }

        druid_cb_t::trigger( a, s );
      }

      void execute( action_t*, action_state_t* s ) override
      {
        moonless->execute_on_target( s->target, s->result_amount );
      }
    };

    const auto driver = new special_effect_t( this );
    driver->name_str = talent.moonless_night->name_cstr();
    driver->spell_id = talent.moonless_night->id();
    driver->proc_flags2_ = PF2_ALL_HIT;
    driver->set_can_only_proc_from_class_abilites( true );
    special_effects.push_back( driver );

    new moonless_night_cb_t( this, *driver );
  }

  // Hero talents
  if ( talent.boundless_moonlight.ok() && talent.lunar_beam.ok() )
  {
    struct boundless_moonlight_heal_cb_t final : public druid_cb_t
    {
      double mul;

      boundless_moonlight_heal_cb_t( druid_t* p, const special_effect_t& e )
        : druid_cb_t( p, e ), mul( p->buff.boundless_moonlight_heal->data().effectN( 1 ).percent() )
      {}

      void execute( action_t*, action_state_t* s ) override
      {
        p()->buff.boundless_moonlight_heal->current_value += s->result_amount * mul;
      }
    };

    const auto driver = new special_effect_t( this );
    driver->name_str = talent.boundless_moonlight->name_cstr();
    // TODO: confirm if driver lasts for 12s as per spell data
    driver->spell_id = 425217;
    special_effects.push_back( driver );

    auto cb = new boundless_moonlight_heal_cb_t( this, *driver );
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
    struct implant_cb_t final : public druid_cb_t
    {
      implant_cb_t( druid_t* p, const special_effect_t& e ) : druid_cb_t( p, e ) {}

      // TODO: whitelist aoe spells as necessary if they can trigger
      void trigger( action_t* a, action_state_t* s ) override
      {
        if ( s->result_amount > 0 && ( a->aoe == 0 || a->aoe == 1 ) )
          druid_cb_t::trigger( a, s );
      }

      void execute( action_t* a, action_state_t* s ) override
      {
        p()->active.bloodseeker_vines_implant->execute_on_target( target( s, p()->active.bloodseeker_vines_implant ) );
        p()->buff.implant->expire( a );
      }
    };

    const auto driver = new special_effect_t( this );
    driver->name_str = talent.implant->name_cstr();
    driver->spell_id = buff.implant->data().id();
    driver->proc_flags2_ = PF2_CAST_DAMAGE;
    special_effects.push_back( driver );

    auto cb = new implant_cb_t( this, *driver );
    cb->activate_with_buff( buff.implant );
  };

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
    case DRUID_FERAL:       is_ptr() ? apl_feral_ptr() : apl_feral();     break;
    case DRUID_BALANCE:     is_ptr() ? apl_balance_ptr() : apl_balance(); break;
    case DRUID_GUARDIAN:    apl_guardian();                               break;
    case DRUID_RESTORATION: apl_restoration();                            break;
    default:                apl_default();                                break;
  }

  use_default_action_list = true;

  player_t::init_action_list();
}

// druid_t::reset ===========================================================
void druid_t::reset()
{
  player_t::reset();

  // Reset druid_t variables to their original state.
  form = NO_FORM;
  base_gcd = 1.5_s;
  eclipse_handler.reset_stacks();
  eclipse_handler.reset_state();

  // Restore main hand attack / weapon to normal state
  main_hand_attack = caster_melee_attack;
  main_hand_weapon = caster_form_weapon;

  if ( mastery.natures_guardian->ok() )
    recalculate_resource_max( RESOURCE_HEALTH );

  // Reset runtime variables
  moon_stage = static_cast<moon_stage_e>( options.initial_moon_stage );
  orbital_bug = true;
  persistent_event_delay.clear();
  astral_power_decay = nullptr;
  dot_lists.moonfire.clear();
  dot_lists.sunfire.clear();
  dot_lists.thrash_bear.clear();
  dot_lists.dreadful_wound.clear();
}

// druid_t::merge ===========================================================
void druid_t::merge( player_t& other )
{
  player_t::merge( other );

  druid_t& od = static_cast<druid_t&>( other );

  for ( size_t i = 0; i < counters.size(); i++ )
    counters[ i ]->merge( *od.counters[ i ] );

  eclipse_handler.merge( od.eclipse_handler );
}

void druid_t::datacollection_begin()
{
  player_t::datacollection_begin();

  eclipse_handler.datacollection_begin();
}

void druid_t::datacollection_end()
{
  player_t::datacollection_end();

  eclipse_handler.datacollection_end();
}

void druid_t::analyze( sim_t& sim )
{
  player_t::analyze( sim );

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

  if ( convoke_counter )
    convoke_counter->analyze();
}

// druid_t::mana_regen_per_second ===========================================
double druid_t::resource_regen_per_second( resource_e r ) const
{
  double reg = player_t::resource_regen_per_second( r );

  if ( r == RESOURCE_MANA )
  {
    if ( specialization() == DRUID_BALANCE && buff.moonkin_form->check() )
      reg *= ( 1.0 + buff.moonkin_form->data().effectN( 5 ).percent() ) / cache.spell_haste();
  }
  else if ( r == RESOURCE_ENERGY )
  {
    if ( buff.savage_fury->check() )
      reg *= 1.0 + buff.savage_fury->data().effectN( 2 ).percent();
  }

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
  if ( primary_resource() != RESOURCE_ENERGY )
    return player_t::available();

  double energy = resources.current[ RESOURCE_ENERGY ];

  if ( energy > 25 )
    return 100_ms;

  return std::max( timespan_t::from_seconds( ( 25 - energy ) / resource_regen_per_second( RESOURCE_ENERGY ) ), 100_ms );
}

// druid_t::precombat_init (called before precombat apl)=======================
void druid_t::precombat_init()
{
  player_t::precombat_init();

  if ( talent.brambles.ok() )
    buff.brambles->trigger();

  if ( talent.orbit_breaker.ok() )
  {
    auto stacks = options.initial_orbit_breaker_stacks >= 0
                    ? options.initial_orbit_breaker_stacks
                    : rng().range( 0, as<int>( talent.orbit_breaker->effectN( 1 ).base_value() ) );

    if ( stacks )
      buff.orbit_breaker->trigger( stacks );
  }

  if ( talent.rising_light_falling_night.ok() )
  {
    if ( timeofday == timeofday_e::DAY_TIME )
      buff.rising_light_falling_night_day->trigger();
    else
      buff.rising_light_falling_night_night->trigger();
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

  if ( talent.lycaras_teachings.ok() )
  {
    switch( get_form() )
    {
      case NO_FORM:      buff.lycaras_teachings_haste->trigger(); break;
      case CAT_FORM:     buff.lycaras_teachings_crit->trigger();  break;
      case BEAR_FORM:    buff.lycaras_teachings_vers->trigger();  break;
      case MOONKIN_FORM: buff.lycaras_teachings_mast->trigger();  break;
      default: break;
    }
  }

  if ( eclipse_handler.enabled() )
  {
    eclipse_handler.reset_stacks();

    eclipse_handler.uptimes[ eclipse_handler.state ]->update( true, sim->current_time() );

    if ( !options.raid_combat )
      in_boss_encounter = 0;

    if ( in_boss_encounter )
    {
      double cap =
        std::max( resources.base[ RESOURCE_ASTRAL_POWER ] * talent.natures_balance->effectN( 2 ).percent(), 20.0 );
      double curr = resources.current[ RESOURCE_ASTRAL_POWER ];

      resources.current[ RESOURCE_ASTRAL_POWER ] = std::min( cap, curr );
      if ( curr > cap )
        sim->print_debug( "Astral Power capped at combat start to {} (was {})", cap, curr );
    }

    // Fallthrough if there wasn't any precombat Starfire cast to trigger Dreamstate
    if ( !buff.dreamstate->check() )
        buff.dreamstate->trigger();
  }
/*
  if ( talent.adaptive_swarm.ok() && !prepull_swarm.empty() && find_action( "adaptive_swarm" ) )
  {
    using swarm_t = spells::adaptive_swarm_t::adaptive_swarm_heal_t;

    auto heal = get_secondary_action<swarm_t>( "adaptive_swarm_heal" );

    for ( auto [ min_stack, max_stack, min_dur, max_dur, chance ] : prepull_swarm )
    {
      if ( !rng().roll( chance ) )
        continue;

      auto stacks   = rng().range( min_stack, max_stack );
      auto duration = rng().range( min_dur, max_dur );
      auto range    = rng().roll( 0.5 ) ? rng().gauss( options.adaptive_swarm_jump_distance_melee,
                                                       options.adaptive_swarm_jump_distance_stddev, true )
                                        : rng().gauss( options.adaptive_swarm_jump_distance_ranged,
                                                       options.adaptive_swarm_jump_distance_stddev, true );
    }
  }*/
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
      if ( buff.ironfur->check() )
        invalidate_cache( CACHE_ARMOR );
      break;

    case CACHE_STAMINA:
      recalculate_resource_max( RESOURCE_HEALTH );
      break;

    default: break;
  }
}

// Composite combat stat override functions =================================

// Armor ====================================================================
double druid_t::composite_armor() const
{
  double a = player_t::composite_armor();

  if ( buff.ironfur->up() )
  {
    auto if_val = buff.ironfur->check_stack_value();

    // TODO: confirm this is dynamic
    if ( buff.killing_strikes->check() )
      if_val *= 1.0 + buff.killing_strikes->data().effectN( 2 ).percent();

    a += if_val * cache.agility();
  }

  return a;
}

// Defense ==================================================================
double druid_t::composite_dodge_rating() const
{
  double dr = player_t::composite_dodge_rating();

  if ( spec.lightning_reflexes->ok() )
    dr += composite_rating( RATING_MELEE_CRIT ) * spec.lightning_reflexes->effectN( 1 ).percent();

  return dr;
}

// Movement =================================================================
double druid_t::non_stacking_movement_modifier() const
{
  double ms = player_t::non_stacking_movement_modifier();

  if ( buff.dash->up() && buff.cat_form->check() )
    ms = std::max( ms, buff.dash->check_value() );
  else if ( buff.tiger_dash->up() && buff.cat_form->check() )
    ms = std::max( ms, buff.tiger_dash->check_value() );

  if ( buff.wild_charge_movement->check() )
    ms = std::max( ms, buff.wild_charge_movement->check_value() );

  if ( talent.flower_walk.ok() && buff.barkskin->check() )
    ms = std::max( ms, talent.flower_walk->effectN( 1 ).percent() );

  return ms;
}

double druid_t::stacking_movement_modifier() const
{
  double ms = player_t::stacking_movement_modifier();

  ms += buff.forestwalk->check_value();

  if ( buff.cat_form->check() )
    ms += spec.cat_form_speed->effectN( 1 ).percent();

  ms += talent.feline_swiftness->effectN( 1 ).percent();

  if ( racials.elusiveness->ok() && buff.prowl->check() )
    ms += racials.elusiveness->effectN( 1 ).percent();

  return ms;
}

// Expressions ==============================================================
std::unique_ptr<expr_t> druid_t::create_action_expression( action_t& a, std::string_view name_str )
{
  auto splits = util::string_split<std::string_view>( name_str, "." );

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
        throw std::invalid_argument( "invalid action specified in ticks_gained_on_refresh" );
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

    if ( dot_action->name_str == "thrash_cat" )
    {
      source_action = find_action( "thrash_cat" );
    }

    return make_fn_expr( name_str, [ dot_action, source_action, multiplier, pmul ]() -> double {
      auto ticks_gained_func = []( double mod, action_t* dot_action, player_t* target, bool pmul ) -> double {
        action_state_t* state = dot_action->get_state();
        state->target = target;
        dot_action->snapshot_state( state, result_amount_type::DMG_OVER_TIME );

        dot_t* dot = dot_action->get_dot( target );
        timespan_t ttd = target->time_to_percent( 0 );
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
        for ( player_t* target : source_action->targets_in_range_list( source_action->target_list() ) )
          accum += ticks_gained_func( multiplier, dot_action, target, pmul );

        return accum;
      }

      return ticks_gained_func( multiplier, dot_action, source_action->target, pmul );
    } );
  }

  return player_t::create_action_expression( a, name_str );
}

std::unique_ptr<expr_t> druid_t::create_expression( std::string_view name_str )
{
  auto splits = util::string_split<std::string_view>( name_str, "." );

  if ( util::str_compare_ci( splits[ 0 ], "druid" ) && splits.size() == 2 )
  {
    if ( util::str_compare_ci( splits[ 1 ], "no_cds" ) )
      return expr_t::create_constant( "no_cds", options.no_cds );
    if ( util::str_compare_ci( splits[ 1 ], "time_spend_healing" ) )
      return expr_t::create_constant( "time_spend_healing", options.time_spend_healing );
    if ( util::str_compare_ci( splits[ 1 ], "initial_orbit_breaker_stacks" ) )
      return expr_t::create_constant( "initial_orbit_breaker_stacks", options.initial_orbit_breaker_stacks );
  }

  if ( util::str_compare_ci( name_str, "combo_points" ) )
    return make_ref_expr( "combo_points", resources.current[ RESOURCE_COMBO_POINT ] );

  if ( specialization() == DRUID_BALANCE )
  {
    if ( util::str_compare_ci( name_str, "astral_power" ) )
      return make_ref_expr( name_str, resources.current[ RESOURCE_ASTRAL_POWER ] );

    // New Moon stage related expressions
    if ( util::str_compare_ci( name_str, "new_moon" ) )
      return make_fn_expr( name_str, [ this ]() { return moon_stage == NEW_MOON; } );
    else if ( util::str_compare_ci( name_str, "half_moon" ) )
      return make_fn_expr( name_str, [ this ]() { return moon_stage == HALF_MOON; } );
    else if ( util::str_compare_ci( name_str, "full_moon" ) )
      return make_fn_expr( name_str, [ this ]() { return moon_stage >= FULL_MOON; } );

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
        return make_fn_expr( name_str, [ this ]() {
          return buff.fury_of_elune->check() + buff.sundered_firmament->check();
        } );
      }
      else if ( util::str_compare_ci( splits[ 2 ], "remains" ) )
      {
        return make_fn_expr( name_str, [ this ]() {
          return std::max( buff.fury_of_elune->remains(), buff.sundered_firmament->remains() );
        } );
      }
    }

    if ( splits.size() == 2 && util::str_compare_ci( splits[ 0 ], "eclipse" ) )
    {
      if ( util::str_compare_ci( splits[ 1 ], "state" ) )
        return make_fn_expr( name_str, [ this ]() { return eclipse_handler.state; } );
      else if ( util::str_compare_ci( splits[ 1 ], "in_none" ) )
        return make_fn_expr( name_str, [ this ]() { return eclipse_handler.in_none(); } );
      else if ( util::str_compare_ci( splits[ 1 ], "in_eclipse" ) )
        return make_fn_expr( name_str, [ this ]() { return eclipse_handler.in_eclipse(); } );
      else if ( util::str_compare_ci( splits[ 1 ], "in_solar" ) )
        return make_fn_expr( name_str, [ this ]() { return eclipse_handler.in_solar(); } );
      else if ( util::str_compare_ci( splits[ 1 ], "in_lunar" ) )
        return make_fn_expr( name_str, [ this ]() { return eclipse_handler.in_lunar(); } );
      else if ( util::str_compare_ci( splits[ 1 ], "in_both" ) )
        return make_fn_expr( name_str, [ this ]() { return eclipse_handler.in_both(); } );
      else if ( util::str_compare_ci( splits[ 1 ], "starfire_counter" ) )
        return make_fn_expr( name_str, [ this ]() { return eclipse_handler.starfire_counter; } );
      else if ( util::str_compare_ci( splits[ 1 ], "wrath_counter" ) )
        return make_fn_expr( name_str, [ this ]() { return eclipse_handler.wrath_counter; } );
    }
  }
  else if ( specialization() == DRUID_FERAL )
  {
    if ( util::str_compare_ci( name_str, "active_bt_triggers" ) )
    {
      return make_fn_expr( "active_bt_triggers", [ this ]() {
        return buff.bt_rake->check() + buff.bt_shred->check() + buff.bt_swipe->check() + buff.bt_thrash->check() +
               buff.bt_moonfire->check() + buff.bt_feral_frenzy->check();
      } );
    }

    if ( util::str_compare_ci( name_str, "bt_trigger_remains" ) )
    {
      return make_fn_expr( "bt_trigger_remains", [ this ]() {
        return std::min( std::initializer_list<double>( {
          buff.bt_rake->check() ? buff.bt_rake->remains().total_seconds() : 6.0,
          buff.bt_shred->check() ? buff.bt_shred->remains().total_seconds() : 6.0,
          buff.bt_moonfire->check() ? buff.bt_moonfire->remains().total_seconds() : 6.0,
          buff.bt_swipe->check() ? buff.bt_swipe->remains().total_seconds() : 6.0,
          buff.bt_thrash->check() ? buff.bt_thrash->remains().total_seconds() : 6.0,
          buff.bt_feral_frenzy->check() ? buff.bt_feral_frenzy->remains().total_seconds() : 6.0
        } ) );
      } );
    }

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

  return player_t::create_expression( name_str );
}

static bool parse_swarm_setup( sim_t* sim, std::string_view, std::string_view setup_str )
{
  sim->error( "druid.adaptive_swarm_prepull_setup is temporarily disabled." );
  return false;

  auto entries = util::string_split<std::string_view>( setup_str, "/" );

  for ( auto entry : entries )
  {
    auto values = util::string_split<std::string_view>( entry, ":" );

    try
    {
      if ( values.size() != 5 )
        throw std::invalid_argument( "Missing value." );

      auto min_stack = std::clamp( util::to_unsigned( values[ 0 ] ), 1U, 5U );
      auto max_stack = std::clamp( util::to_unsigned( values[ 1 ] ), 1U, 5U );
      auto min_dur = timespan_t::from_seconds( std::clamp( util::to_double( values[ 2 ] ), 0.0, 12.0 ) );
      auto max_dur = timespan_t::from_seconds( std::clamp( util::to_double( values[ 3 ] ), 0.0, 12.0 ) );
      auto chance = std::clamp( util::to_double( values[ 4 ] ), 0.0, 1.0 );

      static_cast<druid_t*>( sim->active_player )
        ->prepull_swarm.emplace_back( min_stack, max_stack, min_dur, max_dur, chance );
    }
    catch ( const std::invalid_argument& msg )
    {
      throw std::invalid_argument(
          fmt::format( "\n\tInvalid entry '{}' for druid.adaptive_swarm_prepull_setup. {}"
                       "\n\tFormat is <min stacks>:<max stacks>:<min duration>:<max duration>:<chance>/...",
                       entry, msg.what() ) );
    }
  }

  return true;
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
  add_option( opt_float( "druid.adaptive_swarm_jump_distance_melee", options.adaptive_swarm_jump_distance_melee ) );
  add_option( opt_float( "druid.adaptive_swarm_jump_distance_ranged", options.adaptive_swarm_jump_distance_ranged ) );
  add_option( opt_float( "druid.adaptive_swarm_jump_distance_stddev", options.adaptive_swarm_jump_distance_stddev ) );
  add_option( opt_uint( "druid.adaptive_swarm_melee_targets", options.adaptive_swarm_melee_targets, 1U, 29U ) );
  add_option( opt_uint( "druid.adaptive_swarm_ranged_targets", options.adaptive_swarm_ranged_targets, 1U, 29U ) );
  add_option( opt_func( "druid.adaptive_swarm_prepull_setup", parse_swarm_setup ) );

  // Guardian

  // Restoration
  add_option( opt_float( "druid.time_spend_healing", options.time_spend_healing ) );
}

std::string druid_t::create_profile( save_e type )
{
  std::string profile = player_t::create_profile( type );

  if ( type & SAVE_PLAYER )
  {
    if ( !options.adaptive_swarm_prepull_setup.empty() )
      profile += fmt::format( "druid.adaptive_swarm_prepull_setup={}\n", options.adaptive_swarm_prepull_setup );
  }

  return profile;
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
  add_absorb( buff.earthwarden );   // unknown if EW or RotS comes first
  add_absorb( buff.rage_of_the_sleeper );
}

void druid_t::target_mitigation( school_e school, result_amount_type type, action_state_t* s )
{
  s->result_amount *= 1.0 + buff.barkskin->value();

  s->result_amount *= 1.0 + buff.survival_instincts->value();

  s->result_amount *= 1.0 + talent.thick_hide->effectN( 1 ).percent();

  s->result_amount *= 1.0 + buff.protective_growth->check_value();

  s->result_amount *= 1.0 + buff.guardians_tenacity->check_stack_value();

  if ( spec.ursine_adept->ok() && buff.bear_form->check() )
    s->result_amount *= 1.0 + spec.ursine_adept->effectN( 2 ).percent();

  // as this is run-time, we can't use find_effect. TODO: possibly cache these values somewhere
  if ( talent.glistening_fur.ok() )
  {
    if ( buff.bear_form->check() )
    {
      if ( dbc::is_school( school, SCHOOL_ARCANE ) )
        s->result_amount *= 1.0 + buff.bear_form->data().effectN( 14 ).percent();
      else
        s->result_amount *= 1.0 + buff.bear_form->data().effectN( 13 ).percent();
    }
    else if ( buff.moonkin_form->check() )
    {
      if ( dbc::is_school( school, SCHOOL_ARCANE ) )
        s->result_amount *= 1.0 + buff.moonkin_form->data().effectN( 13 ).percent();
      else
        s->result_amount *= 1.0 + buff.moonkin_form->data().effectN( 12 ).percent();
    }
  }

  if ( talent.empowered_shapeshifting.ok() && buff.bear_form->check() &&
       spec.bear_form_passive_2->effectN( 3 ).has_common_school( school ) )
  {
    s->result_amount *= 1.0 + talent.empowered_shapeshifting->effectN( 4 ).percent();
  }

  if ( s->action->player != this )
  {
    auto td = get_target_data( s->action->player );

    if ( talent.pulverize.ok() )
      s->result_amount *= 1.0 + td->debuff.pulverize->check_value();

    if ( talent.rend_and_tear.ok() )
    {
      s->result_amount *= 1.0 + talent.rend_and_tear->effectN( 3 ).percent() *
                                    td->dots.thrash_bear->current_stack();
    }

    if ( talent.scintillating_moonlight.ok() && td->dots.moonfire->is_ticking() )
      s->result_amount *= 1.0 + talent.scintillating_moonlight->effectN( 1 ).percent();

    if ( talent.tooth_and_claw.ok() )
      s->result_amount *= 1.0 + td->debuff.tooth_and_claw->check_value();

    if ( talent.dreadful_wound.ok() && td->dots.dreadful_wound->is_ticking())
      s->result_amount *= 1.0 + spec.dreadful_wound->effectN( 2 ).percent();
  }

  player_t::target_mitigation( school, type, s );
}

void druid_t::assess_damage( school_e school, result_amount_type dtype, action_state_t* s )
{
  if ( dbc::is_school( school, SCHOOL_PHYSICAL ) && dtype == result_amount_type::DMG_DIRECT && s->result == RESULT_HIT )
    buff.ironfur->up();

  player_t::assess_damage( school, dtype, s );
}

// Trigger effects based on being hit or taking damage.
void druid_t::assess_damage_imminent_pre_absorb( school_e school, result_amount_type dmg, action_state_t* s )
{
  player_t::assess_damage_imminent_pre_absorb( school, dmg, s );

  if ( action_t::result_is_hit( s->result ) && s->result_amount > 0 )
    buff.cenarion_ward->expire();
}

// Target Data ==============================================================
template <typename Buff, typename... Args>
inline buff_t* druid_td_t::make_debuff( bool b, Args&&... args )
{
  return buff_t::make_buff_fallback<Buff>( target->is_enemy() && b, std::forward<Args>( args )... );
}

druid_td_t::druid_td_t( player_t& target, druid_t& source )
  : actor_target_data_t( &target, &source ), dots(), hots(), debuff(), buff()
{
  if ( target.is_enemy() )
  {
    dots.adaptive_swarm_damage = target.get_dot( "adaptive_swarm_damage", &source );
    dots.astral_smolder        = target.get_dot( "astral_smolder", &source );
    dots.bloodseeker_vines     = target.get_dot( "bloodseeker_vines", &source );
    dots.dreadful_wound        = target.get_dot( "dreadful_wound", &source );
    dots.feral_frenzy          = target.get_dot( "feral_frenzy_tick", &source );  // damage dot is triggered by feral_frenzy_tick_t
    dots.frenzied_assault      = target.get_dot( "frenzied_assault", &source );
    dots.fungal_growth         = target.get_dot( "fungal_growth", &source );
    dots.lunar_inspiration     = target.get_dot( "lunar_inspiration", &source );
    dots.moonfire              = target.get_dot( "moonfire", &source );
    dots.rake                  = target.get_dot( "rake", &source );
    dots.rip                   = target.get_dot( "rip", &source );
    dots.stellar_flare         = target.get_dot( "stellar_flare", &source );
    dots.sunfire               = target.get_dot( "sunfire", &source );
    dots.tear                  = target.get_dot( "tear", &source );
    dots.thrash_bear           = target.get_dot( "thrash_bear", &source );
    dots.thrash_cat            = target.get_dot( "thrash_cat", &source );
  }
  else
  {
    hots.adaptive_swarm_heal   = target.get_dot( "adaptive_swarm_heal", &source );
    hots.cenarion_ward         = target.get_dot( "cenarion_ward", &source );
    hots.cultivation           = target.get_dot( "cultivation", &source );
    hots.frenzied_regeneration = target.get_dot( "frenzied_regeneration", &source );
    hots.germination           = target.get_dot( "germination", &source );
    hots.lifebloom             = target.get_dot( "lifebloom", &source );
    hots.regrowth              = target.get_dot( "regrowth", &source );
    hots.rejuvenation          = target.get_dot( "rejuvenation", &source );
    hots.spring_blossoms       = target.get_dot( "spring_blossoms", &source );
    hots.wild_growth           = target.get_dot( "wild_growth", &source );
  }

  debuff.atmospheric_exposure = make_debuff( source.talent.atmospheric_exposure.ok(),
    *this, "atmospheric_exposure", source.spec.atmospheric_exposure )
      ->set_trigger_spell( source.talent.atmospheric_exposure );

  debuff.bloodseeker_vines =
    make_debuff( source.talent.thriving_growth.ok(), *this, "bloodseeker_vines", source.spec.bloodseeker_vines )
      ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
      ->apply_affecting_aura( source.talent.resilient_flourishing )
      ->set_quiet( true );
  if ( source.talent.bursting_growth.ok() || source.talent.root_network.ok() )
  {
    debuff.bloodseeker_vines->set_stack_change_callback( [ & ]( buff_t* b, int old_, int new_ ) {
      auto diff = new_ - old_;

      if ( diff > 0 )
      {
        source.buff.root_network->trigger( diff );
      }
      else if ( diff < 0 )
      {
        source.buff.root_network->decrement( -diff );

        if ( source.active.bursting_growth )
        {
          while ( diff < 0 )
          {
            source.active.bursting_growth->execute_on_target( b->player );
            diff++;
          }
        }
      }
    } );
  }

  debuff.pulverize = make_debuff( source.talent.pulverize.ok(), *this, "pulverize", source.talent.pulverize )
    ->set_cooldown( 0_ms )
    ->set_refresh_behavior( buff_refresh_behavior::DURATION )
    ->set_default_value_from_effect_type( A_MOD_DAMAGE_TO_CASTER )
    ->apply_affecting_aura( source.talent.circle_of_life_and_death_bear );

  debuff.sabertooth = make_debuff( source.talent.sabertooth.ok(), *this, "sabertooth", source.spec.sabertooth )
    ->set_trigger_spell( source.talent.sabertooth )
    ->set_max_stack( as<int>( source.resources.base[ RESOURCE_COMBO_POINT ] ) );

  debuff.stellar_amplification = make_debuff( source.talent.stellar_amplification.ok(),
    *this, "stellar_amplification", source.spec.stellar_amplification )
      ->set_trigger_spell( source.talent.stellar_amplification )
      ->set_refresh_duration_callback(
        [ dur = source.talent.stellar_amplification->effectN( 1 ).time_value() ]( const buff_t* b, timespan_t d ) {
          return std::min( dur, b->remains() + d );
        } );

  debuff.tooth_and_claw =
    make_debuff( source.talent.tooth_and_claw.ok(), *this, "tooth_and_claw", source.spec.tooth_and_claw_debuff )
      ->set_default_value_from_effect_type( A_MOD_DAMAGE_TO_CASTER )
      ->set_stack_change_callback( [ & ]( buff_t* b, int, int new_ ) {
        source.uptime.tooth_and_claw_debuff->update( new_, b->sim->current_time() );
      } );

  debuff.waning_twilight =
    make_debuff( source.talent.waning_twilight.ok(), *this, "waning_twilight", source.spec.waning_twilight )
      ->set_chance( 1.0 )
      ->set_duration( 0_ms );

  buff.ironbark =
    make_buff_fallback( source.talent.ironbark.ok() && !target.is_enemy(), *this, "ironbark", source.talent.ironbark )
      ->set_cooldown( 0_ms );
}

int druid_td_t::hots_ticking() const
{
  auto count = hots.adaptive_swarm_heal->is_ticking() + hots.cenarion_ward->is_ticking() +
               hots.cultivation->is_ticking() + hots.germination->is_ticking() + hots.regrowth->is_ticking() +
               hots.rejuvenation->is_ticking() + hots.spring_blossoms->is_ticking() + hots.wild_growth->is_ticking();

  auto lb_mul = 1 + as<int>( static_cast<druid_t*>( source )->talent.harmonious_blooming->effectN( 1 ).base_value() );
  count += lb_mul * hots.lifebloom->is_ticking();

  return count;
}

const druid_td_t* druid_t::find_target_data( const player_t* target ) const
{
  assert( target );
  return target_data[ target ];
}

druid_td_t* druid_t::get_target_data( player_t* target ) const
{
  assert( target );
  druid_td_t*& td = target_data[ target ];
  if ( !td )
    td = new druid_td_t( *target, const_cast<druid_t&>( *this ) );

  return td;
}

// ==========================================================================
// druid_t utility functions
// ==========================================================================

void druid_t::shapeshift( form_e f )
{
  if ( get_form() == f )
    return;

  buff.bear_form->expire();
  buff.cat_form->expire();
  buff.moonkin_form->expire();

  switch ( f )
  {
    case BEAR_FORM:    buff.bear_form->trigger();    break;
    case CAT_FORM:     buff.cat_form->trigger();     break;
    case MOONKIN_FORM: buff.moonkin_form->trigger(); break;
    case NO_FORM:                                    break;
    default: assert( false ); break;
  }

  form = f;
}

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

const spell_data_t* druid_t::apply_override( const spell_data_t* base, const spell_data_t* passive ) const
{
  if ( !passive->ok() )
    return base;

  return find_spell( as<unsigned>( find_effect( passive, base, A_OVERRIDE_ACTION_SPELL ).base_value() ) );
}

bool druid_t::uses_form( specialization_e spec, std::string_view name, action_t* action ) const
{
  if ( specialization() == spec )
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
        return std::invoke( dot, get_target_data( a )->dots )->remains() <
               std::invoke( dot, get_target_data( b )->dots )->remains();
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

// Eclipse Handler ==========================================================
void eclipse_handler_t::init()
{
  if ( !p->talent.eclipse.ok() )
  {
    p = nullptr;
    return;
  }

  wrath_counter_base = wrath_counter = p->find_spell( 326055 )->max_stacks();
  starfire_counter_base = starfire_counter = p->find_spell( 326053 )->max_stacks();

  uptimes[ 0 ] = p->get_uptime( "No Eclipse" )->collect_uptime( *p->sim );
  uptimes[ eclipse_e::LUNAR ] = p->get_uptime( "Eclipse Lunar" )->collect_uptime( *p->sim );
  uptimes[ eclipse_e::SOLAR ] = p->get_uptime( "Eclipse Solar" )->collect_uptime( *p->sim );
  uptimes[ 3 ] = p->get_uptime( "Both Eclipses" )->collect_uptime( *p->sim );

  size_t res = 3;
  bool sf = p->spec.starfall->ok();
  bool foe = p->talent.fury_of_elune.ok();
  bool nm = p->talent.new_moon.ok();
  bool hm = nm;
  bool fm = nm;

  data.arrays.reserve( res + sf + foe + nm + hm + fm );
  data.wrath = &data.arrays.emplace_back();
  data.starfire = &data.arrays.emplace_back();
  data.starsurge = &data.arrays.emplace_back();
  if ( sf )
    data.starfall = &data.arrays.emplace_back();
  if ( foe )
    data.fury_of_elune = &data.arrays.emplace_back();
  if ( nm )
    data.new_moon = &data.arrays.emplace_back();
  if ( hm )
    data.half_moon = &data.arrays.emplace_back();
  if ( fm )
    data.full_moon = &data.arrays.emplace_back();

  iter.arrays.reserve( res + sf + foe + nm + hm + fm );
  iter.wrath = &iter.arrays.emplace_back();
  iter.starfire = &iter.arrays.emplace_back();
  iter.starsurge = &iter.arrays.emplace_back();
  if ( sf )
    iter.starfall = &iter.arrays.emplace_back();
  if ( foe )
    iter.fury_of_elune = &iter.arrays.emplace_back();
  if ( nm )
    iter.new_moon = &iter.arrays.emplace_back();
  if ( hm )
    iter.half_moon = &iter.arrays.emplace_back();
  if ( fm )
    iter.full_moon = &iter.arrays.emplace_back();

  if ( p->talent.astral_communion.ok() && !p->is_ptr() )
  {
    ac_gain = p->get_gain( "Astral Communion" );
    ac_ap = p->find_spell( 450599 )->effectN( 1 ).resource();
  }

  auto m_data = p->get_modified_spell( p->talent.greater_alignment )
    ->parse_effects( p->talent.potent_enchantments );

  ga_mod = m_data->effectN( 2 ).percent();
}

void eclipse_handler_t::cast_wrath()
{
  if ( !enabled() ) return;

  if ( iter.wrath && p->in_combat )
    ( *iter.wrath )[ state ]++;

  if ( in_none( state ) )
  {
    wrath_counter--;
    if ( wrath_counter <= 0 )
      p->buff.eclipse_lunar->trigger();
  }
}

void eclipse_handler_t::cast_starfire()
{
  if ( !enabled() ) return;

  if ( iter.starfire && p->in_combat )
    ( *iter.starfire )[ state ]++;

  if ( in_none( state ) && !p->talent.lunar_calling.ok() )
  {
    starfire_counter--;
    if ( starfire_counter <= 0 )
      p->buff.eclipse_solar->trigger();
  }
}

void eclipse_handler_t::cast_starsurge()
{
  if ( !enabled() ) return;

  if ( iter.starsurge && p->in_combat )
    ( *iter.starsurge )[ state ]++;
}

void eclipse_handler_t::cast_moon( moon_stage_e moon )
{
  if ( !enabled() ) return;

  if ( moon == moon_stage_e::NEW_MOON && iter.new_moon )
    ( *iter.new_moon )[ state ]++;
  else if ( moon == moon_stage_e::HALF_MOON && iter.half_moon )
    ( *iter.half_moon )[ state ]++;
  else if ( moon == moon_stage_e::FULL_MOON && iter.full_moon )
    ( *iter.full_moon )[ state ]++;
}

void eclipse_handler_t::tick_starfall()
{
  if ( !enabled() ) return;

  if ( iter.starfall )
    ( *iter.starfall )[ state ]++;
}

void eclipse_handler_t::tick_fury_of_elune()
{
  if ( !enabled() ) return;

  if ( iter.fury_of_elune )
    ( *iter.fury_of_elune )[ state ]++;
}

template <eclipse_e E>
buff_t* eclipse_handler_t::get_boat() const
{
  if constexpr ( E == eclipse_e::LUNAR )
    return p->buff.balance_of_all_things_arcane;
  else if constexpr ( E == eclipse_e::SOLAR )
    return p->buff.balance_of_all_things_nature;
  else
    return nullptr;
}

template <eclipse_e E>
buff_t* eclipse_handler_t::get_harmony() const
{
  if constexpr ( E == eclipse_e::LUNAR )
    return p->buff.harmony_of_the_heavens_lunar;
  else if ( E == eclipse_e::SOLAR )
    return p->buff.harmony_of_the_heavens_solar;
  else
    return nullptr;
}

template <eclipse_e E>
buff_t* eclipse_handler_t::get_eclipse() const
{
  if constexpr ( E == eclipse_e::LUNAR )
    return p->buff.eclipse_lunar;
  else if ( E == eclipse_e::SOLAR )
    return p->buff.eclipse_solar;
  else
    return nullptr;
}

template <eclipse_e E>
void eclipse_handler_t::advance_eclipse( bool active )
{
  if ( !enabled() ) return;

  auto old_state = state;

  reset_stacks();

  if ( active )
  {
    state |= E;

    get_boat<E>()->trigger();
    p->buff.parting_skies->trigger();
    p->buff.solstice->trigger();
    p->buff.cenarius_might->trigger();

    if ( p->is_ptr() )
      p->buff.astral_communion->trigger();

    // only when entering from non-eclipse
    if ( !in_eclipse( old_state ) && !p->is_ptr() )
      p->resource_gain( RESOURCE_ASTRAL_POWER, ac_ap, ac_gain );
  }
  else
  {
    state &= ~E;

    get_harmony<E>()->expire();

    // only when completely leaving eclipse
    if ( !in_eclipse() )
      p->buff.dreamstate->trigger();
  }

  if ( old_state ^ state )
  {
    uptimes[ old_state ]->update( false, p->sim->current_time() );

    if ( old_state ^ ( eclipse_e::LUNAR | eclipse_e::SOLAR ) )
      uptimes[ state ]->update( true, p->sim->current_time() );
  }
}

template <eclipse_e E>
void eclipse_handler_t::update_eclipse()
{
  if ( !enabled() ) return;

  auto buff = get_eclipse<E>();
  if ( !buff->check() )
    return;

  auto val = buff->default_value;

  val += get_harmony<E>()->check_stack_value();

  if ( p->buff.ca_inc->check() )
    val += ga_mod;

  buff->current_value = val;
}

void eclipse_handler_t::reset_stacks()
{
  wrath_counter = wrath_counter_base;
  starfire_counter = starfire_counter_base;
}

void eclipse_handler_t::reset_state()
{
  state = 0;
}

void eclipse_handler_t::datacollection_begin()
{
  if ( !enabled() ) return;

  iter.wrath->fill( 0 );
  iter.starfire->fill( 0 );
  iter.starsurge->fill( 0 );
  if ( iter.starfall )
    iter.starfall->fill( 0 );
  if ( iter.fury_of_elune )
    iter.fury_of_elune->fill( 0 );
  if ( iter.new_moon )
    iter.new_moon->fill( 0 );
  if ( iter.half_moon )
    iter.half_moon->fill( 0 );
  if ( iter.full_moon )
    iter.full_moon->fill( 0 );
}

void eclipse_handler_t::datacollection_end()
{
  if ( !enabled() ) return;

  auto end = []( auto& from, auto& to ) {
    static_assert( std::tuple_size_v<std::remove_reference_t<decltype( from )>> <
                     std::tuple_size_v<std::remove_reference_t<decltype( to )>>,
                   "array size mismatch" );

    size_t i = 0;

    for ( ; i < from.size(); i++ )
      to[ i ] += from[ i ];

    to[ i ]++;
  };

  end( *iter.wrath, *data.wrath );
  end( *iter.starfire, *data.starfire );
  end( *iter.starsurge, *data.starsurge );
  if ( iter.starfall )
    end( *iter.starfall, *data.starfall );
  if ( iter.fury_of_elune )
    end( *iter.fury_of_elune, *data.fury_of_elune );
  if ( iter.new_moon )
    end( *iter.new_moon, *data.new_moon );
  if ( iter.half_moon )
    end( *iter.half_moon, *data.half_moon );
  if ( iter.full_moon )
    end( *iter.full_moon, *data.full_moon );
}

void eclipse_handler_t::merge( const eclipse_handler_t& other )
{
  if ( !enabled() ) return;

  auto merge = []( auto& from, auto& to ) {
    static_assert( std::tuple_size_v<std::remove_reference_t<decltype( from )>> ==
                     std::tuple_size_v<std::remove_reference_t<decltype( to )>>,
                   "array size mismatch" );

    for ( size_t i = 0; i < from.size(); i++ )
      to[ i ] += from[ i ];
  };

  merge( *other.data.wrath, *data.wrath );
  merge( *other.data.starfire, *data.starfire );
  merge( *other.data.starsurge, *data.starsurge );
  if ( data.starfall )
    merge( *other.data.starfall, *data.starfall );
  if ( data.fury_of_elune )
    merge( *other.data.fury_of_elune, *data.fury_of_elune );
  if ( data.new_moon )
    merge( *other.data.new_moon, *data.new_moon );
  if ( data.half_moon )
    merge( *other.data.half_moon, *data.half_moon );
  if ( data.full_moon )
    merge( *other.data.full_moon, *data.full_moon );
}

void eclipse_handler_t::print_table( report::sc_html_stream& os )
{
  if ( !enabled() ) return;

  os << R"(<h3 class="toggle open">Eclipse Utilization</h3><div class="toggle-content"><table class="sc even">)"
     << R"(<thead><tr><th class="left">Ability</th>)"
     << R"(<th colspan="2">None</th><th colspan="2">Solar</th><th colspan="2">Lunar</th><th colspan="2">Both</th>)"
     << "</tr></thead>";

  print_line( os, p->spec.wrath, *data.wrath );
  print_line( os, p->talent.starfire, *data.starfire );
  print_line( os, p->talent.starsurge, *data.starsurge );

  if ( data.starfall )      print_line( os, p->spec.starfall, *data.starfall );
  if ( data.fury_of_elune ) print_line( os, p->find_spell( 202770 ), *data.fury_of_elune );
  if ( data.new_moon )      print_line( os, p->find_spell( 274281 ), *data.new_moon );
  if ( data.half_moon )     print_line( os, p->find_spell( 274282 ), *data.half_moon );
  if ( data.full_moon )     print_line( os, p->find_spell( 274283 ), *data.full_moon );

  os << "</table></div>\n";
}

void eclipse_handler_t::print_line( report::sc_html_stream& os, const spell_data_t* spell, const data_array& data )
{
  double iter  = data[ 4 ];  // MAX
  double none  = data[ 0 ];  // NONE
  double lunar = data[ 1 ];  // LUNAR
  double solar = data[ 2 ];  // SOLAR
  double both  = data[ 3 ];  // LUNAR & SOLAR
  double total = none + solar + lunar + both;

  if ( !total )
    return;

  os.format( R"(<tr class="right"><td class="left">{}</td>)"
             R"(<td>{:.2f}</td><td>{:.1f}%</td><td>{:.2f}</td><td>{:.1f}%</td>)"
             R"(<td>{:.2f}</td><td>{:.1f}%</td><td>{:.2f}</td><td>{:.1f}%</td></tr>)",
             report_decorators::decorated_spell_data( *p->sim, spell ),
             none / iter, none / total * 100, solar / iter, solar / total * 100,
             lunar / iter, lunar / total * 100, both / iter, both / total * 100 );
}

void druid_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  options = static_cast<druid_t*>( source )->options;
  prepull_swarm = static_cast<druid_t*>( source )->prepull_swarm;
}

void druid_t::moving()
{
  if ( ( executing && !executing->usable_moving() ) || ( channeling && !channeling->usable_moving() ) )
    player_t::interrupt();
}

// ==========================================================================
// DBC/Spell data based auto-parsing
// ==========================================================================

void druid_t::apply_affecting_auras( action_t& action )
{
  player_t::apply_affecting_auras( action );

  // Spec-wide auras
  action.apply_affecting_aura( spec.druid );
  action.apply_affecting_aura( spec_spell );

  // Rank spells
  action.apply_affecting_aura( spec.moonfire_2 );

  // Class
  action.apply_affecting_aura( talent.astral_influence );
  action.apply_affecting_aura( talent.improved_rejuvenation );
  action.apply_affecting_aura( talent.improved_stampeding_roar );
  action.apply_affecting_aura( talent.instincts_of_the_claw );
  action.apply_affecting_aura( talent.killer_instinct );
  action.apply_affecting_aura( talent.lore_of_the_grove );
  action.apply_affecting_aura( talent.nurturing_instinct );
  action.apply_affecting_aura( talent.packs_endurance );
  action.apply_affecting_aura( talent.primal_fury );
  action.apply_affecting_aura( talent.starlight_conduit );

  // Balance
  action.apply_affecting_aura( talent.astronomical_impact );
  action.apply_affecting_aura( talent.circle_of_life_and_death_owl );
  action.apply_affecting_aura( talent.cosmic_rapidity );
  action.apply_affecting_aura( talent.elunes_guidance );
  action.apply_affecting_aura( talent.lunar_shrapnel );
  action.apply_affecting_aura( talent.orbital_strike );
  action.apply_affecting_aura( talent.power_of_goldrinn );
  action.apply_affecting_aura( talent.radiant_moonlight );
  action.apply_affecting_aura( talent.rattle_the_stars );
  action.apply_affecting_aura( talent.twin_moons );
  action.apply_affecting_aura( talent.wild_surges );
  action.apply_affecting_aura( sets->set( DRUID_BALANCE, TWW1, B2 ) );

  // Feral 
  action.apply_affecting_aura( spec.ashamanes_guidance );
  action.apply_affecting_aura( talent.berserk_heart_of_the_lion );
  action.apply_affecting_aura( talent.circle_of_life_and_death_cat );
  action.apply_affecting_aura( talent.dreadful_bleeding );
  action.apply_affecting_aura( talent.infected_wounds_cat );
  action.apply_affecting_aura( talent.lions_strength );
  action.apply_affecting_aura( talent.taste_for_blood );
  action.apply_affecting_aura( talent.veinripper );
  action.apply_affecting_aura( talent.wild_slashes );

  // Guardian
  action.apply_affecting_aura( talent.circle_of_life_and_death_bear );
  action.apply_affecting_aura( talent.flashing_claws );
  action.apply_affecting_aura( talent.improved_survival_instincts );
  action.apply_affecting_aura( talent.innate_resolve );
  action.apply_affecting_aura( talent.reinvigoration );
  action.apply_affecting_aura( talent.soul_of_the_forest_bear );
  action.apply_affecting_aura( talent.survival_of_the_fittest );
  action.apply_affecting_aura( talent.twin_moonfire );
  action.apply_affecting_aura( talent.untamed_savagery );
  action.apply_affecting_aura( talent.ursocs_guidance );
  action.apply_affecting_aura( talent.vulnerable_flesh );

  // Restoration
  action.apply_affecting_aura( spec.cenarius_guidance );
  action.apply_affecting_aura( talent.germination );
  action.apply_affecting_aura( talent.improved_ironbark );
  action.apply_affecting_aura( talent.boundless_moonlight );
  action.apply_affecting_aura( talent.inner_peace );
  action.apply_affecting_aura( talent.liveliness );
  action.apply_affecting_aura( talent.master_shapeshifter );
  action.apply_affecting_aura( talent.passing_seasons );
  action.apply_affecting_aura( talent.rampant_growth );
  action.apply_affecting_aura( talent.sabertooth );
  action.apply_affecting_aura( talent.soul_of_the_forest_cat );

  // Hero talents
  // thrash family flags only apply with lunar calling
  bool apply_arcane_affinity = talent.arcane_affinity.ok();
  if ( action.data().class_flag( 40 ) ||  //  flag 40: bear thrash dot
       action.data().class_flag( 91 ) ||  //  flag 91: bear thrash direct
       action.data().class_flag( 126 ) )  //  flag 126: cat thrash
  {
    apply_arcane_affinity = apply_arcane_affinity && talent.lunar_calling.ok();
  }
  // wrath is handled in wrath_t
  else if ( action.data().class_flag( 0 ) )  // flag 0: wrath
  {
    apply_arcane_affinity = false;
  }

  if ( apply_arcane_affinity )
    action.apply_affecting_aura( talent.arcane_affinity );

  action.apply_affecting_aura( talent.astral_insight );
  action.apply_affecting_aura( talent.bestial_strength );  // TODO: does fb bonus apply to guardian
  action.apply_affecting_aura( talent.early_spring );
  action.apply_affecting_aura( talent.empowered_shapeshifting );
  action.apply_affecting_aura( talent.groves_inspiration );
  action.apply_affecting_aura( talent.hunt_beneath_the_open_skies );
  action.apply_affecting_aura( talent.lunar_calling );  // TODO: confirm arcane thrash applies to balance, and starfire damage to guardian
  action.apply_affecting_aura( talent.potent_enchantments );
  action.apply_affecting_aura( talent.resilient_flourishing );
  action.apply_affecting_aura( talent.stellar_command );
  action.apply_affecting_aura( talent.strike_for_the_heart );
  action.apply_affecting_aura( talent.tear_down_the_mighty );
  action.apply_affecting_aura( talent.the_eternal_moon );
  action.apply_affecting_aura( talent.wildstalkers_power );
}

void druid_t::apply_affecting_auras( buff_t& buff )
{
  // Class
  buff.apply_affecting_aura( spec_spell );
  buff.apply_affecting_aura( talent.forestwalk );
  buff.apply_affecting_aura( talent.improved_barkskin );
  buff.apply_affecting_aura( talent.oakskin );

  // Balance
  buff.apply_affecting_aura( talent.circle_of_life_and_death_owl );
  buff.apply_affecting_aura( talent.cosmic_rapidity );
  buff.apply_affecting_aura( talent.greater_alignment );
  buff.apply_affecting_aura( talent.whirling_stars );

  // Feral
  buff.apply_affecting_aura( talent.moment_of_clarity );
  buff.apply_affecting_aura( talent.predator );
  buff.apply_affecting_aura( talent.raging_fury );

  // Guardian
  buff.apply_affecting_aura( spec.ursine_adept );
  buff.apply_affecting_aura( talent.berserk_unchecked_aggression );
  buff.apply_affecting_aura( talent.circle_of_life_and_death_bear );
  buff.apply_affecting_aura( talent.reinforced_fur );
  buff.apply_affecting_aura( talent.ursocs_endurance );

  // Restoration
  buff.apply_affecting_aura( talent.master_shapeshifter );
  buff.apply_affecting_aura( talent.waking_dream->effectN( 1 ).trigger() );

  // Hero talents
  buff.apply_affecting_aura( talent.boundless_moonlight );
  buff.apply_affecting_aura( talent.potent_enchantments );
  buff.apply_affecting_aura( talent.the_eternal_moon );
}

void druid_t::parse_action_effects( action_t* action )
{
  auto _a = dynamic_cast<parse_action_base_t*>( action );
  assert( _a );

  // Class
  _a->parse_effects( buff.cat_form );
  _a->parse_effects( spec.cat_form_passive_2, talent.hunt_beneath_the_open_skies,
                     [ this ] { return buff.cat_form->check(); } );
  _a->parse_effects( buff.moonkin_form );
  _a->parse_effects( buff.rising_light_falling_night_day );

  auto hotw_mask = effect_mask_t( true );
  switch( specialization() )
  {
    case DRUID_BALANCE:     hotw_mask.disable(  1,  2,  3,  13,  15 ); break;
    case DRUID_FERAL:       hotw_mask.disable(  4,  5,  6,  14 );      break;
    case DRUID_GUARDIAN:    hotw_mask.disable(  7,  8,  9 );           break;
    case DRUID_RESTORATION: hotw_mask.disable( 10, 11, 12 );           break;
    default: break;
  }

  _a->parse_effects( buff.heart_of_the_wild, hotw_mask );

  // Balance
  _a->parse_effects( mastery.astral_invocation,
                     // arcane passive mastery (eff#1) and nature passive mastery (eff#3) apply to orbital strike &
                     // goldrinn's fang (label 2391) via hidden script
                     affect_list_t( 1, 3 ).add_label( 2391 ),
                     // nature passive mastery (eff#3) applies to dream burst (433850) via hidden script
                     affect_list_t( 3 ).add_spell( 433850 ) );

  _a->parse_effects( buff.astral_communion );

  // talent data for balance of all things only modifies effect#1 of the buff, and is missing modification to effect#3
  // which is done via hidden script. hack around this by overriding the value instead of normally parsing the talent.
  _a->parse_effects( buff.balance_of_all_things_arcane, talent.balance_of_all_things );
  _a->parse_effects( buff.balance_of_all_things_arcane, effect_mask_t( false ).enable( 3 ),
                     talent.balance_of_all_things->effectN( 1 ).percent() );
  _a->parse_effects( buff.balance_of_all_things_nature, talent.balance_of_all_things );
  _a->parse_effects( buff.balance_of_all_things_nature, effect_mask_t( false ).enable( 3 ),
                     talent.balance_of_all_things->effectN( 1 ).percent() );

  // due to harmony of the heavens, we parse the damage effects (#1/#7) separately and use the current buff value
  // instead of data value
  _a->parse_effects( buff.eclipse_lunar, effect_mask_t( true ).disable( 1, 7 ), talent.umbral_intensity );
  _a->parse_effects( buff.eclipse_lunar, effect_mask_t( false ).enable( 1, 7 ), USE_CURRENT,
                     // damage (eff#1) applies to orbital strike and goldrinn's fang (label 2391) via hidden script
                     affect_list_t( 1 ).add_label( 2391 ) );

  // due to harmony of the heavens, we parse the damage effects (#1/#8) separately and use the current buff value
  // instead of data value
  _a->parse_effects( buff.eclipse_solar, effect_mask_t( true ).disable( 1, 8 ), talent.umbral_intensity );
  _a->parse_effects( buff.eclipse_solar, effect_mask_t( false ).enable( 1, 8 ), USE_CURRENT,
                     // damage (eff#1) applies to orbital strike and goldrinn's fang (label 2391) and dream burst(433850)
                     // via hidden script
                     affect_list_t( 1 ).add_label( 2391 ).add_spell( 433850 ) );

  auto owl_mask = effect_mask_t( false ).enable( 1, 2, 3, 4 );
  if ( talent.astral_insight.ok() )
    owl_mask.enable( 6, 7 );
  if ( talent.lunar_calling.ok() )
    owl_mask.enable( 8, 9 );

  _a->parse_effects( buff.incarnation_moonkin, owl_mask, talent.elunes_guidance );
  _a->parse_effects( buff.owlkin_frenzy );
  _a->parse_effects( buff.starweaver_starfall );
  _a->parse_effects( buff.starweaver_starsurge );
  _a->parse_effects( buff.touch_the_cosmos );
  _a->parse_effects( buff.touch_the_cosmos_starfall );
  _a->parse_effects( buff.touch_the_cosmos_starsurge );
  _a->parse_effects( buff.umbral_inspiration );
  _a->parse_effects( buff.warrior_of_elune );

  // Feral
  _a->parse_effects( mastery.razor_claws );
  _a->parse_effects( buff.apex_predators_craving );
  _a->parse_effects( buff.berserk_cat );
  _a->parse_effects( buff.coiled_to_spring, CONSUME_BUFF );
  _a->parse_effects( buff.incarnation_cat );
  _a->parse_effects( buff.predator, USE_CURRENT );
  _a->parse_effects( buff.predatory_swiftness, CONSUME_BUFF );
  _a->parse_effects( talent.taste_for_blood, [ this ] { return buff.tigers_fury->check();},
                     talent.taste_for_blood->effectN( 2 ).percent() );
  _a->parse_effects( spec.feral_overrides, [ this ] { return !buff.moonkin_form->check(); } );
  _a->parse_effects( buff.fell_prey, CONSUME_BUFF );  // TODO: determine if this actually buffs rampant ferority

  // Guardian
  _a->parse_effects( buff.bear_form );

  auto bear_mask = effect_mask_t( false ).enable( 1, 4, 5 );
  if ( talent.berserk_persistence.ok() )
    bear_mask.enable( 2, 3 );
  if ( talent.incarnation_bear.ok() && talent.astral_insight.ok() )
  {
    bear_mask.enable( 14, 15 );
    if ( talent.lunar_calling.ok() )
      bear_mask.enable( 16, 17 );
  }

  _a->parse_effects( buff.berserk_bear, bear_mask, talent.berserk_ravage,
                     talent.berserk_unchecked_aggression );
  _a->parse_effects( buff.incarnation_bear, bear_mask, talent.berserk_ravage,
                     talent.berserk_unchecked_aggression );
  _a->parse_effects( buff.dream_of_cenarius, effect_mask_t( true ).disable( 5 ), CONSUME_BUFF );

  // dot damage is buffed via script so copy da_mult entries to ta_mult
  // thrash damage buff always applies
  _a->parse_effects( spec.elunes_favored, &_a->ta_multiplier_effects, effect_mask_t( false ).enable( 1 ) );
  _a->parse_effects( spec.elunes_favored, effect_mask_t( false ).enable( 3, 4 ) );

  // dot damage is buffed via script so copy da_mult entries to ta_mult
  // thrash damage buff always applies
  // value is set on talent via script
  _a->parse_effects( spec.fury_of_nature, &_a->ta_multiplier_effects, effect_mask_t( false ).enable( 1 ),
                     talent.fury_of_nature->effectN( 1 ).percent() );
  _a->parse_effects( spec.fury_of_nature, effect_mask_t( false ).enable( 2, 3 ),
                     talent.fury_of_nature->effectN( 1 ).percent() );

  _a->parse_effects( buff.gory_fur, CONSUME_BUFF );
  _a->parse_effects( buff.rage_of_the_sleeper );
  _a->parse_effects( talent.reinvigoration, effect_mask_t( true ).disable( talent.innate_resolve.ok() ? 1 : 2 ) );
  _a->parse_effects( buff.tooth_and_claw );
  _a->parse_effects( buff.vicious_cycle_mangle, USE_DEFAULT, CONSUME_BUFF );
  _a->parse_effects( buff.vicious_cycle_maul, USE_DEFAULT, CONSUME_BUFF );

  _a->parse_effects( buff.guardians_tenacity );
  // effects#5 and #6 are ignored regardless of lunar calling
  _a->parse_effects( sets->set( DRUID_GUARDIAN, TWW1, B4 ), effect_mask_t( true ).disable( 5, 6 ) );

  // Restoration
  _a->parse_effects( buff.abundance );
  _a->parse_effects( buff.clearcasting_tree, talent.flash_of_clarity );
  _a->parse_effects( buff.incarnation_tree );
  _a->parse_effects( buff.natures_swiftness, talent.natures_splendor, CONSUME_BUFF );

  // Hero talents
  _a->parse_effects( buff.blooming_infusion_damage, CONSUME_BUFF );
  _a->parse_effects( buff.blooming_infusion_heal, CONSUME_BUFF );
  _a->parse_effects( buff.feline_potential, CONSUME_BUFF );
  _a->parse_effects( buff.harmony_of_the_grove );
  _a->parse_effects( talent.lunar_insight, spec_spell );  // bear aura affects lunar insight
  _a->parse_effects( buff.root_network );
  _a->parse_effects( buff.strategic_infusion );
  _a->parse_effects( buff.ursine_potential, CONSUME_BUFF );
}

void druid_t::parse_action_target_effects( action_t* action )
{
  auto _a = dynamic_cast<parse_action_base_t*>( action );
  assert( _a );

  // Balance
  _a->parse_target_effects( d_fn( &druid_td_t::dots_t::moonfire ),
                            spec.moonfire_dmg, mastery.astral_invocation );

  _a->parse_target_effects( d_fn( &druid_td_t::dots_t::sunfire ),
                            spec.sunfire_dmg, mastery.astral_invocation,
                            // nature dot mastery (eff#4) applies to dream burst (433850) via hidden script
                            affect_list_t( 4 ).add_spell( 433850 ) );

  _a->parse_target_effects( d_fn( &druid_td_t::debuffs_t::stellar_amplification ),
                            spec.stellar_amplification );

  _a->parse_target_effects( d_fn( &druid_td_t::debuffs_t::waning_twilight ),
                            spec.waning_twilight, talent.waning_twilight );

  // Feral
  if ( talent.incarnation_cat.ok() && talent.ashamanes_guidance.ok() )
  {
    _a->parse_target_effects( [ this ]( actor_target_data_t* td )
                              { return buff.ashamanes_guidance->check() &&
                                static_cast<druid_td_t*>( td )->dots.rip->is_ticking(); },
                              talent.rip, spec.ashamanes_guidance_buff );

    _a->parse_target_effects( [ this ]( actor_target_data_t* td )
                              { return buff.ashamanes_guidance->check() &&
                                static_cast<druid_td_t*>( td )->dots.rake->is_ticking(); },
                              find_trigger( talent.rake ).trigger(), spec.ashamanes_guidance_buff );
  }

  _a->parse_target_effects( d_fn( &druid_td_t::dots_t::adaptive_swarm_damage, false ),
                            spec.adaptive_swarm_damage, spec_spell );

  _a->parse_target_effects( d_fn( &druid_td_t::debuffs_t::sabertooth ),
                            spec.sabertooth, talent.sabertooth->effectN( 2 ).percent() );

  // Guardian
  _a->parse_target_effects( d_fn( &druid_td_t::dots_t::thrash_bear ),
                            spec.thrash_bear_bleed, talent.rend_and_tear );

  // Hero talent
  _a->parse_target_effects( d_fn( &druid_td_t::debuffs_t::atmospheric_exposure ),
                            spec.atmospheric_exposure );
}

void druid_t::parse_player_effects()
{
  parse_effects( find_specialization_spell( "Leather Specialization" ) );

  parse_effects( mastery.natures_guardian_AP );

  auto bear_stam = spec.bear_form_passive->effectN( 2 ).percent() +
                   spec.bear_form_2->effectN( 1 ).percent() +
                   talent.ursocs_spirit->effectN( 1 ).percent();

  add_parse_entry( attribute_multiplier_effects )
    .set_buff( buff.bear_form )
    .set_value( bear_stam )
    .set_opt_enum( 1 << ( STAT_STAMINA - 1 ) )
    .set_eff( &find_effect( spec.bear_form_passive, A_MOD_TOTAL_STAT_PERCENTAGE ) );

  parse_effects( buff.bear_form );
  parse_effects( buff.rage_of_the_sleeper );
  parse_effects( buff.ruthless_aggression );
  parse_effects( buff.ursine_vigor, talent.ursine_vigor );
  parse_effects( buff.wildshape_mastery, effect_mask_t( false ).enable( 2 ),       // armor
                 find_effect( buff.bear_form, A_MOD_BASE_RESISTANCE_PCT ).percent() *
                 buff.wildshape_mastery->data().effectN( 1 ).percent() );
  parse_effects( buff.wildshape_mastery, effect_mask_t( false ).enable( 3 ),       // stamina
                 bear_stam * buff.wildshape_mastery->data().effectN( 1 ).percent() );
  parse_effects( buff.wildshape_mastery, effect_mask_t( false ).enable( 4, 5 ) );  // crit avoidance & expertise

  parse_target_effects( d_fn( &druid_td_t::debuffs_t::bloodseeker_vines ), spec.bloodseeker_vines,
                        talent.vigorous_creepers );
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class druid_report_t : public player_report_extension_t
{
private:
  struct feral_counter_data_t
  {
    action_t* action = nullptr;
    double tf_exe = 0.0;
    double tf_tick = 0.0;
    double bt_exe = 0.0;
    double bt_tick = 0.0;
  };

public:
  druid_report_t( druid_t& player ) : p( player ) {}

  void html_customsection( report::sc_html_stream& os ) override
  {
    os << R"(<div class="player-section custom_section">)";

    if ( p.specialization() == DRUID_FERAL )
      feral_snapshot_table( os );

    if ( p.specialization() == DRUID_BALANCE )
      p.eclipse_handler.print_table( os );

    if ( p.convoke_counter )
      p.convoke_counter->print_table( os );

    p.parsed_effects_html( os );
    modified_spell_data_t::parsed_effects_html( os, *p.sim, p.modified_spells );

    os << "</div>\n";
  }

  void feral_parse_counter( snapshot_counter_t* counter, feral_counter_data_t& data )
  {
    if ( range::contains( counter->buffs, p.buff.tigers_fury ) )
    {
      data.tf_exe += counter->execute.mean();

      if ( counter->tick.count() )
        data.tf_tick += counter->tick.mean();
      else
        data.tf_tick += counter->execute.mean();
    }
    else if ( range::contains( counter->buffs, p.buff.bloodtalons ) )
    {
      data.bt_exe += counter->execute.mean();

      if ( counter->tick.count() )
        data.bt_tick += counter->tick.mean();
      else
        data.bt_tick += counter->execute.mean();
    }
  }

  void feral_snapshot_table( report::sc_html_stream& os )
  {
    // Write header
    os << R"(<h3 class="toggle open">Snapshot Table</h3><div class="toggle-content"><table class="sc sort even">)"
       << R"(<thead><tr><th></th><th colspan="2">Tiger's Fury</th>)";

    if ( p.talent.bloodtalons.ok() )
      os << R"(<th colspan="2">Bloodtalons</th>)";

    os << "</tr>\n";

    os << "<tr>\n"
       << R"(<th class="toggle-sort left" data-sortdir="asc" data-sorttype="alpha">Ability</th>)"
       << R"(<th class="toggle-sort">Execute %</th><th class="toggle-sort">Benefit %</th>)";

    if ( p.talent.bloodtalons.ok() )
      os << R"(<th class="toggle-sort">Execute %</th><th class="toggle-sort">Benefit %</th>)";

    os << "</tr></thead>\n";

    std::vector<feral_counter_data_t> data_list;

    // Compile and Write Contents
    for ( size_t i = 0; i < p.counters.size(); i++ )
    {
      auto counter = p.counters[ i ].get();
      feral_counter_data_t data;

      for ( const auto& a : counter->stats->action_list )
      {
        if ( a->s_data->ok() )
        {
          data.action = a;
          break;
        }
      }

      if ( !data.action )
        data.action = counter->stats->action_list.front();

      // We can change the action's reporting name here since we shouldn't need to access it again later in the report
      std::string suf = "_convoke";
      if ( suf.size() <= data.action->name_str.size() &&
           std::equal( suf.rbegin(), suf.rend(), data.action->name_str.rbegin() ) )
      {
        data.action->name_str_reporting += "Convoke";
      }

      feral_parse_counter( counter, data );

      // since the BT counter is created immediately following the TF counter for the same stat in
      // cat_attacks_t::init(), check the next counter and add in the data if it's for the same stat
      if ( i + 1 < p.counters.size() )
      {
        auto next_counter = p.counters[ i + 1 ].get();

        if ( counter->stats == next_counter->stats )
        {
          feral_parse_counter( next_counter, data );
          i++;
        }
      }

      if ( data.tf_exe + data.tf_tick + data.bt_exe + data.bt_tick == 0.0 )
        continue;

      data_list.push_back( std::move( data ) );
    }

    range::sort( data_list, []( const feral_counter_data_t& l, const feral_counter_data_t& r ) {
      return l.action->name_str < r.action->name_str;
    } );

    for ( const auto& data : data_list )
    {
      os.format( R"(<tr class="right"><td class="left">{}</td><td>{:.2f}%</td><td>{:.2f}%</td>)",
                 report_decorators::decorated_action( *data.action ), data.tf_exe * 100, data.tf_tick * 100 );

      if ( p.talent.bloodtalons.ok() )
        os.format( "<td>{:.2f}%</td><td>{:.2f}</td>", data.bt_exe * 100, data.bt_tick * 100 );

      os << "</tr>\n";
    }

    // Write footer
    os << "</table>\n"
       << "</div>\n";
  }

private:
  druid_t& p;
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

  void register_hotfixes() const override
  {
  }

  void combat_begin( sim_t* ) const override {}
  void combat_end( sim_t* ) const override {}
};
}  // UNNAMED NAMESPACE

const module_t* module_t::druid()
{
  static druid_module_t m;
  return &m;
}
