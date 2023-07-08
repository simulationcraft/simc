#include "simulationcraft.hpp"
#include "class_modules/apl/apl_hunter.hpp"

namespace hunter_apl {

std::string potion( const player_t* p )
{
  return ( p -> true_level > 60 ) ? "elemental_potion_of_ultimate_power_3" : 
         ( p -> true_level > 50 ) ? "spectral_agility" :
         ( p -> true_level >= 40 ) ? "unbridled_fury" :
         "disabled";
}

std::string flask( const player_t* p )
{
  return ( p -> true_level > 60 ) ? "phial_of_tepid_versatility_3" :
         ( p -> true_level > 50 ) ? "spectral_flask_of_power" :
         ( p -> true_level >= 40 ) ? "greater_flask_of_the_currents" :
         "disabled";
}

std::string food( const player_t* p )
{
  return ( p -> true_level >= 70 ) ? "fated_fortune_cookie" : 
         ( p -> true_level >= 60 ) ? "feast_of_gluttonous_hedonism" :
         ( p -> true_level >= 45 ) ? "bountiful_captains_feast" :
         "disabled";
}

std::string rune( const player_t* p )
{
  return ( p -> true_level >= 70 ) ? "draconic" :
         ( p -> true_level >= 60 ) ? "veiled" :
         ( p -> true_level >= 50 ) ? "battle_scarred" :
         "disabled";
}

std::string temporary_enchant( const player_t* p )
{
  std::string lvl70_temp_enchant = ( p -> specialization() == HUNTER_SURVIVAL ) ? "main_hand:howling_rune_3" : "main_hand:completely_safe_rockets_3";
  std::string lvl60_temp_enchant = ( p -> specialization() == HUNTER_SURVIVAL ) ? "main_hand:shaded_sharpening_stone" : "main_hand:shadowcore_oil";

  return ( p -> true_level >= 70 ) ? lvl70_temp_enchant :
         ( p -> true_level >= 60 ) ? lvl60_temp_enchant :
         "disabled";
}

//beast_mastery_apl_start
void beast_mastery( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* cleave = p->get_action_priority_list( "cleave" );
  action_priority_list_t* st = p->get_action_priority_list( "st" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );

  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "summon_pet" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "use_item,name=algethar_puzzle_box" );
  precombat->add_action( "steel_trap,precast_time=1.5,if=!talent.wailing_arrow&talent.steel_trap" );

  default_->add_action( "auto_shot" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "call_action_list,name=st,if=active_enemies<2|!talent.beast_cleave&active_enemies<3" );
  default_->add_action( "call_action_list,name=cleave,if=active_enemies>2|talent.beast_cleave&active_enemies>1" );

  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.bestial_wrath.up|cooldown.bestial_wrath.remains<30", "Call for Power Infusion when Bestial Wrath is running, or is close to coming off cooldown (PI lasts 20 seconds and Bestial Wrath lasts 15, so 30 seconds when accounting for Barbed Shot reduction of Bestial Wrath is a good approximate fit)." );
  cds->add_action( "berserking,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<13" );
  cds->add_action( "blood_fury,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<16" );
  cds->add_action( "ancestral_call,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<16" );
  cds->add_action( "fireblood,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&buff.bestial_wrath.up|fight_remains<9" );
  cds->add_action( "potion,if=buff.call_of_the_wild.up|!talent.call_of_the_wild&(buff.bestial_wrath.up&(buff.bloodlust.up|target.health.pct<20))|fight_remains<31" );

  cleave->add_action( "barbed_shot,target_if=max:debuff.latent_poison.stack,if=debuff.latent_poison.stack>9&(pet.main.buff.frenzy.up&pet.main.buff.frenzy.remains<=gcd+0.25|talent.scent_of_blood&cooldown.bestial_wrath.remains<12+gcd|full_recharge_time<gcd&cooldown.bestial_wrath.remains)" );
  cleave->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=pet.main.buff.frenzy.up&pet.main.buff.frenzy.remains<=gcd+0.25|talent.scent_of_blood&cooldown.bestial_wrath.remains<12+gcd|full_recharge_time<gcd&cooldown.bestial_wrath.remains" );
  cleave->add_action( "multishot,if=gcd-pet.main.buff.beast_cleave.remains>0.25" );
  cleave->add_action( "bestial_wrath" );
  cleave->add_action( "kill_command,if=talent.kill_cleave" );
  cleave->add_action( "call_of_the_wild" );
  cleave->add_action( "explosive_shot" );
  cleave->add_action( "stampede,if=buff.bestial_wrath.up|target.time_to_die<15" );
  cleave->add_action( "bloodshed" );
  cleave->add_action( "death_chakram" );
  cleave->add_action( "steel_trap" );
  cleave->add_action( "a_murder_of_crows" );
  cleave->add_action( "barbed_shot,target_if=max:debuff.latent_poison.stack,if=debuff.latent_poison.stack>9&(talent.wild_instincts&buff.call_of_the_wild.up|fight_remains<9|talent.wild_call&charges_fractional>1.2)" );
  cleave->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=talent.wild_instincts&buff.call_of_the_wild.up|fight_remains<9|talent.wild_call&charges_fractional>1.2" );
  cleave->add_action( "kill_command" );
  cleave->add_action( "dire_beast" );
  cleave->add_action( "serpent_sting,target_if=min:remains,if=refreshable&target.time_to_die>duration" );
  cleave->add_action( "barrage,if=pet.main.buff.frenzy.remains>execute_time" );
  cleave->add_action( "multishot,if=pet.main.buff.beast_cleave.remains<gcd*2" );
  cleave->add_action( "aspect_of_the_wild" );
  cleave->add_action( "cobra_shot,if=focus.time_to_max<gcd*2|buff.aspect_of_the_wild.up&focus.time_to_max<gcd*4" );
  cleave->add_action( "wailing_arrow,if=pet.main.buff.frenzy.remains>execute_time|fight_remains<5" );
  cleave->add_action( "bag_of_tricks,if=buff.bestial_wrath.down|target.time_to_die<5" );
  cleave->add_action( "arcane_torrent,if=(focus+focus.regen+30)<focus.max" );
  cleave->add_action( "kill_shot" );

