#include "simulationcraft.hpp"
#include "sc_warlock.hpp"
#include "sc_warlock_pets.hpp"

namespace warlock
{
// Spells
  namespace actions
  {
    struct drain_life_t : public warlock_spell_t
    {

      drain_life_t( warlock_t* p, const std::string& options_str ) :
        warlock_spell_t( p, "Drain Life" )
      {
        parse_options( options_str );
        channeled = true;
        hasted_ticks = false;
        may_crit = false;
      }

      void execute() override
      {

        if (p()->azerite.inevitable_demise.ok() && p()->buffs.inevitable_demise->check() > 0)
        {
          if (p()->buffs.drain_life->check())
            p()->buffs.inevitable_demise->expire();
        }

        warlock_spell_t::execute();

        p()->buffs.drain_life->trigger();
      }

      double bonus_ta(const action_state_t* s) const override
      {
        double ta = warlock_spell_t::bonus_ta(s);

        ta += p()->buffs.inevitable_demise->check_stack_value();

        return ta;
      }

      void last_tick( dot_t* d ) override
      {
        p()->buffs.drain_life->expire();
        p()->buffs.inevitable_demise->expire();

        warlock_spell_t::last_tick( d );
      }
    };

    //TOCHECK: Does the damage proc affect Seed of Corruption? If so, this needs to be split into specs as well
    struct grimoire_of_sacrifice_t : public warlock_spell_t
    {
      grimoire_of_sacrifice_t( warlock_t* p, const std::string& options_str ) :
        warlock_spell_t( "grimoire_of_sacrifice", p, p -> talents.grimoire_of_sacrifice )
      {
        parse_options( options_str );
        harmful = false;
        ignore_false_positive = true;
      }

      bool ready() override
      {
        if ( !p()->warlock_pet_list.active ) return false;

        return warlock_spell_t::ready();
      }

      void execute() override
      {
        warlock_spell_t::execute();

        if ( p()->warlock_pet_list.active )
        {
          p()->warlock_pet_list.active->dismiss();
          p()->warlock_pet_list.active = nullptr;
          p()->buffs.grimoire_of_sacrifice->trigger();
        }
      }
    };

    struct grimoire_of_sacrifice_damage_t : public warlock_spell_t
    {
      grimoire_of_sacrifice_damage_t( warlock_t* p ) :
        warlock_spell_t( "grimoire_of_sacrifice", p, p -> find_spell( 196100 ) )
      {
        background = true;
        proc = true;
      }
    };
  } // end actions namespace

  warlock_td_t::warlock_td_t( player_t* target, warlock_t& p ) :
    actor_target_data_t( target, &p ),
    soc_threshold( 0.0 ),
    warlock( p )
  {
    dots_drain_life = target->get_dot( "drain_life", &p );

    //Aff
    dots_corruption = target->get_dot("corruption", &p);
    dots_agony = target->get_dot("agony", &p);
    for (unsigned i = 0; i < dots_unstable_affliction.size(); ++i)
    {
      dots_unstable_affliction[i] = target->get_dot("unstable_affliction_" + std::to_string(i + 1), &p);
    }
    dots_drain_soul = target->get_dot("drain_soul", &p);
    dots_phantom_singularity = target->get_dot("phantom_singularity", &p);
    dots_siphon_life = target->get_dot("siphon_life", &p);
    dots_seed_of_corruption = target->get_dot("seed_of_corruption", &p);
    dots_vile_taint = target->get_dot("vile_taint", &p);

    debuffs_haunt = make_buff( *this, "haunt", source->find_spell( 48181 ) )->set_refresh_behavior( buff_refresh_behavior::PANDEMIC );
    debuffs_shadow_embrace = make_buff( *this, "shadow_embrace", source->find_spell( 32390 ) )
      ->set_refresh_behavior( buff_refresh_behavior::DURATION )
      ->set_max_stack( 3 );

    //Destro
    dots_immolate = target->get_dot("immolate", &p);
    dots_roaring_blaze = target->get_dot("roaring_blaze", &p);
    dots_channel_demonfire = target->get_dot("channel_demonfire", &p);

    debuffs_eradication = make_buff( *this, "eradication", source->find_spell( 196414 ) )
      ->set_refresh_behavior( buff_refresh_behavior::DURATION );
    debuffs_roaring_blaze = make_buff( *this, "roaring_blaze", source->find_spell( 205690 ) )
      ->set_max_stack( 100 );
    debuffs_shadowburn = make_buff(*this, "shadowburn", source->find_spell(17877));
    debuffs_havoc = make_buff( *this, "havoc", source->find_spell( 80240 ) )
      ->set_stack_change_callback( [ &p ] ( buff_t* b, int, int cur )
      {
        if ( cur == 0 )
          p.havoc_target = nullptr;
        else
          p.havoc_target = b->player;

        range::for_each( p.havoc_spells, [] ( action_t* a ) { a->target_cache.is_valid = false; } );
      } );
    debuffs_chaotic_flames = make_buff(*this, "chaotic_flames", source->find_spell(253092));

    //Demo
    dots_doom = target->get_dot("doom", &p);
    dots_umbral_blaze = target->get_dot("umbral_blaze", &p);

    debuffs_jaws_of_shadow = make_buff( *this, "jaws_of_shadow", source->find_spell( 242922 ) );
    debuffs_from_the_shadows = make_buff(*this, "from_the_shadows", source->find_spell(270569));

    target->callbacks_on_demise.push_back( [this]( player_t* ) { target_demise(); } );
  }

