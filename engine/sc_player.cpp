// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

namespace { // ANONYMOUS NAMESPACE ==========================================

// dark_intent_callback ==================================================

struct dark_intent_callback_t : public action_callback_t
{
  dark_intent_callback_t( player_t* p ) : action_callback_t( p -> sim, p ) {}

  virtual void trigger( action_t* a )
  {
    listener -> buffs.dark_intent_feedback -> trigger();
  }
};

// has_foreground_actions ================================================

static bool has_foreground_actions( player_t* p )
{
  for ( action_t* a = p -> action_list; a; a = a -> next )
  {
    if ( ! a -> background ) return true;
  }
  return false;
}

// init_replenish_targets ================================================

static void init_replenish_targets( sim_t* sim )
{
  if ( sim -> replenishment_candidates.empty() )
  {
    for ( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if ( p -> type != PLAYER_GUARDIAN &&
           p -> primary_resource() == RESOURCE_MANA )
      {
        sim -> replenishment_candidates.push_back( p );
      }
    }
  }
}

// choose_replenish_targets ==============================================

static void choose_replenish_targets( player_t* provider )
{
  sim_t* sim = provider -> sim;

  init_replenish_targets( sim );

  std::vector<player_t*>& candidates = sim      -> replenishment_candidates;
  std::vector<player_t*>& targets    = provider -> replenishment_targets;

  bool replenishment_missing = false;

  for ( int i = ( int ) candidates.size() - 1; i >= 0; i-- )
  {
    player_t* p = candidates[ i ];

    if ( ! p -> sleeping && ! p -> buffs.replenishment -> check() )
    {
      replenishment_missing = true;
      break;
    }
  }

  // If replenishment is not missing from any of the candidates, then the target list will not change

  if ( ! replenishment_missing ) return;

  for ( int i = ( int ) targets.size() - 1; i >= 0; i-- )
  {
    targets[ i ] -> buffs.replenishment -> current_value = -1.0;
  }
  targets.clear();

  for ( int i=0; i < sim -> replenishment_targets; i++ )
  {
    player_t* min_player=0;
    double    min_mana=0;

    for ( int j = ( int ) candidates.size() - 1; j >= 0; j-- )
    {
      player_t* p = candidates[ j ];

      if ( p -> sleeping || ( p -> buffs.replenishment -> current_value == 1.0 ) ) continue;

      if ( ! min_player || min_mana > p -> resource_current[ RESOURCE_MANA ] )
      {
        min_player = p;
        min_mana   = p -> resource_current[ RESOURCE_MANA ];
      }
    }
    if ( min_player )
    {
      min_player -> buffs.replenishment -> trigger( 1, 1.0 );
      targets.push_back( min_player );
    }
    else break;
  }

  for ( int i = ( int ) candidates.size() - 1; i >= 0; i-- )
  {
    player_t* p = candidates[ i ];

    if ( p -> buffs.replenishment -> current_value < 0 )
    {
      p -> buffs.replenishment -> expire();
    }
  }
}

// replenish_raid ========================================================

static void replenish_raid( player_t* provider )
{
  for ( player_t* p = provider -> sim -> player_list; p; p = p -> next )
  {
    p -> buffs.replenishment -> trigger();
  }
}

// trigger_target_swings ====================================================

static void trigger_target_swings( player_t* p )
{
  assert( p -> sim -> target -> attack_speed > 0 );

  struct target_swing_event_t : public event_t
  {
    target_swing_event_t( sim_t* sim, player_t* player, double interval ) : event_t( sim, player )
    {
      name = "Target Swing";
      sim -> add_event( this, interval );
    }
    virtual void execute()
    {
      if ( sim -> log )
        log_t::output( sim, "%s swings at %s", sim -> target -> name(), player -> name() );

      player -> target_swing();

      player -> target_auto_attack = new ( sim ) target_swing_event_t( sim, player, sim -> target -> attack_speed );
    }
  };

  p -> target_auto_attack = new ( p -> sim ) target_swing_event_t( p -> sim, p, p -> sim -> target -> attack_speed );
}

// parse_talent_url =========================================================

static bool parse_talent_url( sim_t* sim,
                              const std::string& name,
                              const std::string& url )
{
  assert( name == "talents" );

  player_t* p = sim -> active_player;

  p -> talents_str = url;

  std::string::size_type cut_pt;

  if ( url.find( "worldofwarcraft" ) != url.npos )
  {
    if ( ( cut_pt = url.find_first_of( "=" ) ) != url.npos )
    {
      return p -> parse_talents_armory( url.substr( cut_pt + 1 ) );
    }
  }
  else if ( url.find( "wowarmory" ) != url.npos )
  {
    if ( ( cut_pt = url.find_last_of( "=" ) ) != url.npos )
    {
      return p -> parse_talents_armory( url.substr( cut_pt + 1 ) );
    }
  }
  else if ( url.find( "mmo-champion" ) != url.npos )
  {
    std::vector<std::string> parts;
    int part_count = util_t::string_split( parts, url, "&" );
    if( part_count <= 0 )
    {
      sim -> errorf( "Player %s has malformed talent string: %s\n", p -> name(), url.c_str() );
      return false;
    }

    if ( ( cut_pt = parts[ 0 ].find_first_of( "?" ) ) != parts[ 0 ].npos )
    {
      return p -> parse_talents_mmo( parts[ 0 ].substr( cut_pt + 1 ) );
    }
  }
  else if ( url.find( "wowhead" ) != url.npos )
  {
    if ( ( cut_pt = url.find_first_of( "#=" ) ) != url.npos )
    {
      return p -> parse_talents_wowhead( url.substr( cut_pt + 1 ) );
    }
  }

  sim -> errorf( "Unable to decode talent string %s for %s\n", url.c_str(), p -> name() );

  return false;
}

} // ANONYMOUS NAMESPACE ===================================================


// ==========================================================================
// Player
// ==========================================================================

// player_t::player_t =======================================================

player_t::player_t( sim_t*             s,
                    player_type        t,
                    const std::string& n,
                    race_type          r ) :
    sim( s ), name_str( n ),
    region_str( s->default_region_str ), server_str( s->default_server_str ), origin_str( "unknown" ),
    next( 0 ), index( -1 ), type( t ), level( 85 ), use_pre_potion( -1 ), tank( -1 ),
    party( 0 ), member( 0 ),
    skill( 0 ), initial_skill( s->default_skill ), distance( 0 ), gcd_ready( 0 ), base_gcd( 1.5 ),
    potion_used( 0 ), sleeping( 0 ), initialized( 0 ),
    pet_list( 0 ), last_modified( 0 ), bugs( true ), pri_tree( TALENT_TAB_NONE ), invert_spirit_scaling( 0 ),
    player_data( &( s->sim_data ) ),
    race_str( "" ), race( r ),
    // Haste
    base_haste_rating( 0 ), initial_haste_rating( 0 ), haste_rating( 0 ),
    spell_haste( 1.0 ),  buffed_spell_haste( 0 ),
    attack_haste( 1.0 ), buffed_attack_haste( 0 ),
    // Mastery
    mastery( 0 ), buffed_mastery ( 0 ), mastery_rating( 0 ), initial_mastery_rating ( 0 ), base_mastery ( 8.0 ),
    // Spell Mechanics
    base_spell_power( 0 ), buffed_spell_power( 0 ),
    base_spell_hit( 0 ),         initial_spell_hit( 0 ),         spell_hit( 0 ),         buffed_spell_hit( 0 ),
    base_spell_crit( 0 ),        initial_spell_crit( 0 ),        spell_crit( 0 ),        buffed_spell_crit( 0 ),
    base_spell_penetration( 0 ), initial_spell_penetration( 0 ), spell_penetration( 0 ), buffed_spell_penetration( 0 ),
    base_mp5( 0 ),               initial_mp5( 0 ),               mp5( 0 ),               buffed_mp5( 0 ),
    spell_power_multiplier( 1.0 ),  initial_spell_power_multiplier( 1.0 ),
    spell_power_per_intellect( 0 ), initial_spell_power_per_intellect( 0 ),
    spell_power_per_spirit( 0 ),    initial_spell_power_per_spirit( 0 ),
    spell_crit_per_intellect( 0 ),  initial_spell_crit_per_intellect( 0 ),
    mp5_per_intellect( 0 ),
    mana_regen_base( 0 ), mana_regen_while_casting( 0 ),
    energy_regen_per_second( 0 ), focus_regen_per_second( 0 ),
    last_cast( 0 ),
    // Attack Mechanics
    base_attack_power( 0 ),       initial_attack_power( 0 ),        attack_power( 0 ),       buffed_attack_power( 0 ),
    base_attack_hit( 0 ),         initial_attack_hit( 0 ),          attack_hit( 0 ),         buffed_attack_hit( 0 ),
    base_attack_expertise( 0 ),   initial_attack_expertise( 0 ),    attack_expertise( 0 ),   buffed_attack_expertise( 0 ),
    base_attack_crit( 0 ),        initial_attack_crit( 0 ),         attack_crit( 0 ),        buffed_attack_crit( 0 ),
    attack_power_multiplier( 1.0 ), initial_attack_power_multiplier( 1.0 ),
    attack_power_per_strength( 0 ), initial_attack_power_per_strength( 0 ),
    attack_power_per_agility( 0 ),  initial_attack_power_per_agility( 0 ),
    attack_crit_per_agility( 0 ),   initial_attack_crit_per_agility( 0 ),
    position( POSITION_BACK ),
    // Defense Mechanics
    target_auto_attack( 0 ),
    base_armor( 0 ),       initial_armor( 0 ),       armor( 0 ),       buffed_armor( 0 ),
    base_bonus_armor( 0 ), initial_bonus_armor( 0 ), bonus_armor( 0 ),
    base_block_value( 0 ), initial_block_value( 0 ), block_value( 0 ), buffed_block_value( 0 ),
    base_miss( 0 ),        initial_miss( 0 ),        miss( 0 ),        buffed_miss( 0 ), buffed_crit( 0 ),
    base_dodge( 0 ),       initial_dodge( 0 ),       dodge( 0 ),       buffed_dodge( 0 ),
    base_parry( 0 ),       initial_parry( 0 ),       parry( 0 ),       buffed_parry( 0 ),
    base_block( 0 ),       initial_block( 0 ),       block( 0 ),       buffed_block( 0 ),
    armor_multiplier( 1.0 ), initial_armor_multiplier( 1.0 ),
    armor_per_agility( 0 ), initial_armor_per_agility( 0 ),
    dodge_per_agility( 0 ), initial_dodge_per_agility( 0 ),
    diminished_dodge_capi( 0 ), diminished_parry_capi( 0 ), diminished_kfactor( 0 ),
    armor_coeff( 0 ),
    half_resistance_rating( 0 ),
    // Attacks
    main_hand_attack( 0 ), off_hand_attack( 0 ), ranged_attack( 0 ),
    // Resources
    mana_per_intellect( 0 ), health_per_stamina( 0 ),
    // Consumables
    elixir_guardian( ELIXIR_NONE ),
    elixir_battle( ELIXIR_NONE ),
    flask( FLASK_NONE ),
    food( FOOD_NONE ),
    // Events
    executing( 0 ), channeling( 0 ), readying( 0 ), in_combat( false ), action_queued( false ),
    // Actions
    action_list( 0 ), action_list_default( 0 ), cooldown_list( 0 ), dot_list( 0 ),
    // Reporting
    quiet( 0 ), last_foreground_action( 0 ),
    current_time( 0 ), total_seconds( 0 ),
    total_waiting( 0 ), total_foreground_actions( 0 ),
    iteration_dmg( 0 ), total_dmg( 0 ),
    dps( 0 ), dps_min( 0 ), dps_max( 0 ), dps_std_dev( 0 ), dps_error( 0 ), dpr( 0 ), rps_gain( 0 ), rps_loss( 0 ),
    buff_list( 0 ), proc_list( 0 ), gain_list( 0 ), stats_list( 0 ), uptime_list( 0 ),
    save_str( "" ), save_gear_str( "" ), save_talents_str( "" ), save_actions_str( "" ),
    comment_str( "" ),
    sets( 0 ),
    meta_gem( META_GEM_NONE ), matching_gear( false ), scaling_lag( 0 ), rng_list( 0 )
{
  if ( sim -> debug ) log_t::output( sim, "Creating Player %s", name() );
  player_t** last = &( sim -> player_list );
  while ( *last ) last = &( ( *last ) -> next );
  *last = this;
  next = 0;
  index = ++( sim -> num_players );

  race_str = util_t::race_type_string( race );

  if ( is_pet() ) skill = 1.0;

  for ( int i=0; i < ATTRIBUTE_MAX; i++ )
  {
    attribute[ i ] = attribute_base[ i ] = attribute_initial[ i ] = 0;
    attribute_multiplier[ i ] = attribute_multiplier_initial[ i ] = 1.0;
    attribute_buffed[ i ] = 0;
  }

  for ( int i=0; i <= SCHOOL_MAX; i++ )
  {
    initial_spell_power[ i ] = spell_power[ i ] = 0;
  }

  for ( int i=0; i < RESOURCE_MAX; i++ )
  {
    resource_base[ i ] = resource_initial[ i ] = resource_max[ i ] = resource_current[ i ] = 0;
    resource_lost[ i ] = resource_gained[ i ] = 0;
    resource_buffed[ i ] = 0;
  }

  for ( int i=0; i < PROFESSION_MAX; i++ ) profession[ i ] = 0;
  for ( int i=0; i < STAT_MAX; i++ )
  {
    scales_with[ i ] = 0;
    over_cap[ i ] = 0;
  }

  items.resize( SLOT_MAX );
  for ( int i=0; i < SLOT_MAX; i++ )
  {
    items[ i ].slot = i;
    items[ i ].sim = sim;
    items[ i ].player = this;
  }

  main_hand_weapon.slot = SLOT_MAIN_HAND;
  off_hand_weapon.slot = SLOT_OFF_HAND;
  ranged_weapon.slot = SLOT_RANGED;

  if ( ! sim -> active_files.empty() ) origin_str = sim -> active_files.back();

  for ( int i=0; i < MAX_TALENT_TREES; i++ )
  {
    talent_tab_points[ i ] = 0;
    tree_type[ i ] = TREE_NONE;
  }
  talent_list2.clear();
  spell_list.clear();
  talent_str.clear();
}

// player_t::~player_t =====================================================

player_t::~player_t()
{
  while ( action_t* a = action_list )
  {
    action_list = a -> next;
    delete a;
  }
  while ( proc_t* p = proc_list )
  {
    proc_list = p -> next;
    delete p;
  }
  while ( gain_t* g = gain_list )
  {
    gain_list = g -> next;
    delete g;
  }
  while ( stats_t* s = stats_list )
  {
    stats_list = s -> next;
    delete s;
  }
  while ( uptime_t* u = uptime_list )
  {
    uptime_list = u -> next;
    delete u;
  }
  while ( rng_t* r = rng_list )
  {
    rng_list = r -> next;
    delete r;
  }

  while ( dot_t* d = dot_list )
  {   
    dot_list = d -> next;
    delete d;
  }

  while ( buff_t* d = buff_list )
  {
    buff_list = d -> next;
    delete d;
  }

  while ( cooldown_t* d = cooldown_list )
  {
    cooldown_list = d -> next;
    delete d;
  }

  for ( int i=0; i < RESOURCE_MAX; i++ )
  {
    resource_gain_callbacks[ i ].clear();
    resource_loss_callbacks[ i ].clear();
  }
  for ( int i=0; i < RESULT_MAX; i++ )
  {
    attack_result_callbacks[ i ].clear();
    spell_result_callbacks[ i ].clear();
    attack_direct_result_callbacks[ i ].clear();
    spell_direct_result_callbacks[ i ].clear();
    spell_cast_result_callbacks[ i ].clear();
    harmful_cast_result_callbacks[ i ].clear();
  }
  for ( int i=0; i < SCHOOL_MAX; i++ )
  {
    tick_damage_callbacks[ i ].clear();
    direct_damage_callbacks[ i ].clear();
  }
  tick_callbacks.clear();

  while ( all_callbacks.size() )
  {
    action_callback_t* a = all_callbacks.back();
    all_callbacks.pop_back();
    delete a;
  }

  while ( spell_list.size() )
  {
    spell_id_t* s = spell_list.back();
    spell_list.pop_back();
    delete s;
  }
  while ( talent_list2.size() )
  {
    talent_t* s = talent_list2.back();
    talent_list2.pop_back();
    delete s;
  }
  while ( unknown_options.size() )
  {
    nvpair_t* t = unknown_options.back();
    unknown_options.pop_back();
    delete t;
  }
  
  if ( sets )
    delete sets;
}

// player_t::id ============================================================

const char* player_t::id()
{
  if ( id_str.empty() )
  {
    // create artifical unit ID, format type+subtype+id= TTTSSSSSSSIIIIII
    char buffer[ 1024 ];
    sprintf( buffer, "0x0100000002%06X,\"%s\",0x511", index, name_str.c_str() );
    id_str = buffer;
  }

  return id_str.c_str();
}

// player_t::init ==========================================================

bool player_t::init( sim_t* sim )
{
  if ( sim -> debug ) log_t::output( sim, "Creating Pets." );

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    p -> create_pets();
  }

  if ( sim -> debug ) log_t::output( sim, "Initializing Auras, Buffs, and De-Buffs." );

  player_t::death_knight_init( sim );
  player_t::druid_init       ( sim );
  player_t::hunter_init      ( sim );
  player_t::mage_init        ( sim );
  player_t::paladin_init     ( sim );
  player_t::priest_init      ( sim );
  player_t::rogue_init       ( sim );
  player_t::shaman_init      ( sim );
  player_t::warlock_init     ( sim );
  player_t::warrior_init     ( sim );

  if ( sim -> debug ) log_t::output( sim, "Initializing Players." );

  bool too_quiet = true;

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    p -> init();
    if ( ! p -> quiet ) too_quiet = false;
  }

  if ( too_quiet && ! sim -> debug )
  {
    sim -> errorf( "No active players in sim!" );
    return false;
  }

  if ( sim -> debug ) log_t::output( sim, "Building Parties." );

  int party_index=0;
  for ( unsigned i=0; i < sim -> party_encoding.size(); i++ )
  {
    std::string& party_str = sim -> party_encoding[ i ];

    if ( party_str == "reset" )
    {
      party_index = 0;
      for ( player_t* p = sim -> player_list; p; p = p -> next ) p -> party = 0;
    }
    else if ( party_str == "all" )
    {
      int member_index = 0;
      for ( player_t* p = sim -> player_list; p; p = p -> next )
      {
        p -> party = 1;
        p -> member = member_index++;
      }
    }
    else
    {
      party_index++;

      std::vector<std::string> player_names;
      int num_players = util_t::string_split( player_names, party_str, ",;/" );
      int member_index=0;

      for ( int j=0; j < num_players; j++ )
      {
        player_t* p = sim -> find_player( player_names[ j ] );
        if ( ! p )
        {
          sim -> errorf( "Unable to find player %s for party creation.\n", player_names[ j ].c_str() );
          return false;
        }
        p -> party = party_index;
        p -> member = member_index++;
        for ( pet_t* pet = p -> pet_list; pet; pet = pet -> next_pet )
        {
          pet -> party = party_index;
          pet -> member = member_index++;
        }
      }
    }
  }

  if ( sim -> debug ) log_t::output( sim, "Registering Callbacks." );

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    p -> register_callbacks();
  }

  return true;
}

// player_t::init ==========================================================

void player_t::init()
{
  if ( sim -> debug ) log_t::output( sim, "Initializing player %s", name() );

  initialized = 1;
  init_data();
  init_talents();
  init_spells();
  init_rating();
  init_glyphs();
  init_race();
  init_base();
  init_items();
  init_core();
  init_spell();
  init_attack();
  init_defense();
  init_weapon( &main_hand_weapon );
  init_weapon( &off_hand_weapon );
  init_weapon( &ranged_weapon );
  init_unique_gear();
  init_enchant();
  init_professions();
  init_consumables();
  init_scaling();
  init_buffs();
  init_actions();
  init_gains();
  init_procs();
  init_uptimes();
  init_rng();
  init_stats();
  init_values();
}

// player_t::init_data =====================================================

void player_t::init_data()
{
  player_data.set_parent( & sim -> sim_data );
}

