/*
 * Use default action types? If not, need both the buff and spell type for the class.
 * Provide an enum to index on or just drop for unsigned indexing
 * Provide tracking buff spell data
 * Provide tick damage spell data
 * Provide a way to describe guard absorbs
 *
 * Override min threshold map
 * Override spell data map (use spell name for level name, autotokenize this)
 * Override purification type map
 * Override base value
 * Override percent
 */

struct stagger_t
{
  enum stagger_level_e
  {
    NONE_STAGGER     = 3,
    LIGHT_STAGGER    = 2,
    MODERATE_STAGGER = 1,
    HEAVY_STAGGER    = 0,
    MAX_STAGGER      = -1
  };

  struct debuff_t : public actions::monk_buff_t
  {
    debuff_t( monk_t &player, std::string_view name, const spell_data_t *spell );
  };

  struct training_of_niuzao_t : public actions::monk_buff_t
  {
    training_of_niuzao_t( monk_t &player );
    bool trigger_();
  };

  struct self_damage_t : residual_action::residual_periodic_action_t<actions::monk_spell_t>
  {
    using base_t = residual_action::residual_periodic_action_t<actions::monk_spell_t>;

    self_damage_t( monk_t *player );
    proc_types proc_type() const override;
    void impact( action_state_t *state ) override;                                  // add to pool
    void assess_damage( result_amount_type type, action_state_t *state ) override;  // tick from pool
    void last_tick( dot_t *d ) override;                                            // callback on last tick
  };

  struct stagger_level_t
  {
    const stagger_level_e level;
    const double min_percent;
    std::string name;
    std::string name_pretty;
    const spell_data_t *spell_data;
    propagate_const<stagger_t::debuff_t *> debuff;
    sample_data_helper_t *absorbed;
    sample_data_helper_t *taken;
    sample_data_helper_t *mitigated;

    stagger_level_t( stagger_level_e level, monk_t *player );
    static double min_threshold( stagger_level_e level );
    static std::string level_name_pretty( stagger_level_e level );
    static std::string level_name( stagger_level_e level );
    static const spell_data_t *level_spell_data( stagger_level_e level, monk_t *player );
  };

  struct sample_data_t
  {
    sc_timeline_t pool_size;          // raw stagger in pool
    sc_timeline_t pool_size_percent;  // pool as a fraction of current maximum hp
    sc_timeline_t effectiveness;      // stagger effectiveness
    double buffed_base_value;
    double buffed_percent_player_level;
    double buffed_percent_target_level;
    // assert(absorbed == taken + mitigated)
    // assert(sum(mitigated_by_ability) == mitigated)
    sample_data_helper_t *absorbed;
    sample_data_helper_t *taken;
    sample_data_helper_t *mitigated;
    std::unordered_map<std::string, sample_data_helper_t *> mitigated_by_ability;

    sample_data_t( monk_t *player );
  };

  monk_t *player;
  std::vector<stagger_level_t *> stagger_levels;
  stagger_level_t *current;
  training_of_niuzao_t *training_of_niuzao;

  stagger_level_e find_current_level();
  void set_pool( double amount );
  void damage_changed( bool last_tick = false );

  self_damage_t *self_damage;
  sample_data_t *sample_data;
  stagger_t( monk_t *player );

  double base_value();
  double percent( unsigned target_level );
  stagger_level_e current_level();
  double level_index();
  // even though this is an index, it is often used in floating point arithmetic,
  // which makes returning a double preferable to help avoid int arithmetic nonsense

  void add_sample( std::string name, double amount );

  double pool_size();
  double pool_size_percent();
  double tick_size();
  double tick_size_percent();
  bool is_ticking();
  timespan_t remains();

  // TODO: implement a more automated procedure for guard-style absorb handling
  double trigger( school_e school, result_amount_type damage_type, action_state_t *state );
  double purify_flat( double amount, bool report_amount = true );
  double purify_percent( double amount );
  double purify_all();
  void delay_tick( timespan_t delay );
};
