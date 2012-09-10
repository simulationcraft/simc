// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
//
// TODO:
//  Sooner:
//   * Recheck the fixmes.
//   * Berserker Stance gives 1 rage for 1% max_health damage.
//   * Watch Raging Blow and see if Blizzard fix the bug where it's
//     not refunding 80% of the rage cost if it misses.
//   * Consider testing the rest of the abilities for that too.
//   * Sanity check init_buffs() wrt durations and chances.
//  Later:
//   * Get Heroic Strike to trigger properly "off gcd" using priority.
//   * Move the bleeds out of being warrior_attack_t to stop them
//     triggering effects or having special cases in the class.

//  Protection:
//   * Shield Block: Check whether shield block also gives critical block (as it gives +100% on top of static block value)
//   * OFF GCD for tank CDs
//   * Double Check Defensive Stats with various item builds

// ==========================================================================

namespace { // UNNAMED NAMESPACE
    
    // ==========================================================================
    // Warrior
    // ==========================================================================
    
    struct warrior_t;
    
    enum warrior_stance { STANCE_BATTLE=1, STANCE_BERSERKER, STANCE_DEFENSE=4 };
    
    struct warrior_td_t : public actor_pair_t
    {
        dot_t* dots_bloodbath;
        dot_t* dots_deep_wounds;
        
        buff_t* debuffs_colossus_smash;
        buff_t* debuffs_demoralizing_shout;
        
        warrior_td_t( player_t* target, warrior_t* p );
    };
    
    struct warrior_t : public player_t
    {
    public:
        
        int initial_rage;
        
        // Active
        action_t* active_bloodbath_dot;
        action_t* active_deep_wounds;
        action_t* active_opportunity_strike;
        action_t* active_retaliation;
        warrior_stance  active_stance;
        
        // Buffs
        struct buffs_t
        {
            buff_t* avatar;
            buff_t* battle_stance;
            buff_t* berserker_rage;
            buff_t* berserker_stance;
            buff_t* bloodbath;
            buff_t* bloodsurge;
            buff_t* deadly_calm;
            buff_t* defensive_stance;
            buff_t* enrage;
            buff_t* glyph_overpower;
            buff_t* glyph_hold_the_line;
            buff_t* incite;
            buff_t* last_stand;
            buff_t* meat_cleaver;
            buff_t* overpower;
            buff_t* raging_blow;
            buff_t* raging_wind;
            buff_t* recklessness;
            buff_t* retaliation;
            absorb_buff_t* shield_barrier;
            buff_t* shield_block;
            buff_t* shield_wall;
            
            buff_t* sweeping_strikes;
            buff_t* sword_and_board;
            buff_t* taste_for_blood;
            buff_t* ultimatum;
            
            haste_buff_t* flurry;
            
            //check
            buff_t* rude_interruption;
            buff_t* thunderstruck;
            buff_t* tier13_2pc_tank;
        } buff;
        
        // Cooldowns
        struct cooldowns_t
        {
            cooldown_t* colossus_smash;
            cooldown_t* revenge;
            cooldown_t* shield_slam;
            cooldown_t* strikes_of_opportunity;
            cooldown_t* rage_from_crit_block;
        } cooldown;
        
        // Gains
        struct gains_t
        {
            gain_t* avoided_attacks;
            gain_t* battle_shout;
            gain_t* bloodthirst;
            gain_t* charge;
            gain_t* commanding_shout;
            gain_t* critical_block;
            gain_t* defensive_stance;
            gain_t* enrage;
            gain_t* melee_main_hand;
            gain_t* melee_off_hand;
            gain_t* mortal_strike;
            gain_t* revenge;
            gain_t* shield_slam;
        } gain;
        
        // Glyphs
        struct glyphs_t
        {
            const spell_data_t* colossus_smash;
            const spell_data_t* death_from_above;
            const spell_data_t* furious_sundering;
            const spell_data_t* heavy_repercussions;
            const spell_data_t* hold_the_line;
            const spell_data_t* incite;
            const spell_data_t* overpower;
            const spell_data_t* raging_wind;
            const spell_data_t* recklessness;
            const spell_data_t* shield_wall;
            const spell_data_t* sweeping_strikes;
            const spell_data_t* unending_rage;
        } glyphs;
        
        // Mastery
        struct mastery_t
        {
            const spell_data_t* critical_block;
            const spell_data_t* strikes_of_opportunity;
            const spell_data_t* unshackled_fury;
        } mastery;
        
        // Procs
        struct procs_t
        {
            proc_t* munched_deep_wounds;
            proc_t* rolled_deep_wounds;
            proc_t* parry_haste;
            proc_t* strikes_of_opportunity;
            proc_t* sudden_death;
            proc_t* tier13_4pc_melee;
        } proc;
        
        // Random Number Generation
        struct rngs_t
        {
            rng_t* strikes_of_opportunity;
            rng_t* sudden_death;
            rng_t* taste_for_blood;
        } rng;
        
        // Spec Passives
        struct spec_t
        {
            const spell_data_t* bastion_of_defense;
            const spell_data_t* blood_and_thunder;
            const spell_data_t* bloodsurge;
            const spell_data_t* crazed_berserker;
            const spell_data_t* flurry;
            const spell_data_t* meat_cleaver;
            const spell_data_t* seasoned_soldier;
            const spell_data_t* single_minded_fury;
            const spell_data_t* sudden_death;
            const spell_data_t* sword_and_board;
            const spell_data_t* taste_for_blood;
            const spell_data_t* ultimatum;
            const spell_data_t* unwavering_sentinel;
        } spec;
        
        // Talents
        struct talents_t
        {
            const spell_data_t* juggernaut;
            const spell_data_t* double_time;
            const spell_data_t* warbringer;
            
            const spell_data_t* enraged_regeneration;
            const spell_data_t* second_wind;
            const spell_data_t* impending_victory;
            
            const spell_data_t* staggering_shout;
            const spell_data_t* piercing_howl;
            const spell_data_t* disrupting_shout;
            
            const spell_data_t* bladestorm;
            const spell_data_t* shockwave;
            const spell_data_t* dragon_roar;
            
            const spell_data_t* mass_spell_reflection;
            const spell_data_t* safeguard;
            const spell_data_t* vigilance;
            
            const spell_data_t* avatar;
            const spell_data_t* bloodbath;
            const spell_data_t* storm_bolt;
        } talents;
        
    private:
        target_specific_t<warrior_td_t> target_data;
    public:
        warrior_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
        player_t( sim, WARRIOR, name, r ),
        buff( buffs_t() ),
        cooldown( cooldowns_t() ),
        gain( gains_t() ),
        glyphs( glyphs_t() ),
        mastery( mastery_t() ),
        proc( procs_t() ),
        rng( rngs_t() ),
        spec( spec_t() ),
        talents( talents_t() )
        {
            target_data.init( "target_data", this );
            // Active
            active_bloodbath_dot      = 0;
            active_deep_wounds        = 0;
            active_opportunity_strike = 0;
            active_retaliation        = 0;
            active_stance             = STANCE_BATTLE;
            
            // Cooldowns
            cooldown.colossus_smash         = get_cooldown( "colossus_smash"         );
            cooldown.shield_slam            = get_cooldown( "shield_slam"            );
            cooldown.strikes_of_opportunity = get_cooldown( "strikes_of_opportunity" );
            cooldown.revenge                = get_cooldown( "revenge" );
            cooldown.rage_from_crit_block   = get_cooldown( "rage_from_crit_block" );
            cooldown.rage_from_crit_block -> duration = timespan_t::from_seconds( 3.0 );
            
            initial_rage = 0;
            
            initial.distance = 3;
        }
        
        // Character Definition
        
        virtual warrior_td_t* get_target_data( player_t* target )
        {
            warrior_td_t*& td = target_data[ target ];
            if ( ! td ) td = new warrior_td_t( target, this );
            return td;
        }
        
        virtual void      init_spells();
        virtual void      init_defense();
        virtual void      init_base();
        virtual void      init_scaling();
        virtual void      init_buffs();
        virtual void      init_gains();
        virtual void      init_procs();
        virtual void      init_rng();
        virtual void      init_actions();
        virtual double    resource_gain( resource_e resource_type, double amount, gain_t* g=0, action_t* a=0 );
        virtual void      combat_begin();
        virtual double    composite_player_multiplier( school_e school, action_t* a = NULL );
        virtual double    matching_gear_multiplier( attribute_e attr );
        virtual double    composite_tank_block();
        virtual double    composite_tank_crit_block();
        virtual double    composite_tank_crit( school_e school );
        virtual double    composite_tank_dodge();
        virtual double    composite_attack_speed();
        virtual void      reset();
        virtual void      regen( timespan_t periodicity );
        virtual void      create_options();
        virtual action_t* create_action( const std::string& name, const std::string& options );
        virtual int       decode_set( item_t& );
        virtual resource_e primary_resource() { return RESOURCE_RAGE; }
        virtual role_e primary_role();
        virtual void      assess_damage( school_e, dmg_e, action_state_t* s );
        virtual void      copy_from( player_t* source );
        
        void enrage( action_state_t* s )
        {
            if ( ! ( ( s -> result == RESULT_CRIT && s -> result_amount > 0 ) || s -> result_amount == 0 ) )
                return;
            /* Getting Enrage always adds 1 charge to Raging Blow EXCEPT if the
             Enrage effect is gained by using Berserker Rage AND you already have
             the Enrage buff. Note that using Berserker Rage while not already
             enraged does generate a charge of Raging Blow though.
             
             You don't get the 10 rage when using Berserker Rage if you already had the Enrage buff.
             
             UPDATE: Berserker Rage will grant a charge of Raging Blow and rage even if you are already enraged.
             NOW: This should mean that Crit BT/MS/CS/Block and Berserker Rage give rage, 1 charge of Raging Blow, and refreshes the enrage
             */
            
            if ( specialization() == WARRIOR_FURY )
            {
                buff.raging_blow -> trigger();
            }
            resource_gain( RESOURCE_RAGE, buff.enrage -> data().effectN( 1 ).resource( RESOURCE_RAGE ), gain.enrage );
            buff.enrage -> trigger();
        }
    };
    
    namespace { // UNNAMED NAMESPACE
        
        // Template for common warrior action code. See priest_action_t.
        template <class Base>
        struct warrior_action_t : public Base
        {
            int stancemask;
            
            typedef Base ab; // action base, eg. spell_t
            typedef warrior_action_t base_t;
            
            warrior_action_t( const std::string& n, warrior_t* player,
                             const spell_data_t* s = spell_data_t::nil() ) :
            ab( n, player, s ),
            stancemask( STANCE_BATTLE|STANCE_BERSERKER|STANCE_DEFENSE )
            {
                ab::may_crit   = true;
            }
            
            warrior_t* cast() const { return static_cast<warrior_t*>( ab::player ); }
            
            warrior_td_t* cast_td( player_t* t = 0 ) { return cast() -> get_target_data( t ? t : ab::target ); }
            
            virtual bool ready()
            {
                if ( ! ab::ready() )
                    return false;
                
                // Attack available in current stance?
                if ( ( stancemask & cast() -> active_stance ) == 0 )
                    return false;
                
                return true;
            }
        };
        
        // ==========================================================================
        // Warrior Attack
        // ==========================================================================
        
        struct warrior_attack_t : public warrior_action_t< melee_attack_t >
        {
            warrior_attack_t( const std::string& n, warrior_t* p,
                             const spell_data_t* s = spell_data_t::nil() ) :
            base_t( n, p, s )
            {
                may_crit   = true;
                may_glance = false;
                special     = true;
            }
            
            virtual double armor()
            {
                warrior_td_t* td = cast_td();
                
                double a = base_t::armor();
                
                a *= 1.0 - td -> debuffs_colossus_smash -> value();
                
                return a;
            }
            
            virtual void   consume_resource();
            
            virtual void   execute();
            
            virtual void   impact( action_state_t* s );
            
            virtual double calculate_weapon_damage( double attack_power )
            {
                double dmg = base_t::calculate_weapon_damage( attack_power );
                
                // Catch the case where weapon == 0 so we don't crash/retest below.
                if ( dmg == 0 )
                    return 0;
                
                warrior_t* p = cast();
                
                if ( weapon -> slot == SLOT_OFF_HAND )
                {
                    dmg *= 1.0 + p -> spec.crazed_berserker -> effectN( 2 ).percent();
                    
                    if ( p -> dual_wield() )
                    {
                        if ( p -> main_hand_weapon .group() == WEAPON_1H &&
                            p -> off_hand_weapon .group() == WEAPON_1H )
                            dmg *= 1.0 + p -> spec.single_minded_fury -> effectN( 2 ).percent();
                    }
                }
                
                return dmg;
            }
            
            virtual void   assess_damage( dmg_e, action_state_t* );
            
            virtual double action_multiplier()
            {
                double am = base_t::action_multiplier();
                
                warrior_t* p = cast();
                
                if ( weapon && weapon -> group() == WEAPON_2H )
                    am *= 1.0 + p -> spec.seasoned_soldier -> effectN( 1 ).percent();
                
                // --- Enrages ---
                if ( spell_data_t::is_school( school, SCHOOL_PHYSICAL ) || school == SCHOOL_BLEED )
                {
                    if ( p -> buff.enrage -> up() )
                    {
                        am *= 1.0 + p -> buff.enrage -> data().effectN( 2 ).percent();
                        if ( p -> specialization() == WARRIOR_FURY )
                            am *= 1.0 + p -> composite_mastery() * p -> mastery.unshackled_fury -> effectN( 1 ).mastery_value();
                    }
                }
                
                // --- Passive Talents ---
                
                if ( p -> spec.single_minded_fury -> ok() && p -> dual_wield() )
                {
                    if (  p -> main_hand_weapon .group() == WEAPON_1H &&
                        p -> off_hand_weapon .group() == WEAPON_1H )
                    {
                        am *= 1.0 + p -> spec.single_minded_fury -> effectN( 1 ).percent();
                    }
                }
                
                // --- Buffs / Procs ---
                
                if ( p -> buff.rude_interruption -> up() )
                    am *= 1.05;
                
                return am;
            }
            
            virtual double composite_crit()
            {
                double cc = base_t::composite_crit();
                
                warrior_t* p = cast();
                
                if ( special )
                    cc += p -> buff.recklessness -> value();
                
                return cc;
            }
        };
        
        // trigger_bloodbath ===================================================
        
        struct bloodbath_dot_t : public ignite::pct_based_action_t< attack_t, warrior_t >
        {
            bloodbath_dot_t( warrior_t* p ) :
            base_t( "bloodbath", p, p -> find_spell( 113344 ) )
            {
                dual = true;
                school = SCHOOL_BLEED;//the dot itself is a Bleed effect
                
            }
        };
        
        // Warrior Bloodbath dot template specialization
        template <class WARRIOR_ACTION>
        void trigger_bloodbath_dot( WARRIOR_ACTION* s, player_t* t, double dmg )
        {
            warrior_t* p = s -> cast();
            if ( ! p -> buff.bloodbath -> up() ) return;
            
            ignite::trigger_pct_based(
                                      p -> active_bloodbath_dot, // ignite spell
                                      t, // target
                                      p -> buff.bloodbath -> data().effectN( 1 ).percent() * dmg );
        }
        
        // ==========================================================================
        // Static Functions
        // ==========================================================================
        
        // trigger_rage_gain ========================================================
        
