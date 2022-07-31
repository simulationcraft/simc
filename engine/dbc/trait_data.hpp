// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SC_TRAIT_DATA_HPP
#define SC_TRAIT_DATA_HPP

#include "util/span.hpp"
#include "util/string_view.hpp"

#include "client_data.hpp"
#include "sc_enums.hpp"
#include "specialization.hpp"

#include <vector>

struct trait_data_t
{
  unsigned    tree_index;
  unsigned    id_class;
  unsigned    id_trait_node_entry;
  unsigned    max_ranks;
  unsigned    req_points;
  unsigned    id_trait_definition;
  unsigned    id_spell;
  unsigned    id_override_spell;
  short       row;
  short       col;
  short       selection_index;
  const char* name;
  std::array<unsigned, 5> id_spec;
  std::array<unsigned, 5> id_spec_starter;

  // static functions
  static const trait_data_t* find( unsigned trait_node_entry_id, bool ptr = false );
  static const trait_data_t* find( talent_tree tree, util::string_view name, unsigned class_id, specialization_e spec, bool ptr = false );
  static const trait_data_t* find_tokenized( talent_tree tree, util::string_view name, unsigned class_id, specialization_e spec, bool ptr = false );
  static std::vector<const trait_data_t*> find_by_spell( talent_tree tree, unsigned spell_id, unsigned class_id = 0, specialization_e spec = SPEC_NONE, bool ptr = false );

  static util::span<const trait_data_t> data( bool ptr = false );
  static util::span<const trait_data_t> data( talent_tree tree, bool ptr = false );
  static util::span<const trait_data_t> data( unsigned class_id, talent_tree tree, bool ptr = false );

  static const trait_data_t& nil()
  { return dbc::nil<trait_data_t>; }
};

struct trait_definition_effect_entry_t
{
  unsigned  id_trait_definition;
  unsigned  effect_index;
  unsigned  operation;
  unsigned  id_curve;

  // static functions
  static util::span<const trait_definition_effect_entry_t> find( unsigned id, bool ptr = false );

  static util::span<const trait_definition_effect_entry_t> data( bool ptr = false );

  static const trait_definition_effect_entry_t& nil()
  { return dbc::nil<trait_definition_effect_entry_t>; }
};

#endif // SC_TRAIT_DATA_HPP

