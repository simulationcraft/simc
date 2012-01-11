// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

struct enchant_data_t
{
  const char* id;
  const char* name;
  const char* encoding;
  const char* ptr_name;
  const char* ptr_encoding;
};

// This data is originally from http://armory-musings.appspot.com/ and published under a Creative Commons Attribution 3.0 License.

static enchant_data_t enchant_db[] =
{
  { "4270",  "+145 Stamina and +55 Dodge Rating",                         "145sta_55dodge",                 NULL,        NULL },
  { "4267",  "Flintlocke's Woodchucker",                                  "flintlockes_woodchucker",        NULL,        NULL },
  { "4266",  "+50 Agility",                                               "50agi",                          NULL,        NULL },
  { "4265",  "+50 Intellect",                                             "50int",                          NULL,        NULL },
  { "4258",  "+50 Agility",                                               "50agi",                          NULL,        NULL },
  { "4257",  "+50 Intellect",                                             "50int",                          NULL,        NULL },
  { "4256",  "+50 Strength",                                              "50str",                          NULL,        NULL },
  { "4250",  "+50 Agility and +25 Resilience Rating",                     "50agi_25resil",                  NULL,        NULL },
  { "4249",  "+50 Strength and +25 Resilience Rating",                    "50str_25resil",                  NULL,        NULL },
  { "4248",  "+50 Intellect and +25 Resilience Rating",                   "50int_25resil",                  NULL,        NULL },
  { "4247",  "+60 Strength and 35 Resilience rating",                     "60str_35resil",                  NULL,        NULL },
  { "4246",  "+60 Agility and 35 Resilience rating",                      "60agi_35resil",                  NULL,        NULL },
  { "4245",  "+60 Intellect and 35 Resilience rating",                    "60int_35resil",                  NULL,        NULL },
  { "4227",  "+130 Agility",                                              "130agi",                         NULL,        NULL },
  { "4223",  "Greatly increase your run speed for 5 sec.",                "",                               NULL,        NULL },
  { "4217",  "Pyrium Weapon Chain",                                       "40hit",                          NULL,        NULL },
  { "4214",  "Summons a Cardboard Assassin to draw the attention of enemies", "",                           NULL,        NULL },
  { "4209",  "+60 Agility and 35 Haste rating",                           "60agi_35haste",                  NULL,        NULL },
  { "4208",  "+60 Strength and 35 Mastery rating",                        "60str_35mastery",                NULL,        NULL },
  { "4207",  "+60 Intellect and 35 Critical Strike rating",               "60int_35crit",                   NULL,        NULL },
  { "4206",  "+90 Stamina and 35 Dodge rating",                           "90sta_35dodge",                  NULL,        NULL },
  { "4205",  "+30 Agility and +20 Mastery Rating",                        "30agi_20mastery",                NULL,        NULL },
  { "4204",  "+50 Agility and +25 Mastery Rating",                        "50agi_25mastery",                NULL,        NULL },
  { "4203",  "+30 Agility and +20 Mastery Rating",                        "30agi_20mastery",                NULL,        NULL },
  { "4202",  "+50 Strength and +25 Critical Strike Rating",               "50str_25crit",                   NULL,        NULL },
  { "4201",  "+30 Strength and +20 Critical Strike Rating",               "30str_20crit",                   NULL,        NULL },
  { "4200",  "+50 Intellect and 25 Haste Rating",                         "50int_25haste",                  NULL,        NULL },
  { "4199",  "+30 Intellect and 20 Haste Rating",                         "30int_20haste",                  NULL,        NULL },
  { "4198",  "+75 Stamina and 25 Dodge Rating",                           "75sta_25dodge",                  NULL,        NULL },
  { "4197",  "+45 Stamina and 20 Dodge Rating",                           "45sta_20dodge",                  NULL,        NULL },
  { "4196",  "+130 Intellect and 25 Haste Rating",                        "130int_25haste",                 NULL,        NULL },
  { "4195",  "+195 Stamina and 25 Dodge Rating",                          "195sta_25dodge",                 NULL,        NULL },
  { "4194",  "+130 Strength and 25 Critical Strike Rating",               "130str_25crit",                  NULL,        NULL },
  { "4193",  "+130 Agility and +25 Mastery Rating",                       "130agi_25mastery",               NULL,        NULL },
  { "4192",  "+130 Intellect",                                            "130int",                         NULL,        NULL },
  { "4191",  "+130 Strength",                                             "130str",                         NULL,        NULL },
  { "4190",  "+130 Agility",                                              "130agi",                         NULL,        NULL },
  { "4189",  "+195 Stamina",                                              "195sta",                         NULL,        NULL },
  { "4188",  "Grounded Plasma Shield",                                    "",                               NULL,        NULL },
  { "4187",  "Invisibility Field",                                        "",                               NULL,        NULL },
  { "4182",  "Spinal Healing Injector",                                   "",                               NULL,        NULL },
  { "4183",  "Injects a Mythical Mana Potion directly into your bloodstream, increasing potency and and restoring 11600 mana.", "", NULL, NULL },
  { "4181",  "Tazik Shocker",                                             "tazik_shocker",                  NULL,        NULL },
  { "4180",  "Quickflip Deflection Plates",                               "quickflip_deflection_plates",    NULL,        NULL },
  { "4179",  "Synapse Springs",                                           "synapse_springs",                NULL,        NULL },
  { "4177",  "Safety Catch Removal Kit",                                  "",                               NULL,        NULL },
  { "4176",  "R19 Threatfinder",                                          "88Hit",                          NULL,        NULL },
  { "4175",  "Gnomish X-Ray Scope",                                       "gnomish_xray",                   NULL,        NULL },
  { "4127",  "+145 Stamina and +55 Agility",                              "145sta_55agi",                   NULL,        NULL },
  { "4126",  "+190 Attack Power and +55 Crit Rating",                     "190AP_55Crit",                   NULL,        NULL },
  { "4124",  "+85 Stamina and +45 Agility",                               "85sta_45agi",                    NULL,        NULL },
  { "4122",  "+110 Attack Power and +45 Crit Rating",                     "110AP_45Crit",                   NULL,        NULL },
  { "4121",  "+44 Stamina",                                               "44sta",                          NULL,        NULL },
  { "4120",  "+36 Stamina",                                               "36sta",                          NULL,        NULL },
  { "4118",  "Swordguard Embroidery",                                     "swordguard_embroidery",          NULL,        NULL },
  { "4116",  "Darkglow Embroidery",                                       "darkglow_embroidery",            NULL,        NULL },
  { "4115",  "Lightweave Embroidery",                                     "lightweave_embroidery",          NULL,        NULL },
  { "4114",  "+95 Intellect and +55 Spirit",                              "95int_55spi",                    NULL,        NULL },
  { "4113",  "+95 Intellect and +80 Stamina",                             "95int_80sta",                    NULL,        NULL },
  { "4112",  "+95 Intellect and +80 Stamina",                             "95int_80sta",                    NULL,        NULL },
  { "4111",  "+55 Intellect and +65 Stamina",                             "55int_65sta",                    NULL,        NULL },
  { "4110",  "+95 Intellect and +55 Spirit",                              "95int_55spi",                    NULL,        NULL },
  { "4109",  "+55 Intellect and +45 Spirit",                              "55int_45spi",                    NULL,        NULL },
  { "4108",  "+65 Haste Rating",                                          "65haste",                        NULL,        NULL },
  { "4107",  "+65 Mastery Rating",                                        "65mastery",                      NULL,        NULL },
  { "4106",  "+50 Strength",                                              "50str",                          NULL,        NULL },
  { "4105",  "+25 Agility and Minor Movement Speed",                      "25agi",                          NULL,        NULL },
  { "4104",  "+35 Mastery Rating and Minor Movement Speed",               "35mastery",                      NULL,        NULL },
  { "4103",  "+75 Stamina",                                               "75sta",                          NULL,        NULL },
  { "4102",  "+20 All Stats",                                             "20all",                          NULL,        NULL },
  { "4101",  "+65 Critical Strike Rating",                                "65crit",                         NULL,        NULL },
  { "4100",  "+65 Critical Strike Rating",                                "65crit",                         NULL,        NULL },
  { "4099",  "Landslide",                                                 "landslide",                      NULL,        NULL },
  { "4098",  "Windwalk",                                                  "windwalk",                       NULL,        NULL },
  { "4097",  "Power Torrent",                                             "power_torrent",                  NULL,        NULL },
  { "4096",  "+50 Intellect",                                             "50int",                          NULL,        NULL },
  { "4095",  "+50 Expertise Rating",                                      "50exp",                          NULL,        NULL },
  { "4094",  "+50 Mastery Rating",                                        "50mastery",                      NULL,        NULL },
  { "4093",  "+50 Spirit",                                                "50spi",                          NULL,        NULL },
  { "4092",  "+50 Hit Rating",                                            "50hit",                          NULL,        NULL },
  { "4091",  "+40 Intellect",                                             "40int",                          NULL,        NULL },
  { "4090",  "+250 Armor",                                                "250Barmor",                      NULL,        NULL },
  { "4089",  "+50 Hit Rating",                                            "50hit",                          NULL,        NULL },
  { "4088",  "+40 Spirit",                                                "40spi",                          NULL,        NULL },
  { "4087",  "+50 Critical Strike Rating",                                "50crit",                         NULL,        NULL },
  { "4086",  "+50 Dodge Rating",                                          "50dodge",                        NULL,        NULL },
  { "4085",  "+50 Mastery Rating",                                        "50mastery",                      NULL,        NULL },
  { "4084",  "Heartsong",                                                 "heartsong",                      NULL,        NULL },
  { "4083",  "Hurricane",                                                 "hurricane",                      NULL,        NULL },
  { "4082",  "+50 Expertise Rating",                                      "50exp",                          NULL,        NULL },
  { "4081",  "+60 Stamina",                                               "60sta",                          NULL,        NULL },
  { "4080",  "+40 Intellect",                                             "40int",                          NULL,        NULL },
  { "4079",  "+40 Agility",                                               "40agi",                          NULL,        NULL },
  { "4078",  "+40 Strength",                                              "40str",                          NULL,        NULL },
  { "4077",  "+40 Resilience Rating",                                     "40resil",                        NULL,        NULL },
  { "4076",  "+35 Agility",                                               "35agi",                          NULL,        NULL },
  { "4075",  "+35 Strength",                                              "35str",                          NULL,        NULL },
  { "4074",  "Elemental Slayer",                                          "elemental_slayer",               NULL,        NULL },
  { "4073",  "+160 Armor",                                                "160Barmor",                      NULL,        NULL },
  { "4072",  "+30 Intellect",                                             "30int",                          NULL,        NULL },
  { "4071",  "+50 Critical Strike Rating",                                "50crit",                         NULL,        NULL },
  { "4070",  "+55 Stamina",                                               "55sta",                          NULL,        NULL },
  { "4069",  "+50 Haste Rating",                                          "50haste",                        NULL,        NULL },
  { "4068",  "+50 Haste Rating",                                          "50haste",                        NULL,        NULL },
  { "4067",  "Avalanche",                                                 "avalanche",                      NULL,        NULL },
  { "4066",  "Mending",                                                   "",                               NULL,        NULL },
  { "4065",  "+50 Haste Rating",                                          "50haste",                        NULL,        NULL },
  { "4064",  "+70 Spell Penetration",                                     "70spen",                         NULL,        NULL },
  { "4063",  "+15 All Stats",                                             "15all",                          NULL,        NULL },
  { "4062",  "+30 Stamina and Minor Movement Speed",                      "30sta",                          NULL,        NULL },
  { "4061",  "+50 Mastery",                                               "50mastery",                      NULL,        NULL },
  { "3878",  "Mind Amplification Dish",                                   "",                               NULL,        NULL },
  { "3876",  "Inscription of the Pinnacle",                               "15Dodge_15Sta",                  NULL,        NULL },
  { "3875",  "+30 Attack Power and +10 Crit Rating",                      "30AP_10Crit",                    NULL,        NULL },
  { "3873",  "+50 Spell Power and +30 Stamina",                           "50SP_30Sta",                     NULL,        NULL },
  { "3872",  "+50 Spell Power and +20 Spirit",                            "50SP_20Spi",                     NULL,        NULL },
  { "3870",  "Blood Draining",                                            "",                               NULL,        NULL },
  { "3869",  "Blade Ward",                                                "",                               NULL,        NULL },
  { "3860",  "+250 Armor",                                                "250BArmor",                      NULL,        NULL },
  { "3859",  "Springy Arachnoweave",                                      "",                               NULL,        NULL },
  { "3855",  "+69 Spell Power",                                           "69SP",                           NULL,        NULL },
  { "3854",  "+81 Spell Power",                                           "81SP",                           NULL,        NULL },
  { "3853",  "+40 Resilience Rating +28 Stamina",                         "28Sta",                          NULL,        NULL },
  { "3852",  "+30 Stamina and +15 Resilience Rating",                     "30Sta",                          NULL,        NULL },
  { "3851",  "+50 Stamina",                                               "50Sta",                          NULL,        NULL },
  { "3850",  "+40 Stamina",                                               "40Sta",                          NULL,        NULL },
  { "3849",  "Titanium Plating",                                          "",                               NULL,        NULL },
  { "3847",  "Rune of the Stoneskin Gargoyle",                            "",                               NULL,        NULL },
  { "3846",  "+40 Spell Power",                                           "40SP",                           NULL,        NULL },
  { "3845",  "+50 Attack Power",                                          "50AP",                           NULL,        NULL },
  { "3844",  "+45 Spirit",                                                "45Spi",                          NULL,        NULL },
  { "3843",  "Scope (+15 Damage)",                                        "",                               NULL,        NULL },
  { "3842",  "+37 Stamina and +20 Dodge",                                 "37Sta_20Dodge",                  NULL,        NULL },
  { "3840",  "+23 Spell Power",                                           "23SP",                           NULL,        NULL },
  { "3839",  "+40 Attack Power",                                          "40AP",                           NULL,        NULL },
  { "3838",  "+70 Spell Power and +15 Crit Rating",                       "70SP_15Crit",                    NULL,        NULL },
  { "3837",  "+60 Dodge Rating and +22 Stamina",                          "60Dodge_22Sta",                  NULL,        NULL },
  { "3836",  "+70 Spell Power and +6 Mana/5 seconds",                     "70SP_6MP5",                      NULL,        NULL },
  { "3835",  "+120 Attack Power and +15 Crit Rating",                     "120AP_15Crit",                   NULL,        NULL },
  { "3834",  "+63 Spell Power",                                           "63SP",                           NULL,        NULL },
  { "3833",  "+65 Attack Power",                                          "65AP",                           NULL,        NULL },
  { "3832",  "+10 All Stats",                                             "10all",                          NULL,        NULL },
  { "3831",  "+23 Haste Rating",                                          "23Haste",                        NULL,        NULL },
  { "3830",  "+50 Spell Power",                                           "50SP",                           NULL,        NULL },
  { "3829",  "+35 Attack Power",                                          "35AP",                           NULL,        NULL },
  { "3828",  "+85 Attack Power",                                          "85AP",                           NULL,        NULL },
  { "3827",  "+110 Attack Power",                                         "110AP",                          NULL,        NULL },
  { "3826",  "Icewalker",                                                 "12Crit_12Hit",                   NULL,        NULL },
  { "3825",  "+15 Haste Rating",                                          "15Haste",                        NULL,        NULL },
  { "3824",  "+24 Attack Power",                                          "24AP",                           NULL,        NULL },
  { "3823",  "+75 Attack Power and +22 Critical Strike Rating",           "75AP_22Crit",                    NULL,        NULL },
  { "3822",  "+55 Stamina and +22 Agility",                               "55Sta_22Agi",                    NULL,        NULL },
  { "3820",  "+30 Spell Power and 20 Critical strike rating.",            "30SP_20Crit",                    NULL,        NULL },
  { "3819",  "+30 Spell Power and 8 mana per 5 seconds.",                 "30SP_8MP5",                      NULL,        NULL },
  { "3818",  "+37 Stamina and +20 Dodge",                                 "37Sta_20Dodge",                  NULL,        NULL },
  { "3817",  "+50 Attack Power and +20 Critical Strike Rating",           "50AP_20Crit",                    NULL,        NULL },
  { "3816",  "+25 Fire Resistance and 30 Stamina",                        "30Sta",                          NULL,        NULL },
  { "3815",  "+25 Arcane Resistance and 30 Stamina",                      "30Sta",                          NULL,        NULL },
  { "3814",  "+25 Shadow Resistance and 30 Stamina",                      "30Sta",                          NULL,        NULL },
  { "3813",  "+25 Nature Resistance and 30 Stamina",                      "30Sta",                          NULL,        NULL },
  { "3812",  "+25 Frost Resistance and 30 Stamina",                       "30Sta",                          NULL,        NULL },
  { "3811",  "+20 Dodge Rating and +22 Stamina",                          "20Dodge_22Sta",                  NULL,        NULL },
  { "3810",  "+24 Spell Power and +15 Critical Strike Rating",            "24SP_15Crit",                    NULL,        NULL },
  { "3809",  "+24 Spell Power and +6 Mana/5 seconds",                     "24SP_6MP5",                      NULL,        NULL },
  { "3808",  "+40 Attack Power and +15 Crit Rating",                      "40AP_15Crit",                    NULL,        NULL },
  { "3807",  "+18 Spell Power and +5 Mana/5 seconds",                     "18SP_5MP5",                      NULL,        NULL },
  { "3806",  "+18 Spell Power and +10 Critical Strike Rating",            "18SP_10Crit",                    NULL,        NULL },
  { "3797",  "+29 Spell Power and +20 Resilience Rating",                 "29SP",                           NULL,        NULL },
  { "3795",  "+50 Attack Power and +20 Resilience Rating",                "50AP",                           NULL,        NULL },
  { "3794",  "+23 Spell Power and +15 Resilience Rating",                 "23SP",                           NULL,        NULL },
  { "3793",  "+40 Attack Power and +15 Resilience Rating",                "45AP",                           NULL,        NULL },
  { "3791",  "+30 Stamina",                                               "30Sta",                          NULL,        NULL },
  { "3790",  "Black Magic",                                               "black_magic",                    NULL,        NULL },
  { "3789",  "Berserking",                                                "berserking",                     NULL,        NULL },
  { "3788",  "Accuracy",                                                  "25Hit_25Crit",                   NULL,        NULL },
  { "3761",  "+70 Shadow Resistance",                                     "",                               NULL,        NULL },
  { "3760",  "+60 Frost Resistance",                                      "",                               NULL,        NULL },
  { "3758",  "+76 Spell Power",                                           "76SP",                           NULL,        NULL },
  { "3757",  "+102 Stamina",                                              "102Sta",                         NULL,        NULL },
  { "3756",  "+130 Attack Power",                                         "130AP",                          NULL,        NULL },
  { "3755",  "+28 Attack Power +12 Dodge Rating",                         "28AP_12Dodge",                   NULL,        NULL },
  { "3754",  "+24 Attack Power/+10 Stamina/+10 Hit Rating",               "24AP_10Sta_10Hit",               NULL,        NULL },
  { "3748",  "Titanium Spike (45-67)",                                    "",                               NULL,        NULL },
  { "3731",  "Titanium Weapon Chain",                                     "28hit",                          NULL,        NULL },
  { "3730",  "Swordguard Embroidery",                                     "swordguard_embroidery_old",      NULL,        NULL },
  { "3728",  "Darkglow Embroidery",                                       "darkglow_embroidery_old",        NULL,        NULL },
  { "3722",  "Lightweave Embroidery",                                     "lightweave_embroidery_old",      NULL,        NULL },
  { "3721",  "+50 Spell Power and +30 Stamina",                           "50SP_30Sta",                     NULL,        NULL },
  { "3720",  "+35 Spell Power and +20 Stamina",                           "35SP_20Sta",                     NULL,        NULL },
  { "3719",  "+50 Spell Power and +20 Spirit",                            "50SP_20Spi",                     NULL,        NULL },
  { "3718",  "+35 Spell Power and +12 Spirit",                            "35SP_12Spi",                     NULL,        NULL },
  { "3608",  "+40 Ranged Critical Strike",                                "40Crit",                         NULL,        NULL },
  { "3607",  "+40 Ranged Haste Rating",                                   "40Haste",                        NULL,        NULL },
  { "3606",  "Nitro Boosts",                                              "",                               NULL,        NULL },
  { "3605",  "Flexweave Underlay",                                        "",                               NULL,        NULL },
  { "3604",  "Hyperspeed Accelerators",                                   "hyperspeed_accelerators",        NULL,        NULL },
  { "3603",  "Hand-Mounted Pyro Rocket",                                  "hand_mounted_pyro_rocket",       NULL,        NULL },
  { "3601",  "Belt-Clipped Spynoculars",                                  "",                               NULL,        NULL },
  { "3599",  "Electromagnetic Pulse Generator",                           "",                               NULL,        NULL },
  { "3595",  "Rune of Spellbreaking",                                     "",                               NULL,        NULL },
  { "3594",  "Rune of Swordbreaking",                                     "",                               NULL,        NULL },
  { "3370",  "Rune of Razorice",                                          "rune_of_razorice",               NULL,        NULL },
  { "3369",  "Rune of Cinderglacier",                                     "rune_of_cinderglacier",          NULL,        NULL },
  { "3368",  "Rune of the Fallen Crusader",                               "rune_of_the_fallen_crusader",    NULL,        NULL },
  { "3367",  "Rune of Spellshattering",                                   "",                               NULL,        NULL },
  { "3366",  "Rune of Lichbane",                                          "",                               NULL,        NULL },
  { "3365",  "Rune of Swordshattering",                                   "",                               NULL,        NULL },
  { "3330",  "+18 Stamina",                                               "18Sta",                          NULL,        NULL },
  { "3329",  "+12 Stamina",                                               "12Sta",                          NULL,        NULL },
  { "3328",  "+75 Attack Power and +22 Critical Strike Rating",           "75AP_22Crit",                    NULL,        NULL },
  { "3327",  "+75 Attack Power and +22 Critical Strike Rating",           "75AP_22Crit",                    NULL,        NULL },
  { "3326",  "+55 Attack Power and +15 Critical Strike Rating",           "55AP_15Crit",                    NULL,        NULL },
  { "3325",  "+45 Stamina and +15 Agility",                               "45Sta_15Agi",                    NULL,        NULL },
  { "3297",  "+275 Health",                                               "275Health",                      NULL,        NULL },
  { "3296",  "Wisdom",                                                    "10Spi",                          NULL,        NULL },
  { "3294",  "+225 Armor",                                                "225BArmor",                      NULL,        NULL },
  { "3273",  "Deathfrost",                                                "",                               NULL,        NULL },
  { "3269",  "Truesilver Line",                                           "",                               NULL,        NULL },
  { "3260",  "+240 Armor",                                                "240BArmor",                      NULL,        NULL },
  { "3256",  "Increased Stealth and Agility 10",                          "10Agi",                          NULL,        NULL },
  { "3253",  "+2% Threat and 10 Parry Rating",                            "",                               NULL,        NULL },
  { "3252",  "+8 All Stats",                                              "8All",                           NULL,        NULL },
  { "3247",  "Titanium Weapon Chain",                                     "28hit",                          NULL,        NULL },
  { "3249",  "+16 Critical Strike Rating",                                "16Crit",                         NULL,        NULL },
  { "3246",  "+28 Spell Power",                                           "28SP",                           NULL,        NULL },
  { "3245",  "+20 Resilience Rating",                                     "",                               NULL,        NULL },
  { "3244",  "Greater Vitality",                                          "6MP5",                           NULL,        NULL },
  { "3243",  "+35 Spell Penetration",                                     "35SPen",                         NULL,        NULL },
  { "3241",  "Lifeward",                                                  "",                               NULL,        NULL },
  { "3239",  "Icebreaker Weapon",                                         "",                               NULL,        NULL },
  { "3238",  "Gatherer",                                                  "",                               NULL,        NULL },
  { "3236",  "+200 Health",                                               "200Health",                      NULL,        NULL },
  { "3234",  "+20 Hit Rating",                                            "20Hit",                          NULL,        NULL },
  { "3233",  "+250 Mana",                                                 "250Mana",                        NULL,        NULL },
  { "3232",  "Tuskar's Vitality",                                         "15Sta",                          NULL,        NULL },
  { "3231",  "+15 Expertise Rating",                                      "15Exp",                          NULL,        NULL },
  { "3230",  "+20 Frost Resistance",                                      "",                               NULL,        NULL },
  { "3229",  "+12 Resilience Rating",                                     "",                               NULL,        NULL },
  { "3225",  "Executioner",                                               "executioner",                    NULL,        NULL },
  { "3223",  "Adamantite Weapon Chain",                                   "15parry",                        NULL,        NULL },
  { "3222",  "+20 Agility",                                               "20Agi",                          NULL,        NULL },
  { "3150",  "+6 mana every 5 sec.",                                      "6MP5",                           NULL,        NULL },
  { "3096",  "+17 Strength and +16 Intellect",                            "17Str_16Int",                    NULL,        NULL },
  { "3013",  "+40 Stamina and +12 Agility",                               "40Sta_12Agi",                    NULL,        NULL },
  { "3012",  "+50 Attack Power and +12 Critical Strike Rating",           "50AP_12Crit",                    NULL,        NULL },
  { "3011",  "+30 Stamina and +10 Agility",                               "30Sta_10Agi",                    NULL,        NULL },
  { "3010",  "+40 Attack Power and +10 Critical Strike Rating",           "40AP_10Crit",                    NULL,        NULL },
  { "3009",  "+20 Shadow Resistance",                                     "",                               NULL,        NULL },
  { "3008",  "+20 Frost Resistance",                                      "",                               NULL,        NULL },
  { "3007",  "+20 Fire Resistance",                                       "",                               NULL,        NULL },
  { "3006",  "+20 Arcane Resistance",                                     "",                               NULL,        NULL },
  { "3005",  "+20 Nature Resistance",                                     "",                               NULL,        NULL },
  { "3004",  "+18 Stamina and +20 Resilience Rating",                     "18Sta",                          NULL,        NULL },
  { "3003",  "+34 Attack Power and +16 Hit Rating",                       "34AP_16Hit",                     NULL,        NULL },
  { "3002",  "+22 Spell Power and +14 Hit Rating",                        "22SP_14Hit",                     NULL,        NULL },
  { "3001",  "+19 Spell Power and +7 Mana every 5 seconds",               "19SP_7MP5",                      NULL,        NULL },
  { "2999",  "+8 Stamina and +17 Dodge Rating",                           "8Sta_17Dodge",                   NULL,        NULL },
  { "2998",  "+7 All Resistances",                                        "",                               NULL,        NULL },
  { "2997",  "+15 Critical Strike Rating and +20 Attack Power",           "15Crit_20AP",                    NULL,        NULL },
  { "2996",  "+13 Critical Strike Rating",                                "13Crit",                         NULL,        NULL },
  { "2995",  "+15 Critical Strike Rating and +12 Spell Power",            "15Crit_12SP",                    NULL,        NULL },
  { "2994",  "+13 Critical Strike Rating",                                "13Crit",                         NULL,        NULL },
  { "2993",  "+12 Spell Power and 6 Mana every 5 seconds",                "12SP_6MP5",                      NULL,        NULL },
  { "2992",  "+5 Mana Regen",                                             "5MP5",                           NULL,        NULL },
  { "2991",  "+22 Stamina and +10 Dodge Rating",                          "22Sta_10Dodge",                  NULL,        NULL },
  { "2990",  "+13 Dodge Rating",                                          "13Dodge",                        NULL,        NULL },
  { "2989",  "+8 Arcane Resist",                                          "",                               NULL,        NULL },
  { "2988",  "+48 Stamina",                                               "48Sta",                          NULL,        NULL },
  { "2987",  "+8 Frost Resist",                                           "",                               NULL,        NULL },
  { "2986",  "+30 Attack Power and +10 Critical Strike Rating",           "30AP_10Crit",                    NULL,        NULL },
  { "2984",  "+8 Shadow Resist",                                          "",                               NULL,        NULL },
  { "2983",  "+26 Attack Power",                                          "26AP",                           NULL,        NULL },
  { "2982",  "+18 Spell Power and +10 Critical Strike Rating",            "18SP_10Crit",                    NULL,        NULL },
  { "2981",  "+15 Spell Power",                                           "15SP",                           NULL,        NULL },
  { "2980",  "+18 Spell Power and +4 Mana every 5 seconds",               "18SP_4MP5",                      NULL,        NULL },
  { "2979",  "+15 Spell Power",                                           "15SP",                           NULL,        NULL },
  { "2978",  "+15 Dodge Rating and +15 Stamina",                          "15Dodge_15Sta",                  NULL,        NULL },
  { "2977",  "+13 Dodge Rating",                                          "13Dodge",                        NULL,        NULL },
  { "2940",  "Minor Speed and +9 Stamina",                                "9Sta",                           NULL,        NULL },
  { "2939",  "Minor Speed and +6 Agility",                                "6Agi",                           NULL,        NULL },
  { "2938",  "+20 Spell Penetration",                                     "20SPen",                         NULL,        NULL },
  { "2937",  "+20 Spell Power",                                           "20SP",                           NULL,        NULL },
  { "2935",  "+15 Hit Rating",                                            "15Hit",                          NULL,        NULL },
  { "2934",  "+10 Critical Strike Rating",                                "10Crit",                         NULL,        NULL },
  { "2933",  "+15 Resilience Rating",                                     "",                               NULL,        NULL },
  { "2931",  "+4 All Stats",                                              "4All",                           NULL,        NULL },
  { "2930",  "+12 Spell Power",                                           "12SP",                           NULL,        NULL },
  { "2929",  "+2 Weapon Damage",                                          "",                               NULL,        NULL },
  { "2928",  "+12 Spell Power",                                           "12SP",                           NULL,        NULL },
  { "2841",  "+10 Stamina",                                               "10Sta",                          NULL,        NULL },
  { "2794",  "+3 Mana restored per 5 seconds",                            "3MP5",                           NULL,        NULL },
  { "2793",  "+21 Strength",                                              "21Str",                          NULL,        NULL },
  { "2792",  "+8 Stamina",                                                "8Sta",                           NULL,        NULL },
  { "2748",  "+35 Spell Power and +20 Stamina",                           "35SP_20Sta",                     NULL,        NULL },
  { "2747",  "+25 Spell Power and +15 Stamina",                           "25SP_15Sta",                     NULL,        NULL },
  { "2746",  "+35 Spell Power and +20 Stamina",                           "35SP_20Sta",                     NULL,        NULL },
  { "2745",  "+25 Spell Power and +15 Stamina",                           "25SP_15Sta",                     NULL,        NULL },
  { "2724",  "Scope (+28 Critical Strike Rating)",                        "28Crit",                         NULL,        NULL },
  { "2723",  "Scope (+12 Damage)",                                        "",                               NULL,        NULL },
  { "2722",  "Scope (+10 Damage)",                                        "",                               NULL,        NULL },
  { "2721",  "+15 Spell Power and +14 Critical Strike Rating",            "15SP_14Crit",                    NULL,        NULL },
  { "2717",  "+26 Attack Power and +14 Critical Strike Rating",           "26AP_14Crit",                    NULL,        NULL },
  { "2716",  "+16 Stamina and +100 Armor",                                "16Sta_100BArmor",                NULL,        NULL },
  { "2715",  "+16 Spell Power and 5 Mana every 5 seconds",                "16SP_5MP5",                      NULL,        NULL },
  { "2714",  "Felsteel Spike (26-38)",                                    "",                               NULL,        NULL },
  { "2683",  "+10 Shadow Resistance",                                     "",                               NULL,        NULL },
  { "2682",  "+10 Frost Resistance",                                      "",                               NULL,        NULL },
  { "2679",  "6 Mana per 5 Sec.",                                         "6MP5",                           NULL,        NULL },
  { "2675",  "Battlemaster",                                              "",                               NULL,        NULL },
  { "2674",  "Spellsurge",                                                "spellsurge",                     NULL,        NULL },
  { "2673",  "Mongoose",                                                  "mongoose",                       NULL,        NULL },
  { "2672",  "Soulfrost",                                                 "",                               NULL,        NULL },
  { "2671",  "Sunfire",                                                   "",                               NULL,        NULL },
  { "2670",  "+35 Agility",                                               "35Agi",                          NULL,        NULL },
  { "2669",  "+40 Spell Power",                                           "40SP",                           NULL,        NULL },
  { "2668",  "+20 Strength",                                              "20Str",                          NULL,        NULL },
  { "2667",  "Savagery",                                                  "70AP",                           NULL,        NULL },
  { "2666",  "+30 Intellect",                                             "30Int",                          NULL,        NULL },
  { "2664",  "+7 Resist All",                                             "",                               NULL,        NULL },
  { "2662",  "+120 Armor",                                                "120BArmor",                      NULL,        NULL },
  { "2661",  "+6 All Stats",                                              "6All",                           NULL,        NULL },
  { "2659",  "+150 Health",                                               "150Health",                      NULL,        NULL },
  { "2658",  "+10 Hit Rating and +10 Critical Strike Rating",             "10Hit_10Crit",                   NULL,        NULL },
  { "2657",  "+12 Agility",                                               "12Agi",                          NULL,        NULL },
  { "2656",  "Vitality",                                                  "4MP5",                           NULL,        NULL },
  { "2655",  "+15 Shield Block Rating",                                   "",                               NULL,        NULL },
  { "2654",  "+12 Intellect",                                             "12Int",                          NULL,        NULL },
  { "2653",  "+18 Block Value",                                           "18BlockV",                       NULL,        NULL },
  { "2650",  "+15 Spell Power",                                           "15SP",                           NULL,        NULL },
  { "2649",  "+12 Stamina",                                               "12Sta",                          NULL,        NULL },
  { "2648",  "+12 Dodge Rating",                                          "12Dodge",                        NULL,        NULL },
  { "2647",  "+12 Strength",                                              "12Str",                          NULL,        NULL },
  { "2646",  "+25 Agility",                                               "25Agi",                          NULL,        NULL },
  { "2622",  "+12 Dodge Rating",                                          "12Dodge",                        NULL,        NULL },
  { "2621",  "Subtlety",                                                  "",                               NULL,        NULL },
  { "2620",  "+15 Nature Resistance",                                     "",                               NULL,        NULL },
  { "2619",  "+15 Fire Resistance",                                       "",                               NULL,        NULL },
  { "2617",  "+16 Spell Power",                                           "16SP",                           NULL,        NULL },
  { "2616",  "+20 Fire Spell Damage",                                     "",                               NULL,        NULL },
  { "2615",  "+20 Frost Spell Damage",                                    "",                               NULL,        NULL },
  { "2614",  "+20 Shadow Spell Damage",                                   "",                               NULL,        NULL },
  { "2613",  "+2% Threat",                                                "",                               NULL,        NULL },
  { "2606",  "+30 Attack Power",                                          "30AP",                           NULL,        NULL },
  { "2605",  "+18 Spell Power",                                           "18SP",                           NULL,        NULL },
  { "2604",  "+18 Spell Power",                                           "18SP",                           NULL,        NULL },
  { "2603",  "Eternium Line",                                             "",                               NULL,        NULL },
  { "2591",  "+10 Intellect +10 Stamina +12 Spell Power",                 "10Int_10Sta_12SP",               NULL,        NULL },
  { "2590",  "+13 Spell Power +10 Stamina and +4 Mana every 5 seconds",   "10SP_10Sta_4MP5",                NULL,        NULL },
  { "2589",  "+18 Spell Power and +10 Stamina",                           "18SP_10Sta",                     NULL,        NULL },
  { "2588",  "+18 Spell Power and +8 Hit Rating",                         "18SP_8Hit",                      NULL,        NULL },
  { "2587",  "+13 Spell Power and +15 Intellect",                         "13SP_15Int",                     NULL,        NULL },
  { "2586",  "+24 Ranged Attack Power/+10 Stamina/+10 Hit Rating",        "_10Sta_10Hit",                   NULL,        NULL },
  { "2585",  "+28 Attack Power/+12 Dodge Rating",                         "28AP",                           NULL,        NULL },
  { "2584",  "+15 Dodge +10 Stamina and +12 Spell Power",                 "10Dodge_10Sta_12SP",             NULL,        NULL },
  { "2583",  "+10 Dodge Rating/+10 Stamina/+15 Block Value",              "10Dodge_10Sta",                  NULL,        NULL },
  { "2568",  "+22 Intellect",                                             "22Int",                          NULL,        NULL },
  { "2567",  "+20 Spirit",                                                "20Spi",                          NULL,        NULL },
  { "2566",  "+13 Spell Power",                                           "13SP",                           NULL,        NULL },
  { "2565",  "Mana Regen 4 per 5 sec.",                                   "4MP5",                           NULL,        NULL },
  { "2564",  "+15 Agility",                                               "15Agi",                          NULL,        NULL },
  { "2563",  "+15 Strength",                                              "15Str",                          NULL,        NULL },
  { "2545",  "+11 Stamina",                                               "11Sta",                          NULL,        NULL },
  { "2544",  "+8 Spell Power",                                            "8SP",                            NULL,        NULL },
  { "2543",  "+8 Agility",                                                "8Agi",                           NULL,        NULL },
  { "2523",  "+30 Ranged Hit Rating",                                     "",                               NULL,        NULL },
  { "2505",  "+29 Spell Power",                                           "29SP",                           NULL,        NULL },
  { "2504",  "+30 Spell Power",                                           "30SP",                           NULL,        NULL },
  { "2503",  "+5 Dodge Rating",                                           "5Dodge",                         NULL,        NULL },
  { "2488",  "+5 All Resistances",                                        "",                               NULL,        NULL },
  { "2487",  "+5 Shadow Resistance",                                      "",                               NULL,        NULL },
  { "2486",  "+5 Nature Resistance",                                      "",                               NULL,        NULL },
  { "2485",  "+5 Arcane Resistance",                                      "",                               NULL,        NULL },
  { "2484",  "+5 Frost Resistance",                                       "",                               NULL,        NULL },
  { "2483",  "+5 Fire Resistance",                                        "",                               NULL,        NULL },
  { "2463",  "+7 Fire Resistance",                                        "",                               NULL,        NULL },
  { "2443",  "+7 Frost Spell Damage",                                     "",                               NULL,        NULL },
  { "2381",  "+8 mana every 5 sec.",                                      "8MP5",                           NULL,        NULL },
  { "2376",  "+6 mana every 5 sec.",                                      "6MP5",                           NULL,        NULL },
  { "2343",  "+43 Spell Power",                                           "43SP",                           NULL,        NULL },
  { "2332",  "+30 Spell Power",                                           "30SP",                           NULL,        NULL },
  { "2326",  "+23 Spell Power",                                           "23SP",                           NULL,        NULL },
  { "2322",  "+19 Spell Power",                                           "19SP",                           NULL,        NULL },
  { "1953",  "+22 Dodge Rating",                                          "22Dodge",                        NULL,        NULL },
  { "1952",  "+20 Dodge Rating",                                          "20Dodge",                        NULL,        NULL },
  { "1951",  "+16 Dodge Rating",                                          "16Dodge",                        NULL,        NULL },
  { "1950",  "+15 Dodge Rating",                                          "15Dodge",                        NULL,        NULL },
  { "1904",  "+9 Intellect",                                              "9Int",                           NULL,        NULL },
  { "1903",  "+9 Spirit",                                                 "9Spi",                           NULL,        NULL },
  { "1900",  "Crusader",                                                  "",                               NULL,        NULL },
  { "1899",  "Unholy Weapon",                                             "",                               NULL,        NULL },
  { "1898",  "Lifestealing",                                              "",                               NULL,        NULL },
  { "1897",  "+5 Weapon Damage",                                          "",                               NULL,        NULL },
  { "1896",  "+9 Weapon Damage",                                          "",                               NULL,        NULL },
  { "1894",  "Icy Weapon",                                                "",                               NULL,        NULL },
  { "1893",  "+100 Mana",                                                 "100Mana",                        NULL,        NULL },
  { "1892",  "+100 Health",                                               "100Health",                      NULL,        NULL },
  { "1891",  "+4 All Stats",                                              "4All",                           NULL,        NULL },
  { "1890",  "+9 Spirit",                                                 "9Spi",                           NULL,        NULL },
  { "1889",  "+70 Armor",                                                 "70BArmor",                       NULL,        NULL },
  { "1888",  "+5 All Resistances",                                        "",                               NULL,        NULL },
  { "1887",  "+7 Agility",                                                "7Agi",                           NULL,        NULL },
  { "1886",  "+9 Stamina",                                                "9Sta",                           NULL,        NULL },
  { "1885",  "+9 Strength",                                               "9Str",                           NULL,        NULL },
  { "1884",  "+13 Intellect",                                             "13Int",                          NULL,        NULL },
  { "1883",  "+7 Intellect",                                              "7Int",                           NULL,        NULL },
  { "1843",  "Reinforced (+40 Armor)",                                    "40BArmor",                       NULL,        NULL },
  { "1704",  "+12 Resilience Rating",                                     "",                               NULL,        NULL },
  { "1606",  "+50 Attack Power",                                          "50AP",                           NULL,        NULL },
  { "1603",  "+44 Attack Power",                                          "44AP",                           NULL,        NULL },
  { "1600",  "+38 Attack Power",                                          "38AP",                           NULL,        NULL },
  { "1597",  "+32 Attack Power",                                          "32AP",                           NULL,        NULL },
  { "1594",  "+26 Attack Power",                                          "26AP",                           NULL,        NULL },
  { "1593",  "+24 Attack Power",                                          "24AP",                           NULL,        NULL },
  { "1510",  "+8 Spirit",                                                 "8Spi",                           NULL,        NULL },
  { "1509",  "+8 Intellect",                                              "8Int",                           NULL,        NULL },
  { "1508",  "+8 Agility",                                                "8Agi",                           NULL,        NULL },
  { "1507",  "+8 Stamina",                                                "8Sta",                           NULL,        NULL },
  { "1506",  "+8 Strength",                                               "8Str",                           NULL,        NULL },
  { "1505",  "+20 Fire Resistance",                                       "",                               NULL,        NULL },
  { "1504",  "+125 Armor",                                                "125BArmor",                      NULL,        NULL },
  { "1503",  "+100 HP",                                                   "100Health",                      NULL,        NULL },
  { "1483",  "+150 Mana",                                                 "150Mana",                        NULL,        NULL },
  { "1441",  "+15 Shadow Resistance",                                     "",                               NULL,        NULL },
  { "1400",  "+20 Nature Resistance",                                     "",                               NULL,        NULL },
  { "1354",  "+20 Fire Reistance",                                        "",                               NULL,        NULL },
  { "1257",  "+15 Arcane Resistance",                                     "",                               NULL,        NULL },
  { "1147",  "+18 Spirit",                                                "18Spi",                          NULL,        NULL },
  { "1144",  "+15 Spirit",                                                "15Spi",                          NULL,        NULL },
  { "1128",  "+25 Intellect",                                             "25Int",                          NULL,        NULL },
  { "1119",  "+16 Intellect",                                             "16Int",                          NULL,        NULL },
  { "1103",  "+26 Agility",                                               "26Agi",                          NULL,        NULL },
  { "1099",  "+22 Agility",                                               "22Agi",                          NULL,        NULL },
  { "1075",  "+22 Stamina",                                               "22Sta",                          NULL,        NULL },
  { "1071",  "+18 Stamina",                                               "18Sta",                          NULL,        NULL },
  {  "983",  "+16 Agility",                                               "16Agi",                          NULL,        NULL },
  {  "963",  "+7 Weapon Damage",                                          "",                               NULL,        NULL },
  {  "943",  "+3 Agility",                                                "3Agi",                           NULL,        NULL },
  {  "931",  "+10 Haste Rating",                                          "10Haste",                        NULL,        NULL },
  {  "930",  "+2% Mount Speed",                                           "",                               NULL,        NULL },
  {  "929",  "+7 Stamina",                                                "7Sta",                           NULL,        NULL },
  {  "928",  "+3 All Stats",                                              "3All",                           NULL,        NULL },
  {  "927",  "+7 Strength",                                               "7Str",                           NULL,        NULL },
  {  "925",  "+6 Intellect",                                              "6Int",                           NULL,        NULL },
  {  "924",  "+2 Dodge Rating",                                           "2Dodge",                         NULL,        NULL },
  {  "923",  "+5 Dodge Rating",                                           "5Dodge",                         NULL,        NULL },
  {  "913",  "+65 Mana",                                                  "65Mana",                         NULL,        NULL },
  {  "912",  "Demonslaying",                                              "",                               NULL,        NULL },
  {  "911",  "Minor Speed Increase",                                      "",                               NULL,        NULL },
  {  "910",  "Increased Stealth",                                         "",                               NULL,        NULL },
  {  "909",  "+5 Herbalism",                                              "",                               NULL,        NULL },
  {  "908",  "+14 Agility",                                               "14Agi",                          NULL,        NULL },
  {  "907",  "+7 Spirit",                                                 "5Spi",                           NULL,        NULL },
  {  "906",  "+5 Mining",                                                 "",                               NULL,        NULL },
  {  "905",  "+5 Intellect",                                              "5Int",                           NULL,        NULL },
  {  "904",  "+5 Agility",                                                "5Agi",                           NULL,        NULL },
  {  "903",  "+19 Shadow Spell Damage",                                   "",                               NULL,        NULL },
  {  "884",  "+50 Armor",                                                 "50BArmor",                       NULL,        NULL },
  {  "866",  "+2 All Stats",                                              "2All",                           NULL,        NULL },
  {  "865",  "+5 Skinning",                                               "",                               NULL,        NULL },
  {  "863",  "+10 Shield Block Rating",                                   "",                               NULL,        NULL },
  {  "857",  "+50 Mana",                                                  "50Mana",                         NULL,        NULL },
  {  "856",  "+5 Strength",                                               "5Str",                           NULL,        NULL },
  {  "854",  "+2 Stamina",                                                "2Sta",                           NULL,        NULL },
  {  "853",  "+6 Beastslaying",                                           "",                               NULL,        NULL },
  {  "852",  "+5 Stamina",                                                "5Sta",                           NULL,        NULL },
  {  "851",  "+5 Spirit",                                                 "5Spi",                           NULL,        NULL },
  {  "850",  "+35 Health",                                                "35Health",                       NULL,        NULL },
  {  "849",  "+3 Agility",                                                "3Agi",                           NULL,        NULL },
  {  "848",  "+30 Armor",                                                 "30BArmor",                       NULL,        NULL },
  {  "847",  "+0 Agility",                                                "",                               NULL,        NULL },
  {  "846",  "+0 Attack Power",                                           "",                               NULL,        NULL },
  {  "845",  "+2 Herbalism",                                              "",                               NULL,        NULL },
  {  "844",  "+2 Mining",                                                 "",                               NULL,        NULL },
  {  "843",  "+30 Mana",                                                  "30Mana",                         NULL,        NULL },
  {  "823",  "+7 Stamina",                                                "7Sta",                           NULL,        NULL },
  {  "805",  "+4 Weapon Damage",                                          "",                               NULL,        NULL },
  {  "804",  "+10 Shadow Resistance",                                     "",                               NULL,        NULL },
  {  "803",  "+9 Strength",                                               "9Str",                           NULL,        NULL },
  {  "783",  "+10 Armor",                                                 "10BArmor",                       NULL,        NULL },
  {  "744",  "+3 Intellect",                                              "3Int",                           NULL,        NULL },
  {  "724",  "+2 Intellect",                                              "2Int",                           NULL,        NULL },
  {  "723",  "+3 Intellect",                                              "3Int",                           NULL,        NULL },
  {  "684",  "+15 Strength",                                              "15Str",                          NULL,        NULL },
  {  "664",  "Scope (+7 Damage)",                                         "",                               NULL,        NULL },
  {  "663",  "Scope (+5 Damage)",                                         "",                               NULL,        NULL },
  {  "464",  "+4% Mount Speed",                                           "",                               NULL,        NULL },
  {  "463",  "Mithril Spike (16-20)",                                     "",                               NULL,        NULL },
  {  "369",  "+12 Intellect",                                             "12Int",                          NULL,        NULL },
  {  "368",  "+12 Agility",                                               "12Agi",                          NULL,        NULL },
  {  "256",  "+5 Fire Resistance",                                        "",                               NULL,        NULL },
  {  "255",  "+3 Spirit",                                                 "3Spi",                           NULL,        NULL },
  {  "254",  "+25 Health",                                                "25Health",                       NULL,        NULL },
  {  "250",  "+1  Weapon Damage",                                         "",                               NULL,        NULL },
  {  "249",  "+2 Beastslaying",                                           "",                               NULL,        NULL },
  {  "248",  "+2 Intellect",                                              "2Int",                           NULL,        NULL },
  {  "247",  "+1 Agility",                                                "1Agi",                           NULL,        NULL },
  {  "246",  "+20 Mana",                                                  "20Mana",                         NULL,        NULL },
  {  "243",  "+3 Intellect",                                              "3Int",                           NULL,        NULL },
  {  "242",  "+15 Health",                                                "15Health",                       NULL,        NULL },
  {  "241",  "+2 Weapon Damage",                                          "",                               NULL,        NULL },
  {   "66",  "+1 Stamina",                                                "1Sta",                           NULL,        NULL },
  {   "65",  "+1 All Resistances",                                        "",                               NULL,        NULL },
  {   "63",  "Absorption (25)",                                           "",                               NULL,        NULL },
  {   "44",  "+11 Intellect",                                             "11Int",                          NULL,        NULL },
  {   "43",  "+24 Intellect",                                             "24Int",                          NULL,        NULL },
  {   "41",  "+5 Health",                                                 "5Health",                        NULL,        NULL },
  {   "37",  "Counterweight (+20 Haste Rating)",                          "20Haste",                        NULL,        NULL },
  {   "36",  "Enchant: Fiery Blaze",                                      "",                               NULL,        NULL },
  {   "34",  "Counterweight (+20 Haste Rating)",                          "20Haste",                        NULL,        NULL },
  {   "33",  "Scope (+3 Damage)",                                         "",                               NULL,        NULL },
  {   "32",  "Scope (+2 Damage)",                                         "",                               NULL,        NULL },
  {   "30",  "Scope (+1 Damage)",                                         "",                               NULL,        NULL },
  {   "24",  "+5 Mana",                                                   "8Mana",                          NULL,        NULL },
  {   "18",  "+12 Agility",                                               "12Agi",                          NULL,        NULL },
  {   "17",  "Reinforced (+24 Armor)",                                    "24BArmor",                       NULL,        NULL },
  {   "16",  "Reinforced (+16 Armor)",                                    "16BArmor",                       NULL,        NULL },
  {   "15",  "Reinforced (+8 Armor)",                                     "8BArmor",                        NULL,        NULL },
  { NULL, NULL, NULL, NULL, NULL }
};

