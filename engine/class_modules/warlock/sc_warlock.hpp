#pragma once
#include "simulationcraft.hpp"

#include "player/pet_spawner.hpp"
#include "sc_warlock_pets.hpp"

namespace warlock
{
    struct warlock_t;

    constexpr int MAX_UAS = 5;

    struct warlock_td_t : public actor_target_data_t
    {
      propagate_const<dot_t*> dots_drain_life;

      //Aff
      propagate_const<dot_t*> dots_agony;
      propagate_const<dot_t*> dots_corruption;
      propagate_const<dot_t*> dots_seed_of_corruption;
      std::array<propagate_const<dot_t*>, MAX_UAS> dots_unstable_affliction;
      propagate_const<dot_t*> dots_drain_soul;
      propagate_const<dot_t*> dots_siphon_life;
      propagate_const<dot_t*> dots_phantom_singularity;
      propagate_const<dot_t*> dots_vile_taint;

      propagate_const<buff_t*> debuffs_haunt;
      propagate_const<buff_t*> debuffs_shadow_embrace;

      //Destro
      propagate_const<dot_t*> dots_immolate;
      propagate_const<dot_t*> dots_channel_demonfire;
      propagate_const<dot_t*> dots_roaring_blaze;

      propagate_const<buff_t*> debuffs_shadowburn;
      propagate_const<buff_t*> debuffs_eradication;
      propagate_const<buff_t*> debuffs_roaring_blaze;
      propagate_const<buff_t*> debuffs_havoc;
      propagate_const<buff_t*> debuffs_chaotic_flames;

      //Demo
      propagate_const<dot_t*> dots_doom;
      propagate_const<dot_t*> dots_umbral_blaze;

      propagate_const<buff_t*> debuffs_from_the_shadows;
      propagate_const<buff_t*> debuffs_jaws_of_shadow;

      double soc_threshold;

      warlock_t& warlock;
      warlock_td_t( player_t* target, warlock_t& p );

      void reset()
      {
        soc_threshold = 0;
      }

      void target_demise();
    };

    struct warlock_t : public player_t
    {
    public:
      player_t* havoc_target;
      std::vector<action_t*> havoc_spells;  // Used for smarter target cache invalidation.
      bool wracking_brilliance;
      double agony_accumulator;
      std::vector<event_t*> wild_imp_spawns; // Used for tracking incoming imps from HoG 
      double flashpoint_threshold; //Flashpoint (Destruction Azerite trait) does not have the 80% in spell data

      unsigned active_pets;

      // Active Pet
      struct pets_t
      {
        pets::warlock_pet_t* active;
        pets::warlock_pet_t* last;
        static const int INFERNAL_LIMIT = 1;
        static const int DARKGLARE_LIMIT = 1;

        std::array<pets::destruction::infernal_t*, INFERNAL_LIMIT> infernals;

        std::array<pets::affliction::darkglare_t*, DARKGLARE_LIMIT> darkglare;

        spawner::pet_spawner_t<pets::demonology::dreadstalker_t, warlock_t> dreadstalkers;
        spawner::pet_spawner_t<pets::demonology::vilefiend_t, warlock_t> vilefiends;
        spawner::pet_spawner_t<pets::demonology::demonic_tyrant_t, warlock_t> demonic_tyrants;

        spawner::pet_spawner_t<pets::demonology::wild_imp_pet_t, warlock_t> wild_imps;

        spawner::pet_spawner_t<pets::demonology::random_demons::shivarra_t, warlock_t> shivarra;
        spawner::pet_spawner_t<pets::demonology::random_demons::darkhound_t, warlock_t> darkhounds;
        spawner::pet_spawner_t<pets::demonology::random_demons::bilescourge_t, warlock_t> bilescourges;
        spawner::pet_spawner_t<pets::demonology::random_demons::urzul_t, warlock_t> urzuls;
        spawner::pet_spawner_t<pets::demonology::random_demons::void_terror_t, warlock_t> void_terrors;
        spawner::pet_spawner_t<pets::demonology::random_demons::wrathguard_t, warlock_t> wrathguards;
        spawner::pet_spawner_t<pets::demonology::random_demons::vicious_hellhound_t, warlock_t> vicious_hellhounds;
        spawner::pet_spawner_t<pets::demonology::random_demons::illidari_satyr_t, warlock_t> illidari_satyrs;
        spawner::pet_spawner_t<pets::demonology::random_demons::eyes_of_guldan_t, warlock_t> eyes_of_guldan;
        spawner::pet_spawner_t<pets::demonology::random_demons::prince_malchezaar_t, warlock_t> prince_malchezaar;