  void warlock_td_t::target_demise()
  {
    if ( !( target->is_enemy() ) )
    {
      return;
    }

    if ( warlock.specialization() == WARLOCK_AFFLICTION )
    {
      for ( auto& current_ua : dots_unstable_affliction )
      {
        if ( current_ua->is_ticking() )
        {
          warlock.sim->print_log( "Player {} demised. Warlock {} gains a shard from unstable affliction.", target->name(), warlock.name() );

          warlock.resource_gain( RESOURCE_SOUL_SHARD, 1, warlock.gains.unstable_affliction_refund );

          // you can only get one soul shard per death from UA refunds
          break;
        }
      }

      if ( dots_drain_soul->is_ticking() )
      {
        warlock.sim->print_log( "Player {} demised. Warlock {} gains a shard from drain soul.", target->name(), warlock.name() );

        warlock.resource_gain( RESOURCE_SOUL_SHARD, 1, warlock.gains.drain_soul );
      }
    }

    if ( debuffs_haunt->check() )
    {
      warlock.sim->print_log( "Player {} demised. Warlock {} reset haunt's cooldown.", target->name(), warlock.name() );

      warlock.cooldowns.haunt->reset( true );
    }

    if ( debuffs_shadowburn->check() )
    {
      warlock.sim->print_log( "Player {} demised. Warlock {} reset shadowburn's cooldown.", target->name(), warlock.name() );

      warlock.cooldowns.shadowburn->reset( true );
    }
  }

  static void accumulate_seed_of_corruption(warlock_td_t* td, double amount)
  {
    td->soc_threshold -= amount;

    if (td->soc_threshold <= 0)
    {
      td->dots_seed_of_corruption->cancel();
    }
    else
    {
      if( td->source->sim->log )
        td->source->sim->out_log.printf("remaining damage to explode seed %f", td->soc_threshold);
    }
  }

warlock_t::warlock_t( sim_t* sim, const std::string& name, race_e r ):
  player_t( sim, WARLOCK, name, r ),
    havoc_target( nullptr ),
    havoc_spells(),
    wracking_brilliance(false),
    agony_accumulator( 0.0 ),
    active_pets( 0 ),
    warlock_pet_list( this ),
    active(),
    talents(),
    mastery_spells(),
    cooldowns(),
    spec(),
    buffs(),
    gains(),
    procs(),
    spells(),
    initial_soul_shards( 3 ),
    default_pet()
  {
    cooldowns.haunt = get_cooldown( "haunt" );
    cooldowns.shadowburn = get_cooldown("shadowburn");
    cooldowns.soul_fire = get_cooldown("soul_fire");
    cooldowns.call_dreadstalkers = get_cooldown("call_dreadstalkers");
    cooldowns.deathbolt = get_cooldown("deathbolt");
    cooldowns.phantom_singularity = get_cooldown("phantom_singularity");
    cooldowns.darkglare = get_cooldown("summon_darkglare");
    cooldowns.demonic_tyrant = get_cooldown( "summon_demonic_tyrant" );

    regen_type = REGEN_DYNAMIC;
    regen_caches[CACHE_HASTE] = true;
    regen_caches[CACHE_SPELL_HASTE] = true;

    flashpoint_threshold = 0.8;
  }

void warlock_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
  case CACHE_MASTERY:
    if ( mastery_spells.master_demonologist->ok() )
      player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    break;

  default: break;
  }
}

double warlock_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = player_t::composite_player_target_multiplier( target, school );

  const warlock_td_t* td = get_target_data( target );

  if (specialization() == WARLOCK_AFFLICTION) {
    if (td->debuffs_haunt->check())
      m *= 1.0 + td->debuffs_haunt->data().effectN(2).percent();
    if (td->debuffs_shadow_embrace->check())
      m *= 1.0 + (td->debuffs_shadow_embrace->data().effectN(1).percent() * td->debuffs_shadow_embrace->check());

    for (auto& current_ua : td->dots_unstable_affliction)
    {
      if (current_ua->is_ticking())
      {
        m *= 1.0 + spec.unstable_affliction->effectN(3).percent();
        break;
      }
    }
  }

  if (td->debuffs_from_the_shadows->check() && school == SCHOOL_SHADOWFLAME)
  {
    m *= 1.0 + td->debuffs_from_the_shadows->data().effectN(1).percent();
  }

  return m;
}

double warlock_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  return m;
}

double warlock_t::composite_player_pet_damage_multiplier(const action_state_t* s) const
{
  double m = player_t::composite_player_pet_damage_multiplier(s);

  if ( specialization() == WARLOCK_DESTRUCTION )
  {
    m *= 1.0 + spec.destruction->effectN(3).percent();
  }
  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    m *= 1.0 + spec.demonology->effectN(3).percent();
  }
  if ( specialization() == WARLOCK_AFFLICTION )
  {
    m *= 1.0 + spec.affliction->effectN(3).percent();
  }
  return m;
}

double warlock_t::composite_spell_crit_chance() const
{
  double sc = player_t::composite_spell_crit_chance();

  if (buffs.dark_soul_instability->check())
    sc += buffs.dark_soul_instability->check_value();

  return sc;
}

