#!/usr/bin/env python

import sys, struct

_REMOVE_FIELD = 0x00
_ADD_FIELD    = 0x01

_DIFF_DATA = {
    17898: { },
    18125: {
        'Spell.dbc' : [
            ( 'extra_coeff', _REMOVE_FIELD ),
        ],
        'SpellMisc.dbc' : [
            ( 'unk_wod_1', _ADD_FIELD, 'flags_16467' ),
        ],
        'SpellEffect.dbc': [
            ( ( 'ap_coefficient', '%13.10f' ),  _ADD_FIELD, 'unk_14040' ),
        ],
        'Item.db2' : [
            ( 'unk_wod_1', _ADD_FIELD, 'sheath' ),
            ( 'unk_wod_2', _ADD_FIELD, 'unk_wod_1' ),
        ],
        'SpellPower.dbc': [
            ( 'unk_wod_1', _ADD_FIELD, 'cost' )
        ],
        'GlyphProperties.dbc': [
            ( 'id_exclusive_category', _ADD_FIELD, 'unk_3' )
        ],
        'ChrSpecialization.dbc': [
            ( 'unk_wod_1', _ADD_FIELD, 'unk_15699' ),
            ( 'unk_wod_2', _ADD_FIELD, 'unk_wod_1' ),
            ( 'unk_wod_3', _ADD_FIELD, 'unk_wod_2' ),
        ],
        'SpellEffectScaling.dbc': [
            ( 'unk_17124', _REMOVE_FIELD )
        ],
        'ItemDisplayInfo.dbc' : [
            ( 'unk_15464', _REMOVE_FIELD ),
            ( 'f24', _REMOVE_FIELD ),
            ( 'f23', _REMOVE_FIELD ),
        ],
        'ItemSet.dbc': [
            ( 'id_spell_1', _REMOVE_FIELD ),
            ( 'id_spell_2', _REMOVE_FIELD ),
            ( 'id_spell_3', _REMOVE_FIELD ),
            ( 'id_spell_4', _REMOVE_FIELD ),
            ( 'id_spell_5', _REMOVE_FIELD ),
            ( 'id_spell_6', _REMOVE_FIELD ),
            ( 'id_spell_7', _REMOVE_FIELD ),
            ( 'id_spell_8', _REMOVE_FIELD ),
            ( 'n_items_1', _REMOVE_FIELD ),
            ( 'n_items_2', _REMOVE_FIELD ),
            ( 'n_items_3', _REMOVE_FIELD ),
            ( 'n_items_4', _REMOVE_FIELD ),
            ( 'n_items_5', _REMOVE_FIELD ),
            ( 'n_items_6', _REMOVE_FIELD ),
            ( 'n_items_7', _REMOVE_FIELD ),
            ( 'n_items_8', _REMOVE_FIELD )
        ],
        'Item-sparse.db2': [
            ( 'id_name_desc', _ADD_FIELD, 'unk_131' ),
            ( 'id_spell_1', _REMOVE_FIELD ),
            ( 'id_spell_2', _REMOVE_FIELD ),
            ( 'id_spell_3', _REMOVE_FIELD ),
            ( 'id_spell_4', _REMOVE_FIELD ),
            ( 'id_spell_5', _REMOVE_FIELD ),
            ( 'trg_spell_1', _REMOVE_FIELD ),
            ( 'trg_spell_2', _REMOVE_FIELD ),
            ( 'trg_spell_3', _REMOVE_FIELD ),
            ( 'trg_spell_4', _REMOVE_FIELD ),
            ( 'trg_spell_5', _REMOVE_FIELD ),
            ( 'chg_spell_1', _REMOVE_FIELD ),
            ( 'chg_spell_2', _REMOVE_FIELD ),
            ( 'chg_spell_3', _REMOVE_FIELD ),
            ( 'chg_spell_4', _REMOVE_FIELD ),
            ( 'chg_spell_5', _REMOVE_FIELD ),
            ( 'cd_spell_1', _REMOVE_FIELD ),
            ( 'cd_spell_2', _REMOVE_FIELD ),
            ( 'cd_spell_3', _REMOVE_FIELD ),
            ( 'cd_spell_4', _REMOVE_FIELD ),
            ( 'cd_spell_5', _REMOVE_FIELD ),
            ( 'cat_spell_1', _REMOVE_FIELD ),
            ( 'cat_spell_2', _REMOVE_FIELD ),
            ( 'cat_spell_3', _REMOVE_FIELD ),
            ( 'cat_spell_4', _REMOVE_FIELD ),
            ( 'cat_spell_5', _REMOVE_FIELD ),
            ( 'cdc_spell_1', _REMOVE_FIELD ),
            ( 'cdc_spell_2', _REMOVE_FIELD ),
            ( 'cdc_spell_3', _REMOVE_FIELD ),
            ( 'cdc_spell_4', _REMOVE_FIELD ),
            ( 'cdc_spell_5', _REMOVE_FIELD )
        ],
    },
    18164: {
        'Item-sparse.db2': [
            ( 'socket_cont_1', _REMOVE_FIELD ),
            ( 'socket_cont_2', _REMOVE_FIELD ),
            ( 'socket_cont_3', _REMOVE_FIELD )
        ]
    },
    18179: {
        'SpellPower.dbc': [
            ( 'id_display', _REMOVE_FIELD )
        ]
    },
    18297: {
        'Item.db2': [
            ( 'id_display', _REMOVE_FIELD )
        ]
    },
    18322: {
        'SpellMisc.dbc': [
            ( 'unk_18322', _ADD_FIELD, 'prj_speed' )
        ]
    },
    18379: {
        'SpellMisc.dbc': [
            ( 'id_spell', _REMOVE_FIELD ),
            ( 'unk_1', _REMOVE_FIELD ),
            ( 'unk_18322', _REMOVE_FIELD ),
            ( 'unk_18379', _ADD_FIELD, 'mask_school' )
        ]
    },
    18505: {
        'ChrSpecialization.dbc': [
            ( 'unk_wod_1', _REMOVE_FIELD ),
            ( 'unk_wod_2', _REMOVE_FIELD )
        ],
        'SpellPower.dbc': [
            ( 'unk_18505', _ADD_FIELD, 'unk_1' )
        ]
    },
    18566: {
        'ChrSpecialization.dbc': [
            ( 'unk_wod_1', _ADD_FIELD, 'ofs_name' ),
        ]
    },
}

