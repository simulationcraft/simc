action_priority_list_t* precombat = get_action_priority_list( "precombat" );
action_priority_list_t* def = get_action_priority_list( "default" );
action_priority_list_t* aoe_builder = get_action_priority_list( "aoe_builder" );
action_priority_list_t* aoe_finisher = get_action_priority_list( "aoe_finisher" );
action_priority_list_t* builder = get_action_priority_list( "builder" );
action_priority_list_t* cooldown = get_action_priority_list( "cooldown" );
action_priority_list_t* cd_variable = get_action_priority_list( "cd_variable" );
action_priority_list_t* finisher = get_action_priority_list( "finisher" );

precombat->add_action( "variable,name=regrowth,op=reset","optional variable that sends regrowth casts. Turned off by default" );
precombat->add_action( "variable,name=dotc_rake_threshold,op=set,value=5" );
precombat->add_action( "variable,name=dotc_rake_threshold,op=set,if=talent.wild_slashes&!talent.infected_wounds,value=3" );
precombat->add_action( "variable,name=dotc_rake_threshold,op=set,if=!talent.wild_slashes&talent.infected_wounds,value=8" );
precombat->add_action( "variable,name=algethar_puzzle_box_precombat_cast,value=3,if=equipped.algethar_puzzle_box" );
precombat->add_action( "use_item,name=algethar_puzzle_box" );
precombat->add_action( "cat_form,if=buff.cat_form.down" );
precombat->add_action( "prowl" );

def->add_action( "prowl,if=buff.bs_inc.down&!buff.prowl.up&!buff.shadowmeld.up" );
def->add_action( "cat_form,if=!buff.cat_form.up&!talent.fluid_form" );
def->add_action( "invoke_external_buff,name=power_infusion,if=buff.bs_inc.up|!talent.berserk_heart_of_the_lion","Line up <a href='https://www.wowhead.com/spell=10060/power-infusion'>Power Infusion</a> with <a href='https://www.wowhead.com/spell=106951/berserk'>Berserk</a>." );
def->add_action( "auto_attack,if=!buff.prowl.up&!buff.shadowmeld.up" );
def->add_action( "tigers_fury","tigers fury on cooldown" );
def->add_action( "rake,if=buff.prowl.up|buff.shadowmeld.up","rake out of stealth" );
def->add_action( "chomp,if=buff.chomp_enabler.up","chomp if its possible" );
def->add_action( "ferocious_bite,if=buff.apex_predators_craving.up","cast bite with apex procs asap" );
def->add_action( "call_action_list,name=cooldown" );
def->add_action( "call_action_list,name=finisher,if=combo_points>=4&spell_targets=1" );
def->add_action( "call_action_list,name=aoe_finisher,if=combo_points>=4&spell_targets>=2" );
def->add_action( "call_action_list,name=builder,if=spell_targets=1&combo_points<4" );
def->add_action( "call_action_list,name=aoe_builder,if=spell_targets>1&combo_points<4" );
def->add_action( "regrowth,if=buff.predatory_swiftness.up&variable.regrowth" );

aoe_builder->add_action( "rake,target_if=refreshable,if=talent.doubleclawed_rake|hero_tree.wildstalker&active_dot.rake=0","ensure at last 1 rake at all times if wildstalker, rake highest prio with dcr" );
aoe_builder->add_action( "swipe_cat,if=hero_tree.druid_of_the_claw&buff.bs_inc.up|buff.clearcasting.up","with dotc, nodcr/nowildslashes, swipe spam in berserk at 2t" );
aoe_builder->add_action( "rake,target_if=refreshable,if=hero_tree.wildstalker|spell_targets.swipe_cat<=variable.dotc_rake_threshold","no dcr, spread rakes up to 3t(ws+noiw)->5t(nows+noiw/wild slashes+iw)->8t(no ws+iw)" );
aoe_builder->add_action( "swipe_cat" );

aoe_finisher->add_action( "primal_wrath,target_if=min:dot.primal_wrath.ticking,if=spell_targets.primal_wrath>1&(dot.primal_wrath.remains<6.5&!buff.bs_inc.up|dot.primal_wrath.refreshable)","primal wrath in pandemic during berserk, else with sub 6.5s remaining" );
aoe_finisher->add_action( "ferocious_bite,if=talent.rampant_ferocity|buff.ravage.up&spell_targets<8|dot.bloodseeker_vines.ticking&spell_targets<5","we can send bite with rampant or under 5t with vines, 8t with ravage" );
aoe_finisher->add_action( "primal_wrath","fallback primal wrath" );