double warlock_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if (buffs.dark_soul_misery->check())
    h *= 1.0 / (1.0 + buffs.dark_soul_misery->check_value());

  if (buffs.reverse_entropy->check())
    h *= 1.0 / (1.0 + buffs.reverse_entropy->check_value());

  return h;
}

double warlock_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  if (buffs.reverse_entropy->check())
    h *= 1.0 / (1.0 + buffs.reverse_entropy->check_value());

  return h;
}

double warlock_t::composite_melee_crit_chance() const
{
  double mc = player_t::composite_melee_crit_chance();

  if (buffs.dark_soul_instability->check())
    mc += buffs.dark_soul_instability->check_value();

  return mc;
}

double warlock_t::composite_mastery() const
{
  double m = player_t::composite_mastery();
  return m;
}

double warlock_t::composite_rating_multiplier( rating_e rating ) const
{
  double m = player_t::composite_rating_multiplier( rating );
  return m;
}

double warlock_t::resource_gain( resource_e resource_type, double amount, gain_t* source, action_t* action )
{
  if ( resource_type == RESOURCE_SOUL_SHARD )
  {
    int current_soul_shards = (int)resources.current[ resource_type ];
    if ( current_soul_shards % 2 == 0 )
    {
      expansion::bfa::trigger_leyshocks_grand_compilation( STAT_CRIT_RATING, this );
    }
    else
    {
      expansion::bfa::trigger_leyshocks_grand_compilation( STAT_VERSATILITY_RATING, this );
    }

    // Chaos Shards triggers for all specializations
    if ( azerite.chaos_shards.ok() )
    {
      // Check if soul shard was filled
      if ( std::floor( resources.current[ RESOURCE_SOUL_SHARD ] ) < std::floor( std::min( resources.current[ RESOURCE_SOUL_SHARD ] + amount, 5.0 ) ) )
      {
        if ( rng().roll( azerite.chaos_shards.spell_ref().effectN( 1 ).percent() / 10.0 ) )
          buffs.chaos_shards->trigger();
      }
    }
  }

  return player_t::resource_gain( resource_type, amount, source, action );
}

double warlock_t::resource_regen_per_second( resource_e r ) const
{
  double reg = player_t::resource_regen_per_second( r );
  return reg;
}

double warlock_t::composite_armor() const
{
  return player_t::composite_armor() + spec.fel_armor -> effectN( 2 ).base_value();
}

void warlock_t::halt()
{
  player_t::halt();
  if ( spells.melee )
    spells.melee->cancel();
}

double warlock_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( attr == ATTR_INTELLECT )
    return spec.nethermancy->effectN( 1 ).percent();

  return 0.0;
}

action_t* warlock_t::create_action( const std::string& action_name, const std::string& options_str )
{
  using namespace actions;

  if ( ( action_name == "summon_pet" ) && default_pet.empty() ) {
    sim->errorf( "Player %s used a generic pet summoning action without specifying a default_pet.\n", name() );
    return nullptr;
  }
  // Pets
  if ( action_name == "summon_felhunter"      ) return new          summon_main_pet_t( "felhunter", this );
  if ( action_name == "summon_felguard"       ) return new           summon_main_pet_t( "felguard", this );
  if ( action_name == "summon_succubus"       ) return new           summon_main_pet_t( "succubus", this );
  if ( action_name == "summon_voidwalker"     ) return new         summon_main_pet_t( "voidwalker", this );
  if ( action_name == "summon_imp"            ) return new                summon_main_pet_t( "imp", this );
  if ( action_name == "summon_pet"            ) return new          summon_main_pet_t( default_pet, this );
  // Base Spells
  if ( action_name == "drain_life"            ) return new               drain_life_t( this, options_str );
  if ( action_name == "grimoire_of_sacrifice" ) return new    grimoire_of_sacrifice_t( this, options_str ); //aff and destro

  if ( specialization() == WARLOCK_AFFLICTION )
  {
    action_t* aff_action = create_action_affliction( action_name, options_str );
    if ( aff_action )
      return aff_action;
  }

  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    action_t* demo_action = create_action_demonology( action_name, options_str );
    if ( demo_action )
      return demo_action;
  }

  if ( specialization() == WARLOCK_DESTRUCTION )
  {
    action_t* destro_action = create_action_destruction(action_name, options_str);
    if (destro_action)
      return destro_action;
  }

  return player_t::create_action( action_name, options_str );
}

pet_t* warlock_t::create_pet( const std::string& pet_name, const std::string& pet_type )
{
  pet_t* p = find_pet( pet_name );
  if ( p ) return p;
  using namespace pets;

  pet_t* summon_pet = create_main_pet(pet_name, pet_type );
  if (summon_pet)
    return summon_pet;

  return nullptr;
}

void warlock_t::create_pets()
{
  create_all_pets();

  for ( auto& pet : pet_name_list )
  {
    create_pet( pet );
  }
}

void warlock_t::create_buffs()
{
  player_t::create_buffs();

  create_buffs_affliction();
  create_buffs_demonology();
  create_buffs_destruction();

  buffs.grimoire_of_sacrifice = make_buff( this, "grimoire_of_sacrifice", talents.grimoire_of_sacrifice->effectN( 2 ).trigger() )
    ->set_chance( 1.0 );
}