        static void trigger_rage_gain( warrior_attack_t* a )
        {
            // MoP: base rage gain is 1.75 * weaponspeed and half that for off-hand
            // Battle stance: +100%
            // Berserker stance: no change
            // Defensive stance: -100%
            
            if ( a -> proc )
                return;
            
            warrior_t* p = a -> cast();
            weapon_t*  w = a -> weapon;
            
            if ( p -> active_stance == STANCE_DEFENSE )
            {
                return;
            }
            double rage_gain = 1.75 * w -> swing_time.total_seconds();
            
            if ( p -> active_stance == STANCE_BATTLE )
            {
                rage_gain *= 1.0 + p -> buff.battle_stance -> data().effectN( 1 ).percent();
            }
            else   if ( p -> active_stance == STANCE_DEFENSE )
            {
                rage_gain *= 1.0 + p -> buff.defensive_stance -> data().effectN( 3 ).percent();
            }
            
            if ( w -> slot == SLOT_OFF_HAND )
                rage_gain /= 2.0;
            
            rage_gain = floor( rage_gain * 10 ) / 10.0;
            
            p -> resource_gain( RESOURCE_RAGE,
                               rage_gain,
                               w -> slot == SLOT_OFF_HAND ? p -> gain.melee_off_hand : p -> gain.melee_main_hand );
        }
        
        // trigger_retaliation ======================================================
        
        static void trigger_retaliation( warrior_t* p, int school, int result )
        {
            if ( ! p -> buff.retaliation -> up() )
                return;
            
            if ( school != SCHOOL_PHYSICAL )
                return;
            
            if ( ! ( result == RESULT_HIT || result == RESULT_CRIT || result == RESULT_BLOCK ) )
                return;
            
            if ( ! p -> active_retaliation )
            {
                struct retaliation_strike_t : public warrior_attack_t
                {
                    retaliation_strike_t( warrior_t* p ) :
                    warrior_attack_t( "retaliation_strike", p )
                    {
                        background = true;
                        proc = true;
                        trigger_gcd = timespan_t::zero();
                        weapon = &( p -> main_hand_weapon );
                        weapon_multiplier = 1.0;
                        
                        init();
                    }
                };
                
                p -> active_retaliation = new retaliation_strike_t( p );
            }
            
            p -> active_retaliation -> execute();
            p -> buff.retaliation -> decrement();
        }
        
        // trigger_strikes_of_opportunity ===========================================
        
        struct opportunity_strike_t : public warrior_attack_t
        {
            opportunity_strike_t( warrior_t* p ) :
            warrior_attack_t( "opportunity_strike", p, p -> find_spell( 76858 ) )
            {
                background = true;
            }
        };
        
        static void trigger_strikes_of_opportunity( warrior_attack_t* a )
        {
            if ( a -> proc )
                return;
            
            warrior_t* p = a -> cast();
            
            if ( ! p -> mastery.strikes_of_opportunity -> ok() )
                return;
            
            if ( p -> cooldown.strikes_of_opportunity -> remains() > timespan_t::zero() )
                return;
            
            double chance = p -> composite_mastery() * p -> mastery.strikes_of_opportunity -> effectN( 2 ).percent() / 100.0;
            
            if ( ! p -> rng.strikes_of_opportunity -> roll( chance ) )
                return;
            
            p -> cooldown.strikes_of_opportunity -> start( timespan_t::from_seconds( 0.5 ) );
            
            assert( p -> active_opportunity_strike );
            
            if ( p -> sim -> debug )
                p -> sim -> output( "Opportunity Strike procced from %s", a -> name() );
            
            
            p -> proc.strikes_of_opportunity -> occur();
            p -> active_opportunity_strike -> execute();
        }
        
        // trigger_sudden_death =====================================================
        
        static void trigger_sudden_death( warrior_attack_t* a )
        {
            warrior_t* p = a -> cast();
            
            if ( a -> proc )
                return;
            
            if ( p -> rng.sudden_death -> roll ( p -> spec.sudden_death -> proc_chance() ) )
            {
                p -> cooldown.colossus_smash -> reset();
                p -> proc.sudden_death       -> occur();
            }
        }
        
        // trigger_flurry ===========================================================
        
        static void trigger_flurry( warrior_attack_t* a, int stacks )
        {
            warrior_t* p = a -> cast();
            
            if ( stacks >= 0 )
                p -> buff.flurry -> trigger( stacks );
            else
                p -> buff.flurry -> decrement();
        }
        
        // ==========================================================================
        // Warrior Attacks
        // ==========================================================================
        
        // warrior_attack_t::assess_damage ==========================================
        
        void warrior_attack_t::assess_damage( dmg_e dmg_type, action_state_t* s )
        {
            base_t::assess_damage( dmg_type, s );
            
            /* warrior_t* p = cast();
             
             if ( t -> is_enemy() )
             {
             target_t* q =  t -> cast_target();
             
             if ( p -> buff.sweeping_strikes -> up() && q -> adds_nearby )
             {
             attack_t::additional_damage( q, amount, dmg_e, impact_result );
             }
             }*/
        }
        
        // warrior_attack_t::consume_resource =======================================
        
        void warrior_attack_t::consume_resource()
        {
            base_t::consume_resource();
            warrior_t* p = cast();
            
            if ( proc )
                return;
            
            // Warrior attacks (non-AoE) which are are avoided by the target consume only 20%
            if ( resource_consumed > 0 && ! aoe && result_is_miss() )
            {
                double rage_restored = resource_consumed * 0.80;
                p -> resource_gain( RESOURCE_RAGE, rage_restored, p -> gain.avoided_attacks );
            }
        }
        
        // warrior_attack_t::execute ================================================
        
        void warrior_attack_t::execute()
        {
            base_t::execute();
            warrior_t* p = cast();
            
            if ( proc ) return;
            
            if ( result_is_hit( execute_state -> result ) )
            {
                trigger_strikes_of_opportunity( this );
            }
            else if ( result == RESULT_DODGE  )
            {
                p -> buff.overpower -> trigger();
            }
        }
        
        // warrior_attack_t::impact =================================================
        
        void warrior_attack_t::impact( action_state_t* s )
        {
            base_t::impact( s );
            
            if ( result_is_hit( s -> result ) && ! proc )
            {
                if ( special )
                {
                    trigger_bloodbath_dot( this, s -> target, s -> result_amount );
                }
                
                trigger_flurry( this, 3 );
            }
        }
        
        // Melee Attack =============================================================
        
        struct melee_t : public warrior_attack_t
        {
            int sync_weapons;
            
            melee_t( const std::string& name, warrior_t* p, int sw ) :
            warrior_attack_t( name, p, spell_data_t::nil() ),
            sync_weapons( sw )
            {
                school          = SCHOOL_PHYSICAL;
                may_glance      = true;
                special         = false;
                background      = true;
                repeating       = true;
                trigger_gcd     = timespan_t::zero();
                
                if ( p -> dual_wield() ) base_hit -= 0.19;
            }
            
            virtual timespan_t execute_time()
            {
                timespan_t t = warrior_attack_t::execute_time();
                
                if ( player -> in_combat )
                    return t;
                
                if ( weapon -> slot == SLOT_MAIN_HAND || sync_weapons )
                    return timespan_t::from_seconds( 0.02 );
                
                // Before combat begins, unless we are under sync_weapons the OH is
                // delayed by half its swing time.
                
                return timespan_t::from_seconds( 0.02 ) + t / 2;
            }
            
            virtual void execute()
            {
                // Be careful changing where this is done.  Flurry that procs from melee
                // must be applied before the (repeating) event schedule, and the decrement
                // here must be done before it.
                trigger_flurry( this, -1 );
                
                warrior_attack_t::execute();
                
            }
            
            virtual void impact( action_state_t* s )
            {
                warrior_attack_t::impact( s );
                
                if ( result_is_hit( s -> result ) )
                {
                    trigger_sudden_death( this );
                }
                // Any attack that hits or is dodged/blocked/parried generates rage
                if ( s -> result != RESULT_MISS )
                {
                    trigger_rage_gain( this );
                }
            }
            
            virtual double action_multiplier()
            {
                double am = warrior_attack_t::action_multiplier();
                
                warrior_t* p = cast();
                
                if ( p -> specialization() == WARRIOR_FURY )
                    am *= 1.0 + p -> spec.crazed_berserker -> effectN( 3 ).percent();
                
                return am;
            }
        };
        
        // Auto Attack ==============================================================
        
        struct auto_attack_t : public warrior_attack_t
        {
            int sync_weapons;
            
            auto_attack_t( warrior_t* p, const std::string& options_str ) :
            warrior_attack_t( "auto_attack", p ), sync_weapons( 0 )
            {
                option_t options[] =
                {
                    { "sync_weapons", OPT_BOOL, &sync_weapons },
                    { NULL, OPT_UNKNOWN, NULL }
                };
                parse_options( options, options_str );
                
                assert( p -> main_hand_weapon.type != WEAPON_NONE );
                
                p -> main_hand_attack = new melee_t( "melee_main_hand", p, sync_weapons );
                p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
                p -> main_hand_attack -> base_execute_time = p -> main_hand_weapon.swing_time;
                
                if ( p -> off_hand_weapon.type != WEAPON_NONE )
                {
                    p -> off_hand_attack = new melee_t( "melee_off_hand", p, sync_weapons );
                    p -> off_hand_attack -> weapon = &( p -> off_hand_weapon );
                    p -> off_hand_attack -> base_execute_time = p -> off_hand_weapon.swing_time;
                    p -> off_hand_attack -> id = 1;
                }
                
                trigger_gcd = timespan_t::zero();
            }
            
            virtual void execute()
            {
                warrior_t* p = cast();
                
                p -> main_hand_attack -> schedule_execute();
                
                if ( p -> off_hand_attack )
                    p -> off_hand_attack -> schedule_execute();
            }
            
            virtual bool ready()
            {
                warrior_t* p = cast();
                
                if ( p -> is_moving() )
                    return false;
                
                return( p -> main_hand_attack -> execute_event == 0 ); // not swinging
            }
        };
        
        // Bladestorm ===============================================================
        
        struct bladestorm_tick_t : public warrior_attack_t
        {
            bladestorm_tick_t( warrior_t* p, const char* name ) :
            warrior_attack_t( name, p, p -> find_spell( 50622 ) )
            {
                background  = true;
                direct_tick = true;
                aoe         = -1;
            }
        };
        
        struct bladestorm_t : public warrior_attack_t
        {
            attack_t* bladestorm_mh;
            attack_t* bladestorm_oh;
            
            bladestorm_t( warrior_t* p, const std::string& options_str ) :
            warrior_attack_t( "bladestorm", p, p -> find_talent_spell( "Bladestorm" ) ),
            bladestorm_mh( 0 ), bladestorm_oh( 0 )
            {
                parse_options( NULL, options_str );
                
                aoe       = -1;
                harmful   = false;
                channeled = true;
                tick_zero = true;
                
                bladestorm_mh = new bladestorm_tick_t( p, "bladestorm_mh" );
                bladestorm_mh -> weapon = &( player -> main_hand_weapon );
                add_child( bladestorm_mh );
                
                if ( player -> off_hand_weapon.type != WEAPON_NONE )
                {
                    bladestorm_oh = new bladestorm_tick_t( p, "bladestorm_oh" );
                    bladestorm_oh -> weapon = &( player -> off_hand_weapon );
                    add_child( bladestorm_oh );
                }
            }
            
            virtual void execute()
            {
                warrior_attack_t::execute();
                
                warrior_t* p = cast();
                
                if ( p -> main_hand_attack )
                    p -> main_hand_attack -> cancel();
                
                if ( p ->  off_hand_attack )
                    p -> off_hand_attack -> cancel();
            }
            
            virtual void tick( dot_t* d )
            {
                warrior_attack_t::tick( d );
                
                bladestorm_mh -> execute();
                
                if ( bladestorm_mh -> result_is_hit( execute_state -> result ) && bladestorm_oh )
                {
                    bladestorm_oh -> execute();
                }
            }
            
            // Bladestorm is not modified by haste effects
            virtual double composite_haste() { return 1.0; }
            virtual double swing_haste() { return 1.0; }
        };
        
        // Bloodthirst Heal ==============================================================
        
        struct bloodthirst_heal_t : public heal_t
        {
            bloodthirst_heal_t( warrior_t* p ) :
            heal_t( "bloodthirst_heal", p, p -> find_class_spell( "Bloodthirst" ) )
            {
                // Add Field Dressing Talent, warrior heal etc.
                
                // Implemented as an actual heal because of spell callbacks ( for Hurricane, etc. )
                background= true;
                target = p;
            }
            
            virtual resource_e current_resource() { return RESOURCE_NONE; }
        };
        
        // Bloodthirst ==============================================================
        
        struct bloodthirst_t : public warrior_attack_t
        {
            bloodthirst_heal_t* bloodthirst_heal;
            
            bloodthirst_t( warrior_t* p, const std::string& options_str ) :
            warrior_attack_t( "bloodthirst", p, p -> find_class_spell( "Bloodthirst" ) ),
            bloodthirst_heal( NULL )
            {
                parse_options( NULL, options_str );
                
                weapon             = &( p -> main_hand_weapon );
                bloodthirst_heal   = new bloodthirst_heal_t( p );
                
                base_multiplier += p -> sets -> set( SET_T14_2PC_MELEE ) -> effectN( 2 ).percent();
            }
            
            virtual double composite_crit()
            {
              double c = warrior_attack_t::composite_crit();

              c *= 2.0;

              return c;
            }
            
            virtual void execute()
            {
                warrior_attack_t::execute();
                
                if ( result_is_hit( execute_state -> result ) )
                {
                    warrior_t* p = cast();
                    
                    if ( p -> set_bonus.tier13_4pc_melee() && sim -> roll( p -> sets -> set( SET_T13_4PC_MELEE ) -> effectN( 1 ).percent() ) )
                    {
                        warrior_td_t* td = cast_td();
                        td -> debuffs_colossus_smash -> trigger();
                        p -> proc.tier13_4pc_melee -> occur();
                    }
                }
            }
            
            virtual void impact( action_state_t* s )
            {
                warrior_attack_t::impact( s );
                
                warrior_t* p = cast();
                
                if ( result_is_hit( s -> result ) )
                {
                    if ( bloodthirst_heal )
                        bloodthirst_heal -> execute();
                    
                    p -> active_deep_wounds -> execute();
                    p -> buff.bloodsurge -> trigger( 3 );
                    p -> resource_gain( RESOURCE_RAGE, data().effectN( 3 ).resource( RESOURCE_RAGE ),
                                       p -> gain.bloodthirst );
                    p -> enrage( s );
                }
            }
        };
        
        // Charge ===================================================================
        
        struct charge_t : public warrior_attack_t
        {
            int use_in_combat;
            
            charge_t( warrior_t* p, const std::string& options_str ) :
            warrior_attack_t( "charge", p, p -> find_class_spell( "Charge" ) ),
            use_in_combat( 0 ) // For now it's not usable in combat by default because we can't model the distance/movement.
            {
                option_t options[] =
                {
                    { "use_in_combat", OPT_BOOL, &use_in_combat },
                    { NULL, OPT_UNKNOWN, NULL }
                };
                parse_options( options, options_str );
                
                cooldown -> duration += p -> talents.juggernaut -> effectN( 3 ).time_value();
            }
            