builder->add_action( "prowl,if=!buff.shadowmeld.up&(action.rake.pmultiplier<1.6|dot.rake.refreshable)" );
builder->add_action( "shadowmeld,if=!buff.prowl.up&(action.rake.pmultiplier<1.6|dot.rake.refreshable)" );
builder->add_action( "rake,if=(buff.tigers_fury.up|remains<cooldown.tigers_fury.remains)&(refreshable&persistent_multiplier>=pmultiplier|remains<2|persistent_multiplier>pmultiplier)","freely upgrade rakes" );
builder->add_action( "moonfire_cat,if=(buff.tigers_fury.up|remains<cooldown.tigers_fury.remains)&(refreshable&persistent_multiplier>=pmultiplier|remains<2|persistent_multiplier>pmultiplier)","freely upgrade li" );
builder->add_action( "shred" );

cooldown->add_action( "call_action_list,name=cd_variable,if=!cooldown.bs_inc.remains|!cooldown.convoke_the_spirits.remains|!cooldown.potion.remains|!trinket.1.cooldown.remains|!trinket.2.cooldown.remains","this line is here to reduce the amount of variable calls in the APL" );
cooldown->add_action( "use_item,slot=trinket1,if=trinket.1.has_use_damage&(trinket.2.cooldown.remains>20|!trinket.2.has_use_buff|cooldown.tigers_fury.remains<25&cooldown.tigers_fury.remains>20)","non-stat on use trinkets get used on cooldown, so long as it wont interfere with a stat on-use trinket" );
cooldown->add_action( "use_item,slot=trinket2,if=trinket.2.has_use_damage&(trinket.1.cooldown.remains>20|!trinket.1.has_use_buff|cooldown.tigers_fury.remains<25&cooldown.tigers_fury.remains>20)" );
cooldown->add_action( "berserking,if=buff.bs_inc.up" );
cooldown->add_action( "potion,if=buff.bs_inc.up|fight_remains<32|buff.tigers_fury.up&!variable.holdPot","potion during berserk, fallback pot if the fight is going to end within its duration. Expedite use if you would lose a pot use by holding it." );
cooldown->add_action( "use_items","non trinket gear-on-uses have variable rules on whether or not they trigger the trinket shared CD. For the cases they do we will need specific APL entries. For now just use on cooldown." );
cooldown->add_action( "use_item,slot=trinket1,use_off_gcd=1,if=(time>10|buff.bs_inc.up)&trinket.1.has_use_buff&(cooldown.tigers_fury.remains>=25|trinket.1.is.algethar_puzzle_box&cooldown.tigers_fury.remains<2)&(buff.potion.up|variable.slot1CountRemaining!=variable.potCountRemaining)&((cooldown.bs_inc.remains<5|buff.bs_inc.remains>=14)&!variable.holdBerserk|cooldown.convoke_the_spirits.remains<10&!variable.holdConvoke&talent.convoke_the_spirits|variable.lowestCDremaining>trinket.1.cooldown.duration|variable.zerkCountRemaining=1&(!talent.convoke_the_spirits|variable.convokeCountRemaining=1)&variable.potCountRemaining=1&(variable.highestCDremaining+3)>trinket.1.cooldown.duration|variable.zerkCountRemaining=variable.convokeCountRemaining&talent.convoke_the_spirits&variable.zerkCountRemaining!=variable.potCountRemaining&(cooldown.bs_inc.remains<?cooldown.convoke_the_spirits.remains)>trinket.1.cooldown.duration|variable.slot1CountRemaining=variable.potCountRemaining-1&buff.potion.up|trinket.2.has_use_buff&(variable.secondLowestCDremaining>trinket.1.cooldown.duration&variable.lowestCDremaining>trinket.2.cooldown.remains|variable.zerkCountRemaining=1&(!talent.convoke_the_spirits|variable.convokeCountRemaining=1)&variable.potCountRemaining=1&variable.highestCDremaining>trinket.2.cooldown.remains|variable.zerkCountRemaining=variable.convokeCountRemaining&talent.convoke_the_spirits&variable.zerkCountRemaining!=variable.potCountRemaining&(cooldown.convoke_the_spirits.remains<?cooldown.bs_inc.remains)>trinket.2.cooldown.remains|variable.convokeCountRemaining=0&variable.zerkCountRemaining=0&variable.potCountRemaining=0))","stat on-use trinkets, essentially this compares the number of trinket uses to the number of other cooldown uses remaining in the fight to determine whether or not to send the trinket." );
cooldown->add_action( "use_item,slot=trinket2,use_off_gcd=1,if=(time>10|buff.bs_inc.up)&trinket.2.has_use_buff&(!trinket.1.has_use_buff|trinket.1.cooldown.remains>20)&(cooldown.tigers_fury.remains>=25|trinket.2.is.algethar_puzzle_box&cooldown.tigers_fury.remains<2)&(buff.potion.up|variable.slot2CountRemaining!=variable.potCountRemaining)&((cooldown.bs_inc.remains<5|buff.bs_inc.remains>=14)&!variable.holdBerserk|cooldown.convoke_the_spirits.remains<10&!variable.holdConvoke&talent.convoke_the_spirits|variable.lowestCDremaining>trinket.2.cooldown.duration|variable.zerkCountRemaining=1&(!talent.convoke_the_spirits|variable.convokeCountRemaining=1)&variable.potCountRemaining=1&(variable.highestCDremaining+3)>trinket.2.cooldown.duration|variable.zerkCountRemaining=variable.convokeCountRemaining&talent.convoke_the_spirits&variable.zerkCountRemaining!=variable.potCountRemaining&(cooldown.bs_inc.remains<?cooldown.convoke_the_spirits.remains)>trinket.2.cooldown.duration|variable.slot1CountRemaining=variable.potCountRemaining-1&buff.potion.up|trinket.1.has_use_buff&(variable.secondLowestCDremaining>trinket.2.cooldown.duration&variable.lowestCDremaining>trinket.1.cooldown.remains|variable.zerkCountRemaining=1&(!talent.convoke_the_spirits|variable.convokeCountRemaining=1)&variable.potCountRemaining=1&variable.highestCDremaining>trinket.1.cooldown.remains|variable.zerkCountRemaining=variable.convokeCountRemaining&talent.convoke_the_spirits&variable.zerkCountRemaining!=variable.potCountRemaining&(cooldown.convoke_the_spirits.remains<?cooldown.bs_inc.remains)>trinket.1.cooldown.remains|variable.convokeCountRemaining=0&variable.zerkCountRemaining=0&variable.potCountRemaining=0))" );
cooldown->add_action( "use_item,slot=trinket1,if=fight_remains<=(21<?trinket.1.proc.any_dps.duration)","fallback use if fight is ending soon" );
cooldown->add_action( "use_item,slot=trinket2,if=fight_remains<=(21<?trinket.2.proc.any_dps.duration)" );
cooldown->add_action( "incarnation,if=buff.tigers_fury.up&!variable.holdBerserk" );
cooldown->add_action( "berserk,if=buff.tigers_fury.up&!variable.holdBerserk" );
cooldown->add_action( "feral_frenzy,if=buff.tigers_fury.up&combo_points<=2+(2*buff.bs_inc.up)","0/1/2 combo points outside zerk, 0-4 combo points inside zerk" );
cooldown->add_action( "frantic_frenzy,if=combo_points<=2+(2*buff.bs_inc.up)" );
cooldown->add_action( "convoke_the_spirits,if=fight_remains<5|buff.bs_inc.up&buff.bs_inc.remains<5-talent.ashamanes_guidance|buff.tigers_fury.up&!variable.holdConvoke&(prev_gcd.1.rip|prev_gcd.1.ferocious_bite)","always line up convoke with berserk if you can" );