void warlock_t::init_spells()
{
  player_t::init_spells();

  warlock_t::init_spells_affliction();
  warlock_t::init_spells_demonology();
  warlock_t::init_spells_destruction();

  // General
  spec.fel_armor = find_spell( 104938 );
  spec.nethermancy = find_spell( 86091 );

  // Specialization Spells
  spec.immolate                         = find_specialization_spell( "Immolate" );
  spec.nightfall                        = find_specialization_spell( "Nightfall" );
  spec.wild_imps                        = find_specialization_spell( "Wild Imps" );
  spec.demonic_core                     = find_specialization_spell( "Demonic Core" );
  spec.shadow_bite                      = find_specialization_spell( "Shadow Bite" );
  spec.unending_resolve                 = find_specialization_spell( "Unending Resolve" );
  spec.firebolt                         = find_specialization_spell( "Firebolt" );

  // Talents
  talents.demon_skin                    = find_talent_spell( "Fire and Brimstone" );
  talents.burning_rush                  = find_talent_spell( "Burning Rush" );
  talents.dark_pact                     = find_talent_spell( "Dark Pact" );
  talents.darkfury                      = find_talent_spell( "Darkfury" );
  talents.mortal_coil                   = find_talent_spell( "Mortal Coil" );
  talents.demonic_circle                = find_talent_spell( "Demonic Circle" );
  talents.grimoire_of_sacrifice         = find_talent_spell( "Grimoire of Sacrifice" ); // aff and destro
  active.grimoire_of_sacrifice_proc     = new actions::grimoire_of_sacrifice_damage_t( this ); // grimoire of sacrifice
  talents.soul_conduit                  = find_talent_spell( "Soul Conduit" );
}

void warlock_t::init_rng()
{
  if ( specialization() == WARLOCK_AFFLICTION )
    init_rng_affliction();
  if ( specialization() == WARLOCK_DEMONOLOGY )
    init_rng_demonology();
  if ( specialization() == WARLOCK_DESTRUCTION )
    init_rng_destruction();

  grimoire_of_sacrifice_rppm     = get_rppm( "grimoire_of_sacrifice", find_spell( 196099 ) );

  player_t::init_rng();
}

void warlock_t::init_gains()
{
  player_t::init_gains();

  if ( specialization() == WARLOCK_AFFLICTION )
    init_gains_affliction();
  if ( specialization() == WARLOCK_DEMONOLOGY )
    init_gains_demonology();
  if ( specialization() == WARLOCK_DESTRUCTION )
    init_gains_destruction();

  gains.miss_refund                     = get_gain( "miss_refund" );
  gains.shadow_bolt                     = get_gain( "shadow_bolt" );
  gains.soul_conduit                    = get_gain( "soul_conduit" );

  gains.chaos_shards                    = get_gain( "chaos_shards" );
}

void warlock_t::init_procs()
{
  player_t::init_procs();

  if ( specialization() == WARLOCK_AFFLICTION )
    init_procs_affliction();
  if ( specialization() == WARLOCK_DEMONOLOGY )
    init_procs_demonology();
  if ( specialization() == WARLOCK_DESTRUCTION )
    init_procs_destruction();

  procs.one_shard_hog                   = get_proc( "one_shard_hog" );
  procs.two_shard_hog                   = get_proc( "two_shard_hog" );
  procs.three_shard_hog                 = get_proc( "three_shard_hog" );
  procs.portal_summon                   = get_proc( "portal_summon" );
  procs.demonic_calling                 = get_proc( "demonic_calling" );
  procs.soul_conduit                    = get_proc( "soul_conduit" );
}

void warlock_t::init_base_stats()
{
  if ( base.distance < 1.0 )
    base.distance = 40.0;

  player_t::init_base_stats();

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility = 0.0;
  base.spell_power_per_intellect = 1.0;

  base.attribute_multiplier[ATTR_STAMINA] *= 1.0 + spec.fel_armor->effectN( 1 ).percent();

  resources.base[RESOURCE_SOUL_SHARD] = 5;

  if ( default_pet.empty() )
  {
    if ( specialization() == WARLOCK_AFFLICTION )
      default_pet = "imp";
    else if ( specialization() == WARLOCK_DEMONOLOGY )
      default_pet = "felguard";
    else if ( specialization() == WARLOCK_DESTRUCTION )
      default_pet = "imp";
  }
}

void warlock_t::init_scaling()
{
  player_t::init_scaling();
}

void warlock_t::init_assessors()
{
  player_t::init_assessors();

  auto assessor_fn = [ this ] ( dmg_e, action_state_t* s )
  {
    if ( get_target_data(s->target)->dots_seed_of_corruption->is_ticking() )
    {
      accumulate_seed_of_corruption(get_target_data(s->target),s->result_total);
    }

    return assessor::CONTINUE;
  };

  assessor_out_damage.add( assessor::TARGET_DAMAGE - 1, assessor_fn );

  for ( auto pet : pet_list )
  {
    pet->assessor_out_damage.add( assessor::TARGET_DAMAGE - 1, assessor_fn );
  }
}