  st->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=pet.main.buff.frenzy.up&pet.main.buff.frenzy.remains<=gcd+0.25|talent.scent_of_blood&pet.main.buff.frenzy.stack<3&cooldown.bestial_wrath.ready" );
  st->add_action( "kill_command,if=full_recharge_time<gcd&talent.alpha_predator" );
  st->add_action( "call_of_the_wild" );
  st->add_action( "death_chakram" );
  st->add_action( "bloodshed" );
  st->add_action( "stampede" );
  st->add_action( "a_murder_of_crows" );
  st->add_action( "steel_trap" );
  st->add_action( "explosive_shot" );
  st->add_action( "bestial_wrath" );
  st->add_action( "kill_command" );
  st->add_action( "barbed_shot,target_if=min:dot.barbed_shot.remains,if=talent.wild_instincts&buff.call_of_the_wild.up|talent.wild_call&charges_fractional>1.4|full_recharge_time<gcd&cooldown.bestial_wrath.remains|talent.scent_of_blood&(cooldown.bestial_wrath.remains<12+gcd|full_recharge_time+gcd<8&cooldown.bestial_wrath.remains<24+(8-gcd)+full_recharge_time)|fight_remains<9" );
  st->add_action( "dire_beast" );
  st->add_action( "serpent_sting,target_if=min:remains,if=refreshable&target.time_to_die>duration" );
  st->add_action( "kill_shot" );
  st->add_action( "aspect_of_the_wild" );
  st->add_action( "cobra_shot" );
  st->add_action( "wailing_arrow,if=pet.main.buff.frenzy.remains>execute_time|target.time_to_die<5" );
  st->add_action( "bag_of_tricks,if=buff.bestial_wrath.down|target.time_to_die<5" );
  st->add_action( "arcane_pulse,if=buff.bestial_wrath.down|target.time_to_die<5" );
  st->add_action( "arcane_torrent,if=(focus+focus.regen+15)<focus.max" );

