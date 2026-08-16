action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
action_priority_list_t* def = p->get_action_priority_list( "default" );
action_priority_list_t* aoe_builder = p->get_action_priority_list( "aoe_builder" );
action_priority_list_t* builder = p->get_action_priority_list( "builder" );
action_priority_list_t* aoe_finisher = p->get_action_priority_list( "aoe_finisher" );
action_priority_list_t* finisher = p->get_action_priority_list( "finisher" );
action_priority_list_t* cd_variable = p->get_action_priority_list( "cd_variable" );
action_priority_list_t* cooldown = p->get_action_priority_list( "cooldown" );

precombat->add_action( "variable,name=regrowth,op=reset","optional variable that sends regrowth casts. Turned off by default" );
precombat->add_action( "variable,name=dotc_rake_threshold,op=set,value=99","see the 2nd rake line in aoe_builders. This determines when that line can return true" );
precombat->add_action( "variable,name=dotc_rake_threshold,op=set,if=talent.wild_slashes&talent.merciless_claws,value=5" );
precombat->add_action( "variable,name=dotc_rake_threshold,op=set,if=talent.wild_slashes&!talent.merciless_claws,value=8" );
precombat->add_action( "variable,name=algethar_puzzle_box_precombat_cast,value=3,if=trinket.1.is.algethar_puzzle_box|!trinket.1.has_use_buff","for practicism, we give a 1s leeway (at 0% haste) from cast end until start of the fight." );
precombat->add_action( "use_item,name=algethar_puzzle_box,if=trinket.1.is.algethar_puzzle_box|!trinket.1.has_use_buff" );
precombat->add_action( "cat_form,if=buff.cat_form.down" );
precombat->add_action( "prowl" );

def->add_action( "prowl,if=buff.bs_inc.down&!buff.prowl.up&!buff.shadowmeld.up" );
def->add_action( "cat_form,if=!buff.cat_form.up&!talent.fluid_form" );
def->add_action( "invoke_external_buff,name=power_infusion,if=buff.bs_inc.up|!talent.berserk_heart_of_the_lion","Line up <a href='https://www.wowhead.com/spell=10060/power-infusion'>Power Infusion</a> with <a href='https://www.wowhead.com/spell=106951/berserk'>Berserk</a> every 2 minutes." );
def->add_action( "auto_attack,if=!buff.prowl.up&!buff.shadowmeld.up" );
def->add_action( "tigers_fury,if=(cooldown.bs_inc.remains<=1|cooldown.bs_inc.remains>10|variable.holdBerserk)&(cooldown.frantic_frenzy.remains<buff.tigers_fury.duration-1.5|cooldown.frantic_frenzy.remains>22|!talent.frantic_frenzy|spell_targets=1|fight_style.dungeonroute|fight_style.dungeonslice)","tigers fury on cooldown, in aoe patchwerk we can hold ~2s for frantic frenzy as needed. Like frantic frenzy, holding can sometimes be a gain in dr/ds, but the conditions are unclear. Addendum: Seems't've been related to holding Berserk, attempt to check holding tigers fury at your own risk." );
def->add_action( "rake,if=buff.prowl.up|buff.shadowmeld.up","rake out of stealth" );
def->add_action( "chomp,if=buff.chomp_enabler.up","chomp if its possible. Check this in aoe" );
def->add_action( "call_action_list,name=cooldown" );
def->add_action( "ferocious_bite,if=buff.apex_predators_craving.up","cast bite with apex procs asap" );
def->add_action( "call_action_list,name=finisher,if=spell_targets=1" );
def->add_action( "call_action_list,name=aoe_finisher,if=spell_targets>=2" );
def->add_action( "call_action_list,name=builder,if=spell_targets=1&combo_points<=4" );
def->add_action( "call_action_list,name=aoe_builder,if=spell_targets>1&combo_points<=4" );
def->add_action( "regrowth,if=buff.predatory_swiftness.up&variable.regrowth" );