            virtual void execute()
            {
                warrior_attack_t::execute();
                warrior_t* p = cast();
                
                if ( p -> position == POSITION_RANGED_FRONT )
                    p -> position = POSITION_FRONT;
                else if ( ( p -> position == POSITION_RANGED_BACK ) || ( p -> position == POSITION_MAX ) )
                    p -> position = POSITION_BACK;
                
                p -> resource_gain( RESOURCE_RAGE,
                                   data().effectN( 2 ).resource( RESOURCE_RAGE ),
                                   p -> gain.charge );
            }
            
            virtual bool ready()
            {
                warrior_t* p = cast();
                
                if ( p -> in_combat )
                {
                    // FIXME:
                    /* if ( ! ( p -> talents.juggernaut -> rank() || p -> talents.warbringer -> rank() ) )
                     return false;
                     
                     else */if ( ! use_in_combat )
                         return false;
                    
                    if ( ( p -> position == POSITION_BACK ) || ( p -> position == POSITION_FRONT ) )
                    {
                        return false;
                    }
                }
                
                return warrior_attack_t::ready();
            }
        };
        
        // Cleave ===================================================================
        
        struct cleave_t : public warrior_attack_t
        {
            cleave_t( warrior_t* p, const std::string& options_str ) :
            warrior_attack_t( "cleave", p, p -> find_class_spell( "Cleave" ) )
            {
                parse_options( NULL, options_str );
                
                weapon = &( player -> main_hand_weapon );
                
                aoe = 1;
                
                normalize_weapon_speed = false;
            }
            
            virtual double cost()
            {
                double c = warrior_attack_t::cost();
                warrior_t* p = cast();
                
                // Needs testing
                if ( p -> buff.deadly_calm -> check() )
                    c += p -> buff.deadly_calm -> data().effectN( 1 ).resource( RESOURCE_RAGE );
                
                if ( p -> buff.incite -> check() )
                    c += p -> buff.incite  -> data().effectN( 1 ).resource( RESOURCE_RAGE );
                
                
                if ( p -> buff.ultimatum -> check() )
                    c*= 1+ p->buff.ultimatum ->data().effectN(1).percent();
                return c;
            }
            
            virtual double action_multiplier()
            {
                double am = warrior_attack_t::action_multiplier();
                
                warrior_t* p = cast();
                
                am *= 1.0 + p -> buff.taste_for_blood -> data().effectN( 1 ).percent() * p -> buff.taste_for_blood -> stack();
                
                return am;
            }
            
            virtual void execute()
            {
                warrior_t* p = cast();
                p -> buff.deadly_calm -> up();
                warrior_attack_t::execute();
                p -> buff.deadly_calm -> decrement();
                p -> buff.taste_for_blood -> expire();
                if ( p -> buff.ultimatum -> check() )
                    p-> buff.ultimatum -> expire();
                
            }
            virtual void impact( action_state_t* s )
            {
                warrior_attack_t::impact( s );
                
                if ( result_is_hit( s -> result ) )
                {
                    warrior_t* p = cast();
                    p -> buff.glyph_overpower -> trigger();
                }
            }
        };
        
        // Colossus Smash ===========================================================
        
        struct colossus_smash_t : public warrior_attack_t
        {
            colossus_smash_t( warrior_t* p, const std::string& options_str ) :
            warrior_attack_t( "colossus_smash",  p, p -> find_class_spell( "Colossus Smash" ) )
            {
                parse_options( NULL, options_str );
                
                weapon = &( player -> main_hand_weapon );
            }
            
            virtual timespan_t travel_time()
            {
                // Dirty hack to ensure you can fit four executes into the colossus smash window, like you can in-game
                return timespan_t::from_millis( 1 );
            }
            
            virtual void impact( action_state_t* s )
            {
                warrior_attack_t::impact( s );
                
                if ( result_is_hit( s -> result ) )
                {
                    warrior_t* p = cast();
                    warrior_td_t* td = cast_td( s -> target );
                    td -> debuffs_colossus_smash -> trigger( 1, data().effectN( 2 ).percent() );
                    
                    if ( ! sim -> overrides.physical_vulnerability )
                        s -> target -> debuffs.physical_vulnerability -> trigger();
                    
                    if ( p -> glyphs.colossus_smash -> ok() && ! sim -> overrides.weakened_armor )
                        s -> target -> debuffs.weakened_armor -> trigger();
                    
                    p -> enrage( s );
                }
            }
        };
        
        // Concussion Blow ==========================================================
        
        struct concussion_blow_t : public warrior_attack_t
        {
            concussion_blow_t( warrior_t* p, const std::string& options_str ) :
            warrior_attack_t( "concussion_blow", p, p -> find_class_spell( "Concussion Blow" ) )
            {
                // FIXME:
                // check_talent( p -> talents.concussion_blow -> rank() );
                
                parse_options( NULL, options_str );
                
                direct_power_mod  = data().effectN( 3 ).percent();
            }
        };
        
        // Deep Wounds ==============================================================
        
        struct deep_wounds_t : public warrior_attack_t
        {
            deep_wounds_t( warrior_t* p ) :
            warrior_attack_t( "deep_wounds", p, p -> find_spell( 115767 ) )
            {
                background = true;
                proc = true;
                tick_may_crit = true;
                may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
                tick_power_mod = data().extra_coeff();
                dot_behavior = DOT_REFRESH;
            }
        };
        // Demoralizing Shout =========================================================

        struct demoralizing_shout : public warrior_attack_t
        {
            demoralizing_shout( warrior_t* p, const std::string& options_str ) :
            warrior_attack_t( "demoralizing_shout", p, p -> find_class_spell( "Demoralizing Shout" ) )
            {
                parse_options( NULL, options_str );
                harmful=false;
                
            }
            virtual void impact( action_state_t* s )
            {
                warrior_attack_t::impact( s );
                if ( result_is_hit( s -> result ) )
                {
                    warrior_td_t* td = cast_td( s -> target );
                    
                    td -> debuffs_demoralizing_shout ->trigger( 1,data().effectN(1).percent());
                }
            }
            
        };
        // Devastate ================================================================
        
        struct devastate_t : public warrior_attack_t
        {
            devastate_t( warrior_t* p, const std::string& options_str ) :
            warrior_attack_t( "devastate", p, p -> find_class_spell( "Devastate" ) )
            {
                parse_options( NULL, options_str );
            }
            
            virtual void execute()
            {
                warrior_attack_t::execute();
                
                warrior_t* p = cast();
                
                if ( result_is_hit( execute_state -> result ) )
                {
                    if ( p -> buff.sword_and_board -> trigger() )
                    {
                        p -> cooldown.shield_slam -> reset();
                    }
                }
                
                if ( p -> buff.deadly_calm -> check() )
                    p -> buff.incite -> trigger();
                
                p -> active_deep_wounds -> execute();
            }
            
            virtual void impact( action_state_t* s )
            {
                warrior_attack_t::impact( s );
                
                if ( ! sim -> overrides.weakened_blows )
                    s -> target -> debuffs.weakened_blows -> trigger();
            }
        };
        
        // Dragon Roar ==============================================================
        
        struct dragon_roar_t : public warrior_attack_t
        {
            dragon_roar_t( warrior_t* p, const std::string& options_str ) :
            warrior_attack_t( "dragon_roar", p, p -> find_talent_spell( "Dragon Roar" ) )
            {
                parse_options( NULL, options_str );
                aoe = -1;
                direct_power_mod = data().extra_coeff();
                
                //lets us benefit from seasoned_soldier, etc. but do not add weapon damage to it
                weapon             = &( p -> main_hand_weapon );
                weapon_multiplier = 0;
                
            }
            
            virtual double armor()
            { return 0; }

            virtual double crit_chance( double /* crit */, int /* delta_level */ )
            {
              // Dragon Roar always crits
              return 1.0;
            }
        };
        
        // Execute ==================================================================
        
        struct execute_t : public warrior_attack_t
        {
            execute_t( warrior_t* p, const std::string& options_str ) :
            warrior_attack_t( "execute", p, p -> find_class_spell( "Execute" ) )
            {
                parse_options( NULL, options_str );
                
                // Include the weapon so we benefit from racials
                weapon             = &( player -> main_hand_weapon );
                weapon_multiplier  = 0;
                direct_power_mod   = data().extra_coeff();
            }
            
            virtual bool ready()
            {
                if ( target -> health_percentage() > 20 )
                    return false;
                
                return warrior_attack_t::ready();
            }
        };
        
        // Heroic Strike ============================================================
        
        struct heroic_strike_t : public warrior_attack_t
        {
            heroic_strike_t( warrior_t* p, const std::string& options_str ) :
            warrior_attack_t( "heroic_strike", p, p -> find_class_spell( "Heroic Strike" ) )
            {
                parse_options( NULL, options_str );
                
                weapon = &( player -> main_hand_weapon );
                harmful           = true;
                
                // FIX ME: The 140% seems to be hardcoded in the tooltip
                if ( weapon -> group() == WEAPON_1H ||
                    weapon -> group() == WEAPON_SMALL )
                    base_multiplier *= 1.40;
            }
            
            virtual double cost()
            {
                double c = warrior_attack_t::cost();
                warrior_t* p = cast();
                
                // Needs testing
                if ( p -> set_bonus.tier13_2pc_melee() )
                    c -= 5.0;
                
                if ( p -> buff.deadly_calm -> check() )
                    c += p -> buff.deadly_calm -> data().effectN( 1 ).resource( RESOURCE_RAGE );
                
                if ( p -> buff.incite -> check() )
                    c += p -> buff.incite  -> data().effectN( 1 ).resource( RESOURCE_RAGE );
                
                if ( p -> buff.ultimatum -> check() )
                    c*= 1+ p->buff.ultimatum ->data().effectN(1).percent();
                return c;
            }
            
            virtual double action_multiplier()
            {
                double am = warrior_attack_t::action_multiplier();
                
                warrior_t* p = cast();
                
                am *= 1.0 + p -> buff.taste_for_blood -> data().effectN( 1 ).percent() * p -> buff.taste_for_blood -> stack();
                
                return am;
            }
            
            
            virtual void execute()
            {
                warrior_t* p = cast();
                p -> buff.deadly_calm -> up();
                
                warrior_attack_t::execute();
                
                //p -> buff.glyph_of_incite -> expire();
                
                p -> buff.deadly_calm -> decrement();
                p -> buff.taste_for_blood -> expire();
                if ( p -> buff.ultimatum -> check() )
                    p-> buff.ultimatum -> expire();
            }
        };
        
        // Heroic Trow ==============================================================
        
        struct heroic_throw_t : public warrior_attack_t
        {
            heroic_throw_t( warrior_t* p, const std::string& options_str ) :
            warrior_attack_t( "heroic_throw", p, p -> find_class_spell( "Heroic Throw" ) )
            {
                parse_options( NULL, options_str );
            }
        };
        
        // Heroic Leap ==============================================================
        
        struct heroic_leap_t : public warrior_attack_t
        {
            heroic_leap_t( warrior_t* p, const std::string& options_str ) :
            warrior_attack_t( "heroic_leap", p, p -> find_class_spell( "Heroic Leap" ) )
            {
                parse_options( NULL, options_str );
                
                aoe = -1;
                may_dodge = may_parry = false;
                harmful = true; // This should be defaulted to true, but it's not
                
                // Damage is stored in a trigger spell
                const spell_data_t* dmg_spell = p -> dbc.spell( 52174 );//data().effectN( 3 ).trigger_spell_id() does not resolve to 52174 anymore
                base_dd_min = p -> dbc.effect_min( dmg_spell -> effectN( 1 ).id(), p -> level );
                base_dd_max = p -> dbc.effect_max( dmg_spell -> effectN( 1 ).id(), p -> level );
                direct_power_mod = dmg_spell -> extra_coeff();
                
                // Heroic Leap can trigger procs from either weapon
                proc_ignores_slot = true;
                
                if ( p -> glyphs.death_from_above ->ok() ) //decreases cd and increases dmg
                {
                    cooldown->duration+= p->glyphs.death_from_above ->effectN( 1 ).time_value();
                    base_multiplier+=p->glyphs.death_from_above ->effectN( 2 ).percent();
                }
            }
        };
        
        // Impending Victory ========================================================
        
        struct impending_victory_heal_t : public heal_t
        {
            impending_victory_heal_t( warrior_t* p ) :
            heal_t( "impending_victory__heal", p, p -> find_spell( 118340 ) )
            {
                // Implemented as an actual heal because of spell callbacks ( for Hurricane, etc. )
                background = true;
                target = p;
            }
            
            virtual resource_e current_resource() { return RESOURCE_NONE; }
        };
        
        
        struct impending_victory_t : public warrior_attack_t
        {
            impending_victory_heal_t* impending_victory_heal;
            
            impending_victory_t( warrior_t* p, const std::string& options_str ) :
            warrior_attack_t( "impending_victory", p, p -> talents.impending_victory ),
            impending_victory_heal( NULL )
            {
                parse_options( NULL, options_str );
                
                weapon                 = &( player -> main_hand_weapon );
                impending_victory_heal = new impending_victory_heal_t( p );
                assert( impending_victory_heal );
            }
            
            virtual void execute()
            {
                warrior_attack_t::execute();
                
                if ( result_is_hit( execute_state -> result ) )
                {
                    impending_victory_heal -> execute();
                }
            }
        };
        
        // Mortal Strike ============================================================
        
        struct mortal_strike_t : public warrior_attack_t
        {
            mortal_strike_t( warrior_t* p, const std::string& options_str ) :
            warrior_attack_t( "mortal_strike", p, p -> find_class_spell( "Mortal Strike" ) )
            {
                parse_options( NULL, options_str );
                base_multiplier += p -> sets -> set( SET_T14_2PC_MELEE ) -> effectN( 1 ).percent();
            }
            
            virtual void execute()
            {
                warrior_attack_t::execute();
                
                warrior_t* p = cast();
                
                if ( result_is_hit( execute_state -> result ) )
                {
                    warrior_td_t* td = cast_td();
                    
                    if ( p -> set_bonus.tier13_4pc_melee() && sim -> roll( p -> sets -> set( SET_T13_4PC_MELEE ) -> proc_chance() ) )
                    {
                        td -> debuffs_colossus_smash -> trigger();
                        p -> proc.tier13_4pc_melee -> occur();
                    }
                    
                    p -> active_deep_wounds -> execute();
                    
                    p -> buff.overpower -> trigger();
                }
            }
            
            virtual void impact( action_state_t* s )
            {
                warrior_attack_t::impact( s );
                
                warrior_t* p = cast();
                
                if ( sim -> overrides.mortal_wounds && result_is_hit( s -> result ) )
                    s -> target -> debuffs.mortal_wounds -> trigger();
                
                p -> resource_gain( RESOURCE_RAGE,
                                   data().effectN( 4 ).resource( RESOURCE_RAGE ),
                                   p -> gain.mortal_strike );
                p -> enrage( s );
            }
        };
        
        // Overpower ================================================================
        
        struct overpower_t : public warrior_attack_t
        {
            overpower_t( warrior_t* p, const std::string& options_str ) :
            warrior_attack_t( "overpower", p, p -> find_class_spell( "Overpower" ) )
            {
                parse_options( NULL, options_str );
                
                may_dodge  = false;
                may_parry  = false;
                may_block  = false; // The Overpower cannot be blocked, dodged or parried.
                
                normalize_weapon_speed = false;
            }
            
            virtual void execute()
            {
                warrior_t* p = cast();
                
                warrior_attack_t::execute();
                
                if ( p -> rng.taste_for_blood -> roll( p -> spec.taste_for_blood -> effectN( 1 ).percent() ) )
                {
                    p -> buff.overpower -> trigger();
                    p -> buff.taste_for_blood -> trigger();
                }
                else
                {
                    p -> buff.overpower -> expire();
                }
            }