  trinkets->add_action( "variable,name=sync_up,value=buff.call_of_the_wild.up|cooldown.call_of_the_wild.remains<2|!talent.call_of_the_wild&(prev_gcd.1.bestial_wrath|cooldown.bestial_wrath.remains_guess<2)" );
  trinkets->add_action( "variable,name=sync_remains,op=setif,value=cooldown.bestial_wrath.remains_guess,value_else=cooldown.call_of_the_wild.remains,condition=!talent.call_of_the_wild" );
  trinkets->add_action( "variable,name=trinket_1_stronger,value=!trinket.2.has_cooldown|trinket.1.has_use_buff&(!trinket.2.has_use_buff|trinket.2.cooldown.duration<trinket.1.cooldown.duration|trinket.2.cast_time<trinket.1.cast_time|trinket.2.cast_time=trinket.1.cast_time&trinket.2.cooldown.duration=trinket.1.cooldown.duration)|!trinket.1.has_use_buff&(!trinket.2.has_use_buff&(trinket.2.cooldown.duration<trinket.1.cooldown.duration|trinket.2.cast_time<trinket.1.cast_time|trinket.2.cast_time=trinket.1.cast_time&trinket.2.cooldown.duration=trinket.1.cooldown.duration))" );
  trinkets->add_action( "variable,name=trinket_2_stronger,value=!trinket.1.has_cooldown|trinket.2.has_use_buff&(!trinket.1.has_use_buff|trinket.1.cooldown.duration<trinket.2.cooldown.duration|trinket.1.cast_time<trinket.2.cast_time|trinket.1.cast_time=trinket.2.cast_time&trinket.1.cooldown.duration=trinket.2.cooldown.duration)|!trinket.2.has_use_buff&(!trinket.1.has_use_buff&(trinket.1.cooldown.duration<trinket.2.cooldown.duration|trinket.1.cast_time<trinket.2.cast_time|trinket.1.cast_time=trinket.2.cast_time&trinket.1.cooldown.duration=trinket.2.cooldown.duration))" );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=(trinket.1.has_use_buff&(variable.sync_up&(variable.trinket_1_stronger|trinket.2.cooldown.remains)|!variable.sync_up&(variable.trinket_1_stronger&(variable.sync_remains>trinket.1.cooldown.duration%2|trinket.2.has_use_buff&trinket.2.cooldown.remains>variable.sync_remains-15&trinket.2.cooldown.remains-5<variable.sync_remains&variable.sync_remains+40>fight_remains)|variable.trinket_2_stronger&(trinket.2.cooldown.remains&(trinket.2.cooldown.remains-5<variable.sync_remains&variable.sync_remains>=20|trinket.2.cooldown.remains-5>=variable.sync_remains&(variable.sync_remains>trinket.1.cooldown.duration%2|trinket.1.cooldown.duration<fight_remains&(variable.sync_remains+trinket.1.cooldown.duration>fight_remains)))|trinket.2.cooldown.ready&variable.sync_remains>20&variable.sync_remains<trinket.2.cooldown.duration%2)))|!trinket.1.has_use_buff&((!trinket.2.has_use_buff&(variable.trinket_1_stronger|trinket.2.cooldown.remains)|trinket.2.has_use_buff&(variable.sync_remains>20|trinket.2.cooldown.remains>20)))|target.time_to_die<25&(variable.trinket_1_stronger|trinket.2.cooldown.remains))&pet.main.buff.frenzy.remains>trinket.1.cast_time" );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=(trinket.2.has_use_buff&(variable.sync_up&(variable.trinket_2_stronger|trinket.1.cooldown.remains)|!variable.sync_up&(variable.trinket_2_stronger&(variable.sync_remains>trinket.2.cooldown.duration%2|trinket.1.has_use_buff&trinket.1.cooldown.remains>variable.sync_remains-15&trinket.1.cooldown.remains-5<variable.sync_remains&variable.sync_remains+40>fight_remains)|variable.trinket_1_stronger&(trinket.1.cooldown.remains&(trinket.1.cooldown.remains-5<variable.sync_remains&variable.sync_remains>=20|trinket.1.cooldown.remains-5>=variable.sync_remains&(variable.sync_remains>trinket.2.cooldown.duration%2|trinket.2.cooldown.duration<fight_remains&(variable.sync_remains+trinket.2.cooldown.duration>fight_remains)))|trinket.1.cooldown.ready&variable.sync_remains>20&variable.sync_remains<trinket.1.cooldown.duration%2)))|!trinket.2.has_use_buff&((!trinket.1.has_use_buff&(variable.trinket_2_stronger|trinket.1.cooldown.remains)|trinket.1.has_use_buff&(variable.sync_remains>20|trinket.1.cooldown.remains>20)))|target.time_to_die<25&(variable.trinket_2_stronger|trinket.1.cooldown.remains))&pet.main.buff.frenzy.remains>trinket.2.cast_time" );
}
//beast_mastery_apl_end