// player_t::init_base =====================================================

void player_t::init_base()
{
  attribute_base[ ATTR_STRENGTH  ] = rating_t::get_attribute_base( sim, player_data, level, type, race, BASE_STAT_STRENGTH );
  attribute_base[ ATTR_AGILITY   ] = rating_t::get_attribute_base( sim, player_data, level, type, race, BASE_STAT_AGILITY );
  attribute_base[ ATTR_STAMINA   ] = rating_t::get_attribute_base( sim, player_data, level, type, race, BASE_STAT_STAMINA );
  attribute_base[ ATTR_INTELLECT ] = rating_t::get_attribute_base( sim, player_data, level, type, race, BASE_STAT_INTELLECT );
  attribute_base[ ATTR_SPIRIT    ] = rating_t::get_attribute_base( sim, player_data, level, type, race, BASE_STAT_SPIRIT );
  resource_base[ RESOURCE_HEALTH ] = rating_t::get_attribute_base( sim, player_data, level, type, race, BASE_STAT_HEALTH );
  resource_base[ RESOURCE_MANA   ] = rating_t::get_attribute_base( sim, player_data, level, type, race, BASE_STAT_MANA );
  base_spell_crit                  = rating_t::get_attribute_base( sim, player_data, level, type, race, BASE_STAT_SPELL_CRIT );
  base_attack_crit                 = rating_t::get_attribute_base( sim, player_data, level, type, race, BASE_STAT_MELEE_CRIT );
  initial_spell_crit_per_intellect = rating_t::get_attribute_base( sim, player_data, level, type, race, BASE_STAT_SPELL_CRIT_PER_INT );
  initial_attack_crit_per_agility  = rating_t::get_attribute_base( sim, player_data, level, type, race, BASE_STAT_MELEE_CRIT_PER_AGI );
  base_mp5                         = rating_t::get_attribute_base( sim, player_data, level, type, race, BASE_STAT_MP5 );
}

// player_t::init_items =====================================================

void player_t::init_items()
{
  if ( is_pet() ) return;

  std::vector<std::string> splits;
  int num_splits = util_t::string_split( splits, items_str, "/" );
  for ( int i=0; i < num_splits; i++ )
  {
    if ( find_item( splits[ i ] ) )
    {
      sim -> errorf( "Player %s has multiple %s equipped.\n", name(), splits[ i ].c_str() );
    }
    items.push_back( item_t( this, splits[ i ] ) );
  }

  gear_stats_t item_stats;

  matching_gear = true;

  int num_items = ( int ) items.size();
  for ( int i=0; i < num_items; i++ )
  {
    item_t& item = items[ i ];

    if ( ! item.init() )
    {
      sim -> errorf( "Unable to initialize item '%s' on player '%s'\n", item.name(), name() );
      return;
    }

    if ( ! item.matching_type() )
    {
      matching_gear = false;
    }

    for ( int j=0; j < STAT_MAX; j++ )
    {
      item_stats.add_stat( j, item.stats.get_stat( j ) );
    }
  }

  init_meta_gem( item_stats );

  for ( int i=0; i < STAT_MAX; i++ )
  {
    if ( gear.get_stat( i ) == 0 )
    {
      gear.set_stat( i, item_stats.get_stat( i ) );
    }
  }

  if ( sim -> debug )
  {
    log_t::output( sim, "%s gear:", name() );
    gear.print( sim -> output_file );
  }

  set_bonus.init( this );
}

// player_t::init_meta_gem ==================================================

void player_t::init_meta_gem( gear_stats_t& item_stats )
{
  if ( ! meta_gem_str.empty() ) meta_gem = util_t::parse_meta_gem_type( meta_gem_str );

  if      ( meta_gem == META_AUSTERE_EARTHSIEGE        ) item_stats.attribute[ ATTR_STAMINA ] += 32;
  else if ( meta_gem == META_AUSTERE_SHADOWSPIRIT      ) item_stats.attribute[ ATTR_STAMINA ] += 81;
  else if ( meta_gem == META_BEAMING_EARTHSIEGE        ) item_stats.crit_rating += 21;
  else if ( meta_gem == META_BRACING_EARTHSIEGE        ) item_stats.attribute[ ATTR_INTELLECT ] += 21;
  else if ( meta_gem == META_BRACING_EARTHSTORM        ) item_stats.attribute[ ATTR_INTELLECT ] += 12;
  else if ( meta_gem == META_BRACING_SHADOWSPIRIT      ) item_stats.attribute[ ATTR_INTELLECT ] += 54;
  else if ( meta_gem == META_CHAOTIC_SHADOWSPIRIT      ) item_stats.crit_rating += 54;
  else if ( meta_gem == META_CHAOTIC_SKYFIRE           ) item_stats.crit_rating += 12;
  else if ( meta_gem == META_CHAOTIC_SKYFLARE          ) item_stats.crit_rating += 21;
  else if ( meta_gem == META_DESTRUCTIVE_SHADOWSPIRIT  ) item_stats.crit_rating += 54;
  else if ( meta_gem == META_DESTRUCTIVE_SKYFIRE       ) item_stats.crit_rating += 12;
  else if ( meta_gem == META_DESTRUCTIVE_SKYFLARE      ) item_stats.crit_rating += 21;
  else if ( meta_gem == META_EFFULGENT_SHADOWSPIRIT    ) item_stats.attribute[ ATTR_STAMINA ] += 81;
  else if ( meta_gem == META_EMBER_SHADOWSPIRIT        ) item_stats.attribute[ ATTR_INTELLECT ] += 54;
  else if ( meta_gem == META_EMBER_SKYFIRE             ) item_stats.attribute[ ATTR_INTELLECT ] += 12;
  else if ( meta_gem == META_EMBER_SKYFLARE            ) item_stats.attribute[ ATTR_INTELLECT ] += 21;
  else if ( meta_gem == META_ENIGMATIC_SHADOWSPIRIT    ) item_stats.crit_rating += 54;
  else if ( meta_gem == META_ENIGMATIC_SKYFLARE        ) item_stats.crit_rating += 21;
  else if ( meta_gem == META_ENIGMATIC_STARFLARE       ) item_stats.crit_rating += 17;
  else if ( meta_gem == META_ENIGMATIC_SKYFIRE         ) item_stats.crit_rating += 12;
  else if ( meta_gem == META_ETERNAL_EARTHSIEGE        ) item_stats.dodge_rating += 21;
  else if ( meta_gem == META_ETERNAL_SHADOWSPIRIT      ) item_stats.attribute[ ATTR_STAMINA ] += 81;
  else if ( meta_gem == META_FLEET_SHADOWSPIRIT        ) item_stats.mastery_rating += 54;
  else if ( meta_gem == META_FORLORN_SHADOWSPIRIT      ) item_stats.attribute[ ATTR_INTELLECT ] += 54;
  else if ( meta_gem == META_FORLORN_SKYFLARE          ) item_stats.attribute[ ATTR_INTELLECT ] += 21;
  else if ( meta_gem == META_FORLORN_STARFLARE         ) item_stats.attribute[ ATTR_INTELLECT ] += 17;
  else if ( meta_gem == META_IMPASSIVE_SHADOWSPIRIT    ) item_stats.crit_rating += 54;
  else if ( meta_gem == META_IMPASSIVE_SKYFLARE        ) item_stats.crit_rating += 21;
  else if ( meta_gem == META_IMPASSIVE_STARFLARE       ) item_stats.crit_rating += 17;
  else if ( meta_gem == META_INSIGHTFUL_EARTHSIEGE     ) item_stats.attribute[ ATTR_INTELLECT ] += 21;
  else if ( meta_gem == META_INSIGHTFUL_EARTHSTORM     ) item_stats.attribute[ ATTR_INTELLECT ] += 12;
  else if ( meta_gem == META_INVIGORATING_EARTHSIEGE   ) item_stats.haste_rating += 21;
  else if ( meta_gem == META_PERSISTENT_EARTHSHATTER   ) item_stats.crit_rating += 21;
  else if ( meta_gem == META_PERSISTENT_EARTHSIEGE     ) item_stats.crit_rating += 17;
  else if ( meta_gem == META_POWERFUL_EARTHSHATTER     ) item_stats.attribute[ ATTR_STAMINA ] += 26;
  else if ( meta_gem == META_POWERFUL_EARTHSIEGE       ) item_stats.attribute[ ATTR_STAMINA ] += 32;
  else if ( meta_gem == META_POWERFUL_EARTHSTORM       ) item_stats.attribute[ ATTR_STAMINA ] += 18;
  else if ( meta_gem == META_POWERFUL_SHADOWSPIRIT     ) item_stats.attribute[ ATTR_STAMINA ] += 81;
  else if ( meta_gem == META_RELENTLESS_EARTHSIEGE     ) item_stats.attribute[ ATTR_AGILITY ] += 21;
  else if ( meta_gem == META_RELENTLESS_EARTHSTORM     ) item_stats.attribute[ ATTR_AGILITY ] += 12;
  else if ( meta_gem == META_REVITALIZING_SHADOWSPIRIT ) item_stats.attribute[ ATTR_SPIRIT  ] += 54;
  else if ( meta_gem == META_REVITALIZING_SKYFLARE     ) item_stats.attribute[ ATTR_SPIRIT  ] += 22;
  else if ( meta_gem == META_SWIFT_SKYFIRE             ) item_stats.crit_rating += 12;
  else if ( meta_gem == META_SWIFT_SKYFLARE            ) item_stats.crit_rating += 21;
  else if ( meta_gem == META_SWIFT_STARFLARE           ) item_stats.crit_rating += 17;
  else if ( meta_gem == META_TIRELESS_STARFLARE        ) item_stats.attribute[ ATTR_INTELLECT ] += 17;
  else if ( meta_gem == META_TIRELESS_SKYFLARE         ) item_stats.attribute[ ATTR_INTELLECT ] += 21;
  else if ( meta_gem == META_TRENCHANT_EARTHSHATTER    ) item_stats.attribute[ ATTR_INTELLECT ] += 17;
  else if ( meta_gem == META_TRENCHANT_EARTHSIEGE      ) item_stats.attribute[ ATTR_INTELLECT ] += 21;

  if ( ( meta_gem == META_AUSTERE_EARTHSIEGE ) || ( meta_gem == META_AUSTERE_SHADOWSPIRIT ) )
  {
    initial_armor_multiplier *= 1.02;
  }
  else if ( ( meta_gem == META_EMBER_SHADOWSPIRIT ) || ( meta_gem == META_EMBER_SKYFIRE ) || ( meta_gem == META_EMBER_SKYFLARE ) )
  {
    mana_per_intellect *= 1.02;
  }
  else if ( meta_gem == META_BEAMING_EARTHSIEGE )
  {
    mana_per_intellect *= 1.02;
  }
  else if ( meta_gem == META_MYSTICAL_SKYFIRE )
  {
    unique_gear_t::register_stat_proc( PROC_SPELL, RESULT_HIT_MASK, "mystical_skyfire", this, STAT_HASTE_RATING, 1, 320, 0.15, 4.0, 45.0 );
  }
  else if ( meta_gem == META_INSIGHTFUL_EARTHSTORM )
  {
    unique_gear_t::register_stat_proc( PROC_SPELL, RESULT_HIT_MASK, "insightful_earthstorm", this, STAT_MANA, 1, 300, 0.05, 0, 15.0 );
  }
  else if ( meta_gem == META_INSIGHTFUL_EARTHSIEGE )
  {
    unique_gear_t::register_stat_proc( PROC_SPELL, RESULT_HIT_MASK, "insightful_earthsiege", this, STAT_MANA, 1, 600, 0.05, 0, 15.0 );
  }
}

// player_t::init_core ======================================================

void player_t::init_core()
{
  initial_stats.  hit_rating = gear.  hit_rating + enchant.  hit_rating + ( is_pet() ? 0 : sim -> enchant.  hit_rating );
  initial_stats. crit_rating = gear. crit_rating + enchant. crit_rating + ( is_pet() ? 0 : sim -> enchant. crit_rating );
  initial_stats.haste_rating = gear.haste_rating + enchant.haste_rating + ( is_pet() ? 0 : sim -> enchant.haste_rating );
  initial_stats.mastery_rating = gear.mastery_rating + enchant.mastery_rating + ( is_pet() ? 0 : sim -> enchant.mastery_rating );

  if ( initial_stats.  hit_rating < 0 ) initial_stats.  hit_rating = 0;
  if ( initial_stats. crit_rating < 0 ) initial_stats. crit_rating = 0;
  if ( initial_stats.haste_rating < 0 ) initial_stats.haste_rating = 0;
  if ( initial_stats.mastery_rating < 0 ) initial_stats.mastery_rating = 0;

  initial_haste_rating 		= initial_stats.haste_rating;

  initial_mastery_rating 	= initial_stats.mastery_rating;

  for ( int i=0; i < ATTRIBUTE_MAX; i++ )
  {
    initial_stats.attribute[ i ] = gear.attribute[ i ] + enchant.attribute[ i ] + ( is_pet() ? 0 : sim -> enchant.attribute[ i ] );

    attribute[ i ] = attribute_initial[ i ] = attribute_base[ i ] + initial_stats.attribute[ i ];
  }
}

// player_t::init_race ======================================================

void player_t::init_race()
{
  race = util_t::parse_race_type( race_str );
}

// player_t::init_spell =====================================================

void player_t::init_spell()
{
  initial_stats.spell_power       = gear.spell_power       + enchant.spell_power       + ( is_pet() ? 0 : sim -> enchant.spell_power );
  initial_stats.spell_penetration = gear.spell_penetration + enchant.spell_penetration + ( is_pet() ? 0 : sim -> enchant.spell_penetration );
  initial_stats.mp5               = gear.mp5               + enchant.mp5               + ( is_pet() ? 0 : sim -> enchant.mp5 );

  if ( initial_stats.spell_power       < 0 ) initial_stats.spell_power       = 0;
  if ( initial_stats.spell_penetration < 0 ) initial_stats.spell_penetration = 0;
  if ( initial_stats.mp5               < 0 ) initial_stats.mp5               = 0;

  initial_spell_power[ SCHOOL_MAX ] = base_spell_power + initial_stats.spell_power;

  initial_spell_hit = base_spell_hit + initial_stats.hit_rating / rating.spell_hit;

  initial_spell_crit = base_spell_crit + initial_stats.crit_rating / rating.spell_crit;

  initial_spell_penetration = base_spell_penetration + initial_stats.spell_penetration;



  initial_mp5 = base_mp5 + initial_stats.mp5;

  mana_regen_base = player_data.spi_regen( type, level ); 

  if ( level >= 61 )
  {
    half_resistance_rating = 150.0 + ( level - 60 ) * ( level - 67.5 );
  }
  else if ( level >= 21 )
  {
    half_resistance_rating = 50.0 + ( level - 20 ) * 2.5;
  }
  else
  {
    half_resistance_rating = 50.0;
  }
}

// player_t::init_attack ====================================================

void player_t::init_attack()
{
  initial_stats.attack_power             = gear.attack_power             + enchant.attack_power             + ( is_pet() ? 0 : sim -> enchant.attack_power );
  initial_stats.expertise_rating         = gear.expertise_rating         + enchant.expertise_rating         + ( is_pet() ? 0 : sim -> enchant.expertise_rating );

  if ( initial_stats.attack_power             < 0 ) initial_stats.attack_power             = 0;
  if ( initial_stats.expertise_rating         < 0 ) initial_stats.expertise_rating         = 0;

  initial_attack_power = base_attack_power + initial_stats.attack_power;

  initial_attack_hit = base_attack_hit + initial_stats.hit_rating / rating.attack_hit;

  initial_attack_crit = base_attack_crit + initial_stats.crit_rating / rating.attack_crit;

  initial_attack_expertise = base_attack_expertise + initial_stats.expertise_rating / rating.expertise;

  double a,b;
  if ( level > 80 )
  {
    a = 2167.5;
    b = -158167.5;
  }
  else if ( level >= 60 )
  {
    a = 467.5;
    b = -22167.5;
  }
  else
  {
    a = 85.0;
    b = 400.0;
  }
  armor_coeff = a * level + b;
}

// player_t::init_defense ====================================================

void player_t::init_defense()
{
  base_dodge = player_data.dodge_base( type );

  initial_stats.armor          = gear.armor          + enchant.armor          + ( is_pet() ? 0 : sim -> enchant.armor );
  initial_stats.bonus_armor    = gear.bonus_armor    + enchant.bonus_armor    + ( is_pet() ? 0 : sim -> enchant.bonus_armor );
  initial_stats.dodge_rating   = gear.dodge_rating   + enchant.dodge_rating   + ( is_pet() ? 0 : sim -> enchant.dodge_rating );
  initial_stats.parry_rating   = gear.parry_rating   + enchant.parry_rating   + ( is_pet() ? 0 : sim -> enchant.parry_rating );
  initial_stats.block_rating   = gear.block_rating   + enchant.block_rating   + ( is_pet() ? 0 : sim -> enchant.block_rating );
  initial_stats.block_value    = gear.block_value    + enchant.block_value    + ( is_pet() ? 0 : sim -> enchant.block_value );

  if ( initial_stats.armor          < 0 ) initial_stats.armor          = 0;
  if ( initial_stats.bonus_armor    < 0 ) initial_stats.bonus_armor    = 0;
  if ( initial_stats.dodge_rating   < 0 ) initial_stats.dodge_rating   = 0;
  if ( initial_stats.parry_rating   < 0 ) initial_stats.parry_rating   = 0;
  if ( initial_stats.block_rating   < 0 ) initial_stats.block_rating   = 0;
  if ( initial_stats.block_value    < 0 ) initial_stats.block_value    = 0;

  initial_armor             = base_armor       + initial_stats.armor;
  initial_bonus_armor       = base_bonus_armor + initial_stats.bonus_armor;
  initial_miss              = base_miss;
  initial_dodge             = base_dodge       + initial_stats.dodge_rating / rating.dodge;
  initial_parry             = base_parry       + initial_stats.parry_rating / rating.parry;
  initial_block             = base_block       + initial_stats.block_rating / rating.block;
  initial_block_value       = base_block_value + initial_stats.block_value;
  initial_dodge_per_agility = player_data.dodge_scale( type, level );

  if ( tank > 0 ) position = POSITION_FRONT;
}

// player_t::init_weapon ===================================================

void player_t::init_weapon( weapon_t* w )
{
  if ( w -> type == WEAPON_NONE ) return;

  if ( w -> slot == SLOT_MAIN_HAND ) assert( w -> type >= WEAPON_NONE && w -> type < WEAPON_2H );
  if ( w -> slot == SLOT_OFF_HAND  ) assert( w -> type >= WEAPON_NONE && w -> type < WEAPON_2H );
  if ( w -> slot == SLOT_RANGED    ) assert( w -> type == WEAPON_NONE || ( w -> type > WEAPON_2H && w -> type < WEAPON_RANGED ) );
}

// player_t::init_unique_gear ==============================================

void player_t::init_unique_gear()
{
  unique_gear_t::init( this );
}

// player_t::init_enchant ==================================================

void player_t::init_enchant()
{
  enchant_t::init( this );
}

// player_t::init_resources ================================================

void player_t::init_resources( bool force )
{
  // The first 20pts of intellect/stamina only provide 1pt of mana/health.
  // Code simplified on the assumption that the minimum player level is 60.
  double adjust = is_pet() ? 0 : 20;

  for ( int i=0; i < RESOURCE_MAX; i++ )
  {
    if ( force || resource_initial[ i ] == 0 )
    {
      resource_initial[ i ] = resource_base[ i ] + gear.resource[ i ] + enchant.resource[ i ] + ( is_pet() ? 0 : sim -> enchant.resource[ i ] );

      if ( i == RESOURCE_MANA ) 
      {
        if ( ( meta_gem == META_EMBER_SHADOWSPIRIT ) || ( meta_gem == META_EMBER_SKYFIRE ) || ( meta_gem == META_EMBER_SKYFLARE ) )
        {
          resource_initial[ i ] *= 1.02;
        }
        resource_initial[ i ] += ( floor( intellect() ) - adjust ) * mana_per_intellect + adjust;
      }
      if ( i == RESOURCE_HEALTH ) 
      {
        resource_initial[ i ] += ( floor( stamina() ) - adjust ) * health_per_stamina + adjust;

        if ( buffs.hellscreams_warsong -> check() || buffs.strength_of_wrynn -> check() )
        {
          // ICC buff.
          resource_initial[ i ] *= 1.30;
        }
      }
    }
    resource_current[ i ] = resource_max[ i ] = resource_initial[ i ];
  }

  if ( timeline_resource.empty() )
  {
    int size = ( int ) sim -> max_time;
    if ( size == 0 ) size = 600; // Default to 10 minutes
    size *= 2;
    timeline_resource.insert( timeline_resource.begin(), size, 0 );
  }
}

