#pragma once
#include "simulationcraft.hpp"

namespace warlock {
    struct warlock_t;

    namespace pets {

    }

    #define MAX_UAS 5

    struct warlock_td_t : public actor_target_data_t {
        dot_t* dots_agony;
        dot_t* dots_corruption;
        dot_t* dots_doom;
        dot_t* dots_drain_life;
        dot_t* dots_immolate;
        dot_t* dots_seed_of_corruption;
        dot_t* dots_shadowflame;
        dot_t* dots_unstable_affliction[MAX_UAS];
        dot_t* dots_siphon_life;
        dot_t* dots_phantom_singularity;
        dot_t* dots_channel_demonfire;

        buff_t* debuffs_haunt;
        buff_t* debuffs_shadowflame;
        buff_t* debuffs_agony;
        buff_t* debuffs_flamelicked;
        buff_t* debuffs_eradication;
        buff_t* debuffs_roaring_blaze;
        buff_t* debuffs_havoc;
        buff_t* debuffs_jaws_of_shadow;
        buff_t* debuffs_tormented_agony;
        buff_t* debuffs_chaotic_flames;

        int agony_stack;
        double soc_threshold;

        warlock_t& warlock;
        warlock_td_t(player_t* target, warlock_t& p);

        void reset() {
            agony_stack = 1;
            soc_threshold = 0;
        }

        void target_demise();
    };

    struct warlock_t : public player_t
    {
    public:
        player_t * havoc_target;
        double agony_accumulator;

        // Active Pet
        struct pets_t {
            pet_t* active;
            pet_t* last;
        } warlock_pet_list;

        std::vector<std::string> pet_name_list;

        struct active_t {
            action_t* demonic_power_proc;
            action_t* cry_havoc;
            action_t* tormented_agony;
            action_t* chaotic_flames;
            spell_t* rain_of_fire;
            spell_t* corruption;
        } active;

        // Talents
        struct talents_t {
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
            const spell_data_t* shadow_embrace;
            const spell_data_t* haunt;
            const spell_data_t* deathbolt;

            const spell_data_t* writhe_in_agony;
            const spell_data_t* absolute_corruption;
            const spell_data_t* deaths_embrace;

            const spell_data_t* sow_the_seeds;
            const spell_data_t* phantom_singularity;
            const spell_data_t* soul_harvest;

            const spell_data_t* nightfall;

            const spell_data_t* creeping_death;
            const spell_data_t* siphon_life;
            // DEMO
            const spell_data_t* demonic_strength;
            const spell_data_t* demonic_calling;
            const spell_data_t* doom;

            const spell_data_t* riders;
            const spell_data_t* power_siphon;
            const spell_data_t* summon_vilefiend;

            const spell_data_t* overloaded;
            const spell_data_t* demonic_strength2;
            const spell_data_t* biliescourge_bombers;

            const spell_data_t* grimoire_of_synergy;
            const spell_data_t* demonic_consumption;
            const spell_data_t* grimoire_of_service;

            const spell_data_t* inner_demons;
            const spell_data_t* nether_portal;
            // DESTRO
            const spell_data_t* eradication;
            const spell_data_t* flashover;
            const spell_data_t* soul_fire;

            const spell_data_t* reverse_entropy;
            const spell_data_t* internal_combustion;
            const spell_data_t* shadowburn;

            const spell_data_t* fire_and_brimstone;
            const spell_data_t* cataclysm;
            const spell_data_t* hellfire;

            const spell_data_t* roaring_blaze;
            const spell_data_t* grimoire_of_supremacy;

            const spell_data_t* channel_demonfire;
            const spell_data_t* dark_soul;
        } talents;

        struct legendary_t
        {
            bool odr_shawl_of_the_ymirjar;
            bool feretory_of_souls;
            bool wilfreds_sigil_of_superior_summoning_flag;
            bool stretens_insanity;
            timespan_t wilfreds_sigil_of_superior_summoning;
            double power_cord_of_lethtendris_chance;
            bool wakeners_loyalty_enabled;
            bool lessons_of_spacetime;
            timespan_t lessons_of_spacetime1;
            timespan_t lessons_of_spacetime2;
            timespan_t lessons_of_spacetime3;
            bool sephuzs_secret;
            double sephuzs_passive;
            bool magistrike;
            bool the_master_harvester;
            bool alythesss_pyrogenics;

        } legendary;

        // Mastery Spells
        struct mastery_spells_t
        {
            const spell_data_t* potent_afflictions;
            const spell_data_t* master_demonologist;
            const spell_data_t* chaotic_energies;
        } mastery_spells;