            virtual double crit_chance( double crit, int delta_level )
           { 
              return warrior_attack_t::crit_chance( crit, delta_level ) + data().effectN( 3 ).percent();
            }
                       
            virtual bool ready()
            {
                warrior_t* p = cast();
                
                if ( ! p -> buff.overpower -> check() )
                    return false;
                
                return warrior_attack_t::ready();
            }
        };
        
        // Pummel ===================================================================
        
        struct pummel_t : public warrior_attack_t
        {
            pummel_t( warrior_t* p, const std::string& options_str ) :
            warrior_attack_t( "pummel", p, p -> find_class_spell( "Pummel" ) )
            {
                parse_options( NULL, options_str );
                
                may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
            }
            
            virtual bool ready()
            {
                if ( ! target -> debuffs.casting -> check() )
                    return false;
                
                return warrior_attack_t::ready();
            }
        };
        
        // Raging Blow ==============================================================
        
        struct raging_blow_attack_t : public warrior_attack_t
        {
            raging_blow_attack_t( warrior_t* p, const char* name, const spell_data_t* s  ) :
            warrior_attack_t( name, p, s )
            {
                may_miss = may_dodge = may_parry = false;
                background = true;
            }
            
            virtual void execute()
            {
                warrior_t* p = cast();
                
                aoe = p -> buff.meat_cleaver -> stack();
                
                warrior_attack_t::execute();
            }
        };
        
        struct raging_blow_t : public warrior_attack_t
        {
            raging_blow_attack_t* mh_attack;
            raging_blow_attack_t* oh_attack;
            
            raging_blow_t( warrior_t* p, const std::string& options_str ) :
            warrior_attack_t( "raging_blow", p, p -> find_class_spell( "Raging Blow" ) ),
            mh_attack( NULL ), oh_attack( NULL )
            {
                // FIXME:
                // check_talent( p -> talents.raging_blow -> rank() );
                
                // Parent attack is only to determine miss/dodge/parry
                base_dd_min = base_dd_max = 0;
                weapon_multiplier = direct_power_mod = 0;
                may_crit = false;
                weapon = &( p -> main_hand_weapon ); // Include the weapon for racial expertise
                
                parse_options( NULL, options_str );
                
                mh_attack = new raging_blow_attack_t( p, "raging_blow_mh", data().effectN( 1 ).trigger() );
                mh_attack -> weapon = &( p -> main_hand_weapon );
                add_child( mh_attack );
                
                oh_attack = new raging_blow_attack_t( p, "raging_blow_oh", data().effectN( 2 ).trigger() );
                oh_attack -> weapon = &( p -> off_hand_weapon );
                add_child( oh_attack );
                
                // Needs weapons in both hands
                if ( p -> main_hand_weapon.type == WEAPON_NONE ||
                    p -> off_hand_weapon.type == WEAPON_NONE )
                    background = true;
            }
            
            virtual void execute()
            {
                attack_t::execute();
                warrior_t* p = cast();
                
                if ( result_is_hit( execute_state -> result ) )
                {
                    mh_attack -> execute();
                    oh_attack -> execute();
                    p -> buff.raging_wind -> trigger();
                }
                
                p -> buff.raging_blow -> expire();
                p -> buff.meat_cleaver -> expire();
            }
            
            virtual bool ready()
            {
                warrior_t* p = cast();
                
                if ( ! p -> buff.raging_blow -> check() )
                    return false;
                
                return warrior_attack_t::ready();
            }
        };
        
        // Revenge ==================================================================
        
        struct revenge_t : public warrior_attack_t
        {
            stats_t* absorb_stats;
            
            revenge_t( warrior_t* p, const std::string& options_str ) :
            warrior_attack_t( "revenge", p, p -> find_class_spell( "Revenge" ) ),
            absorb_stats( 0 )
            {
                parse_options( NULL, options_str );
                
                direct_power_mod = data().extra_coeff();
                
                // Needs testing
                if ( p -> set_bonus.tier13_2pc_tank() )
                {
                    absorb_stats = p -> get_stats( name_str + "_tier13_2pc" );
                    absorb_stats -> type = STATS_ABSORB;
                }
            }
            
            virtual void execute()
            {
                warrior_attack_t::execute();
                warrior_t* p = cast();
                
                if ( result_is_hit( execute_state -> result ) )
                {
                    
                if ( p -> active_stance == STANCE_DEFENSE )
                    p -> resource_gain( RESOURCE_RAGE, data().effectN( 2 ).resource( RESOURCE_RAGE ), p -> gain.revenge );
                }
            }
            virtual double action_multiplier()
            {
                double am = warrior_attack_t::action_multiplier();
                
                warrior_t* p = cast();
                
                if (p->glyphs.hold_the_line ->ok() &&  p -> buff.glyph_hold_the_line -> up() )
                    am *= 1.0 + p -> buff.glyph_hold_the_line -> data().effectN( 1 ).percent();
                
                return am;
            }
            
            virtual void impact( action_state_t* s )
            {
                warrior_attack_t::impact( s );
                
                warrior_t* p = cast();
                
                // Needs testing
                if ( result_is_hit( s -> result ) )
                {
                    if ( absorb_stats )
                    {
                        double amount = 0.20 * s -> result_amount;
                        p -> buff.tier13_2pc_tank -> trigger( 1, amount );
                        absorb_stats -> add_result( amount, amount, ABSORB, s -> result );
                        absorb_stats -> add_execute( timespan_t::zero() );
                    }
                    
                    p -> active_deep_wounds -> execute();
                }
            }
        };
        
        // Shattering Throw =========================================================
        
        // TO-DO: Only a shell at the moment. Needs testing for damage etc.
        struct shattering_throw_t : public warrior_attack_t
        {
            shattering_throw_t( warrior_t* p, const std::string& options_str ) :
            warrior_attack_t( "shattering_throw", p, p -> find_class_spell( "Shattering Throw" ) )
            {
                parse_options( NULL, options_str );
                
                direct_power_mod = data().extra_coeff();
            }
            
            virtual void impact( action_state_t* s )
            {
                warrior_attack_t::impact( s );
                
                if ( result_is_hit( s -> result ) )
                    s -> target -> debuffs.shattering_throw -> trigger();
            }
            
            virtual bool ready()
            {
                if ( target -> debuffs.shattering_throw -> check() )
                    return false;
                
                return warrior_attack_t::ready();
            }
        };
        
        // Shield Slam ==============================================================
        
        struct shield_slam_t : public warrior_attack_t
        {
            double rage_gain;
            
            shield_slam_t( warrior_t* p, const std::string& options_str ) :
            warrior_attack_t( "shield_slam", p, p -> find_class_spell( "Shield Slam" ) ),
            rage_gain( 0.0 )
            {
                check_spec( WARRIOR_PROTECTION );
                
                parse_options( NULL, options_str );
                
                rage_gain = data().effectN( 3 ).resource( RESOURCE_RAGE );
                
                stats -> add_child( player -> get_stats( "shield_slam_combust" ) );
            }
            
            virtual double action_multiplier()
            {
                double am = warrior_attack_t::action_multiplier();
                
                warrior_t* p = cast();
                
                if (p->buff.shield_block->up())
                {
                    am *= 1.0 + p->glyphs.heavy_repercussions -> effectN( 1 ).percent();
                }
                
                
                return am;
            }
            
            virtual void execute()
            {
                warrior_attack_t::execute();
                warrior_t* p = cast();
                
                if ( result_is_hit( execute_state -> result ) )
                {
                    if (  p -> buff.sword_and_board -> up() )
                    {
                        p -> resource_gain( RESOURCE_RAGE,
                                           rage_gain + p -> buff.sword_and_board -> data().effectN( 1 ).resource( RESOURCE_RAGE ),
                                           p -> gain.shield_slam );
                        p -> buff.sword_and_board -> expire();
                    }
                    else
                        p -> resource_gain( RESOURCE_RAGE, rage_gain , p -> gain.shield_slam );
                }
            }
            
            virtual void impact( action_state_t* s )
            {
                warrior_attack_t::impact( s );
                
                warrior_t* p = cast();
                
                if ( result_is_hit( s -> result ) )
                    p -> buff.ultimatum -> trigger();
            }
            
        };
        
        // Shockwave ================================================================
        
        struct shockwave_t : public warrior_attack_t
        {
            shockwave_t( warrior_t* p, const std::string& options_str ) :
            warrior_attack_t( "shockwave", p, p -> find_talent_spell( "Shockwave" ) )
            {
                parse_options( NULL, options_str );
                
                direct_power_mod  = data().effectN( 3 ).percent();
                may_dodge         = false;
                may_parry         = false;
                may_block         = false;
                base_multiplier = 1.4;
            }
        };
        
        // Slam =====================================================================
        
        struct slam_t : public warrior_attack_t
        {
            slam_t( warrior_t* p, const std::string& options_str ) :
            warrior_attack_t( "slam", p, p -> find_class_spell( "Slam" ) )
            {
                parse_options( NULL, options_str );
                
                weapon = &( p -> main_hand_weapon );
            }
        };
        // Storm Bolt ==============================================================
        
        struct storm_bolt_t : public warrior_attack_t
        {
            storm_bolt_t( warrior_t* p, const std::string& options_str ) :
            warrior_attack_t( "storm_bolt", p, p -> find_talent_spell( "Storm Bolt" ) )
            {
                parse_options( NULL, options_str );
                
                //Assuming that our target is stun immune, it gets +300% dmg
                base_multiplier=4;
            }
        };
        
        // Sunder Armor =============================================================
        
        struct sunder_armor_t : public warrior_attack_t
        {
            sunder_armor_t( warrior_t* p, const std::string& options_str ) :
            warrior_attack_t( "sunder_armor", p, p -> find_spell( 7386 ) )
            {
                parse_options( NULL, options_str );
                
                base_costs[ current_resource() ] *= 1.0 + p -> glyphs.furious_sundering -> effectN( 1 ).percent();
                background = ( sim -> overrides.weakened_armor != 0 );
            }
            
            virtual void impact( action_state_t* s )
            {
                warrior_attack_t::impact( s );
                
                if ( result_is_hit( s -> result ) )
                {
                    if ( ! sim -> overrides.weakened_armor )
                        s -> target -> debuffs.weakened_armor -> trigger();
                }
            }
        };
        
        // Thunder Clap =============================================================
        
        struct thunder_clap_t : public warrior_attack_t
        {
            thunder_clap_t( warrior_t* p, const std::string& options_str ) :
            warrior_attack_t( "thunder_clap", p, p -> find_class_spell( "Thunder Clap" ) )
            {
                parse_options( NULL, options_str );
                
                aoe               = -1;
                may_dodge         = false;
                may_parry         = false;
                may_block         = false;
                direct_power_mod  = data().extra_coeff();
                
                if ( p -> spec.unwavering_sentinel -> ok() )
                    base_costs[ current_resource() ] *= 1.0 + p -> spec.unwavering_sentinel -> effectN( 2 ).percent();
                
                // TC can trigger procs from either weapon, even though it doesn't need a weapon
                proc_ignores_slot = true;
            }
            
            virtual void execute()
            {
                warrior_attack_t::execute();
                warrior_t* p = cast();
                //warrior_td_t* td = cast_td();
                
                if ( result_is_hit( execute_state -> result ) )
                {
                    if ( ! sim -> overrides.weakened_blows )
                        target -> debuffs.weakened_blows -> trigger();
                    if (p ->  spec.blood_and_thunder -> ok() )
                        p -> active_deep_wounds -> execute();
                }
            }
        };
        
        // Whirlwind ================================================================
        
        struct whirlwind_t : public warrior_attack_t
        {
            whirlwind_t( warrior_t* p, const std::string& options_str ) :
            warrior_attack_t( "whirlwind", p, p -> find_class_spell( "Whirlwind" ) )
            {
                parse_options( NULL, options_str );
                
                aoe               = -1;
            }
            
            virtual void consume_resource() { }
            
            virtual double action_multiplier()
            {
                warrior_t* p = cast();
                
                double am = warrior_attack_t::action_multiplier();
                am *= 1.0 + p -> buff.raging_wind -> data().effectN( 1 ).percent();
                return am;
            }
            
            virtual void execute()
            {
                bool meat_cleaver = false;
                warrior_t* p = cast();
                
                // MH hit
                weapon = &( p -> main_hand_weapon );
                warrior_attack_t::execute();
                
                if ( result_is_hit( execute_state -> result ) )
                    meat_cleaver = true;
                
                if ( p -> off_hand_weapon.type != WEAPON_NONE )
                {
                    weapon = &( p -> off_hand_weapon );
                    warrior_attack_t::execute();
                    if ( result_is_hit( execute_state -> result ) )
                        meat_cleaver = true;
                }
                
                if ( meat_cleaver )
                    p -> buff.meat_cleaver -> trigger();
                
                p -> buff.raging_wind -> expire();
                
                warrior_attack_t::consume_resource();
            }
        };
        
        // Wild Strike ==============================================================
        
        struct wild_strike_t : public warrior_attack_t
        {
            wild_strike_t( warrior_t* p, const std::string& options_str ) :
            warrior_attack_t( "wild_strike", p, p -> find_class_spell( "Wild Strike" ) )
            {
                parse_options( NULL, options_str );
                
                weapon = &( player -> off_hand_weapon );
                harmful           = true;
                
                if ( player -> off_hand_weapon.type == WEAPON_NONE )
                    background = true;
            }
            
            virtual double cost()
            {
                double c = warrior_attack_t::cost();
                warrior_t* p = cast();
                
                if ( p -> buff.bloodsurge -> check() )
                    c += p -> buff.bloodsurge -> data().effectN( 2 ).resource( RESOURCE_RAGE );
                
                return c;
            }
            
            virtual void schedule_execute()
            {
                warrior_t* p = cast();
                
                if ( p -> buff.bloodsurge -> check() )
                    trigger_gcd = timespan_t::from_seconds( 1.0 );
                else
                    trigger_gcd = data().gcd();
                
                warrior_attack_t::schedule_execute();
            }
            
            virtual void execute()
            {
                warrior_t* p = cast();
                p -> buff.bloodsurge -> up();
                
                warrior_attack_t::execute();
                
                p -> buff.bloodsurge -> decrement();
            }
        };
        
        // ==========================================================================
        // Warrior Spells
        // ==========================================================================
        
        struct warrior_spell_t : public warrior_action_t< spell_t >
        {
            warrior_spell_t( const std::string& n, warrior_t* p, const spell_data_t* s = spell_data_t::nil() ) :
            base_t( n, p, s )
            {
            }
            
            virtual timespan_t gcd()
            {
                // Unaffected by haste
                return trigger_gcd;
            }
        };
        
        // Avatar ===================================================================
        
        struct avatar_t : public warrior_spell_t
        {
            avatar_t( warrior_t* p, const std::string& options_str ) :
            warrior_spell_t( "avatar", p, p -> find_talent_spell( "Avatar" ) )
            {
                parse_options( NULL, options_str );
                
                harmful = false;
            }
            
            virtual void execute()
            {
                warrior_spell_t::execute();
                
                warrior_t* p = cast();
                
                p -> buff.avatar -> trigger();
            }
        };
        
        // Battle Shout =============================================================
        
        struct battle_shout_t : public warrior_spell_t
        {
            double rage_gain;
            