// player_t::init_consumables ==============================================

void player_t::init_consumables()
{
  consumable_t::init_flask  ( this );
  consumable_t::init_elixirs( this );
  consumable_t::init_food   ( this );
}

// player_t::init_professions ==============================================

void player_t::init_professions()
{
  if ( professions_str.empty() ) return;

  std::vector<std::string> splits;
  int size = util_t::string_split( splits, professions_str, ",/" );

  for ( int i=0; i < size; i++ )
  {
    std::string prof_name;
    int prof_value=0;

    if ( 2 != util_t::string_split( splits[ i ], "=", "S i", &prof_name, &prof_value ) )
    {
      prof_name  = splits[ i ];
      prof_value = 450;
    }

    int prof_type = util_t::parse_profession_type( prof_name );
    if ( prof_type == PROFESSION_NONE )
    {
      sim -> errorf( "Invalid profession encoding: %s\n", professions_str.c_str() );
      return;
    }

    profession[ prof_type ] = prof_value;
  }

  // Miners gain additional stamina
  if      ( profession[ PROF_MINING ] >= 450 ) attribute_initial[ ATTR_STAMINA ] += 50.0;
  else if ( profession[ PROF_MINING ] >= 375 ) attribute_initial[ ATTR_STAMINA ] += 30.0;
  else if ( profession[ PROF_MINING ] >= 300 ) attribute_initial[ ATTR_STAMINA ] += 10.0;
  else if ( profession[ PROF_MINING ] >= 225 ) attribute_initial[ ATTR_STAMINA ] +=  7.0;
  else if ( profession[ PROF_MINING ] >= 150 ) attribute_initial[ ATTR_STAMINA ] +=  5.0;
  else if ( profession[ PROF_MINING ] >=  75 ) attribute_initial[ ATTR_STAMINA ] +=  3.0;

  // Skinners gain additional crit rating
  if      ( profession[ PROF_SKINNING ] >= 450 ) initial_attack_crit += 32.0 / rating.attack_crit;
  else if ( profession[ PROF_SKINNING ] >= 375 ) initial_attack_crit += 20.0 / rating.attack_crit;
  else if ( profession[ PROF_SKINNING ] >= 300 ) initial_attack_crit += 12.0 / rating.attack_crit;
  else if ( profession[ PROF_SKINNING ] >= 225 ) initial_attack_crit +=  9.0 / rating.attack_crit;
  else if ( profession[ PROF_SKINNING ] >= 150 ) initial_attack_crit +=  6.0 / rating.attack_crit;
  else if ( profession[ PROF_SKINNING ] >=  75 ) initial_attack_crit +=  3.0 / rating.attack_crit;
}

// player_t::init_actions ==================================================

void player_t::init_actions()
{
  if ( ! action_list_str.empty() )
  {
    if ( action_list_default && sim -> debug ) log_t::output( sim, "Player %s using default actions", name() );

    if ( sim -> debug ) log_t::output( sim, "Player %s: action_list_str=%s", name(), action_list_str.c_str() );

    std::vector<std::string> splits;
    int num_splits = util_t::string_split( splits, action_list_str, "/" );

    for ( int i=0; i < num_splits; i++ )
    {
      std::string action_name    = splits[ i ];
      std::string action_options = "";

      std::string::size_type cut_pt = action_name.find_first_of( ",:" );

      if ( cut_pt != action_name.npos )
      {
        action_options = action_name.substr( cut_pt + 1 );
        action_name    = action_name.substr( 0, cut_pt );
      }

      action_t* a = create_action( action_name, action_options );
      if ( ! a )
      {
        sim -> errorf( "Player %s has unknown action: %s\n", name(), splits[ i ].c_str() );
        return;
      }

      a -> if_expr = action_expr_t::parse( a, a -> if_expr_str );
    }
  }

  if ( ! action_list_skip.empty() )
  {
    if ( sim -> debug ) log_t::output( sim, "Player %s: action_list_skip=%s", name(), action_list_skip.c_str() );
    std::vector<std::string> splits;
    int num_splits = util_t::string_split( splits, action_list_skip, "/" );
    for ( int i=0; i < num_splits; i++ )
    {
      action_t* action = find_action( splits[ i ] );
      if ( action ) action -> background = true;
    }
  }
}

// player_t::init_rating ===================================================

void player_t::init_rating()
{
	 if ( sim -> debug ) log_t::output( sim, "player_t::init_rating(): level=%.f type=%.f",
	                   level,type );
  rating.init( sim, player_data, level, type );
}

// player_t::init_talents =================================================

void player_t::init_talents()
{
  uint32_t i = 0;
  uint32_t j = 0;
  uint32_t tab = 0;
  uint32_t num = 0;
  uint32_t talent_id = 0;
  uint32_t num_talents;
  uint32_t rank;
  uint32_t tab_sizes[ MAX_TALENT_TREES ];
  uint32_t tab_start[ MAX_TALENT_TREES + 1 ];
  std::vector<int>::const_iterator talent_iter;
  
  num_talents = talent_list2.size();

  uint32_t list_size = 0;
  
  tab_start[ 0 ] = 0;

  for ( uint32_t k = 0; k < MAX_TALENT_TREES; k ++ )
  {
    tab_sizes[ k ] = player_data.talent_player_get_num_talents( type, k );
    list_size += tab_sizes[ k ];
  }

  tab_start[ MAX_TALENT_TREES ] = list_size;

  for ( uint32_t k = 1; k < MAX_TALENT_TREES; k++ )
  {
    tab_start[ k ] = tab_start[ k - 1 ] + tab_sizes[ k - 1 ];
  }

  tab = 0;
  num = 0;

  talent_iter = talent_str.begin();

  for ( i = 0; ( i < list_size ) && ( talent_iter != talent_str.end() ); i++, num++, talent_iter++ )
  {
    if ( i >= tab_start[ tab + 1 ] )
    {
      tab++;
      num = 0;
    }

    rank = ( *talent_iter >= 0 ) && ( *talent_iter <= 3 ) ? *talent_iter : 0 ;

    talent_id = player_data.talent_player_get_id_by_num( type, tab, num );

    if ( talent_id != 0 )
    {
      for ( j = 0; j < num_talents; j++ )
      {
        if ( talent_list2[ j ] && talent_list2[ j ] -> t_data && talent_list2[ j ] -> t_data-> id == talent_id )
        {
          talent_str.begin();
          
          if ( rank > player_data.talent_max_rank( talent_id ) )
            rank = player_data.talent_max_rank( talent_id );
          talent_list2[ j ] -> set_rank( rank );
          talent_tab_points[ tab ] += rank;
          break;
        }
      }
    }   
  }

  pri_tree = primary_tab();
}

// player_t::init_spells =================================================

void player_t::init_spells()
{

}


// player_t::init_buffs ====================================================

void player_t::init_buffs()
{
  buffs.berserking           = new buff_t( this, "berserking",          1, 10.0 );
  buffs.heroic_presence      = new buff_t( this, "heroic_presence",     1       );
  buffs.replenishment        = new buff_t( this, 57669, "replenishment" );
  buffs.stoneform            = new buff_t( this, "stoneform",           1,  8.0 );
  buffs.hellscreams_warsong  = new buff_t( this, "hellscreams_warsong", 1       );
  buffs.strength_of_wrynn    = new buff_t( this, "strength_of_wrynn",   1       );
  buffs.dark_intent          = new buff_t( this, 85767, "dark_intent" );
  buffs.dark_intent_feedback = new buff_t( this, "dark_intent_feedback",3, 7.0  );


  // Infinite-Stacking Buffs
  buffs.moving  = new buff_t( this, "moving",  -1 );
  buffs.stunned = new buff_t( this, "stunned", -1 );

  buffs.self_movement = new buff_t( this, "self_movement", 1 );

  // stat_buff_t( sim, name, stat, amount, max_stack, duration, cooldown, proc_chance, quiet )
  buffs.blood_fury_ap          = new stat_buff_t( this, "blood_fury_ap",          STAT_ATTACK_POWER, ( level * 4 ) + 2, 1, 15.0 );
  buffs.blood_fury_sp          = new stat_buff_t( this, "blood_fury_sp",          STAT_SPELL_POWER,  ( level * 2 ) + 3, 1, 15.0 );
  buffs.destruction_potion     = new stat_buff_t( this, "destruction_potion",     STAT_SPELL_POWER,  120.0,             1, 15.0, 60.0 );
  buffs.indestructible_potion  = new stat_buff_t( this, "indestructible_potion",  STAT_ARMOR,        3500.0,            1, 15.0, 60.0 );
  buffs.speed_potion           = new stat_buff_t( this, "speed_potion",           STAT_HASTE_RATING, 500.0,             1, 15.0, 60.0 );
  buffs.earthen_potion         = new stat_buff_t( this, "earthen_potion",         STAT_ARMOR,        4800.0,            1, 25.0, 60.0 );
  buffs.golemblood_potion      = new stat_buff_t( this, "golemblood_potion",      STAT_STRENGTH,     1200.0,            1, 25.0, 60.0 );
  buffs.tolvir_potion          = new stat_buff_t( this, "tolvir_potion",          STAT_AGILITY,      1200.0,            1, 25.0, 60.0 );
  buffs.volcanic_potion        = new stat_buff_t( this, "volcanic_potion",        STAT_SPELL_POWER,  1200.0,            1, 25.0, 60.0 );
  buffs.wild_magic_potion_sp   = new stat_buff_t( this, "wild_magic_potion_sp",   STAT_SPELL_POWER,  200.0,             1, 15.0, 60.0 );
  buffs.wild_magic_potion_crit = new stat_buff_t( this, "wild_magic_potion_crit", STAT_CRIT_RATING,  200.0,             1, 15.0, 60.0 );
}

// player_t::init_gains ====================================================

void player_t::init_gains()
{
  gains.arcane_torrent         = get_gain( "arcane_torrent" );
  gains.blessing_of_wisdom     = get_gain( "blessing_of_wisdom" );
  gains.dark_rune              = get_gain( "dark_rune" );
  gains.energy_regen           = get_gain( "energy_regen" );
  gains.focus_regen            = get_gain( "focus_regen" );
  gains.innervate              = get_gain( "innervate" );
  gains.mana_potion            = get_gain( "mana_potion" );
  gains.mana_spring_totem      = get_gain( "mana_spring_totem" );
  gains.mp5_regen              = get_gain( "mp5_regen" );
  gains.replenishment          = get_gain( "replenishment" );
  gains.restore_mana           = get_gain( "restore_mana" );
  gains.spellsurge             = get_gain( "spellsurge" );
  gains.spirit_intellect_regen = get_gain( "spirit_intellect_regen" );
  gains.vampiric_embrace       = get_gain( "vampiric_embrace" );
  gains.vampiric_touch         = get_gain( "vampiric_touch" );
  gains.water_elemental        = get_gain( "water_elemental" );
}

// player_t::init_procs ====================================================

void player_t::init_procs()
{
  procs.hat_donor = get_proc( "hat_donor" );
}

// player_t::init_uptimes ==================================================

void player_t::init_uptimes()
{
}

// player_t::init_rng ======================================================

void player_t::init_rng()
{
  rngs.lag_channel = get_rng( "lag_channel" );
  rngs.lag_gcd     = get_rng( "lag_gcd"     );
  rngs.lag_queue   = get_rng( "lag_queue"   );
}

// player_t::init_stats ====================================================

void player_t::init_stats()
{
  for ( int i=0; i < RESOURCE_MAX; i++ )
  {
    resource_lost[ i ] = resource_gained[ i ] = 0;
  }

  iteration_dps.clear();
  iteration_dps.insert( iteration_dps.begin(), sim -> iterations, 0 );
}

// player_t::init_values ====================================================

void player_t::init_values()
{

}

// player_t::init_scaling ==================================================

void player_t::init_scaling()
{
  if ( ! is_pet() )
  {
    int role = primary_role();

    int attack = ( ( role == ROLE_ATTACK ) || ( role == ROLE_HYBRID ) ) ? 1 : 0;
    int spell  = ( ( role == ROLE_SPELL  ) || ( role == ROLE_HYBRID ) ) ? 1 : 0;

    scales_with[ STAT_STRENGTH  ] = attack;
    scales_with[ STAT_AGILITY   ] = attack;
    scales_with[ STAT_STAMINA   ] = 0;
    scales_with[ STAT_INTELLECT ] = spell;
    scales_with[ STAT_SPIRIT    ] = spell;

    scales_with[ STAT_HEALTH ] = 0;
    scales_with[ STAT_MANA   ] = 0;
    scales_with[ STAT_RAGE   ] = 0;
    scales_with[ STAT_ENERGY ] = 0;
    scales_with[ STAT_FOCUS  ] = 0;
    scales_with[ STAT_RUNIC  ] = 0;

    scales_with[ STAT_SPELL_POWER       ] = spell;
    scales_with[ STAT_SPELL_PENETRATION ] = 0;
    scales_with[ STAT_MP5               ] = 0;

    scales_with[ STAT_ATTACK_POWER             ] = attack;
    scales_with[ STAT_EXPERTISE_RATING         ] = attack;

    scales_with[ STAT_HIT_RATING   		] = 1;
    scales_with[ STAT_CRIT_RATING  		] = 1;
    scales_with[ STAT_HASTE_RATING 		] = 1;
    scales_with[ STAT_MASTERY_RATING 	        ] = 1;

    scales_with[ STAT_WEAPON_DPS   ] = attack;
    scales_with[ STAT_WEAPON_SPEED ] = sim -> weapon_speed_scale_factors ? attack : 0;

    scales_with[ STAT_WEAPON_OFFHAND_DPS   ] = 0;
    scales_with[ STAT_WEAPON_OFFHAND_SPEED ] = 0;

    scales_with[ STAT_ARMOR          ] = 0;
    scales_with[ STAT_BONUS_ARMOR    ] = 0;
    scales_with[ STAT_DODGE_RATING   ] = 0;
    scales_with[ STAT_PARRY_RATING   ] = 0;

    scales_with[ STAT_BLOCK_RATING ] = 0;
    scales_with[ STAT_BLOCK_VALUE  ] = 0;

    if ( sim -> scaling -> scale_stat != STAT_NONE )
    {
      double v = sim -> scaling -> scale_value;

      switch ( sim -> scaling -> scale_stat )
      {
      case STAT_STRENGTH:  attribute_initial[ ATTR_STRENGTH  ] += v; break;
      case STAT_AGILITY:   attribute_initial[ ATTR_AGILITY   ] += v; break;
      case STAT_STAMINA:   attribute_initial[ ATTR_STAMINA   ] += v; break;
      case STAT_INTELLECT: attribute_initial[ ATTR_INTELLECT ] += v; break;
      case STAT_SPIRIT:    attribute_initial[ ATTR_SPIRIT    ] += v; break;

      case STAT_SPELL_POWER:       initial_spell_power[ SCHOOL_MAX ] += v; break;
      case STAT_SPELL_PENETRATION: initial_spell_penetration         += v; break;
      case STAT_MP5:               initial_mp5                       += v; break;

      case STAT_ATTACK_POWER:             initial_attack_power       += v;                            break;
      case STAT_EXPERTISE_RATING:         initial_attack_expertise   += v / rating.expertise;         break;

      case STAT_HIT_RATING:
        initial_attack_hit += v / rating.attack_hit;
        initial_spell_hit  += v / rating.spell_hit;
        break;

      case STAT_CRIT_RATING:
        initial_attack_crit += v / rating.attack_crit;
        initial_spell_crit  += v / rating.spell_crit;
        break;

      case STAT_HASTE_RATING: initial_haste_rating += v; break;
      case STAT_MASTERY_RATING: initial_mastery_rating += v; break;

      case STAT_WEAPON_DPS:
        if ( main_hand_weapon.damage > 0 )
        {
          main_hand_weapon.damage  += main_hand_weapon.swing_time * v;
          main_hand_weapon.min_dmg += main_hand_weapon.swing_time * v;
          main_hand_weapon.max_dmg += main_hand_weapon.swing_time * v;
        }
        if ( ranged_weapon.damage > 0 )
        {
          ranged_weapon.damage     += ranged_weapon.swing_time * v;
          ranged_weapon.min_dmg    += ranged_weapon.swing_time * v;
          ranged_weapon.max_dmg    += ranged_weapon.swing_time * v;
        }
        break;

      case STAT_WEAPON_SPEED:
        if ( main_hand_weapon.swing_time > 0 )
        {
          double new_speed = ( main_hand_weapon.swing_time + v );
          double mult = new_speed / main_hand_weapon.swing_time;

          main_hand_weapon.min_dmg *= mult;
          main_hand_weapon.max_dmg *= mult;
          main_hand_weapon.damage  *= mult;

          main_hand_weapon.swing_time = new_speed;
        }
        if ( ranged_weapon.swing_time > 0 )
        {
          double new_speed = ( ranged_weapon.swing_time + v );

          double mult = new_speed / ranged_weapon.swing_time;

          ranged_weapon.min_dmg *= mult;
          ranged_weapon.max_dmg *= mult;
          ranged_weapon.damage  *= mult;

          ranged_weapon.swing_time = new_speed;
        }
        break;

      case STAT_WEAPON_OFFHAND_DPS:
        if ( off_hand_weapon.damage > 0 )
        {
          off_hand_weapon.damage   += off_hand_weapon.swing_time * v;
          off_hand_weapon.min_dmg  += off_hand_weapon.swing_time * v;
          off_hand_weapon.max_dmg  += off_hand_weapon.swing_time * v;
        }
        break;

      case STAT_WEAPON_OFFHAND_SPEED:
        if ( off_hand_weapon.swing_time > 0 )
        {
          double new_speed = ( off_hand_weapon.swing_time + v );
          double mult = new_speed / off_hand_weapon.swing_time;

          off_hand_weapon.min_dmg *= mult;
          off_hand_weapon.max_dmg *= mult;
          off_hand_weapon.damage  *= mult;

          off_hand_weapon.swing_time = new_speed;
        }
        break;

      case STAT_ARMOR:          initial_armor       += v; break;
      case STAT_BONUS_ARMOR:    initial_bonus_armor += v; break;
      case STAT_DODGE_RATING:   initial_dodge       += v; break;
      case STAT_PARRY_RATING:   initial_parry       += v; break;

      case STAT_BLOCK_RATING: initial_block       += v; break;
      case STAT_BLOCK_VALUE:  initial_block_value += v; break;


      case STAT_MAX: break;

      default: assert( 0 );
      }
    }
  }
}

// player_t::find_item ======================================================

item_t* player_t::find_item( const std::string& str )
{
  int num_items = ( int ) items.size();
  for ( int i=0; i < num_items; i++ )
    if ( str == items[ i ].name() )
      return &( items[ i ] );

  return 0;
}

// player_t::composite_attack_haste ========================================

double player_t::composite_attack_haste() SC_CONST
{
  double h = attack_haste;

  if ( type != PLAYER_GUARDIAN )
  {
    if ( buffs.bloodlust -> up() )
    {
      h *= 1.0 / ( 1.0 + 0.30 );
    }

    if ( buffs.berserking -> up() )
    {
      h *= 1.0 / ( 1.0 + 0.20 );
    }

    if ( buffs.unholy_frenzy -> up() )
    {
      h *= 1.0 / ( 1.0 + 0.20 );
    }

    if ( type != PLAYER_PET )
    {
      if ( buffs.mongoose_mh && buffs.mongoose_mh -> up() ) h *= 1.0 / ( 1.0 + 30 / rating.attack_haste );
      if ( buffs.mongoose_oh && buffs.mongoose_oh -> up() ) h *= 1.0 / ( 1.0 + 30 / rating.attack_haste );
    }
    if ( ( race == RACE_GOBLIN ) )
    {
      h *= 1.0 / ( 1.0 + 0.01 );
    }
  }

  return h;
}

// player_t::composite_attack_power ========================================