# Base DBC/DB2 fields, works for WoW build 17898 (Mists of Pandaria/5.4.7)
_DBC_FIELDS = {
    # New in WOD, splits spell related functionality out of ItemSet.dbc
    'ItemSetSpell.dbc': [
        'id', 'id_item_set', 'id_spell', 'n_req_items', 'unk_wod_1'
    ],
    'ItemEffect.db2': [
        'id', 'id_item', 'index', ( 'id_spell', '%6u' ), 'trigger_type', ( 'cooldown_category', '%6d' ), ( 'cooldown_category_duration', '%6d' ), ( 'cooldown_group', '%6d' ), ( 'cooldown_group_duration', '%6d' )
    ],
    'MinorTalent.dbc': [
        'id', 'id_spec', 'id_spell', 'index'
    ],
    'ItemBonus.db2': [
        ( 'id', '%6u' ), ( 'id_node', '%4u' ), ( 'type', '%2u' ), ( 'val1', '%6d' ), ( 'val2', '%6d' ), ( 'index', '%2u' )
    ],
    'ItemXBonusTree.db2': [
        ( 'id', '%6u' ), ( 'id_item', '%6u' ), ( 'id_tree', '%4u' )
    ],
    'ItemBonusTreeNode.db2': [
        ( 'id', '%4u' ), ( 'id_tree', '%4u' ), ( 'index', '%3u' ), ( 'id_child', '%4u' ), ( 'id_node', '%4u' )
    ],
    'ArmorLocation.dbc': [
        ( 'id', '%3u' ), ( 'v_1', '%11.6f' ), ( 'v_2', '%11.6f' ), ( 'v_3', '%11.6f' ), ( 'v_4', '%11.6f' ), ( 'v_5', '%11.6f')
    ],
    'ChatChannels.dbc' : [
          'id', 'unk_1', 'unk_2', 'unk_3', 'unk_4'
    ],
    'ChrRaces.dbc' : [
          'id',       'unk_1',      'id_faction', 'unk_3',      'm_model',    'f_model',
          'unk_6',    'id_team',    'unk_8',      'unk_9',      'unk_10',     'unk_11',
          'id_movie', 'faction',    'ofs_name',   'ofs_f_name', 'ofs_n_name', 'unk_17',
          'unk_19',   'expansion',  'unk_21',     'unk_22',     'unk_23',     'unk_24',
          'unk_25'
    ],
    'ChrSpecialization.dbc' : [
          'id', 'f1', 'class_id', 'id_mastery', 'f4', 'spec_id', 'f6', 'spec_type', 'id_icon', 'unk_15650', 'unk_15961', 'ofs_name', 'ofs_desc', 'unk_15699'
    ],
    'CurrencyCategory.dbc' : [
        'id', 'f1', 'ofs_name'
    ],
    'CurrencyTypes.dbc' : [
        'id', 'id_category', 'ofs_name', 'ofs_icon', 'f4', 'f5', 'f6', 'currency_max', 'f8', 'f9', 'ofs_desc'
    ],
    'GameTables.dbc' : [
          'id', 'ofs_data_file', 'f1', 'f3'
    ],
    'GemProperties.dbc' : [
          ( 'id', '%5u' ), ( 'id_enchant', '%5u' ), 'unk_3', 'unk_4', ( 'color', '%3u' ), ( 'min_ilevel', '%4u' )
    ],
    'GlyphProperties.dbc' : [
          'id',      ( 'id_spell', '%5u' ),  'flags',      'unk_3'
    ],
    'GlyphRequiredSpec.db2': [
          'id', 'id_glyph_property', 'id_spec',
    ],
    'GlyphExclusiveCategory.db2': [
          'id', 'ofs_desc'
    ],
    'Item.db2' : [
          'id',      ( 'classs', '%2d' ), ( 'subclass', '%2d' ), ( 'unk_3', '%d' ), ( 'material', '%d' ),
          'id_display', 'type_inv',   'sheath'
    ],
    'ItemArmorQuality.dbc' : [
        'id',
      ( 'v_1', '%11.6f' ), ( 'v_2', '%11.6f' ), ( 'v_3', '%11.6f' ), ( 'v_4', '%11.6f' ), ( 'v_5', '%11.6f' ),
      ( 'v_6', '%11.6f' ), ( 'v_7', '%11.6f' ),
      ( 'ilevel', '%4u' )
    ],
    'ItemArmorShield.dbc' : [
        'id', ( 'ilevel', '%4u' ),
      ( 'v_1', '%11.1f' ), ( 'v_2', '%11.1f' ), ( 'v_3', '%11.1f' ), ( 'v_4', '%11.1f' ), ( 'v_5', '%11.1f' ),
      ( 'v_6', '%11.1f' ), ( 'v_7', '%11.1f' ),
    ],
    'ItemArmorTotal.dbc' : [
        'id', ( 'ilevel', '%4u' ), ( 'v_1', '%12.6f' ), ( 'v_2', '%12.6f' ), ( 'v_3', '%12.6f' ), ( 'v_4', '%12.6f' )
    ],
    'ItemClass.dbc' : [
          ( 'id', '%3d' ), 'unk_1', 'unk_2', 'unk_3', 'ofs_name'
    ],
    'ItemDamageOneHand.dbc' : [
          'id',
          ( 'v_1', '%11.6f' ), ( 'v_2', '%11.6f' ), ( 'v_3', '%11.6f' ), ( 'v_4', '%11.6f' ), ( 'v_5', '%11.6f' ),
          ( 'v_6', '%11.6f' ), ( 'v_7', '%11.6f' ),
          ( 'ilevel', '%4u' )
    ],
    'ItemDamageOneHandCaster.dbc' : [
          'id',
          ( 'v_1', '%11.6f' ), ( 'v_2', '%11.6f' ), ( 'v_3', '%11.6f' ), ( 'v_4', '%11.6f' ), ( 'v_5', '%11.6f' ),
          ( 'v_6', '%11.6f' ), ( 'v_7', '%11.6f' ),
          ( 'ilevel', '%4u' )
    ],
    'ItemDamageRanged.dbc' : [
          'id',
          ( 'v_1', '%11.6f' ), ( 'v_2', '%11.6f' ), ( 'v_3', '%11.6f' ), ( 'v_4', '%11.6f' ), ( 'v_5', '%11.6f' ),
          ( 'v_6', '%11.6f' ), ( 'v_7', '%11.6f' ),
          ( 'ilevel', '%4u' )
    ],
    'ItemDamageTwoHand.dbc' : [
          'id',
          ( 'v_1', '%11.6f' ), ( 'v_2', '%11.6f' ), ( 'v_3', '%11.6f' ), ( 'v_4', '%11.6f' ), ( 'v_5', '%11.6f' ),
          ( 'v_6', '%11.6f' ), ( 'v_7', '%11.6f' ),
          ( 'ilevel', '%4u' )
    ],
    'ItemDamageTwoHandCaster.dbc' : [
          'id',
          ( 'v_1', '%11.6f' ), ( 'v_2', '%11.6f' ), ( 'v_3', '%11.6f' ), ( 'v_4', '%11.6f' ), ( 'v_5', '%11.6f' ),
          ( 'v_6', '%11.6f' ), ( 'v_7', '%11.6f' ),
          ( 'ilevel', '%4u' )
    ],
    'ItemDamageThrown.dbc' : [
          'id',
          ( 'v_1', '%11.6f' ), ( 'v_2', '%11.6f' ), ( 'v_3', '%11.6f' ), ( 'v_4', '%11.6f' ), ( 'v_5', '%11.6f' ),
          ( 'v_6', '%11.6f' ), ( 'v_7', '%11.6f' ),
          ( 'ilevel', '%4u' )
    ],
    'ItemDamageWand.dbc' : [
          'id',
          ( 'v_1', '%11.6f' ), ( 'v_2', '%11.6f' ), ( 'v_3', '%11.6f' ), ( 'v_4', '%11.6f' ), ( 'v_5', '%11.6f' ),
          ( 'v_6', '%11.6f' ), ( 'v_7', '%11.6f' ),
          ( 'ilevel', '%4u' )
    ],
    'ItemDisplayInfo.dbc' : [
          'id',
          'f1', 'f2', 'f3', 'f4', 'ofs_icon', 'f6', 'f7', 'f8', 'f9', 'f10',
          'f11', 'f12', 'f13', 'f14', 'f15', 'f16', 'f17', 'f18', 'f19', 'f20',
          'f21', 'f22', 'f23', 'f24', 'unk_15464'
    ],
    'ItemNameDescription.dbc' : [
        'id', 'ofs_desc', ( 'flags', '%#.8x' )
    ],
    'Item-sparse.db2': [
          ( 'id', '%5u' ),          ( 'quality', '%2u' ),     ( 'flags', '%#.8x' ),     ( 'flags_2', '%#.8x' ),       ( 'unk_14732', '%#.8x' ),
          ( 'unk_14732_2', '%f' ),  ( 'unk_14890', '%#.8x' ), ( 'unk_16173', '%#.8x' ),   'buy_price',
            'sell_price',           ( 'inv_type', '%2u' ),    ( 'race_mask', '%#.8x' ), ( 'class_mask', '%#.8x' ),    ( 'ilevel', '%4u' ),
          ( 'req_level', '%3d' ),   ( 'req_skill', '%4u' ),   ( 'req_skill_rank', '%4u' ), 'req_spell',                  'req_honor_rank',
            'req_city_rank',          'req_rep_faction',        'req_rep_rank',           'max_count',                  'stackable',
            'container_slots', 
          ( 'stat_type_1', '%3d' ), ( 'stat_type_2', '%3d' ), ( 'stat_type_3', '%3d' ), ( 'stat_type_4', '%3d' ),     ( 'stat_type_5', '%3d' ),
          ( 'stat_type_6', '%3d' ), ( 'stat_type_7', '%3d' ), ( 'stat_type_8', '%3d' ), ( 'stat_type_9', '%3d' ),     ( 'stat_type_10', '%3d' ),
          ( 'stat_val_1', '%4d' ),  ( 'stat_val_2', '%4d' ),  ( 'stat_val_3', '%4d' ),  ( 'stat_val_4', '%4d' ),      ( 'stat_val_5', '%4d' ),
          ( 'stat_val_6', '%4d' ),  ( 'stat_val_7', '%4d' ),  ( 'stat_val_8', '%4d' ),  ( 'stat_val_9', '%4d' ),      ( 'stat_val_10', '%4d' ),
          ( 'stat_alloc_1', '%5u' ), ( 'stat_alloc_2', '%5u' ), ( 'stat_alloc_3', '%5u' ), ( 'stat_alloc_4', '%5u' ),     ( 'stat_alloc_5', '%5u' ),
          ( 'stat_alloc_6', '%5u' ), ( 'stat_alloc_7', '%5u' ), ( 'stat_alloc_8', '%5u' ), ( 'stat_alloc_9', '%5u' ),     ( 'stat_alloc_10', '%5u' ),
          ( 'stat_socket_mul_1', '%.3f'  ), ( 'stat_socket_mul_2', '%.3f'  ), ( 'stat_socket_mul_3', '%.3f'  ), ( 'stat_socket_mul_4', '%.3f'  ),     ( 'stat_socket_mul_5', '%.3f'  ),
          ( 'stat_socket_mul_6', '%.3f'  ), ( 'stat_socket_mul_7', '%.3f'  ), ( 'stat_socket_mul_8', '%.3f'  ), ( 'stat_socket_mul_9', '%.3f'  ),     ( 'stat_socket_mul_10', '%.3f' ),
            'scale_stat_dist',        'damage_type',          ( 'delay', '%5d' ),     ( 'ranged_mod_range', '%f' ), 
          ( 'id_spell_1', '%5d' ),  ( 'id_spell_2', '%5d' ),  ( 'id_spell_3', '%5d' ),  ( 'id_spell_4', '%5d' ),      ( 'id_spell_5', '%5d' ),
          ( 'trg_spell_1', '%3d' ), ( 'trg_spell_2', '%3d' ), ( 'trg_spell_3', '%3d' ), ( 'trg_spell_4', '%3d' ),     ( 'trg_spell_5', '%3d' ),
          ( 'chg_spell_1', '%5d' ), ( 'chg_spell_2', '%5d' ), ( 'chg_spell_3', '%5d' ), ( 'chg_spell_4', '%5d' ),     ( 'chg_spell_5', '%5d' ),
          ( 'cd_spell_1', '%7d' ),  ( 'cd_spell_2', '%7d' ),  ( 'cd_spell_3', '%7d' ),  ( 'cd_spell_4', '%7d' ),      ( 'cd_spell_5', '%7d' ),
          ( 'cat_spell_1', '%5d' ), ( 'cat_spell_2', '%5d' ), ( 'cat_spell_3', '%5d' ), ( 'cat_spell_4', '%5d' ),     ( 'cat_spell_5', '%5d' ),
          ( 'cdc_spell_1', '%6d' ), ( 'cdc_spell_2', '%6d' ), ( 'cdc_spell_3', '%6d' ), ( 'cdc_spell_4', '%6d' ),     ( 'cdc_spell_5', '%6d' ),
          ( 'bonding', '%2d' ),
            'ofs_name',               'ofs_name_2',             'ofs_name_3',             'ofs_name_4',                 'ofs_desc',
            'page_text',              'id_lang',                'page_mat',               'start_quest',                'id_lock',
            'material',               'sheath',                 'rand_prop',            ( 'rand_suffix', '%4u' ),     ( 'item_set', '%4u' ),
            'area',                   'map',                    'bag_family',                 'totem_category',
          ( 'socket_color_1', '%3d' ), ( 'socket_color_2', '%3d' ), ( 'socket_color_3', '%3d' ),
          ( 'socket_cont_1', '%3d' ),  ( 'socket_cont_2', '%3d' ),  ( 'socket_cont_3', '%3d' ),
          ( 'socket_bonus', '%5d' ),   ( 'gem_props', '%4u' ),
          ( 'item_damage_modifier', '%8.3f' ),  'duration',               'item_limit_category',    'id_holiday',               ( 'weapon_damage_range', '%7.6f' ),
            'unk_130',                'unk_131',
    ],
    'ItemSet.dbc' : [
          'id', 'ofs_name', 
          'id_item_1', 'id_item_2', 'id_item_3', 'id_item_4', 'id_item_5',
          'id_item_6', 'id_item_7', 'id_item_8', 'id_item_9', 'id_item_10',
          'id_item_11', 'id_item_12', 'id_item_13', 'id_item_14', 'id_item_15',
          'id_item_16', 'id_item_17',
          'id_spell_1', 'id_spell_2', 'id_spell_3', 'id_spell_4',
          'id_spell_5', 'id_spell_6', 'id_spell_7', 'id_spell_8',
          'n_items_1', 'n_items_2', 'n_items_3', 'n_items_4',
          'n_items_5', 'n_items_6', 'n_items_7', 'n_items_8',
          'id_req_skill', 'val_req_skill'
    ],
    'ItemReforge.dbc': [
         'id', 'src_stat', ( 'm_src', '%.2f' ), 'dst_stat', ( 'm_dst', '%.2f' )
    ],
    'ItemRandomProperties.dbc' : [
          'id',         'ofs_name_int',
          'id_spell_1', 'id_spell_2', 'id_spell_3', 'id_spell_4', 'id_spell_5', 
          'ofs_name_sfx'
    ],
    'ItemRandomSuffix.dbc' : [
          ( 'id','%4u' ),   'ofs_name_sfx',   'ofs_name_int',
          ( 'id_property_1', '%5u' ),  ( 'id_property_2',  '%5u' ), ( 'id_property_3', '%5u' ),  ( 'id_property_4', '%5u' ),  ( 'id_property_5', '%5u' ),
          ( 'property_pct_1', '%5u' ), ( 'property_pct_2', '%5u' ), ( 'property_pct_3', '%5u' ), ( 'property_pct_4', '%5u' ), ( 'property_pct_5', '%5u' )
    ],
    'ItemSpec.dbc' : [
        'id', 'f1', 'f2', 'f3', 'f4', 'f5', 'f6'
    ],
    'ItemUpgrade.db2' : [
          ( 'id', '%5u' ), 'f1', ( 'upgrade_ilevel', '%4u' ), 'prev_id', 'id_currency_type', 'cost'
    ],
    'ItemUpgradePath.dbc' : [
        'id'
    ],
    'JournalEncounterItem.dbc' : [
        'id', 'id_encounter', 'id_item', ( 'flags_1', '%#.8x' ), ( 'flags_2', '%#.8x' ), 'unk_17124',
    ],
    'OverrideSpellData.dbc' : [
        'id', 'f1', 'f2', 'f3', 'f4', 'f6', 'f7', 'f8', 'f9', 'f10', 'f11', 'f12', ( 'f13', '%8d' )
    ],
    'RandPropPoints.dbc' : [
          ( 'id', '%3u' ),
          ( 'epic_points_1', '%4u' ), ( 'epic_points_2', '%4u' ), ( 'epic_points_3', '%4u' ), ( 'epic_points_4', '%4u' ), ( 'epic_points_5', '%4u' ),
          ( 'rare_points_1', '%4u' ), ( 'rare_points_2', '%4u' ), ( 'rare_points_3', '%4u' ), ( 'rare_points_4', '%4u' ), ( 'rare_points_5', '%4u' ),
          ( 'uncm_points_1', '%4u' ), ( 'uncm_points_2', '%4u' ), ( 'uncm_points_3', '%4u' ), ( 'uncm_points_4', '%4u' ), ( 'uncm_points_5', '%4u' ),
    ],
    'RulesetItemUpgrade.db2' : [
        ( 'id', '%5u' ), 'upgrade_level', ( 'id_upgrade_base', '%5u' ), ( 'id_item', '%5u' )
    ],
    'RulesetRaidOverride.dbc' : [
        'id', 'f1', 'f2', 'f3', 'f4', 'f5'
    ],
    'SkillLine.dbc' : [
          'id',         'id_category',    'ofs_name', 'ofs_desc',
          'spell_icon', 'alternate_verb', 'can_link', 'unk_15464',
          'unk_1'
    ],
    'SkillLineAbility.dbc' : [
          'id',          ( 'id_skill', '%4u' ),   'id_spell',        ( 'mask_race', '%#.8x' ),        ( 'mask_class', '%#.8x' ),
          'req_skill_level', 'replace_id', 'unk_3', 'max_learn_skill', 'unk_5', 'reward_skill_pts', 'idx', 'id_filter'
    ],
    'SkillLineAbilitySortedSpell.dbc' : [
          'id',          'field'
    ],
    'SkillRaceClassInfo.dbc' : [
          ( 'id', '%4u' ),          'id_skill',      ( 'mask_race', '%#.8x' ),   ('mask_class', '%#.8x' ),       ( 'flags', '%#.8x' ),
          'req_level',   'id_skill_tier', 'id_skill_cost',   'unk'
    ],
    'SpecializationSpells.dbc' : [
        'id', 'spec_id', 'spell_id', 'replace_spell_id', 'f5'
    ],
    'Spell.dbc' : [
        ( 'id', '%6u' ),           ( 'ofs_name', '%5u' ),     ( 'ofs_rank', '%5u' ),       ( 'ofs_desc', '%5u' ),       ( 'ofs_tool_tip', '%5u' ),
          'id_rune_cost',          ( 'id_missile', '%5u' ),   ( 'id_desc_var', '%5u' ),    ( 'extra_coeff', '%5.7f' ),
        ( 'id_scaling', '%5u' ),   ( 'id_aura_opt', '%5u' ),  ( 'id_aura_rest', '%5u' ),   ( 'id_cast_req', '%5u' ),    ( 'id_categories', '%5u' ),  
        ( 'id_class_opts', '%5u' ),( 'id_cooldowns', '%5u' ), ( 'id_equip_items', '%5u' ), ( 'id_interrupts', '%5u' ),  ( 'id_levels', '%5u' ),
        ( 'id_power', '%5u' ),     ( 'id_reagents', '%5u' ),  ( 'id_shapeshift', '%5u' ),  ( 'id_tgt_rest', '%5u' ),    ( 'id_totems', '%5u' ),
        ( 'id_misc', '%5u' ),
    ],
    'SpellMisc.dbc' : [
        'id', 'id_spell', 'unk_1', ( 'flags', '%#.8x' ),      ( 'flags_1', '%#.8x' ),      ( 'flags_2', '%#.8x' ),      ( 'flags_3', '%#.8x' ),
        ( 'flags_4', '%#.8x' ),    ( 'flags_5', '%#.8x' ),    ( 'flags_6', '%#.8x' ),      ( 'flags_7', '%#.8x' ),      ( 'flags_12694', '%#.8x' ),
        ( 'flags_8', '%#.8x' ), ( 'unk_2', '%#.8x' ), ( 'flags_15668', '%#.8x' ), ( 'flags_16467', '%#.8x' ), 'id_cast_time', 'id_duration',
        'id_range', ( 'prj_speed', '%f' ), 'id_spell_visual_1', 'id_spell_visual_2', 'id_icon',
        'id_active_icon', ( 'mask_school', '%#.2x' )
    ],
    'SpellAuraOptions.dbc' : [
          'id', 'id_spell', 'unk_15589_1', ( 'stack_amount', '%3u' ), ( 'proc_chance', '%3u' ), ( 'proc_charges', '%2d' ), ( 'proc_flags', '%#.8x' ),
          ( 'internal_cooldown', '%7u' ), ( 'id_ppm', '%u' ),
    ],
    'SpellCastTimes.dbc' : [
          'id', ( 'cast_time', '%5d' ), ( 'cast_time_per_level', '%d' ), ( 'min_cast_time', '%5d' )
    ],
    'SpellChainEffects.dbc' : [
        'id',
        ( 'f1', '%f' ), ( 'f2', '%f' ), 'f3', ( 'f4', '%f' ), 'f5', 'f6', 'f7', 'f8', 'f9', 'f10',
        'f11', 'f12', ( 'f13', '%f' ), ( 'f14', '%f' ), 'f15', ( 'f16', '%f' ), 'f17', 'f18', ( 'f19', '%f' ), 'f10',
        ( 'f21', '%f' ), 'f22', 'f23', 'f24', 'f25', ( 'f26', '%f' ), 'f27', 'f28', 'f29', 'f10',
        'f31', 'f32', 'f33', 'f34', 'f35', 'f36', 'f37', 'f38', 'f39', 'f10',
        'f41', 'f42', 'f43', 'f44',
    ],
    'SpellClassOptions.dbc' : [
          'id', 'modal_next_spell', ( 'spell_family_flags_1', '%#.8x' ), ( 'spell_family_flags_2', '%#.8x' ), ( 'spell_family_flags_3', '%#.8x' ), ( 'spell_family_flags_4', '%#.8x' ), 'spell_family_name'
    ],
    'SpellCastingRequirements.dbc' : [
          'id', 'f1', 'f2', 'f3', 'f4', 'f5', 'f6'
    ],
    'SpellCategory.dbc' : [
          'id', 'field', 'f2', 'f3', 'f4', 'f5'
    ],
    'SpellCategories.dbc' : [
          'id',              'id_spell', 'unk_15589_1', ( 'category', '%4u' ), 'dmg_class', 'dispel', 'mechanic', 
          'type_prevention', 'start_recovery_category', 'unk_15464'
    ],
    'SpellCooldowns.dbc' : [
        'id', 'id_spell', 'unk_15589_1', ( 'category_cooldown', '%7u' ), ( 'cooldown', '%7u' ), ( 'gcd_cooldown', '%4u' )
    ],
    'SpellDescriptionVariables.dbc' : [
        'id', 'ofs_var'
    ],
    'SpellDifficulty.dbc' : [
        'id', 'id_spell_1', 'id_spell_2', 'id_spell_3', 'id_spell_4'
    ],
    'SpellDuration.dbc'  : [
        'id', ( 'duration_1', '%9d' ), 'duration_2', 'duration_3'
    ],
    'SpellEffect.dbc' : [
        ( 'id', '%6u' ),               'unk_15589_1',                  ( 'type', '%4u' ),                ( 'multiple_value', '%f' ), ( 'sub_type', '%4u' ),             ( 'amplitude', '%5u' ),
        ( 'base_value', '%7d' ),     ( 'coefficient', '%13.10f' ),     ( 'dmg_multiplier', '%f' ),   'chain_target',                  ( 'die_sides', '%2d' ),
          'item_type',                   'mechanic',                     ( 'misc_value', '%7d' ),    ( 'misc_value_2', '%7d' ),         ( 'points_per_combo_points', '%5.1f' ), 
          'id_radius',                   'id_radius_max',                ( 'real_ppl', '%5.3f' ),    ( 'class_mask_1', '%#.8x' ),    ( 'class_mask_2', '%#.8x' ),
        ( 'class_mask_3', '%#.8x' ),   ( 'class_mask_4', '%#.8x' ),  ( 'trigger_spell', '%5d' ),     ( 'unk_15589_2', '%f' ), 'implicit_target_1',              'implicit_target_2',
    ( 'id_spell', '%6u' ),             ( 'index', '%2u' ),           'unk_14040'
    ],
    # New in MoP
    'SpellEffectScaling.dbc' : [
        'id', ( 'average', '%13.10f' ), ( 'delta', '%13.10f' ), ( 'bonus', '%13.10f' ), ( 'unk_17124', '%f' ), ( 'id_effect', '%6u' )
    ],
    'SpellEquippedItems.dbc' : [
        'id', 'id_spell', 'unk_15589_1', ( 'item_class', '%4d' ), ( 'mask_inv_type', '%#.8x' ), ( 'mask_sub_class', '%#.8x' )
    ],
    'SpellIcon.dbc' : [
        'id', 'ofs_name'
    ],
    'SpellItemEnchantment.dbc' : [
          ( 'id', '%4u' ), 'charges', 
          ( 'type_1', '%3u' ), ( 'type_2', '%3u' ), ( 'type_3', '%3u' ), 
          ( 'amount_1', '%4d' ), ( 'amount_2', '%4d' ), ( 'amount_3', '%4d' ),
          ( 'id_property_1', '%6u' ), ( 'id_property_2', '%6u' ), ( 'id_property_3', '%6u' ), 
            'ofs_desc', 'id_aura', ( 'slot', '%2u' ), ( 'id_gem', '%6u' ), 'enchantment_condition', 
          ( 'req_skill', '%4u' ), ( 'req_skill_value', '%3u' ), 'req_player_level', ( 'max_scaling_level', '%3u' ), ( 'min_scaling_level', '%3u' ),
          ( 'id_scaling', '%2d' ), ( 'unk_15464_3', '%2d' ), ( 'coeff_1', '%7.4f' ), ( 'coeff_2', '%7.4f' ), ( 'coeff_3', '%7.4f' ),
    ],
    'SpellLearnSpell.dbc' : [
          'id', 'unk_1', 'unk_2', 'unk_3'
    ],
    'SpellLevels.dbc' : [
          'id', 'id_spell', 'unk_15589_1', ( 'base_level', '%3u' ), ( 'max_level', '%2u' ), 'spell_level'
    ],
    'SpellPower.dbc' : [
          ( 'id', '%6u' ), 'id_spell', 'unk_15589_1', ( 'type_power', '%2d' ),( 'cost', '%6d' ),     ( 'cost_per_second', '%3d' ),     'unk_1', 'unk_2',
          'id_display', ( 'cost_2', '%5.2f' ),     ( 'cost_per_second2', '%5.2f' ),   ( 'aura_id', '%6u' ), ( 'unk_15589_2', '%f' )
    ],
    'SpellProcsPerMinute.dbc' : [
         'id', ( 'ppm', '%5.3f' ), 'unk_1'
    ],
    'SpellProcsPerMinuteMod.dbc' : [
        'id', 'unk_1', 'id_chr_spec', ( 'coefficient', '%f' ), 'id_ppm'
    ],
    'SpellRadius.dbc' : [
          'id', ( 'radius_1', '%7.1f' ), 'radius_2', 'unk_15589_1', ( 'radius_3', '%7.f' )
    ],
    'SpellRange.dbc' : [
          'id',       ( 'min_range', '%7.1f' ), ( 'min_range_2', '%5.1f' ), ( 'max_range', '%7.1f' ), ( 'max_range_2', '%5.1f' ),
          'id_range',   'unk_6',                  'unk_7'
    ],
    'SpellRuneCost.dbc' : [
          'id', 'rune_cost_1', 'rune_cost_2', 'rune_cost_3', 'rune_cost_4', ( 'rune_power_gain', '%3u' )
    ],
    'SpellScaling.dbc' : [
          'id',                      ( 'cast_min', '%5d' ),           ( 'cast_max', '%5d' ),       ( 'cast_div', '%2u' ),     ( 'id_class', '%2d' ),
        ( 'c_scaling', '%13.10f' ),  ( 'c_scaling_threshold', '%2u' ), ( 'max_scaling_level', '%2u' ),               'unk_15464_2'
          
    ],
    'SpellTargetRestrictions.dbc' : [
          'id', 'max_affected_targets', 'max_target_level', 'target_type', 'targets'
    ],
    'Talent.dbc' : [
        ( 'id', '%5u' ),      ( 'spec_id', '%3u' ),    ( 'row', '%3u' ),         ( 'col', '%3u' ),       ( 'id_spell', '%7u' ),
        ( 'pet', '%5u' ),     ( 'unk_15464_1', '%#.8x' ),       ( 'unk_15464_2', '%#.8x' ),     'class_id',           ( 'id_replace', '%9u' ),
        'ofs_desc',  
    ],
    'TalentTab.dbc' : [
        ( 'id', '%5u' ),       'ofs_name',          'spell_icon', ( 'mask_class', '%#.3x' ), ( 'mask_pet_talent', '%#.1x' ),
        ( 'tab_page', '%2u' ), 'ofs_internal_name', 'ofs_desc',     'unk_8',               'unk_12759_1',
          'unk_12759_2'
    ],
    'TalentTreePrimarySpells.dbc' : [
          'id', 'id_talent_tab', 'id_spell', 'field_3'
    ],
    'gtChanceToMeleeCrit.dbc' : [
          'id', ( 'gt_value', '%.10f' )
    ],
    'gtChanceToMeleeCritBase.dbc' : [
          'id', ( 'gt_value', '%.10f' )
    ],
    'gtChanceToSpellCrit.dbc' : [
          'id', ( 'gt_value', '%.10f' )
    ],
    'gtChanceToSpellCritBase.dbc' : [
          'id', ( 'gt_value', '%.10f' )
    ],
    'gtCombatRatings.dbc' : [
          'id', ( 'gt_value', '%.10f' )
    ],
    'gtOCTClassCombatRatingScalar.dbc' : [
          'id', ( 'gt_value', '%.10f' )
    ],
    'gtOCTBaseMPByClass.dbc' : [
        'id', ( 'gt_value', '%.10f' )
    ],
    'gtOCTRegenMP.dbc' : [
          'id', ( 'gt_value', '%.10f' )
    ],
    'gtRegenMPPerSpt.dbc' : [
          'id', ( 'gt_value', '%.10f' )
    ],
    'gtSpellScaling.dbc' : [
          'id', ( 'gt_value', '%.10f' )
    ],
    'gtOCTBaseMPByClass.dbc' : [
          'id', ( 'gt_value', '%7.0f' )
    ],
    'gtOCTBaseHPByClass.dbc' : [
          'id', ( 'gt_value', '%7.0f' )
    ],
    'gtOCTHpPerStamina.dbc' : [
          'id', ( 'gt_value', '%.10f' )
    ],
    'gtItemSocketCostPerLevel.dbc' : [
        'id', ( 'gt_value', '%.10f' )
    ],
    'gtArmorMitigationByLvl.dbc' : [
        'id', ( 'gt_value', '%.10f' )
    ]
}

