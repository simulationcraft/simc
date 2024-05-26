#include "action/residual_action.hpp"
#include "action/spell.hpp"
#include "buff/buff.hpp"
#include "dbc/spell_data.hpp"
#include "player/player.hpp"
#include "report/charts.hpp"
#include "report/highchart.hpp"
#include "sim/sim.hpp"
#include "util/generic.hpp"
#include "util/io.hpp"
#include "util/timeline.hpp"
#include "util/timespan.hpp"
#include "util/util.hpp"

#include <memory>
#include <vector>

/*
 * sc_stagger.hpp:stagger_data_t
 *  - provide the following data:
 *     1. std::string_view name; stagger object name (used as a key in stagger
 *        map and for statistics)
 *     2. std::function<bool()> active; enable stagger object condition (talent,
 *        buff state, etc)
 *     3. std::function<bool(school_e, result_amount_type, action_state_t*)> trigger;
 *        validate whether or not stagger_t->trigger(...) should occur
 *     4. std::vector<stagger_level_data_t*> levels; see sc_stagger.hpp:stagger_level_data_t
 *     5. const spell_data_t *self_damage; self damage spell
 *     6. std::vector<std::string_view> mitigation_tokens; list of tokenized
 *        names of all purification spells
 */

/*
 * sc_stagger.hpp:stagger_level_data_t
 *  - provide the following data:
 *     1. const spell_data_t *spell_data; spell data for level debuff. must be non-nil
 *     2. const double min_threshold; minimum value for stagger threshold to be active
 *  - it is expected for there to be 2 additional meta-levels
 *     1. `none` -> no stagger debuff level ( min_threshold = -1.0, spell_data = nil )
 *     2. `maximum` -> maximum stagger level, all stagger overflows ( min_threshold = m, spell_data = nil )
 */

/*
 * FAQ:
 *  - How do I create my own stagger type?
 *     Follow the steps to create a valid `stagger_data_t` object in
 *     `player_t::create_buffs` after base, or any point after during init if you
 *     wish to initalize yourself, otherwise do so before base `player_t::create_buffs`.
 *     Initialize the `stagger_t` object in the `stagger_data_t` object with the
 *     `stagger_data_t::init_stagger` method. Optionally, provide a derived `stagger_impl::stagger_t`
 *     type.
 *  - How do I access methods on my stagger type?
 *     Cast the `stagger_data_t::stagger_t` object to your type. The default interface
 *     should be sufficient in practically all cases.
 *  - I need to modify the level debuffs.
 *  - I need to modify the self damage.
 *  - I need to modify purification behaviour.
 *  - I need to modify tick behaviour.
 *  - I need to modify reporting.
 */

/*
 * Use default action types? If not, need both the buff and spell type for the class.
 * Provide an enum to index on or just drop for unsigned indexing
 * Provide tracking buff spell data
 * Provide tick damage spell data
 * Provide a way to describe guard absorbs
 * Make purification more ergonomic
 * Snapshot_stats helper
 * Collect_resource_timeline helper
 * Pre_analyze_hook helper
 * Combat_begin helper
 * assess_damage_imminent_pre_absorb helper (or turn it into a fractional absorb buff)
 * create_exprn helper
 * merge helper
 * html_customsection helper
 *  - register a vector of custom sections in player_t, which mixins can hook into
 *
 * Override min threshold map
 * Override spell data map (use spell name for level name, autotokenize this)
 * Override purification type map
 * Override base value
 * Override percent
 *
 * TODO: How are `default` stagger types provided (ala undulating maneuvers)
 */
// {}

// struct stagger_p_t
// {
//   enum stagger_level_e
//   {
//     NONE_STAGGER     = 3,
//     LIGHT_STAGGER    = 2,
//     MODERATE_STAGGER = 1,
//     HEAVY_STAGGER    = 0,
//     MAX_STAGGER      = -1
//   };