double player_t::composite_attack_power() SC_CONST
{
  double ap = attack_power;

  ap += attack_power_per_strength * strength();
  ap += attack_power_per_agility  * agility();

  return ap;
}

// player_t::composite_attack_crit =========================================

double player_t::composite_attack_crit() SC_CONST
{
  double ac = attack_crit + attack_crit_per_agility * agility();

  if ( type != PLAYER_GUARDIAN )
  {
    if ( sim -> auras.leader_of_the_pack -> check() 
      || sim -> auras.honor_among_thieves -> check() 
      || sim -> auras.elemental_oath -> check()
      || sim -> auras.rampage -> check() )
    {
      ac += 0.05;
    }
  }
  if ( race == RACE_WORGEN )
  {
    ac += 0.01;
  }
  return ac;
}

// player_t::composite_attack_hit ==========================================

double player_t::composite_attack_hit() SC_CONST
{
  double ah = attack_hit;

  // Changes here may need to be reflected in the corresponding pet_t
  // function in simulationcraft.h
  if ( buffs.heroic_presence -> check() ) ah += 0.01;

  return ah;
}

// player_t::composite_armor =========================================

double player_t::composite_armor() SC_CONST
{
  double a = armor;

  a *= armor_multiplier;

  a += bonus_armor;

  a += armor_per_agility * floor( agility() );

  if ( sim -> auras.devotion_aura -> check() )
    a += sim -> auras.devotion_aura -> value();

  if ( buffs.stoneform -> check() ) a *= 1.10;

  return a;
}

// player_t::composite_tank_miss ===========================================

double player_t::composite_tank_miss( const school_type school ) SC_CONST
{
  double m = 0;

  if ( school == SCHOOL_PHYSICAL )
  {
    m = 0.05;

    double delta = 5.0 * ( level - sim -> target -> level );

    if( delta > 0 )
    {
      m += delta * 0.0004;
    }
    else
    {
      m += delta * 0.0002;
    }

    if ( race == RACE_NIGHT_ELF ) // Quickness
    {
      m += 0.02;
    }
  }
  else
  {
    m = 0.04;

    int delta = level - sim -> target -> level;

    if( delta > 0 )
    {
      m += delta * 0.01;
    }
    else if ( delta > -2 )
    {
      m += delta * 0.01;
    }
    else
    {
      m += 0.02 - ( 2 + delta ) * 0.07;
    }

    if ( ( race == RACE_NIGHT_ELF && school == SCHOOL_NATURE ) ||
         ( race == RACE_DWARF     && school == SCHOOL_FROST  ) ||
         ( race == RACE_GNOME     && school == SCHOOL_ARCANE ) ||
         ( race == RACE_DRAENEI   && school == SCHOOL_SHADOW ) ||
         ( race == RACE_TAUREN    && school == SCHOOL_NATURE ) ||
         ( race == RACE_UNDEAD    && school == SCHOOL_SHADOW ) ||
         ( race == RACE_BLOOD_ELF ) )
    {
      m += 0.02;
    }
  }

  if      ( m > 1.0 ) m = 1.0;
  else if ( m < 0.0 ) m = 0.0;

  return m;
}

// player_t::composite_tank_dodge ====================================

double player_t::composite_tank_dodge() SC_CONST
{
  double d = dodge;

  d += agility() * dodge_per_agility;

  double delta = 5.0 * ( level - sim -> target -> level );

  if( delta > 0 )
  {
    d += delta * 0.0004;
  }
  else
  {
    d += delta * 0.0002;
  }

  if      ( d > 1.0 ) d = 1.0;
  else if ( d < 0.0 ) d = 0.0;

  return d;
}

// player_t::composite_tank_parry ====================================

double player_t::composite_tank_parry() SC_CONST
{
  double p = parry;

  double delta = 5.0 * ( level - sim -> target -> level );

  if( delta > 0 )
  {
    p += delta * 0.0004;
  }
  else
  {
    p += delta * 0.0002;
  }

  if      ( p > 1.0 ) p = 1.0;
  else if ( p < 0.0 ) p = 0.0;

  return p;
}

// player_t::composite_tank_block ===================================

double player_t::composite_tank_block() SC_CONST
{
  double b = block;

  double delta = 5.0 * ( level - sim -> target -> level );

  if( delta > 0 )
  {
    b += delta * 0.0004;
  }
  else
  {
    b += delta * 0.0002;
  }

  if      ( b > 1.0 ) b = 1.0;
  else if ( b < 0.0 ) b = 0.0;

  return b;
}

// player_t::composite_block_value ===================================

double player_t::composite_block_value() SC_CONST
{
  double bv = block_value;

  bv += strength() / 2.0;

  return bv;
}

// player_t::composite_tank_crit ==========================================

double player_t::composite_tank_crit( const school_type school ) SC_CONST
{
  double c = 0;

  if ( school == SCHOOL_PHYSICAL )
  {
    c = 0.05 + 0.002 * ( sim -> target -> level - level );

    double delta = 5.0 * ( level - sim -> target -> level );

    if( delta > 0 )
    {
      c -= delta * 0.0004;
    }
    else
    {
      c -= delta * 0.0002;
    }
  }
  else
  {
    c = 0.05;
  }

  if      ( c > 1.0 ) c = 1.0;
  else if ( c < 0.0 ) c = 0.0;

  return c;
}

// player_t::diminished_dodge ========================================

double player_t::diminished_dodge() SC_CONST
{
  if ( diminished_kfactor == 0 || diminished_dodge_capi == 0 ) return 0;

  // Only contributions from gear are subject to diminishing returns;

  double d = stats.dodge_rating / rating.dodge;

  d += dodge_per_agility * stats.attribute[ ATTR_AGILITY ] * composite_attribute_multiplier( ATTR_AGILITY );

  if ( d == 0 ) return 0;

  double diminished_d = 0.01 / ( diminished_dodge_capi + diminished_kfactor / d );

  double loss = d - diminished_d;

  return loss > 0 ? loss : 0;
}

// player_t::diminished_parry ========================================

double player_t::diminished_parry() SC_CONST
{
  if ( diminished_kfactor == 0 || diminished_parry_capi == 0 ) return 0;

  // Only contributions from gear are subject to diminishing returns;

  double p = stats.parry_rating / rating.parry;

  if ( p == 0 ) return 0;

  double diminished_p = 0.01 / ( diminished_parry_capi + diminished_kfactor / p );

  double loss = p - diminished_p;

  return loss > 0 ? loss : 0;
}

// player_t::composite_spell_haste ========================================

double player_t::composite_spell_haste() SC_CONST
{
  double h = spell_haste;

  if ( type != PLAYER_GUARDIAN )
  {
    if ( buffs.dark_intent -> check() ) h *= 1.0 / 1.03;
    if (      buffs.bloodlust      -> check() ) h *= 1.0 / ( 1.0 + 0.30 );
    else if ( buffs.power_infusion -> check() ) h *= 1.0 / ( 1.0 + 0.20 );

    if ( buffs.berserking -> check() )          h *= 1.0 / ( 1.0 + 0.20 );

    if ( sim -> auras.wrath_of_air -> check() || sim -> auras.moonkin -> check() || sim -> auras.mind_quickening -> check())
    {
      h *= 1.0 / ( 1.0 + 0.05 );
    }

    if ( sim -> auras.celerity -> check() )
    {
      h *= 1.0 / ( 1.0 + 0.20 );
    }
    if ( ( race == RACE_GOBLIN ) )
    {
      h *= 1 / ( 1.0 + 0.01 );
    }
  }

  return h;
}


// player_t::composite_spell_power ========================================

double player_t::composite_spell_power( const school_type school ) SC_CONST
{
  double sp = spell_power[ school ];

  if ( school == SCHOOL_FROSTFIRE )
  {
    sp = std::max( spell_power[ SCHOOL_FROST ],
                   spell_power[ SCHOOL_FIRE  ] );
  }
  else if ( school == SCHOOL_SPELLSTORM )
  {
    sp = std::max( spell_power[ SCHOOL_NATURE ],
                   spell_power[ SCHOOL_ARCANE ] );
  }
  else if ( school == SCHOOL_SHADOWFROST )
  {
    sp = std::max( spell_power[ SCHOOL_SHADOW ],
                   spell_power[ SCHOOL_FROST ] );
  }
  else if ( school == SCHOOL_SHADOWFLAME )
  {
    sp = std::max( spell_power[ SCHOOL_SHADOW ],
                   spell_power[ SCHOOL_FIRE ] );
  }
  if ( school != SCHOOL_MAX ) sp += spell_power[ SCHOOL_MAX ];

  sp += spell_power_per_intellect * ( intellect() - 10 ); // The spellpower is always lower by 10, cata beta build 12803
  sp += spell_power_per_spirit    * spirit();

  return sp;
}

// player_t::composite_spell_power_multiplier =============================

double player_t::composite_spell_power_multiplier() SC_CONST
{
  double m = spell_power_multiplier;
  /**/
  if ( sim -> auras.demonic_pact -> check() )
  {
    m *= 1.10;
  }
  else
  {
     m *= 1.0 + std::max( sim -> auras.flametongue_totem -> value(),
                          buffs.arcane_brilliance -> check() * 0.06 );
  }

  return m;
}

// player_t::composite_spell_crit ==========================================

double player_t::composite_spell_crit() SC_CONST
{
  double sc = spell_crit + spell_crit_per_intellect * intellect();

  if ( type != PLAYER_GUARDIAN )
  {
    if ( buffs.focus_magic -> check() ) sc += 0.03;

    if ( sim -> auras.leader_of_the_pack -> check() 
      || sim -> auras.honor_among_thieves -> check() 
      || sim -> auras.elemental_oath -> check()
      || sim -> auras.rampage -> check() )
    {
      sc += 0.05;
    }

    if ( buffs.destruction_potion -> check() ) sc += 0.02;
  }

  if ( ( race == RACE_WORGEN ) )
  {
    sc += 0.01;
  }

  return sc;
}

// player_t::composite_spell_hit ===========================================

double player_t::composite_spell_hit() SC_CONST
{
  double sh = spell_hit;

  // Changes here may need to be reflected in the corresponding pet_t
  // function in simulationcraft.h
  if ( buffs.heroic_presence -> check() ) sh += 0.01;

  return sh;
}

// player_t::composite_mp5 =================================================

double player_t::composite_mp5() SC_CONST
{
  return mp5 + mp5_per_intellect * floor( intellect() );
}

// player_t::composite_attack_power_multiplier =============================

double player_t::composite_attack_power_multiplier() SC_CONST
{
  double m = attack_power_multiplier;

  if ( sim -> auras.trueshot -> up() || buffs.blessing_of_might -> up() )
  {
    m *= 1.10;
  }
  else
  {
    m *= 1.0 + std::max( sim -> auras.unleashed_rage -> value(), sim -> auras.abominations_might -> value() );
  }

  return m;
}

// player_t::composite_attribute_multiplier ================================

double player_t::composite_attribute_multiplier( int attr ) SC_CONST
{
  double m = attribute_multiplier[ attr ];
  // MotW / BoK
  // ... increasing Strength, Agility, Stamina, and Intellect by 5%
  if ( attr == ATTR_STRENGTH || attr ==  ATTR_AGILITY || attr ==  ATTR_STAMINA || attr ==  ATTR_INTELLECT )
    if ( buffs.blessing_of_kings -> check() || buffs.mark_of_the_wild -> check() ) m *= 1.05;
  if ( attr == ATTR_SPIRIT ) 
    if ( buffs.mana_tide -> check() ) m *= 1.0 + buffs.mana_tide -> value();

  return m;
}

// player_t::composite_player_multiplier ================================

double player_t::composite_player_multiplier( const school_type school ) SC_CONST
{
  double m = 1.0;

  if ( type != PLAYER_GUARDIAN )
  {
    if ( buffs.hellscreams_warsong -> check() || buffs.strength_of_wrynn -> check() )
    {
      // ICC buff.
      m *= 1.30;
    }

    if ( buffs.tricks_of_the_trade -> up() )
    {
      // because of the glyph we now track the damage % increase in the buff value
      m *= 1.0 + buffs.tricks_of_the_trade -> value();
    }

    if ( sim -> auras.ferocious_inspiration  -> up()
      || sim -> auras.sanctified_retribution -> up()
      || sim -> auras.arcane_tactics         -> up() )
    {
      m *= 1.03;
    }
  }

  if ( ( race == RACE_TROLL ) && ( sim -> target -> race == RACE_BEAST ) )
  {
    m *= 1.05;
  }

  return m;
}

// player_t::strength() ====================================================

double player_t::strength() SC_CONST
{
  double a = attribute[ ATTR_STRENGTH ];
  a += std::max( std::max( sim -> auras.strength_of_earth -> value(),
                           sim -> auras.horn_of_winter -> value() ),
                           sim -> auras.battle_shout -> value() );
  a *= composite_attribute_multiplier( ATTR_STRENGTH );
  return a;
}

// player_t::agility() =====================================================

double player_t::agility() SC_CONST
{
  double a = attribute[ ATTR_AGILITY ];
  a += std::max( std::max( sim -> auras.strength_of_earth -> value(),
                           sim -> auras.horn_of_winter -> value() ),
                           sim -> auras.battle_shout -> value() );
  a *= composite_attribute_multiplier( ATTR_AGILITY );
  return a;
}

// player_t::stamina() =====================================================

double player_t::stamina() SC_CONST
{
  double a = attribute[ ATTR_STAMINA ];
  a *= composite_attribute_multiplier( ATTR_STAMINA );
  return a;
}

// player_t::intellect() ===================================================

double player_t::intellect() SC_CONST
{
  double a = attribute[ ATTR_INTELLECT ];
  a *= composite_attribute_multiplier( ATTR_INTELLECT );
  return a;
}

// player_t::spirit() ======================================================

double player_t::spirit() SC_CONST
{
  double a = attribute[ ATTR_SPIRIT ];
  if ( race == RACE_HUMAN )
  {
    a += ( a - attribute_base[ ATTR_SPIRIT ] ) * 0.03;
  }
  a *= composite_attribute_multiplier( ATTR_SPIRIT );
  return a;
}

// player_t::combat_begin ==================================================

void player_t::combat_begin( sim_t* sim )
{
  player_t::death_knight_combat_begin( sim );
  player_t::druid_combat_begin       ( sim );
  player_t::hunter_combat_begin      ( sim );
  player_t::mage_combat_begin        ( sim );
  player_t::paladin_combat_begin     ( sim );
  player_t::priest_combat_begin      ( sim );
  player_t::rogue_combat_begin       ( sim );
  player_t::shaman_combat_begin      ( sim );
  player_t::warlock_combat_begin     ( sim );
  player_t::warrior_combat_begin     ( sim );
}

// player_t::combat_begin ==================================================

void player_t::combat_begin()
{
  if ( sim -> debug ) log_t::output( sim, "Combat begins for player %s", name() );

  if ( ! is_pet() && ! sleeping )
  {
    schedule_ready();
  }

  if ( sim -> overrides.strength_of_wrynn )
  {
    buffs.strength_of_wrynn -> trigger();
  }
  if ( sim -> overrides.hellscreams_warsong )
  {
    buffs.hellscreams_warsong -> trigger();
  }

  if ( race == RACE_DRAENEI )
  {
    buffs.heroic_presence -> trigger();
  }

  if ( sim -> overrides.replenishment ) buffs.replenishment -> override();

  init_resources( true );

  if ( primary_resource() == RESOURCE_MANA )
  {
    get_gain( "initial_mana" ) -> add( resource_max[ RESOURCE_MANA ] );
  }

  if ( tank > 0 ) trigger_target_swings( this );
}

// player_t::combat_end ====================================================

void player_t::combat_end( sim_t* sim )
{
  player_t::death_knight_combat_end( sim );
  player_t::druid_combat_end       ( sim );
  player_t::hunter_combat_end      ( sim );
  player_t::mage_combat_end        ( sim );
  player_t::paladin_combat_end     ( sim );
  player_t::priest_combat_end      ( sim );
  player_t::rogue_combat_end       ( sim );
  player_t::shaman_combat_end      ( sim );
  player_t::warlock_combat_end     ( sim );
  player_t::warrior_combat_end     ( sim );
}

// player_t::combat_end ====================================================

void player_t::combat_end()
{
  if ( sim -> debug ) log_t::output( sim, "Combat ends for player %s", name() );

  for( buff_t* b = buff_list; b; b = b -> next )
  {
    b -> expire();
  }

  double iteration_seconds = current_time;

  if ( iteration_seconds > 0 )
  {
    total_seconds += iteration_seconds;

    for ( pet_t* pet = pet_list; pet; pet = pet -> next_pet )
    {
      iteration_dmg += pet -> iteration_dmg;
    }
    iteration_dps[ sim -> current_iteration ] = iteration_dmg / iteration_seconds;
  }
}

// player_t::reset =========================================================

void player_t::reset()
{
  if ( sim -> debug ) log_t::output( sim, "Reseting player %s", name() );

  skill = initial_skill;

  last_cast = 0;
  gcd_ready = 0;

  stats = initial_stats;

  haste_rating = initial_haste_rating;
  mastery_rating = initial_mastery_rating;
  mastery = base_mastery + mastery_rating / rating.mastery;
  recalculate_haste();

  for ( int i=0; i < ATTRIBUTE_MAX; i++ )
  {
    attribute           [ i ] = attribute_initial           [ i ];
    attribute_multiplier[ i ] = attribute_multiplier_initial[ i ];
    // Matched gear. i.e. Mysticism etc.
    if ( ( level >= 50 ) && matching_gear )
      attribute_multiplier[ i ] *= 1.0 + matching_gear_multiplier( (const attribute_type) i );
  }

  for ( int i=0; i <= SCHOOL_MAX; i++ )
  {
    spell_power[ i ] = initial_spell_power[ i ];
  }

  spell_hit         = initial_spell_hit;
  spell_crit        = initial_spell_crit;
  spell_penetration = initial_spell_penetration;
  mp5               = initial_mp5;

  attack_power       = initial_attack_power;
  attack_hit         = initial_attack_hit;
  attack_expertise   = initial_attack_expertise;
  attack_crit        = initial_attack_crit;

  armor              = initial_armor;
  bonus_armor        = initial_bonus_armor;
  dodge              = initial_dodge;
  parry              = initial_parry;
  block              = initial_block;
  block_value        = initial_block_value;

  spell_power_multiplier    = initial_spell_power_multiplier;
  spell_power_per_intellect = initial_spell_power_per_intellect;
  spell_power_per_spirit    = initial_spell_power_per_spirit;
  spell_crit_per_intellect  = initial_spell_crit_per_intellect;

  attack_power_multiplier   = initial_attack_power_multiplier;
  attack_power_per_strength = initial_attack_power_per_strength;
  attack_power_per_agility  = initial_attack_power_per_agility;
  attack_crit_per_agility   = initial_attack_crit_per_agility;

  armor_multiplier  = initial_armor_multiplier;
  armor_per_agility = initial_armor_per_agility;
  dodge_per_agility = initial_dodge_per_agility;

  for ( buff_t* b = buff_list; b; b = b -> next )
  {
    b -> reset();
  }

  last_foreground_action = 0;
  current_time = 0;

  executing = 0;
  channeling = 0;
  readying = 0;
  in_combat = false;
  iteration_dmg = 0;

  main_hand_weapon.buff_type  = 0;
  main_hand_weapon.buff_value = 0;

  off_hand_weapon.buff_type  = 0;
  off_hand_weapon.buff_value = 0;

  ranged_weapon.buff_type  = 0;
  ranged_weapon.buff_value = 0;

  elixir_battle   = ELIXIR_NONE;
  elixir_guardian = ELIXIR_NONE;
  flask           = FLASK_NONE;
  food            = FOOD_NONE;

  for ( int i=0; i < RESOURCE_MAX; i++ )
  {
    action_callback_t::reset( resource_gain_callbacks[ i ] );
    action_callback_t::reset( resource_loss_callbacks[ i ] );
  }
  for ( int i=0; i < RESULT_MAX; i++ )
  {
    action_callback_t::reset( attack_result_callbacks       [ i ] );
    action_callback_t::reset( spell_result_callbacks        [ i ] );
    action_callback_t::reset( attack_direct_result_callbacks[ i ] );
    action_callback_t::reset( spell_direct_result_callbacks [ i ] );
    action_callback_t::reset( spell_cast_result_callbacks   [ i ] );
    action_callback_t::reset( harmful_cast_result_callbacks [ i ] );
  }
  for ( int i=0; i < SCHOOL_MAX; i++ )
  {
    action_callback_t::reset( tick_damage_callbacks         [ i ] );
    action_callback_t::reset( direct_damage_callbacks       [ i ] );
  }
  action_callback_t::reset( tick_callbacks );

  replenishment_targets.clear();

  init_resources( true );

  for ( action_t* a = action_list; a; a = a -> next ) a -> reset();

  for ( cooldown_t* c = cooldown_list; c; c = c -> next ) c -> reset();

  for ( dot_t* d = dot_list; d; d = d -> next ) d -> reset();

  potion_used = 0;

  if ( sleeping ) quiet = 1;
}