//marksmanship_apl_start
void marksmanship( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* st = p->get_action_priority_list( "st" );
  action_priority_list_t* trickshots = p->get_action_priority_list( "trickshots" );
  action_priority_list_t* trinkets = p->get_action_priority_list( "trinkets" );

  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "summon_pet,if=!talent.lone_wolf" );
  precombat->add_action( "snapshot_stats" );
  precombat->add_action( "salvo,precast_time=10" );
  precombat->add_action( "use_item,name=algethar_puzzle_box" );
  precombat->add_action( "aimed_shot,if=active_enemies<3&(!talent.volley|active_enemies<2)", "Precast Aimed Shot on one or two targets unless we could cleave it with Volley on two targets." );
  precombat->add_action( "wailing_arrow,if=active_enemies>2|!talent.steady_focus" );
  precombat->add_action( "steady_shot,if=active_enemies>2|talent.volley&active_enemies=2", "Precast Steady Shot on two targets if we are saving Aimed Shot to cleave with Volley, otherwise on three or more targets." );

  default_->add_action( "auto_shot" );
  default_->add_action( "variable,name=trueshot_ready,value=cooldown.trueshot.ready&buff.trueshot.down&(!raid_event.adds.exists&(!talent.bullseye|fight_remains>cooldown.trueshot.duration_guess+buff.trueshot.duration%2|buff.bullseye.stack=buff.bullseye.max_stack)&(!trinket.1.has_use_buff|trinket.1.cooldown.remains>30|trinket.1.cooldown.ready)&(!trinket.2.has_use_buff|trinket.2.cooldown.remains>30|trinket.2.cooldown.ready)|raid_event.adds.exists&(!raid_event.adds.up&(raid_event.adds.duration+raid_event.adds.in<25|raid_event.adds.in>60)|raid_event.adds.up&raid_event.adds.remains>10)|active_enemies>1|fight_remains<25)", "Determine if it is a good time to use Trueshot. Raid event optimization takes priority so usage is saved for multiple targets as long as it won't delay over half its duration. Otherwise allow for small delays to line up buff effect trinkets, and when using Bullseye, delay the last usage of the fight for max stacks." );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=trinkets" );
  default_->add_action( "call_action_list,name=st,if=active_enemies<3|!talent.trick_shots" );
  default_->add_action( "call_action_list,name=trickshots,if=active_enemies>2" );

  cds->add_action( "invoke_external_buff,name=power_infusion,if=buff.trueshot.remains>12", "Call for Power Infusion when Trueshot is up." );
  cds->add_action( "berserking,if=buff.trueshot.up|fight_remains<13" );
  cds->add_action( "blood_fury,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<16" );
  cds->add_action( "ancestral_call,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<16" );
  cds->add_action( "fireblood,if=buff.trueshot.up|cooldown.trueshot.remains>30|fight_remains<9" );
  cds->add_action( "lights_judgment,if=buff.trueshot.down" );
  cds->add_action( "potion,if=buff.trueshot.up&(buff.bloodlust.up|target.health.pct<20)|fight_remains<26" );
  cds->add_action( "salvo,if=active_enemies>2|cooldown.volley.remains<10" );

  st->add_action( "steady_shot,if=talent.steady_focus&(steady_focus_count&buff.steady_focus.remains<5|buff.steady_focus.down&!buff.trueshot.up)" );
  st->add_action( "aimed_shot,if=buff.trueshot.up&full_recharge_time<gcd+cast_time&talent.legacy_of_the_windrunners&talent.windrunners_guidance" );
  st->add_action( "kill_shot,if=buff.trueshot.down" );
  st->add_action( "volley,if=buff.salvo.up" );
  st->add_action( "steel_trap,if=buff.trueshot.down" );
  st->add_action( "serpent_sting,target_if=min:dot.serpent_sting.remains,if=refreshable&!talent.serpentstalkers_trickery&buff.trueshot.down" );
  st->add_action( "explosive_shot" );
  st->add_action( "stampede" );
  st->add_action( "death_chakram" );
  st->add_action( "wailing_arrow,if=active_enemies>1" );
  st->add_action( "rapid_fire,if=talent.surging_shots|action.aimed_shot.full_recharge_time>action.aimed_shot.cast_time+cast_time" );
  st->add_action( "kill_shot" );
  st->add_action( "trueshot,if=variable.trueshot_ready" );
  st->add_action( "multishot,if=buff.salvo.up&!talent.volley", "Trigger Salvo if Volley isn't being used to trigger it." );
  st->add_action( "aimed_shot,target_if=min:dot.serpent_sting.remains+action.serpent_sting.in_flight_to_target*99,if=talent.serpentstalkers_trickery&(buff.precise_shots.down|(buff.trueshot.up|full_recharge_time<gcd+cast_time)&(!talent.chimaera_shot|active_enemies<2|ca_active)|buff.trick_shots.remains>execute_time&active_enemies>1)", "With Serpentstalker's Trickery target the lowest remaining Serpent Sting. Without Chimaera Shot don't overwrite Precise Shots unless either Trueshot is active or Aimed Shot would cap before its next cast. On two targets with Chimaera Shot don't overwrite Precise Shots unless the target is within Careful Aim range in addition to either Trueshot being active or Aimed Shot capping before its next cast. Overwrite freely if it can cleave." );
  st->add_action( "aimed_shot,target_if=max:debuff.latent_poison.stack,if=buff.precise_shots.down|(buff.trueshot.up|full_recharge_time<gcd+cast_time)&(!talent.chimaera_shot|active_enemies<2|ca_active)|buff.trick_shots.remains>execute_time&active_enemies>1", "Without Serpentstalker's Trickery, target the highest Latent Poison stack. Same rules as the previous line." );
  st->add_action( "volley" );
  st->add_action( "rapid_fire" );
  st->add_action( "wailing_arrow,if=buff.trueshot.down" );
  st->add_action( "kill_command,if=buff.trueshot.down" );
  st->add_action( "steel_trap" );
  st->add_action( "chimaera_shot,if=buff.precise_shots.up|focus>cost+action.aimed_shot.cost" );
  st->add_action( "arcane_shot,if=buff.precise_shots.up|focus>cost+action.aimed_shot.cost" );
  st->add_action( "bag_of_tricks,if=buff.trueshot.down" );
  st->add_action( "steady_shot" );

  trickshots->add_action( "steady_shot,if=talent.steady_focus&steady_focus_count&buff.steady_focus.remains<8" );
  trickshots->add_action( "kill_shot,if=buff.razor_fragments.up" );
  trickshots->add_action( "explosive_shot" );
  trickshots->add_action( "death_chakram" );
  trickshots->add_action( "stampede" );
  trickshots->add_action( "wailing_arrow" );
  trickshots->add_action( "serpent_sting,target_if=min:dot.serpent_sting.remains,if=refreshable&talent.hydras_bite&!talent.serpentstalkers_trickery" );
  trickshots->add_action( "barrage,if=active_enemies>7" );
  trickshots->add_action( "volley" );
  trickshots->add_action( "trueshot,if=buff.trueshot.down" );
  trickshots->add_action( "rapid_fire,if=buff.trick_shots.remains>=execute_time&talent.surging_shots" );
  trickshots->add_action( "aimed_shot,target_if=min:dot.serpent_sting.remains+action.serpent_sting.in_flight_to_target*99,if=talent.serpentstalkers_trickery&(buff.trick_shots.remains>=execute_time&(buff.precise_shots.down|buff.trueshot.up|full_recharge_time<cast_time+gcd))", "For Serpentstalker's Trickery, target the lowest remaining Serpent Sting. Generally only cast if it would cleave with Trick Shots. Don't overwrite Precise Shots unless Trueshot is up or Aimed Shot would cap otherwise." );
  trickshots->add_action( "aimed_shot,target_if=max:debuff.latent_poison.stack,if=(buff.trick_shots.remains>=execute_time&(buff.precise_shots.down|buff.trueshot.up|full_recharge_time<cast_time+gcd))", "For no Serpentstalker's Trickery, target the highest Latent Poison stack. Same general rules as the previous line." );
  trickshots->add_action( "rapid_fire,if=buff.trick_shots.remains>=execute_time" );
  trickshots->add_action( "chimaera_shot,if=buff.trick_shots.up&buff.precise_shots.up&focus>cost+action.aimed_shot.cost&active_enemies<4" );
  trickshots->add_action( "multishot,if=buff.trick_shots.down|(buff.precise_shots.up|buff.bulletstorm.stack=10)&focus>cost+action.aimed_shot.cost" );
  trickshots->add_action( "serpent_sting,target_if=min:dot.serpent_sting.remains,if=refreshable&talent.poison_injection&!talent.serpentstalkers_trickery", "Only use baseline Serpent Sting as a filler in cleave if it's the only source of applying Latent Poison." );
  trickshots->add_action( "steel_trap,if=buff.trueshot.down" );
  trickshots->add_action( "kill_shot,if=focus>cost+action.aimed_shot.cost" );
  trickshots->add_action( "multishot,if=focus>cost+action.aimed_shot.cost" );
  trickshots->add_action( "bag_of_tricks,if=buff.trueshot.down" );
  trickshots->add_action( "steady_shot" );

  trinkets->add_action( "variable,name=sync_ready,value=variable.trueshot_ready", "Signals that cooldowns are active or ready to activate that is desirable to sync a buff effect with." );
  trinkets->add_action( "variable,name=sync_active,value=buff.trueshot.up", "Signals that the cooldowns that are desirable to sync a buff with are active." );
  trinkets->add_action( "variable,name=sync_remains,value=cooldown.trueshot.remains", "The amount of time until the cooldowns will be ready that are desirable to sync a buff effect with." );
  trinkets->add_action( "variable,name=trinket_1_stronger,value=!trinket.2.has_cooldown|trinket.1.has_use_buff&(!trinket.2.has_use_buff|trinket.2.cooldown.duration<trinket.1.cooldown.duration|trinket.2.cast_time<trinket.1.cast_time|trinket.2.cast_time=trinket.1.cast_time&trinket.2.cooldown.duration=trinket.1.cooldown.duration)|!trinket.1.has_use_buff&(!trinket.2.has_use_buff&(trinket.2.cooldown.duration<trinket.1.cooldown.duration|trinket.2.cast_time<trinket.1.cast_time|trinket.2.cast_time=trinket.1.cast_time&trinket.2.cooldown.duration=trinket.1.cooldown.duration))", "Determine the stronger trinket to sync with cooldowns. In descending priority: buff effects > damage effects, longer > shorter cooldowns, longer > shorter cast times." );
  trinkets->add_action( "variable,name=trinket_2_stronger,value=!trinket.1.has_cooldown|trinket.2.has_use_buff&(!trinket.1.has_use_buff|trinket.1.cooldown.duration<trinket.2.cooldown.duration|trinket.1.cast_time<trinket.2.cast_time|trinket.1.cast_time=trinket.2.cast_time&trinket.1.cooldown.duration=trinket.2.cooldown.duration)|!trinket.2.has_use_buff&(!trinket.1.has_use_buff&(trinket.1.cooldown.duration<trinket.2.cooldown.duration|trinket.1.cast_time<trinket.2.cast_time|trinket.1.cast_time=trinket.2.cast_time&trinket.1.cooldown.duration=trinket.2.cooldown.duration))" );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=trinket1,if=trinket.1.has_use_buff&(variable.sync_ready&(variable.trinket_1_stronger|trinket.2.cooldown.remains)|!variable.sync_ready&(variable.trinket_1_stronger&(variable.sync_remains>trinket.1.cooldown.duration%2|trinket.2.has_use_buff&trinket.2.cooldown.remains>variable.sync_remains-15&trinket.2.cooldown.remains-5<variable.sync_remains&variable.sync_remains+40>fight_remains)|variable.trinket_2_stronger&(trinket.2.cooldown.remains&(trinket.2.cooldown.remains-5<variable.sync_remains&variable.sync_remains>=20|trinket.2.cooldown.remains-5>=variable.sync_remains&(variable.sync_remains>trinket.1.cooldown.duration%2|trinket.1.cooldown.duration<fight_remains&(variable.sync_remains+trinket.1.cooldown.duration>fight_remains)))|trinket.2.cooldown.ready&variable.sync_remains>20&variable.sync_remains<trinket.2.cooldown.duration%2)))|!trinket.1.has_use_buff&(trinket.1.cast_time=0|!variable.sync_active)&((!trinket.2.has_use_buff&(variable.trinket_1_stronger|trinket.2.cooldown.remains)|trinket.2.has_use_buff&(variable.sync_remains>20|trinket.2.cooldown.remains>20)))|target.time_to_die<25&(variable.trinket_1_stronger|trinket.2.cooldown.remains)", "Uses buff effect trinkets with cooldowns and is willing to delay usage up to half the trinket cooldown if it won't lose a usage in the fight. Fills in downtime with weaker buff effects if they won't also be saved for later cooldowns (happens if it won't delay over half the trinket cooldown and a stronger trinket won't be up in time) or damage effects if they won't inferfere with any buff effect usage. Intended to be slot-agnostic so that any order of the same trinket pair should result in the same usage." );
  trinkets->add_action( "use_item,use_off_gcd=1,slot=trinket2,if=trinket.2.has_use_buff&(variable.sync_ready&(variable.trinket_2_stronger|trinket.1.cooldown.remains)|!variable.sync_ready&(variable.trinket_2_stronger&(variable.sync_remains>trinket.2.cooldown.duration%2|trinket.1.has_use_buff&trinket.1.cooldown.remains>variable.sync_remains-15&trinket.1.cooldown.remains-5<variable.sync_remains&variable.sync_remains+40>fight_remains)|variable.trinket_1_stronger&(trinket.1.cooldown.remains&(trinket.1.cooldown.remains-5<variable.sync_remains&variable.sync_remains>=20|trinket.1.cooldown.remains-5>=variable.sync_remains&(variable.sync_remains>trinket.2.cooldown.duration%2|trinket.2.cooldown.duration<fight_remains&(variable.sync_remains+trinket.2.cooldown.duration>fight_remains)))|trinket.1.cooldown.ready&variable.sync_remains>20&variable.sync_remains<trinket.1.cooldown.duration%2)))|!trinket.2.has_use_buff&(trinket.2.cast_time=0|!variable.sync_active)&((!trinket.1.has_use_buff&(variable.trinket_2_stronger|trinket.1.cooldown.remains)|trinket.1.has_use_buff&(variable.sync_remains>20|trinket.1.cooldown.remains>20)))|target.time_to_die<25&(variable.trinket_2_stronger|trinket.1.cooldown.remains)" );
}
//marksmanship_apl_end