//   struct debuff_t : public actions::monk_buff_t
//   {
//     debuff_t( monk_t &player, std::string_view name, const spell_data_t *spell );
//   };

//   struct training_of_niuzao_t : public actions::monk_buff_t
//   {
//     training_of_niuzao_t( monk_t &player );
//     bool trigger_();
//   };

//   struct self_damage_t : residual_action::residual_periodic_action_t<actions::monk_spell_t>
//   {
//     using base_t = residual_action::residual_periodic_action_t<actions::monk_spell_t>;

//     self_damage_t( monk_t *player );
//     proc_types proc_type() const override;
//     void impact( action_state_t *state ) override;                                  // add to pool
//     void assess_damage( result_amount_type type, action_state_t *state ) override;  // tick from pool
//     void last_tick( dot_t *d ) override;                                            // callback on last tick
//   };

//   struct level_t
//   {
//     const stagger_level_e level;
//     const double min_percent;
//     std::string name;
//     std::string name_pretty;
//     const spell_data_t *spell_data;
//     propagate_const<stagger_t::debuff_t *> debuff;
//     sample_data_helper_t *absorbed;
//     sample_data_helper_t *taken;
//     sample_data_helper_t *mitigated;

//     level_t( stagger_level_e level, monk_t *player );
//     static double min_threshold( stagger_level_e level );
//     static std::string level_name_pretty( stagger_level_e level );
//     static std::string level_name( stagger_level_e level );
//     static const spell_data_t *level_spell_data( stagger_level_e level, monk_t *player );
//   };

//   struct sample_data_t
//   {
//     sc_timeline_t pool_size;          // raw stagger in pool
//     sc_timeline_t pool_size_percent;  // pool as a fraction of current maximum hp
//     sc_timeline_t effectiveness;      // stagger effectiveness
//     double buffed_base_value;
//     double buffed_percent_player_level;
//     double buffed_percent_target_level;
//     // assert(absorbed == taken + mitigated)
//     // assert(sum(mitigated_by_ability) == mitigated)
//     sample_data_helper_t *absorbed;
//     sample_data_helper_t *taken;
//     sample_data_helper_t *mitigated;
//     std::unordered_map<std::string, sample_data_helper_t *> mitigated_by_ability;

//     sample_data_t( monk_t *player );
//   };

//   monk_t *player;
//   std::vector<level_t *> stagger_levels;
//   level_t *current;
//   training_of_niuzao_t *training_of_niuzao;

//   stagger_level_e find_current_level();
//   void set_pool( double amount );
//   void damage_changed( bool last_tick = false );

//   self_damage_t *self_damage;
//   sample_data_t *sample_data;
//   stagger_t( monk_t *player );

//   double base_value();
//   double percent( unsigned target_level );
//   stagger_level_e current_level();
//   double level_index();
//   // even though this is an index, it is often used in floating point arithmetic,
//   // which makes returning a double preferable to help avoid int arithmetic nonsense

//   void add_sample( std::string name, double amount );

//   double pool_size();
//   double pool_size_percent();
//   double tick_size();
//   double tick_size_percent();
//   bool is_ticking();
//   timespan_t remains();

//   // TODO: implement a more automated procedure for guard-style absorb handling
//   double trigger( school_e school, result_amount_type damage_type, action_state_t *state );
//   double purify_flat( double amount, bool report_amount = true );
//   double purify_percent( double amount );
//   double purify_all();
//   void delay_tick( timespan_t delay );
// };
// //

/*
 * Each stagger effect gets a `stagger_data_t` object registered in actor stagger_data vector.
 * This contains:
 *  - name of stagger effect, as used in reporting
 *  - level spell data in `std::vector<stagger_level_data_t> levels`
 *  - self damage spell data in `self_damage`
 *  - an activator function `std::function<bool()>` which determines if the effect is active
 *  - a trigger filter function `std::function<bool(school_e, result_amount_type, action_state_t*)>` which determines if
 * the hit should be staggered
 */