// Add-Ons use the same enchant data-base for now
static enchant_data_t* addon_db = enchant_db;

static const stat_type reforge_stats[] =
{
  STAT_SPIRIT,
  STAT_DODGE_RATING,
  STAT_PARRY_RATING,
  STAT_HIT_RATING,
  STAT_CRIT_RATING,
  STAT_HASTE_RATING,
  STAT_EXPERTISE_RATING,
  STAT_MASTERY_RATING,
  STAT_NONE
};

// Weapon Stat Proc Callback ================================================

struct weapon_stat_proc_callback_t : public action_callback_t
{
  weapon_t* weapon;
  buff_t* buff;
  double PPM;
  bool all_damage;

  weapon_stat_proc_callback_t( player_t* p, weapon_t* w, buff_t* b, double ppm=0.0, bool all=false ) :
    action_callback_t( p -> sim, p ), weapon( w ), buff( b ), PPM( ppm ), all_damage( all ) {}

  virtual void trigger( action_t* a, void* /* call_data */ )
  {
    if ( ! all_damage && a -> proc ) return;
    if ( weapon && a -> weapon != weapon ) return;

    if ( PPM > 0 )
    {
      buff -> trigger( 1, 0, weapon -> proc_chance_on_swing( PPM ) ); // scales with haste
    }
    else
    {
      buff -> trigger();
    }
    buff -> up();  // track uptime info
  }
};