//survival_apl_start
void survival( player_t* p )
{
  action_priority_list_t* default_ = p->get_action_priority_list( "default" );
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* cds = p->get_action_priority_list( "cds" );
  action_priority_list_t* cleave = p->get_action_priority_list( "cleave" );
  action_priority_list_t* st = p->get_action_priority_list( "st" );

  precombat->add_action( "flask" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "food" );
  precombat->add_action( "variable,name=mb_rs_cost,op=setif,value=action.mongoose_bite.cost,value_else=action.raptor_strike.cost,condition=talent.mongoose_bite" );
  precombat->add_action( "summon_pet" );
  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat->add_action( "use_item,name=algethar_puzzle_box" );
  precombat->add_action( "steel_trap,precast_time=2" );

  default_->add_action( "auto_attack" );
  default_->add_action( "call_action_list,name=cds" );
  default_->add_action( "call_action_list,name=st,if=active_enemies<3" );
  default_->add_action( "call_action_list,name=cleave,if=active_enemies>2" );
  default_->add_action( "arcane_torrent" );

  cds->add_action( "blood_fury,if=buff.coordinated_assault.up|buff.spearhead.up|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "harpoon,if=talent.terms_of_engagement.enabled&focus<focus.max" );
  cds->add_action( "ancestral_call,if=buff.coordinated_assault.up|buff.spearhead.up|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "fireblood,if=buff.coordinated_assault.up|buff.spearhead.up|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "lights_judgment" );
  cds->add_action( "bag_of_tricks,if=cooldown.kill_command.full_recharge_time>gcd" );
  cds->add_action( "berserking,if=buff.coordinated_assault.up|buff.spearhead.up|!talent.spearhead&!talent.coordinated_assault|time_to_die<13" );
  cds->add_action( "muzzle" );
  cds->add_action( "potion,if=target.time_to_die<25|buff.coordinated_assault.up|buff.spearhead.up|!talent.spearhead&!talent.coordinated_assault" );
  cds->add_action( "use_item,name=algethar_puzzle_box,use_off_gcd=1,if=gcd.remains>gcd.max-0.1" );
  cds->add_action( "use_item,name=manic_grieftorch,use_off_gcd=1,if=gcd.remains>gcd.max-0.1&!buff.spearhead.up" );
  cds->add_action( "use_items,use_off_gcd=1,if=gcd.remains>gcd.max-0.1&!buff.spearhead.up" );
  cds->add_action( "aspect_of_the_eagle,if=target.distance>=6" );

  cleave->add_action( "kill_shot,if=buff.coordinated_assault_empower.up&talent.birds_of_prey" );
  cleave->add_action( "death_chakram" );
  cleave->add_action( "wildfire_bomb" );
  cleave->add_action( "stampede" );
  cleave->add_action( "coordinated_assault,if=cooldown.fury_of_the_eagle.remains|!talent.fury_of_the_eagle" );
  cleave->add_action( "explosive_shot" );
  cleave->add_action( "carve,if=cooldown.wildfire_bomb.full_recharge_time>spell_targets%2" );
  cleave->add_action( "use_item,name=djaruun_pillar_of_the_elder_flame" );
  cleave->add_action( "fury_of_the_eagle,if=raid_event.adds.exists&(!talent.butchery|cooldown.butchery.full_recharge_time>cast_time)" );
  cleave->add_action( "butchery,if=raid_event.adds.exists" );
  cleave->add_action( "butchery,if=(full_recharge_time<gcd|dot.shrapnel_bomb.ticking&(dot.internal_bleeding.stack<2|dot.shrapnel_bomb.remains<gcd|raid_event.adds.remains<10))&!raid_event.adds.exists" );
  cleave->add_action( "fury_of_the_eagle,if=!raid_event.adds.exists" );
  cleave->add_action( "carve,if=dot.shrapnel_bomb.ticking" );
  cleave->add_action( "butchery,if=(!next_wi_bomb.shrapnel|!talent.wildfire_infusion)" );
  cleave->add_action( "mongoose_bite,target_if=max:debuff.latent_poison.stack,if=debuff.latent_poison.stack>8" );
  cleave->add_action( "raptor_strike,target_if=max:debuff.latent_poison.stack,if=debuff.latent_poison.stack>8" );
  cleave->add_action( "kill_command,target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max&full_recharge_time<gcd" );
  cleave->add_action( "flanking_strike,if=focus+cast_regen<focus.max" );
  cleave->add_action( "carve" );
  cleave->add_action( "kill_shot,if=!buff.coordinated_assault.up" );
  cleave->add_action( "steel_trap,if=focus+cast_regen<focus.max" );
  cleave->add_action( "spearhead" );
  cleave->add_action( "mongoose_bite,target_if=min:dot.serpent_sting.remains,if=buff.spearhead.remains" );
  cleave->add_action( "serpent_sting,target_if=min:remains,if=refreshable&target.time_to_die>12&(!talent.vipers_venom|talent.hydras_bite)" );
  cleave->add_action( "mongoose_bite,target_if=min:dot.serpent_sting.remains" );
  cleave->add_action( "raptor_strike,target_if=min:dot.serpent_sting.remains" );

  st->add_action( "kill_command,target_if=min:bloodseeker.remains,if=talent.spearhead&debuff.shredded_armor.stack<1&cooldown.spearhead.remains<2*gcd" );
  st->add_action( "wildfire_bomb,if=talent.spearhead&cooldown.spearhead.remains<2*gcd&debuff.shredded_armor.stack>0" );
  st->add_action( "death_chakram,if=focus+cast_regen<focus.max|talent.spearhead&!cooldown.spearhead.remains" );
  st->add_action( "use_item,name=djaruun_pillar_of_the_elder_flame,if=!talent.fury_of_the_eagle|talent.spearhead" );
  st->add_action( "spearhead,if=focus+action.kill_command.cast_regen>focus.max-10&(cooldown.death_chakram.remains|!talent.death_chakram)" );
  st->add_action( "kill_shot,if=buff.coordinated_assault_empower.up" );
  st->add_action( "wildfire_bomb,if=(raid_event.adds.in>cooldown.wildfire_bomb.full_recharge_time-(cooldown.wildfire_bomb.full_recharge_time%3.5)&debuff.shredded_armor.stack>0&(full_recharge_time<2*gcd|talent.bombardier&!cooldown.coordinated_assault.remains|talent.bombardier&buff.coordinated_assault.up&buff.coordinated_assault.remains<2*gcd)|!raid_event.adds.exists&time_to_die<7)&set_bonus.tier30_4pc" );
  st->add_action( "kill_command,target_if=min:bloodseeker.remains,if=full_recharge_time<gcd&focus+cast_regen<focus.max&(buff.deadly_duo.stack>2|buff.spearhead.remains&dot.pheromone_bomb.remains)" );
  st->add_action( "kill_command,target_if=min:bloodseeker.remains,if=cooldown.wildfire_bomb.full_recharge_time<3*gcd&debuff.shredded_armor.stack<1&set_bonus.tier30_4pc&!buff.spearhead.remains" );
  st->add_action( "mongoose_bite,if=buff.spearhead.remains" );
  st->add_action( "mongoose_bite,if=active_enemies=1&target.time_to_die<focus%(variable.mb_rs_cost-cast_regen)*gcd|buff.mongoose_fury.up&buff.mongoose_fury.remains<gcd" );
  st->add_action( "kill_shot,if=!buff.coordinated_assault.up" );
  st->add_action( "raptor_strike,if=active_enemies=1&target.time_to_die<focus%(variable.mb_rs_cost-cast_regen)*gcd" );
  st->add_action( "serpent_sting,target_if=min:remains,if=!dot.serpent_sting.ticking&target.time_to_die>7&!talent.vipers_venom" );
  st->add_action( "fury_of_the_eagle,if=buff.seething_rage.up&buff.seething_rage.remains<3*gcd&(!raid_event.adds.exists|active_enemies>1|raid_event.adds.exists&raid_event.adds.in>40)" );
  st->add_action( "use_item,name=djaruun_pillar_of_the_elder_flame,if=talent.coordinated_assault|talent.fury_of_the_eagle&cooldown.fury_of_the_eagle.remains<5" );
  st->add_action( "mongoose_bite,if=talent.alpha_predator&buff.mongoose_fury.up&buff.mongoose_fury.remains<focus%(variable.mb_rs_cost-cast_regen)*gcd|buff.seething_rage.remains&active_enemies=1" );
  st->add_action( "flanking_strike,if=focus+cast_regen<focus.max" );
  st->add_action( "stampede" );
  st->add_action( "coordinated_assault,if=(!talent.coordinated_kill&target.health.pct<20&(!buff.spearhead.remains&cooldown.spearhead.remains|!talent.spearhead)|talent.coordinated_kill&(!buff.spearhead.remains&cooldown.spearhead.remains|!talent.spearhead))&(!raid_event.adds.exists|raid_event.adds.in>90)" );
  st->add_action( "kill_command,target_if=min:bloodseeker.remains,if=full_recharge_time<gcd&focus+cast_regen<focus.max&(cooldown.flanking_strike.remains|!talent.flanking_strike)" );
  st->add_action( "serpent_sting,target_if=min:remains,if=refreshable&!talent.vipers_venom" );
  st->add_action( "wildfire_bomb,if=raid_event.adds.in>cooldown.wildfire_bomb.full_recharge_time-(cooldown.wildfire_bomb.full_recharge_time%3.5)&full_recharge_time<2*gcd" );
  st->add_action( "mongoose_bite,if=dot.shrapnel_bomb.ticking" );
  st->add_action( "wildfire_bomb,if=raid_event.adds.in>cooldown.wildfire_bomb.full_recharge_time-(cooldown.wildfire_bomb.full_recharge_time%3.5)&set_bonus.tier30_4pc&(!dot.wildfire_bomb.ticking&debuff.shredded_armor.stack>0&focus+cast_regen<focus.max|active_enemies>1)" );
  st->add_action( "mongoose_bite,target_if=max:debuff.latent_poison.stack,if=buff.mongoose_fury.up" );
  st->add_action( "explosive_shot,if=talent.ranger&(!raid_event.adds.exists|raid_event.adds.in>28)" );
  st->add_action( "fury_of_the_eagle,if=(!equipped.djaruun_pillar_of_the_elder_flame|cooldown.elder_flame_408821.remains>40)&target.health.pct<65&talent.ruthless_marauder&(!raid_event.adds.exists|raid_event.adds.exists&raid_event.adds.in>40)" );
  st->add_action( "mongoose_bite,target_if=max:debuff.latent_poison.stack,if=focus+action.kill_command.cast_regen>focus.max-10|set_bonus.tier30_4pc" );
  st->add_action( "raptor_strike,target_if=max:debuff.latent_poison.stack" );
  st->add_action( "steel_trap" );
  st->add_action( "wildfire_bomb,if=raid_event.adds.in>cooldown.wildfire_bomb.full_recharge_time-(cooldown.wildfire_bomb.full_recharge_time%3.5)&!dot.wildfire_bomb.ticking" );
  st->add_action( "kill_command,target_if=min:bloodseeker.remains,if=focus+cast_regen<focus.max" );
  st->add_action( "coordinated_assault,if=!talent.coordinated_kill&time_to_die>140" );
}
//survival_apl_end

} // namespace hunter_apl