namespace stagger_impl
{
template <class derived_actor_t>
struct stagger_t;
}

struct stagger_level_data_t
{
  const spell_data_t *spell_data;
  const double min_threshold;

  friend bool operator<( const stagger_level_data_t &lhs, const stagger_level_data_t &rhs )
  {
    return lhs.min_threshold < rhs.min_threshold;
  }
};

template <class derived_actor_t>
struct stagger_data_t
{
  stagger_impl::stagger_t<derived_actor_t> *stagger;
  std::string_view name;
  std::function<bool()> active;
  std::function<bool( school_e, result_amount_type, action_state_t * )> trigger;
  std::vector<stagger_level_data_t *> *levels;
  const spell_data_t *self_damage;
  std::vector<std::string_view> mitigation_tokens;

  // TODO: check if this type is derived from stagger_impl::stagger_t
  template <class stagger_type_t = stagger_impl::stagger_t<derived_actor_t>>
  void init_stagger( derived_actor_t *player )
  {
    if ( player->stagger.find( name ) == player->stagger.end() )
      player->stagger[ name ] = this;
    stagger = new stagger_type_t( player, *this );
  }
};

namespace stagger_impl
{
struct debuff_t : buff_t
{
  debuff_t( player_t *p, std::string_view name, const spell_data_t *s ) : buff_t( p, name, s )
  {
  }
};

struct self_damage_t : residual_action::residual_periodic_action_t<spell_t>
{
  using base_t = residual_action::residual_periodic_action_t<spell_t>;
  self_damage_t( player_t *player, std::string_view name, const spell_data_t *spell ) : base_t( name, player, spell )
  {
  }
};

// stagger_t::self_damage_t::self_damage_t( monk_t *player )
//   : base_t( "stagger_self_damage", player, player->passives.stagger_self_damage )

template <class derived_actor_t>
struct level_t
{
  derived_actor_t *player;
  const spell_data_t *spell_data;
  const double min_threshold;
  propagate_const<debuff_t *> debuff;
  sample_data_helper_t *absorbed;
  sample_data_helper_t *taken;
  sample_data_helper_t *mitigated;

  std::string_view name() const
  {
    return spell_data->name_cstr();
  }
  std::string_view name_tokenized() const
  {
    return util::tokenize_fn( name() );
  }
  level_t( derived_actor_t *player, const stagger_level_data_t &data )
    : player( player ), spell_data( data.spell_data ), min_threshold( data.min_threshold )
  {
    // TODO: Get stagger name to fill these strings
    absorbed  = player->get_sample_data( fmt::format( "{} added to pool while at {}.", "Stagger", name() ) );
    taken     = player->get_sample_data( fmt::format( "{} damage taken from {}.", "Stagger", name() ) );
    mitigated = player->get_sample_data( fmt::format( "{} damage mitigated while at {}.", "Stagger", name() ) );
    debuff    = make_buff<debuff_t>( player, name(), spell_data );
  }
};

template <class derived_actor_t>
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
  std::unordered_map<std::string_view, sample_data_helper_t *> mitigated_by_ability;

  sample_data_t( derived_actor_t *player, const stagger_data_t<derived_actor_t> &data )
  {
    absorbed  = player->get_sample_data( fmt::format( "Total damage absorbed by {}.", data.name ) );
    taken     = player->get_sample_data( fmt::format( "Total damage taken from {}.", data.name ) );
    mitigated = player->get_sample_data( fmt::format( "Total damage mitigated by {}.", data.name ) );

    for ( const std::string_view &token : data.mitigation_tokens )
      mitigated_by_ability[ token ] =
          player->get_sample_data( fmt::format( "Total {} purified by {}.", data.name, token ) );
  }
};

template <class derived_actor_t>
struct stagger_t
{
  derived_actor_t *player;

  std::vector<level_t<derived_actor_t> *> levels;
  level_t<derived_actor_t> *current;
  self_damage_t *self_damage;
  sample_data_t<derived_actor_t> *sample_data;