// Weapon Discharge Proc Callback ===========================================

struct weapon_discharge_proc_callback_t : public action_callback_t
{
  std::string name_str;
  weapon_t* weapon;
  int stacks, max_stacks;
  double fixed_chance, PPM;
  cooldown_t* cooldown;
  spell_t* spell;
  proc_t* proc;
  rng_t* rng;

  weapon_discharge_proc_callback_t( const std::string& n, player_t* p, weapon_t* w, int ms, const school_type school, double dmg, double fc, double ppm=0, timespan_t cd=timespan_t::zero, int rng_type=RNG_DEFAULT ) :
    action_callback_t( p -> sim, p ),
    name_str( n ), weapon( w ), stacks( 0 ), max_stacks( ms ), fixed_chance( fc ), PPM( ppm )
  {
    if ( rng_type == RNG_DEFAULT ) rng_type = RNG_CYCLIC; // default is CYCLIC since discharge should not have duration

    struct discharge_spell_t : public spell_t
    {
      discharge_spell_t( const char* n, player_t* p, double dmg, const school_type s ) :
        spell_t( n, p, RESOURCE_NONE, ( s == SCHOOL_DRAIN ) ? SCHOOL_SHADOW : s )
      {
        trigger_gcd = timespan_t::zero;
        base_dd_min = dmg;
        base_dd_max = dmg;
        may_crit = ( s != SCHOOL_DRAIN );
        background  = true;
        proc = true;
        base_spell_power_multiplier = 0;
        init();
      }
    };

    cooldown = p -> get_cooldown( name_str );
    cooldown -> duration = cd;

    spell = new discharge_spell_t( name_str.c_str(), p, dmg, school );

    proc = p -> get_proc( name_str.c_str() );
    rng  = p -> get_rng ( name_str.c_str(), rng_type );
  }