            battle_shout_t( warrior_t* p, const std::string& options_str ) :
            warrior_spell_t( "battle_shout", p, p -> find_class_spell( "Battle Shout" ) ),
            rage_gain( 0.0 )
            {
                parse_options( NULL, options_str );
                
                harmful   = false;
                
                rage_gain = data().effectN( 3 ).trigger() -> effectN( 1 ).resource( RESOURCE_RAGE );
                
                cooldown = p -> get_cooldown( "shout" );
                cooldown -> duration = data().cooldown();
            }
            
            virtual void execute()
            {
                warrior_spell_t::execute();
                
                warrior_t* p = cast();
                
                if ( ! sim -> overrides.attack_power_multiplier )
                    sim -> auras.attack_power_multiplier -> trigger( 1, -1.0, -1.0, data().duration() );
                
                p -> resource_gain( RESOURCE_RAGE, rage_gain , p -> gain.battle_shout );
            }
        };
        
        // Bloodbath ===================================================================
        
        struct bloodbath_t : public warrior_spell_t
        {
            bloodbath_t( warrior_t* p, const std::string& options_str ) :
            warrior_spell_t( "bloodbath", p, p -> find_talent_spell( "Bloodbath" ) )
            {
                parse_options( NULL, options_str );
                
                harmful = false;
                school = SCHOOL_BLEED;//bloodbath itself is physical, but its dot is bleed, so we make this also bleed to have the pie chart piece red as well.
            }
            
            virtual void execute()
            {
                warrior_spell_t::execute();
                
                warrior_t* p = cast();
                
                p -> buff.bloodbath -> trigger();
            }
        };
        
        // Commanding Shout =============================================================
        
        struct commanding_shout_t : public warrior_spell_t
        {
            double rage_gain;
            
            commanding_shout_t( warrior_t* p, const std::string& options_str ) :
            warrior_spell_t( "commanding_shout", p, p -> find_class_spell( "Commanding Shout" ) ),
            rage_gain( 0.0 )
            {
                parse_options( NULL, options_str );
                
                harmful   = false;
                
                rage_gain = data().effectN( 2 ).trigger() -> effectN( 1 ).resource( RESOURCE_RAGE );
                
                cooldown = p -> get_cooldown( "shout" );
                cooldown -> duration = data().cooldown();
            }
            
            virtual void execute()
            {
                warrior_spell_t::execute();
                
                warrior_t* p = cast();
                
                if ( ! sim -> overrides.stamina )
                    sim -> auras.stamina -> trigger( 1, -1.0, -1.0, data().duration() );
                
                p -> resource_gain( RESOURCE_RAGE, rage_gain , p -> gain.commanding_shout );
            }
        };
        
        // Berserker Rage ===========================================================
        
        struct berserker_rage_t : public warrior_spell_t
        {
            berserker_rage_t( warrior_t* p, const std::string& options_str ) :
            warrior_spell_t( "berserker_rage", p, p -> find_class_spell( "Berserker Rage" ) )
            {
                parse_options( NULL, options_str );
                
                harmful = false;
            }
            
            virtual void execute()
            {
                warrior_spell_t::execute();
                
                warrior_t* p = cast();
                
                p -> buff.berserker_rage -> trigger();
                p -> enrage( execute_state );
            }
        };
        
        // Deadly Calm ==============================================================
        
        struct deadly_calm_t : public warrior_spell_t
        {
            deadly_calm_t( warrior_t* p, const std::string& options_str ) :
            warrior_spell_t( "deadly_calm", p, p -> find_class_spell( "Deadly Calm" ) )
            {
                parse_options( NULL, options_str );
                
                harmful = false;
            }
            
            virtual void execute()
            {
                warrior_spell_t::execute();
                warrior_t* p = cast();
                
                p -> buff.deadly_calm -> trigger( 3 );
            }
        };
        
        // Recklessness =============================================================
        
        struct recklessness_t : public warrior_spell_t
        {
            double bonus_crit;
            
            recklessness_t( warrior_t* p, const std::string& options_str ) :
            warrior_spell_t( "recklessness", p, p -> find_class_spell( "Recklessness" ) ),
            bonus_crit( 0.0 )
            {
                parse_options( NULL, options_str );
                
                harmful = false;
                bonus_crit = data().effectN( 1 ).percent();
                if ( p -> glyphs.recklessness -> ok() )
                {
                    bonus_crit += p -> glyphs.recklessness -> effectN( 1 ).percent();
                }
                cooldown -> duration += p -> sets -> set( SET_T14_4PC_MELEE ) -> effectN( 1 ).time_value();
            }
            
            virtual void execute()
            {
                warrior_spell_t::execute();
                warrior_t* p = cast();
                
                p -> buff.recklessness -> trigger( 1, bonus_crit );
            }
        };
        
        // Retaliation ==============================================================
        
        struct retaliation_t : public warrior_spell_t
        {
            retaliation_t( warrior_t* p,  const std::string& options_str ) :
            warrior_spell_t( "retaliation", p, p -> find_spell( 20230 ) )
            {
                parse_options( NULL, options_str );
                
                harmful = false;
            }
            
            virtual void execute()
            {
                warrior_spell_t::execute();
                warrior_t* p = cast();
                
                p -> buff.retaliation -> trigger( 20 );
            }
        };
        // Shield Barrier =============================================================
        struct shield_barrier_t : public warrior_action_t<absorb_t>
        {
            
 
            double rage_cost;
            shield_barrier_t( warrior_t* p, const std::string& options_str ) :
            warrior_action_t<absorb_t>( "shield_barrier", p, p -> find_class_spell( "Shield Barrier" ) )
            {
                parse_options( NULL, options_str );
                
                may_crit=false;
                tick_may_crit     = false;
                may_miss          = false;
                target = player;
            }

            virtual void consume_resource()
            {
                resource_consumed=rage_cost;
                player -> resource_loss( current_resource(), resource_consumed, 0, this );
                
                if ( sim -> log )
                    sim -> output( "%s consumes %.1f %s for %s (%.0f)", player -> name(),
                                  resource_consumed, util::resource_type_string( current_resource() ),
                                  name(), player -> resources.current[ current_resource() ] );
                
                stats -> consume_resource( current_resource(), resource_consumed );
       
            }
            
            virtual void execute()
            {
                 warrior_t* p = cast();
                //get rage so we can use it in calc_direct_damage
                rage_cost=std::min(60.0,std::max( p -> resources.current[RESOURCE_RAGE], cost() ));
                base_t::execute();
               
            }

            //stripped down version to calculate s-> result_amount, i.e., how big our shield is, Formula: max(2*(AP-Str*2), Sta*2.5)*RAGE/60
            virtual double calculate_direct_damage( result_e r, int chain_target, double ap, double sp, double multiplier, player_t* t )
            {
                double dmg = sim -> averaged_range( base_dd_min, base_dd_max );
                
                if ( round_base_dmg ) dmg = floor( dmg + 0.5 );
                
                double base_direct_dmg = dmg;
                
                
                warrior_t* p = cast();
                dmg+= std::max(2*(p->composite_attack_power()-p->current.attribute[ATTR_STRENGTH]*2),p->current.attribute[ATTR_STAMINA]*2.5)*rage_cost/60;

                dmg *= 1.0 + p -> sets -> set( SET_T14_4PC_TANK ) -> effectN( 2 ).percent();
    
                if ( ! sim -> average_range ) dmg = floor( dmg + sim -> real() );
                
                if ( sim -> debug )
                {
                    sim -> output( "%s dmg for %s: dd=%.0f i_dd=%.0f w_dd=0.0 b_dd=%.0f mod=%.2f power=%.0f mult=%.2f w_mult=%.2f",
                                  player -> name(), name(), dmg, dmg, base_direct_dmg, direct_power_mod,
                                  ( ap + sp ), multiplier, weapon_multiplier );
                }
                
                return dmg;
            }

            
            virtual void impact( action_state_t* s )
            {
                warrior_t* p = cast();
                
                //1) Buff does not stack with itself.
                //2) Will overwrite existing buff if new one is bigger.
                if (!p->buff.shield_barrier ->up()||
                    (p->buff.shield_barrier ->up() && p->buff.shield_barrier->current_value < s->result_amount)
                    )
                {
                    p -> buff.shield_barrier -> trigger(1,s -> result_amount);
                    stats -> add_result( 0, s -> result_amount, ABSORB, s -> result );
                }

            }
        };
        
        // Shield Block =============================================================
        
        struct shield_block_buff_t : public buff_t
        {
            shield_block_buff_t( warrior_t* p ) :
            buff_t( buff_creator_t( p, "shield_block", p -> find_spell( 132404 )  ) )
            { }
        };
        
        struct shield_block_t : public warrior_spell_t
        {
            shield_block_t( warrior_t* p, const std::string& options_str ) :
            warrior_spell_t( "shield_block", p, p -> find_class_spell( "Shield Block" ) )
            {
                parse_options( NULL, options_str );
                
                harmful = false;
                cooldown -> duration = timespan_t::from_seconds( 9.0 );
                cooldown -> charges = 2;

            }
            virtual double cost()
            {
                double c = warrior_spell_t::cost();
                warrior_t* p = cast();
                
                c+= p -> sets -> set( SET_T14_4PC_TANK ) -> effectN( 1 ).base_value()/10;
                return c;

            }
            
            virtual void execute()
            {
                warrior_spell_t::execute();
                warrior_t* p = cast();
                
                if (p -> buff.shield_block -> up())
                {
                    p -> buff.shield_block -> extend_duration(p,timespan_t::from_seconds( 6.0 ));
                }
                else
                p -> buff.shield_block -> trigger();
            }
        };
        
        
        // Shield Wall =============================================================
        struct shield_wall_t : public warrior_spell_t
        {
            shield_wall_t( warrior_t* p, const std::string& options_str ) :
            warrior_spell_t( "shield_wall", p, p -> find_class_spell( "Shield Wall" ) )
            {
                parse_options( NULL, options_str );
                
                harmful = false;
                cooldown -> duration += p -> spec.bastion_of_defense -> effectN(2).time_value()
                                     +( p -> glyphs.shield_wall -> ok() ? p->glyphs.shield_wall -> effectN( 1 ).time_value() : timespan_t::zero() )  ;

            }
            
            virtual void execute()
            {
                warrior_spell_t::execute();
                warrior_t* p = cast();
                
                p -> buff.shield_wall -> trigger(1,p-> buff.shield_wall -> data().effectN(1).percent() + (p->glyphs.shield_wall ->ok()? p ->glyphs.shield_wall->effectN(2).percent():0) );
            }
        };
        
        
        // Skull Banner =============================================================
        
        struct skull_banner_t : public warrior_spell_t
        {
            skull_banner_t( warrior_t* p, const std::string& options_str ) :
            warrior_spell_t( "skull_banner", p, p -> find_class_spell( "Skull Banner" ) )
            {
                parse_options( NULL, options_str );
                harmful = false;
            }
            
            virtual void execute()
            {
                warrior_spell_t::execute();
                for ( size_t i = 0; i < sim -> player_list.size(); ++i )
                {
                    player_t* p = sim -> player_list[ i ];
                    if ( p -> current.sleeping || p -> is_pet() || p -> is_enemy() )
                        continue;
                    p -> buffs.skull_banner -> trigger();
                }
            }
        };
        
        // Stance ===================================================================
        
        struct stance_t : public warrior_spell_t
        {
            warrior_stance switch_to_stance;
            std::string stance_str;
            
            stance_t( warrior_t* p, const std::string& options_str ) :
            warrior_spell_t( "stance", p ),
            switch_to_stance( STANCE_BATTLE ), stance_str( "" )
            {
                option_t options[] =
                {
                    { "choose",  OPT_STRING, &stance_str     },
                    { NULL, OPT_UNKNOWN, NULL }
                };
                parse_options( options, options_str );
                
                if ( ! stance_str.empty() )
                {
                    if ( stance_str == "battle" )
                        switch_to_stance = STANCE_BATTLE;
                    else if ( stance_str == "berserker" || stance_str == "zerker" )
                        switch_to_stance = STANCE_BERSERKER;
                    else if ( stance_str == "def" || stance_str == "defensive" )
                        switch_to_stance = STANCE_DEFENSE;
                }
                
                harmful = false;
                trigger_gcd = timespan_t::zero();
                cooldown -> duration = timespan_t::from_seconds( 1.0 );
            }
            
            virtual resource_e current_resource() { return RESOURCE_RAGE; }
            
            virtual void execute()
            {
                warrior_t* p = cast();
                
                switch ( p -> active_stance )
                {
                    case STANCE_BATTLE:     p -> buff.battle_stance    -> expire(); break;
                    case STANCE_BERSERKER:  p -> buff.berserker_stance -> expire(); break;
                    case STANCE_DEFENSE:    p -> buff.defensive_stance -> expire(); break;
                }
                p -> active_stance = switch_to_stance;
                
                switch ( p -> active_stance )
                {
                    case STANCE_BATTLE:     p -> buff.battle_stance    -> trigger(); break;
                    case STANCE_BERSERKER:  p -> buff.berserker_stance -> trigger(); break;
                    case STANCE_DEFENSE:    p -> buff.defensive_stance -> trigger(); break;
                }
                
                consume_resource();
                
                update_ready();
            }
            
            virtual bool ready()
            {
                warrior_t* p = cast();
                
                if ( p -> active_stance == switch_to_stance )
                    return false;
                
                return warrior_spell_t::ready();
            }
        };
        
        // Sweeping Strikes =========================================================
        
        struct sweeping_strikes_t : public warrior_spell_t
        {
            sweeping_strikes_t( warrior_t* p, const std::string& options_str ) :
            warrior_spell_t( "sweeping_strikes", p, p -> find_spell( 12328 ) )
            {
                // FIXME:
                // check_talent( p -> talents.sweeping_strikes -> rank() );
                
                parse_options( NULL, options_str );
                
                harmful = false;
            }
            
            virtual void execute()
            {
                warrior_spell_t::execute();
                warrior_t* p = cast();
                
                p -> buff.sweeping_strikes -> trigger();
            }
        };
        
        // Last Stand ===============================================================
        
        struct last_stand_t : public warrior_spell_t
        {
            last_stand_t( warrior_t* p, const std::string& options_str ) :
            warrior_spell_t( "last_stand", p, p -> find_spell( 12975 ) )
            {
                harmful = false;
                
                parse_options( NULL, options_str );
                cooldown->duration += p -> sets -> set( SET_T14_2PC_TANK ) -> effectN( 1 ).time_value();
            }
            
            virtual void execute()
            {
                warrior_spell_t::execute();
                warrior_t* p = cast();
                
                p -> buff.last_stand -> trigger();
            }
        };
        
        struct buff_last_stand_t : public buff_t
        {
            int health_gain;
            
            buff_last_stand_t( warrior_t* p, const uint32_t id, const std::string& /* n */ ) :
            buff_t( buff_creator_t( p, "last_stand", p -> find_spell( id ) ) ), health_gain( 0 )
            { }
            
            virtual bool trigger( int stacks, double value, double chance, timespan_t duration )
            {
                health_gain = ( int ) floor( player -> resources.max[ RESOURCE_HEALTH ] * 0.3 );
                player -> stat_gain( STAT_MAX_HEALTH, health_gain );
                
                return buff_t::trigger( stacks, value, chance, duration );
            }
            
            virtual void expire()
            {
                player -> stat_loss( STAT_MAX_HEALTH, health_gain );
                
                buff_t::expire();
            }
        };
        
    } // UNNAMED NAMESPACE
    
