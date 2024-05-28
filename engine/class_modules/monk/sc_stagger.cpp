#include "sc_stagger.hpp"

namespace stagger_impl
{
sample_data_t::sample_data_t( player_t* player, std::string_view name, std::vector<std::string_view> mitigation_tokens )
{
  absorbed  = player->get_sample_data( fmt::format( "Total damage absorbed by {}.", name ) );
  taken     = player->get_sample_data( fmt::format( "Total damage taken from {}.", name ) );
  mitigated = player->get_sample_data( fmt::format( "Total damage mitigated by {}.", name ) );

  for ( const std::string_view& token : mitigation_tokens )
    mitigated_by_ability[ token ] = player->get_sample_data( fmt::format( "Total {} purified by {}.", name, token ) );
}
}  // namespace stagger_impl

// // stagger_t implementation
// // stagger_t::debuff_t
// stagger_t::debuff_t::debuff_t( player_t &player, std::string_view name, const spell_data_t *spell )
//   : buff_t( player, name, spell )
// {
//   // duration is controlled by stagger_t::self_damage_t
//   // logic is more convenient if we prevent this from ever naturally expiring
//   base_buff_duration = 0_s;
//   set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
//   add_invalidate( CACHE_HASTE );
//   set_default_value_from_effect_type( A_HASTE_ALL );
//   set_pct_buff_type( STAT_PCT_BUFF_HASTE );
//   apply_affecting_aura( player.talent.brewmaster.high_tolerance );
//   set_stack_change_callback( [ &stagger = player.stagger ]( buff_t *, int old_, int new_ ) {
//     if ( old_ )
//       stagger->training_of_niuzao->expire();
//     if ( new_ )
//       stagger->training_of_niuzao->trigger_();
//   } );
// }

// // stagger_t::training_of_niuzao_t::training_of_niuzao_t( monk_t &player )
// //   : actions::monk_buff_t( player, "training_of_niuzao", player.talent.brewmaster.training_of_niuzao )
// // {
// //   add_invalidate( CACHE_MASTERY );
// //   set_default_value( 0.0 );
// //   set_pct_buff_type( STAT_PCT_BUFF_MASTERY );
// // }

// bool stagger_t::training_of_niuzao_t::trigger_()
// {
//   double value = p().stagger->level_index() * data().effectN( 1 ).base_value();
//   return trigger( 1, value );
// }

// // stagger_t::self_damage_t
// stagger_t::self_damage_t::self_damage_t( monk_t *player )
//   : base_t( "stagger_self_damage", player, player->passives.stagger_self_damage )
// {
//   dot_duration = player->baseline.brewmaster.light_stagger->duration();
//   dot_duration += timespan_t::from_seconds( player->talent.brewmaster.bob_and_weave->effectN( 1 ).base_value() / 10
//   ); base_tick_time = player->passives.stagger_self_damage->effectN( 1 ).period(); hasted_ticks = tick_may_crit =
//   false; target                       = player; stats->type                  = stats_e::STATS_NEUTRAL;
// }

// proc_types stagger_t::self_damage_t::proc_type() const
// {
//   return PROC1_ANY_DAMAGE_TAKEN;
// }

// void stagger_t::self_damage_t::impact( action_state_t *state )
// {
//   base_t::impact( state );

//   p()->sim->print_debug(
//       "{} current pool statistics pool_size={} pool_size_percent={} tick_size={} tick_size_percent={} cap={}",
//       p()->name(), p()->stagger->pool_size(), p()->stagger->pool_size_percent(), p()->stagger->tick_size(),
//       p()->stagger->tick_size_percent(), p()->resources.max[ RESOURCE_HEALTH ] );
//   p()->stagger->damage_changed();
//   p()->stagger->sample_data->absorbed->add( state->result_amount );
//   p()->stagger->current->absorbed->add( state->result_amount );
//   p()->stagger->sample_data->pool_size.add( sim->current_time(), p()->stagger->pool_size() );
//   p()->stagger->sample_data->pool_size_percent.add( sim->current_time(), p()->stagger->pool_size_percent() );
//   p()->buff.shuffle->up();
// }

// void stagger_t::self_damage_t::last_tick( dot_t *dot )
// {
//   base_t::last_tick( dot );

//   p()->stagger->damage_changed( true );
// }

// void stagger_t::self_damage_t::assess_damage( result_amount_type type, action_state_t *state )
// {
//   base_t::assess_damage( type, state );