  virtual void reset() { stacks=0; }

  virtual void deactivate() { action_callback_t::deactivate(); stacks=0; }

  virtual void trigger( action_t* a, void* /* call_data */ )
  {
    if ( a -> proc ) return;
    if ( weapon && a -> weapon != weapon ) return;

    if ( cooldown -> remains() > timespan_t::zero )
      return;

    double chance = fixed_chance;
    if ( weapon && PPM > 0 )
      chance = weapon -> proc_chance_on_swing( PPM ); // scales with haste

    if ( chance > 0 )
      if ( ! rng -> roll( chance ) )
        return;

    cooldown -> start();

    if ( ++stacks < max_stacks )
    {
      listener -> aura_gain( name_str.c_str() );
    }
    else
    {
      stacks = 0;
      spell -> execute();
      proc -> occur();
    }
  }
};

// register_synapse_springs =================================================

static void register_synapse_springs( item_t* item )
{
  player_t* p = item -> player;

  if ( p -> profession[ PROF_ENGINEERING ] < 425 )
  {
    item -> sim -> errorf( "Player %s attempting to use synapse springs without 425 in engineering.\n", p -> name() );
    return;
  }

  int attr[] = { ATTR_STRENGTH, ATTR_AGILITY, ATTR_INTELLECT, ATTRIBUTE_NONE };
  int stat[] = { STAT_STRENGTH, STAT_AGILITY, STAT_INTELLECT, STAT_NONE };

  int    max_stat  = STAT_INTELLECT;
  double max_value = -1;

  for ( int i=0; attr[ i ] != ATTRIBUTE_NONE; i++ )
  {
    if ( p -> attribute[ attr[ i ] ] > max_value )
    {
      max_value = p -> attribute[ attr[ i ] ];
      max_stat = stat[ i ];
    }
  }

  item -> use.name_str = "synapse_springs";
  item -> use.stat = max_stat;
  item -> use.stat_amount = 480.0;
  item -> use.duration = timespan_t::from_seconds(10.0);
  item -> use.cooldown = timespan_t::from_seconds(60.0);
}