        pets_t( warlock_t* w );
      } warlock_pet_list;

      std::vector<std::string> pet_name_list;

      struct active_t
      {
        action_t* grimoire_of_sacrifice_proc;
        action_t* chaotic_flames;
        spell_t* pandemic_invocation;
        spell_t* corruption;
        spell_t* roaring_blaze;
        spell_t* internal_combustion;
        spell_t* rain_of_fire;
        spell_t* bilescourge_bombers;
        spell_t* summon_random_demon;
        melee_attack_t* soul_strike;
      } active;

      // Talents
      struct talents_t
      {
        // shared
        // tier 45
        const spell_data_t* demon_skin;
        const spell_data_t* burning_rush;
        const spell_data_t* dark_pact;
        // tier 75
        const spell_data_t* darkfury;
        const spell_data_t* mortal_coil;
        const spell_data_t* demonic_circle;
        // tier 90
        const spell_data_t* grimoire_of_sacrifice; // aff and destro
        // tier 100
        const spell_data_t* soul_conduit;
        // AFF
        const spell_data_t* nightfall;
        const spell_data_t* drain_soul;
        const spell_data_t* haunt;

        const spell_data_t* writhe_in_agony;
        const spell_data_t* absolute_corruption;
        const spell_data_t* siphon_life;

        const spell_data_t* sow_the_seeds;
        const spell_data_t* phantom_singularity;
        const spell_data_t* vile_taint;

        const spell_data_t* shadow_embrace;
        const spell_data_t* deathbolt;
        // grimoire of sacrifice

        // soul conduit
        const spell_data_t* creeping_death;
        const spell_data_t* dark_soul_misery;

        // DEMO
        const spell_data_t* dreadlash;
        const spell_data_t* demonic_strength;
        const spell_data_t* bilescourge_bombers;

        const spell_data_t* demonic_calling;
        const spell_data_t* power_siphon;
        const spell_data_t* doom;

        const spell_data_t* from_the_shadows;
        const spell_data_t* soul_strike;
        const spell_data_t* summon_vilefiend;

        const spell_data_t* inner_demons;
        const spell_data_t* grimoire_felguard;

        const spell_data_t* sacrificed_souls;
        const spell_data_t* demonic_consumption;
        const spell_data_t* nether_portal;
        // DESTRO
        const spell_data_t* flashover;
        const spell_data_t* eradication;
        const spell_data_t* soul_fire;

        const spell_data_t* reverse_entropy;
        const spell_data_t* internal_combustion;
        const spell_data_t* shadowburn;

        const spell_data_t* inferno;
        const spell_data_t* fire_and_brimstone;
        const spell_data_t* cataclysm;

        const spell_data_t* roaring_blaze;
        const spell_data_t* grimoire_of_supremacy;

        const spell_data_t* channel_demonfire;
        const spell_data_t* dark_soul_instability;
      } talents;

      // Azerite traits
      struct azerite_t
      {
        //Shared
        azerite_power_t desperate_power; //healing
        azerite_power_t lifeblood; //healing

        //Demo
        azerite_power_t demonic_meteor;
        azerite_power_t shadows_bite;
        azerite_power_t supreme_commander;
        azerite_power_t umbral_blaze;
        azerite_power_t explosive_potential;
        azerite_power_t baleful_invocation;

        //Aff
        azerite_power_t cascading_calamity;
        azerite_power_t dreadful_calling;
        azerite_power_t inevitable_demise;
        azerite_power_t sudden_onset;
        azerite_power_t wracking_brilliance;
        azerite_power_t pandemic_invocation;

        //Destro
        azerite_power_t bursting_flare;
        azerite_power_t chaotic_inferno;
        azerite_power_t crashing_chaos;
        azerite_power_t rolling_havoc;
        azerite_power_t flashpoint;
        azerite_power_t chaos_shards;
      } azerite;