void warlock_t::apl_precombat()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action("summon_pet");
  if (specialization() == WARLOCK_DEMONOLOGY)
    precombat->add_action("inner_demons,if=talent.inner_demons.enabled");
  if ( specialization() != WARLOCK_DEMONOLOGY )
    precombat->add_action( "grimoire_of_sacrifice,if=talent.grimoire_of_sacrifice.enabled" );

  precombat->add_action( "snapshot_stats" );

  if ( sim->allow_potions )
  {
    precombat->add_action( "potion" );
  }
  if (specialization() == WARLOCK_DEMONOLOGY)
    precombat->add_action("demonbolt");
  if ( specialization() == WARLOCK_DESTRUCTION )
  {
    precombat->add_talent ( this, "Soul Fire" );
    precombat->add_action("incinerate,if=!talent.soul_fire.enabled");
  }
  if ( specialization() == WARLOCK_AFFLICTION )
  {
    precombat->add_action( "seed_of_corruption,if=spell_targets.seed_of_corruption_aoe>=3" );
    precombat->add_action( "haunt" );
    precombat->add_action( "shadow_bolt,if=!talent.haunt.enabled&spell_targets.seed_of_corruption_aoe<3" );
  }
}

std::string warlock_t::default_potion() const
{
  std::string lvl110_potion = "prolonged_power";

  return ( true_level >  110 ) ? "battle_potion_of_intellect" :
         ( true_level >= 100 ) ? lvl110_potion :
         ( true_level >=  90 ) ? "draenic_intellect" :
         ( true_level >=  85 ) ? "jade_serpent" :
         ( true_level >=  80 ) ? "volcanic" :
                                 "disabled";
}

std::string warlock_t::default_flask() const
{
  return ( true_level >  110 ) ? "endless_fathoms" :
         ( true_level >= 100 ) ? "whispered_pact" :
         ( true_level >=  90 ) ? "greater_draenic_intellect_flask" :
         ( true_level >=  85 ) ? "warm_sun" :
         ( true_level >=  80 ) ? "draconic_mind" :
                                 "disabled";
}

std::string warlock_t::default_food() const
{
  std::string lvl100_food =
    (specialization() == WARLOCK_DESTRUCTION) ?   "frosty_stew" :
    (specialization() == WARLOCK_DEMONOLOGY) ?    "frosty_stew" :
    (specialization() == WARLOCK_AFFLICTION) ?    "felmouth_frenzy" :
                                                  "felmouth_frenzy";

  std::string lvl110_food =
    (specialization() == WARLOCK_AFFLICTION) ?    "nightborne_delicacy_platter" :
                                                  "azshari_salad";

  return ( true_level > 110 ) ? "bountiful_captains_feast" :
         ( true_level > 100 ) ? lvl110_food :
         ( true_level >  90 ) ? lvl100_food :
                                "disabled";
}

std::string warlock_t::default_rune() const
{
  return ( true_level >= 120 ) ? "battle_scarred" :
         ( true_level >= 110 ) ? "defiled" :
         ( true_level >= 100 ) ? "focus" :
                                 "disabled";
}

void warlock_t::apl_global_filler() { }

void warlock_t::apl_default()
{
  if ( specialization() == WARLOCK_AFFLICTION )
    create_apl_affliction();
  else if ( specialization() == WARLOCK_DEMONOLOGY )
    create_apl_demonology();
  else if ( specialization() == WARLOCK_DESTRUCTION )
    create_apl_destruction();
}

void warlock_t::init_action_list()
{
  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    apl_precombat();

    apl_default();

    use_default_action_list = true;
  }

  player_t::init_action_list();
}

void warlock_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[RESOURCE_SOUL_SHARD] = initial_soul_shards;
}

void warlock_t::combat_begin()
{
  player_t::combat_begin();
}

void warlock_t::reset()
{
  player_t::reset();

  range::for_each( sim->target_list, [this]( const player_t* t )
  {
    if ( auto td = target_data[t] )
    {
      td->reset();
    }

    range::for_each( t->pet_list, [this]( const player_t* add )
    {
      if ( auto td = target_data[add] )
      {
        td->reset();
      }
    } );
  } );

  warlock_pet_list.active = nullptr;
  havoc_target = nullptr;
  agony_accumulator = rng().range( 0.0, 0.99 );
  wild_imp_spawns.clear();
}

void warlock_t::create_options()
{
  player_t::create_options();

  add_option( opt_int( "soul_shards", initial_soul_shards ) );
  add_option( opt_string( "default_pet", default_pet ) );
  add_option( opt_float( "flashpoint_threshold", flashpoint_threshold, 0.0, 1.0) );
}

//Used to determine how many Wild Imps are waiting to be spawned from Hand of Guldan
int warlock_t::get_spawning_imp_count()
{
  return as<int>(wild_imp_spawns.size());
}

//Function for returning the time until a certain number of imps will have spawned
//In the case where count is equal to or greater than number of incoming imps, time to last imp is returned
//Otherwise, time to the Nth imp spawn will be returned
//All other cases will return 0. A negative (or zero) count value will behave as "all" 
timespan_t warlock_t::time_to_imps(int count)
{
  timespan_t max = 0_ms;
  if (count >= as<int>(wild_imp_spawns.size()) || count <= 0){
    for (auto ev : wild_imp_spawns)
    {
      timespan_t ex = debug_cast<actions::imp_delay_event_t*>(ev)->expected_time();
      if(ex > max)
        max = ex;
    }
    return max;
  }
  else
  {
    std::priority_queue<timespan_t> shortest;
    for (auto ev : wild_imp_spawns)
    {
      timespan_t ex = debug_cast<actions::imp_delay_event_t*>(ev)->expected_time();
      if (shortest.size() >= count && ex < shortest.top())
      {
        shortest.pop();
        shortest.push(ex);
      }
      else if (shortest.size() < count)
      {
        shortest.push(ex);
      }
    }

    if (shortest.size() > 0)
    {
      return shortest.top();
    }
    else
    {
      return 0_ms;
    }
  }
}