        //Procs and RNG
        real_ppm_t* affliction_t20_2pc_rppm;
        real_ppm_t* demonic_power_rppm; // grimoire of sacrifice
        real_ppm_t* grimoire_of_synergy; //caster ppm, i.e., if it procs, the wl will create a buff for the pet.
        real_ppm_t* grimoire_of_synergy_pet; //pet ppm, i.e., if it procs, the pet will create a buff for the wl.

                                          // Cooldowns
        struct cooldowns_t {
            cooldown_t* haunt;
            cooldown_t* sindorei_spite_icd;
        } cooldowns;

        // Passives
        struct specs_t {
            // All Specs
            const spell_data_t* fel_armor;
            const spell_data_t* nethermancy;

            // Affliction only
            const spell_data_t* affliction;
            const spell_data_t* nightfall;
            const spell_data_t* unstable_affliction;
            const spell_data_t* unstable_affliction_2;
            const spell_data_t* agony;
            const spell_data_t* agony_2;
            const spell_data_t* shadow_bite;
            const spell_data_t* shadow_bite_2;

            // Demonology only
            const spell_data_t* demonology;
            const spell_data_t* doom;
            const spell_data_t* wild_imps;

            // Destruction only
            const spell_data_t* destruction;
            const spell_data_t* immolate;
            const spell_data_t* conflagrate;
            const spell_data_t* conflagrate_2;
            const spell_data_t* unending_resolve;
            const spell_data_t* unending_resolve_2;
            const spell_data_t* firebolt;
            const spell_data_t* firebolt_2;
        } spec;

        // Buffs
        struct buffs_t  {
            buff_t* demonic_power;
            buff_t* soul_harvest;

            //affliction buffs
            buff_t* active_uas;
            buff_t* demonic_speed; // t20 4pc

            //demonology buffs
            buff_t* demonic_synergy;
            buff_t* dreaded_haste; // t20 4pc
            buff_t* rage_of_guldan; // t21 2pc

            //destruction_buffs
            buff_t* backdraft;
            buff_t* conflagration_of_chaos;
            buff_t* embrace_chaos;
            buff_t* active_havoc;

            // legendary buffs
            buff_t* sindorei_spite;
            buff_t* stretens_insanity;
            buff_t* lessons_of_spacetime;
            haste_buff_t* sephuzs_secret;
            buff_t* alythesss_pyrogenics;
            buff_t* wakeners_loyalty;
        } buffs;

        // Gains
        struct gains_t {
            gain_t* agony;
            gain_t* conflagrate;
            gain_t* shadowburn;
            gain_t* immolate;
            gain_t* immolate_crits;
            gain_t* shadowburn_shard;
            gain_t* miss_refund;
            gain_t* seed_of_corruption;
            gain_t* unstable_affliction_refund;
            gain_t* power_trip;
            gain_t* shadow_bolt;
            gain_t* doom;
            gain_t* soul_conduit;
            gain_t* reverse_entropy;
            gain_t* soulsnatcher;
            gain_t* t19_2pc_demonology;
            gain_t* recurrent_ritual;
            gain_t* feretory_of_souls;
            gain_t* power_cord_of_lethtendris;
            gain_t* incinerate;
            gain_t* incinerate_crits;
            gain_t* dimensional_rift;
            gain_t* affliction_t20_2pc;
            gain_t* destruction_t20_2pc;
        } gains;

        // Procs
        struct procs_t {
            proc_t* soul_conduit;
            proc_t* the_master_harvester;
            //aff
            proc_t* affliction_t21_2pc;
            //demo
            proc_t* demonic_calling;
            proc_t* power_trip;
            proc_t* souls_consumed;
            proc_t* one_shard_hog;
            proc_t* two_shard_hog;
            proc_t* three_shard_hog;
            proc_t* four_shard_hog;
            proc_t* wild_imp;
            proc_t* fragment_wild_imp;
            proc_t* demonology_t20_2pc;
            //destro
            proc_t* t19_2pc_chaos_bolts;
        } procs;

        struct spells_t
        {
            spell_t* melee;
            spell_t* seed_of_corruption_aoe;
            spell_t* implosion_aoe;
        } spells;

        int initial_soul_shards;
        bool allow_sephuz;
        bool deaths_embrace_fixed_time;
        std::string default_pet;

        timespan_t shard_react;

        warlock_t(sim_t* sim, const std::string& name, race_e r = RACE_UNDEAD);