      // Mastery Spells
      struct mastery_spells_t
      {
        const spell_data_t* potent_afflictions;
        const spell_data_t* master_demonologist;
        const spell_data_t* chaotic_energies;
      } mastery_spells;

      //Procs and RNG
      propagate_const<real_ppm_t*> grimoire_of_sacrifice_rppm; // grimoire of sacrifice

      // Cooldowns
      struct cooldowns_t
      {
        propagate_const<cooldown_t*> haunt;
        propagate_const<cooldown_t*> shadowburn;
        propagate_const<cooldown_t*> soul_fire;
        propagate_const<cooldown_t*> call_dreadstalkers;
        propagate_const<cooldown_t*> deathbolt;
        propagate_const<cooldown_t*> phantom_singularity;
        propagate_const<cooldown_t*> darkglare;
        propagate_const<cooldown_t*> demonic_tyrant;
      } cooldowns;

      // Passives
      struct specs_t
      {
        // All Specs
        const spell_data_t* fel_armor;
        const spell_data_t* nethermancy;

        // Affliction only
        const spell_data_t* affliction;
        const spell_data_t* nightfall;
        const spell_data_t* unstable_affliction;
        const spell_data_t* agony;
        const spell_data_t* agony_2;
        const spell_data_t* shadow_bite;
        const spell_data_t* shadow_bolt; // also demo
        const spell_data_t* summon_darkglare;

        // Demonology only
        const spell_data_t* demonology;
        const spell_data_t* doom;
        const spell_data_t* wild_imps;
        const spell_data_t* demonic_core;

        // Destruction only
        const spell_data_t* destruction;
        const spell_data_t* immolate;
        const spell_data_t* conflagrate;
        const spell_data_t* conflagrate_2; // Conflagrate has 2 charges
        const spell_data_t* havoc;
        const spell_data_t* unending_resolve;
        const spell_data_t* firebolt;
      } spec;

      // Buffs
      struct buffs_t
      {
        propagate_const<buff_t*> demonic_power;
        propagate_const<buff_t*> grimoire_of_sacrifice;

        //affliction buffs
        propagate_const<buff_t*> active_uas;
        propagate_const<buff_t*> drain_life;
        propagate_const<buff_t*> nightfall;
        propagate_const<buff_t*> dark_soul_misery;

        propagate_const<buff_t*> cascading_calamity;
        propagate_const<buff_t*> inevitable_demise;
        propagate_const<buff_t*> wracking_brilliance;

        //demonology buffs
        propagate_const<buff_t*> demonic_core;
        propagate_const<buff_t*> demonic_calling;
        propagate_const<buff_t*> inner_demons;
        propagate_const<buff_t*> nether_portal;
        propagate_const<buff_t*> wild_imps;
        propagate_const<buff_t*> dreadstalkers;
        propagate_const<buff_t*> vilefiend;
        propagate_const<buff_t*> tyrant;
        propagate_const<buff_t*> portal_summons;
        propagate_const<buff_t*> grimoire_felguard;
        propagate_const<buff_t*> prince_malchezaar;
        propagate_const<buff_t*> eyes_of_guldan;

        propagate_const<buff_t*> shadows_bite;
        propagate_const<buff_t*> supreme_commander;
        propagate_const<buff_t*> explosive_potential;

        //destruction_buffs
        propagate_const<buff_t*> backdraft;
        propagate_const<buff_t*> reverse_entropy;
        propagate_const<buff_t*> grimoire_of_supremacy;
        propagate_const<buff_t*> dark_soul_instability;

        propagate_const<buff_t*> bursting_flare;
        propagate_const<buff_t*> chaotic_inferno;
        propagate_const<buff_t*> crashing_chaos;
        propagate_const<buff_t*> rolling_havoc;
        propagate_const<buff_t*> flashpoint;
        propagate_const<buff_t*> chaos_shards;
      } buffs;

      // Gains
      struct gains_t
      {
        gain_t* soul_conduit;