//   p()->sim->print_debug(
//       "{} current pool statistics pool_size={} pool_size_percent={} tick_size={} tick_size_percent={} cap={}",
//       p()->name(), p()->stagger->pool_size(), p()->stagger->pool_size_percent(), p()->stagger->tick_size(),
//       p()->stagger->tick_size_percent(), p()->resources.max[ RESOURCE_HEALTH ] );
//   p()->stagger->damage_changed();
//   p()->stagger->sample_data->taken->add( state->result_amount );
//   p()->stagger->current->taken->add( state->result_amount );
//   p()->stagger->sample_data->pool_size.add( sim->current_time(), p()->stagger->pool_size() );
//   p()->stagger->sample_data->pool_size_percent.add( sim->current_time(), p()->stagger->pool_size_percent() );
// }

// // stagger_t
// stagger_t::stagger_t( monk_t *player )
//   : player( player ), self_damage( new self_damage_t( player ) ), sample_data( new sample_data_t( player ) )
// {
//   auto init_level = [ this, player ]( stagger_level_e level ) {
//     stagger_levels.insert( stagger_levels.begin(), new stagger_t::stagger_level_t( level, player ) );
//   };
//   training_of_niuzao = make_buff<stagger_t::training_of_niuzao_t>( *player );
//   init_level( NONE_STAGGER );
//   init_level( LIGHT_STAGGER );
//   init_level( MODERATE_STAGGER );
//   init_level( HEAVY_STAGGER );
//   current = stagger_levels[ NONE_STAGGER ];
// }

// auto stagger_t::find_current_level() -> stagger_level_e
// {
//   double current_percent = pool_size_percent();
//   for ( const stagger_level_t *level : stagger_levels )
//   {
//     if ( current_percent > level->min_percent )
//       return level->level;
//   }
//   return NONE_STAGGER;
// }

// void stagger_t::set_pool( double amount )
// {
//   if ( !is_ticking() )
//     return;

//   dot_t *dot        = self_damage->get_dot();
//   auto state        = debug_cast<residual_action::residual_periodic_state_t *>( dot->state );
//   double ticks_left = dot->ticks_left();

//   state->tick_amount = amount / ticks_left;
//   player->sim->print_debug( "{} set pool amount={} ticks_left={}, tick_size={}", player->name(), amount, ticks_left,
//                             amount / ticks_left );

//   damage_changed();
// }

// void stagger_t::damage_changed( bool last_tick )
// {
//   // TODO: Guarantee a debuff is applied by providing debuffs for all stagger_level_t's
//   if ( last_tick )
//   {
//     current->debuff->expire();
//     return;
//   }

//   stagger_level_e level = find_current_level();
//   player->sim->print_debug( "{} level changing current={} ({}) new={} ({}) pool_size_percent={}", player->name(),
//                             current->name, static_cast<int>( current->level ), stagger_levels[ level ]->name,
//                             static_cast<int>( stagger_levels[ level ]->level ), pool_size_percent() );
//   if ( level == current->level )
//     return;

//   current->debuff->expire();
//   current = stagger_levels[ level ];
//   current->debuff->trigger();
// }

// double stagger_t::base_value()
// {
//   if ( player->specialization() != MONK_BREWMASTER )
//     return 0.0;

//   double stagger_base = player->agility() * player->spec.stagger->effectN( 1 ).percent();

//   if ( player->talent.brewmaster.high_tolerance->ok() )
//     stagger_base *= 1.0 + player->talent.brewmaster.high_tolerance->effectN( 5 ).percent();

//   if ( player->talent.brewmaster.fortifying_brew_determination->ok() && player->buff.fortifying_brew->up() )
//     stagger_base *= 1.0 + player->talents.monk.fortifying_brew_buff->effectN( 6 ).percent();

//   if ( player->buff.shuffle->check() )
//     stagger_base *= 1.0 + player->passives.shuffle->effectN( 1 ).percent();

//   return stagger_base;
// }

// double stagger_t::percent( unsigned target_level )
// {
//   double stagger_base = base_value();
//   double k            = player->dbc->armor_mitigation_constant( target_level );
//   k *= player->dbc->get_armor_constant_mod( difficulty_e::MYTHIC );

//   double stagger = stagger_base / ( stagger_base + k );

//   return std::min( stagger, 0.99 );
// }

// auto stagger_t::current_level() -> stagger_level_e
// {
//   return current->level;
// }

// double stagger_t::level_index()
// {
//   return NONE_STAGGER - current->level;
// }

// void stagger_t::add_sample( std::string name, double amount )
// {
//   assert( sample_data->mitigated_by_ability.count( name ) > 0 );
//   sample_data->mitigated_by_ability[ name ]->add( amount );
// }

// double stagger_t::pool_size()
// {
//   dot_t *dot = self_damage->get_dot();
//   if ( !dot || !dot->state )
//     return 0.0;

