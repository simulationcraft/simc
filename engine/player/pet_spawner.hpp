// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef PET_SPAWNER_HPP
#define PET_SPAWNER_HPP

#include "config.hpp"
#include "spawner_base.hpp"
#include "player/player.hpp"

namespace spawner
{
enum pet_spawn_type
{
  PET_SPAWN_DYNAMIC,      /// A dynamic pet that can be created during simulation run time
  PET_SPAWN_PERSISTENT    /// A persistent pet that is alive through the simulation run
};

/// Pet replacement strategy when maximum number of active pets are up during spawn
enum pet_replacement_strategy
{
  NO_REPLACE,            /// Replace nothing (nothing is spawned if max number of pets up)
  REPLACE_OLDEST,        /// Replace oldest active pet (pet with least remaining time)
  REPLACE_RANDOM,        /// Replace a random active pet
};

/// Pet spawner event types
enum class pet_event_type : unsigned
{
  PRE_SPAWN = 0U, // The pet has been selected for summoning, but is still inactive
  SPAWN,          // The pet has been summoned and is active
  PRE_DESPAWN,    // The pet has been selected for despawning, but is still active
  DESPAWN,        // The pet has been despawned and is inactive
  // Normal pet arise/demise events chained through pet spawner's own arise/demise callbacks
  ARISE,          // The pet has been summoned and is active
  DEMISE,         // The pet has been dismissed and is inactive
};

/**
 * A wrapper object to enable dynamic pet spawns in simulationcraft. Normally, all pets must be
 * fully initialized at the beginning of the simulation run (i.e., init phase). This wrapper class
 * allows for the creation of pets during simulation run time.
 *
 * Primary use is for dynamically spawned pets as a result of a proc or such.
 *
 * The class uses two template parameters, T which is the type of the pet to be dynamically
 * spawned (e.g., a pet class type in a class module), and O which is the type of the owner who
 * performs the spawning (e.g., warlock_t). The owner type can be omitted, in which case it defaults
 * to player_t.
 *
 * Each pet spawner object is associated with an unique case insensitive textual id, which should
 * correlate with the tokenized name of the pet (e.g., wild_imp). The id is used in various places
 * to find a spawner context for a pet (for example expressions, or data analysis).
 *
 * Note that since this wrapper object breaks the strict rule where each simulation thread has an
 * identical actor end state (in terms of actor objects created), the merging operation of pets
 * spawned by this wrapper object is no longer guaranteed to be deterministic (except under
 * deterministic simulations of the same user input).
 *
 * This means that the spawning order, or the number of dynamic pets across different threads is
 * no longer identical. As such, if during the merge process the main thread does not have enough
 * dynamic pets controlled by this object, it will create new ones until the requirement is
 * satisfied.
 *
 */
template<typename T, typename O = player_t>
class pet_spawner_t : public base_actor_spawner_t
{
public:
  using create_fn_t = std::function<T*(O*)>;
  using apply_fn_t = std::function<void(T*)>;
  using check_arg_fn_t = std::function<bool(const T*)>;
  using check_fn_t = std::function<bool(void)>;
  using expr_fn_t = std::function<std::unique_ptr<expr_t>(util::span<const util::string_view>)>;
  using event_fn_t = std::function<void(pet_event_type, T*)>;

private:
  // Creation phase determines what components of the init process are run on the created pet
  enum create_phase
  {
    PHASE_COMBAT,       /// Pet is being created during combat
    PHASE_MERGE,        /// Pet is being created in the main thread during merge as a placeholder
    PHASE_INIT          /// Pet is being created in the initialization phase of the thread
  };

  /// Maximum number of active pets (defaults: dynamic = infinite, persistent = 1)
  unsigned        m_max_pets;
  /// Pet construction function (default: new T( m_owner ));
  create_fn_t     m_creator;
  /// Check function for creation of persistent pets
  check_fn_t      m_check_create;
  /// A list of all dynamically spawned, active, and inactive pets of type T, respectively
  std::vector<T*> m_pets, m_active_pets, m_inactive_pets;
  /// Pre-defined duration for the spawn
  timespan_t      m_duration;
  /// Type of spawn
  pet_spawn_type  m_type;
  /// Replacement strategy
  pet_replacement_strategy m_replacement_strategy;

  // Callbacks

  /// Creation callbacks
  std::vector<apply_fn_t> m_event_create;

  /// Event-based callbacks
  std::vector<std::pair<pet_event_type, event_fn_t>> m_event_callbacks;

  // Expressions
  std::unordered_map<std::string, expr_fn_t> m_expressions;

  // Iteration uptime calculation
  timespan_t m_cumulative_uptime;
  timespan_t m_spawn_time;

  // Bookkeeping optimization

  /// Pet states changed, must recompute active/inactive pet vectors
  bool m_dirty;
  /// Number of currently active pets
  size_t m_active;
  /// First created pet, required for proper data collection
  T* m_initial_pet;

  // Internal helper methods

  /// Create and initialize a completely new pet
  T* create_pet( create_phase phase );

  /// Recreate m_active pets, m_inactive pets if m_dirty == 1
  void update_state();

  /// Select replacement pet
  T* replacement_pet();