aoe_builder->add_action( "rake,target_if=refreshable,if=(talent.doubleclawed_rake&(!talent.lunar_inspiration|!talent.panthers_guile|active_dot.rake<5))|hero_tree.wildstalker&(active_dot.rake<2+!talent.panthers_guile+talent.lunar_inspiration)","ensure at last 2(pg)/3(nopg) rake at all times if wildstalker, rake highest prio with dcr. on 4t raking all 4 first (no pg) is 0.3% gain. DCR PG LI rakes up to 6 then moonfire" );
aoe_builder->add_action( "moonfire_cat,target_if=refreshable","there are scenarios where this is -0.1/-0.2% vs being under cc swipe line, but im willing to scrap that complexity" );
aoe_builder->add_action( "swipe_cat,if=hero_tree.druid_of_the_claw&buff.bs_inc.up|buff.clearcasting.react&(hero_tree.druid_of_the_claw|spell_targets<7)","with dotc, nodcr/nowildslashes, swipe spam in berserk at 2t. Wildstalker begins ignoring cc again if you have the energy to rake at 7t" );
aoe_builder->add_action( "swipe_cat,if=buff.sudden_ambush.up&spell_targets.swipe_cat>=5+(2*hero_tree.wildstalker)","sa swipe takes precedence over single-rake at 5t for dotc 7t for wildstalker (0.1% at 7t for wildstalker, 0.2% for dotc at 5t)" );
aoe_builder->add_action( "rake,target_if=refreshable,if=hero_tree.wildstalker|spell_targets.swipe_cat<=variable.dotc_rake_threshold","no dcr, spread rakes up to 3t(ws+noiw)->5t(nows+noiw/wild slashes+iw)->8t(no ws+iw)" );
aoe_builder->add_action( "rake,target_if=min:pmultiplier,if=persistent_multiplier>pmultiplier&spell_targets=2","on 2t replace weaker rakes with stronger ones. (its a tiny gain for 3t too, @0.2% but keeping track of 3 snapshots is a bit much I think)" );
aoe_builder->add_action( "shred,if=combo_points<=1&spell_targets=2&talent.panthers_guile","with panther's guile, its worth using shred at 0/1 combo points on 2t" );
aoe_builder->add_action( "swipe_cat,if=combo_points>1|spell_targets>2|!talent.panthers_guile" );

builder->add_action( "prowl,if=!buff.shadowmeld.up&(action.rake.pmultiplier<1.6|dot.rake.refreshable)" );
builder->add_action( "shadowmeld,if=!buff.prowl.up&(action.rake.pmultiplier<1.6|dot.rake.refreshable)" );
builder->add_action( "shred,if=buff.sudden_ambush.up&hero_tree.druid_of_the_claw","0.2% for dotc, like, do we even care? Like, the only reason I even put this in is to feel something" );
builder->add_action( "rake,if=(buff.tigers_fury.up|remains<cooldown.tigers_fury.remains)&(refreshable&persistent_multiplier>=pmultiplier|remains<2|persistent_multiplier>pmultiplier)","freely upgrade rakes" );
builder->add_action( "moonfire_cat,if=(buff.tigers_fury.up|remains<cooldown.tigers_fury.remains)&(refreshable&persistent_multiplier>=pmultiplier|remains<2|persistent_multiplier>pmultiplier)","freely upgrade li, aggressive moonfire clipping can be a gain in some talent builds, look further into this." );
builder->add_action( "shred","!buff.clearcasting.react&persistent_multiplier>=pmultiplier|remains<2" );