  // TODO: check if these types are derived from stagger_impl::self_damage_t or stagger_impl::level_t
  template <class self_damage_type_t = self_damage_t, class stagger_level_type_t = level_t<derived_actor_t>>
  stagger_t( derived_actor_t *player, const stagger_data_t<derived_actor_t> &data ) : player( player )
  {
    std::vector<stagger_level_data_t *> data_levels = *data.levels;
    std::sort( data_levels.begin(), data_levels.end() );
    for ( const stagger_level_data_t *level : data_levels )
      levels.push_back( new stagger_level_type_t( player, *level ) );
    current = levels.front();

    self_damage = new self_damage_type_t( player, "TODO_name", data.self_damage );
    sample_data = new sample_data_t( player, data );
  }

  // trigger
  virtual double trigger( school_e school, result_amount_type damage_type, action_state_t *state );
  void set_pool( double amount );
  void damage_changed( bool last_tick = false );
  size_t find_current_level();

  // purify
  double purify_flat( double amount, std::string_view trigger_ability, bool report_amount = true );
  double purify_percent( double percent, std::string_view trigger_ability );
  double purify_all( std::string_view trigger_ability );

  // other
  void delay_tick( timespan_t delay );

  // information
  double base_value()
  {
    return 0.0;
  }
  double pool_size();
  double pool_size_percent();
  double tick_size();
  double tick_size_percent();
  bool is_ticking();
  timespan_t remains();

  virtual double percent( unsigned target_level );
  size_t current_level();
  double level_index();  // even though this is an index, returning a double avoids integer demotion concerns
};

template <class derived_actor_t>
struct stagger_report_t : player_report_extension_t
{
  derived_actor_t *player;