  /// Invoke event callbacks helper
  void _invoke_callbacks( pet_event_type t, T* selected_pet );
public:
  pet_spawner_t( util::string_view id, O* p, pet_spawn_type st = PET_SPAWN_DYNAMIC );
  pet_spawner_t( util::string_view id, O* p, unsigned max_pets,
                   pet_spawn_type st = PET_SPAWN_DYNAMIC );
  pet_spawner_t( util::string_view id, O* p, unsigned max_pets, const create_fn_t& creator,
                   pet_spawn_type st = PET_SPAWN_DYNAMIC );
  pet_spawner_t( util::string_view id, O* p, const create_fn_t& creator,
                   pet_spawn_type st = PET_SPAWN_DYNAMIC );

  /// Spawn n new pets (defaults, 1 for dynamic, max_pets for persistent),
  /// return spawned pets as a vector of pointers
  std::vector<T*> spawn( timespan_t duration = timespan_t::min(), unsigned n = 0 );
  /// Spawns n pets using the default summon duration
  std::vector<T*> spawn( unsigned n );
  /// Despawn all active pets, return number of pets despawned
  size_t despawn();
  /// Despawn a specific active pet, return true if pet was despawned
  bool despawn( T* obj );
  /// Despawn a set of active pets, return number of pets despawned
  size_t despawn( const std::vector<T*>& obj );
  /// Despawn a set of active pets based on a functor, return number of pets despawned
  size_t despawn( const check_arg_fn_t& fn );

  /// Adjust the expiration time of active pets of type T
  void extend_expiration( timespan_t adjustment );

  /// Number of active pets of type T
  size_t n_active_pets() const;
  /// Number of active pets of type T satisfying fn check
  size_t n_active_pets( const check_arg_fn_t& fn );
  /// Number of inactive pets of type T
  size_t n_inactive_pets() const;
  /// Total number of active and inactive pets of type T
  size_t n_pets() const;
  /// Maximum number of active pets
  size_t max_pets() const;
  /// Default spawning duration
  timespan_t duration() const;

  // Iteration
  typename std::vector<T*>::iterator begin();
  typename std::vector<T*>::iterator end();
  typename std::vector<T*>::const_iterator cbegin();
  typename std::vector<T*>::const_iterator cend();

  /// Sets the maximum number of active pets
  pet_spawner_t<T, O>& set_max_pets( unsigned v );
  /// Sets the creation callback for the pet
  pet_spawner_t<T, O>& set_creation_callback( const create_fn_t& fn );
  /// Set creation check callback for persistent pets. Dynamic spawns will always be created.
  pet_spawner_t<T, O>& set_creation_check_callback( const check_fn_t& fn );
  /// Set (add) creation callback. Creation callbacks will be invoked for every new pet
  pet_spawner_t<T, O>& set_creation_event_callback( const apply_fn_t& fn );
  /// Set (add) event-type based callback.
  pet_spawner_t<T, O>& set_event_callback( pet_event_type t, const event_fn_t& fn );
  pet_spawner_t<T, O>& set_event_callback( std::initializer_list<pet_event_type> t, const event_fn_t& fn );
  /// Set default duration based on a spell
  pet_spawner_t<T, O>& set_default_duration( const spell_data_t* spell );
  /// Set default duration based on a spell
  pet_spawner_t<T, O>& set_default_duration( const spell_data_t& spell );
  /// Set default duration based on a timespan object
  pet_spawner_t<T, O>& set_default_duration( timespan_t duration );
  /// Registers custom expressions for the pet type
  pet_spawner_t<T, O>& add_expression( const std::string& name, const expr_fn_t& fn );

  /// Sets the replacement strategy of spawned pets if maximum is reached
  pet_spawner_t<T, O>&  set_replacement_strategy( pet_replacement_strategy new_ );

  // Access

  /// All pets
  std::vector<T*> pets();
  /// All pets (read only)
  std::vector<const T*> pets() const;
  /// All currently active pets
  std::vector<T*> active_pets();
  /// Access the zero-indexth created pet, nullptr if no pets are created
  T* pet( size_t index = 0 ) const;
  /// Access the zero-indexth active pet, nullptr if no pet of index active
  T* active_pet( size_t index = 0 );
  /// Access the first active pet based on an unary check function, nullptr if no pet found
  T* active_pet( const check_arg_fn_t& fn );
  /// Acccess the active pet with highest remaining time left using an optional unary constraint
  T* active_pet_max_remains( const check_arg_fn_t& fn = check_arg_fn_t() );
  /// Acccess the active pet with lowest remaining time left using an optional unary constraint
  T* active_pet_min_remains( const check_arg_fn_t& fn = check_arg_fn_t() );

  // Pure virtual implementations

  /// Merge dynamic pet information
  void merge( base_actor_spawner_t* other ) override;

  /// Creates persistent pet objects during pet creation
  void create_persistent_actors() override;

  /// Returns a pet-related expression
  std::unique_ptr<expr_t> create_expression( util::span<const util::string_view> expr, util::string_view full_expression_str ) override;

  /// Returns total uptime of the spawned pet type
  timespan_t iteration_uptime() const override;

  /// Reset internal state
  void reset() override;

  /// Collect statistical data
  void datacollection_end() override;
};
} // Namespace spawner ends

#include "pet_spawner_impl.hpp"

#endif /* PET_SPAWNER_HPP */