aoe_finisher->add_action( "primal_wrath,target_if=min:remains,if=combo_points>=5&spell_targets.primal_wrath>1&(dot.primal_wrath.remains<6.5&!buff.bs_inc.up|dot.primal_wrath.refreshable)","primal wrath in pandemic during berserk, else with sub 6.5s remaining" );
aoe_finisher->add_action( "ferocious_bite,if=buff.ravage.up&combo_points>=5&!talent.primal_wrath&spell_targets>=2+(3*!talent.rampant_ferocity)","without pw, we prio ravage>rip at 2t with rampant ferocity, and 5t without TODO: check vines" );
aoe_finisher->add_action( "rip,target_if=min:remains,if=combo_points>=5&!talent.primal_wrath&refreshable","if we dont have primal wrath, indefinitely apply manual 4cp rips (minmax note: without rampant, its ~0.3% better to do 5cp rip/5cp bite on 3/4t)" );
aoe_finisher->add_action( "ferocious_bite,target_if=min:dot.rip.remains,if=combo_points>=5&(talent.rampant_ferocity|buff.ravage.up&spell_targets<8|dot.bloodseeker_vines.ticking&spell_targets<5)","we can send bite with rampant or under 5t with vines, 8t with ravage. TODO: check these numbers" );
aoe_finisher->add_action( "primal_wrath,if=combo_points>=5","fallback primal wrath" );
aoe_finisher->add_action( "ferocious_bite,target_if=min:dot.rip.remains,if=combo_points>=5","if we dont have primal wrath then bite here" );

finisher->add_action( "rip,if=combo_points>=5&refreshable&(buff.tigers_fury.up|remains<cooldown.tigers_fury.remains)","maintain rip in single-target. 4cp rips is a ~0.1-0.2% gain, omitted for simplicity" );
finisher->add_action( "pool_resource,for_next=1" );
finisher->add_action( "ferocious_bite,max_energy=1,if=combo_points>=5" );

cd_variable->add_action( "variable,name=convokeCountRemaining,value=floor(cooldown.convoke_the_spirits.charges_fractional+fight_remains%cooldown.convoke_the_spirits.duration-0.05)","these variables math out how many uses of each relevant cooldown remain, with a buffer (as a %of a cast) as we dont care if a cd comes up as the fight ends" );
cd_variable->add_action( "variable,name=zerkCountRemaining,value=floor(cooldown.bs_inc.charges_fractional+fight_remains%cooldown.bs_inc.duration-0.05)" );
cd_variable->add_action( "variable,name=potCountRemaining,value=floor(cooldown.potion.charges_fractional+fight_remains%cooldown.potion.duration-0.06)" );
cd_variable->add_action( "variable,name=slot1CountRemaining,value=floor(trinket.1.cooldown.charges_fractional+fight_remains%trinket.1.cooldown.duration-0.05)" );
cd_variable->add_action( "variable,name=slot2CountRemaining,value=floor(trinket.2.cooldown.charges_fractional+fight_remains%trinket.2.cooldown.duration-0.05)" );
cd_variable->add_action( "variable,name=holdBerserk,value=variable.zerkCountRemaining=1&variable.potCountRemaining=1&cooldown.potion.remains","hold berserk for potion if able without losing a use" );
cd_variable->add_action( "variable,name=holdPot,value=variable.potCountRemaining=floor((fight_remains-cooldown.bs_inc.remains-10)%cooldown.potion.duration+(fight_remains>cooldown.bs_inc.remains))","hold potion for berserk if doing so does not lose you a cast (compares the thereotical potion casts remaining to what potCountRemaining will be once the berserk is ready). If they are same, this variable is true. Potion always gets used if berserk is already casted." );
cd_variable->add_action( "variable,name=highestCDremaining,op=setif,condition=talent.convoke_the_spirits,value=cooldown.convoke_the_spirits.remains<?cooldown.bs_inc.remains<?cooldown.potion.remains,value_else=cooldown.bs_inc.remains<?cooldown.potion.remains","tells us when our upcoming cooldown windows are for use in trinket lines" );
cd_variable->add_action( "variable,name=lowestCDremaining,op=setif,condition=talent.convoke_the_spirits,value=cooldown.convoke_the_spirits.remains>?cooldown.bs_inc.remains>?cooldown.potion.remains,value_else=cooldown.bs_inc.remains>?cooldown.potion.remains" );
cd_variable->add_action( "variable,name=secondLowestCDremaining,op=setif,condition=cooldown.convoke_the_spirits.remains>cooldown.bs_inc.remains,value=cooldown.convoke_the_spirits.remains>?cooldown.potion.remains,value_else=cooldown.bs_inc.remains>?cooldown.potion.remains" );