        // Character Definition
        virtual void      init_spells() override;
        virtual void      init_base_stats() override;
        virtual void      init_scaling() override;
        virtual void      create_buffs() override;
        virtual void      init_gains() override;
        virtual void      init_procs() override;
        virtual void      init_rng() override;
        virtual void      init_action_list() override;
        virtual void      init_resources(bool force) override;
        virtual void      reset() override;
        virtual void      create_options() override;
        virtual action_t* create_action(const std::string& name, const std::string& options) override;
        bool create_actions() override;
        virtual pet_t*    create_pet(const std::string& name, const std::string& type = std::string()) override;
        virtual void      create_pets() override;
        virtual std::string      create_profile(save_e = SAVE_ALL) override;
        virtual void      copy_from(player_t* source) override;
        virtual resource_e primary_resource() const override { return RESOURCE_MANA; }
        virtual role_e    primary_role() const override { return ROLE_SPELL; }
        virtual stat_e    convert_hybrid_stat(stat_e s) const override;
        virtual stat_e    primary_stat() const override { return STAT_INTELLECT; }
        virtual double    matching_gear_multiplier(attribute_e attr) const override;
        virtual double    composite_player_multiplier(school_e school) const override;
        virtual double    composite_player_target_multiplier(player_t* target, school_e school) const override;
        virtual double    composite_rating_multiplier(rating_e rating) const override;
        virtual void      invalidate_cache(cache_e) override;
        virtual double    composite_spell_crit_chance() const override;
        virtual double    composite_spell_haste() const override;
        virtual double    composite_melee_haste() const override;
        virtual double    composite_melee_crit_chance() const override;
        virtual double    composite_mastery() const override;
        virtual double    resource_gain(resource_e, double, gain_t* = nullptr, action_t* = nullptr) override;
        virtual double    mana_regen_per_second() const override;
        virtual double    composite_armor() const override;

        virtual void      halt() override;
        virtual void      combat_begin() override;
        virtual expr_t*   create_expression(action_t* a, const std::string& name_str) override;

        target_specific_t<warlock_td_t> target_data;

        virtual warlock_td_t* get_target_data(player_t* target) const override
        {
            warlock_td_t*& td = target_data[target];
            if (!td)
            {
                td = new warlock_td_t(target, const_cast<warlock_t&>(*this));
            }
            return td;
        }

        // sc_warlock_affliction
        action_t* warlock_t::create_action_affliction(const std::string& action_name, const std::string& options_str);
        void create_buffs_affliction();
        void init_spells_affliction();
        void init_gains_affliction();
        void init_rng_affliction();
        void init_procs_affliction();
        void create_options_affliction();
        void create_apl_affliction();
        virtual void legendaries_affliction();

        // sc_warlock_demonology
        action_t* warlock_t::create_action_demonology(const std::string& action_name, const std::string& options_str);
        pet_t* warlock_t::create_pet_demonology(const std::string& pet_name, const std::string& );
        void create_buffs_demonology();
        void init_spells_demonology();
        void init_gains_demonology();
        void init_rng_demonology();
        void init_procs_demonology();
        void create_options_demonology();
        void create_apl_demonology();
        virtual void legendaries_demonology();

        // sc_warlock_destruction
        action_t* warlock_t::create_action_destruction(const std::string& action_name, const std::string& options_str);
        void create_buffs_destruction();
        void init_spells_destruction();
        void init_gains_destruction();
        void init_rng_destruction();
        void init_procs_destruction();
        void create_options_destruction();
        void create_apl_destruction();
        virtual void legendaries_destruction();

        std::string       default_potion() const override;
        std::string       default_flask() const override;
        std::string       default_food() const override;
        std::string       default_rune() const override;

    private:
        void apl_precombat();
        void apl_default();
        void apl_global_filler();
    };

    void parse_spell_coefficient(action_t& a);

    namespace pets {
        struct warlock_pet_t : public pet_t {
            action_t* special_action;
            action_t* special_action_two;
            melee_attack_t* melee_attack;
            stats_t* summon_stats;
            spell_t *ascendance;

            warlock_pet_t(sim_t* sim, warlock_t* owner, const std::string& pet_name, pet_e pt, bool guardian = false);
            virtual void init_base_stats() override;
            virtual void init_action_list() override;
            virtual void create_buffs() override;
            virtual bool create_actions() override;
            virtual void schedule_ready(timespan_t delta_time = timespan_t::zero(),
                bool   waiting = false) override;
            virtual double composite_player_multiplier(school_e school) const override;
            virtual double composite_melee_crit_chance() const override;
            virtual double composite_spell_crit_chance() const override;
            virtual double composite_melee_haste() const override;
            virtual double composite_spell_haste() const override;
            virtual double composite_melee_speed() const override;
            virtual double composite_spell_speed() const override;
            virtual resource_e primary_resource() const override { return RESOURCE_ENERGY; }
            warlock_t* o()
            {
                return static_cast<warlock_t*>(owner);
            }
            const warlock_t* o() const
            {
                return static_cast<warlock_t*>(owner);
            }

