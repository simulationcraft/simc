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
};

// This data is originally from http://armory-musings.appspot.com/ and published under a Creative Commons Attribution 3.0 License.

static enchant_data_t enchant_db[] =
{
  { "4258",  "+50 Agility",                                               "50agi"                          },
  { "4257",  "+50 Intellect",                                             "50int"                          },
  { "4256",  "+50 Strength",                                              "50str"                          },
  { "4227",  "+130 Agility",                                              "130agi"                         },
  { "4223",  "Greatly increase your run speed for 5 sec.",                ""                               },
  { "4217",  "Pyrium Weapon Chain",                                       ""                               },
  { "4214",  "Summons a Cardboard Assassin to draw the attention of enemies", ""                           },
  { "4209",  "+60 Agility and 35 Haste rating",                           "60agi_35haste"                  },
  { "4208",  "+60 Strength and 35 Mastery rating",                        "60str_35mastery"                },
  { "4207",  "+60 Intellect and 35 Critical Strike rating",               "60int_35crit"                   },
  { "4206",  "+90 Stamina and 35 Dodge rating",                           "90sta_35dodge"                  },
  { "4205",  "+30 Agility and +20 Mastery Rating",                        "30agi_20mastery"                },
  { "4204",  "+50 Agility and +25 Mastery Rating",                        "50agi_25mastery"                },
  { "4203",  "+30 Agility and +20 Mastery Rating",                        "30agi_20mastery"                },
  { "4202",  "+50 Strength and +25 Critical Strike Rating",               "50str_25crit"                   },
  { "4201",  "+30 Strength and +20 Critical Strike Rating",               "30str_20crit"                   },
  { "4200",  "+50 Intellect and 25 Haste Rating",                         "50int_25haste"                  },
  { "4199",  "+30 Intellect and 20 Haste Rating",                         "30int_20haste"                  },
  { "4198",  "+75 Stamina and 25 Dodge Rating",                           "75sta_25dodge"                  },
  { "4197",  "+45 Stamina and 20 Dodge Rating",                           "45sta_20dodge"                  },
  { "4196",  "+130 Intellect and 25 Haste Rating",                        "130int_25haste"                 },
  { "4195",  "+195 Stamina and 25 Dodge Rating",                          "195sta_25dodge"                 },
  { "4194",  "+130 Strength and 25 Critical Strike Rating",               "130str_25crit"                  },
  { "4193",  "+130 Agility and +25 Mastery Rating",                       "130agi_25mastery"               },
  { "4192",  "+130 Intellect",                                            "130int"                         },
  { "4191",  "+130 Strength",                                             "130str"                         },
  { "4190",  "+130 Agility",                                              "130agi"                         },
  { "4189",  "+195 Stamina",                                              "195sta"                         },
  { "4188",  "Grounded Plasma Shield",                                    ""                               },
  { "4187",  "Invisibility Field",                                        ""                               },
  { "4182",  "Spinal Healing Injector",                                   ""                               },
  { "4181",  "Tazik Shocker",                                             "tazik_shocker"                  },
  { "4180",  "Quickflip Deflection Plates",                               "quickflip_deflection_plates"    },
  { "4179",  "Synapse Springs",                                           "synapse_springs"                },
  { "4176",  "R19 Threatfinder",                                          "88Hit"                          },
  { "4175",  "Gnomish X-Ray Scope",                                       "gnomish_xray"                   },
  { "4127",  "+145 Stamina and +55 Agility",                              "145sta_55agi"                   },
  { "4126",  "+190 Attack Power and +55 Crit Rating",                     "190AP_55Crit"                   },
  { "4124",  "+85 Stamina and +45 Agility",                               "85sta_45agi"                    },
  { "4122",  "+110 Attack Power and +45 Crit Rating",                     "110AP_45Crit"                   },
  { "4121",  "+44 Stamina",                                               "44sta"                          },
  { "4120",  "+36 Stamina",                                               "36sta"                          },
  { "4118",  "Swordguard Embroidery",                                     "swordguard_embroidery"          },
  { "4116",  "Darkglow Embroidery",                                       "darkglow_embroidery"            },
  { "4115",  "Lightweave Embroidery",                                     "lightweave_embroidery"          },
  { "4114",  "+95 Intellect and +55 Spirit",                              "95int_55spi"                    },
  { "4113",  "+95 Intellect and +80 Stamina",                             "95int_80sta"                    },
  { "4112",  "+95 Intellect and +80 Stamina",                             "95int_80sta"                    },
  { "4111",  "+55 Intellect and +65 Stamina",                             "55int_65sta"                    },
  { "4110",  "+95 Intellect and +55 Spirit",                              "95int_55spi"                    },
  { "4109",  "+55 Intellect and +45 Spirit",                              "55int_45spi"                    },
  { "4108",  "+65 Haste Rating",                                          "65haste"                        },
  { "4107",  "+65 Mastery Rating",                                        "65mastery"                      },
  { "4106",  "+50 Strength",                                              "50str"                          },
  { "4105",  "+25 Agility and Minor Movement Speed",                      "25agi"                          },
  { "4104",  "+35 Mastery Rating and Minor Movement Speed",               "35mastery"                      },
  { "4103",  "+75 Stamina",                                               "75sta"                          },
  { "4102",  "+20 All Stats",                                             "20all"                          },
  { "4101",  "+65 Critical Strike Rating",                                "65crit"                         },
  { "4100",  "+65 Critical Strike Rating",                                "65crit"                         },
  { "4099",  "Landslide",                                                 "landslide"                      },
  { "4098",  "Windwalk",                                                  "windwalk"                       },
  { "4097",  "Power Torrent",                                             "power_torrent"                  },
  { "4096",  "+50 Intellect",                                             "50int"                          },
  { "4095",  "+50 Expertise Rating",                                      "50exp"                          },
  { "4094",  "+50 Mastery Rating",                                        "50mastery"                      },
  { "4093",  "+50 Spirit",                                                "50spi"                          },
  { "4092",  "+50 Hit Rating",                                            "50hit"                          },
  { "4091",  "+40 Intellect",                                             "40int"                          },
  { "4090",  "+250 Armor",                                                "250Barmor"                      },
  { "4089",  "+50 Hit Rating",                                            "50hit"                          },
  { "4088",  "+40 Spirit",                                                "40spi"                          },
  { "4087",  "+50 Critical Strike Rating",                                "50crit"                         },
  { "4086",  "+50 Dodge Rating",                                          "50dodge"                        },
  { "4085",  "+40 Block Rating",                                          ""                               },
  { "4084",  "Heartsong",                                                 "heartsong"                      },
  { "4083",  "Hurricane",                                                 "hurricane"                      },
  { "4082",  "+50 Expertise Rating",                                      "50exp"                          },
  { "4081",  "+60 Stamina",                                               "60sta"                          },
  { "4080",  "+40 Intellect",                                             "40int"                          },
  { "4079",  "+40 Agility",                                               "40agi"                          },
  { "4078",  "+40 Strength",                                              "40str"                          },
  { "4077",  "+40 Resilience Rating",                                     "40resil"                        },
  { "4076",  "+35 Agility",                                               "35agi"                          },
  { "4075",  "+35 Strength",                                              "35str"                          },
  { "4074",  "Elemental Slayer",                                          "elemental_slayer"               },
  { "4073",  "+160 Armor",                                                "160Barmor"                      },
  { "4072",  "+30 Intellect",                                             "30int"                          },
  { "4071",  "+50 Critical Strike Rating",                                "50crit"                         },
  { "4070",  "+55 Stamina",                                               "55sta"                          },
  { "4069",  "+50 Haste Rating",                                          "50haste"                        },
  { "4068",  "+50 Haste Rating",                                          "50haste"                        },
  { "4067",  "Avalanche",                                                 "avalanche"                      },
  { "4066",  "Mending",                                                   ""                               },
  { "4065",  "+50 Haste Rating",                                          "50haste"                        },
  { "4064",  "+70 Spell Penetration",                                     "70spen"                         },
  { "4063",  "+15 All Stats",                                             "15all"                          },
  { "4062",  "+30 Stamina and Minor Movement Speed",                      "30sta"                          },
  { "4061",  "+50 Mastery",                                               "50mastery"                      },
  { "3878",  "Mind Amplification Dish",                                   ""                               },
  { "3876",  "Inscription of the Pinnacle",                               "15Dodge_15Sta"                  },
  { "3875",  "+30 Attack Power and +10 Crit Rating",                      "30AP_10Crit"                    },
  { "3873",  "+50 Spell Power and +30 Stamina",                           "50SP_30Sta"                     },
  { "3872",  "+50 Spell Power and +20 Spirit",                            "50SP_20Spi"                     },
  { "3870",  "Blood Draining",                                            ""                               },
  { "3869",  "Blade Ward",                                                ""                               },
  { "3860",  "+250 Armor",                                                "250BArmor"                      },
  { "3859",  "Springy Arachnoweave",                                      ""                               },
  { "3855",  "+69 Spell Power",                                           "69SP"                           },
  { "3854",  "+81 Spell Power",                                           "81SP"                           },
  { "3853",  "+40 Resilience Rating +28 Stamina",                         "28Sta"                          },
  { "3852",  "+30 Stamina and +15 Resilience Rating",                     "30Sta"                          },
  { "3851",  "+50 Stamina",                                               "50Sta"                          },
  { "3850",  "+40 Stamina",                                               "40Sta"                          },
  { "3849",  "Titanium Plating",                                          ""                               },
  { "3847",  "Rune of the Stoneskin Gargoyle",                            ""                               },
  { "3846",  "+40 Spell Power",                                           "40SP"                           },
  { "3845",  "+50 Attack Power",                                          "50AP"                           },
  { "3844",  "+45 Spirit",                                                "45Spi"                          },
  { "3843",  "Scope (+15 Damage)",                                        ""                               },
  { "3842",  "+37 Stamina and +20 Dodge",                                 "37Sta_20Dodge"                  },
  { "3840",  "+23 Spell Power",                                           "23SP"                           },
  { "3839",  "+40 Attack Power",                                          "40AP"                           },
  { "3838",  "+70 Spell Power and +15 Crit Rating",                       "70SP_15Crit"                    },
  { "3837",  "+60 Dodge Rating and +22 Stamina"       ,                   "60Dodge_22Sta"                  },
  { "3836",  "+70 Spell Power and +6 Mana/5 seconds",                     "70SP_6MP5"                      },
  { "3835",  "+120 Attack Power and +15 Crit Rating",                     "120AP_15Crit"                   },
  { "3834",  "+63 Spell Power",                                           "63SP"                           },
  { "3833",  "+65 Attack Power",                                          "65AP"                           },
  { "3832",  "+10 All Stats",                                             "10all"                          },
  { "3831",  "+23 Haste Rating",                                          "23Haste"                        },
  { "3830",  "+50 Spell Power",                                           "50SP"                           },
  { "3829",  "+35 Attack Power",                                          "35AP"                           },
  { "3828",  "+85 Attack Power",                                          "85AP"                           },
  { "3827",  "+110 Attack Power",                                         "110AP"                          },
  { "3826",  "Icewalker",                                                 "12Crit_12Hit"                   },
  { "3825",  "+15 Haste Rating",                                          "15Haste"                        },
  { "3824",  "+24 Attack Power",                                          "24AP"                           },
  { "3823",  "+75 Attack Power and +22 Critical Strike Rating",           "75AP_22Crit"                    },
  { "3822",  "+55 Stamina and +22 Agility",                               "55Sta_22Agi"                    },
  { "3820",  "+30 Spell Power and 20 Critical strike rating.",            "30SP_20Crit"                    },
  { "3819",  "+30 Spell Power and 8 mana per 5 seconds.",                 "30SP_8MP5"                      },
  { "3818",  "+37 Stamina and +20 Dodge",                                 "37Sta_20Dodge"                  },
  { "3817",  "+50 Attack Power and +20 Critical Strike Rating",           "50AP_20Crit"                    },
  { "3816",  "+25 Fire Resistance and 30 Stamina",                        "30Sta"                          },
  { "3815",  "+25 Arcane Resistance and 30 Stamina",                      "30Sta"                          },
  { "3814",  "+25 Shadow Resistance and 30 Stamina",                      "30Sta"                          },
  { "3813",  "+25 Nature Resistance and 30 Stamina",                      "30Sta"                          },
  { "3812",  "+25 Frost Resistance and 30 Stamina",                       "30Sta"                          },
  { "3811",  "+20 Dodge Rating and +22 Stamina",                          "20Dodge_22Sta"                  },
  { "3810",  "+24 Spell Power and +15 Critical Strike Rating",            "24SP_15Crit"                    },
  { "3809",  "+24 Spell Power and +6 Mana/5 seconds",                     "24SP_6MP5"                      },
  { "3808",  "+40 Attack Power and +15 Crit Rating",                      "40AP_15Crit"                    },
  { "3807",  "+18 Spell Power and +5 Mana/5 seconds",                     "18SP_5MP5"                      },
  { "3806",  "+18 Spell Power and +10 Critical Strike Rating",            "18SP_10Crit"                    },
  { "3797",  "+29 Spell Power and +20 Resilience Rating",                 "29SP"                           },
  { "3795",  "+50 Attack Power and +20 Resilience Rating",                "50AP"                           },
  { "3794",  "+23 Spell Power and +15 Resilience Rating",                 "23SP"                           },
  { "3793",  "+40 Attack Power and +15 Resilience Rating",                "45AP"                           },
  { "3791",  "+30 Stamina",                                               "30Sta"                          },
  { "3790",  "Black Magic",                                               "black_magic"                    },
  { "3789",  "Berserking",                                                "berserking"                     },
  { "3788",  "Accuracy",                                                  "25Hit_25Crit"                   },
  { "3761",  "+70 Shadow Resistance",                                     ""                               },
  { "3760",  "+60 Frost Resistance",                                      ""                               },
  { "3758",  "+76 Spell Power",                                           "76SP"                           },
  { "3757",  "+102 Stamina",                                              "102Sta"                         },
  { "3756",  "+130 Attack Power",                                         "130AP"                          },
  { "3755",  "+28 Attack Power +12 Dodge Rating",                         "28AP_12Dodge"                   },
  { "3754",  "+24 Attack Power/+10 Stamina/+10 Hit Rating",               "24AP_10Sta_10Hit"               },
  { "3748",  "Titanium Spike (45-67)",                                    ""                               },
  { "3731",  "Titanium Weapon Chain",                                     ""                               },
  { "3730",  "Swordguard Embroidery",                                     "swordguard_embroidery_old"      },
  { "3728",  "Darkglow Embroidery",                                       "darkglow_embroidery_old"        },
  { "3722",  "Lightweave Embroidery",                                     "lightweave_embroidery_old"      },
  { "3721",  "+50 Spell Power and +30 Stamina",                           "50SP_30Sta"                     },
  { "3720",  "+35 Spell Power and +20 Stamina",                           "35SP_20Sta"                     },
  { "3719",  "+50 Spell Power and +20 Spirit",                            "50SP_20Spi"                     },
  { "3718",  "+35 Spell Power and +12 Spirit",                            "35SP_12Spi"                     },
  { "3608",  "+40 Ranged Critical Strike",                                "40Crit"                         },
  { "3607",  "+40 Ranged Haste Rating",                                   "40Haste"                        },
  { "3606",  "Nitro Boosts",                                              ""                               },
  { "3605",  "Flexweave Underlay",                                        ""                               },
  { "3604",  "Hyperspeed Accelerators",                                   "hyperspeed_accelerators"        },
  { "3603",  "Hand-Mounted Pyro Rocket",                                  "hand_mounted_pyro_rocket"       },
  { "3601",  "Belt-Clipped Spynoculars",                                  ""                               },
  { "3599",  "Electromagnetic Pulse Generator",                           ""                               },
  { "3595",  "Rune of Spellbreaking",                                     ""                               },
  { "3594",  "Rune of Swordbreaking",                                     ""                               },
  { "3370",  "Rune of Razorice",                                          "rune_of_razorice"               },
  { "3369",  "Rune of Cinderglacier",                                     "rune_of_cinderglacier"          },
  { "3368",  "Rune of the Fallen Crusader",                               "rune_of_the_fallen_crusader"    },
  { "3367",  "Rune of Spellshattering",                                   ""                               },
  { "3366",  "Rune of Lichbane",                                          ""                               },
  { "3365",  "Rune of Swordshattering",                                   ""                               },
  { "3330",  "+18 Stamina",                                               "18Sta"                          },
  { "3329",  "+12 Stamina",                                               "12Sta"                          },
  { "3328",  "+75 Attack Power and +22 Critical Strike Rating",           "75AP_22Crit"                    },
  { "3327",  "+75 Attack Power and +22 Critical Strike Rating",           "75AP_22Crit"                    },
  { "3326",  "+55 Attack Power and +15 Critical Strike Rating",           "55AP_15Crit"                    },
  { "3325",  "+45 Stamina and +15 Agility",                               "45Sta_15Agi"                    },
  { "3297",  "+275 Health",                                               "275Health"                      },
  { "3296",  "Wisdom",                                                    "10Spi"                          },
  { "3294",  "+225 Armor",                                                "225BArmor"                      },
  { "3273",  "Deathfrost",                                                ""                               },
  { "3269",  "Truesilver Line",                                           ""                               },
  { "3260",  "+240 Armor",                                                "240BArmor"                      },
  { "3256",  "Increased Stealth and Agility 10",                          "10Agi"                          },
  { "3253",  "+2% Threat and 10 Parry Rating",                            ""                               },
  { "3252",  "+8 All Stats",                                              "8All"                           },
  { "3247",  "Titanium Weapon Chain",                                     ""                               },
  { "3249",  "+16 Critical Strike Rating",                                "16Crit"                         },
  { "3246",  "+28 Spell Power",                                           "28SP"                           },
  { "3245",  "+20 Resilience Rating",                                     ""                               },
  { "3244",  "Greater Vitality",                                          "6MP5"                           },
  { "3243",  "+35 Spell Penetration",                                     "35SPen"                         },
  { "3241",  "Lifeward",                                                  ""                               },
  { "3239",  "Icebreaker Weapon",                                         ""                               },
  { "3238",  "Gatherer",                                                  ""                               },
  { "3236",  "+200 Health",                                               "200Health"                      },
  { "3234",  "+20 Hit Rating",                                            "20Hit"                          },
  { "3233",  "+250 Mana",                                                 "250Mana"                        },
  { "3232",  "Tuskar's Vitality",                                         "15Sta"                          },
  { "3231",  "+15 Expertise Rating",                                      "15Exp"                          },
  { "3230",  "+20 Frost Resistance",                                      ""                               },
  { "3229",  "+12 Resilience Rating",                                     ""                               },
  { "3225",  "Executioner",                                               "executioner"                    },
  { "3223",  "Adamantite Weapon Chain",                                   ""                               },
  { "3222",  "+20 Agility",                                               "20Agi"                          },
  { "3150",  "+6 mana every 5 sec.",                                      "6MP5"                           },
  { "3096",  "+17 Strength and +16 Intellect",                            "17Str_16Int"                    },
  { "3013",  "+40 Stamina and +12 Agility",                               "40Sta_12Agi"                    },
  { "3012",  "+50 Attack Power and +12 Critical Strike Rating",           "50AP_12Crit"                    },
  { "3011",  "+30 Stamina and +10 Agility",                               "30Sta_10Agi"                    },
  { "3010",  "+40 Attack Power and +10 Critical Strike Rating",           "40AP_10Crit"                    },
  { "3009",  "+20 Shadow Resistance",                                     ""                               },
  { "3008",  "+20 Frost Resistance",                                      ""                               },
  { "3007",  "+20 Fire Resistance",                                       ""                               },
  { "3006",  "+20 Arcane Resistance",                                     ""                               },
  { "3005",  "+20 Nature Resistance",                                     ""                               },
  { "3004",  "+18 Stamina and +20 Resilience Rating",                     "18Sta"                          },
  { "3003",  "+34 Attack Power and +16 Hit Rating",                       "34AP_16Hit"                     },
  { "3002",  "+22 Spell Power and +14 Hit Rating",                        "22SP_14Hit"                     },
  { "3001",  "+19 Spell Power and +7 Mana every 5 seconds",               "19SP_7MP5"                      },
  { "2999",  "+8 Stamina and +17 Dodge Rating",                           "8Sta_17Dodge"                   },
  { "2998",  "+7 All Resistances",                                        ""                               },
  { "2997",  "+15 Critical Strike Rating and +20 Attack Power",           "15Crit_20AP"                    },
  { "2996",  "+13 Critical Strike Rating",                                "13Crit"                         },
  { "2995",  "+15 Critical Strike Rating and +12 Spell Power",            "15Crit_12SP"                    },
  { "2994",  "+13 Critical Strike Rating",                                "13Crit"                         },
  { "2993",  "+12 Spell Power and 6 Mana every 5 seconds",                "12SP_6MP5"                      },
  { "2992",  "+5 Mana Regen",                                             "5MP5"                           },
  { "2991",  "+22 Stamina and +10 Dodge Rating",                          "22Sta_10Dodge"                  },
  { "2990",  "+13 Dodge Rating",                                          "13Dodge"                        },
  { "2989",  "+8 Arcane Resist",                                          ""                               },
  { "2988",  "+48 Stamina",                                               "48Sta"                          },
  { "2987",  "+8 Frost Resist",                                           ""                               },
  { "2986",  "+30 Attack Power and +10 Critical Strike Rating",           "30AP_10Crit"                    },
  { "2984",  "+8 Shadow Resist",                                          ""                               },
  { "2983",  "+26 Attack Power",                                          "26AP"                           },
  { "2982",  "+18 Spell Power and +10 Critical Strike Rating",            "18SP_10Crit"                    },
  { "2981",  "+15 Spell Power",                                           "15SP"                           },
  { "2980",  "+18 Spell Power and +4 Mana every 5 seconds",               "18SP_4MP5"                      },
  { "2979",  "+15 Spell Power",                                           "15SP"                           },
  { "2978",  "+15 Dodge Rating and +15 Stamina",                          "15Dodge_15Sta"                  },
  { "2977",  "+13 Dodge Rating",                                          "13Dodge"                        },
  { "2940",  "Minor Speed and +9 Stamina",                                "9Sta"                           },
  { "2939",  "Minor Speed and +6 Agility",                                "6Agi"                           },
  { "2938",  "+20 Spell Penetration",                                     "20SPen"                         },
  { "2937",  "+20 Spell Power",                                           "20SP"                           },
  { "2935",  "+15 Hit Rating",                                            "15Hit"                          },
  { "2934",  "+10 Critical Strike Rating",                                "10Crit"                         },
  { "2933",  "+15 Resilience Rating",                                     ""                               },
  { "2931",  "+4 All Stats",                                              "4All"                           },
  { "2930",  "+12 Spell Power",                                           "12SP"                           },
  { "2929",  "+2 Weapon Damage",                                          ""                               },
  { "2928",  "+12 Spell Power",                                           "12SP"                           },
  { "2841",  "+10 Stamina",                                               "10Sta"                          },
  { "2794",  "+3 Mana restored per 5 seconds",                            "3MP5"                           },
  { "2793",  "+21 Strength",                                              "21Str"                          },
  { "2792",  "+8 Stamina",                                                "8Sta"                           },
  { "2748",  "+35 Spell Power and +20 Stamina",                           "35SP_20Sta"                     },
  { "2747",  "+25 Spell Power and +15 Stamina",                           "25SP_15Sta"                     },
  { "2746",  "+35 Spell Power and +20 Stamina",                           "35SP_20Sta"                     },
  { "2745",  "+25 Spell Power and +15 Stamina",                           "25SP_15Sta"                     },
  { "2724",  "Scope (+28 Critical Strike Rating)",                        "28Crit"                         },
  { "2723",  "Scope (+12 Damage)",                                        ""                               },
  { "2722",  "Scope (+10 Damage)",                                        ""                               },
  { "2721",  "+15 Spell Power and +14 Critical Strike Rating",            "15SP_14Crit"                    },
  { "2717",  "+26 Attack Power and +14 Critical Strike Rating",           "26AP_14Crit"                    },
  { "2716",  "+16 Stamina and +100 Armor",                                "16Sta_100BArmor"                },
  { "2715",  "+16 Spell Power and 5 Mana every 5 seconds",                "16SP_5MP5"                      },
  { "2714",  "Felsteel Spike (26-38)",                                    ""                               },
  { "2683",  "+10 Shadow Resistance",                                     ""                               },
  { "2682",  "+10 Frost Resistance",                                      ""                               },
  { "2679",  "6 Mana per 5 Sec.",                                         "6MP5"                           },
  { "2675",  "Battlemaster",                                              ""                               },
  { "2674",  "Spellsurge",                                                "spellsurge"                     },
  { "2673",  "Mongoose",                                                  "mongoose"                       },
  { "2672",  "Soulfrost",                                                 ""                               },
  { "2671",  "Sunfire",                                                   ""                               },
  { "2670",  "+35 Agility",                                               "35Agi"                          },
  { "2669",  "+40 Spell Power",                                           "40SP"                           },
  { "2668",  "+20 Strength",                                              "20Str"                          },
  { "2667",  "Savagery",                                                  "70AP"                           },
  { "2666",  "+30 Intellect",                                             "30Int"                          },
  { "2664",  "+7 Resist All",                                             ""                               },
  { "2662",  "+120 Armor",                                                "120BArmor"                      },
  { "2661",  "+6 All Stats",                                              "6All"                           },
  { "2659",  "+150 Health",                                               "150Health"                      },
  { "2658",  "+10 Hit Rating and +10 Critical Strike Rating",             "10Hit_10Crit"                   },
  { "2657",  "+12 Agility",                                               "12Agi"                          },
  { "2656",  "Vitality",                                                  "4MP5"                           },
  { "2655",  "+15 Shield Block Rating",                                   ""                               },
  { "2654",  "+12 Intellect",                                             "12Int"                          },
  { "2653",  "+18 Block Value",                                           "18BlockV"                       },
  { "2650",  "+15 Spell Power",                                           "15SP"                           },
  { "2649",  "+12 Stamina",                                               "12Sta"                          },
  { "2648",  "+12 Dodge Rating",                                          "12Dodge"                        },
  { "2647",  "+12 Strength",                                              "12Str"                          },
  { "2646",  "+25 Agility",                                               "25Agi"                          },
  { "2622",  "+12 Dodge Rating",                                          "12Dodge"                        },
  { "2621",  "Subtlety",                                                  ""                               },
  { "2620",  "+15 Nature Resistance",                                     ""                               },
  { "2619",  "+15 Fire Resistance",                                       ""                               },
  { "2617",  "+16 Spell Power",                                           "16SP"                           },
  { "2616",  "+20 Fire Spell Damage",                                     ""                               },
  { "2615",  "+20 Frost Spell Damage",                                    ""                               },
  { "2614",  "+20 Shadow Spell Damage",                                   ""                               },
  { "2613",  "+2% Threat",                                                ""                               },
  { "2606",  "+30 Attack Power",                                          "30AP"                           },
  { "2605",  "+18 Spell Power",                                           "18SP"                           },
  { "2604",  "+18 Spell Power",                                           "18SP"                           },
  { "2603",  "Eternium Line",                                             ""                               },
  { "2591",  "+10 Intellect +10 Stamina +12 Spell Power",                 "10Int_10Sta_12SP"               },
  { "2590",  "+13 Spell Power +10 Stamina and +4 Mana every 5 seconds",   "10SP_10Sta_4MP5"                },
  { "2589",  "+18 Spell Power and +10 Stamina",                           "18SP_10Sta"                     },
  { "2588",  "+18 Spell Power and +8 Hit Rating",                         "18SP_8Hit"                      },
  { "2587",  "+13 Spell Power and +15 Intellect",                         "13SP_15Int"                     },
  { "2586",  "+24 Ranged Attack Power/+10 Stamina/+10 Hit Rating",        "_10Sta_10Hit"                   },
  { "2585",  "+28 Attack Power/+12 Dodge Rating",                         "28AP"                           },
  { "2584",  "+15 Dodge +10 Stamina and +12 Spell Power",                 "10Dodge_10Sta_12SP"             },
  { "2583",  "+10 Dodge Rating/+10 Stamina/+15 Block Value",              "10Dodge_10Sta"                  },
  { "2568",  "+22 Intellect",                                             "22Int"                          },
  { "2567",  "+20 Spirit",                                                "20Spi"                          },
  { "2566",  "+13 Spell Power",                                           "13SP"                           },
  { "2565",  "Mana Regen 4 per 5 sec.",                                   "4MP5"                           },
  { "2564",  "+15 Agility",                                               "15Agi"                          },
  { "2563",  "+15 Strength",                                              "15Str"                          },
  { "2545",  "+11 Stamina",                                               "11Sta"                          },
  { "2544",  "+8 Spell Power",                                            "8SP"                            },
  { "2543",  "+8 Agility",                                                "8Agi"                           },
  { "2523",  "+30 Ranged Hit Rating",                                     ""                               },
  { "2505",  "+29 Spell Power",                                           "29SP"                           },
  { "2504",  "+30 Spell Power",                                           "30SP"                           },
  { "2503",  "+5 Dodge Rating",                                           "5Dodge"                         },
  { "2488",  "+5 All Resistances",                                        ""                               },
  { "2487",  "+5 Shadow Resistance",                                      ""                               },
  { "2486",  "+5 Nature Resistance",                                      ""                               },
  { "2485",  "+5 Arcane Resistance",                                      ""                               },
  { "2484",  "+5 Frost Resistance",                                       ""                               },
  { "2483",  "+5 Fire Resistance",                                        ""                               },
  { "2463",  "+7 Fire Resistance",                                        ""                               },
  { "2443",  "+7 Frost Spell Damage",                                     ""                               },
  { "2381",  "+8 mana every 5 sec.",                                      "8MP5"                           },
  { "2376",  "+6 mana every 5 sec.",                                      "6MP5"                           },
  { "2343",  "+43 Spell Power",                                           "43SP"                           },
  { "2332",  "+30 Spell Power",                                           "30SP"                           },
  { "2326",  "+23 Spell Power",                                           "23SP"                           },
  { "2322",  "+19 Spell Power",                                           "19SP"                           },
  { "1953",  "+22 Dodge Rating",                                          "22Dodge"                        },
  { "1952",  "+20 Dodge Rating",                                          "20Dodge"                        },
  { "1951",  "+16 Dodge Rating",                                          "16Dodge"                        },
  { "1950",  "+15 Dodge Rating",                                          "15Dodge"                        },
  { "1904",  "+9 Intellect",                                              "9Int"                           },
  { "1903",  "+9 Spirit",                                                 "9Spi"                           },
  { "1900",  "Crusader",                                                  ""                               },
  { "1899",  "Unholy Weapon",                                             ""                               },
  { "1898",  "Lifestealing",                                              ""                               },
  { "1897",  "+5 Weapon Damage",                                          ""                               },
  { "1896",  "+9 Weapon Damage",                                          ""                               },
  { "1894",  "Icy Weapon",                                                ""                               },
  { "1893",  "+100 Mana",                                                 "100Mana"                        },
  { "1892",  "+100 Health",                                               "100Health"                      },
  { "1891",  "+4 All Stats",                                              "4All"                           },
  { "1890",  "+9 Spirit",                                                 "9Spi"                           },
  { "1889",  "+70 Armor",                                                 "70BArmor"                       },
  { "1888",  "+5 All Resistances",                                        ""                               },
  { "1887",  "+7 Agility",                                                "7Agi"                           },
  { "1886",  "+9 Stamina",                                                "9Sta"                           },
  { "1885",  "+9 Strength",                                               "9Str"                           },
  { "1884",  "+13 Intellect",                                             "13Int"                          },
  { "1883",  "+7 Intellect",                                              "7Int"                           },
  { "1843",  "Reinforced (+40 Armor)",                                    "40BArmor"                       },
  { "1704",  "+12 Resilience Rating",                                     ""                               },
  { "1606",  "+50 Attack Power",                                          "50AP"                           },
  { "1603",  "+44 Attack Power",                                          "44AP"                           },
  { "1600",  "+38 Attack Power",                                          "38AP"                           },
  { "1597",  "+32 Attack Power",                                          "32AP"                           },
  { "1594",  "+26 Attack Power",                                          "26AP"                           },
  { "1593",  "+24 Attack Power",                                          "24AP"                           },
  { "1510",  "+8 Spirit",                                                 "8Spi"                           },
  { "1509",  "+8 Intellect",                                              "8Int"                           },
  { "1508",  "+8 Agility",                                                "8Agi"                           },
  { "1507",  "+8 Stamina",                                                "8Sta"                           },
  { "1506",  "+8 Strength",                                               "8Str"                           },
  { "1505",  "+20 Fire Resistance",                                       ""                               },
  { "1504",  "+125 Armor",                                                "125BArmor"                      },
  { "1503",  "+100 HP",                                                   "100Health"                      },
  { "1483",  "+150 Mana",                                                 "150Mana"                        },
  { "1441",  "+15 Shadow Resistance",                                     ""                               },
  { "1400",  "+20 Nature Resistance",                                     ""                               },
  { "1354",  "+15 Haste Rating",                                          "15Haste"                        },
  { "1257",  "+15 Arcane Resistance",                                     ""                               },
  { "1147",  "+18 Spirit",                                                "18Spi"                          },
  { "1144",  "+15 Spirit",                                                "15Spi"                          },
  { "1128",  "+25 Intellect",                                             "25Int"                          },
  { "1119",  "+16 Intellect",                                             "16Int"                          },
  { "1103",  "+26 Agility",                                               "26Agi"                          },
  { "1099",  "+22 Agility",                                               "22Agi"                          },
  { "1075",  "+22 Stamina",                                               "22Sta"                          },
  { "1071",  "+18 Stamina",                                               "18Sta"                          },
  {  "983",  "+16 Agility",                                               "16Agi"                          },
  {  "963",  "+7 Weapon Damage",                                          ""                               },
  {  "943",  "+3 Agility",                                                "3Agi"                           },
  {  "931",  "+10 Haste Rating",                                          "10Haste"                        },
  {  "930",  "+2% Mount Speed",                                           ""                               },
  {  "929",  "+7 Stamina",                                                "7Sta"                           },
  {  "928",  "+3 All Stats",                                              "3All"                           },
  {  "927",  "+7 Strength",                                               "7Str"                           },
  {  "925",  "+6 Intellect",                                              "6Int"                           },
  {  "924",  "+2 Dodge Rating",                                           "2Dodge"                         },
  {  "923",  "+5 Dodge Rating",                                           "5Dodge"                         },
  {  "913",  "+65 Mana",                                                  "65Mana"                         },
  {  "912",  "Demonslaying",                                              ""                               },
  {  "911",  "Minor Speed Increase",                                      ""                               },
  {  "910",  "Increased Stealth",                                         ""                               },
  {  "909",  "+5 Herbalism",                                              ""                               },
  {  "908",  "+14 Agility",                                               "14Agi"                          },
  {  "907",  "+7 Spirit",                                                 "5Spi"                           },
  {  "906",  "+5 Mining",                                                 ""                               },
  {  "905",  "+5 Intellect",                                              "5Int"                           },
  {  "904",  "+5 Agility",                                                "5Agi"                           },
  {  "903",  "+19 Shadow Spell Damage",                                   ""                               },
  {  "884",  "+50 Armor",                                                 "50BArmor"                       },
  {  "866",  "+2 All Stats",                                              "2All"                           },
  {  "865",  "+5 Skinning",                                               ""                               },
  {  "863",  "+10 Shield Block Rating",                                   ""                               },
  {  "857",  "+50 Mana",                                                  "50Mana"                         },
  {  "856",  "+5 Strength",                                               "5Str"                           },
  {  "854",  "+2 Stamina",                                                "2Sta"                           },
  {  "853",  "+6 Beastslaying",                                           ""                               },
  {  "852",  "+5 Stamina",                                                "5Sta"                           },
  {  "851",  "+5 Spirit",                                                 "5Spi"                           },
  {  "850",  "+35 Health",                                                "35Health"                       },
  {  "849",  "+3 Agility",                                                "3Agi"                           },
  {  "848",  "+30 Armor",                                                 "30BArmor"                       },
  {  "847",  "+0 Agility",                                                ""                               },
  {  "846",  "+0 Attack Power",                                           ""                               },
  {  "845",  "+2 Herbalism",                                              ""                               },
  {  "844",  "+2 Mining",                                                 ""                               },
  {  "843",  "+30 Mana",                                                  "30Mana"                         },
  {  "823",  "+7 Stamina",                                                "7Sta"                           },
  {  "805",  "+4 Weapon Damage",                                          ""                               },
  {  "804",  "+10 Shadow Resistance",                                     ""                               },
  {  "803",  "+9 Strength",                                               "9Str"                           },
  {  "783",  "+10 Armor",                                                 "10BArmor"                       },
  {  "744",  "+3 Intellect",                                              "3Int"                           },
  {  "724",  "+2 Intellect",                                              "2Int"                           },
  {  "723",  "+3 Intellect",                                              "3Int"                           },
  {  "684",  "+15 Strength",                                              "15Str"                          },
  {  "664",  "Scope (+7 Damage)",                                         ""                               },
  {  "663",  "Scope (+5 Damage)",                                         ""                               },
  {  "464",  "+4% Mount Speed",                                           ""                               },
  {  "463",  "Mithril Spike (16-20)",                                     ""                               },
  {  "369",  "+12 Intellect",                                             "12Int"                          },
  {  "368",  "+12 Agility",                                               "12Agi"                          },
  {  "256",  "+5 Fire Resistance",                                        ""                               },
  {  "255",  "+3 Spirit",                                                 "3Spi"                           },
  {  "254",  "+25 Health",                                                "25Health"                       },
  {  "250",  "+1  Weapon Damage",                                         ""                               },
  {  "249",  "+2 Beastslaying",                                           ""                               },
  {  "248",  "+2 Intellect",                                              "2Int"                           },
  {  "247",  "+1 Agility",                                                "1Agi"                           },
  {  "246",  "+20 Mana",                                                  "20Mana"                         },
  {  "243",  "+3 Intellect",                                              "3Int"                           },
  {  "242",  "+15 Health",                                                "15Health"                       },
  {  "241",  "+2 Weapon Damage",                                          ""                               },
  {   "66",  "+1 Stamina",                                                "1Sta"                           },
  {   "65",  "+1 All Resistances",                                        ""                               },
  {   "63",  "Absorption (25)",                                           ""                               },
  {   "44",  "+11 Intellect",                                             "11Int"                          },
  {   "43",  "+24 Intellect",                                             "24Int"                          },
  {   "41",  "+5 Health",                                                 "5Health"                        },
  {   "37",  "Counterweight (+20 Haste Rating)",                          "20Haste"                        },
  {   "36",  "Enchant: Fiery Blaze",                                      ""                               },
  {   "34",  "Counterweight (+20 Haste Rating)",                          "20Haste"                        },
  {   "33",  "Scope (+3 Damage)",                                         ""                               },
  {   "32",  "Scope (+2 Damage)",                                         ""                               },
  {   "30",  "Scope (+1 Damage)",                                         ""                               },
  {   "24",  "+5 Mana",                                                   "8Mana"                          },
  {   "18",  "+12 Agility",                                               "12Agi"                          },
  {   "17",  "Reinforced (+24 Armor)",                                    "24BArmor"                       },
  {   "16",  "Reinforced (+16 Armor)",                                    "16BArmor"                       },
  {   "15",  "Reinforced (+8 Armor)",                                     "8BArmor"                        },
  { NULL, NULL, NULL }
};