class DBCRecord(object):
    __default = None

    @classmethod
    def default(cls, *args):
        if not cls.__default:
            cls.__default = cls(None, None)

        # Ugly++ but it will have to do
        for i in xrange(0, len(args), 2):
            setattr(cls.__default, args[i], args[i + 1])

        return cls.__default
        
    @classmethod
    def data_size(cls):
        return struct.Struct(''.join(cls._format)).size

    def __init__(self, parser, record):
        self._record     = record
        self._dbc_parser = parser

    def has_value(self, field, value):
        if not hasattr(self, field):
            return False
        
        if type(value) in [list, tuple] and getattr(self, field) in value:
            return True
        elif type(value) in [str, int, float] and getattr(self, field) == value:
            return True

        return False
        
    def parse(self):
        if not self._record: return None
        record_tuple = struct.unpack(''.join(self._format), self._record)
        
        for i in xrange(0, len(record_tuple)):
            setattr(self, self._fields[i], record_tuple[i])
        
        return self

    def value(self, *args):
        v = [ ]
        for attr in args:
            if attr in dir():
                v += [ getattr(self, attr) ]
            else:
                v += [ None ]

        return v

    def field(self, *args):
        f = [ ]
        for field in args:
            if field in self._fields:
                f += [ self._field_fmt[self._fields.index(field)] % getattr(self, field) ]
            else:
                f += [ None ]

        return f
    
    def __str__(self):
        s = ''

        for i in self._fields:
            s += '%s=%s ' % (i, (self._field_fmt[self._fields.index(i)] % getattr(self, i)).strip())

        if self._dbc_parser._options.debug == True:
            s += 'bytes=['
            for b in xrange(0, len(self._record)):
                s += '%.02x' % ord(self._record[b])
                if (b + 1) % 4 == 0 and b < len(self._record) - 1:
                    s += ' '
            
            s += ']'
        return s