            struct buffs_t
            {
                buff_t* demonic_synergy;
                haste_buff_t* demonic_empowerment;
                buff_t* the_expendables;
                buff_t* rage_of_guldan;
            } buffs;

            bool is_grimoire_of_service = false;
            bool is_demonbolt_enabled = true;
            bool is_lord_of_flames = false;
            bool t21_4pc_reset = false;
            bool is_warlock_pet = true;
            int bites_executed = 0;
            int dreadbite_executes = 0;

            void trigger_sephuzs_secret(const action_state_t* state, spell_mechanic mechanic)
            {
                if (!o()->legendary.sephuzs_secret)
                    return;

                // trigger by default on interrupts and on adds/lower level stuff
                if (o()->allow_sephuz || mechanic == MECHANIC_INTERRUPT || state->target->is_add() ||
                    (state->target->level() < o()->sim->max_player_level + 3))
                {
                    o()->buffs.sephuzs_secret->trigger();
                }
            }

            struct travel_t : public action_t
            {
                travel_t(player_t* player) : action_t(ACTION_OTHER, "travel", player) {
                    trigger_gcd = timespan_t::zero();
                }
                void execute() override { player->current.distance = 1; }
                timespan_t execute_time() const override { return timespan_t::from_seconds(player->current.distance / 33.0); }
                bool ready() override { return (player->current.distance > 1); }
                bool usable_moving() const override { return true; }
            };

            action_t* create_action(const std::string& name,
                const std::string& options_str) override
            {
                if (name == "travel") return new travel_t(this);

                return pet_t::create_action(name, options_str);
            }
        };
        // Template for common warlock pet action code. See priest_action_t.
        template <class ACTION_BASE>
        struct warlock_pet_action_t : public ACTION_BASE
        {
        public:
        private:
            typedef ACTION_BASE ab; // action base, eg. spell_t
        public:
            typedef warlock_pet_action_t base_t;

            warlock_pet_action_t(const std::string& n, warlock_pet_t* p,
                const spell_data_t* s = spell_data_t::nil()) :
                ab(n, p, s)
            {
                ab::may_crit = true;

                // If pets are not reported separately, create single stats_t objects for the various pet
                // abilities.
                if (!ab::sim->report_pets_separately)
                {
                    auto first_pet = p->owner->find_pet(p->name_str);
                    if (first_pet != nullptr && first_pet != p)
                    {
                        auto it = range::find(p->stats_list, ab::stats);
                        if (it != p->stats_list.end())
                        {
                            p->stats_list.erase(it);
                            delete ab::stats;
                            ab::stats = first_pet->get_stats(ab::name_str, this);
                        }
                    }
                }
            }
            virtual ~warlock_pet_action_t() {}

            warlock_pet_t* p()
            {
                return static_cast<warlock_pet_t*>(ab::player);
            }
            const warlock_pet_t* p() const
            {
                return static_cast<warlock_pet_t*>(ab::player);
            }

            virtual void execute()
            {
                ab::execute();

                // Some aoe pet abilities can actually reduce to 0 targets, so bail out early if we hit that
                // situation
                if (ab::n_targets() != 0 && ab::target_list().size() == 0)
                {
                    return;
                }

                if (ab::hit_any_target && ab::result_is_hit(ab::execute_state->result))
                {
                    if (p()->o()->talents.grimoire_of_synergy->ok())
                    {
                        bool procced = p()->o()->grimoire_of_synergy_pet->trigger(); //check for RPPM
                        if (procced) p()->o()->buffs.demonic_synergy->trigger(); //trigger the buff
                    }
                }
            }

            warlock_td_t* td(player_t* t) const
            {
                return p()->o()->get_target_data(t);
            }


        };

        struct warlock_pet_melee_t : public warlock_pet_action_t<melee_attack_t>
        {
            struct off_hand_swing : public warlock_pet_action_t<melee_attack_t>
            {
                off_hand_swing(warlock_pet_t* p, const char* name = "melee_oh") :
                    warlock_pet_action_t<melee_attack_t>(name, p, spell_data_t::nil())
                {
                    school = SCHOOL_PHYSICAL;
                    weapon = &(p->off_hand_weapon);
                    base_execute_time = weapon->swing_time;
                    may_crit = true;
                    background = true;
                    base_multiplier = 0.5;
                }
            };

            off_hand_swing* oh;

            warlock_pet_melee_t(warlock_pet_t* p, const char* name = "melee") :
                warlock_pet_action_t<melee_attack_t>(name, p, spell_data_t::nil()), oh(nullptr)
            {
                school = SCHOOL_PHYSICAL;
                weapon = &(p->main_hand_weapon);
                base_execute_time = weapon->swing_time;
                may_crit = background = repeating = true;

                if (p->dual_wield())
                    oh = new off_hand_swing(p);
            }