    // ==========================================================================
    // Warrior Character Definition
    // ==========================================================================
    
    warrior_td_t::warrior_td_t( player_t* target, warrior_t* p  ) :
    actor_pair_t( target, p )
    {
        dots_bloodbath   = target -> get_dot( "bloodbath",    p );
        dots_deep_wounds = target -> get_dot( "deep_wounds",  p );
        
        debuffs_colossus_smash = buff_creator_t( *this, "colossus_smash" ).duration( timespan_t::from_seconds( 6.0 ) );
        debuffs_demoralizing_shout = buff_creator_t( *this, "demoralizing_shout" ).duration( timespan_t::from_seconds( 10.0 ) )
                                     .default_value( p -> find_spell( 1160 ) -> effectN( 1 ).percent() );
        
        p ->  buff.shield_barrier= absorb_buff_creator_t( *this, "shield_barrier", source -> find_spell( 112048 ) )
        .source( source -> get_stats( "shield_barrier" ) );
        
        p -> absorb_buffs.push_back( p->buff.shield_barrier );

    }
    
    // warrior_t::create_action  ================================================
    
    action_t* warrior_t::create_action( const std::string& name,
                                       const std::string& options_str )
    {
        if ( name == "auto_attack"        ) return new auto_attack_t        ( this, options_str );
        if ( name == "avatar"             ) return new avatar_t             ( this, options_str );
        if ( name == "battle_shout"       ) return new battle_shout_t       ( this, options_str );
        if ( name == "berserker_rage"     ) return new berserker_rage_t     ( this, options_str );
        if ( name == "bladestorm"         ) return new bladestorm_t         ( this, options_str );
        if ( name == "bloodbath"          ) return new bloodbath_t          ( this, options_str );
        if ( name == "bloodthirst"        ) return new bloodthirst_t        ( this, options_str );
        if ( name == "charge"             ) return new charge_t             ( this, options_str );
        if ( name == "cleave"             ) return new cleave_t             ( this, options_str );
        if ( name == "colossus_smash"     ) return new colossus_smash_t     ( this, options_str );
        if ( name == "concussion_blow"    ) return new concussion_blow_t    ( this, options_str );
        if ( name == "deadly_calm"        ) return new deadly_calm_t        ( this, options_str );
        if ( name == "demoralizing_shout" ) return new demoralizing_shout   ( this, options_str );        
        if ( name == "devastate"          ) return new devastate_t          ( this, options_str );
        if ( name == "dragon_roar"        ) return new dragon_roar_t        ( this, options_str );
        if ( name == "execute"            ) return new execute_t            ( this, options_str );
        if ( name == "heroic_leap"        ) return new heroic_leap_t        ( this, options_str );
        if ( name == "heroic_strike"      ) return new heroic_strike_t      ( this, options_str );
        if ( name == "heroic_throw"       ) return new heroic_throw_t       ( this, options_str );
        if ( name == "impending_victory"  ) return new impending_victory_t  ( this, options_str );
        if ( name == "last_stand"         ) return new last_stand_t         ( this, options_str );
        if ( name == "mortal_strike"      ) return new mortal_strike_t      ( this, options_str );
        if ( name == "overpower"          ) return new overpower_t          ( this, options_str );
        if ( name == "pummel"             ) return new pummel_t             ( this, options_str );
        if ( name == "raging_blow"        ) return new raging_blow_t        ( this, options_str );
        if ( name == "recklessness"       ) return new recklessness_t       ( this, options_str );
        if ( name == "retaliation"        ) return new retaliation_t        ( this, options_str );
        if ( name == "revenge"            ) return new revenge_t            ( this, options_str );
        if ( name == "shattering_throw"   ) return new shattering_throw_t   ( this, options_str );
        if ( name == "shield_barrier"     ) return new shield_barrier_t     ( this, options_str );
        if ( name == "shield_block"       ) return new shield_block_t       ( this, options_str );
        if ( name == "shield_wall"        ) return new shield_wall_t        ( this, options_str );
        if ( name == "shield_slam"        ) return new shield_slam_t        ( this, options_str );
        if ( name == "shockwave"          ) return new shockwave_t          ( this, options_str );
        if ( name == "skull_banner"       ) return new skull_banner_t       ( this, options_str );
        if ( name == "slam"               ) return new slam_t               ( this, options_str );
        if ( name == "storm_bolt"         ) return new storm_bolt_t         ( this, options_str );
        if ( name == "stance"             ) return new stance_t             ( this, options_str );
        if ( name == "sunder_armor"       ) return new sunder_armor_t       ( this, options_str );
        if ( name == "sweeping_strikes"   ) return new sweeping_strikes_t   ( this, options_str );
        if ( name == "thunder_clap"       ) return new thunder_clap_t       ( this, options_str );
        if ( name == "whirlwind"          ) return new whirlwind_t          ( this, options_str );
        if ( name == "wild_strike"        ) return new wild_strike_t        ( this, options_str );
        
        return player_t::create_action( name, options_str );
    }
    
    // warrior_t::init_spells ===================================================
    
    void warrior_t::init_spells()
    {
        player_t::init_spells();
        
        // Mastery
        mastery.critical_block         = find_mastery_spell( WARRIOR_PROTECTION );
        mastery.strikes_of_opportunity = find_mastery_spell( WARRIOR_ARMS );
        mastery.unshackled_fury        = find_mastery_spell( WARRIOR_FURY );
        
        // Spec Passives
        spec.bastion_of_defense               = find_specialization_spell( "Bastion of Defense" );
        spec.blood_and_thunder                = find_specialization_spell( "Blood and Thunder" );
        spec.bloodsurge                       = find_specialization_spell( "Bloodsurge" );
        spec.crazed_berserker                 = find_specialization_spell( "Crazed Berserker" );
        spec.flurry                           = find_specialization_spell( "Flurry" );
        spec.meat_cleaver                     = find_specialization_spell( "Meat Cleaver" );
        spec.seasoned_soldier                 = find_specialization_spell( "Seasoned Soldier" );
        spec.unwavering_sentinel              = find_specialization_spell( "Unwavering Sentinel" );
        spec.single_minded_fury               = find_specialization_spell( "Single-Minded Fury" );
        spec.sword_and_board                  = find_specialization_spell( "Sword and Board" );
        spec.sudden_death                     = find_specialization_spell( "Sudden Death" );
        spec.taste_for_blood                  = find_specialization_spell( "Taste for Blood" );
        spec.ultimatum                        = find_specialization_spell( "Ultimatum" );
        
        // Talents
        talents.juggernaut            = find_talent_spell( "Juggernaut" );
        talents.double_time           = find_talent_spell( "Double Time" );
        talents.warbringer            = find_talent_spell( "Warbringer" );
        
        talents.enraged_regeneration  = find_talent_spell( "Enraged Regeneration" );
        talents.second_wind           = find_talent_spell( "Second Wind" );
        talents.impending_victory     = find_talent_spell( "Impending Victory" );
        
        talents.staggering_shout      = find_talent_spell( "Staggering Shout" );
        talents.piercing_howl         = find_talent_spell( "Piercing Howl" );
        talents.disrupting_shout      = find_talent_spell( "Disrupting Shout" );
        
        talents.bladestorm            = find_talent_spell( "Bladestorm" );
        talents.shockwave             = find_talent_spell( "Shockwave" );
        talents.dragon_roar           = find_talent_spell( "Dragon Roar" );
        
        talents.mass_spell_reflection = find_talent_spell( "Mass Spell Reflection" );
        talents.safeguard             = find_talent_spell( "Safeguard" );
        talents.vigilance             = find_talent_spell( "Vigilance" );
        
        talents.avatar                = find_talent_spell( "Avatar" );
        talents.bloodbath             = find_talent_spell( "Bloodbath" );
        talents.storm_bolt            = find_talent_spell( "Storm Bolt" );
        
        // Glyphs
        glyphs.colossus_smash      = find_glyph_spell( "Glyph of Colossus Smash" );
        glyphs.death_from_above    = find_glyph_spell( "Glyph of Death From Above" );
        glyphs.furious_sundering   = find_glyph_spell( "Glyph of Forious Sundering" );
        glyphs.heavy_repercussions = find_glyph_spell( "Glyph of Heavy Repercussions" );
        glyphs.hold_the_line       = find_glyph_spell( "Glyph of Hold the Line" );
        glyphs.incite              = find_glyph_spell( "Glyph of Incite" );
        glyphs.overpower           = find_glyph_spell( "Glyph of Overpower" );
        glyphs.raging_wind         = find_glyph_spell( "Glyph of Raging Wind" );
        glyphs.recklessness        = find_glyph_spell( "Glyph of Recklessness" );
        glyphs.shield_wall         = find_glyph_spell( "Glyph of Shield Wall" );
        glyphs.sweeping_strikes    = find_glyph_spell( "Glyph of Sweeping Strikes" );
        glyphs.unending_rage       = find_glyph_spell( "Glyph of Unending Rage" );
        
        // Active spells
        active_deep_wounds = new deep_wounds_t( this );
        active_bloodbath_dot = new bloodbath_dot_t( this );
        
        if ( mastery.strikes_of_opportunity -> ok() )
            active_opportunity_strike = new opportunity_strike_t( this );
        
        
        static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
        {
            //  C2P    C4P     M2P     M4P     T2P     T4P    H2P    H4P
            {     0,     0, 105797, 105907, 105908, 105911,     0,     0 }, // Tier13
            {     0,     0, 123142, 123144, 123146, 123147,     0,     0 }, // Tier14
            {     0,     0,      0,      0,      0,      0,     0,     0 },
        };
        
        sets = new set_bonus_array_t( this, set_bonuses );
    }
    
    // warrior_t::init_defense ==================================================
    
    void warrior_t::init_defense()
    {
        player_t::init_defense();
        
        initial.parry_rating_per_strength = 0.27;
    }
    
    // warrior_t::init_base =====================================================
    
    void warrior_t::init_base()
    {
        player_t::init_base();
        
        resources.base[  RESOURCE_RAGE  ] = 100;
        if ( glyphs.unending_rage -> ok() )
            resources.base[  RESOURCE_RAGE  ] += glyphs.unending_rage -> effectN( 1 ).resource( RESOURCE_RAGE );
        
        initial.attack_power_per_strength = 2.0;
        initial.attack_power_per_agility  = 0.0;
        
        base.attack_power = level * 2 + 60;
        
        // FIXME! Level-specific!
        base.miss    = 0.05;
        base.parry   = 0.05;
        base.dodge   = 0.05;
        base.block   = 0.05;
        
        base.block_reduction=0.3;
        
        if ( specialization() == WARRIOR_PROTECTION )
            vengeance.enabled = true;
        
        //updated from http://sacredduty.net/2012/07/06/avoidance-diminishing-returns-in-mop-part-3/
        diminished_kfactor    = 0.00885;
        diminished_dodge_capi = 0.01523660;
        diminished_parry_capi = 0.00424628450106;
        
        if ( spec.unwavering_sentinel -> ok() )
        {
            initial.attribute_multiplier[ ATTR_STAMINA ] *= 1.0  + spec.unwavering_sentinel -> effectN( 1 ).percent();
            initial.armor_multiplier *= 1.0 + spec.unwavering_sentinel-> effectN( 3 ).percent();
        }
        
        base_gcd = timespan_t::from_seconds( 1.5 );
    }
    
    // warrior_t::init_scaling ==================================================
    
    void warrior_t::init_scaling()
    {
        player_t::init_scaling();
        
        if ( specialization() == WARRIOR_FURY )
        {
            scales_with[ STAT_WEAPON_OFFHAND_DPS    ] = true;
            scales_with[ STAT_WEAPON_OFFHAND_SPEED  ] = sim -> weapon_speed_scale_factors != 0;
            scales_with[ STAT_HIT_RATING2           ] = true;
        }
        
        if ( primary_role() == ROLE_TANK )
        {
            scales_with[ STAT_PARRY_RATING ] = true;
            scales_with[ STAT_BLOCK_RATING ] = true;
        }
    }
    
    // warrior_t::init_buffs ====================================================
    
    void warrior_t::init_buffs()
    {
        player_t::init_buffs();
        
        // Haste buffs
        buff.flurry           = haste_buff_creator_t( this, "flurry",           spec.flurry -> effectN( 1 ).trigger() )
        .chance( spec.flurry -> proc_chance() );
        
        // Regular buffs
        buff.avatar           = buff_creator_t( this, "avatar",           talents.avatar )
        .cd( timespan_t::zero() );
        buff.battle_stance    = buff_creator_t( this, "battle_stance",    find_spell( 21156 ) );
        buff.berserker_rage   = buff_creator_t( this, "berserker_rage",   find_class_spell( "Berserker Rage" ) )
        .cd( timespan_t::zero() );
        buff.berserker_stance = buff_creator_t( this, "berserker_stance", find_spell( 7381 ) );
        buff.bloodbath        = buff_creator_t( this, "bloodbath",        talents.bloodbath )
        .cd( timespan_t::zero() );
        buff.bloodsurge       = buff_creator_t( this, "bloodsurge",       spec.bloodsurge -> effectN( 1 ).trigger() )
        .chance( ( spec.bloodsurge -> ok() ? spec.bloodsurge -> proc_chance() : 0 ) );
        buff.deadly_calm      = buff_creator_t( this, "deadly_calm",      find_class_spell( "Deadly Calm" ) )
        .cd( timespan_t::zero() );
        buff.defensive_stance = buff_creator_t( this, "defensive_stance", find_spell( 7376 ) );
        buff.enrage           = buff_creator_t( this, "enrage",           find_spell( 12880 ) );
        buff.glyph_overpower  = buff_creator_t( this, "glyph_of_overpower", glyphs.overpower -> effectN( 1 ).trigger() )
        .chance( glyphs.overpower -> ok() ? glyphs.overpower -> proc_chance() : 0 );
        buff.glyph_hold_the_line    = buff_creator_t( this, "hold_the_line",    glyphs.hold_the_line -> effectN( 1 ).trigger() );
        buff.incite           = buff_creator_t( this, "incite",           glyphs.incite -> effectN( 1 ).trigger() )
        .chance( glyphs.incite -> ok () ? glyphs.incite -> proc_chance() : 0 );
        buff.meat_cleaver     = buff_creator_t( this, "meat_cleaver",     spec.meat_cleaver -> effectN( 1 ).trigger() );
        buff.overpower        = buff_creator_t( this, "overpower",        spell_data_t::nil() )
        .duration( timespan_t::from_seconds( 9.0 ) );
        buff.raging_blow      = buff_creator_t( this, "raging_blow",      find_spell( 131116 ) )
        .max_stack( find_spell( 131116 ) -> effectN( 1 ).base_value() );
        buff.raging_wind      = buff_creator_t( this, "raging_wind",      glyphs.raging_wind -> effectN( 1 ).trigger() )
        .chance( ( glyphs.raging_wind -> ok() ? 1 : 0 ) );
        buff.recklessness     = buff_creator_t( this, "recklessness",     find_class_spell( "Recklessness" ) )
        .duration( find_class_spell( "Recklessness" ) -> duration() * ( 1.0 + ( glyphs.recklessness -> ok() ? glyphs.recklessness -> effectN( 2 ).percent() : 0 )  ) )
        .cd( timespan_t::zero() );
        buff.retaliation      = buff_creator_t( this, "retaliation", find_spell( 20230 ) )
        .cd( timespan_t::zero() );
        buff.taste_for_blood  = buff_creator_t( this, "taste_for_blood",  find_spell( 125831 ) );
        
        buff.shield_block     = new shield_block_buff_t( this );
        buff.shield_wall      = buff_creator_t(this, "shield_wall", find_class_spell("Shield Wall"))
        .default_value(find_class_spell("Shield Wall")-> effectN(1).percent())
        .cd( timespan_t::zero());
        buff.sweeping_strikes = buff_creator_t( this, "sweeping_strikes",  find_class_spell( "Sweeping Strikes" ) )
        .cd( timespan_t::zero() );
        buff.sword_and_board  = buff_creator_t( this, "sword_and_board",   find_spell( 50227 ) )
        .chance( spec.sword_and_board -> effectN( 1 ).percent() );
        buff.ultimatum        = buff_creator_t( this, "ultimatum",   spec.ultimatum -> effectN( 1 ).trigger() )
        .chance( spec.ultimatum -> ok() ? spec.ultimatum -> proc_chance() : 0 );
        
        buff.last_stand       = new buff_last_stand_t( this, 12975, "last_stand" );
        buff.tier13_2pc_tank  = buff_creator_t( this, "tier13_2pc_tank", find_spell( 105909 ) );
        // FIX ME
        // absorb_buffs.push_back( buff.tier13_2pc_tank );
        buff.rude_interruption = buff_creator_t( this, "rude_interruption", find_spell( 86663 ) )
        .default_value( find_spell( 86663 ) -> effectN( 1 ).percent() );
    }
    