class SpellEffect(DBCRecord):
    def __init__(self, dbc_parser, record):
        DBCRecord.__init__(self, dbc_parser, record)
        
        self.scaling = None

class Item_sparse(DBCRecord):
    def __init__(self, dbc_parser, record):
        DBCRecord.__init__(self, dbc_parser, record)

        self.name     = 0
        self.spells   = []

    def field(self, *args):
        f = DBCRecord.field(self, *args)

        if 'name' in args:
            f[args.index('name')] = '%-55s' % (self.name and ('"%s"' % self.name.replace('"', '\\"'))) or ('%s' % self.name)

        return f

    def parse(self):
        DBCRecord.parse(self)

        # Find DBCStrings available for the spell
        if self.ofs_name != 0:
            self.name = self._dbc_parser.get_string_block(self.ofs_name)
        else:
            self.name = 0

        if self.ofs_desc != 0:
            self.desc = self._dbc_parser.get_string_block(self.ofs_desc)
        else:
            self.desc = 0

        return self

    def __str__(self):
        s = DBCRecord.__str__(self)
        s += ' name=\"%s\" ' % self.name

        if self.desc != 0:
            s += 'desc=\"%s\" ' % self.desc

        return s
    
class ItemDisplayInfo(DBCRecord):
    def __init__(self, dbc_parser, record):
        DBCRecord.__init__(self, dbc_parser, record)

        self.icon     = ''

    def field(self, *args):
        f = DBCRecord.field(self, *args)

        if 'icon' in args:
            if self.icon != "":
                f[args.index('icon')] = '%-40s' % ('"%s"' % self.icon.replace('"', '\\"').lower())
            else:
                f[args.index('icon')] = '%-40s' % '0'

        return f

    def parse(self):
        DBCRecord.parse(self)

        # Find DBCStrings available for the spell
        if self.ofs_icon != 0:
            self.icon = self._dbc_parser.get_string_block(self.ofs_icon)
        else:
            self.icon = ""

        return self

    def __str__(self):
        s = DBCRecord.__str__(self)
        if self.ofs_icon != 0:
            s += 'icon=\"%s\" ' % self.icon

        return s