        gain_t* agony;
        gain_t* drain_soul;
        gain_t* seed_of_corruption;
        gain_t* unstable_affliction_refund;
        gain_t* pandemic_invocation;

        gain_t* conflagrate;
        gain_t* shadowburn;
        gain_t* incinerate;
        gain_t* incinerate_crits;
        gain_t* incinerate_fnb;
        gain_t* incinerate_fnb_crits;
        gain_t* immolate;
        gain_t* immolate_crits;
        gain_t* soul_fire;
        gain_t* infernal;
        gain_t* shadowburn_shard;
        gain_t* inferno;
        gain_t* chaos_shards;

        gain_t* miss_refund;

        gain_t* shadow_bolt;
        gain_t* doom;
        gain_t* demonic_meteor;
        gain_t* baleful_invocation;
      } gains;

      // Procs
      struct procs_t
      {
        proc_t* soul_conduit;
        //aff
        proc_t* nightfall;
        //demo
        proc_t* demonic_calling;
        proc_t* souls_consumed;
        proc_t* one_shard_hog;
        proc_t* two_shard_hog;
        proc_t* three_shard_hog;
        proc_t* wild_imp;
        proc_t* fragment_wild_imp;
        proc_t* dreadstalker_debug;
        proc_t* summon_random_demon;
        proc_t* portal_summon;
        //destro
        proc_t* reverse_entropy;
      } procs;

      struct spells_t
      {
        spell_t* melee;
        spell_t* seed_of_corruption_aoe;
        spell_t* implosion_aoe;
      } spells;

      int initial_soul_shards;
      std::string default_pet;
      timespan_t shard_react;


      warlock_t( sim_t* sim, const std::string& name, race_e r );

      // Character Definition
      void      init_spells() override;
      void      init_base_stats() override;
      void      init_scaling() override;
      void      create_buffs() override;
      void      init_gains() override;
      void      init_procs() override;
      void      init_rng() override;
      void      init_action_list() override;
      void      init_resources( bool force ) override;
      void      reset() override;
      void      create_options() override;
      int       get_spawning_imp_count();
      timespan_t time_to_imps(int count);
      int       imps_spawned_during( timespan_t period );
      action_t* create_action( const std::string& name, const std::string& options ) override;
      pet_t*    create_pet( const std::string& name, const std::string& type = std::string() ) override;
      void      create_pets() override;
      std::string      create_profile( save_e ) override;
      void      copy_from( player_t* source ) override;
      resource_e primary_resource() const override { return RESOURCE_MANA; }
      role_e    primary_role() const override { return ROLE_SPELL; }
      stat_e    convert_hybrid_stat( stat_e s ) const override;
      double    matching_gear_multiplier( attribute_e attr ) const override;
      double    composite_player_multiplier( school_e school ) const override;
      double    composite_player_target_multiplier( player_t* target, school_e school ) const override;
      double    composite_player_pet_damage_multiplier( const action_state_t* ) const override;
      double    composite_rating_multiplier( rating_e rating ) const override;
      void      invalidate_cache( cache_e ) override;
      double    composite_spell_crit_chance() const override;
      double    composite_spell_haste() const override;
      double    composite_melee_haste() const override;
      double    composite_melee_crit_chance() const override;
      double    composite_mastery() const override;
      double    resource_gain( resource_e, double, gain_t* = nullptr, action_t* = nullptr ) override;
      double    resource_regen_per_second( resource_e ) const override;
      double    composite_armor() const override;
      void      halt() override;
      void      combat_begin() override;
      void      init_assessors() override;
      expr_t*   create_expression( const std::string& name_str ) override;
      std::string       default_potion() const override;
      std::string       default_flask() const override;
      std::string       default_food() const override;
      std::string       default_rune() const override;


      target_specific_t<warlock_td_t> target_data;

      warlock_td_t* get_target_data( player_t* target ) const override
      {
        warlock_td_t*& td = target_data[target];
        if ( !td )
        {
          td = new warlock_td_t( target, const_cast< warlock_t& >( *this ) );
        }
        return td;
      }

      // sc_warlock_affliction
      action_t* create_action_affliction( const std::string& action_name, const std::string& options_str );
      void create_buffs_affliction();
      void init_spells_affliction();
      void init_gains_affliction();
      void init_rng_affliction();
      void init_procs_affliction();
      void create_options_affliction();
      void create_apl_affliction();
      expr_t*   create_aff_expression(const std::string& name_str);