            virtual void execute() override
            {
                if (!player->executing && !player->channeling)
                {
                    melee_attack_t::execute();
                    if (oh)
                    {
                        oh->time_to_execute = time_to_execute;
                        oh->execute();
                    }
                }
                else
                {
                    schedule_execute();
                }
            }
        };

        struct warlock_pet_melee_attack_t : public warlock_pet_action_t < melee_attack_t >
        {
        private:
            void _init_warlock_pet_melee_attack_t()
            {
                weapon = &(player->main_hand_weapon);
                special = true;
            }

        public:
            warlock_pet_melee_attack_t(warlock_pet_t* p, const std::string& n) :
                base_t(n, p, p -> find_pet_spell(n))
            {
                _init_warlock_pet_melee_attack_t();
            }

            warlock_pet_melee_attack_t(const std::string& token, warlock_pet_t* p, const spell_data_t* s = spell_data_t::nil()) :
                base_t(token, p, s)
            {
                _init_warlock_pet_melee_attack_t();
            }
        };

        struct warlock_pet_spell_t : public warlock_pet_action_t < spell_t >
        {
        private:
            void _init_warlock_pet_spell_t()
            {
                parse_spell_coefficient(*this);
            }

        public:
            warlock_pet_spell_t(warlock_pet_t* p, const std::string& n) :
                base_t(n, p, p -> find_pet_spell(n))
            {
                _init_warlock_pet_spell_t();
            }

            warlock_pet_spell_t(const std::string& token, warlock_pet_t* p, const spell_data_t* s = spell_data_t::nil()) :
                base_t(token, p, s)
            {
                _init_warlock_pet_spell_t();
            }
        };

        namespace felhunter {
            struct felhunter_pet_t : public warlock_pet_t {
                felhunter_pet_t(sim_t* sim, warlock_t* owner, const std::string& name = "felhunter");

                virtual void init_base_stats() override;

                virtual action_t* create_action(const std::string& name, const std::string& options_str) override;
            };
        }
    }

    namespace actions {
        struct warlock_heal_t
            : public heal_t
        {
            warlock_heal_t(const std::string& n, warlock_t* p, const uint32_t id) :
                heal_t(n, p, p -> find_spell(id))
            {
                target = p;
            }

            warlock_t* p()
            {
                return static_cast<warlock_t*>(player);
            }
        };

        struct warlock_spell_t : public spell_t
        {
        private:
            void _init_warlock_spell_t()
            {
                may_crit = true;
                tick_may_crit = true;
                weapon_multiplier = 0.0;
                gain = player->get_gain(name_str);

                can_havoc = false;
                affected_by_destruction_t20_4pc = false;
                affected_by_deaths_embrace = false;
                destro_mastery = true;
                can_feretory = true;

                parse_spell_coefficient(*this);
            }

        public:
            gain_t * gain;

            mutable std::vector< player_t* > havoc_targets;
            bool can_havoc;
            bool affected_by_destruction_t20_4pc;
            bool affected_by_flamelicked;
            bool affected_by_odr_shawl_of_the_ymirjar;
            bool affected_by_deaths_embrace;
            bool affliction_direct_increase;
            bool affliction_dot_increase;
            bool destruction_direct_increase;
            bool destruction_dot_increase;
            bool destro_mastery;
            bool can_feretory;

            warlock_spell_t(warlock_t* p, const std::string& n) :
                spell_t(n, p, p -> find_class_spell(n))
            {
                _init_warlock_spell_t();
            }

            warlock_spell_t(const std::string& token, warlock_t* p, const spell_data_t* s = spell_data_t::nil()) :
                spell_t(token, p, s)
            {
                _init_warlock_spell_t();
            }

            warlock_t* p()
            {
                return static_cast<warlock_t*>(player);
            }
            const warlock_t* p() const
            {
                return static_cast<warlock_t*>(player);
            }

            warlock_td_t* td(player_t* t) const
            {
                return p()->get_target_data(t);
            }

            bool use_havoc() const
            {
                if (!p()->havoc_target || target == p()->havoc_target || !can_havoc)
                    return false;

                return true;
            }

            void reset() override
            {
                spell_t::reset();
            }

