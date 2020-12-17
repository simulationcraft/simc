# Python APL Converter: APL text to C++ code

## Usage: >python ConvertAPL.py -i path/to/input/file -o path/to/output/file -s spec

### -i input file documentation
The input file can be any text file type, but must only contain APL commands (``actions=...``, ``actions+=/...``, ``actions.bla+=/``, etc.) 
and APL comments (``# This is a comment``).

Example:
```
actions=auto_attack
# Only use Obliterate if it will not waste runic power
actions+=/obliterate,if=runic_power.deficit>=20
actions+=/run_action_list,name=spenders
actions.spenders=frost_strike
```

**DO NOT use** with a full profile (character definition, sim options, etc.) or things **will** break.

### -o output file documentation

The output file has to be an already existing file, that contains a C++ method named after the 'spec' argument surrounded with //\<spec\>_apl_start and
//\<spec\>_apl_end comments, where the generated APL code will be placed.

Example:
```cpp
//frost_apl_start
void frost( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* spenders = p->get_action_priority_list( "spenders" );

  default_->add_action( "auto_attack" );
  default_->add_action( "obliterate,if=runic_power.deficit>=20", "Only use Obliterate if it will not waste runic power" );
  default_->add_action( "run_action_list,name=spenders" );

  spenders->add_action( "frost_strike" );
}
//frost_apl_end
```

### -s spec documentation

The final argument 'spec' has to be a member of specList, defined in 
[ConvertAPL.py](https://github.com/simulationcraft/simc/blob/shadowlands/engine/class_modules/apl/ConvertAPL.py#L102)