      // sc_warlock_demonology
      action_t* create_action_demonology( const std::string& action_name, const std::string& options_str );
      void create_buffs_demonology();
      void init_spells_demonology();
      void init_gains_demonology();
      void init_rng_demonology();
      void init_procs_demonology();
      void create_options_demonology();
      void create_apl_demonology();

      // sc_warlock_destruction
      action_t* create_action_destruction( const std::string& action_name, const std::string& options_str );
      void create_buffs_destruction();
      void init_spells_destruction();
      void init_gains_destruction();
      void init_rng_destruction();
      void init_procs_destruction();
      void create_options_destruction();
      void create_apl_destruction();

      // sc_warlock_pets
      pet_t* create_main_pet(const std::string& pet_name, const std::string& options_str);
      pet_t* create_demo_pet(const std::string& pet_name, const std::string& options_str);
      void   create_all_pets();
      expr_t*   create_pet_expression(const std::string& name_str);

    private:
      void apl_precombat();
      void apl_default();
      void apl_global_filler();
    };

    namespace actions
    {
      struct warlock_heal_t
        : public heal_t
      {
        warlock_heal_t( const std::string& n, warlock_t* p, const uint32_t id ) :
          heal_t( n, p, p -> find_spell( id ) )
        {
          target = p;
        }

        warlock_t* p()
        {
          return static_cast<warlock_t*>( player );
        }
        const warlock_t* p() const
        {
          return static_cast<warlock_t*>( player );
        }
      };

      struct warlock_spell_t : public spell_t
      {
      public:
        gain_t * gain;

        warlock_spell_t( warlock_t* p, const std::string& n ) :
          warlock_spell_t( n, p, p -> find_class_spell( n ) )
        {
        }

        warlock_spell_t( warlock_t* p, const std::string& n, specialization_e s ) :
          warlock_spell_t( n, p, p -> find_class_spell( n, s ) )
        {
        }

        warlock_spell_t( const std::string& token, warlock_t* p, const spell_data_t* s = spell_data_t::nil() ) :
          spell_t( token, p, s )
        {
          may_crit = true;
          tick_may_crit = true;
          weapon_multiplier = 0.0;
          gain = player->get_gain( name_str );
        }

        warlock_t* p()
        {
          return static_cast<warlock_t*>( player );
        }
        const warlock_t* p() const
        {
          return static_cast<warlock_t*>( player );
        }

        warlock_td_t* td( player_t* t )
        {
          return p()->get_target_data( t );
        }

        const warlock_td_t* td( player_t* t ) const
        {
          return p()->get_target_data( t );
        }

        void reset() override
        {
          spell_t::reset();
        }

        void init() override
        {
          action_t::init();

          if ( p()->specialization() == WARLOCK_AFFLICTION )
          {
            if ( data().affected_by( p()->spec.affliction->effectN( 1 ) ) )
              base_dd_multiplier *= 1.0 + p()->spec.affliction->effectN( 1 ).percent();

            if ( data().affected_by( p()->spec.affliction->effectN( 2 ) ) )
              base_td_multiplier *= 1.0 + p()->spec.affliction->effectN( 2 ).percent();
          }

          if ( p()->specialization() == WARLOCK_DEMONOLOGY )
          {
            if ( data().affected_by( p()->spec.demonology->effectN( 1 ) ) )
              base_dd_multiplier *= 1.0 + p()->spec.demonology->effectN( 1 ).percent();

            if ( data().affected_by( p()->spec.demonology->effectN( 2 ) ) )
              base_td_multiplier *= 1.0 + p()->spec.demonology->effectN( 2 ).percent();
          }

          if ( p()->specialization() == WARLOCK_DESTRUCTION )
          {
            if ( data().affected_by( p()->spec.destruction->effectN( 1 ) ) )
              base_dd_multiplier *= 1.0 + p()->spec.destruction->effectN( 1 ).percent();

            if ( data().affected_by( p()->spec.destruction->effectN( 2 ) ) )
              base_td_multiplier *= 1.0 + p()->spec.destruction->effectN( 2 ).percent();
          }
        }