// ==========================================================================
// Enchant
// ==========================================================================

// enchant_t::init ==========================================================

void enchant_t::init( player_t* p )
{
  if ( p -> is_pet() ) return;

  std::string& mh_enchant     = p -> items[ SLOT_MAIN_HAND ].encoded_enchant_str;
  std::string& oh_enchant     = p -> items[ SLOT_OFF_HAND  ].encoded_enchant_str;
  std::string& ranged_enchant = p -> items[ SLOT_RANGED    ].encoded_enchant_str;

  weapon_t* mhw = &( p -> main_hand_weapon );
  weapon_t* ohw = &( p -> off_hand_weapon );
  weapon_t* rw  = &( p -> ranged_weapon );

  if ( mh_enchant == "avalanche" || oh_enchant == "avalanche" )
  {
    // Reference: http://elitistjerks.com/f79/t110302-enhsim_cataclysm/p4/#post1832162
    if ( mh_enchant == "avalanche" )
    {
      action_callback_t* cb = new weapon_discharge_proc_callback_t( "avalanche_w", p, mhw, 1, SCHOOL_NATURE, 500, 0, 5.0/*PPM*/, timespan_t::from_seconds(0.01)/*CD*/ );
      p -> register_attack_callback( RESULT_HIT_MASK, cb );
    }
    if ( oh_enchant == "avalanche" )
    {
      action_callback_t* cb = new weapon_discharge_proc_callback_t( "avalanche_w", p, ohw, 1, SCHOOL_NATURE, 500, 0, 5.0/*PPM*/, timespan_t::from_seconds(0.01)/*CD*/ );
      p -> register_attack_callback( RESULT_HIT_MASK, cb );
    }
    action_callback_t* cb = new weapon_discharge_proc_callback_t( "avalanche_s", p, 0, 1, SCHOOL_NATURE, 500, 0.25/*FIXED*/, 0, timespan_t::from_seconds(10.0)/*CD*/ );
    p -> register_spell_callback ( RESULT_HIT_MASK, cb );
  }
  if ( mh_enchant == "berserking" )
  {
    buff_t* buff = new stat_buff_t( p, "berserking_mh", STAT_ATTACK_POWER, 400, 1, timespan_t::from_seconds(15), timespan_t::zero, 0, false, false, RNG_DISTRIBUTED );
    buff -> activated = false;
    p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, mhw, buff, 1.0/*PPM*/ ) );
  }
  if ( oh_enchant == "berserking" )
  {
    buff_t* buff = new stat_buff_t( p, "berserking_oh", STAT_ATTACK_POWER, 400, 1, timespan_t::from_seconds(15), timespan_t::zero, 0, false, false, RNG_DISTRIBUTED );
    buff -> activated = false;
    p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, ohw, buff, 1.0/*PPM*/ ) );
  }
  if ( mh_enchant == "elemental_slayer" )
  {
    // TO-DO
  }
  if ( oh_enchant == "elemental_slayer" )
  {
    // TO-DO
  }
  if ( mh_enchant == "executioner" || oh_enchant == "executioner" )
  {
    // MH-OH trigger/refresh the same Executioner buff.  It does not stack.

    buff_t* buff = new stat_buff_t( p, "executioner", STAT_CRIT_RATING, 120, 1, timespan_t::from_seconds(15), timespan_t::zero, 0, false, false, RNG_DISTRIBUTED );
    buff -> activated = false;

    if ( mh_enchant == "executioner" )
    {
      p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, mhw, buff, 1.0/*PPM*/ ) );
    }
    if ( oh_enchant == "executioner" )
    {
      p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, ohw, buff, 1.0/*PPM*/ ) );
    }
  }
  if ( mh_enchant == "hurricane" || oh_enchant == "hurricane" )
  {
    buff_t *mh_buff=0, *oh_buff=0;
    if ( mh_enchant == "hurricane" )
    {
      mh_buff = new stat_buff_t( p, "hurricane_mh", STAT_HASTE_RATING, 450, 1, timespan_t::from_seconds(12), timespan_t::zero, 0, false, false, RNG_DISTRIBUTED );
      mh_buff -> activated = false;
      p -> register_direct_damage_callback( SCHOOL_ATTACK_MASK, new weapon_stat_proc_callback_t( p, mhw, mh_buff, 1.0/*PPM*/, true/*ALL*/ ) );
      p -> register_tick_damage_callback  ( SCHOOL_ATTACK_MASK, new weapon_stat_proc_callback_t( p, mhw, mh_buff, 1.0/*PPM*/, true/*ALL*/ ) );
    }
    if ( oh_enchant == "hurricane" )
    {
      oh_buff = new stat_buff_t( p, "hurricane_oh", STAT_HASTE_RATING, 450, 1, timespan_t::from_seconds(12), timespan_t::zero, 0, false, false, RNG_DISTRIBUTED );
      oh_buff -> activated = false;
      p -> register_direct_damage_callback( SCHOOL_ATTACK_MASK, new weapon_stat_proc_callback_t( p, ohw, oh_buff, 1.0/*PPM*/, true /*ALL*/ ) );
      p -> register_tick_damage_callback  ( SCHOOL_ATTACK_MASK, new weapon_stat_proc_callback_t( p, ohw, oh_buff, 1.0/*PPM*/, true /*ALL*/ ) );
    }
    // Custom proc is required for spell damage procs.
    // If MH buff is up, then refresh it, else
    // IF OH buff is up, then refresh it, else
    // Trigger a new buff not associated with either MH or OH
    // This means that it is possible to have three stacks
    struct hurricane_spell_proc_callback_t : public action_callback_t
    {
      buff_t *mh_buff, *oh_buff, *s_buff;
      hurricane_spell_proc_callback_t( player_t* p, buff_t* mhb, buff_t* ohb, buff_t* sb ) :
        action_callback_t( p -> sim, p ), mh_buff( mhb ), oh_buff( ohb ), s_buff( sb )
      {
      }
      virtual void trigger( action_t* /* a */, void* /* call_data */ )
      {
        if ( s_buff -> cooldown -> remains() > timespan_t::zero ) return;
        if ( ! s_buff -> rng -> roll( 0.15 ) ) return;
        if ( mh_buff && mh_buff -> check() )
        {
          mh_buff -> trigger();
          s_buff -> cooldown -> start();
        }
        else if ( oh_buff && oh_buff -> check() )
        {
          oh_buff -> trigger();
          s_buff -> cooldown -> start();
        }
        else s_buff -> trigger();
      }
    };
    buff_t* s_buff = new stat_buff_t( p, "hurricane_s", STAT_HASTE_RATING, 450, 1, timespan_t::from_seconds(12), timespan_t::from_seconds(45.0) );
    s_buff -> activated = false;
    p -> register_direct_damage_callback( SCHOOL_SPELL_MASK, new hurricane_spell_proc_callback_t( p, mh_buff, oh_buff, s_buff ) );
    p -> register_tick_damage_callback  ( SCHOOL_SPELL_MASK, new hurricane_spell_proc_callback_t( p, mh_buff, oh_buff, s_buff ) );
    p -> register_direct_heal_callback( SCHOOL_SPELL_MASK, new hurricane_spell_proc_callback_t( p, mh_buff, oh_buff, s_buff ) );
    p -> register_tick_heal_callback  ( SCHOOL_SPELL_MASK, new hurricane_spell_proc_callback_t( p, mh_buff, oh_buff, s_buff ) );
  }
  if ( mh_enchant == "landslide" )
  {
    buff_t* buff = new stat_buff_t( p, "landslide_mh", STAT_ATTACK_POWER, 1000, 1, timespan_t::from_seconds(12), timespan_t::zero, 0, false, false, RNG_DISTRIBUTED );
    buff -> activated = false;
    p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, mhw, buff, 1.0/*PPM*/ ) );
  }
  if ( oh_enchant == "landslide" )
  {
    buff_t* buff = new stat_buff_t( p, "landslide_oh", STAT_ATTACK_POWER, 1000, 1, timespan_t::from_seconds(12), timespan_t::zero, 0, false, false, RNG_DISTRIBUTED );
    buff -> activated = false;
    p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, ohw, buff, 1.0/*PPM*/ ) );
  }
  if ( mh_enchant == "mongoose" )
  {
    p -> buffs.mongoose_mh = new stat_buff_t( p, "mongoose_main_hand", STAT_AGILITY, 120, 1, timespan_t::from_seconds(15), timespan_t::zero, 0, false, false, RNG_DISTRIBUTED );
    p -> buffs.mongoose_mh -> activated = false;
    p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, mhw, p -> buffs.mongoose_mh, 1.0/*PPM*/ ) );
  }
  if ( oh_enchant == "mongoose" )
  {
    p -> buffs.mongoose_oh = new stat_buff_t( p, "mongoose_off_hand" , STAT_AGILITY, 120, 1, timespan_t::from_seconds(15), timespan_t::zero, 0, false, false, RNG_DISTRIBUTED );
    p -> buffs.mongoose_oh -> activated = false;
    p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, ohw, p -> buffs.mongoose_oh, 1.0/*PPM*/ ) );
  }
  if ( mh_enchant == "power_torrent" )
  {
    buff_t* buff = new stat_buff_t( p, "power_torrent_mh", STAT_INTELLECT, 500, 1, timespan_t::from_seconds(12), timespan_t::from_seconds(45), 0.20, false, false, RNG_DISTRIBUTED );
    buff -> activated = false;
    weapon_stat_proc_callback_t* cb = new weapon_stat_proc_callback_t( p, NULL, buff );
    p -> register_tick_damage_callback  ( RESULT_ALL_MASK, cb );
    p -> register_direct_damage_callback( RESULT_ALL_MASK, cb );
    p -> register_tick_heal_callback    ( RESULT_ALL_MASK, cb );
    p -> register_direct_heal_callback  ( RESULT_ALL_MASK, cb );
  }
  if ( oh_enchant == "power_torrent" )
  {
    buff_t* buff = new stat_buff_t( p, "power_torrent_oh", STAT_INTELLECT, 500, 1, timespan_t::from_seconds(12), timespan_t::from_seconds(45), 0.20, false, false, RNG_DISTRIBUTED );
    buff -> activated = false;
    weapon_stat_proc_callback_t* cb = new weapon_stat_proc_callback_t( p, NULL, buff );
    p -> register_tick_damage_callback  ( RESULT_ALL_MASK, cb );
    p -> register_direct_damage_callback( RESULT_ALL_MASK, cb );
    p -> register_tick_heal_callback    ( RESULT_ALL_MASK, cb );
    p -> register_direct_heal_callback  ( RESULT_ALL_MASK, cb );
  }
  if ( mh_enchant == "windwalk" )
  {
    buff_t* buff = new stat_buff_t( p, "windwalk_mh", STAT_DODGE_RATING, 600, 1, timespan_t::from_seconds(10), timespan_t::from_seconds(45), 0.15, false, false, RNG_DISTRIBUTED );
    buff -> activated = false;
    p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, mhw, buff ) );
  }
  if ( oh_enchant == "windwalk" )
  {
    buff_t* buff = new stat_buff_t( p, "windwalk_oh", STAT_DODGE_RATING, 600, 1, timespan_t::from_seconds(10), timespan_t::from_seconds(45), 0.15, false, false, RNG_DISTRIBUTED );
    buff -> activated = false;
    p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, ohw, buff ) );
  }
  if ( ranged_enchant == "gnomish_xray" )
  {
    //FIXME: 1.0 ppm and 40 second icd seems to roughly match in-game behavior, but we need to verify the exact mechanics
    buff_t* buff = new stat_buff_t( p, "xray_targeting", STAT_ATTACK_POWER, 800, 1, timespan_t::from_seconds(10), timespan_t::from_seconds(40), 0, false, false, RNG_DISTRIBUTED, 95712 );
    buff -> activated = false;
    p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, rw, buff, 1.0/*PPM*/ ) );
  }
  if ( p -> meta_gem == META_THUNDERING_SKYFIRE )
  {
    //FIXME: 0.2 ppm and 40 second icd seems to roughly match in-game behavior, but we need to verify the exact mechanics
    buff_t* buff = new stat_buff_t( p, "skyfire_swiftness", STAT_HASTE_RATING, 240, 1, timespan_t::from_seconds(6), timespan_t::from_seconds(40), 0, false, false, RNG_DISTRIBUTED, 39959 );
    buff -> activated = false;
    p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, mhw, buff, 0.2/*PPM*/ ) );
    p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, ohw, buff, 0.2/*PPM*/ ) );
    p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, rw,  buff, 0.2/*PPM*/ ) );
  }
  if ( p -> meta_gem == META_THUNDERING_SKYFLARE )
  {
    //FIXME: 0.2 ppm and 40 second icd seems to roughly match in-game behavior, but we need to verify the exact mechanics
    buff_t* buff = new stat_buff_t( p, "skyflare_swiftness", STAT_HASTE_RATING, 480, 1, timespan_t::from_seconds(6), timespan_t::from_seconds(40), 0, false, false, RNG_DISTRIBUTED, 55379 );
    buff -> activated = false;
    p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, mhw, buff, 0.2/*PPM*/ ) );
    p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, ohw, buff, 0.2/*PPM*/ ) );
    p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, rw,  buff, 0.2/*PPM*/ ) );
  }

  int num_items = ( int ) p -> items.size();
  for ( int i=0; i < num_items; i++ )
  {
    item_t& item = p -> items[ i ];

    if ( item.enchant.stat && item.enchant.school )
    {
      unique_gear_t::register_stat_discharge_proc( item, item.enchant );
    }
    else if ( item.enchant.stat )
    {
      unique_gear_t::register_stat_proc( item, item.enchant );
    }
    else if ( item.enchant.school )
    {
      unique_gear_t::register_discharge_proc( item, item.enchant );
    }
    else if ( item.encoded_enchant_str == "synapse_springs" )
    {
      register_synapse_springs( &item );
      item.unique_enchant = true;
    }
    else if ( item.encoded_addon_str == "synapse_springs" )
    {
      register_synapse_springs( &item );
      item.unique_addon = true;
    }
  }
}

