#include "class_modules/apl/apl_demon_hunter.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace demon_hunter_apl
{

//vengeance_apl_start
void vengeance( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* brand = p->get_action_priority_list( "brand" );
  action_priority_list_t* defensives = p->get_action_priority_list( "defensives" );
  action_priority_list_t* cooldowns = p->get_action_priority_list( "cooldowns" );
  action_priority_list_t* normal = p->get_action_priority_list( "normal" );

  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  default_->add_action( "auto_attack" );
  default_->add_action( "variable,name=brand_build,value=talent.agonizing_flames&talent.burning_alive&talent.charred_flesh" );
  default_->add_action( "disrupt" );
  default_->add_action( "call_action_list,name=brand,if=variable.brand_build" );
  default_->add_action( "call_action_list,name=defensives" );
  default_->add_action( "call_action_list,name=cooldowns" );
  default_->add_action( "call_action_list,name=normal" );

  brand->add_action( "fiery_brand" );
  brand->add_action( "immolation_aura,if=dot.fiery_brand.ticking" );

  defensives->add_action( "demon_spikes" );
  defensives->add_action( "metamorphosis,if=!buff.metamorphosis.up|target.time_to_die<15" );
  defensives->add_action( "fiery_brand" );

  cooldowns->add_action( "potion" );
  cooldowns->add_action( "use_items" );
  cooldowns->add_action( "the_hunt" );
  cooldowns->add_action( "elysian_decree" );

  normal->add_action( "infernal_strike" );
  normal->add_action( "bulk_extraction" );
  normal->add_action( "spirit_bomb,if=((buff.metamorphosis.up&talent.fracture.enabled&soul_fragments>=3)|soul_fragments>=4)" );
  normal->add_action( "fel_devastation" );
  normal->add_action( "soul_cleave,if=((talent.spirit_bomb.enabled&soul_fragments=0)|!talent.spirit_bomb.enabled)&((talent.fracture.enabled&fury>=55)|(!talent.fracture.enabled&fury>=70)|cooldown.fel_devastation.remains>target.time_to_die|(buff.metamorphosis.up&((talent.fracture.enabled&fury>=35)|(!talent.fracture.enabled&fury>=50))))" );
  normal->add_action( "immolation_aura,if=((variable.brand_build&cooldown.fiery_brand.remains>10)|!variable.brand_build)&fury.deficit>=10" );
  normal->add_action( "felblade,if=fury.deficit>=40" );
  normal->add_action( "fracture,if=((talent.spirit_bomb.enabled&soul_fragments<=3)|(!talent.spirit_bomb.enabled&((buff.metamorphosis.up&fury.deficit>=45)|(buff.metamorphosis.down&fury.deficit>=30))))" );
  normal->add_action( "sigil_of_flame" );
  normal->add_action( "shear" );
  normal->add_action( "throw_glaive" );
}
//vengeance_apl_end

}