        double cost() const override
        {
          double c = spell_t::cost();
          return c;
        }

        void execute() override
        {
          spell_t::execute();

          if ( hit_any_target && result_is_hit( execute_state->result ) && p()->talents.grimoire_of_sacrifice->ok() && p()->buffs.grimoire_of_sacrifice->up() )
          {
            bool procced = p()->grimoire_of_sacrifice_rppm->trigger();
            if ( procced )
            {
              p()->active.grimoire_of_sacrifice_proc->set_target( execute_state->target );
              p()->active.grimoire_of_sacrifice_proc->execute();
            }
          }
        }

        void consume_resource() override
        {
          spell_t::consume_resource();

          if (resource_current == RESOURCE_SOUL_SHARD && p()->in_combat)
          {
            // lets try making all lock specs not react instantly to shard gen
            if (p()->talents.soul_conduit->ok())
            {
              struct sc_event :
                public player_event_t
              {
                gain_t* shard_gain;
                warlock_t* pl;
                int shards_used;

                sc_event(warlock_t* p, int c) :
                  player_event_t(*p, timespan_t::from_millis(100)), shard_gain(p -> gains.soul_conduit), pl(p), shards_used(c) { }

                virtual const char* name() const override
                {
                  return "sc_event";
                }

                virtual void execute() override
                {
                  double soul_conduit_rng = pl->talents.soul_conduit->effectN(1).percent();

                  for (int i = 0; i < shards_used; i++) {
                    if (rng().roll(soul_conduit_rng)) {
                      pl->resource_gain(RESOURCE_SOUL_SHARD, 1.0, pl->gains.soul_conduit);
                      pl->procs.soul_conduit->occur();
                    }
                  }
                }
              };

              make_event<sc_event>(*p()->sim, p(), as<int>(last_resource_cost));
            }
          }
        }

        void impact( action_state_t* s ) override
        {
          spell_t::impact( s );
        }

        double composite_target_multiplier( player_t* t ) const override
        {
          double m = spell_t::composite_target_multiplier(t);
          return m;
        }

        double action_multiplier() const override
        {
          double pm = spell_t::action_multiplier();

          return pm;
        }

        void extend_dot( dot_t* dot, timespan_t extend_duration )
        {
          if ( dot->is_ticking() )
          {
            dot->extend_duration( extend_duration, dot->current_action->dot_duration * 1.5 );
          }
        }

        expr_t* create_expression(const std::string& name_str) override
        {
          if (name_str == "target_uas")
          {
            return make_fn_expr("target_uas", [this]() {
              double uas = 0.0;

              for (int i = 0; i < MAX_UAS; i++)
              {
                uas += td(target)->dots_unstable_affliction[i]->is_ticking();
              }

              return uas;
            });
          }
          else if (name_str == "contagion")
          {
            return make_fn_expr(name_str, [this]()
            {
              timespan_t con = 0_ms;

              for (int i = 0; i < MAX_UAS; i++)
              {
                timespan_t rem = td(target)->dots_unstable_affliction[i]->remains();

                if (rem > con)
                {
                  con = rem;
                }
              }
              return con;
            });
          }

          return spell_t::create_expression(name_str);
        }
      };

      using residual_action_t = residual_action::residual_periodic_action_t<warlock_spell_t>;

      struct summon_pet_t : public warlock_spell_t
      {
        timespan_t summoning_duration;
        std::string pet_name;
        pets::warlock_pet_t* pet;

      private:
        void _init_summon_pet_t()
        {
          util::tokenize( pet_name );
          harmful = false;

          if ( data().ok() &&
            std::find( p()->pet_name_list.begin(), p()->pet_name_list.end(), pet_name ) ==
            p()->pet_name_list.end() )
          {
            p()->pet_name_list.push_back( pet_name );
          }
        }

      public:
        summon_pet_t( const std::string& n, warlock_t* p, const std::string& sname = "" ) :
          warlock_spell_t( p, sname.empty() ? "Summon " + n : sname ),
          summoning_duration( timespan_t::zero() ),
          pet_name( sname.empty() ? n : sname ), pet( nullptr )
        {
          _init_summon_pet_t();
        }