cd_variable->add_action( "variable,name=convokeCountRemaining,value=floor(((fight_remains-variable.convoke_cd)%cooldown.convoke_the_spirits.duration)+(fight_remains>cooldown.convoke_the_spirits.remains))","TLDR, maximize number of cooldown overlaps whilst minimizing number of casts lost." );
cd_variable->add_action( "variable,name=zerkCountRemaining,value=floor(((fight_remains-variable.bs_inc_cd)%cooldown.bs_inc.duration)+(fight_remains>cooldown.bs_inc.remains))" );
cd_variable->add_action( "variable,name=potCountRemaining,value=floor(((fight_remains-variable.pot_cd)%cooldown.potion.duration)+(fight_remains>cooldown.potion.remains))" );
cd_variable->add_action( "variable,name=slot1CountRemaining,value=floor(((fight_remains-trinket.1.cooldown.remains-10)%trinket.1.cooldown.duration)+(fight_remains>trinket.1.cooldown.remains))" );
cd_variable->add_action( "variable,name=slot2CountRemaining,value=floor(((fight_remains-trinket.2.cooldown.remains-10)%trinket.2.cooldown.duration)+(fight_remains>trinket.2.cooldown.remains))" );
cd_variable->add_action( "variable,name=firstHoldBerserkCondition,value=variable.zerkCountRemaining=1&(variable.convokeCountRemaining=1&cooldown.convoke_the_spirits.remains>10|variable.potCountRemaining=1&cooldown.potion.remains)","hold berserk if there is 1 convoke or 1 potion remaining in the fight but they aren't available yet" );
cd_variable->add_action( "variable,name=secondHoldBerserkCondition,value=cooldown.convoke_the_spirits.remains>20&variable.convokeCountRemaining=variable.zerkCountRemaining&variable.zerkCountRemaining=floor(((fight_remains-variable.convoke_cd)%cooldown.bs_inc.duration)+(fight_remains>cooldown.convoke_the_spirits.remains))","hold berserk if there are as many zerk casts as convoke casts remaining and holding zerk for convoke does not lose you a cast" );
cd_variable->add_action( "variable,name=holdBerserk,value=variable.firstHoldBerserkCondition|variable.secondHoldBerserkCondition" );
cd_variable->add_action( "variable,name=holdConvoke,value=variable.convokeCountRemaining=1&variable.zerkCountRemaining=1&!buff.bs_inc.up","hold the last convoke if there is another berserk cast remaining and Berserk isn't already up" );
cd_variable->add_action( "variable,name=holdPot,value=variable.potCountRemaining=floor(((fight_remains-variable.bs_inc_cd)%cooldown.potion.duration)+(fight_remains>cooldown.bs_inc.remains))","hold potion for berserk if doing so does not lose you a cast" );
cd_variable->add_action( "variable,name=bs_inc_cd,value=cooldown.bs_inc.remains+10" );
cd_variable->add_action( "variable,name=convoke_cd,value=cooldown.convoke_the_spirits.remains+10" );
cd_variable->add_action( "variable,name=pot_cd,value=cooldown.potion.remains+25" );
cd_variable->add_action( "variable,name=highestCDremaining,op=setif,condition=talent.convoke_the_spirits,value=cooldown.convoke_the_spirits.remains<?cooldown.bs_inc.remains<?cooldown.potion.remains,value_else=cooldown.bs_inc.remains<?cooldown.potion.remains" );
cd_variable->add_action( "variable,name=lowestCDremaining,op=setif,condition=talent.convoke_the_spirits,value=cooldown.convoke_the_spirits.remains>?cooldown.bs_inc.remains>?cooldown.potion.remains,value_else=cooldown.bs_inc.remains>?cooldown.potion.remains" );
cd_variable->add_action( "variable,name=secondLowestCDremaining,op=setif,condition=cooldown.convoke_the_spirits.remains>cooldown.bs_inc.remains,value=cooldown.convoke_the_spirits.remains>?cooldown.potion.remains,value_else=cooldown.bs_inc.remains>?cooldown.potion.remains" );

finisher->add_action( "rip,if=refreshable&(buff.tigers_fury.up|dot.rip.remains<cooldown.tigers_fury.remains)","maintain rip in single-target, try to have it buffed by tf if it lines up (0.1% gain)" );
finisher->add_action( "pool_resource,for_next=1","actions.finisher+=/wait,sec=buff.ravage.up|energy.deficit<30,if=!buff.ravage.up&energy.deficit>30" );
finisher->add_action( "ferocious_bite,if=!talent.saber_jaws|!buff.ravage.up","ferocious bite asap, dont pool" );
finisher->add_action( "pool_resource,for_next=1" );
finisher->add_action( "ferocious_bite,max_energy=1" );
