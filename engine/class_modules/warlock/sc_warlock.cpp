#include "simulationcraft.hpp"
#include "sc_warlock.hpp"
namespace warlock
{ 
// Pets

  void parse_spell_coefficient( action_t& a )
  {
    for ( size_t i = 1; i <= a.data()._effects->size(); i++ )
    {
      if ( a.data().effectN( i ).type() == E_SCHOOL_DAMAGE )
        a.spell_power_mod.direct = a.data().effectN( i ).sp_coeff();
      else if ( a.data().effectN( i ).type() == E_APPLY_AURA && a.data().effectN( i ).subtype() == A_PERIODIC_DAMAGE )
        a.spell_power_mod.tick = a.data().effectN( i ).sp_coeff();
    }
  }
// Spells
  namespace actions
  {
    struct drain_life_t : public warlock_spell_t
    {
      double inevitable_demise;

      drain_life_t( warlock_t* p, const std::string& options_str ) :
        warlock_spell_t( p, "Drain Life" )
      {
        parse_options( options_str );
        channeled = true;
        hasted_ticks = false;
        may_crit = false;
      }

      double bonus_ta(const action_state_t* s) const override
      {
        double ta = warlock_spell_t::bonus_ta(s);
        ta += inevitable_demise;
        return ta;
      }

      void execute() override
      {
        inevitable_demise = p()->buffs.inevitable_demise->check_stack_value();
        warlock_spell_t::execute();
        p()->buffs.inevitable_demise->expire();
      }
    };

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

  namespace buffs
  {
    struct debuff_havoc_t : public warlock_buff_t<buff_t>
    {
      debuff_havoc_t( warlock_td_t& p ) :
        base_t( p, "havoc", p.source -> find_spell( 80240 ) ) { }

      void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
      {
        base_t::expire_override( expiration_stacks, remaining_duration );

        p()->havoc_target = nullptr;
      }
    };
  }

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
    debuffs_tormented_agony = make_buff(*this, "tormented_agony", source->find_spell(252938));

    //Destro
    dots_immolate = target->get_dot("immolate", &p);
    dots_roaring_blaze = target->get_dot("roaring_blaze", &p);
    dots_channel_demonfire = target->get_dot("channel_demonfire", &p);

    debuffs_eradication = make_buff( *this, "eradication", source->find_spell( 196414 ) )
      ->set_refresh_behavior( buff_refresh_behavior::DURATION );
    debuffs_roaring_blaze = make_buff( *this, "roaring_blaze", source->find_spell( 205690 ) )
      ->set_max_stack( 100 );
    debuffs_shadowburn = make_buff(*this, "shadowburn", source->find_spell(17877));
    debuffs_havoc = new buffs::debuff_havoc_t(*this);
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

      if (dots_drain_soul->is_ticking())
      {
        warlock.resource_gain(RESOURCE_SOUL_SHARD, 1, warlock.gains.drain_soul);
      }
    }

    if ( debuffs_haunt->check() )
    {
      warlock.sim->print_log( "Player {} demised. Warlock {} reset haunt's cooldown.", target->name(), warlock.name() );

      warlock.cooldowns.haunt->reset( true );
    }

    if (debuffs_shadowburn->check())
    {
      warlock.sim->print_log("Player {} demised. Warlock {} reset haunt's cooldown.", target->name(), warlock.name());

      warlock.cooldowns.shadowburn->reset(true);
    }
  }