class SpellIcon(DBCRecord):
    def __init__(self, dbc_parser, record):
        DBCRecord.__init__(self, dbc_parser, record)

        self.name     = 0

    def field(self, *args):
        f = DBCRecord.field(self, *args)

        if 'name' in args:
            if self.name:
                f[args.index('name')] = '%s' % ('"%s"' % self.name.replace('"', '\\"').lower()[self.name.rfind('\\') + 1:])
            else:
                f[args.index('name')] = '0'

        return f

    def parse(self):
        DBCRecord.parse(self)

        # Find DBCStrings available for the spell
        if self.ofs_name != 0:
            self.name = self._dbc_parser.get_string_block(self.ofs_name)

        return self

    def __str__(self):
        s = DBCRecord.__str__(self)
        if self.ofs_name:
            s += 'name=\"%s\" ' % self.name

        return s

class Spell(DBCRecord):
    def __init__(self, dbc_parser, record):
        DBCRecord.__init__(self, dbc_parser, record)

        self._effects = [ None ]
        self.name     = 0
        self.desc     = 0
        self.tt       = 0
        self.rank     = 0
        self.max_effect_index = 0
        self._powers  = []
        self._misc    = []

    def has_effect(self, field, value):
        for effect in self._effects:
            if not effect:
                continue

            if effect.has_value(field, value):
                return True

        return False

    def add_effect(self, spell_effect):
        if spell_effect.index > self.max_effect_index:
            self.max_effect_index = spell_effect.index
            for i in xrange(0, (self.max_effect_index + 1) - len(self._effects)):
                self._effects.append( None )

        self._effects[ spell_effect.index ] = spell_effect

        setattr(self, 'effect_%d' % ( spell_effect.index + 1 ), spell_effect)

    def add_power(self, spell_power):
        self._powers.append(spell_power)
        #if len(self._powers) < spell_power.type_power + 2 + 1:
        #    for i in xrange(0, spell_power.type_power + 2 + 1 - len(self._powers)):
        #        self._powers.append( None )
        #    
        #    self._powers[ spell_power.type_power + 2 ] = spell_power
    
    def add_misc(self, misc):
        self._misc.append( misc )
    
    def __getattr__(self, name):
        # Hack to get effect default values if spell has no effect_x, as those fields 
        # are the only ones that may be missing in Spellxxx. Just in case raise an 
        # attributeerror if something else is being accessed
        if 'effect_' in name:
            return SpellEffect.default()
            
        raise AttributeError

    def field(self, *args):
        f = DBCRecord.field(self, *args)

        if 'name' in args:
            if self.name == 0:
                f[args.index('name')] = '%-36d' % self.name
            else:
                f[args.index('name')] = '%-36s' % ('"%s"' % self.name.replace(r'"', r'\"'))
           
        for field_id in [ 'desc', 'tt', 'rank' ]:
            if field_id in args:
                v = getattr(self, field_id)
                if v == 0:
                    f[args.index(field_id)] = repr('%s' % v)[1:-1]
                else:
                    s = repr(v)
                    s = s[1:-1]
                    s = s.replace(r'"', r'\"')
                    f[args.index(field_id)] = '"%s"' % s

        return f

    def parse(self):
        DBCRecord.parse(self)

        # Find DBCStrings available for the spell
        if self.ofs_name != 0:
            self.name = self._dbc_parser.get_string_block(self.ofs_name)

        if self.ofs_rank != 0:
            self.rank = self._dbc_parser.get_string_block(self.ofs_rank)

        if self.ofs_desc != 0:
            self.desc = self._dbc_parser.get_string_block(self.ofs_desc)

        if self.ofs_tool_tip != 0:
            self.tt = self._dbc_parser.get_string_block(self.ofs_tool_tip)

        return self

    def __str__(self):
        s = DBCRecord.__str__(self)
        s += ' name=\"%s\" ' % self.name
        if self.rank != 0:
            s += 'rank=\"%s\" ' % self.rank

        if self.desc != 0:
            s += 'desc=\"%s\" ' % self.desc

        if self.tt != 0:
            s += 'tt=\"%s\" ' % self.tt

        for i in xrange(len(self._effects)):
            if not self._effects[i]:
                continue
            s += 'effect_%d={ %s} ' % (i + 1, str(self._effects[i]))

        return s