            void init() override
            {
                action_t::init();

                affected_by_flamelicked = false;

                affected_by_odr_shawl_of_the_ymirjar = data().affected_by(p()->find_spell(212173)->effectN(1));
                destruction_direct_increase = data().affected_by(p()->spec.destruction->effectN(1));
                destruction_dot_increase = data().affected_by(p()->spec.destruction->effectN(2));
                if (destruction_direct_increase)
                    base_dd_multiplier *= 1.0 + p()->spec.destruction->effectN(1).percent();
                if (destruction_dot_increase)
                    base_td_multiplier *= 1.0 + p()->spec.destruction->effectN(2).percent();

                affliction_direct_increase = data().affected_by(p()->spec.affliction->effectN(1));
                affliction_dot_increase = data().affected_by(p()->spec.affliction->effectN(2));
                if (affliction_direct_increase)
                    base_dd_multiplier *= 1.0 + p()->spec.affliction->effectN(1).percent();
                if (affliction_dot_increase)
                    base_td_multiplier *= 1.0 + p()->spec.affliction->effectN(2).percent();
            }

            int n_targets() const override
            {
                if (aoe == 0 && use_havoc())
                    return 2;

                return spell_t::n_targets();
            }

            std::vector< player_t* >& target_list() const override
            {
                if (use_havoc())
                {
                    if (!target_cache.is_valid)
                    {
                        available_targets(target_cache.list);
                        check_distance_targeting(target_cache.list);
                        target_cache.is_valid = true;
                    }

                    havoc_targets.clear();
                    if (range::find(target_cache.list, target) != target_cache.list.end())
                        havoc_targets.push_back(target);

                    if (!p()->havoc_target->is_sleeping() &&
                        range::find(target_cache.list, p()->havoc_target) != target_cache.list.end())
                        havoc_targets.push_back(p()->havoc_target);
                    return havoc_targets;
                }
                else
                    return spell_t::target_list();
            }

            double cost() const override
            {
                double c = spell_t::cost();
                return c;
            }

            void execute() override {
                spell_t::execute();

                if (hit_any_target && result_is_hit(execute_state->result) && p()->talents.grimoire_of_synergy->ok())
                {
                    pets::warlock_pet_t* my_pet = static_cast<pets::warlock_pet_t*>(p()->warlock_pet_list.active); //get active pet
                    if (my_pet != nullptr)
                    {
                        bool procced = p()->grimoire_of_synergy->trigger();
                        if (procced) my_pet->buffs.demonic_synergy->trigger();
                    }
                }
                if (hit_any_target && result_is_hit(execute_state->result) && p()->talents.grimoire_of_sacrifice->ok() && p()->buffs.demonic_power->up())
                {
                    bool procced = p()->demonic_power_rppm->trigger();
                    if (procced)
                    {
                        p()->active.demonic_power_proc->target = execute_state->target;
                        p()->active.demonic_power_proc->execute();
                    }
                }

                p()->buffs.demonic_synergy->up();

                if (can_feretory && p()->legendary.feretory_of_souls && rng().roll(p()->find_spell(205702)->proc_chance()) && dbc::is_school(school, SCHOOL_FIRE))
                {
                    p()->resource_gain(RESOURCE_SOUL_SHARD, 1.0, p()->gains.feretory_of_souls);
                }
            }

            void consume_resource() override
            {
                spell_t::consume_resource();

                if (resource_current == RESOURCE_SOUL_SHARD && p()->in_combat)
                {
                    if (p()->legendary.the_master_harvester)
                    {
                        timespan_t sh_duration = timespan_t::from_seconds(p()->find_spell(248113)->effectN(4).base_value());
                        double sh_proc_chance;
                        switch (p()->specialization())
                        {
                        case WARLOCK_AFFLICTION:
                            sh_proc_chance = p()->find_spell(248113)->effectN(1).percent();
                            break;
                        case WARLOCK_DEMONOLOGY:
                            sh_proc_chance = p()->find_spell(248113)->effectN(2).percent();
                            break;
                        case WARLOCK_DESTRUCTION:
                            sh_proc_chance = p()->find_spell(248113)->effectN(3).percent();
                            break;
                        default:
                            sh_proc_chance = 0;
                            break;
                        }

                        for (int i = 0; i < last_resource_cost; i++)
                        {
                            if (p()->rng().roll(sh_proc_chance))
                            {
                                p()->buffs.soul_harvest->trigger(1, 0.2, -1.0, sh_duration);
                                p()->procs.the_master_harvester->occur();
                            }
                        }

                    }

                    if (p()->talents.soul_conduit->ok())
                    {

                        if (p()->specialization() == WARLOCK_DEMONOLOGY)
                        {
                            struct demo_sc_event : public player_event_t
                            {
                                gain_t* shard_gain;
                                warlock_t* pl;
                                int shards_used;


                                demo_sc_event(warlock_t* p, int c) :
                                    player_event_t(*p, timespan_t::from_millis(100)), shard_gain(p -> gains.soul_conduit), pl(p), shards_used(c)
                                {
                                }
                                virtual const char* name() const override
                                {
                                    return "demonology_sc_event";
                                }
                                virtual void execute() override
                                {
                                    double soul_conduit_rng = pl->talents.soul_conduit->effectN(1).percent() + pl->spec.destruction->effectN(4).percent();

                                    for (int i = 0; i < shards_used; i++)
                                    {
                                        if (rng().roll(soul_conduit_rng))
                                        {
                                            pl->resource_gain(RESOURCE_SOUL_SHARD, 1.0, pl->gains.soul_conduit);
                                            pl->procs.soul_conduit->occur();
                                        }
                                    }

                                }
                            };

                            make_event<demo_sc_event>(*p()->sim, p(), last_resource_cost);


                        }
                        else
                        {
                            double soul_conduit_rng = p()->talents.soul_conduit->effectN(1).percent() + p()->spec.destruction->effectN(4).percent();

                            for (int i = 0; i < last_resource_cost; i++)
                            {
                                if (rng().roll(soul_conduit_rng))
                                {
                                    p()->resource_gain(RESOURCE_SOUL_SHARD, 1.0, p()->gains.soul_conduit);
                                    p()->procs.soul_conduit->occur();
                                }
                            }
                        }
                    }
                    if (p()->legendary.wakeners_loyalty_enabled && p()->specialization() == WARLOCK_DEMONOLOGY)
                    {
                        for (int i = 0; i < last_resource_cost; i++)
                        {
                            p()->buffs.wakeners_loyalty->trigger();
                        }
                    }

                    if (p()->specialization() == WARLOCK_AFFLICTION && p()->sets->has_set_bonus(WARLOCK_AFFLICTION, T20, B4))
                    {
                        p()->buffs.demonic_speed->trigger();
                    }
                }
            }