// enchant_t::get_encoding ==================================================

bool enchant_t::get_encoding( std::string& name,
                              std::string& encoding,
                              const std::string& enchant_id,
                              const bool ptr )
{
  for ( int i=0; enchant_db[ i ].id; i++ )
  {
    enchant_data_t& enchant = enchant_db[ i ];

    if ( enchant_id == enchant.id )
    {
      if ( ptr && enchant.ptr_name && enchant.ptr_encoding )
      {
        name     = enchant.ptr_name;
        encoding = enchant.ptr_encoding;
      }
      else
      {
        name     = enchant.name;
        encoding = enchant.encoding;
      }
      return true;
    }
  }
  return false;
}

// enchant_t::get_addon_encoding ============================================

bool enchant_t::get_addon_encoding( std::string& name,
                                    std::string& encoding,
                                    const std::string& addon_id,
                                    const bool ptr )
{
  for ( int i=0; addon_db[ i ].id; i++ )
  {
    enchant_data_t& addon = addon_db[ i ];

    if ( addon_id == addon.id )
    {
      if ( ptr && addon.ptr_name && addon.ptr_encoding )
      {
        name     = addon.ptr_name;
        encoding = addon.ptr_encoding;
      }
      else
      {
        name     = addon.name;
        encoding = addon.encoding;
      }
      return true;
    }
  }
  return false;
}

