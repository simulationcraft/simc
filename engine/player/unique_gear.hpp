// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef UNIQUE_GEAR_HPP
#define UNIQUE_GEAR_HPP

#include "simulationcraft.hpp"

namespace unique_gear
{
struct darkmoon_deck_t
{
  const special_effect_t& effect;
  player_t* player;
  timespan_t shuffle_period;

  darkmoon_deck_t( const special_effect_t& e ) :
    effect( e ), player( e.player ),
    shuffle_period( effect.driver() -> effectN( 1 ).period() )
  { }

  virtual ~darkmoon_deck_t()
  { }

  virtual void initialize() {}
  virtual void shuffle() = 0;
};

template <typename BUFF_TYPE>
struct darkmoon_buff_deck_t : public darkmoon_deck_t
{
  std::vector<unsigned>   card_ids;
  std::vector<BUFF_TYPE*> cards;
  BUFF_TYPE*              top_card;

  darkmoon_buff_deck_t( const special_effect_t& effect, const std::vector<unsigned>& c ) :
    darkmoon_deck_t( effect ), card_ids( c ), top_card( nullptr )
  { }

  void initialize() override
  {
    range::for_each( card_ids, [this]( unsigned spell_id ) {
      const spell_data_t* s = player->find_spell( spell_id );
      assert( s->found() );

      std::string n = s->name_cstr();
      util::tokenize( n );

      cards.push_back( make_buff<BUFF_TYPE>( player, n, s, effect.item ) );
    } );

    // Pick a card during init so top_card is always initialized.
    size_t index = static_cast<size_t>( player->rng().range( 0u, cards.size() ) );
    top_card = cards[ index ];
  }

  void shuffle() override
  {
    size_t index = static_cast<size_t>( player->rng().range( 0u, cards.size() ) );

    if ( top_card )
    {
      top_card->expire();
    }

    top_card = cards[ index ];

    if ( top_card->sim->debug )
    {
      top_card->sim->out_debug.print( "{} top card is now {} ({}, id={}, index={})",
        top_card->player->name(), top_card->name(), top_card->data().name_cstr(),
        top_card->data().id(), index );
    }

    top_card->trigger();
  }
};

template <typename ACTION_TYPE>
struct darkmoon_action_deck_t : public darkmoon_deck_t
{
  std::vector<unsigned>  card_ids;
  std::vector<action_t*> cards;
  action_t*              top_card;
  bool                   trigger_on_shuffle;

  darkmoon_action_deck_t( const special_effect_t& effect, const std::vector<unsigned>& c ) :
    darkmoon_deck_t( effect ), card_ids( c ), top_card( nullptr ), trigger_on_shuffle( false )
  { }

  darkmoon_action_deck_t<ACTION_TYPE>& set_trigger_on_shuffle( bool value )
  { trigger_on_shuffle = value; return *this; }

  void initialize() override
  {
    range::for_each( card_ids, [this]( unsigned spell_id ) {
      const spell_data_t* s = player->find_spell( spell_id );
      assert( s->found() );

      std::string n = s->name_cstr();
      util::tokenize( n );

      cards.push_back( unique_gear::create_proc_action<ACTION_TYPE>( n, effect, spell_id ) );
    } );

    // Pick a card during init so top_card is always initialized.
    size_t index = static_cast<size_t>( player->rng().range( 0u, cards.size() ) );
    top_card = cards[ index ];
  }

  // For actions, just shuffle the deck, and optionally trigger the action if trigger_on_shuffle is
  // set
  void shuffle() override
  {
    size_t index = static_cast<size_t>( player->rng().range( 0u, cards.size() ) );
    top_card = cards[ index ];

    if ( top_card->sim->debug )
    {
      top_card->sim->out_debug.print( "{} top card is now {} ({}, id={}, index={})",
        top_card->player->name(), top_card->name(), top_card->data().name_cstr(),
        top_card->data().id(), index );
    }

    if ( trigger_on_shuffle )
    {
      top_card->set_target( player->target );
      top_card->schedule_execute();
    }
  }
};

struct shuffle_event_t : public event_t
{
  darkmoon_deck_t* deck;

  static timespan_t delta_time( sim_t& sim, bool initial, darkmoon_deck_t* deck )
  {
    if ( initial )
    {
      return deck->shuffle_period * sim.rng().real();
    }
    else
    {
      return deck->shuffle_period;
    }
  }

  shuffle_event_t( darkmoon_deck_t* d, bool initial = false )
    : event_t( *d->player, delta_time( *d->player->sim, initial, d ) ), deck( d )
  {
    /* Shuffle when we schedule an event instead of when it executes.
    This will assure the deck starts shuffled */
    deck->shuffle();
  }

  const char* name() const override
  { return "shuffle_event"; }

  void execute() override
  { make_event<shuffle_event_t>( sim(), deck ); }
};

// Generic BFA darkmoon card template class to reduce copypasta
template <typename T>
struct bfa_darkmoon_deck_cb_t : public dbc_proc_callback_t
{
  darkmoon_action_deck_t<T>* deck;

  bfa_darkmoon_deck_cb_t( const special_effect_t& effect, const std::vector<unsigned>& cards ) :
    dbc_proc_callback_t( effect.player, effect ),
    deck( new darkmoon_action_deck_t<T>( effect, cards ) )
  {
    deck->initialize();

    effect.player->register_combat_begin( [ this ]( player_t* ) {
      make_event<shuffle_event_t>( *listener->sim, deck, true );
    } );
  }

  void execute( action_t*, action_state_t* state ) override
  {
    deck->top_card->set_target( state->target );
    deck->top_card->schedule_execute();
  }

  ~bfa_darkmoon_deck_cb_t()
  { delete deck; }
};

} // Namespace unique_gear ends

#endif /* UNIQUE_GEAR_HPP */