            void tick(dot_t* d) override
            {
                spell_t::tick(d);

                if (d->state->result > 0 && result_is_hit(d->state->result) && td(d->target)->dots_seed_of_corruption->is_ticking()
                    && id != p()->spells.seed_of_corruption_aoe->id)
                {
                    accumulate_seed_of_corruption(td(d->target), d->state->result_amount);
                }

                p()->buffs.demonic_synergy->up();
            }

            void impact(action_state_t* s) override
            {
                spell_t::impact(s);

                if (s->result_amount > 0 && result_is_hit(s->result) && td(s->target)->dots_seed_of_corruption->is_ticking()
                    && id != p()->spells.seed_of_corruption_aoe->id)
                {
                    accumulate_seed_of_corruption(td(s->target), s->result_amount);
                }
            }

            double composite_target_multiplier(player_t* t) const override
            {
                double m = 1.0;

                warlock_td_t* td = this->td(t);

                if (td->debuffs_eradication->check())
                    m *= 1.0 + p()->find_spell(196414)->effectN(1).percent();

                if (target == p()->havoc_target && affected_by_odr_shawl_of_the_ymirjar && p()->legendary.odr_shawl_of_the_ymirjar)
                    m *= 1.0 + p()->find_spell(212173)->effectN(1).percent();

                double deaths_embrace_health = p()->talents.deaths_embrace->effectN(2).base_value();

                if (p()->talents.deaths_embrace->ok() && target->health_percentage() <= deaths_embrace_health && affected_by_deaths_embrace)
                {
                    m *= 1.0 + p()->talents.deaths_embrace->effectN(1).percent() * (1 - target->health_percentage() / deaths_embrace_health);
                }

                return spell_t::composite_target_multiplier(t) * m;
            }

            double action_multiplier() const override {
                double pm = spell_t::action_multiplier();

                if (p()->mastery_spells.chaotic_energies->ok() && destro_mastery) {
                    double destro_mastery_value = p()->cache.mastery_value() / 2.0;
                    double chaotic_energies_rng;

                    if (p()->sets->has_set_bonus(WARLOCK_DESTRUCTION, T20, B4) && affected_by_destruction_t20_4pc)
                        chaotic_energies_rng = destro_mastery_value;
                    else
                        chaotic_energies_rng = rng().range(0, destro_mastery_value);

                    pm *= 1.0 + chaotic_energies_rng + (destro_mastery_value);
                }

                return pm;
            }

            resource_e current_resource() const override {
                return spell_t::current_resource();
            }

            double composite_target_crit_chance(player_t* target) const override {
                double c = spell_t::composite_target_crit_chance(target);
                return c;
            }

            bool consume_cost_per_tick(const dot_t& dot) override {
                bool consume = spell_t::consume_cost_per_tick(dot);
                return consume;
            }

            void extend_dot(dot_t* dot, timespan_t extend_duration) {
                if (dot->is_ticking()) {
                    dot->extend_duration(extend_duration, dot->current_action->dot_duration * 1.5);
                }
            }