class SpellCooldowns(DBCRecord):
    def __getattribute__(self, name):
        if name == 'cooldown_duration':
            if self.category_cooldown > 0:
                return self.category_cooldown
            elif self.cooldown > 0:
                return self.cooldown
            else:
                return 0
        else:
            return DBCRecord.__getattribute__(self, name)

    def field(self, *args):
        f = DBCRecord.field(self, *args)

        if 'cooldown_duration' in args:
            if self.cooldown > 0:
                f[args.index('cooldown_duration')] = '%7u' % self.cooldown
            elif self.category_cooldown > 0:
                f[args.index('cooldown_duration')] = '%7u' % self.category_cooldown
            else:
                f[args.index('cooldown_duration')] = '%7u' % 0

        return f

class SpellPower(DBCRecord):
    def __getattribute__(self, name):
        if name == 'power_cost':
            if self.cost > 0:
                return self.cost
            elif self.cost_perc > 0:
                return self.cost_perc
            elif self.unk_12759 > 0:
                return self.unk_12759
            else:
                return 0
        else:
            return DBCRecord.__getattribute__(self, name)
    
    def field(self, *args):
        f = DBCRecord.field(self, *args)

        # Add a special 'power_cost' field kludge
        if 'power_cost' in args:
            if self.cost > 0:
                f[args.index('power_cost')] = '%6.2f' % self.cost
            elif self.cost_perc > 0:
                f[args.index('power_cost')] = '%6.2f' % self.cost_perc
            elif self.unk_12759 > 0:
                f[args.index('power_cost')] = '%6.2f' % self.unk_12759
            else:
                f[args.index('power_cost')] = '%6.2f' % 0

        return f