// enchant_t::get_reforge_encoding ==========================================

bool enchant_t::get_reforge_encoding( std::string& name,
                                      std::string& encoding,
                                      const std::string& reforge_id )
{
  name.clear();
  encoding.clear();

  if ( reforge_id.empty() || reforge_id == "0" )
    return true;

  int start = 0;
  int target = atoi( reforge_id.c_str() );
  target %= 56;
  if ( target == 0 ) target = 56;
  else if ( target <= start ) return false;

  for ( int i=0; reforge_stats[ i ] != STAT_NONE; i++ )
  {
    for ( int j=0; reforge_stats[ j ] != STAT_NONE; j++ )
    {
      if ( i == j ) continue;
      if ( ++start == target )
      {
        std::string source_stat = util_t::stat_type_abbrev( reforge_stats[ i ] );
        std::string target_stat = util_t::stat_type_abbrev( reforge_stats[ j ] );

        name += "Reforge " + source_stat + " to " + target_stat;
        encoding = source_stat + "_" + target_stat;

        return true;
      }
    }
  }

  return false;
}

// enchant_t::get_reforge_id ================================================

int enchant_t::get_reforge_id( stat_type stat_from,
                               stat_type stat_to )
{
  int index_from;
  for ( index_from=0; reforge_stats[ index_from ] != STAT_NONE; index_from++ )
    if ( reforge_stats[ index_from ] == stat_from )
      break;

  int index_to;
  for ( index_to=0; reforge_stats[ index_to ] != STAT_NONE; index_to++ )
    if ( reforge_stats[ index_to ] == stat_to )
      break;

  int id=0;
  for ( int i=0; reforge_stats[ i ] != STAT_NONE; i++ )
  {
    for ( int j=0; reforge_stats[ j ] != STAT_NONE; j++ )
    {
      if ( i == j ) continue;
      id++;
      if ( index_from == i &&
          index_to   == j )
      {
        return id;
      }
    }
  }

  return 0;
}