    // warrior_t::init_gains ====================================================
    
    void warrior_t::init_gains()
    {
        player_t::init_gains();
        
        gain.avoided_attacks        = get_gain( "avoided_attacks"       );
        gain.battle_shout           = get_gain( "battle_shout"          );
        gain.bloodthirst            = get_gain( "bloodthirst"           );
        gain.charge                 = get_gain( "charge"                );
        gain.commanding_shout       = get_gain( "commanding_shout"      );
        gain.critical_block         = get_gain( "critical_block"        );
        gain.defensive_stance       = get_gain( "defensive_stance"      );
        gain.enrage                 = get_gain( "enrage"                );
        gain.melee_main_hand        = get_gain( "melee_main_hand"       );
        gain.melee_off_hand         = get_gain( "melee_off_hand"        );
        gain.mortal_strike          = get_gain( "mortal_strike"         );
        gain.revenge                = get_gain( "revenge"               );
        gain.shield_slam            = get_gain( "shield_slam"           );
    }
    
    // warrior_t::init_procs ====================================================
    
    void warrior_t::init_procs()
    {
        player_t::init_procs();
        
        proc.munched_deep_wounds     = get_proc( "munched_deep_wounds"     );
        proc.rolled_deep_wounds      = get_proc( "rolled_deep_wounds"      );
        proc.parry_haste             = get_proc( "parry_haste"             );
        proc.strikes_of_opportunity  = get_proc( "strikes_of_opportunity"  );
        proc.sudden_death            = get_proc( "sudden_death"            );
        proc.tier13_4pc_melee        = get_proc( "tier13_4pc_melee"        );
    }
    
    // warrior_t::init_rng ======================================================
    
    void warrior_t::init_rng()
    {
        player_t::init_rng();
        
        rng.strikes_of_opportunity    = get_rng( "strikes_of_opportunity"    );
        rng.sudden_death              = get_rng( "sudden_death"              );
        rng.taste_for_blood           = get_rng( "taste_for_blood"           );
    }
    
    // warrior_t::init_actions ==================================================
    
    void warrior_t::init_actions()
    {
        if ( main_hand_weapon.type == WEAPON_NONE )
        {
            if ( !quiet )
                sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
            quiet = true;
            return;
        }
        
        if ( action_list_str.empty() )
        {
            clear_action_priority_lists();
            
            std::string& precombat_list = get_action_priority_list( "precombat" ) -> action_list_str;
            
            switch ( specialization() )
            {
                case WARRIOR_FURY:
                case WARRIOR_ARMS:
                    if ( sim -> allow_flasks )
                    {
                        // Flask
                        if ( level > 85 )
                            precombat_list += "/flask,type=winters_bite";
                        else if ( level >= 80 )
                            precombat_list += "/flask,type=titanic_strength";
                    }
                    
                    if ( sim -> allow_food )
                    {
                        // Food
                        if ( level > 85 )
                            precombat_list += "/food,type=black_pepper_ribs_and_shrimp";
                        else if ( level >= 80 )
                            precombat_list += "/food,type=seafood_magnifique_feast";
                    }
                    
                    break;
                    
                case WARRIOR_PROTECTION:
                    if ( sim -> allow_flasks )
                    {
                        // Flask
                        if ( level >= 80 )
                            precombat_list += "/flask,type=earth";
                        else if ( level >= 75 )
                            precombat_list += "/flask,type=steelskin";
                    }
                    
                    if ( sim -> allow_food )
                    {
                        // Food
                        if ( level >= 80 )
                            precombat_list += "/food,type=great_pandaren_banquet";
                        else if ( level >= 70 )
                            precombat_list += "/food,type=beer_basted_crocolisk";
                    }
                    
                    break; default: break;
            }
            
            precombat_list += "/snapshot_stats";
            
            if ( specialization() != WARRIOR_PROTECTION )
            {
                precombat_list += "/stance,choose=battle";
            }
            else
            {
                precombat_list += "/stance,choose=defensive";
            }
            
            action_list_str += "/auto_attack";
            
            if ( sim -> allow_potions )
            {
                // Potion
                if ( specialization() == WARRIOR_ARMS )
                {
                    if ( level > 85 )
                    {
                        precombat_list  += "/mogu_power_potion";
                        action_list_str += "/mogu_power_potion,if=(target.health.pct<20&buff.recklessness.up)|buff.bloodlust.react|target.time_to_die<=25";
                    }
                    else if ( level >= 80 )
                    {
                        precombat_list  += "/golemblood_potion";
                        action_list_str += "/golemblood_potion,if=(target.health.pct<20&buff.recklessness.up)|buff.bloodlust.react|target.time_to_die<=25";
                    }
                }
                else if ( specialization() == WARRIOR_FURY )
                {
                    if ( level > 85 )
                    {
                        precombat_list  += "/mogu_power_potion";
                        action_list_str += "/mogu_power_potion,if=(target.health.pct<20&buff.recklessness.up)|buff.bloodlust.react|target.time_to_die<=25";
                    }
                    else if ( level >= 80 )
                    {
                        precombat_list  += "/golemblood_potion";
                        action_list_str += "/golemblood_potion,if=(target.health.pct<20&buff.recklessness.up)|buff.bloodlust.react|target.time_to_die<=25";
                    }
                }
                else
                {
                    if ( level > 85 )
                    {
                        precombat_list  += "/mountains_potion";
                        action_list_str += "/mountains_potion,if=health_pct<35&buff.mountains_potion.down";
                    }
                    else if ( level >= 80 )
                    {
                        precombat_list  += "/earthen_potion";
                        action_list_str += "/earthen_potion,if=health_pct<35&buff.earthen_potion.down";
                    }
                }
            }
            
            // Usable Item
            int num_items = ( int ) items.size();
            for ( int i=0; i < num_items; i++ )
            {
                if ( items[ i ].use.active() )
                {
                    action_list_str += "/use_item,name=";
                    action_list_str += items[ i ].name();
                }
            }
            
            action_list_str += init_use_profession_actions();
            action_list_str += init_use_racial_actions();
            
            // Arms
            if ( specialization() == WARRIOR_ARMS )
            {
                if ( level >= 90 )
                {
                    action_list_str += "/recklessness,use_off_gcd=1,if=((debuff.colossus_smash.remains>=5|cooldown.colossus_smash.remains<=4)&((!talent.avatar.enabled|!set_bonus.tier14_4pc_melee)&((target.health.pct<20|target.time_to_die>315|(target.time_to_die>165&set_bonus.tier14_4pc_melee)))|(talent.avatar.enabled&set_bonus.tier14_4pc_melee&buff.avatar.up)))|target.time_to_die<=18";
                    action_list_str += "/avatar,use_off_gcd=1,if=talent.avatar.enabled&(((cooldown.recklessness.remains>=180|buff.recklessness.up)|(target.health.pct>=20&target.time_to_die>195)|(target.health.pct<20&set_bonus.tier14_4pc_melee))|target.time_to_die<=20)";
                    action_list_str += "/bloodbath,use_off_gcd=1,if=talent.bloodbath.enabled&(((cooldown.recklessness.remains>=10|buff.recklessness.up)|(target.health.pct>=20&(target.time_to_die<=165|(target.time_to_die<=315&!set_bonus.tier14_4pc_melee))&target.time_to_die>75))|target.time_to_die<=19)";
                    //        action_list_str += "/skull_banner,use_off_gcd=1,if=buff.recklessness.up|(cooldown.recklessness.remains>=75&(buff.avatar.up|buff.bloodbath.up))|(cooldown.recklessness.remains>=75&(!talent.avatar.enabled&!talent.bloodbath.enabled))|target.time_to_die<=10";
                    action_list_str += "/berserker_rage,use_off_gcd=1,if=!buff.enrage.up";
                    action_list_str += "/heroic_leap,use_off_gcd=1,if=debuff.colossus_smash.up";
                    action_list_str += "/deadly_calm,use_off_gcd=1,if=rage>=40";
                    action_list_str += "/heroic_strike,use_off_gcd=1,if=((buff.taste_for_blood.up&buff.taste_for_blood.remains<=2)|(buff.taste_for_blood.stack=5&buff.overpower.up)|(buff.taste_for_blood.up&debuff.colossus_smash.remains<=2&!cooldown.colossus_smash.remains=0)|buff.deadly_calm.up|rage>110)&target.health.pct>=20&debuff.colossus_smash.up";
                    action_list_str += "/mortal_strike";
                    action_list_str += "/colossus_smash,if=debuff.colossus_smash.remains<=1.5";
                    action_list_str += "/execute";
                    action_list_str += "/storm_bolt,if=talent.storm_bolt.enabled";
                    action_list_str += "/overpower,if=buff.overpower.up";
                    action_list_str += "/shockwave,if=talent.shockwave.enabled";
                    action_list_str += "/dragon_roar,if=talent.dragon_roar.enabled";
                    action_list_str += "/slam,if=(rage>=70|debuff.colossus_smash.up)&target.health.pct>=20";
                    action_list_str += "/heroic_throw";
                    action_list_str += "/battle_shout,if=rage<70&!debuff.colossus_smash.up";
                    action_list_str += "/bladestorm,if=talent.bladestorm.enabled&cooldown.colossus_smash.remains>=5&!debuff.colossus_smash.up&cooldown.bloodthirst.remains>=2&target.health.pct>=20";
                    action_list_str += "/slam,if=target.health.pct>=20";
                    action_list_str += "/impending_victory,if=talent.impending_victory.enabled&target.health.pct>=20";
                    action_list_str += "/battle_shout,if=rage<70";
                }
                else if ( level >= 85 )
                {
                    action_list_str += "/recklessness,use_off_gcd=1,if=((debuff.colossus_smash.remains>=5|cooldown.colossus_smash.remains<=4)&(target.health.pct<20|target.time_to_die>315))|target.time_to_die<=18";
                    action_list_str += "/berserker_rage,use_off_gcd=1,if=!buff.enrage.up";
                    action_list_str += "/heroic_leap,use_off_gcd=1,if=debuff.colossus_smash.up";
                    action_list_str += "/deadly_calm,use_off_gcd=1,if=rage>=40";
                    action_list_str += "/heroic_strike,use_off_gcd=1,if=((buff.taste_for_blood.up&buff.taste_for_blood.remains<=2)|(buff.taste_for_blood.stack=5&buff.overpower.up)|(buff.taste_for_blood.up&debuff.colossus_smash.remains<=2&!cooldown.colossus_smash.remains=0)|buff.deadly_calm.up|rage>110)&target.health.pct>=20&debuff.colossus_smash.up";
                    action_list_str += "/mortal_strike";
                    action_list_str += "/colossus_smash,if=debuff.colossus_smash.remains<=1.5";
                    action_list_str += "/execute";
                    action_list_str += "/overpower,if=buff.overpower.up";
                    action_list_str += "/shockwave,if=talent.shockwave.enabled";
                    action_list_str += "/dragon_roar,if=talent.dragon_roar.enabled";
                    action_list_str += "/slam,if=(rage>=70|debuff.colossus_smash.up)&target.health.pct>=20";
                    action_list_str += "/heroic_throw";
                    action_list_str += "/battle_shout,if=rage<70&!debuff.colossus_smash.up";
                    action_list_str += "/bladestorm,if=talent.bladestorm.enabled&cooldown.colossus_smash.remains>=5&!debuff.colossus_smash.up&cooldown.bloodthirst.remains>=2&target.health.pct>=20";
                    action_list_str += "/slam,if=target.health.pct>=20";
                    action_list_str += "/impending_victory,if=talent.impending_victory.enabled&target.health.pct>=20";
                    action_list_str += "/battle_shout,if=rage<70";
                }
            }
            
            // Fury
            else if ( specialization() == WARRIOR_FURY )
            {
                if ( level >= 90 )
                {
                    action_list_str += "/recklessness,use_off_gcd=1,if=((debuff.colossus_smash.remains>=5|cooldown.colossus_smash.remains<=4)&((!talent.avatar.enabled|!set_bonus.tier14_4pc_melee)&((target.health.pct<20|target.time_to_die>315|(target.time_to_die>165&set_bonus.tier14_4pc_melee)))|(talent.avatar.enabled&set_bonus.tier14_4pc_melee&buff.avatar.up)))|target.time_to_die<=18";
                    action_list_str += "/avatar,use_off_gcd=1,if=talent.avatar.enabled&(((cooldown.recklessness.remains>=180|buff.recklessness.up)|(target.health.pct>=20&target.time_to_die>195)|(target.health.pct<20&set_bonus.tier14_4pc_melee))|target.time_to_die<=20)";
                    action_list_str += "/bloodbath,use_off_gcd=1,if=talent.bloodbath.enabled&(((cooldown.recklessness.remains>=10|buff.recklessness.up)|(target.health.pct>=20&(target.time_to_die<=165|(target.time_to_die<=315&!set_bonus.tier14_4pc_melee))&target.time_to_die>75))|target.time_to_die<=19)";
                    //        action_list_str += "/skull_banner,use_off_gcd=1,if=buff.recklessness.up|(cooldown.recklessness.remains>=75&(buff.avatar.up|buff.bloodbath.up))|(cooldown.recklessness.remains>=75&(!talent.avatar.enabled&!talent.bloodbath.enabled))|target.time_to_die<=10";
                    action_list_str += "/berserker_rage,use_off_gcd=1,if=!(buff.enrage.react|(buff.raging_blow.react=2&target.health.pct>=20))";
                    action_list_str += "/heroic_leap,use_off_gcd=1,if=debuff.colossus_smash.up";
                    action_list_str += "/deadly_calm,use_off_gcd=1,if=rage>=40";
                    action_list_str += "/heroic_strike,use_off_gcd=1,if=(((debuff.colossus_smash.up&rage>=40)|(buff.deadly_calm.up&rage>=30))&target.health.pct>=20)|rage>=110";
                    action_list_str += "/bloodthirst,if=!(target.health.pct<20&debuff.colossus_smash.up&rage>=30)";
                    action_list_str += "/wild_strike,if=buff.bloodsurge.react&target.health.pct>=20&cooldown.bloodthirst.remains<=1";
                    action_list_str += "/wait,sec=cooldown.bloodthirst.remains,if=!(target.health.pct<20&debuff.colossus_smash.up&rage>=30)&cooldown.bloodthirst.remains<=1";
                    action_list_str += "/colossus_smash";
                    action_list_str += "/execute";
                    action_list_str += "/storm_bolt,if=talent.storm_bolt.enabled";
                    action_list_str += "/raging_blow,if=buff.raging_blow.react";
                    action_list_str += "/wild_strike,if=buff.bloodsurge.react&target.health.pct>=20";
                    action_list_str += "/shockwave,if=talent.shockwave.enabled";
                    action_list_str += "/dragon_roar,if=talent.dragon_roar.enabled";
                    action_list_str += "/heroic_throw";
                    action_list_str += "/battle_shout,if=rage<70&!debuff.colossus_smash.up";
                    action_list_str += "/bladestorm,if=talent.bladestorm.enabled&cooldown.colossus_smash.remains>=5&!debuff.colossus_smash.up&cooldown.bloodthirst.remains>=2&target.health.pct>=20";
                    action_list_str += "/wild_strike,if=debuff.colossus_smash.up&target.health.pct>=20";
                    action_list_str += "/impending_victory,if=talent.impending_victory.enabled&target.health.pct>=20";
                    action_list_str += "/wild_strike,if=cooldown.colossus_smash.remains>=1&rage>=60&target.health.pct>=20";
                    action_list_str += "/battle_shout,if=rage<70";
                }
                else if ( level >= 85 )
                {
                    action_list_str += "/recklessness,use_off_gcd=1,if=((debuff.colossus_smash.remains>=5|cooldown.colossus_smash.remains<=4)&(target.health.pct<20|target.time_to_die>315))|target.time_to_die<=18";
                    action_list_str += "/berserker_rage,use_off_gcd=1,if=!(buff.enrage.react|(buff.raging_blow.react=2&target.health.pct>=20))";
                    action_list_str += "/heroic_leap,use_off_gcd=1,if=debuff.colossus_smash.up";
                    action_list_str += "/deadly_calm,use_off_gcd=1,if=rage>=40";
                    action_list_str += "/heroic_strike,use_off_gcd=1,if=(((debuff.colossus_smash.up&rage>=40)|(buff.deadly_calm.up&rage>=30))&target.health.pct>=20)|rage>=110";
                    action_list_str += "/bloodthirst,if=!(target.health.pct<20&debuff.colossus_smash.up&rage>=30)";
                    action_list_str += "/wild_strike,if=buff.bloodsurge.react&target.health.pct>=20&cooldown.bloodthirst.remains<=1";
                    action_list_str += "/wait,sec=cooldown.bloodthirst.remains,if=!(target.health.pct<20&debuff.colossus_smash.up&rage>=30)&cooldown.bloodthirst.remains<=1";
                    action_list_str += "/colossus_smash";
                    action_list_str += "/execute";
                    action_list_str += "/raging_blow,if=buff.raging_blow.react";
                    action_list_str += "/wild_strike,if=buff.bloodsurge.react&target.health.pct>=20";
                    action_list_str += "/shockwave,if=talent.shockwave.enabled";
                    action_list_str += "/dragon_roar,if=talent.dragon_roar.enabled";
                    action_list_str += "/heroic_throw";
                    action_list_str += "/battle_shout,if=rage<70&!debuff.colossus_smash.up";
                    action_list_str += "/bladestorm,if=talent.bladestorm.enabled&cooldown.colossus_smash.remains>=5&!debuff.colossus_smash.up&cooldown.bloodthirst.remains>=2&target.health.pct>=20";
                    action_list_str += "/wild_strike,if=debuff.colossus_smash.up&target.health.pct>=20";
                    action_list_str += "/impending_victory,if=talent.impending_victory.enabled&target.health.pct>=20";
                    action_list_str += "/wild_strike,if=cooldown.colossus_smash.remains>=1&rage>=60&target.health.pct>=20";
                    action_list_str += "/battle_shout,if=rage<70";
                }
            }
            
            // Protection
            else if ( specialization() == WARRIOR_PROTECTION )
            {
                action_list_str += "/last_stand,if=health<30000";
                action_list_str += "/heroic_strike,if=buff.ultimatum.up,use_off_gcd=1";
                action_list_str += "/berserker_rage,use_off_gcd=1";
                action_list_str += "/shield_slam,if=rage<75";
                action_list_str += "/revenge,if=rage<75";

                action_list_str += "/shield_block";
                action_list_str += "/thunder_clap";
                action_list_str += "/battle_shout,if=rage<80";
                action_list_str += "/devastate";
            }
            
            // Default
            
            action_list_default = 1;
        }
        
        player_t::init_actions();
    }
    
