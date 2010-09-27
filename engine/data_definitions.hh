#ifndef DATA_DEFINITIONS_HH
#define DATA_DEFINITIONS_HH

// Spell.dbc
struct spell_data_t {
  const char * name;               // Spell name from Spell.dbc stringblock (enGB)
  unsigned     id;                 // Spell ID in dbc
  unsigned     flags;              // Unused for now, 0x00 for all
  double       prj_speed;          // Projectile Speed
  unsigned     school;             // Spell school mask
  int          power_type;         // Resource type
  unsigned     class_mask;         // Class mask for spell
  unsigned     race_mask;          // Racial mask for the spell
  int          scaling_type;       // Array index for gtSpellScaling.dbc. -1 means the last sub-array
  // SpellLevels.dbc
  unsigned     spell_level;        // Spell learned on level. NOTE: Only accurate for "class abilities"
  // SpellRange.dbc
  double       min_range;          // Minimum range (in yards?)
  double       max_range;          // Maximum range (in yards?)
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
  // SpellEffect.dbc
  unsigned     effect[3];          // Effect identifiers
  // Spell.dbc flags
  unsigned     attributes[10];     // Spell.dbc "flags", record field 1..10, note that 12694 added a field here after flags_7
  const char * desc;               // Spell.dbc description stringblock
  const char * tooltip;            // Spell.dbc tooltip stringblock
};

// SpellEffect.dbc
struct spelleffect_data_t {
  unsigned    id;              // Effect id
  unsigned    flags;           // Unused for now, 0x00 for all
  unsigned    spell_id;        // Spell this effect belongs to
  unsigned    index;           // Effect index for the spell
  int         type;            // Effect type
  int         subtype;         // Effect sub-type
  // SpellScaling.dbc
  double      m_avg;           // Effect average spell scaling multiplier
  double      m_delta;         // Effect delta spell scaling multiplier
  double      m_unk;           // 
  // 
  double      coeff;           // Effect coefficient
  double      amplitude;       // Effect amplitude (e.g., tick time)
  // SpellRadius.dbc
  double      radius;          // Spell radius variables
  double      radius_max;
  // 
  int         base_value;      // Effect value
  int         misc_value;      // Effect miscellaneous value
  int         misc_value_2;    // Effect miscellaneous value
  int         trigger_spell;   // Effect triggers this spell id
  double      pp_combo_points; // Effect points per combo points
  double      real_ppl;        // Effect real points per level
  int         die_sides;       // Effect damage range
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
};

#endif