// player_t::schedule_ready =================================================

void player_t::schedule_ready( double delta_time,
                               bool   waiting )
{
  assert( ! readying );

  executing = 0;
  channeling = 0;
  action_queued = false;

  if ( ! has_foreground_actions( this ) ) return;

  double gcd_adjust = gcd_ready - ( sim -> current_time + delta_time );
  if ( gcd_adjust > 0 ) delta_time += gcd_adjust;

  if ( waiting )
  {
    total_waiting += delta_time;
  }
  else
  {
    double lag = 0;

    if ( last_foreground_action && ! last_foreground_action -> auto_cast )
    {
      if ( last_foreground_action -> gcd() == 0 )
      {
        lag = 0;
      }
      else if ( last_foreground_action -> channeled )
      {
        lag = rngs.lag_channel -> gauss( sim -> channel_lag, sim -> channel_lag_stddev );
      }
      else
      {
        double   gcd_lag = rngs.lag_gcd   -> gauss( sim ->   gcd_lag, sim ->   gcd_lag_stddev );
        double queue_lag = rngs.lag_queue -> gauss( sim -> queue_lag, sim -> queue_lag_stddev );

        double diff = ( gcd_ready + gcd_lag ) - ( sim -> current_time + queue_lag );

        if ( diff > 0 && ( last_foreground_action -> time_to_execute == 0 || sim -> strict_gcd_queue ) )
        {
          lag = gcd_lag;
        }
        else
        {
          lag = queue_lag;
          action_queued = true;
        }
      }
    }

    if ( lag < 0 ) lag = 0;

    delta_time += lag;
  }

  if ( last_foreground_action )
  {
    last_foreground_action -> stats -> total_execute_time += delta_time;
  }

  if ( delta_time == 0 ) delta_time = 0.000001;

  readying = new ( sim ) player_ready_event_t( sim, this, delta_time );
}

// player_t::interrupt ======================================================

void player_t::interrupt()
{
  // FIXME! Players will need to override this to handle background repeating actions.

  if ( sim -> log ) log_t::output( sim, "%s is interrupted", name() );

  if ( executing  ) executing  -> cancel();
  if ( channeling ) channeling -> cancel();

  if ( buffs.stunned -> check() )
  {
    if ( readying ) event_t::cancel( readying );
  }
  else
  {
    if ( ! readying && ! sleeping ) schedule_ready();
  }
}

// player_t::clear_debuffs===================================================

void player_t::clear_debuffs()
{
  // FIXME! At the moment we are just clearing DoTs

  if ( sim -> log ) log_t::output( sim, "%s clears debuffs from %s", name(), sim -> target -> name() );

  for ( action_t* a = action_list; a; a = a -> next )
  {
    if ( a -> ticking ) a -> cancel();
  }
}

// player_t::execute_action =================================================

action_t* player_t::execute_action()
{
  readying = 0;

  action_t* action=0;

  for ( action = action_list; action; action = action -> next )
  {
    if ( action -> background ||
         action -> sequence )
      continue;

    if ( action -> ready() )
      break;

    if ( action -> wait_on_ready == 1 )
    {
      action = 0;
      break;
    }
  }

  last_foreground_action = action;

  if ( action )
  {
    action -> schedule_execute();
    total_foreground_actions++;
  }

  return action;
}

// player_t::regen =========================================================

void player_t::regen( double periodicity )
{
  int resource_type = primary_resource();

  if ( sim -> infinite_resource[ resource_type ] == 0 )
  {
    if ( resource_type == RESOURCE_ENERGY )
    {
      double energy_regen = periodicity * energy_regen_per_second;

      resource_gain( RESOURCE_ENERGY, energy_regen, gains.energy_regen );
    }
    else if ( resource_type == RESOURCE_FOCUS )
    {
      double focus_regen = periodicity * focus_regen_per_second;

      resource_gain( RESOURCE_FOCUS, focus_regen, gains.focus_regen );
    }
    else if ( resource_type == RESOURCE_MANA )
    {
      if( buffs.innervate -> check() )
      {
        resource_gain( RESOURCE_MANA, buffs.innervate -> value() * periodicity, gains.innervate );
      }

      double spirit_regen = periodicity * sqrt( floor( intellect() ) ) * floor( spirit() ) * mana_regen_base;

      if ( mana_regen_while_casting < 1.0 )
      {
        spirit_regen *= mana_regen_while_casting;
      }
      if( spirit_regen > 0 )
      {
        resource_gain( RESOURCE_MANA, spirit_regen, gains.spirit_intellect_regen );
      }

      double mp5_regen = periodicity * composite_mp5() / 5.0;

      resource_gain( RESOURCE_MANA, mp5_regen, gains.mp5_regen );

      if ( buffs.replenishment -> up() )
      {
        double replenishment_regen = periodicity * resource_max[ RESOURCE_MANA ] * 0.0010 / 1.0;

        resource_gain( RESOURCE_MANA, replenishment_regen, gains.replenishment );
      }

      double bow = buffs.blessing_of_wisdom -> current_value;
      double ms  = sim -> auras.mana_spring_totem -> current_value;

      if ( ms > bow )
      {
        double mana_spring_regen = periodicity * ms / 5.0;

        resource_gain( RESOURCE_MANA, mana_spring_regen, gains.mana_spring_totem );
      }
      else if ( bow > 0 )
      {
        double wisdom_regen = periodicity * bow / 5.0;

        resource_gain( RESOURCE_MANA, wisdom_regen, gains.blessing_of_wisdom );
      }
    }
  }

  if ( resource_type != RESOURCE_NONE )
  {
    int index = ( int ) sim -> current_time;
    int size = ( int ) timeline_resource.size();

    if ( index >= size ) timeline_resource.insert( timeline_resource.begin() + size, size, 0 );

    timeline_resource[ index ] += resource_current[ resource_type ];
  }
}

// player_t::resource_loss =================================================

double player_t::resource_loss( int       resource,
                                double    amount,
                                action_t* action )
{
  if ( amount == 0 ) return 0;

  double actual_amount;

  if ( sim -> infinite_resource[ resource ] == 0 )
  {
    resource_current[ resource ] -= amount;
    actual_amount = amount;
  }
  else
  {
    actual_amount = 0;
  }

  resource_lost[ resource ] += amount;

  if ( resource == RESOURCE_MANA )
  {
    last_cast = sim -> current_time;
  }

  if ( action ) action_callback_t::trigger( resource_loss_callbacks[ resource ], action );

  if ( sim -> debug ) log_t::output( sim, "Player %s loses %.0f %s", name(), amount, util_t::resource_type_string( resource ) );

  return actual_amount;
}

// player_t::resource_gain =================================================

double player_t::resource_gain( int       resource,
                                double    amount,
                                gain_t*   source,
                                action_t* action )
{
  if ( sleeping ) return 0;

  double actual_amount = std::min( amount, resource_max[ resource ] - resource_current[ resource ] );
  if ( actual_amount > 0 )
  {
    resource_current[ resource ] += actual_amount;
  }

  resource_gained [ resource ] += actual_amount;

  if ( source ) source -> add( actual_amount, amount - actual_amount );

  if ( action ) action_callback_t::trigger( resource_gain_callbacks[ resource ], action );

  if ( sim -> log )
  {
    log_t::output( sim, "%s gains %.0f (%.0f) %s from %s (%.0f)",
                   name(), actual_amount, amount,
                   util_t::resource_type_string( resource ), source ? source -> name() : "unknown",
                   resource_current[ resource ] );

    if ( amount && source )
    {
      log_t::resource_gain_event( this, resource, amount, actual_amount, source );
    }
  }

  return actual_amount;
}

// player_t::resource_available ============================================

bool player_t::resource_available( int    resource,
                                   double cost ) SC_CONST
{
  if ( resource == RESOURCE_NONE || cost == 0 )
  {
    return true;
  }

  return resource_current[ resource ] >= cost;
}

// player_t::primary_tab ===================================================
talent_tab_name player_t::primary_tab() SC_CONST
{
  if ( level > 10 && level <= 69 )
  {
    if ( talent_tab_points[ 0 ] > 0 ) return ( talent_tab_name ) 0;
    if ( talent_tab_points[ 1 ] > 0 ) return ( talent_tab_name ) 1;
    if ( talent_tab_points[ 2 ] > 0 ) return ( talent_tab_name ) 2;
    return TALENT_TAB_NONE;
  }
  else
  {
    if ( talent_tab_points[ 0 ] >= talent_tab_points[ 1 ] ) 
    {
      if ( talent_tab_points[ 0 ] >= talent_tab_points[ 2 ] )
      {
        if ( ! talent_tab_points[ 0 ] )
          return TALENT_TAB_NONE;

        return ( talent_tab_name ) 0;
      }
      else
      {
        return ( talent_tab_name ) 2;
      }
    }
    else if ( talent_tab_points[ 2 ] >= talent_tab_points[ 1 ] )
    {
      return ( talent_tab_name ) 2;
    }
    else
    {
      return ( talent_tab_name ) 1;
    }
  }
}

// player_t::primary_tree ===================================================
talent_tree_type player_t::primary_tree() SC_CONST
{
  if ( pri_tree == TALENT_TAB_NONE )
    return TREE_NONE;

  return tree_type[ pri_tree ];
}

// player_t::normalize_by ===================================================

int player_t::normalize_by() SC_CONST
{
  if ( sim -> normalized_stat != STAT_NONE ) 
  {
    return sim -> normalized_stat; 
  }

  return ( primary_role() == ROLE_SPELL ) ? STAT_INTELLECT : STAT_ATTACK_POWER;
}


// player_t::stat_gain ======================================================

void player_t::stat_gain( int    stat,
                          double amount )
{
  if ( sim -> log ) log_t::output( sim, "%s gains %.0f %s", name(), amount, util_t::stat_type_string( stat ) );

  switch ( stat )
  {
  case STAT_STRENGTH:  stats.attribute[ ATTR_STRENGTH  ] += amount; attribute[ ATTR_STRENGTH  ] += amount; break;
  case STAT_AGILITY:   stats.attribute[ ATTR_AGILITY   ] += amount; attribute[ ATTR_AGILITY   ] += amount; break;
  case STAT_STAMINA:   stats.attribute[ ATTR_STAMINA   ] += amount; attribute[ ATTR_STAMINA   ] += amount; break;
  case STAT_INTELLECT: stats.attribute[ ATTR_INTELLECT ] += amount; attribute[ ATTR_INTELLECT ] += amount; break;
  case STAT_SPIRIT:    stats.attribute[ ATTR_SPIRIT    ] += amount; attribute[ ATTR_SPIRIT    ] += amount; break;

  case STAT_MAX: for( int i=0; i < ATTRIBUTE_MAX; i++ ) { stats.attribute[ i ] += amount; attribute[ i ] += amount; } break;

  case STAT_HEALTH: resource_gain( RESOURCE_HEALTH, amount ); break;
  case STAT_MANA:   resource_gain( RESOURCE_MANA,   amount ); break;
  case STAT_RAGE:   resource_gain( RESOURCE_RAGE,   amount ); break;
  case STAT_ENERGY: resource_gain( RESOURCE_ENERGY, amount ); break;
  case STAT_FOCUS:  resource_gain( RESOURCE_FOCUS,  amount ); break;
  case STAT_RUNIC:  resource_gain( RESOURCE_RUNIC,  amount ); break;

  case STAT_SPELL_POWER:       stats.spell_power       += amount; spell_power[ SCHOOL_MAX ] += amount; break;
  case STAT_SPELL_PENETRATION: stats.spell_penetration += amount; spell_penetration         += amount; break;
  case STAT_MP5:               stats.mp5               += amount; mp5                       += amount; break;

  case STAT_ATTACK_POWER:             stats.attack_power             += amount; attack_power       += amount;                            break;
  case STAT_EXPERTISE_RATING:         stats.expertise_rating         += amount; attack_expertise   += amount / rating.expertise;         break;

  case STAT_HIT_RATING:
    stats.hit_rating += amount;
    attack_hit       += amount / rating.attack_hit;
    spell_hit        += amount / rating.spell_hit;
    break;

  case STAT_CRIT_RATING:
    stats.crit_rating += amount;
    attack_crit       += amount / rating.attack_crit;
    spell_crit        += amount / rating.spell_crit;
    break;

  case STAT_HASTE_RATING:
    stats.haste_rating += amount;
    haste_rating       += amount;
    recalculate_haste();
    break;

  case STAT_ARMOR:          stats.armor          += amount; armor       += amount;                  break;
  case STAT_BONUS_ARMOR:    stats.bonus_armor    += amount; bonus_armor += amount;                  break;
  case STAT_DODGE_RATING:   stats.dodge_rating   += amount; dodge       += amount / rating.dodge;   break;
  case STAT_PARRY_RATING:   stats.parry_rating   += amount; parry       += amount / rating.parry;   break;

  case STAT_BLOCK_RATING: stats.block_rating += amount; block       += amount / rating.block; break;
  case STAT_BLOCK_VALUE:  stats.block_value  += amount; block_value += amount;                break;

  case STAT_MASTERY_RATING:
    stats.mastery_rating += amount;
    mastery += amount / rating.mastery;
    break;

  default: assert( 0 );
  }
}

// player_t::stat_loss ======================================================

void player_t::stat_loss( int    stat,
                          double amount )
{
  if ( sim -> log ) log_t::output( sim, "%s loses %.0f %s", name(), amount, util_t::stat_type_string( stat ) );

  switch ( stat )
  {
  case STAT_STRENGTH:  stats.attribute[ ATTR_STRENGTH  ] -= amount; attribute[ ATTR_STRENGTH  ] -= amount; break;
  case STAT_AGILITY:   stats.attribute[ ATTR_AGILITY   ] -= amount; attribute[ ATTR_AGILITY   ] -= amount; break;
  case STAT_STAMINA:   stats.attribute[ ATTR_STAMINA   ] -= amount; attribute[ ATTR_STAMINA   ] -= amount; break;
  case STAT_INTELLECT: stats.attribute[ ATTR_INTELLECT ] -= amount; attribute[ ATTR_INTELLECT ] -= amount; break;
  case STAT_SPIRIT:    stats.attribute[ ATTR_SPIRIT    ] -= amount; attribute[ ATTR_SPIRIT    ] -= amount; break;

  case STAT_MAX: for( int i=0; i < ATTRIBUTE_MAX; i++ ) { stats.attribute[ i ] -= amount; attribute[ i ] -= amount; } break;

  case STAT_HEALTH: resource_gain( RESOURCE_HEALTH, amount ); break;
  case STAT_MANA:   resource_gain( RESOURCE_MANA,   amount ); break;
  case STAT_RAGE:   resource_gain( RESOURCE_RAGE,   amount ); break;
  case STAT_ENERGY: resource_gain( RESOURCE_ENERGY, amount ); break;
  case STAT_FOCUS:  resource_gain( RESOURCE_FOCUS,  amount ); break;
  case STAT_RUNIC:  resource_gain( RESOURCE_RUNIC,  amount ); break;

  case STAT_SPELL_POWER:       stats.spell_power       -= amount; spell_power[ SCHOOL_MAX ] -= amount; break;
  case STAT_SPELL_PENETRATION: stats.spell_penetration -= amount; spell_penetration         -= amount; break;
  case STAT_MP5:               stats.mp5               -= amount; mp5                       -= amount; break;

  case STAT_ATTACK_POWER:             stats.attack_power             -= amount; attack_power       -= amount;                            break;
  case STAT_EXPERTISE_RATING:         stats.expertise_rating         -= amount; attack_expertise   -= amount / rating.expertise;         break;

  case STAT_HIT_RATING:
    stats.hit_rating -= amount;
    attack_hit       -= amount / rating.attack_hit;
    spell_hit        -= amount / rating.spell_hit;
    break;

  case STAT_CRIT_RATING:
    stats.crit_rating -= amount;
    attack_crit       -= amount / rating.attack_crit;
    spell_crit        -= amount / rating.spell_crit;
    break;

  case STAT_HASTE_RATING:
    stats.haste_rating -= amount;
    haste_rating       -= amount;
    recalculate_haste();
    break;

  case STAT_ARMOR:          stats.armor          -= amount; armor       -= amount;                  break;
  case STAT_BONUS_ARMOR:    stats.bonus_armor    -= amount; bonus_armor -= amount;                  break;
  case STAT_DODGE_RATING:   stats.dodge_rating   -= amount; dodge       -= amount / rating.dodge;   break;
  case STAT_PARRY_RATING:   stats.parry_rating   -= amount; parry       -= amount / rating.parry;   break;

  case STAT_BLOCK_RATING: stats.block_rating -= amount; block       -= amount / rating.block; break;
  case STAT_BLOCK_VALUE:  stats.block_value  -= amount; block_value -= amount;                break;

  case STAT_MASTERY_RATING:
    stats.mastery_rating -= amount;
    mastery -= amount / rating.mastery;
    break;

  default: assert( 0 );
  }
}

// player_t::summon_pet =====================================================

void player_t::summon_pet( const char* pet_name,
                           double      duration )
{
  for ( pet_t* p = pet_list; p; p = p -> next_pet )
  {
    if ( p -> name_str == pet_name )
    {
      p -> summon( duration );
      return;
    }
  }
  sim -> errorf( "Player %s is unable to summon pet '%s'\n", name(), pet_name );
}

// player_t::dismiss_pet ====================================================

void player_t::dismiss_pet( const char* pet_name )
{
  for ( pet_t* p = pet_list; p; p = p -> next_pet )
  {
    if ( p -> name_str == pet_name )
    {
      p -> dismiss();
      return;
    }
  }
  assert( 0 );
}

// player_t::register_callbacks =============================================

void player_t::register_callbacks()
{
  dark_intent_cb = new dark_intent_callback_t( this );
  dark_intent_cb -> active = false;
  register_spell_result_callback ( RESULT_CRIT_MASK, dark_intent_cb );
}

// player_t::register_resource_gain_callback ================================

void player_t::register_resource_gain_callback( int             resource,
                                                action_callback_t* cb )
{
  resource_gain_callbacks[ resource ].push_back( cb );
}

// player_t::register_resource_loss_callback ================================

void player_t::register_resource_loss_callback( int             resource,
                                                action_callback_t* cb )
{
  resource_loss_callbacks[ resource ].push_back( cb );
}

// player_t::register_attack_result_callback ================================

void player_t::register_attack_result_callback( int64_t             mask,
                                                action_callback_t* cb )
{
  for ( int64_t i=0; i < RESULT_MAX; i++ )
  {
    if ( mask < 0 || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      attack_result_callbacks[ i ].push_back( cb );
    }
  }
}

// player_t::register_spell_result_callback =================================

void player_t::register_spell_result_callback( int64_t             mask,
                                               action_callback_t* cb )
{
  for ( int64_t i=0; i < RESULT_MAX; i++ )
  {
    if ( mask < 0 || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      spell_result_callbacks[ i ].push_back( cb );
    }
  }
}

// player_t::register_attack_direct_result_callback ================================

void player_t::register_attack_direct_result_callback( int64_t             mask,
                                                       action_callback_t* cb )
{
  for ( int64_t i=0; i < RESULT_MAX; i++ )
  {
    if ( mask < 0 || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      attack_direct_result_callbacks[ i ].push_back( cb );
    }
  }
}

// player_t::register_spell_direct_result_callback =================================

void player_t::register_spell_direct_result_callback( int64_t             mask,
                                                      action_callback_t* cb )
{
  for ( int64_t i=0; i < RESULT_MAX; i++ )
  {
    if ( mask < 0 || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      spell_direct_result_callbacks[ i ].push_back( cb );
    }
  }
}