cooldown->add_action( "call_action_list,name=cd_variable,if=!cooldown.bs_inc.remains|!cooldown.potion.remains|!trinket.1.cooldown.remains|!trinket.2.cooldown.remains","this line is here to reduce the amount of variable calls in the APL by only checking variables when they are potentially relevant." );
cooldown->add_action( "use_item,slot=trinket1,if=trinket.1.has_use_damage&(trinket.2.cooldown.remains>20|!trinket.2.has_use_buff|(cooldown.tigers_fury.remains<25&cooldown.tigers_fury.remains>20))&(!trinket.1.is.font_of_venomous_rage|!buff.bs_inc.up)","non-stat on use trinkets get used on cooldown, so long as it wont interfere with a stat on-use trinket" );
cooldown->add_action( "use_item,slot=trinket2,if=trinket.2.has_use_damage&(trinket.1.cooldown.remains>20|!trinket.1.has_use_buff|(cooldown.tigers_fury.remains<25&cooldown.tigers_fury.remains>20))&(!trinket.2.is.font_of_venomous_rage|!buff.bs_inc.up)" );
cooldown->add_action( "berserking" );
cooldown->add_action( "potion,if=buff.bs_inc.up|fight_remains<32|buff.tigers_fury.up&!variable.holdPot","potion during berserk, fallback pot if the fight is going to end within its duration. Expedite use if you would lose a pot use by holding it." );
cooldown->add_action( "use_items","non trinket gear-on-uses have variable rules on whether or not they trigger the trinket shared CD. For the cases they do we will need specific APL entries. For now just use on cooldown." );
cooldown->add_action( "use_item,slot=trinket1,use_off_gcd=1,if=trinket.1.has_use_buff&(cooldown.tigers_fury.remains>=25|trinket.1.is.algethar_puzzle_box&(!raid_event.adds.up|raid_event.adds.remains>13)&cooldown.tigers_fury.remains<2)&((cooldown.bs_inc.remains<5|buff.bs_inc.remains>=14)&!variable.holdBerserk|cooldown.convoke_the_spirits.remains<10&talent.ashamanes_guidance&talent.convoke_the_spirits|variable.lowestCDremaining>trinket.1.cooldown.duration|variable.zerkCountRemaining=1&(!talent.convoke_the_spirits|variable.convokeCountRemaining=1)&variable.potCountRemaining=1&(variable.highestCDremaining+3)>trinket.1.cooldown.duration|variable.zerkCountRemaining=variable.convokeCountRemaining&talent.convoke_the_spirits&variable.zerkCountRemaining!=variable.potCountRemaining&(cooldown.bs_inc.remains<?cooldown.convoke_the_spirits.remains)>trinket.1.cooldown.duration|variable.slot1CountRemaining=variable.potCountRemaining+1&buff.potion.up|trinket.2.has_use_buff&(variable.secondLowestCDremaining>trinket.1.cooldown.duration&variable.lowestCDremaining>trinket.2.cooldown.remains|variable.zerkCountRemaining=1&(!talent.convoke_the_spirits|variable.convokeCountRemaining=1)&variable.potCountRemaining=1&variable.highestCDremaining>trinket.2.cooldown.remains|variable.zerkCountRemaining=variable.convokeCountRemaining&talent.convoke_the_spirits&variable.zerkCountRemaining!=variable.potCountRemaining&(cooldown.convoke_the_spirits.remains<?cooldown.bs_inc.remains)>trinket.2.cooldown.remains|variable.convokeCountRemaining=0&variable.zerkCountRemaining=0&variable.potCountRemaining=0))","stat on-use trinkets, essentially this compares the number of trinket uses to the number of other cooldown uses remaining in the fight to determine whether or not to send the trinket." );
cooldown->add_action( "use_item,slot=trinket2,use_off_gcd=1,if=trinket.2.has_use_buff&(!trinket.1.has_use_buff|trinket.1.cooldown.remains>20)&(cooldown.tigers_fury.remains>=25|trinket.2.is.algethar_puzzle_box&(!raid_event.adds.up|raid_event.adds.remains>13)&(cooldown.tigers_fury.remains<2|buff.tigers_fury.up))&(buff.potion.up|variable.slot2CountRemaining!=variable.potCountRemaining)&((cooldown.bs_inc.remains<5|buff.bs_inc.remains>=14)&!variable.holdBerserk|cooldown.convoke_the_spirits.remains<10&talent.ashamanes_guidance&talent.convoke_the_spirits|variable.lowestCDremaining>trinket.2.cooldown.duration|variable.zerkCountRemaining=1&(!talent.convoke_the_spirits|variable.convokeCountRemaining=1)&variable.potCountRemaining=1&(variable.highestCDremaining+3)>trinket.2.cooldown.duration|variable.zerkCountRemaining=variable.convokeCountRemaining&talent.convoke_the_spirits&variable.zerkCountRemaining!=variable.potCountRemaining&(cooldown.bs_inc.remains<?cooldown.convoke_the_spirits.remains)>trinket.2.cooldown.duration|variable.slot1CountRemaining=variable.potCountRemaining-1&buff.potion.up|trinket.1.has_use_buff&(variable.secondLowestCDremaining>trinket.2.cooldown.duration&variable.lowestCDremaining>trinket.1.cooldown.remains|variable.zerkCountRemaining=1&(!talent.convoke_the_spirits|variable.convokeCountRemaining=1)&variable.potCountRemaining=1&variable.highestCDremaining>trinket.1.cooldown.remains|variable.zerkCountRemaining=variable.convokeCountRemaining&talent.convoke_the_spirits&variable.zerkCountRemaining!=variable.potCountRemaining&(cooldown.convoke_the_spirits.remains<?cooldown.bs_inc.remains)>trinket.1.cooldown.remains|variable.convokeCountRemaining=0&variable.zerkCountRemaining=0&variable.potCountRemaining=0))" );
cooldown->add_action( "use_item,slot=trinket1,if=fight_remains<=(21<?trinket.1.proc.any_dps.duration)","fallback use if fight is ending soon" );
cooldown->add_action( "use_item,slot=trinket2,if=fight_remains<=(21<?trinket.2.proc.any_dps.duration)" );
cooldown->add_action( "berserk,if=buff.tigers_fury.up&!variable.holdBerserk" );
cooldown->add_action( "feral_frenzy,if=!talent.frantic_frenzy&combo_points<=2+(2*buff.bs_inc.up)","0-2 combo points outside zerk, 0-4 combo points inside zerk" );
cooldown->add_action( "frantic_frenzy,if=(!fight_style.dungeonroute&!fight_style.dungeonslice|raid_event.adds.remains>5)&(buff.tigers_fury.up&spell_targets>=2|combo_points<=2+(2*buff.bs_inc.up))","we can find some gains in dslice by holding frantic frenzy for larger target counts occasionally, but its not by a lot, whereas holding tends to devolve pretty quickly in droute. I suspect this is due to how reliant on single target droute ends up being. Further investigations should be looked into." );
cooldown->add_action( "convoke_the_spirits,if=(buff.bs_inc.up|talent.ashamanes_guidance&(cooldown.bs_inc.remains>45|variable.holdBerserk))&buff.tigers_fury.up&(prev_gcd.1.rip|prev_gcd.1.ferocious_bite|prev_gcd.1.primal_wrath|buff.tigers_fury.remains<=1+action.convoke_the_spirits.execute_time)|fight_remains<5","line up convoke with berserk" );