  void html_customsection( report::sc_html_stream &os ) override
  {
    // os << "\t\t\t\t<div class=\"player-section custom_section\">\n"
    //    << "\t\t\t\t\t<h3 class=\"toggle open\">Stagger Analysis</h3>\n"
    //    << "\t\t\t\t\t<div class=\"toggle-content\">\n";

    // os << "\t\t\t\t\t<p>Note that these charts are extremely sensitive to bucket"
    //    << "sizes. It is not uncommon to exceed stagger cap, etc. If you wish to "
    //    << "see exact values, use debug output.</p>\n";

    // auto chart_print = [ &os, this ]( std::string title, std::string id, sc_timeline_t &timeline ) {
    //   highchart::time_series_t chart_( highchart::build_id( *player, id ), *player->sim );
    //   chart::generate_actor_timeline( chart_, *player, title, color::resource_color( RESOURCE_HEALTH ), timeline );
    //   chart_.set( "tooltip.headerFormat", "<b>{point.key}</b> s<br/>" );
    //   chart_.set( "chart.width", "575" );
    //   os << chart_.to_target_div();
    //   player->sim->add_chart_data( chart_ );
    // };

    // chart_print( "Stagger Pool", "stagger_pool", player->stagger->sample_data->pool_size );
    // chart_print( "Stagger Pool / Maximum Health", "stagger_pool_percent",
    //              player->stagger->sample_data->pool_size_percent );
    // chart_print( "Stagger Percent", "stagger_percent", player->stagger->sample_data->effectiveness );

    // fmt::print( os, "\t\t\t\t\t\t<p>Stagger base Unbuffed: {} Raid Buffed: {}</p>\n", player->stagger->base_value(),
    //             player->stagger->sample_data->buffed_base_value );

    // fmt::print( os, "\t\t\t\t\t\t<p>Stagger pct (player level) Unbuffed: {:.2f}% Raid Buffed: {:.2f}%</p>\n",
    //             100.0 * player->stagger->percent( player->level() ),
    //             100.0 * player->stagger->sample_data->buffed_percent_player_level );

    // fmt::print( os, "\t\t\t\t\t\t<p>Stagger pct (target level) Unbuffed: {:.2f}% Raid Buffed: {:.2f}%</p>\n",
    //             100.0 * player->stagger->percent( player->target->level() ),
    //             100.0 * player->stagger->sample_data->buffed_percent_target_level );

    // double absorbed_mean = player->stagger->sample_data->absorbed->mean();

    // fmt::print( os, "\t\t\t\t\t\t<p>Total Stagger damage added: {} / {:.2f}%</p>\n", absorbed_mean, 100.0 );

    // // Purification Stats
    // os << "\t\t\t\t\t\t<table class=\"sc\">\n"
    //    << "\t\t\t\t\t\t\t<tbody>\n"
    //    << "\t\t\t\t\t\t\t\t<tr>\n"
    //    << "\t\t\t\t\t\t\t\t\t<th class=\"left\">Purification Stats</th>\n"
    //    << "\t\t\t\t\t\t\t\t\t<th>Purified</th>\n"
    //    << "\t\t\t\t\t\t\t\t\t<th>Purified %</th>\n"
    //    // << "\t\t\t\t\t\t\t\t\t<th>Purification Count</th>\n"
    //    << "\t\t\t\t\t\t\t\t</tr>\n";

    // auto purify_print = [ absorbed_mean, &os, this ]( std::string name, std::string key ) {
    //   double mean = player->stagger->sample_data->mitigated_by_ability[ key ]->mean();
    //   // int count      = player->stagger->sample_data->mitigated_by_ability[ key ]->count();
    //   double percent = mean / absorbed_mean * 100.0;
    //   os << "\t\t\t\t\t\t\t\t<tr>\n"
    //      << "\t\t\t\t\t\t\t\t\t<td class=\"left small\" rowspan=\"1\">" << name << "</td>\n"
    //      << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << mean << "</td>\n"
    //      << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << percent
    //      << "%</td>\n"
    //      // << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << count << "</td>\n"
    //      << "\t\t\t\t\t\t\t\t</tr>\n";
    // };

    // purify_print( "Quick Sip", "quick_sip" );
    // purify_print( "Staggering Strikes", "staggering_strikes" );
    // purify_print( "Touch of Death", "touch_of_death" );
    // purify_print( "Purifying Brew", "purifying_brew" );
    // purify_print( "Tranquil Spirit: Gift of the Ox", "tranquil_spirit_gift_of_the_ox" );
    // purify_print( "Tranquil Spirit: Expel Harm", "tranquil_spirit_expel_harm" );

    // os << "\t\t\t\t\t\t\t</tbody>\n"
    //    << "\t\t\t\t\t\t</table>\n";

    // // Stagger Level Stats
    // os << "\t\t\t\t\t\t<table class=\"sc\">\n"
    //    << "\t\t\t\t\t\t\t<tbody>\n"
    //    << "\t\t\t\t\t\t\t\t<tr>\n"
    //    << "\t\t\t\t\t\t\t\t\t<th class=\"left\">Stagger Stats</th>\n"
    //    << "\t\t\t\t\t\t\t\t\t<th>Absorbed</th>\n"
    //    << "\t\t\t\t\t\t\t\t\t<th>Taken</th>\n"
    //    << "\t\t\t\t\t\t\t\t\t<th>Mitigated</th>\n"
    //    << "\t\t\t\t\t\t\t\t\t<th>Mitigated %</th>\n"
    //    // << "\t\t\t\t\t\t\t\t\t<th>Ticks</th>\n"
    //    << "\t\t\t\t\t\t\t\t</tr>\n";

    // auto stagger_print = [ absorbed_mean, &os ]( const level_t<derived_actor_t> *level ) {
    //   os << "\t\t\t\t\t\t\t\t<tr>\n"
    //      << "\t\t\t\t\t\t\t\t\t<td class=\"left small\" rowspan=\"1\">" << level->name() << "</td>\n"
    //      << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << level->absorbed->mean() << "</td>\n"
    //      << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << level->taken->mean() << "</td>\n"
    //      << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << level->mitigated->mean() << "</td>\n"
    //      << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">"
    //      << ( absorbed_mean != 0.0 ? level->mitigated->mean() / absorbed_mean * 100.0 : 0 )
    //      // << ( level->absorbed->mean() != 0.0 ? level->mitigated->mean() / level->absorbed->mean() * 100.0 : 0 )
    //      << "%</td>\n"
    //         // << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << level->absorbed->count() <<
    //         "</td>\n"
    //      << "\t\t\t\t\t\t\t\t</tr>\n";
    // };

    // for ( const level_t<derived_actor_t> *level : player->stagger->levels )
    // {
    //   stagger_print( level );
    // }

    // double taken_mean     = player->stagger->sample_data->taken->mean();
    // double mitigated_mean = player->stagger->sample_data->mitigated->mean();
    // // int absorbed_count    = player.stagger->sample_data->absorbed->count();

    // os << "\t\t\t\t\t\t\t\t<tr>\n"
    //    << "\t\t\t\t\t\t\t\t\t<td class=\"left small\" rowspan=\"1\">"
    //    << "Total"
    //    << "</td>\n"
    //    << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << absorbed_mean << "</td>\n"
    //    << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << taken_mean << "</td>\n"
    //    << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << mitigated_mean << "</td>\n"
    //    << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">"
    //    << ( absorbed_mean != 0.0 ? mitigated_mean / absorbed_mean * 100.0 : 0 )
    //    << "%</td>\n"
    //    // << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << absorbed_count << "</td>\n"
    //    << "\t\t\t\t\t\t\t\t</tr>\n";

    // os << "\t\t\t\t\t\t\t</tbody>\n"
    //    << "\t\t\t\t\t\t</table>\n";

    // os << "\t\t\t\t\t\t</div>\n"
    //    << "\t\t\t\t\t</div>\n";
  }

