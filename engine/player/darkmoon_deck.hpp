// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

#include "action/action_state.hpp"
#include "action/dbc_proc_callback.hpp"
#include "dbc/dbc.hpp"
#include "player/unique_gear_helper.hpp"
#include "sim/event.hpp"
#include "util/rng.hpp"
#include "util/timespan.hpp"

struct player_t;
struct special_effect_t;

struct darkmoon_deck_t
{
  const special_effect_t& effect;
  player_t* player;
  std::vector<unsigned> card_ids;
  event_t* shuffle_event;
  timespan_t shuffle_period;
  size_t top_index;

  darkmoon_deck_t( const special_effect_t& e, std::vector<unsigned> c );

  virtual ~darkmoon_deck_t() = default;

  virtual void initialize() = 0;
  virtual void shuffle() = 0;
  virtual size_t get_index( bool init = false );
};

template <typename BUFF_TYPE>
struct darkmoon_buff_deck_t : public darkmoon_deck_t
{
  std::vector<BUFF_TYPE*> cards;
  BUFF_TYPE* top_card;

  darkmoon_buff_deck_t( const special_effect_t& effect, std::vector<unsigned> c )
    : darkmoon_deck_t( effect, std::move( c ) ), top_card( nullptr )
  {}

  void initialize() override
  {
    range::for_each( card_ids, [ this ]( unsigned spell_id ) {
      const spell_data_t* s = player->find_spell( spell_id );
      assert( s->found() );

      auto n = util::tokenize_fn( s->name_cstr() );

      cards.push_back( make_buff<BUFF_TYPE>( player, n, s, effect.item ) );
    } );

    // Pick a card during init so top_card is always initialized.
    top_card = cards[ get_index( true ) ];
  }

  void shuffle() override
  {
    get_index();

    if ( top_card )
      top_card->expire();

    top_card = cards[ top_index ];
    player->sim->print_debug( "{} top card is now {} ({}, id={}, index={})", top_card->player->name(), top_card->name(),
                              top_card->data().name_cstr(), top_card->data().id(), top_index );

    top_card->trigger();
  }
};

template <typename ACTION_TYPE>
struct darkmoon_action_deck_t : public darkmoon_deck_t
{
  std::vector<action_t*> cards;
  action_t* top_card;
  bool trigger_on_shuffle;

  darkmoon_action_deck_t( const special_effect_t& effect, std::vector<unsigned> c )
    : darkmoon_deck_t( effect, std::move( c ) ), top_card( nullptr ), trigger_on_shuffle( false )
  {}

  darkmoon_action_deck_t<ACTION_TYPE>& set_trigger_on_shuffle( bool value )
  {
    trigger_on_shuffle = value;
    return *this;
  }

  void initialize() override
  {
    range::for_each( card_ids, [ this ]( unsigned spell_id ) {
      const spell_data_t* s = player->find_spell( spell_id );
      assert( s->found() );

      auto n = util::tokenize_fn( s->name_cstr() );

      cards.push_back( unique_gear::create_proc_action<ACTION_TYPE>( n, effect, spell_id ) );
    } );

    // Pick a card during init so top_card is always initialized.
    top_card = cards[ get_index( true ) ];
  }

  // For actions, just shuffle the deck, and optionally trigger the action if trigger_on_shuffle is
  // set
  void shuffle() override
  {
    top_card = cards[ get_index() ];
    player->sim->print_debug( "{} top card is now {} ({}, id={}, index={})", top_card->player->name(), top_card->name(),
                              top_card->data().name_cstr(), top_card->data().id(), top_index );

    if ( trigger_on_shuffle )
    {
      top_card->set_target( player->target );
      top_card->schedule_execute();
    }
  }
};

// Generic deck to hold spell_data_t objects
struct darkmoon_spell_deck_t : public darkmoon_deck_t
{
  std::vector<const spell_data_t*> cards;
  const spell_data_t* top_card;

  darkmoon_spell_deck_t( const special_effect_t& effect, std::vector<unsigned> c )
    : darkmoon_deck_t( effect, std::move( c ) ), top_card( spell_data_t::nil() )
  {}

  void initialize() override
  {
    for ( auto c : card_ids )
    {
      auto s = player->find_spell( c );
      assert( s->ok() );
      cards.push_back( s );
    }

    // Pick a card during init so top_card is always initialized.
    top_card = cards[ get_index( true ) ];
  }

  void shuffle() override
  {
    top_card = cards[ get_index() ];
    player->sim->print_debug( "{} top card is now {} (id={}, index={})", player->name(), top_card->name_cstr(),
                              top_card->id(), top_index );
  }
};

struct shuffle_event_t : public event_t
{
  darkmoon_deck_t* deck;

  static timespan_t delta_time( sim_t& sim, bool initial, darkmoon_deck_t* deck );

  shuffle_event_t( darkmoon_deck_t* d, bool initial = false )
    : event_t( *d->player, delta_time( *d->player->sim, initial, d ) ), deck( d )
  {
    /* Shuffle when we schedule an event instead of when it executes.
    This will assure the deck starts shuffled */
    deck->shuffle();
  }

  const char* name() const override
  {
    return "shuffle_event";
  }

  void execute() override
  {
    deck->shuffle_event = make_event<shuffle_event_t>( sim(), deck );
  }
};

// Generic darkmoon card callback template class
template <typename T>
struct darkmoon_deck_cb_t : public dbc_proc_callback_t
{
  std::unique_ptr<darkmoon_action_deck_t<T>> deck;

  darkmoon_deck_cb_t( const special_effect_t& effect, std::vector<unsigned> cards )
    : dbc_proc_callback_t( effect.player, effect ),
      deck( std::make_unique<darkmoon_action_deck_t<T>>( effect, std::move( cards ) ) )
  {
    deck->initialize();

    effect.player->register_combat_begin( [ this ]( player_t* ) {
      deck->shuffle_event = make_event<shuffle_event_t>( *listener->sim, deck.get(), true );
    } );
  }

  void execute( action_t*, action_state_t* state ) override
  {
    deck->top_card->set_target( state->target );
    deck->top_card->schedule_execute();
  }
};

// Generic darkmoon card on-use template class
template <typename Base = unique_gear::proc_spell_t, typename T = darkmoon_spell_deck_t>
struct darkmoon_deck_proc_t : public Base
{
  static_assert( std::is_base_of_v<darkmoon_deck_t, T> );

  std::unique_ptr<T> deck;

  darkmoon_deck_proc_t( const special_effect_t& e, std::string_view n, unsigned shuffle_id,
                        std::vector<unsigned> cards )
    : Base( n, e.player, e.trigger(), e.item )
  {
    auto shuffle = unique_gear::find_special_effect( Base::player, shuffle_id );
    if ( !shuffle )
      return;

    deck = std::make_unique<T>( *shuffle, std::move( cards ) );
    deck->initialize();

    Base::player->register_combat_begin( [ this ]( player_t* ) {
      deck->shuffle_event = make_event<shuffle_event_t>( *Base::player->sim, deck.get(), true );
    } );
  }
};