//Function for returning the the number of imps that will spawn in a specified time period.
int warlock_t::imps_spawned_during( timespan_t period )
{
  int count = 0;

  for ( auto ev : wild_imp_spawns )
  {
    timespan_t ex = debug_cast<actions::imp_delay_event_t*>( ev )->expected_time();

    if ( ex < period )
      count++;
  }

  return count;
}

std::string warlock_t::create_profile( save_e stype )
{
  std::string profile_str = player_t::create_profile( stype );

  if ( stype & SAVE_PLAYER )
  {
    if ( initial_soul_shards != 3 )    profile_str += "soul_shards=" + util::to_string( initial_soul_shards ) + "\n";
    if ( !default_pet.empty() )        profile_str += "default_pet=" + default_pet + "\n";
  }

  return profile_str;
}

void warlock_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  warlock_t* p = debug_cast< warlock_t* >( source );

  initial_soul_shards = p->initial_soul_shards;
  default_pet = p->default_pet;
  flashpoint_threshold = p->flashpoint_threshold;
}

stat_e warlock_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
    // This is all a guess at how the hybrid primaries will work, since they
    // don't actually appear on cloth gear yet. TODO: confirm behavior
  case STAT_STR_AGI_INT:
  case STAT_AGI_INT:
  case STAT_STR_INT:
    return STAT_INTELLECT;
  case STAT_STR_AGI:
    return STAT_NONE;
  case STAT_SPIRIT:
    return STAT_NONE;
  case STAT_BONUS_ARMOR:
    return STAT_NONE;
  default: return s;
  }
}

pet_t* warlock_t::create_main_pet(const std::string& pet_name, const std::string& pet_type)
{
  pet_t* p = find_pet(pet_name);
  if (p) return p;
  using namespace pets;

  if (pet_name == "felhunter")          return new pets::base::felhunter_pet_t(this, pet_name);
  if (pet_name == "imp")                return new pets::base::imp_pet_t(this, pet_name);
  if (pet_name == "succubus")           return new pets::base::succubus_pet_t(this, pet_name);
  if (pet_name == "voidwalker")         return new pets::base::voidwalker_pet_t(this, pet_name);
  if (specialization() == WARLOCK_DEMONOLOGY)
  {
    return create_demo_pet(pet_name, pet_type);
  }

  return nullptr;
}

void warlock_t::create_all_pets()
{
  if (specialization() == WARLOCK_DESTRUCTION)
  {
    for (size_t i = 0; i < warlock_pet_list.infernals.size(); i++)
    {
      warlock_pet_list.infernals[i] = new pets::destruction::infernal_t(this);
    }
  }

  if (specialization() == WARLOCK_AFFLICTION)
  {
    for (size_t i = 0; i < warlock_pet_list.darkglare.size(); i++)
    {
      warlock_pet_list.darkglare[i] = new pets::affliction::darkglare_t(this);
    }
  }
}

expr_t* warlock_t::create_pet_expression(const std::string& name_str)
{
  if (name_str == "last_cast_imps")
  {
    return make_fn_expr( "last_cast_imps", [ this ]() {
      return warlock_pet_list.wild_imps.n_active_pets(
        []( const pets::demonology::wild_imp_pet_t* pet ) {
          return pet->resources.current[RESOURCE_ENERGY] <= 20;
        } );
    } );
  }
  else if (name_str == "two_cast_imps")
  {
    return make_fn_expr( "two_cast_imps", [ this ]() {
      return warlock_pet_list.wild_imps.n_active_pets(
        []( const pets::demonology::wild_imp_pet_t* pet ) {
          return pet->resources.current[RESOURCE_ENERGY] <= 40;
        } );
    } );
  }

  return player_t::create_expression(name_str);
}