        summon_pet_t( const std::string& n, warlock_t* p, int id ) :
          warlock_spell_t( n, p, p -> find_spell( id ) ),
          summoning_duration( timespan_t::zero() ),
          pet_name( n ), pet( nullptr )
        {
          _init_summon_pet_t();
        }

        summon_pet_t( const std::string& n, warlock_t* p, const spell_data_t* sd ) :
          warlock_spell_t( n, p, sd ),
          summoning_duration( timespan_t::zero() ),
          pet_name( n ), pet( nullptr )
        {
          _init_summon_pet_t();
        }

        void init_finished() override
        {
          pet = debug_cast< pets::warlock_pet_t* >( player->find_pet( pet_name ) );

          warlock_spell_t::init_finished();
        }

        virtual void execute() override
        {
          pet->summon( summoning_duration );

          warlock_spell_t::execute();
        }

        bool ready() override
        {
          if ( !pet )
          {
            return false;
          }
          return warlock_spell_t::ready();
        }
      };

      struct summon_main_pet_t : public summon_pet_t
      {
        cooldown_t* instant_cooldown;

        summon_main_pet_t( const std::string& n, warlock_t* p ) :
          summon_pet_t( n, p ), instant_cooldown( p -> get_cooldown( "instant_summon_pet" ) )
        {
          instant_cooldown->duration = timespan_t::from_seconds( 60 );
          ignore_false_positive = true;
        }

        virtual void schedule_execute( action_state_t* state = nullptr ) override
        {
          warlock_spell_t::schedule_execute( state );

          if ( p()->warlock_pet_list.active )
          {
            p()->warlock_pet_list.active->dismiss();
            p()->warlock_pet_list.active = nullptr;
          }
        }

        virtual bool ready() override {
          if ( p()->warlock_pet_list.active == pet )
            return false;

          return summon_pet_t::ready();
        }

        virtual void execute() override
        {
          summon_pet_t::execute();

          p()->warlock_pet_list.active = p()->warlock_pet_list.last = pet;

          if ( p()->buffs.grimoire_of_sacrifice->check() )
            p()->buffs.grimoire_of_sacrifice->expire();
        }
      };

      //Event for spawning wild imps for Demonology
      //Placed in warlock.cpp for expression purposes
      struct imp_delay_event_t : public player_event_t
      {
        timespan_t diff;

        imp_delay_event_t( warlock_t* p, double delay, double exp ) :
          player_event_t( *p, timespan_t::from_millis( delay ) ) 
        {
          diff = timespan_t::from_millis(exp - delay);
        }

        virtual const char* name() const override
        {
          return  "imp_delay";
        }

        virtual void execute() override
        {
          warlock_t* p = static_cast< warlock_t* >( player() );

          p->warlock_pet_list.wild_imps.spawn();
          expansion::bfa::trigger_leyshocks_grand_compilation( STAT_HASTE_RATING, p );

          //Remove this event from the vector
          auto it = std::find(p->wild_imp_spawns.begin(), p->wild_imp_spawns.end(), this);
          if(it != p->wild_imp_spawns.end())
            p->wild_imp_spawns.erase(it);
        }

        //Used for APL expressions to estimate when imp is "supposed" to spawn
        timespan_t expected_time()
        {
          return std::max(0_ms, this->remains()+diff);
        }
      };
    }

    namespace buffs
    {
      template <typename Base>
      struct warlock_buff_t : public Base
      {
      public:
        typedef warlock_buff_t base_t;
        warlock_buff_t( warlock_td_t& p, const std::string& name, const spell_data_t* s = spell_data_t::nil(), const item_t* item = nullptr ) :
          Base( p, name, s, item ) { }

        warlock_buff_t( warlock_t& p, const std::string& name, const spell_data_t* s = spell_data_t::nil(), const item_t* item = nullptr ) :
          Base( &p, name, s, item ) { }

      protected:
        warlock_t* p()
        {
          return static_cast<warlock_t*>( Base::source );
        }
        const warlock_t* p() const
        {
          return static_cast<warlock_t*>( Base::source );
        }
      };
    }
}