// Add-Ons use the same enchant data-base for now
static enchant_data_t* addon_db = enchant_db;

static const stat_type reforge_stats[] = { 
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

// Weapon Stat Proc Callback ==================================================

struct weapon_stat_proc_callback_t : public action_callback_t
{
  weapon_t* weapon;
  buff_t* buff;
  double PPM;
  bool all_damage;

  weapon_stat_proc_callback_t( player_t* p, weapon_t* w, buff_t* b, double ppm=0.0, bool all=false ) :
    action_callback_t( p -> sim, p ), weapon(w), buff(b), PPM(ppm), all_damage(all) {}

  virtual void trigger( action_t* a, void* call_data )
  {
    if( ! all_damage && a -> proc ) return;
    if( weapon && a -> weapon != weapon ) return;

    if( PPM > 0 )
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

// Weapon Discharge Proc Callback =============================================

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

  weapon_discharge_proc_callback_t( const std::string& n, player_t* p, weapon_t* w, int ms, const school_type school, double dmg, double fc, double ppm=0, double cd=0, int rng_type=RNG_DEFAULT ) :
      action_callback_t( p -> sim, p ),
      name_str( n ), weapon( w ), stacks( 0 ), max_stacks( ms ), fixed_chance( fc ), PPM( ppm )
  {
    if ( rng_type == RNG_DEFAULT ) rng_type = RNG_CYCLIC; // default is CYCLIC since discharge should not have duration

    struct discharge_spell_t : public spell_t
    {
      discharge_spell_t( const char* n, player_t* p, double dmg, const school_type s ) :
          spell_t( n, p, RESOURCE_NONE, ( s == SCHOOL_DRAIN ) ? SCHOOL_SHADOW : s )
      {
        trigger_gcd = 0;
        base_dd_min = dmg;
        base_dd_max = dmg;
        may_crit = ( s != SCHOOL_DRAIN );
        background  = true;
        proc = true;
        base_spell_power_multiplier = 0;
        reset();
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

  virtual void trigger( action_t* a, void* call_data )
  {
    if( a -> proc ) return;
    if( weapon && a -> weapon != weapon ) return;

    if ( cooldown -> remains() > 0 )
        return;

    double chance = fixed_chance;
    if( weapon && PPM > 0 )
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

  item -> unique_addon = true;

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
  item -> use.duration = 10.0;
  item -> use.cooldown = 60.0;
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
      action_callback_t* cb = new weapon_discharge_proc_callback_t( "avalanche_w", p, mhw, 1, SCHOOL_NATURE, 500, 0, 5.0/*PPM*/, 0.01/*CD*/ );
      p -> register_attack_callback( RESULT_HIT_MASK, cb );
    }
    if ( oh_enchant == "avalanche" )
    {
      action_callback_t* cb = new weapon_discharge_proc_callback_t( "avalanche_w", p, ohw, 1, SCHOOL_NATURE, 500, 0, 5.0/*PPM*/, 0.01/*CD*/ );
      p -> register_attack_callback( RESULT_HIT_MASK, cb );
    }
    action_callback_t* cb = new weapon_discharge_proc_callback_t( "avalanche_s", p, 0, 1, SCHOOL_NATURE, 500, 0.25/*FIXED*/, 0, 10.0/*CD*/ );
    p -> register_spell_callback ( RESULT_HIT_MASK, cb );
  }
  if ( mh_enchant == "berserking" )
  {
    buff_t* buff = new stat_buff_t( p, "berserking_mh", STAT_ATTACK_POWER, 400, 1, 15, 0, 0, false, false, RNG_DISTRIBUTED );
    p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, mhw, buff, 1.0/*PPM*/ ) );
  }
  if ( oh_enchant == "berserking" )
  {
    buff_t* buff = new stat_buff_t( p, "berserking_oh", STAT_ATTACK_POWER, 400, 1, 15, 0, 0, false, false, RNG_DISTRIBUTED );
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

    buff_t* buff = new stat_buff_t( p, "executioner", STAT_CRIT_RATING, 120, 1, 15, 0, 0, false, false, RNG_DISTRIBUTED );

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
      mh_buff = new stat_buff_t( p, "hurricane_mh", STAT_HASTE_RATING, 450, 1, 12, 0, 0, false, false, RNG_DISTRIBUTED );
      p -> register_direct_damage_callback( SCHOOL_ALL_MASK, new weapon_stat_proc_callback_t( p, mhw, mh_buff, 1.0/*PPM*/, true/*ALL*/ ) );
      p -> register_tick_damage_callback  ( SCHOOL_ALL_MASK, new weapon_stat_proc_callback_t( p, mhw, mh_buff, 1.0/*PPM*/, true/*ALL*/ ) );
    }
    if ( oh_enchant == "hurricane" )
    {
      oh_buff = new stat_buff_t( p, "hurricane_oh", STAT_HASTE_RATING, 450, 1, 12, 0, 0, false, false, RNG_DISTRIBUTED );
      p -> register_direct_damage_callback( SCHOOL_ALL_MASK, new weapon_stat_proc_callback_t( p, ohw, oh_buff, 1.0/*PPM*/, true /*ALL*/ ) );
      p -> register_tick_damage_callback  ( SCHOOL_ALL_MASK, new weapon_stat_proc_callback_t( p, ohw, oh_buff, 1.0/*PPM*/, true /*ALL*/ ) );
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
        action_callback_t( p -> sim, p ), mh_buff(mhb), oh_buff(ohb), s_buff(sb)
      {
      }
      virtual void trigger( action_t* a, void* call_data )
      {
        if( s_buff -> cooldown -> remains() > 0 ) return;
        if( ! s_buff -> rng -> roll( 0.15 ) ) return;
        if( mh_buff && mh_buff -> check() )
        {
          mh_buff -> trigger();
          s_buff -> cooldown -> start();
        }
        else if( oh_buff && oh_buff -> check() )
        {
          oh_buff -> trigger();
          s_buff -> cooldown -> start();
        }
        else s_buff -> trigger();
      }
    };
    buff_t* s_buff = new stat_buff_t( p, "hurricane_s", STAT_HASTE_RATING, 450, 1, 12, 45.0 );
    p -> register_direct_damage_callback( SCHOOL_ALL_MASK, new hurricane_spell_proc_callback_t( p, mh_buff, oh_buff, s_buff ) );
    p -> register_tick_damage_callback  ( SCHOOL_ALL_MASK, new hurricane_spell_proc_callback_t( p, mh_buff, oh_buff, s_buff ) );
  }
  if ( mh_enchant == "landslide" )
  {
    buff_t* buff = new stat_buff_t( p, "landslide_mh", STAT_ATTACK_POWER, 1000, 1, 12, 0, 0, false, false, RNG_DISTRIBUTED );
    p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, mhw, buff, 1.0/*PPM*/ ) );
  }
  if ( oh_enchant == "landslide" )
  {
    buff_t* buff = new stat_buff_t( p, "landslide_oh", STAT_ATTACK_POWER, 1000, 1, 12, 0, 0, false, false, RNG_DISTRIBUTED );
    p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, ohw, buff, 1.0/*PPM*/ ) );
  }
  if ( mh_enchant == "mongoose" )
  {
    p -> buffs.mongoose_mh = new stat_buff_t( p, "mongoose_main_hand", STAT_AGILITY, 120, 1, 15, 0, 0, false, false, RNG_DISTRIBUTED );
    p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, mhw, p -> buffs.mongoose_mh, 1.0/*PPM*/ ) );
  }
  if ( oh_enchant == "mongoose" )
  {
    p -> buffs.mongoose_oh = new stat_buff_t( p, "mongoose_off_hand" , STAT_AGILITY, 120, 1, 15, 0, 0, false, false, RNG_DISTRIBUTED );
    p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, ohw, p -> buffs.mongoose_oh, 1.0/*PPM*/ ) );
  }
  if ( mh_enchant == "power_torrent" )
  {
    buff_t* buff = new stat_buff_t( p, "power_torrent_mh", STAT_INTELLECT, 500, 1, 12, 45, 0.20, false, false, RNG_DISTRIBUTED );
    weapon_stat_proc_callback_t* cb = new weapon_stat_proc_callback_t( p, NULL, buff );
    p -> register_tick_damage_callback  ( RESULT_ALL_MASK, cb );
    p -> register_direct_damage_callback( RESULT_ALL_MASK, cb );
  }
  if ( oh_enchant == "power_torrent" )
  {
    buff_t* buff = new stat_buff_t( p, "power_torrent_oh", STAT_INTELLECT, 500, 1, 12, 45, 0.20, false, false, RNG_DISTRIBUTED );
    weapon_stat_proc_callback_t* cb = new weapon_stat_proc_callback_t( p, NULL, buff );
    p -> register_tick_damage_callback  ( RESULT_ALL_MASK, cb );
    p -> register_direct_damage_callback( RESULT_ALL_MASK, cb );
  }
  if ( mh_enchant == "windwalk" )
  {
    buff_t* buff = new stat_buff_t( p, "windwalk_mh", STAT_DODGE_RATING, 600, 1, 10, 45, 0.15, false, false, RNG_DISTRIBUTED );
    p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, mhw, buff ) );
  }
  if ( oh_enchant == "windwalk" )
  {
    buff_t* buff = new stat_buff_t( p, "windwalk_oh", STAT_DODGE_RATING, 600, 1, 10, 45, 0.15, false, false, RNG_DISTRIBUTED );
    p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, ohw, buff ) );
  }
  if ( ranged_enchant == "gnomish_xray" )
  {
    //FIXME: 1.0 ppm and 40 second icd seems to roughly match in-game behavior, but we need to verify the exact mechanics
    buff_t* buff = new stat_buff_t( p, "xray_targeting", STAT_ATTACK_POWER, 800, 1, 10, 40, 0, false, false, RNG_DISTRIBUTED, 95712 );
    p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, rw, buff, 1.0/*PPM*/ ) );
  }
  if ( p -> meta_gem == META_THUNDERING_SKYFIRE )
  {
    //FIXME: 0.2 ppm and 40 second icd seems to roughly match in-game behavior, but we need to verify the exact mechanics
    buff_t* buff = new stat_buff_t( p, "skyfire_swiftness", STAT_HASTE_RATING, 240, 1, 6, 40, 0, false, false, RNG_DISTRIBUTED, 39959 );
    p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, mhw, buff, 0.2/*PPM*/ ) );
    p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, ohw, buff, 0.2/*PPM*/ ) );
    p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, rw,  buff, 0.2/*PPM*/ ) );
  }
  if ( p -> meta_gem == META_THUNDERING_SKYFLARE )
  {
    //FIXME: 0.2 ppm and 40 second icd seems to roughly match in-game behavior, but we need to verify the exact mechanics
    buff_t* buff = new stat_buff_t( p, "skyflare_swiftness", STAT_HASTE_RATING, 480, 1, 6, 40, 0, false, false, RNG_DISTRIBUTED, 55379 );
    p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, mhw, buff, 0.2/*PPM*/ ) );
    p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, ohw, buff, 0.2/*PPM*/ ) );
    p -> register_attack_callback( RESULT_HIT_MASK, new weapon_stat_proc_callback_t( p, rw,  buff, 0.2/*PPM*/ ) );
  }

  int num_items = ( int ) p -> items.size();
  for ( int i=0; i < num_items; i++ )
  {
    item_t& item = p -> items[ i ];

    if ( item.enchant.stat )
    {
      unique_gear_t::register_stat_proc( item, item.enchant );
    }
    else if ( item.enchant.school )
    {
      unique_gear_t::register_discharge_proc( item, item.enchant );
    }
    else if ( item.encoded_addon_str == "synapse_springs" )
    {
      register_synapse_springs( &item );
    }
  }
}