warlock_t::warlock_t( sim_t* sim, const std::string& name, race_e r ):
  player_t( sim, WARLOCK, name, r ),
    havoc_target( nullptr ),
    wracking_brilliance(false),
    agony_accumulator( 0.0 ),
    agony_expression_thing( 0.0 ),
    warlock_pet_list(),
    active(),
    talents(),
    legendary(),
    mastery_spells(),
    cooldowns(),
    spec(),
    buffs(),
    gains(),
    procs(),
    spells(),
    initial_soul_shards( 3 ),
    allow_sephuz( false ),
    default_pet()
  {
    legendary.hood_of_eternal_disdain = nullptr;
    legendary.insignia_of_the_grand_army = nullptr;
    legendary.lessons_of_spacetime = nullptr;
    legendary.power_cord_of_lethtendris = nullptr;
    legendary.reap_and_sow = nullptr;
    legendary.sacrolashs_dark_strike = nullptr;
    legendary.sephuzs_secret = nullptr;
    legendary.sindorei_spite = nullptr;
    legendary.soul_of_the_netherlord = nullptr;
    legendary.stretens_sleepless_shackles = nullptr;
    legendary.the_master_harvester = nullptr;
    legendary.wakeners_loyalty = nullptr;
    legendary.kazzaks_final_curse = nullptr;
    legendary.sindorei_spite = nullptr;
    legendary.recurrent_ritual = nullptr;
    legendary.magistrike_restraints = nullptr;
    legendary.feretory_of_souls = nullptr;
    legendary.alythesss_pyrogenics = nullptr;
    legendary.odr_shawl_of_the_ymirjar = nullptr;
    legendary.wilfreds_sigil_of_superior_summoning = nullptr;

    cooldowns.haunt = get_cooldown( "haunt" );
    cooldowns.shadowburn = get_cooldown("shadowburn");
    cooldowns.soul_fire = get_cooldown("soul_fire");
    cooldowns.sindorei_spite_icd = get_cooldown( "sindorei_spite_icd" );
    cooldowns.call_dreadstalkers = get_cooldown("call_dreadstalkers");
    cooldowns.darkglare = get_cooldown("summon_darkglare");
    cooldowns.demonic_tyrant = get_cooldown( "summon_demonic_tyrant" );

    regen_type = REGEN_DYNAMIC;
    regen_caches[CACHE_HASTE] = true;
    regen_caches[CACHE_SPELL_HASTE] = true;

    talent_points.register_validity_fn( [this]( const spell_data_t* spell )
    {
      if ( find_item( 151649 ) ) // Soul of the Netherlord
      {
        switch ( specialization() )
        {
          case WARLOCK_AFFLICTION:
            return spell -> id() == 215941; // Soul Conduit
          case WARLOCK_DEMONOLOGY:
            return spell -> id() == 267216; // Inner Demons
          case WARLOCK_DESTRUCTION:
            return spell -> id() == 196412; // Eradication
          default:
            return false;
        }
      }
      return false;
    } );
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

  if ( legendary.stretens_sleepless_shackles )
    m *= 1.0 + buffs.stretens_insanity->check() * buffs.stretens_insanity->data().effectN( 1 ).percent();

  if ( specialization() == WARLOCK_DESTRUCTION && dbc::is_school( school, SCHOOL_FIRE ) ) 
    m *= 1.0 + buffs.alythesss_pyrogenics->check_stack_value();

  m *= 1.0 + buffs.sindorei_spite->check_stack_value();

  if ( legendary.insignia_of_the_grand_army )
    m *= 1.0 + find_spell( 152626 )->effectN( 2 ).percent();

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
  if ( legendary.reap_and_sow )
  {
    m *= 1.0 + find_spell( 281494 )->effectN( 3 ).percent();
  }
  if ( legendary.wakeners_loyalty )
  {
    m *= 1.0 + find_spell( 281495 )->effectN( 3 ).percent();
  }
  if ( legendary.lessons_of_spacetime )
  {
    m *= 1.0 + find_spell( 281496 )->effectN( 3 ).percent();
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

  if ( buffs.sephuzs_secret->check() )
  {
    h *= 1.0 / ( 1.0 + buffs.sephuzs_secret->check_value() );
  }

  if ( legendary.sephuzs_secret )
  {
    h *= 1.0 / ( 1.0 + legendary.sephuzs_secret->driver()->effectN( 3 ).percent() );
  }

  if ( buffs.demonic_speed->check() )
    h *= 1.0 / ( 1.0 + buffs.demonic_speed->check_value() );

  if (buffs.dark_soul_misery->check())
    h *= 1.0 / (1.0 + buffs.dark_soul_misery->check_value());

  if (buffs.reverse_entropy->check())
    h *= 1.0 / (1.0 + buffs.reverse_entropy->check_value());

  if (specialization() == WARLOCK_DEMONOLOGY)
  {
    if (buffs.dreaded_haste->check())
      h *= 1.0 / (1.0 + buffs.dreaded_haste->check_value());
  }

  return h;
}

double warlock_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  if ( buffs.sephuzs_secret->check() )
  {
    h *= 1.0 / ( 1.0 + buffs.sephuzs_secret->check_value() );
  }

  if ( legendary.sephuzs_secret )
  {
    h *= 1.0 / ( 1.0 + legendary.sephuzs_secret->driver()->effectN( 3 ).percent() );
  }

  if ( buffs.demonic_speed->check() )
    h *= 1.0 / ( 1.0 + buffs.demonic_speed->check_value() );

  if (buffs.reverse_entropy->check())
    h *= 1.0 / (1.0 + buffs.reverse_entropy->check_value());

  if (specialization() == WARLOCK_DEMONOLOGY)
  {
    if (buffs.dreaded_haste->check())
      h *= 1.0 / (1.0 + buffs.dreaded_haste->check_value());
  }

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

  buffs.grimoire_of_sacrifice = make_buff( this, "grimoire_of_sacrifice", talents.grimoire_of_sacrifice->effectN( 2 ).trigger() );

  //legendary buffs
  buffs.stretens_insanity = make_buff( this, "stretens_insanity", find_spell( 208822 ) )
    ->set_default_value( find_spell( 208822 )->effectN( 1 ).percent() )
    ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    ->set_tick_behavior( buff_tick_behavior::NONE );
  buffs.sephuzs_secret =
    make_buff( this, "sephuzs_secret", find_spell( 208052 ) )
    ->add_invalidate(CACHE_HASTE);
  buffs.sephuzs_secret->set_default_value( find_spell( 208052 )->effectN( 2 ).percent() )
    ->set_cooldown( find_spell( 226262 )->duration() );
  buffs.alythesss_pyrogenics = make_buff( this, "alythesss_pyrogenics", find_spell( 205675 ) )
    ->set_default_value( find_spell( 205675 )->effectN( 1 ).percent() )
    ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    ->set_refresh_behavior( buff_refresh_behavior::DURATION );
  buffs.sindorei_spite = make_buff( this, "sindorei_spite", find_spell( 208871 ) )
    ->set_default_value( find_spell( 208871 )->effectN( 1 ).percent() )
    ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.soul_harvest = make_buff( this, "soul_harvest" )
    ->set_default_value( 0.2 )
    ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    ->set_refresh_behavior( buff_refresh_behavior::DURATION )
    ->set_duration( timespan_t::from_seconds( 8 ) );
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
  spec.shadow_bite_2                    = find_specialization_spell( 231799 );
  spec.unending_resolve                 = find_specialization_spell( "Unending Resolve" );
  spec.unending_resolve_2               = find_specialization_spell( 231794 );
  spec.firebolt                         = find_specialization_spell( "Firebolt" );
  spec.firebolt_2                       = find_specialization_spell( 231795 );

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
  
  gains.soulsnatcher                    = get_gain( "soulsnatcher" );
  gains.power_trip                      = get_gain( "power_trip" );
  gains.recurrent_ritual                = get_gain( "recurrent_ritual" );
  gains.feretory_of_souls               = get_gain( "feretory_of_souls" );
  gains.power_cord_of_lethtendris       = get_gain( "power_cord_of_lethtendris" );
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
  procs.power_trip                      = get_proc( "power_trip" );
  procs.soul_conduit                    = get_proc( "soul_conduit" );
  procs.t19_2pc_chaos_bolts             = get_proc( "t19_2pc_chaos_bolt" );
  procs.demonology_t20_2pc              = get_proc( "demonology_t20_2pc" );
  procs.wilfreds_dog                    = get_proc( "wilfreds_dog" );
  procs.wilfreds_imp                    = get_proc( "wilfreds_imp" );
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
      default_pet = "felhunter";
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

void warlock_t::apl_precombat()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action("summon_pet");
  if (specialization() == WARLOCK_DEMONOLOGY)
    precombat->add_action("inner_demons,if=talent.inner_demons.enabled");

  precombat->add_action( "snapshot_stats" );

  if (specialization() != WARLOCK_DEMONOLOGY)
    precombat->add_action("grimoire_of_sacrifice,if=talent.grimoire_of_sacrifice.enabled");
  if (specialization() == WARLOCK_DEMONOLOGY)
    precombat->add_action("demonbolt");

  if ( sim -> allow_potions )
  {
    precombat->add_action( "potion" );
  }
}