            static void accumulate_seed_of_corruption(warlock_td_t* td, double amount) {
                td->soc_threshold -= amount;

                if (td->soc_threshold <= 0)
                    td->dots_seed_of_corruption->cancel();
            }

            /*
            static void trigger_wild_imp(warlock_t* p, bool doge = false, int duration = 12001)
            {
                for (size_t i = 0; i < p->warlock_pet_list.wild_imps.size(); i++)
                {
                    if (p->warlock_pet_list.wild_imps[i]->is_sleeping())
                    {

                        p->warlock_pet_list.wild_imps[i]->trigger(duration, doge);
                        p->procs.wild_imp->occur();
                        if (p->legendary.wilfreds_sigil_of_superior_summoning_flag && !p->talents.grimoire_of_supremacy->ok())
                        {
                            p->cooldowns.doomguard->adjust(p->legendary.wilfreds_sigil_of_superior_summoning);
                            p->cooldowns.infernal->adjust(p->legendary.wilfreds_sigil_of_superior_summoning);
                            p->procs.wilfreds_imp->occur();
                        }
                        return;
                    }
                }
            }
            */
        };

        typedef residual_action::residual_periodic_action_t< warlock_spell_t > residual_action_t;

        struct summon_pet_t : public warlock_spell_t {
            timespan_t summoning_duration;
            std::string pet_name;
            pets::warlock_pet_t* pet;

        private:
            void _init_summon_pet_t() {
                util::tokenize(pet_name);
                harmful = false;

                if (data().ok() &&
                    std::find(p()->pet_name_list.begin(), p()->pet_name_list.end(), pet_name) ==
                    p()->pet_name_list.end()) {
                    p()->pet_name_list.push_back(pet_name);
                }
            }

        public:
            summon_pet_t(const std::string& n, warlock_t* p, const std::string& sname = "") :
                warlock_spell_t(p, sname.empty() ? "Summon " + n : sname),
                summoning_duration(timespan_t::zero()),
                pet_name(sname.empty() ? n : sname), pet(nullptr) {
                _init_summon_pet_t();
            }

            summon_pet_t(const std::string& n, warlock_t* p, int id) :
                warlock_spell_t(n, p, p -> find_spell(id)),
                summoning_duration(timespan_t::zero()),
                pet_name(n), pet(nullptr) {
                _init_summon_pet_t();
            }

            summon_pet_t(const std::string& n, warlock_t* p, const spell_data_t* sd) :
                warlock_spell_t(n, p, sd),
                summoning_duration(timespan_t::zero()),
                pet_name(n), pet(nullptr) {
                _init_summon_pet_t();
            }

            bool init_finished() override {
                pet = debug_cast<pets::warlock_pet_t*>(player->find_pet(pet_name));
                return warlock_spell_t::init_finished();
            }

            virtual void execute() override {
                pet->summon(summoning_duration);
                warlock_spell_t::execute();
            }

            bool ready() override {
                if (!pet) {
                    return false;
                }
                return warlock_spell_t::ready();
            }
        };

        struct summon_main_pet_t : public summon_pet_t {
            cooldown_t* instant_cooldown;

            summon_main_pet_t(const std::string& n, warlock_t* p) :
                summon_pet_t(n, p), instant_cooldown(p -> get_cooldown("instant_summon_pet")) {
                instant_cooldown->duration = timespan_t::from_seconds(60);
                ignore_false_positive = true;
            }

            virtual void schedule_execute(action_state_t* state = nullptr) override {
                warlock_spell_t::schedule_execute(state);

                if (p()->warlock_pet_list.active) {
                    p()->warlock_pet_list.active->dismiss();
                    p()->warlock_pet_list.active = nullptr;
                }
            }

            virtual bool ready() override {
                if (p()->warlock_pet_list.active == pet)
                    return false;

                if (p()->talents.grimoire_of_supremacy->ok()) //if we have the uberpets, we can't summon our standard pets
                    return false;
                return summon_pet_t::ready();
            }

            virtual void execute() override {
                summon_pet_t::execute();

                p()->warlock_pet_list.active = p()->warlock_pet_list.last = pet;

                if (p()->buffs.demonic_power->check())
                    p()->buffs.demonic_power->expire();
            }
        };
    }

    namespace buffs {
        template <typename Base>
        struct warlock_buff_t : public Base {
        public:
            typedef warlock_buff_t base_t;
            warlock_buff_t(warlock_td_t& p, const buff_creator_basics_t& params) :
                Base(params), warlock(p.warlock)
            {}

            warlock_buff_t(warlock_t& p, const buff_creator_basics_t& params) :
                Base(params), warlock(p)
            {}

            warlock_td_t& get_td(player_t* t) const
            {
                return *(warlock.get_target_data(t));
            }

        protected:
            warlock_t & warlock;
        };
    }
}