// player_t::register_tick_callback =========================================

void player_t::register_tick_callback( action_callback_t* cb )
{
  tick_callbacks.push_back( cb );
}

// player_t::register_tick_damage_callback ==================================

void player_t::register_tick_damage_callback( int64_t             mask,
                                              action_callback_t* cb )
{
  for ( int64_t i=0; i < SCHOOL_MAX; i++ )
  {
    if ( mask < 0 || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      tick_damage_callbacks[ i ].push_back( cb );
    }
  }
}

// player_t::register_direct_damage_callback ================================

void player_t::register_direct_damage_callback( int64_t             mask,
                                                action_callback_t* cb )
{
  for ( int64_t i=0; i < SCHOOL_MAX; i++ )
  {
    if ( mask < 0 || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      direct_damage_callbacks[ i ].push_back( cb );
    }
  }
}

// player_t::register_spell_cast_result_callback =================================

void player_t::register_spell_cast_result_callback( int64_t             mask,
                                                    action_callback_t* cb )
{
  for ( int64_t i=0; i < RESULT_MAX; i++ )
  {
    if ( mask < 0 || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      spell_cast_result_callbacks[ i ].push_back( cb );
    }
  }
}

// player_t::register_harmful_cast_result_callback =================================

void player_t::register_harmful_cast_result_callback( int64_t             mask,
                                                      action_callback_t* cb )
{
  for ( int64_t i=0; i < RESULT_MAX; i++ )
  {
    if ( mask < 0 || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      harmful_cast_result_callbacks[ i ].push_back( cb );
    }
  }
}

// player_t::recalculate_haste ==============================================

void player_t::recalculate_haste()
{
  spell_haste = 1.0 / ( 1.0 + haste_rating / rating. spell_haste );
  attack_haste = 1.0 / ( 1.0 + haste_rating / rating.attack_haste );
}

// player_t::recent_cast ====================================================

bool player_t::recent_cast()
{
  return ( last_cast > 0 ) && ( ( last_cast + 5.0 ) > sim -> current_time );
}

// player_t::find_action ====================================================

action_t* player_t::find_action( const std::string& str )
{
  for ( action_t* a = action_list; a; a = a -> next )
    if ( str == a -> name_str )
      return a;

  return 0;
}

// player_t::aura_gain ======================================================

void player_t::aura_gain( const char* aura_name , int aura_id )
{
  if ( sim -> log && ! sleeping )
  {
    log_t::output( sim, "%s gains %s", name(), aura_name );
    log_t::aura_gain_event( this, aura_name, aura_id );
  }

}

// player_t::aura_loss ======================================================

void player_t::aura_loss( const char* aura_name , int aura_id )
{
  if ( sim -> log && ! sleeping )
  {
    log_t::output( sim, "%s loses %s", name(), aura_name );
    log_t::aura_loss_event( this, aura_name, aura_id );
  }
}

// player_t::find_cooldown ==================================================

cooldown_t* player_t::find_cooldown( const std::string& name )
{
  for ( cooldown_t* c = cooldown_list; c; c = c -> next )
  {
    if ( c -> name_str == name )
      return c;
  }

  return 0;
}

// player_t::find_dot =======================================================

dot_t* player_t::find_dot( const std::string& name )
{
  for ( dot_t* d = dot_list; d; d = d -> next )
  {
    if ( d -> name_str == name )
      return d;
  }

  return 0;
}

// player_t::get_cooldown ===================================================

cooldown_t* player_t::get_cooldown( const std::string& name )
{
  cooldown_t* c=0;

  for ( c = cooldown_list; c; c = c -> next )
  {
    if ( c -> name_str == name )
      return c;
  }

  c = new cooldown_t( name, this );

  cooldown_t** tail = &cooldown_list;

  while ( *tail && name > ( ( *tail ) -> name_str ) )
  {
    tail = &( ( *tail ) -> next );
  }

  c -> next = *tail;
  *tail = c;

  return c;
}

// player_t::get_dot ========================================================

dot_t* player_t::get_dot( const std::string& name )
{
  dot_t* d=0;

  for ( d = dot_list; d; d = d -> next )
  {
    if ( d -> name_str == name )
      return d;
  }

  d = new dot_t( name, this );

  dot_t** tail = &dot_list;

  while ( *tail && name > ( ( *tail ) -> name_str ) )
  {
    tail = &( ( *tail ) -> next );
  }

  d -> next = *tail;
  *tail = d;

  return d;
}

// player_t::get_gain =======================================================

gain_t* player_t::get_gain( const std::string& name )
{
  gain_t* g=0;

  for ( g = gain_list; g; g = g -> next )
  {
    if ( g -> name_str == name )
      return g;
  }

  g = new gain_t( name );

  gain_t** tail = &gain_list;

  while ( *tail && name > ( ( *tail ) -> name_str ) )
  {
    tail = &( ( *tail ) -> next );
  }

  g -> next = *tail;
  *tail = g;

  return g;
}

// player_t::get_proc =======================================================

proc_t* player_t::get_proc( const std::string& name )
{
  proc_t* p=0;

  for ( p = proc_list; p; p = p -> next )
  {
    if ( p -> name_str == name )
      return p;
  }

  p = new proc_t( sim, name );

  proc_t** tail = &proc_list;

  while ( *tail && name > ( ( *tail ) -> name_str ) )
  {
    tail = &( ( *tail ) -> next );
  }

  p -> next = *tail;
  *tail = p;

  return p;
}

// player_t::get_stats ======================================================

stats_t* player_t::get_stats( const std::string& n )
{
  stats_t* stats=0;

  for ( stats = stats_list; stats; stats = stats -> next )
  {
    if ( stats -> name_str == n )
      return stats;
  }

  if ( ! stats )
  {
    stats = new stats_t( n, this );
    stats -> init();
    stats_t** tail= &stats_list;
    while ( *tail && n > ( ( *tail ) -> name_str ) )
    {
      tail = &( ( *tail ) -> next );
    }
    stats -> next = *tail;
    *tail = stats;
  }

  return stats;
}

// player_t::get_uptime =====================================================

uptime_t* player_t::get_uptime( const std::string& name )
{
  uptime_t* u=0;

  for ( u = uptime_list; u; u = u -> next )
  {
    if ( u -> name_str == name )
      return u;
  }

  u = new uptime_t( name );

  uptime_t** tail = &uptime_list;

  while ( *tail && name > ( ( *tail ) -> name_str ) )
  {
    tail = &( ( *tail ) -> next );
  }

  u -> next = *tail;
  *tail = u;

  return u;
}

// player_t::get_rng =======================================================

rng_t* player_t::get_rng( const std::string& n, int type )
{
  assert( sim -> rng );

  if ( ! sim -> smooth_rng || type == RNG_GLOBAL ) return sim -> rng;

  if ( type == RNG_DETERMINISTIC ) return sim -> deterministic_rng;

  rng_t* rng=0;

  for ( rng = rng_list; rng; rng = rng -> next )
  {
    if ( rng -> name_str == n )
      return rng;
  }

  if ( ! rng )
  {
    rng = rng_t::create( sim, n, type );
    rng -> next = rng_list;
    rng_list = rng;
  }

  return rng;
}

// player_t::target_swing ==================================================

int player_t::target_swing()
{
  double roll = sim -> rng -> real();
  double chance = 0;

  chance += ( composite_tank_miss( SCHOOL_PHYSICAL ) );
  if ( roll <= chance ) return RESULT_MISS;

  chance += ( composite_tank_dodge() - diminished_dodge() );
  if ( roll <= chance ) return RESULT_DODGE;

  chance += ( composite_tank_parry() - diminished_parry() );
  if ( roll <= chance ) return RESULT_PARRY;

  chance += composite_tank_block();
  if ( roll <= chance ) return RESULT_BLOCK;

  chance += composite_tank_crit( SCHOOL_PHYSICAL );
  if ( roll <= chance ) return RESULT_CRIT;

  return RESULT_HIT;
}

// Chosen Movement Actions

struct start_moving_t : public action_t
{
  start_moving_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "start_moving", player )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };

    parse_options( options, options_str );

    trigger_gcd = 0;

    cooldown -> duration = 0.5;
    harmful = false;
  }

  virtual void execute()
  {   
    player -> buffs.self_movement -> trigger();
    player -> buffs.moving -> increment();

    if ( sim -> log ) log_t::output( sim, "%s starts moving.", player -> name() );
    update_ready();
  }

  virtual bool ready()
  {
    if ( player -> buffs.self_movement -> check() )
      return false;

    return action_t::ready();
  }
};

struct stop_moving_t : public action_t
{
  stop_moving_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "stop_moving", player )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };

    parse_options( options, options_str );

    trigger_gcd = 0;

    cooldown -> duration = 0.5; 
    harmful = false;
  }

  virtual void execute()
  {    
    player -> buffs.self_movement -> expire();
    player -> buffs.moving -> decrement();

    if ( sim -> log ) log_t::output( sim, "%s stops moving.", player -> name() );
    update_ready();
  }

  virtual bool ready()
  {
    if ( ! player -> buffs.self_movement -> check() )
      return false;

    return action_t::ready();
  }
};

// ===== Racial Abilities ==================================================

// Arcane Torrent ==========================================================

struct arcane_torrent_t : public action_t
{
  arcane_torrent_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "arcane_torrent", player )
  {
    trigger_gcd = 0;
    cooldown -> duration = 120;
  }

  virtual void execute()
  {
    int resource = player -> primary_resource();
    double gain = 0;
    switch( resource )
    {
      case RESOURCE_MANA:
        gain = player -> resource_max [ RESOURCE_MANA ] * 0.06;
        break;
      case RESOURCE_ENERGY:
      case RESOURCE_RUNIC:
        gain = 15;
        break;
      default:
        break;
    }

    if( gain > 0 )
      player -> resource_gain( resource, gain, player -> gains.arcane_torrent );

    update_ready();
  }

  virtual bool ready()
  {
    if( ! action_t::ready() )
      return false;

    if ( player -> race != RACE_BLOOD_ELF )
      return false;

    int resource = player -> primary_resource();
    switch( resource )
    {
      case RESOURCE_MANA:
        if( player -> resource_current [ resource ] / player -> resource_max [ resource ] <= 0.94 )
          return true;
        break;
      case RESOURCE_ENERGY:
      case RESOURCE_RUNIC:
        if( player -> resource_max [ resource ] - player -> resource_current [ resource ] <= 15 )
          return true;
        break;
      default:
        break;
    }

    return false;
  }
};

// Berserking ==========================================================

struct berserking_t : public action_t
{
  berserking_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "berserking", player )
  {
    trigger_gcd = 0;
    cooldown -> duration = 180;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    update_ready();

    player -> buffs.berserking -> trigger();
  }

  virtual bool ready()
  {
    if( ! action_t::ready() )
      return false;

    if ( player -> race != RACE_TROLL )
      return false;

    return true;
  }
};

// Blood Fury ==========================================================

struct blood_fury_t : public action_t
{
  blood_fury_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "blood_fury", player )
  {
    trigger_gcd = 0;
    cooldown -> duration = 120;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    update_ready();

    if ( player -> type == WARRIOR || player -> type == ROGUE || player -> type == DEATH_KNIGHT ||
         player -> type == HUNTER  || player -> type == SHAMAN )
    {
      player -> buffs.blood_fury_ap -> trigger();
    }

    if ( player -> type == SHAMAN  || player -> type == WARLOCK )
    {
      player -> buffs.blood_fury_sp -> trigger();
    }
  }

  virtual bool ready()
  {
    if( ! action_t::ready() )
      return false;

    if ( player -> race != RACE_ORC )
      return false;

    return true;
  }
};

// Stoneform ==========================================================

struct stoneform_t : public action_t
{
  stoneform_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "stoneform", player )
  {
    trigger_gcd = 0;
    cooldown -> duration = 120;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    update_ready();

    player -> buffs.stoneform -> trigger();
  }

  virtual bool ready()
  {
    if( ! action_t::ready() )
      return false;

    if ( player -> race != RACE_DWARF )
      return false;

    return true;
  }
};

// Cycle Action ============================================================

struct cycle_t : public action_t
{
  action_t* current_action;

  cycle_t( player_t* player, const std::string& options_str ) :
      action_t( ACTION_OTHER, "cycle", player ), current_action( 0 )
  {}

  virtual void reset()
  {
    action_t::reset();

    if ( ! current_action )
    {
      current_action = next;
      if ( ! current_action )
      {
        sim -> errorf( "Player %s has no actions after 'cycle'\n", player -> name() );
        sim -> cancel();
      }
      for ( action_t* a = next; a; a = a -> next ) a -> background = true;
    }
  }

  virtual void schedule_execute()
  {
    player -> last_foreground_action = current_action;
    current_action -> schedule_execute();
    current_action = current_action -> next;
    if ( ! current_action ) current_action = next;
  }

  virtual bool ready()
  {
    if ( ! current_action ) return false;

    return current_action -> ready();
  }
};

// Restart Sequence Action =================================================

struct restart_sequence_t : public action_t
{
  std::string seq_name_str;

  restart_sequence_t( player_t* player, const std::string& options_str ) :
      action_t( ACTION_OTHER, "restart_sequence", player )
  {
    seq_name_str = "default"; // matches default name for sequences
    option_t options[] =
    {
      { "name", OPT_STRING, &seq_name_str },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = 0;
  }

  virtual void execute()
  {
    for ( action_t* a = player -> action_list; a; a = a -> next )
    {
      if ( a -> type != ACTION_SEQUENCE )
        continue;

      if ( ! seq_name_str.empty() )
        if ( seq_name_str != a -> name_str )
          continue;

      ( ( sequence_t* ) a ) -> restart();
    }
  }
};

// Restore Mana Action =====================================================

struct restore_mana_t : public action_t
{
  double mana;

  restore_mana_t( player_t* player, const std::string& options_str ) :
      action_t( ACTION_OTHER, "restore_mana", player ), mana( 0 )
  {
    option_t options[] =
    {
      { "mana", OPT_FLT, &mana },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = 0;
  }

  virtual void execute()
  {
    double mana_missing = player -> resource_max[ RESOURCE_MANA ] - player -> resource_current[ RESOURCE_MANA ];
    double mana_gain = mana;

    if ( mana_gain == 0 || mana_gain > mana_missing ) mana_gain = mana_missing;

    if ( mana_gain > 0 )
    {
      player -> resource_gain( RESOURCE_MANA, mana_gain, player -> gains.restore_mana );
    }
  }
};

// Snapshot Stats ============================================================

struct snapshot_stats_t : public action_t
{
  attack_t* attack;
  spell_t* spell;

  snapshot_stats_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "snapshot_stats", player ), attack(0), spell(0)
  {
    base_execute_time = 0.001; // Needs to be non-zero to ensure all the buffs have been setup.
    trigger_gcd = 0;
  }

  virtual void execute()
  {
    player_t* p = player;

    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );

    p -> buffed_spell_haste  = p -> composite_spell_haste();
    p -> buffed_attack_haste = p -> composite_attack_haste();
    p -> buffed_mastery      = p -> composite_mastery();

    p -> attribute_buffed[ ATTR_STRENGTH  ] = floor( p -> strength()  );
    p -> attribute_buffed[ ATTR_AGILITY   ] = floor( p -> agility()   );
    p -> attribute_buffed[ ATTR_STAMINA   ] = floor( p -> stamina()   );
    p -> attribute_buffed[ ATTR_INTELLECT ] = floor( p -> intellect() );
    p -> attribute_buffed[ ATTR_SPIRIT    ] = floor( p -> spirit()    );

    for( int i=0; i < RESOURCE_MAX; i++ ) p -> resource_buffed[ i ] = p -> resource_max[ i ];

    p -> buffed_spell_power       = floor( p -> composite_spell_power( SCHOOL_MAX ) * p -> composite_spell_power_multiplier() );
    p -> buffed_spell_hit         = p -> composite_spell_hit();
    p -> buffed_spell_crit        = p -> composite_spell_crit();
    p -> buffed_spell_penetration = p -> composite_spell_penetration();
    p -> buffed_mp5               = p -> composite_mp5();

    p -> buffed_attack_power       = p -> composite_attack_power() * p -> composite_attack_power_multiplier();
    p -> buffed_attack_hit         = p -> composite_attack_hit();
    p -> buffed_attack_expertise   = p -> composite_attack_expertise();
    p -> buffed_attack_crit        = p -> composite_attack_crit();

    p -> buffed_armor       = p -> composite_armor();
    p -> buffed_block_value = p -> composite_block_value();
    p -> buffed_miss        = p -> composite_tank_miss( SCHOOL_PHYSICAL );
    p -> buffed_dodge       = p -> composite_tank_dodge() - p -> diminished_dodge();
    p -> buffed_parry       = p -> composite_tank_parry() - p -> diminished_parry();
    p -> buffed_block       = p -> composite_tank_block();
    p -> buffed_crit        = p -> composite_tank_crit( SCHOOL_PHYSICAL );

    int role = p -> primary_role();
    int delta_level = sim -> target -> level - p -> level;
    double spell_hit_extra=0, attack_hit_extra=0, expertise_extra=0;

    if ( role == ROLE_SPELL || role == ROLE_HYBRID )
    {
      if ( ! spell ) spell = new spell_t( "snapshot_spell", p );
      spell -> background = true;
      spell -> player_buff();
      spell -> target_debuff( DMG_DIRECT );
      double chance = spell -> miss_chance( delta_level );
      if ( chance < 0 ) spell_hit_extra = -chance * p -> rating.spell_hit;
    }

    if ( role == ROLE_ATTACK || role == ROLE_HYBRID )
    {
      if ( ! attack ) attack = new attack_t( "snapshot_attack", p );
      attack -> background = true;
      attack -> player_buff();
      attack -> target_debuff( DMG_DIRECT );
      double chance = attack -> miss_chance( delta_level );
      if( p -> dual_wield() ) chance += 0.19;
      if ( chance < 0 ) attack_hit_extra = -chance * p -> rating.attack_hit;
      chance = attack -> dodge_chance(  p -> level, sim -> target -> level );
      if ( chance < 0 ) expertise_extra = -chance * 4 * p -> rating.expertise;
    }

    p -> over_cap[ STAT_HIT_RATING ] = std::max( spell_hit_extra, attack_hit_extra );
    p -> over_cap[ STAT_EXPERTISE_RATING ] = expertise_extra;

    p -> attribute_buffed[ ATTRIBUTE_NONE ] = 1;
  }

  virtual bool ready()
  {
    if ( sim -> current_iteration > 0 ) return false;
    return player -> attribute_buffed[ ATTRIBUTE_NONE ] == 0;
  }
};

// Wait Until Ready Action ===================================================

struct wait_until_ready_t : public action_t
{
  double sec;

  wait_until_ready_t( player_t* player, const std::string& options_str ) :
      action_t( ACTION_OTHER, "wait", player ), sec( 1.0 )
  {
    option_t options[] =
    {
      { "sec", OPT_FLT, &sec },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = 0;
  }

  virtual double execute_time() SC_CONST
  {
    double wait = sec;
    double remains = 0;

    for ( action_t* a = player -> action_list; a; a = a -> next )
    {
      if ( a -> background ) continue;

      remains = a -> cooldown -> remains();
      if ( remains > 0 && remains < wait ) wait = remains;

      remains = a -> dot -> remains();
      if ( remains > 0 && remains < wait ) wait = remains;
    }

    return wait;
  }

  virtual void execute()
  {
    player -> total_waiting += time_to_execute;
  }
};

// Wait Fixed Action =========================================================

struct wait_fixed_t : public action_t
{
  action_expr_t* time_expr;