std::string warlock_t::default_potion() const
{
  std::string lvl110_potion = "prolonged_power";

  return ( true_level >= 100 ) ? lvl110_potion :
         ( true_level >=  90 ) ? "draenic_intellect" :
         ( true_level >=  85 ) ? "jade_serpent" :
         ( true_level >=  80 ) ? "volcanic" :
                                 "disabled";
}

std::string warlock_t::default_flask() const
{
  return ( true_level >= 100 ) ? "whispered_pact" :
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

  return ( true_level > 100 ) ? lvl110_food :
         ( true_level >  90 ) ? lvl100_food :
                                "disabled";
}

std::string warlock_t::default_rune() const
{
  return ( true_level >= 110 ) ? "defiled" :
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

  if ( warlock_pet_list.active )
    warlock_pet_list.active->init_resources( force );
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
  agony_expression_thing = 0.0;
  agony_accumulator = rng().range( 0.0, 0.99 );
}

void warlock_t::create_options()
{
  player_t::create_options();

  add_option( opt_int( "soul_shards", initial_soul_shards ) );
  add_option( opt_string( "default_pet", default_pet ) );
  add_option( opt_bool( "allow_sephuz", allow_sephuz ) );
}

std::string warlock_t::create_profile( save_e stype )
{
  std::string profile_str = player_t::create_profile( stype );

  if ( stype & SAVE_PLAYER )
  {
    if ( initial_soul_shards != 3 )    profile_str += "soul_shards=" + util::to_string( initial_soul_shards ) + "\n";
    if ( !default_pet.empty() )        profile_str += "default_pet=" + default_pet + "\n";
    if ( allow_sephuz != 0 )           profile_str += "allow_sephuz=" + util::to_string( allow_sephuz ) + "\n";
  }

  return profile_str;
}