// enchant_t::get_encoding ==================================================

bool enchant_t::get_encoding( std::string& name,
                              std::string& encoding,
                              const std::string& enchant_id )
{
  for ( int i=0; enchant_db[ i ].id; i++ )
  {
    enchant_data_t& enchant = enchant_db[ i ];

    if ( enchant_id == enchant.id )
    {
      name     = enchant.name;
      encoding = enchant.encoding;
      return true;
    }
  }
  return false;
}

// enchant_t::get_addon_encoding ============================================

bool enchant_t::get_addon_encoding( std::string& name,
                                    std::string& encoding,
                                    const std::string& addon_id )
{
  for ( int i=0; addon_db[ i ].id; i++ )
  {
    enchant_data_t& addon = addon_db[ i ];

    if ( addon_id == addon.id )
    {
      name     = addon.name;
      encoding = addon.encoding;
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
  name = encoding = "";

  if ( reforge_id.empty() || reforge_id == "" || reforge_id == "0" )
    return true;

  int start = 0;
  int target = atoi( reforge_id.c_str() );
  target %= 56;
  if( target == 0 ) target = 56;
  else if( target <= start ) return false;

  for( int i=0; reforge_stats[ i ] != STAT_NONE; i++ )
  {
    for( int j=0; reforge_stats[ j ] != STAT_NONE; j++ )
    {
      if( i == j ) continue;
      if( ++start == target )
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
  for( index_from=0; reforge_stats[ index_from ] != STAT_NONE; index_from++ )
    if ( reforge_stats[ index_from ] == stat_from )
      break;

  int index_to;
  for( index_to=0; reforge_stats[ index_to ] != STAT_NONE; index_to++ )
    if ( reforge_stats[ index_to ] == stat_to )
      break;
  
  int id=0;
  for( int i=0; reforge_stats[ i ] != STAT_NONE; i++ )
  {
    for( int j=0; reforge_stats[ j ] != STAT_NONE; j++ )
    {
      if( i == j ) continue;
      id++;
      if( index_from == i &&
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

  if ( enchant_id.empty() || enchant_id == "" || enchant_id == "0" )
    return true;

  std::string description;
  if ( get_encoding( description, item.armory_enchant_str, enchant_id ) )
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

  if ( addon_id.empty() || addon_id == "" || addon_id == "0" )
    return true;

  std::string description;
  if ( get_addon_encoding( description, item.armory_addon_str, addon_id ) )
  {
    armory_t::format( item.armory_addon_str );
    return true;
  }

  return false;
}

// enchant_t::download_reforge ===============================================

bool enchant_t::download_reforge( item_t&            item,
                                  const std::string& reforge_id )
{
  item.armory_reforge_str.clear();

  if ( reforge_id.empty() || reforge_id == "" || reforge_id == "0" )
    return true;

  std::string description;
  if ( get_reforge_encoding( description, item.armory_reforge_str, reforge_id ) )
  {
    armory_t::format( item.armory_reforge_str );
    return true;
  }

  return false;
}

// enchant_t::download_rsuffix ===============================================

bool enchant_t::download_rsuffix( item_t&            item,
                                  const std::string& rsuffix_id )
{
  item.armory_random_suffix_str = rsuffix_id;
  return true;
}
