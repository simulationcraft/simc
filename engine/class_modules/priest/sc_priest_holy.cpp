#include "simulationcraft.hpp"
#include "sc_priest.hpp"

namespace priestspace 
{
  namespace actions
  {
    namespace spells
    {
       /// Holy Fire Base Spell, used for both Holy Fire and its overriding spell Purge the Wicked
      struct holy_fire_base_t : public priest_spell_t
      {
        holy_fire_base_t(const std::string& name, priest_t& p, const spell_data_t* sd) : priest_spell_t(name, p, sd)
        {
          procs_courageous_primal_diamond = false;
        }

        void execute() override
        {
          priest_spell_t::execute();

          priest().buffs.holy_evangelism->trigger();
        }

        double action_multiplier() const override
        {
          double m = priest_spell_t::action_multiplier();

          m *= 1.0 + (priest().buffs.holy_evangelism->check() * priest().buffs.holy_evangelism->data().effectN(1).percent());

          return m;
        }
      };

      struct holy_fire_t final : public holy_fire_base_t
      {
        holy_fire_t(priest_t& player, const std::string& options_str)
          : holy_fire_base_t("holy_fire", player, player.find_class_spell("Holy Fire"))
        {
          parse_options(options_str);
        }
      };
    } // Spells Namespace

  } // Actions Namespace

  void priest_t::create_buffs_holy()
  {    

  }

  void priest_t::init_rng_holy()
  {
   
  }

  void priest_t::init_spells_holy()
  {
    // Talents 
    // T15
    talents.trail_of_light       = find_talent_spell("Trail of Light");
    talents.enduring_renewal     = find_talent_spell("Enduring Renewal");
    talents.enlightenment        = find_talent_spell("Enlightenment");
    // T30
    talents.angelic_feather      = find_talent_spell("Angelic Feather");
    talents.body_and_mind        = find_talent_spell("Body and Mind");
    talents.perseverance         = find_talent_spell("Perseverance");
    // T45
    talents.light_of_the_naaru   = find_talent_spell("Light of the Naaru");
    talents.guardian_angel       = find_talent_spell("Guardian Angel");
    talents.after_life           = find_talent_spell("After Life");  
    // T60
    talents.shining_force        = find_talent_spell("Shining Force");
    talents.psychic_voice        = find_talent_spell("Psychic Voice");
    talents.censure              = find_talent_spell("Censure");
    // T75
    talents.surge_of_light       = find_talent_spell("Surge of Light");
    talents.binding_heal         = find_talent_spell("Binding Heal");
    talents.circle_of_healing    = find_talent_spell("Circle of Healing");
    // T90
    talents.benediction          = find_talent_spell("Benediction");
    talents.divine_star          = find_talent_spell("Divine Star");
    talents.halo                 = find_talent_spell("Halo");
    // T100
    talents.cosmic_ripple        = find_talent_spell("Cosmic Ripple");
    talents.apotheosis           = find_talent_spell("Apotheosis");
    talents.holy_word_salvation  = find_talent_spell("Holy Word: Salvation");

    // General Spells
    specs.meditation_holy     = find_specialization_spell("Meditation");
    specs.serendipity         = find_specialization_spell("Serendipity");
    specs.rapid_renewal       = find_specialization_spell("Rapid Renewal");
    specs.divine_providence   = find_specialization_spell("Divine Providence");
    specs.focused_will        = find_specialization_spell("Focused Will");

    // Range Based on Talents
    if (base.distance != 5)
    {
      if (talents.divine_star->ok())
        base.distance = 24.0;
      else if (talents.halo->ok())
        base.distance = 27.0;
      else
        base.distance = 27.0;
    }

  }

  action_t* priest_t::create_action_holy( const std::string& name, const std::string& options_str )
  {
    using namespace actions::spells;
    using namespace actions::heals;
    
    if (name == "holy_fire" )            return new holy_fire_t(*this, options_str);

    return nullptr;
  }

  expr_t* priest_t::create_expression_holy(action_t* a, const std::string& name_str)
  {
    return nullptr;
  }

  /** Holy Damage Combat Action Priority List */
  void priest_t::generate_apl_holy_d()
  {
    create_apl_default();
  }

  /** Holy Heal Combat Action Priority List */
  void priest_t::generate_apl_holy_h()
  {
    create_apl_default();
  }

  
} // Priest Namespace