expr_t* warlock_t::create_expression( const std::string& name_str )
{
  if ( name_str == "time_to_shard" )
  {
    auto agony_id = find_action_id("agony");

    return make_fn_expr( name_str, [this,agony_id]()
    {
      auto td = get_target_data(target);
      double active_agonies = get_active_dots(agony_id);
      if (sim->debug)
        sim->out_debug.printf("active agonies: %f", active_agonies);
      dot_t* agony = td->dots_agony;
      if ( active_agonies == 0 || !agony->current_action )
      {
        return std::numeric_limits<double>::infinity();
      }
      action_state_t* agony_state = agony->current_action->get_state(agony->state);
      timespan_t dot_tick_time = agony->current_action->tick_time(agony_state);

      // Seeks to return the average expected time for the player to generate a single soul shard.
      // TOCHECK regularly.

      double average = 1.0 / ( 0.184 * std::pow( active_agonies, -2.0 / 3.0 ) ) * dot_tick_time.total_seconds() / active_agonies;

      if ( talents.creeping_death->ok() )
        average /= 1.0 + talents.creeping_death->effectN( 1 ).percent();

      if (sim->debug)
        sim->out_debug.printf("time to shard return: %f", average);
      action_state_t::release(agony_state);
      return average;
    } );
  }
  else if ( name_str == "pet_count" )
  {
    return make_ref_expr( name_str, active_pets );
  }
  else if (name_str == "last_cast_imps")
  {
    return create_pet_expression(name_str);
  }
  else if (name_str == "two_cast_imps")
  {
    return create_pet_expression(name_str);
  }
  else if (name_str == "deathbolt_setup") {
    return create_aff_expression(name_str);
  }
  // TODO: Remove the deprecated buff expressions once the APL is adjusted for
  // havoc_active/havoc_remains.
  else if ( name_str == "havoc_active" || name_str == "buff.active_havoc.up" ) {
    return make_fn_expr( name_str, [ this ]
    {
      return havoc_target != nullptr;
    } );
  }
  else if ( name_str == "havoc_remains" || name_str == "buff.active_havoc.remains" ) {
    return make_fn_expr( name_str, [ this ]
    {
      return havoc_target ? get_target_data( havoc_target )->debuffs_havoc->remains().total_seconds() : 0.0;
    } );
  }
  else if (name_str == "incoming_imps") {
    return make_fn_expr(name_str, [this]{
      return this->get_spawning_imp_count();
    });
  }

  std::vector<std::string> splits = util::string_split( name_str, "." );

  if (splits.size() == 3 && splits[0] == "time_to_imps" && splits[2] == "remains")
  {
    std::string amt = splits[1];

    return make_fn_expr(name_str, [this, amt](){
      if(amt == "all")
      {
        return this->time_to_imps(-1);
      }
      else
      {
        return this->time_to_imps(std::stoi(amt));
      }
    });
  }
  else if ( splits.size() == 2 && util::str_compare_ci( splits[0], "imps_spawned_during" ) )
  {
    std::string period = splits[1];

    return make_fn_expr( name_str, [this, period]()
    {
      // Add a custom split .summon_demonic_tyrant which returns its cast time.
      return this->imps_spawned_during( timespan_t::from_millis( std::stod( period ) ) );
    } );
  }

  return player_t::create_expression( name_str );
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class warlock_report_t: public player_report_extension_t
{
public:
  warlock_report_t( warlock_t& player ):
    p( player )
  {

  }

  virtual void html_customsection( report::sc_html_stream& /* os*/ ) override
  {
    (void)p;
    /*// Custom Class Section
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n"
    << "\t\t\t\t\t<h3 class=\"toggle open\">Custom Section</h3>\n"
    << "\t\t\t\t\t<div class=\"toggle-content\">\n";

    os << p.name();

    os << "\t\t\t\t\t\t</div>\n" << "\t\t\t\t\t</div>\n";*/
  }
private:
  warlock_t& p;
};

static void do_trinket_init( warlock_t*                player,
  specialization_e         spec,
  const special_effect_t*& ptr,
  const special_effect_t&  effect )
{
  // Ensure we have the spell data. This will prevent the trinket effect from working on live
  // Simulationcraft. Also ensure correct specialization.
  if ( !player->find_spell( effect.spell_id )->ok() ||
    player->specialization() != spec )
  {
    return;
  }
  // Set pointer, module considers non-null pointer to mean the effect is "enabled"
  ptr = &( effect );
}

struct warlock_module_t : public module_t
{
  warlock_module_t() : module_t( WARLOCK ) { }

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const override
  {
    auto  p = new warlock_t( sim, name, r );
    p->report_extension = std::unique_ptr<player_report_extension_t>( new warlock_report_t( *p ) );
    return p;
  }

  virtual void static_init() const override
  {
    // Leyshock's!
    // Shared spells
    // Drain Life
    expansion::bfa::register_leyshocks_trigger( 234153, STAT_HASTE_RATING );
    // Burning Rush
    expansion::bfa::register_leyshocks_trigger( 111400, STAT_CRIT_RATING );
    // Mortal Coil
    expansion::bfa::register_leyshocks_trigger( 6789, STAT_HASTE_RATING );
    // Summon Demonic Circle
    expansion::bfa::register_leyshocks_trigger( 268358, STAT_CRIT_RATING );
    // Demonic Circle: Teleport
    expansion::bfa::register_leyshocks_trigger( 48020, STAT_VERSATILITY_RATING );

    // Affliction-specific spells
    // Agony
    expansion::bfa::register_leyshocks_trigger( 980, STAT_CRIT_RATING );
    // Corruption
    expansion::bfa::register_leyshocks_trigger( 172, STAT_VERSATILITY_RATING );
    // Seed of Corruption
    expansion::bfa::register_leyshocks_trigger( 27243, STAT_HASTE_RATING );
    // Shadow Bolt
    expansion::bfa::register_leyshocks_trigger( 232670, STAT_MASTERY_RATING );
    // Darkglare
    expansion::bfa::register_leyshocks_trigger( 205180, STAT_HASTE_RATING );
    // Unstable Affliction
    expansion::bfa::register_leyshocks_trigger( 233490, STAT_MASTERY_RATING );
    expansion::bfa::register_leyshocks_trigger( 233496, STAT_HASTE_RATING );
    expansion::bfa::register_leyshocks_trigger( 233497, STAT_CRIT_RATING );
    expansion::bfa::register_leyshocks_trigger( 233498, STAT_MASTERY_RATING );
    expansion::bfa::register_leyshocks_trigger( 233499, STAT_VERSATILITY_RATING );
    // Dark Soul: Misery
    expansion::bfa::register_leyshocks_trigger( 113860, STAT_CRIT_RATING );
    // Haunt
    expansion::bfa::register_leyshocks_trigger( 48181, STAT_HASTE_RATING );
    // Vile Taint
    expansion::bfa::register_leyshocks_trigger( 278350, STAT_HASTE_RATING );
    // Phantom Singularity
    expansion::bfa::register_leyshocks_trigger( 205179, STAT_CRIT_RATING );
    expansion::bfa::register_leyshocks_trigger( 205246, STAT_MASTERY_RATING );
    // Siphon Life
    expansion::bfa::register_leyshocks_trigger( 63106, STAT_MASTERY_RATING );
    // Deathbolt
    expansion::bfa::register_leyshocks_trigger( 264106, STAT_MASTERY_RATING );

    // Destruction
    // Chaos Bolt
    expansion::bfa::register_leyshocks_trigger( 116858, STAT_HASTE_RATING );
    // Conflagrate
    expansion::bfa::register_leyshocks_trigger( 17962, STAT_MASTERY_RATING );
    // Havoc
    expansion::bfa::register_leyshocks_trigger( 80240, STAT_CRIT_RATING );
    // Immolate
    expansion::bfa::register_leyshocks_trigger( 348, STAT_CRIT_RATING );
    // Internal Combustion
    expansion::bfa::register_leyshocks_trigger( 266134, STAT_CRIT_RATING );
    // Incinerate
    expansion::bfa::register_leyshocks_trigger( 29722, STAT_MASTERY_RATING );
    // Rain of Fire
    expansion::bfa::register_leyshocks_trigger( 5740, STAT_HASTE_RATING );
    expansion::bfa::register_leyshocks_trigger( 42223, STAT_VERSATILITY_RATING );
    // Infernal
    expansion::bfa::register_leyshocks_trigger( 1122, STAT_HASTE_RATING );
    expansion::bfa::register_leyshocks_trigger( 22703, STAT_VERSATILITY_RATING );
    // Cataclysm
    expansion::bfa::register_leyshocks_trigger( 152108, STAT_CRIT_RATING );
    // Channel Demonfire
    expansion::bfa::register_leyshocks_trigger( 196447, STAT_VERSATILITY_RATING );
    // Dark Soul: Instability
    expansion::bfa::register_leyshocks_trigger( 113858, STAT_MASTERY_RATING );
    // Soul Fire
    expansion::bfa::register_leyshocks_trigger( 6353, STAT_MASTERY_RATING );
    // Shadowburn
    expansion::bfa::register_leyshocks_trigger( 17877, STAT_VERSATILITY_RATING );

    // Demonology
    // Call Dreadstalkers
    expansion::bfa::register_leyshocks_trigger( 104316, STAT_CRIT_RATING );
    // Demonbolt
    expansion::bfa::register_leyshocks_trigger( 264178, STAT_VERSATILITY_RATING );
    // Hand of Gul'dan
    expansion::bfa::register_leyshocks_trigger( 105174, STAT_CRIT_RATING );
    // Implosion
    expansion::bfa::register_leyshocks_trigger( 196277, STAT_HASTE_RATING );
    // Demonic Tyrant
    expansion::bfa::register_leyshocks_trigger( 265187, STAT_HASTE_RATING );
    // Demonic Strength
    expansion::bfa::register_leyshocks_trigger( 267171, STAT_VERSATILITY_RATING );
    // Bilescourge Bombers
    expansion::bfa::register_leyshocks_trigger( 267211, STAT_CRIT_RATING );
    expansion::bfa::register_leyshocks_trigger( 267213, STAT_CRIT_RATING );
    // Doom
    expansion::bfa::register_leyshocks_trigger( 265412, STAT_CRIT_RATING );
    // Soul Strike
    expansion::bfa::register_leyshocks_trigger( 264057, STAT_VERSATILITY_RATING );
    // Grimoire: felguard
    expansion::bfa::register_leyshocks_trigger( 111898, STAT_HASTE_RATING );
    // Nether Portal
    expansion::bfa::register_leyshocks_trigger( 267217, STAT_MASTERY_RATING );
    // Summon Vilefiend
    expansion::bfa::register_leyshocks_trigger( 264119, STAT_HASTE_RATING );
  }

  virtual void register_hotfixes() const override { }

  virtual bool valid() const override { return true; }
  virtual void init( player_t* ) const override { }
  virtual void combat_begin( sim_t* ) const override { }
  virtual void combat_end( sim_t* ) const override { }
};

warlock::warlock_t::pets_t::pets_t( warlock_t* w ) :
  active( nullptr ),
  last( nullptr ),
  dreadstalkers( "dreadstalker", w ),
  vilefiends( "vilefiend", w ),
  demonic_tyrants( "demonic_tyrant", w ),
  wild_imps( "wild_imp", w ),
  shivarra( "shivarra", w ),
  darkhounds( "darkhound", w ),
  bilescourges( "bilescourge", w ),
  urzuls( "urzul", w ),
  void_terrors( "void_terror", w ),
  wrathguards( "wrathguard", w ),
  vicious_hellhounds( "vicious_hellhound", w ),
  illidari_satyrs( "illidari_satyr", w ),
  eyes_of_guldan( "eye_of_guldan", w ),
  prince_malchezaar( "prince_malchezaar", w )
{ }
}

const module_t* module_t::warlock()
{
  static warlock::warlock_module_t m;
  return &m;
}