void warlock_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  warlock_t* p = debug_cast< warlock_t* >( source );

  initial_soul_shards = p->initial_soul_shards;
  allow_sephuz = p->allow_sephuz;
  default_pet = p->default_pet;
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

expr_t* warlock_t::create_expression( const std::string& name_str )
{
  if ( name_str == "time_to_shard" )
  {
    return make_fn_expr( name_str, [this]()
    { return agony_expression_thing; } );
  }
  else if ( name_str == "pet_count" )
  {
    return make_fn_expr(name_str,
        [this]()
        {
          double t = 0;
          for ( const auto& pet : pet_list )
          {
            if ( auto lock_pet = dynamic_cast<pets::warlock_pet_t*>( pet ) )
            {
              if ( !lock_pet->is_sleeping() )
              {
                t++;
              }
            }
          }
          return t;});
  }
  else if (name_str == "last_cast_imps")
  {
    return create_pet_expression(name_str);
  }
  else if (name_str == "two_cast_imps")
  {
    return create_pet_expression(name_str);
  }
  else if (name_str == "contagion")
  {
    return make_fn_expr(name_str, [this]()
    { 
      timespan_t con = timespan_t::from_millis(0.0);

      auto td = find_target_data(target);
      for (int i = 0; i < MAX_UAS; i++)
      {
        if (!td)
        {
          break;
        }

        timespan_t rem = td->dots_unstable_affliction[i]->remains();

        if (rem > con)
        {
          con = rem;
        }
      }
      return con; 
    });
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

//// Legiondaries
static void stretens_sleepless_shackles( special_effect_t& effect )
{
  warlock_t* s = debug_cast<warlock_t*>( effect.player );
  do_trinket_init( s, WARLOCK_AFFLICTION, s->legendary.stretens_sleepless_shackles, effect );
}
static void hood_of_eternal_disdain( special_effect_t& effect )
{
  warlock_t* s = debug_cast<warlock_t*>( effect.player );
  do_trinket_init( s, WARLOCK_AFFLICTION, s->legendary.hood_of_eternal_disdain, effect );
}
static void power_cord_of_lethtendris( special_effect_t& effect )
{
  warlock_t* s = debug_cast<warlock_t*>( effect.player );
  do_trinket_init( s, WARLOCK_AFFLICTION, s->legendary.power_cord_of_lethtendris, effect );
}
static void sacrolashs_dark_strike( special_effect_t& effect )
{
  warlock_t* s = debug_cast<warlock_t*>( effect.player );
  do_trinket_init( s, WARLOCK_AFFLICTION, s->legendary.sacrolashs_dark_strike, effect );
}
static void reap_and_sow( special_effect_t& effect )
{
  warlock_t* s = debug_cast<warlock_t*>( effect.player );
  do_trinket_init( s, WARLOCK_AFFLICTION, s->legendary.reap_and_sow, effect );
  do_trinket_init( s, WARLOCK_DEMONOLOGY, s->legendary.reap_and_sow, effect );
  do_trinket_init( s, WARLOCK_DESTRUCTION, s->legendary.reap_and_sow, effect );
}
static void wakeners_loyalty( special_effect_t& effect )
{
  warlock_t* s = debug_cast<warlock_t*>( effect.player );
  do_trinket_init( s, WARLOCK_AFFLICTION, s->legendary.wakeners_loyalty, effect );
  do_trinket_init( s, WARLOCK_DEMONOLOGY, s->legendary.wakeners_loyalty, effect );
  do_trinket_init( s, WARLOCK_DESTRUCTION, s->legendary.wakeners_loyalty, effect );
}
static void lessons_of_spacetime( special_effect_t& effect )
{
  warlock_t* s = debug_cast<warlock_t*>( effect.player );
  do_trinket_init( s, WARLOCK_AFFLICTION, s->legendary.lessons_of_spacetime, effect );
  do_trinket_init( s, WARLOCK_DEMONOLOGY, s->legendary.lessons_of_spacetime, effect );
  do_trinket_init( s, WARLOCK_DESTRUCTION, s->legendary.lessons_of_spacetime, effect );
}
static void insignia_of_the_grand_army( special_effect_t& effect )
{
  warlock_t* s = debug_cast<warlock_t*>( effect.player );
  do_trinket_init( s, WARLOCK_AFFLICTION, s->legendary.insignia_of_the_grand_army, effect );
  do_trinket_init( s, WARLOCK_DEMONOLOGY, s->legendary.insignia_of_the_grand_army, effect );
  do_trinket_init( s, WARLOCK_DESTRUCTION, s->legendary.insignia_of_the_grand_army, effect );
}
static void kazzaks_final_curse( special_effect_t& effect )
{
  warlock_t* s = debug_cast<warlock_t*>( effect.player );
  do_trinket_init( s, WARLOCK_DEMONOLOGY, s->legendary.kazzaks_final_curse, effect );
}
static void recurrent_ritual( special_effect_t& effect )
{
  warlock_t* s = debug_cast<warlock_t*>( effect.player );
  do_trinket_init( s, WARLOCK_DEMONOLOGY, s->legendary.recurrent_ritual, effect );
}
static void magistrike_restraints( special_effect_t& effect )
{
  warlock_t* s = debug_cast<warlock_t*>( effect.player );
  do_trinket_init( s, WARLOCK_DESTRUCTION, s->legendary.magistrike_restraints, effect );
}
static void feretory_of_souls( special_effect_t& effect )
{
  warlock_t* s = debug_cast<warlock_t*>( effect.player );
  do_trinket_init( s, WARLOCK_DEMONOLOGY, s->legendary.feretory_of_souls, effect );
  do_trinket_init( s, WARLOCK_DESTRUCTION, s->legendary.feretory_of_souls, effect );
}
static void alythesss_pyrogenics( special_effect_t& effect )
{
  warlock_t* s = debug_cast<warlock_t*>( effect.player );
  do_trinket_init( s, WARLOCK_DESTRUCTION, s->legendary.alythesss_pyrogenics, effect );
}
static void odr_shawl_of_the_ymirjar( special_effect_t& effect )
{
  warlock_t* s = debug_cast<warlock_t*>( effect.player );
  do_trinket_init( s, WARLOCK_DESTRUCTION, s->legendary.odr_shawl_of_the_ymirjar, effect );
}
static void wilfreds_sigil_of_superior_summoning( special_effect_t& effect )
{
  warlock_t* s = debug_cast<warlock_t*>( effect.player );
  do_trinket_init( s, WARLOCK_DEMONOLOGY, s->legendary.wilfreds_sigil_of_superior_summoning, effect );
}
struct sephuzs_secret_t : public unique_gear::scoped_actor_callback_t<warlock_t>
{
  sephuzs_secret_t() : scoped_actor_callback_t( WARLOCK )
  { }

  void manipulate( warlock_t* warlock, const special_effect_t& e ) override
  {
    warlock->legendary.sephuzs_secret = e.driver();
  }
};
static void sindorei_spite( special_effect_t& effect )
{
  warlock_t* s = debug_cast<warlock_t*>( effect.player );
  do_trinket_init( s, WARLOCK_DESTRUCTION, s->legendary.sindorei_spite, effect );
  do_trinket_init( s, WARLOCK_DEMONOLOGY, s->legendary.sindorei_spite, effect );
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
  //  // Legendaries

    unique_gear::register_special_effect( 208821, stretens_sleepless_shackles );
    unique_gear::register_special_effect( 205797, hood_of_eternal_disdain );
    unique_gear::register_special_effect( 205753, power_cord_of_lethtendris );
    unique_gear::register_special_effect( 207592, sacrolashs_dark_strike );
    unique_gear::register_special_effect( 281494, reap_and_sow );
    unique_gear::register_special_effect( 281495, wakeners_loyalty );
    unique_gear::register_special_effect( 281496, lessons_of_spacetime );
    // unique_gear::register_special_effect( 280740, insignia_of_the_grand_army );
    unique_gear::register_special_effect( 214225, kazzaks_final_curse );
    unique_gear::register_special_effect( 205721, recurrent_ritual );
    unique_gear::register_special_effect( 213014, magistrike_restraints );
    unique_gear::register_special_effect( 205702, feretory_of_souls );
    unique_gear::register_special_effect( 205678, alythesss_pyrogenics );
    unique_gear::register_special_effect( 212172, odr_shawl_of_the_ymirjar );
    unique_gear::register_special_effect( 214345, wilfreds_sigil_of_superior_summoning );
    unique_gear::register_special_effect( 208051, sephuzs_secret_t() );
    unique_gear::register_special_effect( 208868, sindorei_spite );
  }

  virtual void register_hotfixes() const override { }

  virtual bool valid() const override { return true; }
  virtual void init( player_t* ) const override { }
  virtual void combat_begin( sim_t* ) const override { }
  virtual void combat_end( sim_t* ) const override { }
};
}

const module_t* module_t::warlock()
{
  static warlock::warlock_module_t m;
  return &m;
}