  stagger_report_t( derived_actor_t *player ) : player( player )
  {
  }
};
}  // namespace stagger_impl

template <class base_actor_t, class derived_actor_t /* , class derived_actor_td_t */>
struct stagger_t : base_actor_t
{
  std::vector<stagger_data_t<derived_actor_t> *> *stagger_data;

  std::unordered_map<std::string_view, stagger_data_t<derived_actor_t> *> stagger;

  virtual void create_buffs() override
  {
    base_actor_t::create_buffs();

    for ( auto &[ key, effect ] : stagger )
      effect->init_stagger( static_cast<derived_actor_t *>( this ) );
  }

  virtual void combat_begin() override
  {
    base_actor_t::combat_begin();
  }

  virtual void pre_analyze_hook() override
  {
    base_actor_t::pre_analyze_hook();
  }

  virtual void assess_damage_imminent_pre_absorb( school_e school, result_amount_type damage_type,
                                                  action_state_t *state ) override
  {
    base_actor_t::assess_damage_imminent_pre_absorb( school, damage_type, state );
  }

  virtual void merge( player_t &other ) override
  {
    base_actor_t::merge( other );
  }

  virtual std::unique_ptr<expr_t> create_expression( std::string_view name_str ) override
  {
    return base_actor_t::create_expression( name_str );
  }

  template <typename... Args>
  stagger_t( Args &&...args ) : base_actor_t( std::forward<Args>( args )... )
  {
    static_assert(
        std::is_base_of_v<player_t, base_actor_t>,
        "stagger_t<base_actor_t, ..> base_actor is not derived from player_t (should be module actor base type)" );
    static_assert(
        std::is_base_of_v<player_t, derived_actor_t>,
        "stagger_t<.., derived_actor_t> derived_actor is not derived from player_t (should be actor derived type)" );

    this->mixin_reports.push_back(
        std::make_unique<stagger_impl::stagger_report_t<derived_actor_t>>( static_cast<derived_actor_t *>( this ) ) );
  }
};