  wait_fixed_t( player_t* player, const std::string& options_str ) :
      action_t( ACTION_OTHER, "wait", player )
  {
    std::string sec_str = "1.0";

    option_t options[] =
    {
      { "sec", OPT_STRING, &sec_str },
      { NULL,  OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    time_expr = action_expr_t::parse( this, sec_str );
    trigger_gcd = 0;
  }

  virtual double execute_time() SC_CONST
  {
    int result = time_expr -> evaluate();
    assert( result == TOK_NUM );
    return time_expr -> result_num;
  }

  virtual void execute()
  {
    player -> total_waiting += time_to_execute;
  }
};

// Use Item Action ===========================================================

struct use_item_t : public action_t
{
  item_t* item;
  spell_t* discharge;
  action_callback_t* trigger;
  stat_buff_t* buff;
  std::string use_name;

  use_item_t( player_t* player, const std::string& options_str ) :
      action_t( ACTION_OTHER, "use_item", player ),
      item(0), discharge(0), trigger(0), buff(0)
  {
    std::string item_name;
    option_t options[] =
    {
      { "name", OPT_STRING, &item_name },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    if ( item_name.empty() )
    {
      sim -> errorf( "Player %s has 'use_item' action with no 'name=' option.\n", player -> name() );
      return;
    }

    item = player -> find_item( item_name );
    if ( ! item )
    {
      sim -> errorf( "Player %s attempting 'use_item' action with item '%s' which is not currently equipped.\n", player -> name(), item_name.c_str() );
      return;
    }
    if ( ! item -> use.active() )
    {
      sim -> errorf( "Player %s attempting 'use_item' action with item '%s' which has no 'use=' encoding.\n", player -> name(), item_name.c_str() );
      item = 0;
      return;
    }

    item_t::special_effect_t& e = item -> use;

    use_name = e.name_str.empty() ? item_name : e.name_str;

    if ( e.trigger_type )
    {
      if ( e.stat )
      {
        trigger = unique_gear_t::register_stat_proc( e.trigger_type, e.trigger_mask, use_name, player,
                                                     e.stat, e.max_stacks, e.amount, e.proc_chance, 0.0/*dur*/, 0.0/*cd*/, e.tick, e.reverse, 0 );
      }
      else if ( e.school )
      {
        trigger = unique_gear_t::register_discharge_proc( e.trigger_type, e.trigger_mask, use_name, player,
                                                          e.max_stacks, e.school, e.amount, e.amount, e.proc_chance, 0.0/*cd*/ );
      }

      if ( trigger ) trigger -> deactivate();
    }
    else if ( e.school )
    {
      struct discharge_spell_t : public spell_t
      {
        discharge_spell_t( const char* n, player_t* p, double a, const school_type s ) :
            spell_t( n, p, RESOURCE_NONE, s )
        {
          trigger_gcd = 0;
          base_dd_min = a;
          base_dd_max = a;
          may_crit    = true;
          background  = true;
          base_spell_power_multiplier = 0;
          reset();
        }
      };

      discharge = new discharge_spell_t( use_name.c_str(), player, e.amount, e.school );
    }
    else if ( e.stat )
    {
      if( e.max_stacks  == 0 ) e.max_stacks  = 1;
      if( e.proc_chance == 0 ) e.proc_chance = 1;

      buff = new stat_buff_t( player, use_name, e.stat, e.amount, e.max_stacks, e.duration, 0, e.proc_chance, false, e.reverse );
    }
    else assert( false );

    std::string cooldown_name = use_name;
    cooldown_name += "_";
    cooldown_name += item -> slot_name();

    cooldown = player -> get_cooldown( cooldown_name );
    cooldown -> duration = item -> use.cooldown;
    trigger_gcd = 0;
  }

  void lockout( double duration )
  {
    if( duration <= 0 ) return;
    double ready = sim -> current_time + duration;
    for( action_t* a = player -> action_list; a; a = a -> next )
    {
      if( a -> name_str == "use_item" )
      {
        if( ready > a -> cooldown -> ready )
        {
          a -> cooldown -> ready = ready;
        }
      }
    }
  }

  virtual void execute()
  {
    if ( discharge )
    {
      discharge -> execute();
    }
    else if ( trigger )
    {
      if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), use_name.c_str() );

      trigger -> activate();

      if ( item -> use.duration )
      {
        struct trigger_expiration_t : public event_t
        {
          item_t* item;
          action_callback_t* trigger;

          trigger_expiration_t( sim_t* sim, player_t* player, item_t* i, action_callback_t* t ) : event_t( sim, player ), item( i ), trigger( t )
          {
            name = item -> name();
            sim -> add_event( this, item -> use.duration );
          }
          virtual void execute()
          {
            trigger -> deactivate();
          }
        };

        new ( sim ) trigger_expiration_t( sim, player, item, trigger );

        lockout( item -> use.duration );
      }
    }
    else if( buff )
    {
      if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), use_name.c_str() );
      buff -> trigger();
      lockout( buff -> buff_duration );
    }
    else assert( false );

    update_ready();
  }

  virtual void reset()
  {
    action_t::reset();
    if ( trigger ) trigger -> deactivate();
  }

  virtual bool ready()
  {
    if( ! item ) return false;
    return action_t::ready();
  }
};

// player_t::create_action ==================================================

action_t* player_t::create_action( const std::string& name,
                                   const std::string& options_str )
{
  if ( name == "arcane_torrent"   ) return new   arcane_torrent_t( this, options_str );
  if ( name == "berserking"       ) return new       berserking_t( this, options_str );
  if ( name == "blood_fury"       ) return new       blood_fury_t( this, options_str );
  if ( name == "cycle"            ) return new            cycle_t( this, options_str );
  if ( name == "restart_sequence" ) return new restart_sequence_t( this, options_str );
  if ( name == "restore_mana"     ) return new     restore_mana_t( this, options_str );
  if ( name == "sequence"         ) return new         sequence_t( this, options_str );
  if ( name == "snapshot_stats"   ) return new   snapshot_stats_t( this, options_str );
  if ( name == "start_moving"     ) return new     start_moving_t( this, options_str );
  if ( name == "stoneform"        ) return new        stoneform_t( this, options_str );
  if ( name == "stop_moving"      ) return new      stop_moving_t( this, options_str );
  if ( name == "use_item"         ) return new         use_item_t( this, options_str );
  if ( name == "wait"             ) return new       wait_fixed_t( this, options_str );
  if ( name == "wait_until_ready" ) return new wait_until_ready_t( this, options_str );

  return consumable_t::create_action( this, name, options_str );
}

// player_t::find_pet =======================================================

pet_t* player_t::find_pet( const std::string& pet_name )
{
  for ( pet_t* p = pet_list; p; p = p -> next_pet )
    if ( p -> name_str == pet_name )
      return p;

  return 0;
}

// player_t::trigger_replenishment ================================================

void player_t::trigger_replenishment()
{
  if ( sim -> overrides.replenishment )
    return;

  if ( sim -> replenishment_targets > 0 )
  {
    choose_replenish_targets( this );
  }
  else
  {
    replenish_raid( this );
  }
}

// player_t::get_talent_list ======================================================

std::vector<talent_translation_t>& player_t::get_talent_list()
{
        return talent_list;
}


// player_t::parse_talent_trees ===================================================

bool player_t::parse_talent_trees( int talents[], const uint32_t size )
{
  std::vector<talent_translation_t> translations = this->get_talent_list();

  for( unsigned int i = 0; i < translations.size() ; i++ )
  {
    int *address = translations[ i ].address;
    if ( ! address ) continue;
    *address = talents[i];
  }

  uint32_t i = 0;
  while ( i < size )
  {
    talent_str.push_back( talents[ i ] );
    i++;
  }

  return true;
}

// player_t::parse_talents_armory ===========================================

bool player_t::parse_talents_armory( const std::string& talent_string )
{
  int talents[MAX_TALENT_SLOTS] = {0};

  const char *buffer = talent_string.c_str();

  for(unsigned int i = 0; i < talent_string.size(); i++)
  {
    char c = buffer[ i ];
    if ( c < '0' || c > '5' )
    {
      sim -> errorf( "Player %s has illegal character '%c' in talent encoding.\n", name(), c );
      return false;
    }
    talents[i] = c - '0';
  }

  return parse_talent_trees( talents, talent_string.size() );
}

// player_t::parse_talents_mmo ==============================================

bool player_t::parse_talents_mmo( const std::string& talent_string )
{
  std::string player_class, talent_string_extract;
  size_t cut_pos1 = talent_string.find_first_of("#");
  size_t cut_pos2 = talent_string.find_first_of(",");

  if((cut_pos1 < cut_pos2) && (cut_pos2 < talent_string.npos))
  {
        player_class = talent_string.substr(0, cut_pos1);
        talent_string_extract = talent_string.substr(cut_pos1+1, cut_pos2-cut_pos1);
        return mmo_champion_t::parse_talents( this, talent_string_extract );
  }
  else if((cut_pos1 = talent_string.find_first_of("=")) != talent_string.npos)
  {
          return parse_talents_armory( talent_string.substr( cut_pos1 + 1 ) );
  }

  return false;
}

// player_t::parse_talents_wowhead ==========================================

bool player_t::parse_talents_wowhead( const std::string& talent_string )
{
  // wowhead format: [tree_1]Z[tree_2]Z[tree_3] where the trees are character encodings
  // each character expands to a pair of numbers [0-5][0-5]
  // unused deeper talents are simply left blank instead of filling up the string with zero-zero encodings

  struct decode_t
  {
    char key, first, second;
  }
  decoding[] =
  {
    { '0', '0', '0' },  { 'z', '0', '1' },  { 'M', '0', '2' },  { 'c', '0', '3' }, { 'm', '0', '4' },  { 'V', '0', '5' },
    { 'o', '1', '0' },  { 'k', '1', '1' },  { 'R', '1', '2' },  { 's', '1', '3' }, { 'a', '1', '4' },  { 'q', '1', '5' },
    { 'b', '2', '0' },  { 'd', '2', '1' },  { 'r', '2', '2' },  { 'f', '2', '3' }, { 'w', '2', '4' },  { 'i', '2', '5' },
    { 'h', '3', '0' },  { 'u', '3', '1' },  { 'G', '3', '2' },  { 'I', '3', '3' }, { 'N', '3', '4' },  { 'A', '3', '5' },
    { 'L', '4', '0' },  { 'p', '4', '1' },  { 'T', '4', '2' },  { 'j', '4', '3' }, { 'n', '4', '4' },  { 'y', '4', '5' },
    { 'x', '5', '0' },  { 't', '5', '1' },  { 'g', '5', '2' },  { 'e', '5', '3' }, { 'v', '5', '4' },  { 'E', '5', '5' },
    { '\0', '\0', '\0' }
  };

  this->get_talent_list();
  int talents[ MAX_TALENT_SLOTS ] = {0};
  unsigned int tree_size[ MAX_TALENT_TREES ] = {0};
  unsigned int tree_count[ MAX_TALENT_TREES ] = {0};
  unsigned int number_talents = 0;


  for (unsigned int i=0; i < MAX_TALENT_TREES; i++ )
  {
    tree_size[ i ] = player_data.talent_player_get_num_talents( type, i );
    number_talents += tree_size[i];
  }

  unsigned int tree = 0;
  unsigned int count = 0;

  for ( unsigned int i=1; i < talent_string.length(); i++ )
  {
    if ( (tree >= MAX_TALENT_TREES) )
    {
      sim -> errorf( "Player %s has malformed wowhead talent string. Too many talent trees specified.\n", name() );
      return false;
    }
    if ( (count >  number_talents) )
    {
      sim -> errorf( "Player %s has malformed wowhead talent string. Too many talents specified.\n", name() );
      return false;
    }

    char c = talent_string[ i ];

    if ( c == ':' ) break; // glyph encoding follows

    if ( c == 'Z' )
    {
                count = 0;
                for(unsigned int j=0; j <= tree; j++)
                        count += tree_size[j];
                tree++;
      continue;
    }

    decode_t* decode = 0;
    for ( int j=0; decoding[ j ].key != '\0' && ! decode; j++ )
    {
      if ( decoding[ j ].key == c ) decode = decoding + j;
    }

    if ( ! decode )
    {
      sim -> errorf( "Player %s has malformed wowhead talent string. Translation for '%c' unknown.\n", name(), c );
      return false;
    }

    talents[ count++ ] += decode -> first - '0';
    tree_count[ tree ] += 1;

    if ( tree_count[ tree ] < tree_size[ tree ] )
    {
      talents[ count++ ] += decode -> second - '0';
      tree_count[ tree ] += 1;
    }

    if ( tree_count[ tree ] >= tree_size[ tree ] )
    {
      tree++;
    }
  }

  if ( sim -> debug )
  {
    std::string str_out = "";
    for(unsigned int i=0; i < talent_list.size(); i++)
      str_out += (char)talents[i];
    util_t::fprintf( sim -> output_file, "%s Wowhead talent string translation: %s\n", name(), str_out.c_str() );
  }

  return parse_talent_trees( talents, number_talents );
}

// player_t::create_expression ==============================================

action_expr_t* player_t::create_expression( action_t* a,
                                            const std::string& name_str )
{
  int resource_type = util_t::parse_resource_type( name_str );
  if ( resource_type != RESOURCE_NONE )
  {
    struct resource_expr_t : public action_expr_t
    {
      int resource_type;
      resource_expr_t( action_t* a, const std::string& n, int r ) : action_expr_t( a, n, TOK_NUM ), resource_type(r) {}
      virtual int evaluate() { result_num = action -> player -> resource_current[ resource_type ]; return TOK_NUM; }
    };
    return new resource_expr_t( a, name_str, resource_type );
  }
  if ( name_str == "mana_pct" )
  {
    struct mana_pct_expr_t : public action_expr_t
    {
      mana_pct_expr_t( action_t* a ) : action_expr_t( a, "mana_pct", TOK_NUM ) {}
      virtual int evaluate() { player_t* p = action -> player; result_num = 100 * ( p -> resource_current[ RESOURCE_MANA ] / p -> resource_max[ RESOURCE_MANA ] ); return TOK_NUM; }
    };
    return new mana_pct_expr_t( a );
  }
  if ( name_str == "in_combat" )
  {
    struct in_combat_expr_t : public action_expr_t
    {
      in_combat_expr_t( action_t* a ) : action_expr_t( a, "in_combat", TOK_NUM ) {}
      virtual int evaluate() { result_num = ( action -> player -> in_combat  ? 1 : 0 ); return TOK_NUM; }
    };
    return new in_combat_expr_t( a );
  }
  if ( name_str == "attack_haste" )
  {
    struct attack_haste_expr_t : public action_expr_t
    {
      attack_haste_expr_t( action_t* a ) : action_expr_t( a, "attack_haste", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> player -> composite_attack_haste(); return TOK_NUM; }
    };
    return new attack_haste_expr_t( a );
  }
  if ( name_str == "spell_haste" )
  {
    struct spell_haste_expr_t : public action_expr_t
    {
      spell_haste_expr_t( action_t* a ) : action_expr_t( a, "spell_haste", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> player -> composite_spell_haste(); return TOK_NUM; }
    };
    return new spell_haste_expr_t( a );
  }

  std::vector<std::string> splits;
  int num_splits = util_t::string_split( splits, name_str, "." );
  if ( num_splits == 3 )
  {
    if ( splits[ 0 ] == "buff" )
    {
      buff_t* buff = buff_t::find( this, splits[ 1 ] );
      if ( ! buff ) buff = buff_t::find( sim, splits[ 1 ] );
      if ( ! buff ) return 0;
      return buff -> create_expression( a, splits[ 2 ] );
    }
    else if ( splits[ 0 ] == "cooldown" )
    {
      cooldown_t* cooldown = get_cooldown( splits[ 1 ] );
      if ( splits[ 2 ] == "remains" )
      {
        struct cooldown_remains_expr_t : public action_expr_t
        {
          cooldown_t* cooldown;
          cooldown_remains_expr_t( action_t* a, cooldown_t* c ) : action_expr_t( a, "cooldown_remains", TOK_NUM ), cooldown(c) {}
          virtual int evaluate() { result_num = cooldown -> remains(); return TOK_NUM; }
        };
        return new cooldown_remains_expr_t( a, cooldown );
      }
    }
    else if ( splits[ 0 ] == "dot" )
    {
      dot_t* dot = get_dot( splits[ 1 ] );
      if ( splits[ 2 ] == "remains" )
      {
        struct dot_remains_expr_t : public action_expr_t
        {
          dot_t* dot;
          dot_remains_expr_t( action_t* a, dot_t* d ) : action_expr_t( a, "dot_remains", TOK_NUM ), dot(d) {}
          virtual int evaluate() { result_num = dot -> remains(); return TOK_NUM; }
        };
        return new dot_remains_expr_t( a, dot );
      }
    }
    else if ( splits[ 0 ] == "swing" )
    {
      std::string& s = splits[ 1 ];
      int hand = SLOT_NONE;
      if ( s == "mh" || s == "mainhand" || s == "main_hand" ) hand = SLOT_MAIN_HAND;
      if ( s == "oh" || s ==  "offhand" || s ==  "off_hand" ) hand = SLOT_OFF_HAND;
      if ( hand == SLOT_NONE ) return 0;
      if ( splits[ 2 ] == "remains" )
      {
        struct swing_remains_expr_t : public action_expr_t
        {
	  int slot;
          swing_remains_expr_t( action_t* a, int s ) : action_expr_t( a, "swing_remains", TOK_NUM ), slot(s) {}
          virtual int evaluate()
	  {
	    result_num = 9999;
	    player_t* p = action -> player;
	    attack_t* attack = ( slot == SLOT_MAIN_HAND ) ? p -> main_hand_attack : p -> off_hand_attack;
	    if ( attack && attack -> execute_event ) result_num = attack -> execute_event -> occurs() - action -> sim -> current_time;
	    return TOK_NUM;
	  }
        };
        return new swing_remains_expr_t( a, hand );
      }
    }
  }

  return sim -> create_expression( a, name_str );
}

// player_t::create_profile =================================================

