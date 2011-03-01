#ifndef DATA_DEFINITIONS_HH
#define DATA_DEFINITIONS_HH

#include "data_enums.hh"

// Spell.dbc

#ifdef __OpenBSD__
#pragma pack(1)
#else
#pragma pack( push, 1 )
#endif

struct spell_data_t;
struct spelleffect_data_t;
struct talent_data_t;

struct dbc_t
{
  static void set_ptr( bool );
  static bool get_ptr();
  static const char* build_level();
  static const char* wow_version();
  static void init();
  static void de_init();
  static int glyphs( std::vector<unsigned>& glyph_ids, int cid );
  
  static void create_spell_data_index();
  static spell_data_t** get_spell_data_index();
  static unsigned get_spell_data_index_size();

  static void create_spelleffect_data_index();
  static spelleffect_data_t** get_spelleffect_data_index();
  static unsigned get_spelleffect_data_index_size();

  static void create_talent_data_index();
  static talent_data_t** get_talent_data_index();
  static unsigned get_talent_data_index_size();
};

struct spell_data_t {
  const char * name;               // Spell name from Spell.dbc stringblock (enGB)
  unsigned     id;                 // Spell ID in dbc
  unsigned     flags;              // Unused for now, 0x00 for all
  double       prj_speed;          // Projectile Speed
  unsigned     school;             // Spell school mask
  int          power_type;         // Resource type
  unsigned     class_mask;         // Class mask for spell
  unsigned     race_mask;          // Racial mask for the spell
  int          scaling_type;       // Array index for gtSpellScaling.dbc. -1 means the last sub-array, 0 disabled
  double       extra_coeff;        // An "extra" coefficient (used for some spells to indicate AP based coefficient)
  // SpellLevels.dbc
  unsigned     spell_level;        // Spell learned on level. NOTE: Only accurate for "class abilities"
  unsigned     max_level;          // Maximum level for scaling
  // SpellRange.dbc
  double       min_range;          // Minimum range in yards
  double       max_range;          // Maximum range in yards
  // SpellCooldown.dbc
  unsigned     cooldown;           // Cooldown in milliseconds
  unsigned     gcd;                // GCD in milliseconds
  // SpellCategories.dbc
  unsigned     category;           // Spell category (for shared cooldowns, effects?)
  // SpellDuration.dbc
  double       duration;           // Spell duration in milliseconds
  // SpellPower.dbc
  unsigned     cost;               // Resource cost
  // SpellRuneCost.dbc
  unsigned     rune_cost;          // Bitmask of rune cost 0x1, 0x2 = Blood | 0x4, 0x8 = Unholy | 0x10, 0x20 = Frost
  unsigned     runic_power_gain;   // Amount of runic power gained ( / 10 )
  // SpellAuraOptions.dbc
  unsigned     max_stack;          // Maximum stack size for spell
  unsigned     proc_chance;        // Spell proc chance in percent
  unsigned     proc_charges;       // Per proc charge amount
  // SpellScaling.dbc
  int          cast_min;           // Minimum casting time in milliseconds
  int          cast_max;           // Maximum casting time in milliseconds
  int          cast_div;           // A divisor used in the formula for casting time scaling (20 always?)
  double       c_scaling;          // A scaling multiplier for level based scaling
  unsigned     c_scaling_level;    // A scaling divisor for level based scaling
  // SpellEffect.dbc
  unsigned     effect[3];          // Effect identifiers
  // Spell.dbc flags
  unsigned     attributes[10];     // Spell.dbc "flags", record field 1..10, note that 12694 added a field here after flags_7
  const char * desc;               // Spell.dbc description stringblock
  const char * tooltip;            // Spell.dbc tooltip stringblock
  // SpellDescriptionVariables.dbc
  const char * desc_vars;          // Spell description variable stringblock, if present

  // Pointers for runtime linking
  spelleffect_data_t* effect1;
  spelleffect_data_t* effect2;
  spelleffect_data_t* effect3;

  static spell_data_t* list();
  static spell_data_t* nil();
  static spell_data_t* find( unsigned, const std::string& confirmation = std::string() );
  static spell_data_t* find( const std::string& name );
  static void link();
};

