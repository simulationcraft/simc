// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "util/timespan.hpp"
#include "util/span.hpp"
#include "util/string_view.hpp"

#include <string>
#include <memory>

struct expr_t;
struct player_t;
struct sim_t;

namespace spawner
{
  void merge(sim_t& parent_sim, sim_t& other_sim);
  void create_persistent_actors(player_t& player);

  // Minimal base class to store in owner actors automatically, all functionality should be
  // implemented in a templated class (pet_spawner_t for example). Methods that need to be invoked
  // from the core simulator should be declared here as pure virtual (must be implemented in the
  // derived class).
  class base_actor_spawner_t
  {
  protected:
    std::string m_name;
    player_t* m_owner;

  public:
    base_actor_spawner_t(util::string_view id, player_t* o) :
      m_name(id), m_owner(o)
    {
      register_object();
    }

    virtual ~base_actor_spawner_t()
    { }

    const std::string& name() const
    {
      return m_name;
    }

    virtual void create_persistent_actors() = 0;

    // Data merging
    virtual void merge(base_actor_spawner_t* other) = 0;

    // Expressions
    virtual std::unique_ptr<expr_t> create_expression(util::span<const util::string_view> expr, util::string_view full_expression_str) = 0;

    // Uptime
    virtual timespan_t iteration_uptime() const = 0;

    // State reset
    virtual void reset() = 0;

    // Data collection
    virtual void datacollection_end() = 0;

  private:
    // Register this pet spawner object to owner
    void register_object();
  };
} // Namespace spawner ends