//   return self_damage->base_ta( dot->state ) * dot->ticks_left();
// }

// double stagger_t::pool_size_percent()
// {
//   return pool_size() / player->resources.max[ RESOURCE_HEALTH ];
// }

// double stagger_t::tick_size()
// {
//   dot_t *dot = self_damage->get_dot();
//   if ( !dot || !dot->state )
//     return 0.0;

//   return self_damage->base_ta( dot->state );
// }

// double stagger_t::tick_size_percent()
// {
//   return tick_size() / player->resources.max[ RESOURCE_HEALTH ];
// }

// bool stagger_t::is_ticking()
// {
//   dot_t *dot = self_damage->get_dot();
//   if ( !dot )
//     return false;

//   return dot->is_ticking();
// }

// timespan_t stagger_t::remains()
// {
//   dot_t *dot = self_damage->get_dot();
//   if ( !dot || !dot->is_ticking() )
//     return 0_s;

//   return dot->remains();
// }

// double stagger_t::trigger( school_e school, result_amount_type /* damage_type */, action_state_t *state )
// {
//   if ( state->result_amount <= 0.0 )
//     return 0.0;

//   if ( state->action->id == player->passives.stagger_self_damage->id() )
//     return 0.0;

//   double percent = player->stagger->percent( state->target->level() );
//   if ( school != SCHOOL_PHYSICAL )
//     percent *= player->spec.stagger->effectN( 5 ).percent();

//   double amount   = state->result_amount * percent;
//   double absorb   = player->buff.brewmaster_t31_4p_fake_absorb->check_value();
//   double absorbed = std::min( amount, absorb );
//   amount -= absorbed;

//   if ( absorbed > 0.0 )
//     player->sim->print_debug( "{} absorbed {} out of {} damage about to be added to stagger pool", player->name(),
//                               absorbed, state->result_amount * percent );
//   if ( absorbed < absorb )
//     player->buff.brewmaster_t31_4p_fake_absorb->trigger( 1, absorbed );
//   else if ( absorb > 0.0 && absorbed == absorb )
//     player->buff.brewmaster_t31_4p_fake_absorb->expire();

//   double cap = player->resources.max[ RESOURCE_HEALTH ] * player->spec.stagger->effectN( 4 ).percent();
//   amount -= std::max( amount + pool_size() - cap, 0.0 );

//   player->sim->print_debug( "{} added {} to stagger pool base_hit={} absorbed={} overcapped={}", player->name(),
//   amount,
//                             state->result_amount, absorbed, std::max( amount + pool_size() - cap, 0.0 ) );
//   player->sim->print_debug(
//       "{} current pool statistics pool_size={} pool_size_percent={} tick_size={} tick_size_percent={} cap={}",
//       player->name(), pool_size(), pool_size_percent(), tick_size(), tick_size_percent(), cap );

//   state->result_amount -= amount;
//   state->result_mitigated -= amount;
//   residual_action::trigger( self_damage, player, amount );

//   return amount;
// }

// double stagger_t::purify_flat( double amount, bool report_amount )
// {
//   if ( !is_ticking() )
//     return 0.0;

//   double pool    = pool_size();
//   double cleared = std::clamp( amount, 0.0, pool );
//   double remains = pool - cleared;

//   sample_data->mitigated->add( cleared );
//   current->mitigated->add( cleared );

//   set_pool( remains );
//   if ( report_amount )
//     player->sim->print_debug( "{} reduced pool from {} to {} ({})", player->name(), pool, remains, cleared );

//   return cleared;
// }

// double stagger_t::purify_percent( double amount )
// {
//   double pool    = pool_size();
//   double cleared = purify_flat( amount * pool, false );

//   if ( cleared )
//     player->sim->print_debug( "{} reduced pool by {}% from {} to {} ({})", player->name(), amount * 100.0, pool,
//                               ( 1.0 - amount ) * pool, cleared );

//   return cleared;
// }

// double stagger_t::purify_all()
// {
//   return purify_flat( pool_size(), true );
// }

// void stagger_t::delay_tick( timespan_t delay )
// {
//   dot_t *dot = self_damage->get_dot();
//   if ( !dot || !dot->is_ticking() || !dot->tick_event )
//     return;

//   player->sim->print_debug( "{} delayed tick scheduled for {} to {}", player->name(),
//                             player->sim->current_time() + dot->tick_event->remains(),
//                             player->sim->current_time() + dot->tick_event->remains() + delay );
//   dot->tick_event->reschedule( dot->tick_event->remains() + delay );
//   if ( dot->end_event )
//     dot->end_event->reschedule( dot->end_event->remains() + delay );
// }