// SpellEffect.dbc
struct spelleffect_data_t {
  unsigned         id;              // Effect id
  unsigned         flags;           // Unused for now, 0x00 for all
  unsigned         spell_id;        // Spell this effect belongs to
  unsigned         index;           // Effect index for the spell
  effect_type_t    type;            // Effect type
  effect_subtype_t subtype;         // Effect sub-type
  // SpellScaling.dbc
  double           m_avg;           // Effect average spell scaling multiplier
  double           m_delta;         // Effect delta spell scaling multiplier
  double           m_unk;           // Unused effect scaling multiplier
  // 
  double           coeff;           // Effect coefficient
  double           amplitude;       // Effect amplitude (e.g., tick time)
  // SpellRadius.dbc
  double           radius;          // Minimum spell radius
  double           radius_max;      // Maximum spell radius
  // 
  int              base_value;      // Effect value
  int              misc_value;      // Effect miscellaneous value
  int              misc_value_2;    // Effect miscellaneous value 2
  int              trigger_spell_id;// Effect triggers this spell id
  double           m_chain;         // Effect chain multiplier
  double           pp_combo_points; // Effect points per combo points
  double           real_ppl;        // Effect real points per level
  int              die_sides;       // Effect damage range

  // Pointers for runtime linking
  spell_data_t* spell;
  spell_data_t* trigger_spell;

  static spelleffect_data_t* list();
  static spelleffect_data_t* nil();
  static spelleffect_data_t* find( unsigned );
  static void link();
};

struct talent_data_t {
  const char * name;        // Talent name
  unsigned     id;          // Talent id
  unsigned     flags;       // Unused for now, 0x00 for all
  unsigned     tab_page;    // Talent tab page
  unsigned     m_class;     // Class mask
  unsigned     m_pet;       // Pet mask
  unsigned     dependance;  // Talent depends on this talent id
  unsigned     depend_rank; // Requires this rank of depended talent
  unsigned     col;         // Talent column
  unsigned     row;         // Talent row
  unsigned     rank_id[3];  // Talent spell rank identifiers for ranks 1..3

  // Pointers for runtime linking
  spell_data_t* spell1;
  spell_data_t* spell2;
  spell_data_t* spell3;

  static talent_data_t* list();
  static talent_data_t* nil();
  static talent_data_t* find( unsigned, const std::string& confirmation = std::string() );
  static talent_data_t* find( const std::string& name );
  static void link();
};

struct random_prop_data_t {
  unsigned ilevel;
  double   p_epic[5];
  double   p_rare[5];
  double   p_uncommon[5];
};

struct random_suffix_data_t {
  unsigned    id;
  const char* suffix;
  unsigned    enchant_id[5];
  unsigned    enchant_alloc[5];
};

struct item_enchantment_data_t {
  unsigned    id;
  const char* name;
  unsigned    id_gem;
  unsigned    ench_type[3];          // item_enchantment
  unsigned    ench_amount[3];
  unsigned    ench_prop[3];
};

struct item_data_t {
  unsigned id;
  const char* name;
  const char* icon;               // Icon filename
  unsigned flags_1;
  unsigned flags_2;
  int      level;                 // Ilevel
  int      req_level;
  int      quality;
  int      inventory_type;
  int      item_class;
  int      item_subclass;
  int      bind_type;
  double   delay;
  double   dmg_range;
  double   item_modifier;
  int      race_mask;
  int      class_mask;
  int      stat_type[10];         // item_mod_type
  int      stat_val[10];
  int      id_spell[5];
  int      trigger_spell[5];      // item_spell_trigger_type
  int      cooldown_spell[5];
  int      cooldown_category[5];
  int      socket_color[3];       // item_socket_color
  int      gem_properties;
  int      id_socket_bonus;

  static item_data_t* list();
  static item_data_t* find( unsigned item_id );
};

struct item_scale_data_t {
  unsigned ilevel;
  double   values[7];             // quality based values for dps
};

struct item_armor_type_data_t {
  unsigned ilevel;
  double   armor_type[4];
};

struct gem_property_data_t {
  unsigned id;
  unsigned enchant_id;
  unsigned color;
};

#ifdef __OpenBSD__
#pragma pack()
#else
#pragma pack( pop )
#endif

#endif