bool player_t::create_profile( std::string& profile_str, int save_type )
{
  if ( save_type == SAVE_ALL )
  {
    profile_str += "#!./simc \n\n";
  }

  if ( ! comment_str.empty() )
  {
    profile_str += "# " + comment_str + "\n";
  }

  if ( save_type == SAVE_ALL )
  {
    profile_str += util_t::player_type_string( type ); profile_str += "=" + name_str + "\n";
    profile_str += "origin=\"" + origin_str + "\"\n";
    profile_str += "level=" + util_t::to_string( level ) + "\n";
    profile_str += "race=" + race_str + "\n";
    profile_str += "use_pre_potion=" + util_t::to_string( use_pre_potion ) + "\n";

    if ( professions_str.size() > 0 )
    {
      profile_str += "professions=" + professions_str + "\n";
    };
  }

  if ( save_type == SAVE_ALL || save_type == SAVE_TALENTS )
  {
    if ( talents_str.size() > 0 )
    {
      profile_str += "talents=" + talents_str + "\n";
    };
    if ( glyphs_str.size() > 0 )
    {
      profile_str += "glyphs=" + glyphs_str + "\n";
    }
  }

  if ( save_type == SAVE_ALL || save_type == SAVE_ACTIONS )
  {
    if ( action_list_str.size() > 0 )
    {
      std::vector<std::string> splits;
      int num_splits = util_t::string_split( splits, action_list_str, "/" );
      for ( int i=0; i < num_splits; i++ )
      {
        profile_str += "actions";
        profile_str += i ? "+=/" : "=";
        profile_str += splits[ i ] + "\n";
      }
    }
  }

  if ( save_type == SAVE_ALL || save_type == SAVE_GEAR )
  {
    for ( int i=0; i < SLOT_MAX; i++ )
    {
      item_t& item = items[ i ];

      if ( item.active() )
      {
        profile_str += item.slot_name();
        profile_str += "=" + item.options_str + "\n";
      }
    }
    if ( ! items_str.empty() )
    {
      profile_str += "items=" + items_str + "\n";
    }

    profile_str += "# Gear Summary\n";
    for ( int i=0; i < STAT_MAX; i++ )
    {
      double value = initial_stats.get_stat( i );
      if ( value != 0 )
      {
        profile_str += "# gear_";
        profile_str += util_t::stat_type_string( i );
        profile_str += "=" + util_t::to_string( value, 0 ) + "\n";
      }
    }
    if ( meta_gem != META_GEM_NONE )
    {
      profile_str += "# meta_gem=";
      profile_str += util_t::meta_gem_type_string( meta_gem );
      profile_str += "\n";
    }

    if ( set_bonus.tier7_2pc_caster() ) profile_str += "# tier7_2pc_caster=1\n";
    if ( set_bonus.tier7_4pc_caster() ) profile_str += "# tier7_4pc_caster=1\n";
    if ( set_bonus.tier7_2pc_melee()  ) profile_str += "# tier7_2pc_melee=1\n";
    if ( set_bonus.tier7_4pc_melee()  ) profile_str += "# tier7_4pc_melee=1\n";
    if ( set_bonus.tier7_2pc_tank()   ) profile_str += "# tier7_2pc_tank=1\n";
    if ( set_bonus.tier7_4pc_tank()   ) profile_str += "# tier7_4pc_tank=1\n";

    if ( set_bonus.tier8_2pc_caster() ) profile_str += "# tier8_2pc_caster=1\n";
    if ( set_bonus.tier8_4pc_caster() ) profile_str += "# tier8_4pc_caster=1\n";
    if ( set_bonus.tier8_2pc_melee()  ) profile_str += "# tier8_2pc_melee=1\n";
    if ( set_bonus.tier8_4pc_melee()  ) profile_str += "# tier8_4pc_melee=1\n";
    if ( set_bonus.tier8_2pc_tank()   ) profile_str += "# tier8_2pc_tank=1\n";
    if ( set_bonus.tier8_4pc_tank()   ) profile_str += "# tier8_4pc_tank=1\n";

    if ( set_bonus.tier9_2pc_caster() ) profile_str += "# tier9_2pc_caster=1\n";
    if ( set_bonus.tier9_4pc_caster() ) profile_str += "# tier9_4pc_caster=1\n";
    if ( set_bonus.tier9_2pc_melee()  ) profile_str += "# tier9_2pc_melee=1\n";
    if ( set_bonus.tier9_4pc_melee()  ) profile_str += "# tier9_4pc_melee=1\n";
    if ( set_bonus.tier9_2pc_tank()   ) profile_str += "# tier9_2pc_tank=1\n";
    if ( set_bonus.tier9_4pc_tank()   ) profile_str += "# tier9_4pc_tank=1\n";

    if ( set_bonus.tier10_2pc_caster() ) profile_str += "# tier10_2pc_caster=1\n";
    if ( set_bonus.tier10_4pc_caster() ) profile_str += "# tier10_4pc_caster=1\n";
    if ( set_bonus.tier10_2pc_melee()  ) profile_str += "# tier10_2pc_melee=1\n";
    if ( set_bonus.tier10_4pc_melee()  ) profile_str += "# tier10_4pc_melee=1\n";
    if ( set_bonus.tier10_2pc_tank()   ) profile_str += "# tier10_2pc_tank=1\n";
    if ( set_bonus.tier10_4pc_tank()   ) profile_str += "# tier10_4pc_tank=1\n";

    if ( set_bonus.tier11_2pc_caster() ) profile_str += "# tier11_2pc_caster=1\n";
    if ( set_bonus.tier11_4pc_caster() ) profile_str += "# tier11_4pc_caster=1\n";
    if ( set_bonus.tier11_2pc_melee()  ) profile_str += "# tier11_2pc_melee=1\n";
    if ( set_bonus.tier11_4pc_melee()  ) profile_str += "# tier11_4pc_melee=1\n";
    if ( set_bonus.tier11_2pc_tank()   ) profile_str += "# tier11_2pc_tank=1\n";
    if ( set_bonus.tier11_4pc_tank()   ) profile_str += "# tier11_4pc_tank=1\n";

    for ( int i=0; i < SLOT_MAX; i++ )
    {
      item_t& item = items[ i ];
      if ( ! item.active() ) continue;
      if ( item.unique || item.unique_enchant || ! item.encoded_weapon_str.empty() )
      {
        profile_str += "# ";
        profile_str += item.slot_name();
        profile_str += "=";
        profile_str += item.name();
        if ( item.heroic() ) profile_str += ",heroic=1";
        if ( ! item.encoded_weapon_str.empty() ) profile_str += ",weapon=" + item.encoded_weapon_str;
        if ( item.unique_enchant ) profile_str += ",enchant=" + item.encoded_enchant_str;
        profile_str += "\n";
      }
    }
  }

  return true;
}

// player_t::get_options ====================================================

std::vector<option_t>& player_t::get_options()
{
  if ( options.empty() )
  {
    option_t player_options[] =
    {
      // @option_doc loc=player/all/general title="General"
      { "name",                                 OPT_STRING,   &( name_str                                     ) },
      { "origin",                               OPT_STRING,   &( origin_str                                   ) },
      { "id",                                   OPT_STRING,   &( id_str                                       ) },
      { "talents",                              OPT_FUNC,     ( void* ) ::parse_talent_url                      },
      { "glyphs",                               OPT_STRING,   &( glyphs_str                                   ) },
      { "race",                                 OPT_STRING,   &( race_str                                     ) },
      { "level",                                OPT_INT,      &( level                                        ) },
      { "use_pre_potion",                       OPT_INT,      &( use_pre_potion                               ) },
      { "tank",                                 OPT_INT,      &( tank                                         ) },
      { "skill",                                OPT_FLT,      &( initial_skill                                ) },
      { "distance",                             OPT_FLT,      &( distance                                     ) },
      { "professions",                          OPT_STRING,   &( professions_str                              ) },
      { "actions",                              OPT_STRING,   &( action_list_str                              ) },
      { "actions+",                             OPT_APPEND,   &( action_list_str                              ) },
      { "sleeping",                             OPT_BOOL,     &( sleeping                                     ) },
      { "quiet",                                OPT_BOOL,     &( quiet                                        ) },
      { "save",                                 OPT_STRING,   &( save_str                                     ) },
      { "save_gear",                            OPT_STRING,   &( save_gear_str                                ) },
      { "save_talents",                         OPT_STRING,   &( save_talents_str                             ) },
      { "save_actions",                         OPT_STRING,   &( save_actions_str                             ) },
      { "comment",                              OPT_STRING,   &( comment_str                                  ) },
      { "bugs",                                 OPT_BOOL,     &( bugs                                         ) },
      // @option_doc loc=player/all/items title="Items"
      { "meta_gem",                             OPT_STRING,   &( meta_gem_str                                 ) },
      { "items",                                OPT_STRING,   &( items_str                                    ) },
      { "items+",                               OPT_APPEND,   &( items_str                                    ) },
      { "head",                                 OPT_STRING,   &( items[ SLOT_HEAD      ].options_str          ) },
      { "neck",                                 OPT_STRING,   &( items[ SLOT_NECK      ].options_str          ) },
      { "shoulders",                            OPT_STRING,   &( items[ SLOT_SHOULDERS ].options_str          ) },
      { "shirt",                                OPT_STRING,   &( items[ SLOT_SHIRT     ].options_str          ) },
      { "chest",                                OPT_STRING,   &( items[ SLOT_CHEST     ].options_str          ) },
      { "waist",                                OPT_STRING,   &( items[ SLOT_WAIST     ].options_str          ) },
      { "legs",                                 OPT_STRING,   &( items[ SLOT_LEGS      ].options_str          ) },
      { "feet",                                 OPT_STRING,   &( items[ SLOT_FEET      ].options_str          ) },
      { "wrists",                               OPT_STRING,   &( items[ SLOT_WRISTS    ].options_str          ) },
      { "hands",                                OPT_STRING,   &( items[ SLOT_HANDS     ].options_str          ) },
      { "finger1",                              OPT_STRING,   &( items[ SLOT_FINGER_1  ].options_str          ) },
      { "finger2",                              OPT_STRING,   &( items[ SLOT_FINGER_2  ].options_str          ) },
      { "trinket1",                             OPT_STRING,   &( items[ SLOT_TRINKET_1 ].options_str          ) },
      { "trinket2",                             OPT_STRING,   &( items[ SLOT_TRINKET_2 ].options_str          ) },
      { "back",                                 OPT_STRING,   &( items[ SLOT_BACK      ].options_str          ) },
      { "main_hand",                            OPT_STRING,   &( items[ SLOT_MAIN_HAND ].options_str          ) },
      { "off_hand",                             OPT_STRING,   &( items[ SLOT_OFF_HAND  ].options_str          ) },
      { "ranged",                               OPT_STRING,   &( items[ SLOT_RANGED    ].options_str          ) },
      { "tabard",                               OPT_STRING,   &( items[ SLOT_TABARD    ].options_str          ) },
      // @option_doc loc=player/all/items title="Items"
      { "tier6_2pc_caster",                     OPT_BOOL,     &( set_bonus.count[ SET_T6_2PC_CASTER ]         ) },
      { "tier6_4pc_caster",                     OPT_BOOL,     &( set_bonus.count[ SET_T6_4PC_CASTER ]         ) },
      { "tier6_2pc_melee",                      OPT_BOOL,     &( set_bonus.count[ SET_T6_2PC_MELEE ]          ) },
      { "tier6_4pc_melee",                      OPT_BOOL,     &( set_bonus.count[ SET_T6_4PC_MELEE ]          ) },
      { "tier6_2pc_tank",                       OPT_BOOL,     &( set_bonus.count[ SET_T6_2PC_TANK ]           ) },
      { "tier6_4pc_tank",                       OPT_BOOL,     &( set_bonus.count[ SET_T6_4PC_TANK ]           ) },
      { "tier7_2pc_caster",                     OPT_BOOL,     &( set_bonus.count[ SET_T7_2PC_CASTER ]         ) },
      { "tier7_4pc_caster",                     OPT_BOOL,     &( set_bonus.count[ SET_T7_4PC_CASTER ]         ) },
      { "tier7_2pc_melee",                      OPT_BOOL,     &( set_bonus.count[ SET_T7_2PC_MELEE ]          ) },
      { "tier7_4pc_melee",                      OPT_BOOL,     &( set_bonus.count[ SET_T7_4PC_MELEE ]          ) },
      { "tier7_2pc_tank",                       OPT_BOOL,     &( set_bonus.count[ SET_T7_2PC_TANK ]           ) },
      { "tier7_4pc_tank",                       OPT_BOOL,     &( set_bonus.count[ SET_T7_4PC_TANK ]           ) },
      { "tier8_2pc_caster",                     OPT_BOOL,     &( set_bonus.count[ SET_T8_2PC_CASTER ]         ) },
      { "tier8_4pc_caster",                     OPT_BOOL,     &( set_bonus.count[ SET_T8_4PC_CASTER ]         ) },
      { "tier8_2pc_melee",                      OPT_BOOL,     &( set_bonus.count[ SET_T8_2PC_MELEE ]          ) },
      { "tier8_4pc_melee",                      OPT_BOOL,     &( set_bonus.count[ SET_T8_4PC_MELEE ]          ) },
      { "tier8_2pc_tank",                       OPT_BOOL,     &( set_bonus.count[ SET_T8_2PC_TANK ]           ) },
      { "tier8_4pc_tank",                       OPT_BOOL,     &( set_bonus.count[ SET_T8_4PC_TANK ]           ) },
      { "tier9_2pc_caster",                     OPT_BOOL,     &( set_bonus.count[ SET_T9_2PC_CASTER ]         ) },
      { "tier9_4pc_caster",                     OPT_BOOL,     &( set_bonus.count[ SET_T9_4PC_CASTER ]         ) },
      { "tier9_2pc_melee",                      OPT_BOOL,     &( set_bonus.count[ SET_T9_2PC_MELEE ]          ) },
      { "tier9_4pc_melee",                      OPT_BOOL,     &( set_bonus.count[ SET_T9_4PC_MELEE ]          ) },
      { "tier9_2pc_tank",                       OPT_BOOL,     &( set_bonus.count[ SET_T9_2PC_TANK ]           ) },
      { "tier9_4pc_tank",                       OPT_BOOL,     &( set_bonus.count[ SET_T9_4PC_TANK ]           ) },
      { "tier10_2pc_caster",                    OPT_BOOL,     &( set_bonus.count[ SET_T10_2PC_CASTER ]        ) },
      { "tier10_4pc_caster",                    OPT_BOOL,     &( set_bonus.count[ SET_T10_4PC_CASTER ]        ) },
      { "tier10_2pc_melee",                     OPT_BOOL,     &( set_bonus.count[ SET_T10_2PC_MELEE ]         ) },
      { "tier10_4pc_melee",                     OPT_BOOL,     &( set_bonus.count[ SET_T10_4PC_MELEE ]         ) },
      { "tier10_2pc_tank",                      OPT_BOOL,     &( set_bonus.count[ SET_T10_2PC_TANK ]          ) },
      { "tier10_4pc_tank",                      OPT_BOOL,     &( set_bonus.count[ SET_T10_4PC_TANK ]          ) },
      { "tier11_2pc_caster",                    OPT_BOOL,     &( set_bonus.count[ SET_T11_2PC_CASTER ]        ) },
      { "tier11_4pc_caster",                    OPT_BOOL,     &( set_bonus.count[ SET_T11_4PC_CASTER ]        ) },
      { "tier11_2pc_melee",                     OPT_BOOL,     &( set_bonus.count[ SET_T11_2PC_MELEE ]         ) },
      { "tier11_4pc_melee",                     OPT_BOOL,     &( set_bonus.count[ SET_T11_4PC_MELEE ]         ) },
      { "tier11_2pc_tank",                      OPT_BOOL,     &( set_bonus.count[ SET_T11_2PC_TANK ]          ) },
      { "tier11_4pc_tank",                      OPT_BOOL,     &( set_bonus.count[ SET_T11_4PC_TANK ]          ) },
      // @option_doc loc=skip
      { "shoulder",                             OPT_STRING,   &( items[ SLOT_SHOULDERS ].options_str          ) },
      { "leg",                                  OPT_STRING,   &( items[ SLOT_LEGS      ].options_str          ) },
      { "foot",                                 OPT_STRING,   &( items[ SLOT_FEET      ].options_str          ) },
      { "wrist",                                OPT_STRING,   &( items[ SLOT_WRISTS    ].options_str          ) },
      { "hand",                                 OPT_STRING,   &( items[ SLOT_HANDS     ].options_str          ) },
      { "ring1",                                OPT_STRING,   &( items[ SLOT_FINGER_1  ].options_str          ) },
      { "ring2",                                OPT_STRING,   &( items[ SLOT_FINGER_2  ].options_str          ) },
      // @option_doc loc=player/all/gear title="Gear Stats"
      { "gear_strength",                        OPT_FLT,  &( gear.attribute[ ATTR_STRENGTH  ]                 ) },
      { "gear_agility",                         OPT_FLT,  &( gear.attribute[ ATTR_AGILITY   ]                 ) },
      { "gear_stamina",                         OPT_FLT,  &( gear.attribute[ ATTR_STAMINA   ]                 ) },
      { "gear_intellect",                       OPT_FLT,  &( gear.attribute[ ATTR_INTELLECT ]                 ) },
      { "gear_spirit",                          OPT_FLT,  &( gear.attribute[ ATTR_SPIRIT    ]                 ) },
      { "gear_spell_power",                     OPT_FLT,  &( gear.spell_power                                 ) },
      { "gear_mp5",                             OPT_FLT,  &( gear.mp5                                         ) },
      { "gear_attack_power",                    OPT_FLT,  &( gear.attack_power                                ) },
      { "gear_expertise_rating",                OPT_FLT,  &( gear.expertise_rating                            ) },
      { "gear_haste_rating",                    OPT_FLT,  &( gear.haste_rating                                ) },
      { "gear_hit_rating",                      OPT_FLT,  &( gear.hit_rating                                  ) },
      { "gear_crit_rating",                     OPT_FLT,  &( gear.crit_rating                                 ) },
      { "gear_health",                          OPT_FLT,  &( gear.resource[ RESOURCE_HEALTH ]                 ) },
      { "gear_mana",                            OPT_FLT,  &( gear.resource[ RESOURCE_MANA   ]                 ) },
      { "gear_rage",                            OPT_FLT,  &( gear.resource[ RESOURCE_RAGE   ]                 ) },
      { "gear_energy",                          OPT_FLT,  &( gear.resource[ RESOURCE_ENERGY ]                 ) },
      { "gear_focus",                           OPT_FLT,  &( gear.resource[ RESOURCE_FOCUS  ]                 ) },
      { "gear_runic",                           OPT_FLT,  &( gear.resource[ RESOURCE_RUNIC  ]                 ) },
      { "gear_armor",                           OPT_FLT,  &( gear.armor                                       ) },
      { "gear_block_value",                     OPT_FLT,  &( gear.block_value                                 ) },
      { "gear_mastery_rating",                  OPT_FLT,  &( gear.mastery_rating                              ) },
      // @option_doc loc=player/all/enchant/stats title="Stat Enchants"
      { "enchant_strength",                     OPT_FLT,  &( enchant.attribute[ ATTR_STRENGTH  ]              ) },
      { "enchant_agility",                      OPT_FLT,  &( enchant.attribute[ ATTR_AGILITY   ]              ) },
      { "enchant_stamina",                      OPT_FLT,  &( enchant.attribute[ ATTR_STAMINA   ]              ) },
      { "enchant_intellect",                    OPT_FLT,  &( enchant.attribute[ ATTR_INTELLECT ]              ) },
      { "enchant_spirit",                       OPT_FLT,  &( enchant.attribute[ ATTR_SPIRIT    ]              ) },
      { "enchant_spell_power",                  OPT_FLT,  &( enchant.spell_power                              ) },
      { "enchant_mp5",                          OPT_FLT,  &( enchant.mp5                                      ) },
      { "enchant_attack_power",                 OPT_FLT,  &( enchant.attack_power                             ) },
      { "enchant_expertise_rating",             OPT_FLT,  &( enchant.expertise_rating                         ) },
      { "enchant_armor",                        OPT_FLT,  &( enchant.armor                                    ) },
      { "enchant_block_value",                  OPT_FLT,  &( enchant.block_value                              ) },
      { "enchant_haste_rating",                 OPT_FLT,  &( enchant.haste_rating                             ) },
      { "enchant_hit_rating",                   OPT_FLT,  &( enchant.hit_rating                               ) },
      { "enchant_crit_rating",                  OPT_FLT,  &( enchant.crit_rating                              ) },
      { "enchant_mastery_rating",               OPT_FLT,  &( enchant.mastery_rating                           ) },
      { "enchant_health",                       OPT_FLT,  &( enchant.resource[ RESOURCE_HEALTH ]              ) },
      { "enchant_mana",                         OPT_FLT,  &( enchant.resource[ RESOURCE_MANA   ]              ) },
      { "enchant_rage",                         OPT_FLT,  &( enchant.resource[ RESOURCE_RAGE   ]              ) },
      { "enchant_energy",                       OPT_FLT,  &( enchant.resource[ RESOURCE_ENERGY ]              ) },
      { "enchant_focus",                        OPT_FLT,  &( enchant.resource[ RESOURCE_FOCUS  ]              ) },
      { "enchant_runic",                        OPT_FLT,  &( enchant.resource[ RESOURCE_RUNIC  ]              ) },
      // @option_doc loc=skip
      { "skip_actions",                         OPT_STRING, &( action_list_skip                               ) },
      { "elixirs",                              OPT_STRING, &( elixirs_str                                    ) },
      { "flask",                                OPT_STRING, &( flask_str                                      ) },
      { "food",                                 OPT_STRING, &( food_str                                       ) },
      { NULL, OPT_UNKNOWN, NULL }
    };

    option_t::copy( options, player_options );
  }

  return options;
}

// player_t::create =========================================================

player_t* player_t::create( sim_t*             sim,
                            const std::string& type,
                            const std::string& name,
                            race_type r )
{
  if ( type == "death_knight" )
  {
    return player_t::create_death_knight( sim, name, r );
  }
  else if ( type == "druid" )
  {
    return player_t::create_druid( sim, name, r );
  }
  else if ( type == "hunter" )
  {
    return player_t::create_hunter( sim, name, r );
  }
  else if ( type == "mage" )
  {
    return player_t::create_mage( sim, name, r );
  }
  else if ( type == "priest" )
  {
    return player_t::create_priest( sim, name, r );
  }
  else if ( type == "paladin" )
  {
    return player_t::create_paladin( sim, name, r );
  }
  else if ( type == "rogue" )
  {
    return player_t::create_rogue( sim, name, r );
  }
  else if ( type == "shaman" )
  {
    return player_t::create_shaman( sim, name, r );
  }
  else if ( type == "warlock" )
  {
    return player_t::create_warlock( sim, name, r );
  }
  else if ( type == "warrior" )
  {
    return player_t::create_warrior( sim, name, r );
  }
  else if ( type == "pet" )
  {
    return sim -> active_player -> create_pet( name );
  }
  return 0;
}