class ItemRandomSuffix(DBCRecord):
    def __init__(self, dbc_parser, record):
        DBCRecord.__init__(self, dbc_parser, record)

        self.name_sfx = 0
        self.name_int = 0

    def parse(self):
        DBCRecord.parse(self)

        if self.ofs_name_sfx != 0:
            self.name_sfx = self._dbc_parser.get_string_block(self.ofs_name_sfx)

        if self.ofs_name_int != 0:
            self.name_int = self._dbc_parser.get_string_block(self.ofs_name_int)

    def __str__(self):
        s = ''

        if self.name_sfx:
            s += 'suffix=\"%s\" ' % self.name_sfx

        if self.name_int:
            s += 'internal_name=\"%s\" ' % self.name_int

        s += DBCRecord.__str__(self)

        return s

    def field(self, *args):
        f = DBCRecord.field(self, *args)

        if 'suffix' in args:
            f[args.index('suffix')] = '%-42s' % ((self.name_sfx and '"%s"' or '%s') % self.name_sfx)

        if 'internal' in args:
            f[args.index('internal')] = '%-42s' % ((self.name_int and '"%s"' or '%s') % self.name_int)

        return f

class ItemRandomProperties(DBCRecord):
    def parse(self):
        DBCRecord.parse(self)

        if self.ofs_name_int != 0:
            self.name_int = self._dbc_parser.get_string_block(self.ofs_name_int)
        else:
            self.name_int = ''

        if self.ofs_name_sfx != 0:
            self.name_sfx = self._dbc_parser.get_string_block(self.ofs_name_sfx)
        else:
            self.name_sfx = ''

    def __str__(self):
        s = ''

        if self.name_int:
            s += 'name_int=\"%s\" ' % self.name_int

        if self.name_sfx:
            s += 'name_sfx=\"%s\" ' % self.name_sfx

        s += DBCRecord.__str__(self)

        return s

class SkillLine(DBCRecord):
    def parse(self):
        DBCRecord.parse(self)

        # Find DBCStrings available for the spell
        if self.ofs_name != 0:
            self.name = self._dbc_parser.get_string_block(self.ofs_name)
        else:
            self.name = ''

        if self.ofs_desc != 0:
            self.desc = self._dbc_parser.get_string_block(self.ofs_desc)
        else:
            self.desc = ''
    
    def __str__(self):
        s = 'name="%s" ' % self.name

        if self.desc:
            s += 'desc=\"%s\" ' % self.desc

        s += DBCRecord.__str__(self)

        return s

class Talent(DBCRecord):
    def parse(self):
        DBCRecord.parse(self)

        if self.ofs_desc != 0:
            self.desc = self._dbc_parser.get_string_block(self.ofs_desc)
        else:
            self.desc = ''
    
    def __str__(self):
        s = DBCRecord.__str__(self)
        if self.desc:
            s += 'desc=\"%s\" ' % self.desc

        return s

class GlyphExclusiveCategory(DBCRecord):
    def parse(self):
        DBCRecord.parse(self)

        if self.ofs_desc != 0:
            self.desc = self._dbc_parser.get_string_block(self.ofs_desc)
        else:
            self.desc = ''
    
    def __str__(self):
        s = DBCRecord.__str__(self)
        if self.desc:
            s += 'desc=\"%s\" ' % self.desc

        return s

class ChrSpecialization(DBCRecord):
    def parse(self):
        DBCRecord.parse(self)

        # Find DBCStrings available for the spell
        if self.ofs_name != 0:
            self.name = self._dbc_parser.get_string_block(self.ofs_name)
        else:
            self.name = ''

        if self.ofs_desc != 0:
            self.desc = self._dbc_parser.get_string_block(self.ofs_desc)
        else:
            self.desc = ''

    def __str__(self):
        s = DBCRecord.__str__(self)
        s += 'name="%s" ' % self.name

        if self.desc:
            s += 'desc=\"%s\" ' % self.desc


        return s

class CurrencyTypes(DBCRecord):
    def parse(self):
        DBCRecord.parse(self)

        # Find DBCStrings available for the spell
        if self.ofs_desc != 0:
            self.desc = self._dbc_parser.get_string_block(self.ofs_desc)
        else:
            self.desc = ''
        
        if self.ofs_name != 0:
            self.name = self._dbc_parser.get_string_block(self.ofs_name)
        else:
            self.name = ''

        if self.ofs_icon != 0:
            self.icon = self._dbc_parser.get_string_block(self.ofs_icon)
        else:
            self.icon = ''

    def __str__(self):
        s = DBCRecord.__str__(self)
        s += 'desc="%s" ' % self.desc
        s += 'name="%s" ' % self.name
        s += 'icon="%s" ' % self.icon

        return s

class CurrencyCategory(DBCRecord):
    def parse(self):
        DBCRecord.parse(self)

        # Find DBCStrings available for the spell
        if self.ofs_name != 0:
            self.name = self._dbc_parser.get_string_block(self.ofs_name)
        else:
            self.name = ''

    def __str__(self):
        s = DBCRecord.__str__(self)
        s += 'name="%s" ' % self.name

        return s

class ItemNameDescription(DBCRecord):
    def __init__(self, dbc_parser, record):
        DBCRecord.__init__(self, dbc_parser, record)

        self.desc = ''

    def parse(self):
        DBCRecord.parse(self)

        # Find DBCStrings available for the spell
        if self.ofs_desc != 0:
            self.desc = self._dbc_parser.get_string_block(self.ofs_desc)
        else:
            self.desc = ''

    def __str__(self):
        s = DBCRecord.__str__(self)
        s += 'desc="%s" ' % self.desc

        return s

class SpellItemEnchantment(DBCRecord):
    def __init__(self, dbc_parser, record):
        DBCRecord.__init__(self, dbc_parser, record)

        self.desc = 0

    def parse(self):
        DBCRecord.parse(self)

        if self.ofs_desc != 0:
            self.desc = self._dbc_parser.get_string_block(self.ofs_desc)
    
    def field(self, *args):
        f = DBCRecord.field(self, *args)

        if 'desc' in args:
            f[args.index('desc')] = '%-25s' % ((self.desc and '"%s"' or '%s') % self.desc)

        return f

    def __str__(self):
        s = ''
        if self.desc:
            s += 'desc=\"%s\" ' % self.desc

        s += DBCRecord.__str__(self)

        return s