    // warrior_t::combat_begin ==================================================
    
    void warrior_t::combat_begin()
    {
        player_t::combat_begin();
        
        // We (usually) start combat with zero rage.
        resources.current[ RESOURCE_RAGE ] = std::min( initial_rage, 100 );
        
        if ( active_stance == STANCE_BATTLE && ! buff.battle_stance -> check() )
            buff.battle_stance -> trigger();
    }
    
    // warrior_t::reset =========================================================
    
    void warrior_t::reset()
    {
        player_t::reset();
        
        active_stance = STANCE_BATTLE;
    }
    
    // warrior_t::resource_gain =================================================
    
    double warrior_t::resource_gain( resource_e resource_type,
                                    double    amount,
                                    gain_t*   source,
                                    action_t* action )
    {
        // Avatar increaes rage gained from your attacks by x%
        if ( resource_type == RESOURCE_RAGE )
        {
            if ( buff.avatar -> check() )
                amount *= 1.0 + buff.avatar -> data().effectN( 4 ).percent();
        }
        return player_t::resource_gain( resource_type, amount, source, action );
    }
    
    // warrior_t::composite_player_multiplier ===================================
    
    double warrior_t::composite_player_multiplier( school_e school, action_t* a )
    {
        double m = player_t::composite_player_multiplier( school, a );
        
        if ( buff.avatar -> up() )
            m *= 1.0 + buff.avatar -> data().effectN( 1 ).percent();
        
        return m;
    }
    
    // warrior_t::matching_gear_multiplier ======================================
    
    double warrior_t::matching_gear_multiplier( attribute_e attr )
    {
        if ( ( attr == ATTR_STRENGTH ) && ( specialization() == WARRIOR_ARMS || specialization() == WARRIOR_FURY ) )
            return 0.05;
        
        if ( ( attr == ATTR_STAMINA ) && ( specialization() == WARRIOR_PROTECTION ) )
            return 0.05;
        
        return 0.0;
    }
    
    
    
    // warrior_t::composite_tank_block ==========================================
    
    double warrior_t::composite_tank_block()
    {
        double b = player_t::composite_tank_block();
        
        //this looks strange to have it divided here, but not in the next line, yet the values are correct 
        b += composite_mastery() * mastery.critical_block -> effectN( 3 ).percent() / 100.0;
        
        b += spec.bastion_of_defense -> effectN(1).percent();
        if ( buff.shield_block -> up() )
            b = 1.0;
        
        return b;
    }
    
    // warrior_t::composite_tank_crit_block =====================================
    
    double warrior_t::composite_tank_crit_block()
    {
        double b = player_t::composite_tank_crit_block();
        
        b += composite_mastery() * mastery.critical_block -> effectN( 1 ).mastery_value();
        
        return b;
    }
    
    // warrior_t::composite_tank_crit ===========================================
    
    double warrior_t::composite_tank_crit( const school_e school )
    {
        double c = player_t::composite_tank_crit( school );
        
        c += spec.unwavering_sentinel -> effectN( 4 ).percent();
        
        
        if (c<0.0) c=0; //seems like right now the standard tank_crit is set to 0.
        
        return c;
    }
    
    // warrior_t::composite_tank_dodge ===========================================
    
    double warrior_t::composite_tank_dodge()
    {
        double d = player_t::composite_tank_dodge();
        
        d += spec.bastion_of_defense -> effectN(3).percent();
        
        return d;
    }
    
    
    
    // warrior_t::composite_attack_speed ========================================
    
    double warrior_t::composite_attack_speed()
    {
        double s = player_t::composite_attack_speed();
        
        if ( buff.flurry -> up() )
            s *= 1.0 / ( 1.0 + buff.flurry -> data().effectN( 1 ).percent() );
        
        return s;
    }
    
    // warrior_t::regen =========================================================
    
    void warrior_t::regen( timespan_t periodicity )
    {
        player_t::regen( periodicity );
        
        if ( buff.defensive_stance -> check() )
            player_t::resource_gain( RESOURCE_RAGE, ( periodicity.total_seconds() / 3.0 ), gain.defensive_stance );
    }
    
    // warrior_t::primary_role() ================================================
    
    role_e warrior_t::primary_role()
    {
        if ( player_t::primary_role() == ROLE_TANK )
            return ROLE_TANK;
        
        if ( player_t::primary_role() == ROLE_DPS || player_t::primary_role() == ROLE_ATTACK )
            return ROLE_ATTACK;
        
        if ( specialization() == WARRIOR_PROTECTION )
            return ROLE_TANK;
        
        return ROLE_ATTACK;
    }
    
    // warrior_t::assess_damage =================================================
    
    void warrior_t::assess_damage( school_e school,
                                  dmg_e    dtype,
                                  action_state_t* s )
    {
        
        if ( s -> result == RESULT_HIT    ||
            s -> result == RESULT_CRIT   ||
            s -> result == RESULT_GLANCE ||
            s -> result == RESULT_BLOCK  ||
            s -> result == RESULT_CRIT_BLOCK)
        {
            if ( buff.defensive_stance -> check())
                s-> result_amount *= 1.0 + buff.defensive_stance -> data().effectN(1).percent();
            
             warrior_td_t *td= get_target_data(s->action->player);
            
             if (td -> debuffs_demoralizing_shout -> up())
             {
                 s-> result_amount *= 1.0 + td -> debuffs_demoralizing_shout ->value();
             }
            
            //take care of dmg reduction CDs
            if (buff.shield_wall -> up())
                s-> result_amount *= 1.0 + buff.shield_wall -> value();
            
            if (s -> result == RESULT_CRIT_BLOCK)
            {

                if ( cooldown.rage_from_crit_block -> remains() == timespan_t::zero() )
                {
                  cooldown.rage_from_crit_block -> start();
                  resource_gain( RESOURCE_RAGE, buff.enrage -> data().effectN( 1 ).resource( RESOURCE_RAGE ), gain.critical_block );

                  buff.enrage ->trigger();
                }
            }
        }
        
        if ( s -> result == RESULT_DODGE ||
            s -> result == RESULT_PARRY )
        {
            cooldown.revenge -> reset();
        }
        
        if ( s -> result == RESULT_PARRY )
        {
            warrior_t* p = (warrior_t*) s ->target;
            if (p->glyphs.hold_the_line ->ok())
                buff.glyph_hold_the_line -> trigger();
            
            if ( main_hand_attack && main_hand_attack -> execute_event )
            {
                timespan_t swing_time = main_hand_attack -> time_to_execute;
                timespan_t max_reschedule = ( main_hand_attack -> execute_event -> occurs() - 0.20 * swing_time ) - sim -> current_time;
                
                if ( max_reschedule > timespan_t::zero() )
                {
                    main_hand_attack -> reschedule_execute( std::min( ( 0.40 * swing_time ), max_reschedule ) );
                    proc.parry_haste -> occur();
                }
            }
        }
        
        trigger_retaliation( this, school, s -> result );
        
        player_t::assess_damage( school, dtype, s );
    }
    
    // warrior_t::create_options ================================================
    
    void warrior_t::create_options()
    {
        player_t::create_options();
        
        option_t warrior_options[] =
        {
            { "initial_rage",            OPT_INT,  &initial_rage            },
            { NULL, OPT_UNKNOWN, NULL }
        };
        
        option_t::copy( options, warrior_options );
    }
    
    // warrior_t::copy_from =====================================================
    
    void warrior_t::copy_from( player_t* source )
    {
        player_t::copy_from( source );
        
        warrior_t* p = debug_cast<warrior_t*>( source );
        
        initial_rage            = p -> initial_rage;
    }
    
    // warrior_t::decode_set ====================================================
    
    int warrior_t::decode_set( item_t& item )
    {
        if ( item.slot != SLOT_HEAD      &&
            item.slot != SLOT_SHOULDERS &&
            item.slot != SLOT_CHEST     &&
            item.slot != SLOT_HANDS     &&
            item.slot != SLOT_LEGS      )
        {
            return SET_NONE;
        }
        
        const char* s = item.name();
        
        if ( strstr( s, "colossal_dragonplate" ) )
        {
            bool is_melee = ( strstr( s, "helmet"        ) ||
                             strstr( s, "pauldrons"     ) ||
                             strstr( s, "battleplate"   ) ||
                             strstr( s, "legplates"     ) ||
                             strstr( s, "gauntlets"     ) );
            
            bool is_tank = ( strstr( s, "faceguard"      ) ||
                            strstr( s, "shoulderguards" ) ||
                            strstr( s, "chestguard"     ) ||
                            strstr( s, "legguards"      ) ||
                            strstr( s, "handguards"     ) );
            
            if ( is_melee ) return SET_T13_MELEE;
            if ( is_tank  ) return SET_T13_TANK;
        }
        
        if ( strstr( s, "resounding_rings" ) )
        {
            bool is_melee = ( strstr( s, "helmet"        ) ||
                             strstr( s, "pauldrons"     ) ||
                             strstr( s, "battleplate"   ) ||
                             strstr( s, "legplates"     ) ||
                             strstr( s, "gauntlets"     ) );
            
            bool is_tank = ( strstr( s, "faceguard"      ) ||
                            strstr( s, "shoulderguards" ) ||
                            strstr( s, "chestguard"     ) ||
                            strstr( s, "legguards"      ) ||
                            strstr( s, "handguards"     ) );
            
            if ( is_melee ) return SET_T14_MELEE;
            if ( is_tank  ) return SET_T14_TANK;
        }
        
        if ( strstr( s, "_gladiators_plate_"   ) ) return SET_PVP_MELEE;
        
        return SET_NONE;
    }
    
    // WARRIOR MODULE INTERFACE ================================================
    
    struct warrior_module_t : public module_t
    {
        warrior_module_t() : module_t( WARRIOR ) {}
        
        virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE )
        { return new warrior_t( sim, name, r ); }
        
        virtual bool valid() { return true; }
        
        virtual void init( sim_t* sim )
        {
            for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
            {
                player_t* p = sim -> actor_list[ i ];
                p -> debuffs.shattering_throw      = buff_creator_t( p, "shattering_throw", p -> find_spell( 64382 ) )
                .default_value( std::fabs( p -> find_spell( 64382 ) -> effectN( 2 ).percent() ) )
                .cd( timespan_t::zero() );
                p -> buffs.skull_banner  = buff_creator_t( p, "skull_banner", p -> find_spell( 114207 ) )
                .cd( timespan_t::zero() )
                .default_value( p -> find_spell( 114207 ) -> effectN( 1 ).percent() );
            }
        }
        
        virtual void combat_begin( sim_t* ) {}
        
        virtual void combat_end  ( sim_t* ) {}
    };
    
} // UNNAMED NAMESPACE

module_t* module_t::warrior()
{
    static module_t* m = 0;
    if ( ! m ) m = new warrior_module_t();
    return m;
}