// enchant_t::download ======================================================

bool enchant_t::download( item_t&            item,
                          const std::string& enchant_id )
{
  item.armory_enchant_str.clear();

  if ( enchant_id.empty() || enchant_id == "0" )
    return true;

  std::string description;
  if ( get_encoding( description, item.armory_enchant_str, enchant_id, item.ptr() ) )
  {
    armory_t::format( item.armory_enchant_str );
    return true;
  }

  return false;
}

// enchant_t::download_addon ================================================

bool enchant_t::download_addon( item_t&            item,
                                const std::string& addon_id )
{
  item.armory_addon_str.clear();

  if ( addon_id.empty() || addon_id == "0" )
    return true;

  std::string description;
  if ( get_addon_encoding( description, item.armory_addon_str, addon_id, item.ptr() ) )
  {
    armory_t::format( item.armory_addon_str );
    return true;
  }

  return false;
}

// enchant_t::download_reforge ==============================================

bool enchant_t::download_reforge( item_t&            item,
                                  const std::string& reforge_id )
{
  item.armory_reforge_str.clear();

  if ( reforge_id.empty() || reforge_id == "0" )
    return true;

  std::string description;
  if ( get_reforge_encoding( description, item.armory_reforge_str, reforge_id ) )
  {
    armory_t::format( item.armory_reforge_str );
    return true;
  }

  return false;
}

// enchant_t::download_rsuffix ==============================================

bool enchant_t::download_rsuffix( item_t&            item,
                                  const std::string& rsuffix_id )
{
  item.armory_random_suffix_str = rsuffix_id;
  return true;
}