class ItemClass(DBCRecord):
    def parse(self):
        DBCRecord.parse(self)

        if self.ofs_name != 0:
            self.name = self._dbc_parser.get_string_block(self.ofs_name)
        else:
            self.name = ''

    def __str__(self):
        s = ''
        if self.name:
            s += 'name=\"%s\" ' % self.name

        s += DBCRecord.__str__(self)

        return s

class SpellDescriptionVariables(DBCRecord):
    def __init__(self, dbc_parser, record):
        DBCRecord.__init__(self, dbc_parser, record)

        self.var = 0
    
    def parse(self):
        DBCRecord.parse(self)

        if self.ofs_var != 0:
            self.var = self._dbc_parser.get_string_block(self.ofs_var)

    def field(self, *args):
        f = DBCRecord.field(self, *args)

        if 'var' in args:
            #print dir(self), self.ofs_var
            v = getattr(self, 'var')
            if v == 0:
                f[args.index('var')] = repr('%s' % v)[1:-1]
            else:
                s = repr(v)
                s = s[1:-1]
                s = s.replace(r'"', r'\"')
                f[args.index('var')] = '"%s"' % s

        return f
    
    def __str__(self):
        s = DBCRecord.__str__(self)

        if self.var:
            s += 'var=\"%s\" ' % self.var

        return s

class ItemSet(DBCRecord):
    def parse(self):
        DBCRecord.parse(self)

        self.bonus = []

        if self.ofs_name != 0:
            self.name = self._dbc_parser.get_string_block(self.ofs_name)
        else:
            self.name = ''
    
    def __str__(self):
        s = ''
        if self.name:
            s += 'name=\"%s\" ' % self.name

        s += DBCRecord.__str__(self)

        return s

class ChrRaces(DBCRecord):
    def parse(self):
        DBCRecord.parse(self)

        # Find DBCStrings available for the spell
        if self.ofs_name != 0:
            self.name = self._dbc_parser.get_string_block(self.ofs_name)
        else:
            self.name = ''

        if self.ofs_f_name != 0:
            self.f_name = self._dbc_parser.get_string_block(self.ofs_f_name)
        else:
            self.f_name = ''
    
        if self.ofs_n_name != 0:
            self.n_name = self._dbc_parser.get_string_block(self.ofs_n_name)
        else:
            self.n_name = ''

    def __str__(self):
        if self.name:
            s = 'name="%s" ' % self.name

        if self.f_name:
            s += 'female_name=\"%s\" ' % self.f_name
            
        if self.n_name:
            s += 'neutral_name=\"%s\" ' % self.n_name

        s += DBCRecord.__str__(self)

        return s

class SkillLineCategory(DBCRecord):
    def parse(self):
        DBCRecord.parse(self)

        # Find DBCStrings available for the spell
        if self.ofs_name != 0:
            self.name = self._dbc_parser.get_string_block(self.ofs_name)
        else:
            self.name = ''
    
    def __str__(self):
        s = 'name="%s" ' % self.name

        s += DBCRecord.__str__(self)

        return s
        
class GameTables(DBCRecord):
    def parse(self):
        DBCRecord.parse(self)

        # Find DBCStrings available for the spell
        if self.ofs_data_file != 0:
            self.data_file = self._dbc_parser.get_string_block(self.ofs_data_file)
        else:
            self.data_file = ''
    
    def __str__(self):
        s = 'data_file="%s" ' % self.data_file

        s += DBCRecord.__str__(self)

        return s
        
def initialize_data_model(build, version, obj):
    # First, create base classes, based on build id 0
    for dbc_file_name, field_data in _DBC_FIELDS.iteritems():
        class_name = '%s' % dbc_file_name.split('.')[0].replace('-', '_')
        cls_fields = [ ]
        cls_format = [ ]
        cls_field_fmt = [ ]
    
        if class_name not in dir(obj):
            setattr(obj, class_name, type(r'%s' % class_name, ( DBCRecord, ), dict( DBCRecord.__dict__ )))
        
        cls = getattr(obj, class_name)
        
        for field in field_data:
            if isinstance(field, tuple):
                cls_fields.append(field[0])
                cls_field_fmt.append(field[1])
                if 'x' in field[1] or 'u' in field[1]:
                    cls_format.append('I')
                elif 'f' in field[1]:
                    cls_format.append('f')
                elif 'd' in field[1]:
                    cls_format.append('i')
                else:
                    sys.stderr.write('Unknown formatting attribute %s for %s\n' % ( field[1], field[0] ))
                
                setattr(cls, field[0], 0)
            else:
                cls_fields.append(field)
                cls_field_fmt.append('%u')
                cls_format.append('I')
                setattr(cls, field, 0)
                setattr(cls, '%s_raw' % field, 0)
        
        setattr(cls, '_fields', cls_fields)
        setattr(cls, '_format', cls_format)
        setattr(cls, '_field_fmt', cls_field_fmt)

    # Then, derive patch classes from base
    for build_id in sorted(_DIFF_DATA.keys()):
        diff_version = _DIFF_DATA[build_id].get('WOW_VERSION', 0)
        if version > 0 and diff_version > 0 and diff_version > version:
            continue

        for dbc_file_name, dbc_fields in _DBC_FIELDS.iteritems():
            if build_id > build:
                break
            
            class_base_name = dbc_file_name.split('.')[0].replace('-', '_')
            class_name      = r'%s%d' % ( class_base_name, build )
            dbc_diff_data   = _DIFF_DATA.get(build_id, { }).get(dbc_file_name)

            if class_name not in dir(obj):
                parent_class = getattr(obj, '%s' % class_base_name)
                setattr(obj, 
                    r'%s%d' % ( class_base_name, build ), 
                    type(r'%s%d' % ( class_base_name, build ), ( parent_class, ), dict(parent_class.__dict__))
                )

            cls             = getattr(obj, '%s%d' % ( class_base_name, build ) )

            if not dbc_diff_data:
                continue
            
            # If there's some diff data for us, we need to go through it and modify the created class
            # Accordingly
            for diff_data in dbc_diff_data:
                if diff_data[1] == _ADD_FIELD:
                    idx_field = cls._fields.index(diff_data[2])
                    if idx_field < 0:
                        sys.stderr.write('Unable to find field %s for build %d for addition\n' % ( diff_data[2], build_id ))
                        continue

                    field_name = diff_data[0]
                    field_format = '%u'
                    if isinstance(diff_data[0], tuple):
                        field_name = diff_data[0][0]
                        field_format = diff_data[0][1]
                        
                    cls._fields.insert(idx_field + 1, field_name)
                    if 'x' in field_format or 'u' in field_format:
                        cls._format.insert(idx_field + 1, 'I')
                    elif 'f' in field_format:
                        cls._format.insert(idx_field + 1, 'f')
                    elif 'd' in field_format:
                        cls._format.insert(idx_field + 1, 'i')
                    else:
                        sys.stderr.write('Unknown formatting attribute %s for %s\n' % ( field_format, field_name ))
                    
                    cls._field_fmt.insert(idx_field + 1, field_format)
                    setattr(cls, field_name, 0)
                    setattr(cls, '%s_raw' % field_name, 0)
                elif diff_data[1] == _REMOVE_FIELD:
                    idx_field = cls._fields.index(diff_data[0])
                    if idx_field < 0:
                        sys.stderr.write('Unable to find field %s for build %d for removal\n' % ( diff_data[0], build_id ))
                        continue
                    
                    del cls._format[idx_field]
                    del cls._fields[idx_field]
                    del cls._field_fmt[idx_field]
                    delattr(cls, diff_data[0])

