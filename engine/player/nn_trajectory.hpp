// Trajectory buffer for policy training data.
//
// One buffer per actor (and therefore per thread); iterations are recorded
// sequentially. Buffers from all threads are merged with append() at the end.
//
// Usage inside an iteration:
//   at iteration start:  buf.begin_episode();
//   at every decision:   buf.record_decision( time, score, features, mask, action, log_prob );
//   at iteration end:    buf.end_episode( final_score );
//
// `score` is the actor's cumulative score counter. The reward of step t is the score
// gained between decision t and decision t+1 (or the end of the iteration), so
// the rewards of one episode sum to final_score - score_at_first_decision.
//
// Storage is struct-of-arrays so steps cost no per-step allocation and the
// arrays can be written to disk as-is.

#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

struct nn_trajectory_t
{
  std::size_t n_inputs  = 0;
  std::size_t n_actions = 0;

  // Per step (n_steps entries, or n_steps * n_inputs / n_actions for the flat arrays).
  std::vector<float>    features;   // RAW expression values (before offset/scale/clamp), row-major
  std::vector<uint8_t>  mask;       // 1 = action was legal, row-major
  std::vector<uint32_t> action;     // index of the chosen action
  std::vector<float>    log_prob;   // log-probability of the chosen action under the acting policy
  std::vector<double>   time;       // sim time of the decision
  std::vector<double>   reward;     // score gained until the next decision / end of iteration
  std::vector<uint32_t> episode;    // episode index (contiguous runs)
  std::vector<uint8_t>  terminal;   // 1 on the last step of an episode

  nn_trajectory_t() = default;
  nn_trajectory_t( std::size_t n_inputs, std::size_t n_actions ) : n_inputs( n_inputs ), n_actions( n_actions ) {}

  std::size_t size() const { return action.size(); }

  void reserve( std::size_t steps )
  {
    features.reserve( steps * n_inputs );
    mask.reserve( steps * n_actions );
    action.reserve( steps );
    log_prob.reserve( steps );
    time.reserve( steps );
    reward.reserve( steps );
    episode.reserve( steps );
    terminal.reserve( steps );
  }

  // Call at the start of every iteration. Discards the steps of an episode that
  // was never closed with end_episode(), so a missed end can't fuse two iterations.
  void begin_episode()
  {
    if ( pending ) truncate( episode_start );
    pending = false;
  }

  void record_decision( double t, double score, const float* feat, const uint8_t* legal, uint32_t chosen, float lp )
  {
    if ( pending && t < time.back() )
      throw std::runtime_error( "nn_trajectory_t: time went backwards within an episode" );
    if ( pending ) reward.back() = score - last_score;
    else episode_start = size();

    features.insert( features.end(), feat, feat + n_inputs );
    mask.insert( mask.end(), legal, legal + n_actions );
    action.push_back( chosen );
    log_prob.push_back( lp );
    time.push_back( t );
    reward.push_back( 0.0 );  // filled in by the next decision or end_episode()
    episode.push_back( n_episodes );
    terminal.push_back( 0 );

    last_score = score;
    pending    = true;
  }

  void end_episode( double final_score )
  {
    if ( pending )  // an iteration with no decisions contributes nothing
    {
      reward.back()   = final_score - last_score;
      terminal.back() = 1;
      ++n_episodes;
    }
    pending = false;
  }

  // Concatenates another (finished) buffer, renumbering its episodes.
  void append( const nn_trajectory_t& other )
  {
    if ( other.n_inputs != n_inputs || other.n_actions != n_actions )
      throw std::runtime_error( "nn_trajectory_t::append: layout mismatch" );
    if ( pending || other.pending )
      throw std::runtime_error( "nn_trajectory_t::append: unfinished episode" );

    features.insert( features.end(), other.features.begin(), other.features.end() );
    mask.insert( mask.end(), other.mask.begin(), other.mask.end() );
    action.insert( action.end(), other.action.begin(), other.action.end() );
    log_prob.insert( log_prob.end(), other.log_prob.begin(), other.log_prob.end() );
    time.insert( time.end(), other.time.begin(), other.time.end() );
    reward.insert( reward.end(), other.reward.begin(), other.reward.end() );
    terminal.insert( terminal.end(), other.terminal.begin(), other.terminal.end() );
    for ( uint32_t e : other.episode ) episode.push_back( e + n_episodes );
    n_episodes += other.n_episodes;
  }

  uint32_t episodes() const { return n_episodes; }

private:
  void truncate( std::size_t steps )
  {
    features.resize( steps * n_inputs );
    mask.resize( steps * n_actions );
    action.resize( steps );
    log_prob.resize( steps );
    time.resize( steps );
    reward.resize( steps );
    episode.resize( steps );
    terminal.resize( steps );
  }

  std::size_t episode_start = 0;
  uint32_t n_episodes = 0;
  double last_score   = 0.0;
  bool pending        = false;
};
