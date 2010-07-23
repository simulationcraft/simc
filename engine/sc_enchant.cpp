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
  { "3878",  "Mind Amplification Dish",                                   "45Sta"                          },
  { "3876",  "Inscription of the Pinnacle",                               "15Dodge_10Def"                  },
  { "3875",  "+30 Attack Power and +10 Crit Rating",                      "30AP_10Crit"                    },
  { "3873",  "+50 Spell Power and +30 Stamina",                           "50SP_30Sta"                     },
  { "3872",  "+50 Spell Power and +20 Spirit",                            "50SP_20Spi"                     },
  { "3870",  "Blood Draining",                                            ""                               },
  { "3869",  "Blade Ward",                                                ""                               },
  { "3860",  "+885 Armor",                                                "885BArmor"                      },
  { "3859",  "Springy Arachnoweave",                                      "27SP"                           },
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
  { "3842",  "+37 Stamina and +20 Defense",                               "37Sta_20Def"                    },
  { "3840",  "+23 Spell Power",                                           "23SP"                           },
  { "3839",  "+40 Attack Power",                                          "40AP"                           },
  { "3838",  "+70 Spell Power and +15 Crit Rating",                       "70SP_15Crit"                    },
  { "3837",  "+60 Dodge Rating and +15 Defense Rating",                   "60Dodge_15Def"                  },
  { "3836",  "+70 Spell Power and +6 Mana/5 seconds",                     "70SP_6MP5"                      },
  { "3835",  "+120 Attack Power and +15 Crit Rating",                     "120AP_15Crit"                   },
  { "3834",  "+63 Spell Power",                                           "63SP"                           },
  { "3833",  "+65 Attack Power",                                          "65AP"                           },
  { "3832",  "+10 All Stats",                                             "10Str_10Agi_10Sta_10Int_10Spi"  },
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
  { "3818",  "+37 Stamina and +20 Defense",                               "37Sta_20Def"                    },
  { "3817",  "+50 Attack Power and +20 Critical Strike Rating",           "50AP_20Crit"                    },
  { "3816",  "+25 Fire Resistance and 30 Stamina",                        "30Sta"                          },
  { "3815",  "+25 Arcane Resistance and 30 Stamina",                      "30Sta"                          },
  { "3814",  "+25 Shadow Resistance and 30 Stamina",                      "30Sta"                          },
  { "3813",  "+25 Nature Resistance and 30 Stamina",                      "30Sta"                          },
  { "3812",  "+25 Frost Resistance and 30 Stamina",                       "30Sta"                          },
  { "3811",  "+20 Dodge Rating and +15 Defense Rating",                   "20Dodge_15Def"                  },
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
  { "3754",  "+24 Attack Power/+10 Stamina/+10 Hit Rating",               "24AP_10Sta_10Hit"               },
  { "3748",  "Titanium Spike (45-67)",                                    ""                               },
  { "3731",  "Titanium Weapon Chain",                                     ""                               },
  { "3730",  "Swordguard Embroidery",                                     "swordguard_embroidery"          },
  { "3728",  "Darkglow Embroidery",                                       "darkglow_embroidery"            },
  { "3722",  "Lightweave Embroidery",                                     "lightweave_embroidery"          },
  { "3721",  "+50 Spell Power and +30 Stamina",                           "50SP_30Sta"                     },
  { "3720",  "+35 Spell Power and +20 Stamina",                           "35SP_20Sta"                     },
  { "3719",  "+50 Spell Power and +20 Spirit",                            "50SP_20Spi"                     },
  { "3718",  "+35 Spell Power and +12 Spirit",                            "35SP_12Spi"                     },
  { "3608",  "+40 Ranged Critical Strike",                                "40Crit"                         },
  { "3607",  "+40 Ranged Haste Rating",                                   "40Haste"                        },
  { "3606",  "Nitro Boosts",                                              "24Crit"                         },
  { "3605",  "Flexweave Underlay",                                        "23Agi"                          },
  { "3604",  "Hyperspeed Accelerators",                                   "hyperspeed_accelerators"        },
  { "3603",  "Hand-Mounted Pyro Rocket",                                  "hand_mounted_pyro_rocket"       },
  { "3601",  "Belt-Clipped Spynoculars",                                  ""                               },
  { "3599",  "Electromagnetic Pulse Generator",                           ""                               },
  { "3595",  "Rune of Spellbreaking",                                     ""                               },
  { "3594",  "Rune of Swordbreaking",                                     ""                               },
  { "3370",  "Rune of Razorice",                                          "rune_of_razorice"               },
  { "3369",  "Rune of Cinderglacier",                                     ""                               },
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
  { "3252",  "+8 All Stats",                                              "8Str_8Agi_8Sta_8Int_8Spi"       },
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
  { "2999",  "+16 Defense Rating and +17 Dodge Rating",                   "16Def_17Dodge"                  },
  { "2998",  "+7 All Resistances",                                        ""                               },
  { "2997",  "+15 Critical Strike Rating and +20 Attack Power",           "15Crit_20AP"                    },
  { "2996",  "+13 Critical Strike Rating",                                "13Crit"                         },
  { "2995",  "+15 Critical Strike Rating and +12 Spell Power",            "15Crit_12SP"                    },
  { "2994",  "+13 Critical Strike Rating",                                "13Crit"                         },
  { "2993",  "+12 Spell Power and 6 Mana every 5 seconds",                "12SP_6MP5"                      },
  { "2992",  "+5 Mana Regen",                                             "5MP5"                           },
  { "2991",  "+15 Defense Rating and +10 Dodge Rating",                   "15Def_10Dodge"                  },
  { "2990",  "+13 Defense Rating",                                        "13Def"                          },
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
  { "2978",  "+15 Dodge Rating and +10 Defense Rating",                   "15Dodge_10Def"                  },
  { "2977",  "+13 Dodge Rating",                                          "13Dodge"                        },
  { "2940",  "Minor Speed and +9 Stamina",                                "9Sta"                           },
  { "2939",  "Minor Speed and +6 Agility",                                "6Agi"                           },
  { "2938",  "+20 Spell Penetration",                                     "20SPen"                         },
  { "2937",  "+20 Spell Power",                                           "20SP"                           },
  { "2935",  "+15 Hit Rating",                                            "15Hit"                          },
  { "2934",  "+10 Critical Strike Rating",                                "10Crit"                         },
  { "2933",  "+15 Resilience Rating",                                     ""                               },
  { "2931",  "+4 All Stats",                                              "4Str_4Agi_4Sta_4Int_4Spi"       },
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
  { "2661",  "+6 All Stats",                                              "6Str_6Agi_6Sta_6Int_6Spi"       },
  { "2659",  "+150 Health",                                               "150Health"                      },
  { "2658",  "+10 Hit Rating and +10 Critical Strike Rating",             "10Hit_10Crit"                   },
  { "2657",  "+12 Agility",                                               "12Agi"                          },
  { "2656",  "Vitality",                                                  "4MP5"                           },
  { "2655",  "+15 Shield Block Rating",                                   ""                               },
  { "2654",  "+12 Intellect",                                             "12Int"                          },
  { "2653",  "+18 Block Value",                                           "18BlockV"                       },
  { "2650",  "+15 Spell Power",                                           "15SP"                           },
  { "2649",  "+12 Stamina",                                               "12Sta"                          },
  { "2648",  "+12 Defense Rating",                                        "12Def"                          },
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
  { "2584",  "+10 Defense +10 Stamina and +12 Spell Power",               "10Def_10Sta_12SP"               },
  { "2583",  "+10 Defense Rating/+10 Stamina/+15 Block Value",            "10Def_10Sta"                    },
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
  { "2503",  "+5 Defense Rating",                                         "5Def"                           },
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
  { "1953",  "+22 Defense Rating",                                        "22Def"                          },
  { "1952",  "+20 Defense Rating",                                        "20Def"                          },
  { "1951",  "+16 Defense Rating",                                        "16Def"                          },
  { "1950",  "+15 Defense Rating",                                        "15Def"                          },
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
  { "1891",  "+4 All Stats",                                              "4Str_4Agi_4Sta_4Int_4Spi"       },
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
  {  "928",  "+3 All Stats",                                              "3Str_3Agi_3Sta_3Int_3Spi"       },
  {  "927",  "+7 Strength",                                               "7Str"                           },
  {  "925",  "+6 Intellect",                                              "6Int"                           },
  {  "924",  "+2 Defense Rating",                                         "2Def"                           },
  {  "923",  "+5 Defense Rating",                                         "5Def"                           },
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
  {  "866",  "+2 All Stats",                                              "2Str_2Agi_2Sta_2Int_2Spi"       },
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

// Spellsurge Enchant =======================================================

struct spellsurge_callback_t : public action_callback_t
{
  spell_t* spell;
  rng_t* rng;

  spellsurge_callback_t( player_t* p ) : action_callback_t( p -> sim, p )
  {
    struct spellsurge_t : public spell_t
    {
      spellsurge_t( player_t* p ) :
          spell_t( "spellsurge", p, RESOURCE_MANA, SCHOOL_ARCANE )
      {
        background     = true;
        base_tick_time = 1.0;
        num_ticks      = 10;
        trigger_gcd    = 0;
        cooldown -> duration = 60;
      }
      virtual void execute()
      {
        assert( current_tick == 0 );
        schedule_tick();
      }
      virtual void tick()
      {
        for ( player_t* p = sim -> player_list; p; p = p -> next )
        {
          if ( p -> party == player -> party )
          {
            if ( sim -> log ) log_t::output( sim, "Player %s gains mana from %s 's Spellsurge.", p -> name(), player -> name() );
            p -> resource_gain( RESOURCE_MANA, 10.0, p -> gains.spellsurge );
          }
        }
      }
    };

    spell = new spellsurge_t( p );
    rng = p -> get_rng( "spellsurge" );
  }

  virtual void trigger( action_t* a )
  {
    if ( spell -> ready() && rng -> roll( 0.15 ) )
    {
      for ( player_t* p = a -> sim -> player_list; p; p = p -> next )
      {
        // Invalidate any existing Spellsurge procs.

        if ( p -> party == a -> player -> party )
        {
          action_t* spellsurge = p -> find_action( "spellsurge" );

          if ( spellsurge && spellsurge -> ticking )
          {
            spellsurge -> cancel();
            break;
          }
        }
      }

      spell -> execute();
    }
  }
};

// Berserking Enchant =========================================================

struct berserking_callback_t : public action_callback_t
{
  int slot;
  buff_t* buff;

  berserking_callback_t( player_t* p, int s, buff_t* b ) : action_callback_t( p -> sim, p ), slot(s), buff(b) {}

  virtual void trigger( action_t* a )
  {
    if ( ! a -> weapon ) 
      return;

    if ( a -> weapon -> slot != slot ) 
      return;

    // Apparently, Berserking has a 1.0 PPM (proc per minute) that scales with haste, so
    // we no longer use time_to_execute to determine proc chance.

    double PPM    = 1.0;
    double chance = a -> weapon -> proc_chance_on_swing( PPM );
   
    buff -> trigger( 1, 0, chance );
    buff -> up();  // track uptime info
  }
};

// Mongoose Enchant =========================================================

struct mongoose_callback_t : public action_callback_t
{
  int slot;
  buff_t* buff;

  mongoose_callback_t( player_t* p, int s, buff_t* b ) : action_callback_t( p -> sim, p ), slot(s), buff(b) {}

  virtual void trigger( action_t* a )
  {
    if ( ! a -> weapon ) 
      return;

    if ( a -> weapon -> slot != slot ) 
      return;

    // Apparently, Mongoose has a 1.0 PPM (proc per minute) that scales with haste, so
    // we no longer use time_to_execute to determine proc chance.

    double PPM    = 1.0;
    double chance = a -> weapon -> proc_chance_on_swing( PPM );

    buff -> trigger( 1, 0, chance );
  }
};

// Executioner Enchant =========================================================

struct executioner_callback_t : public action_callback_t
{
  int slot;
  buff_t* buff;

  executioner_callback_t( player_t* p, int s, buff_t* b ) : action_callback_t( p -> sim, p ), slot(s), buff(b) {}

  virtual void trigger( action_t* a )
  {
    if ( ! a -> weapon ) 
      return;

    if ( a -> weapon -> slot != slot ) 
      return;

    // Apparently, Executioner has a 1.0 PPM (proc per minute) that scales with haste, so
    // we no longer use time_to_execute to determine proc chance.

    double PPM    = 1.0;
    double chance = a -> weapon -> proc_chance_on_swing( PPM );

    buff -> trigger( 1, 0, chance );
    buff -> up();  // track uptime info
  }
};

// ==========================================================================
// Enchant
// ==========================================================================

// enchant_t::init ==========================================================

void enchant_t::init( player_t* p )
{
  if ( p -> is_pet() ) return;

  // Need to expose the Mongoose buffs for attack_t::haste()
  p -> buffs.mongoose_mh = new stat_buff_t( p, "mongoose_main_hand", STAT_AGILITY, 120, 1, 15, 0, 0, false, false, RNG_DISTRIBUTED );
  p -> buffs.mongoose_oh = new stat_buff_t( p, "mongoose_off_hand" , STAT_AGILITY, 120, 1, 15, 0, 0, false, false, RNG_DISTRIBUTED );

  std::string& mh_enchant = p -> items[ SLOT_MAIN_HAND ].encoded_enchant_str;
  std::string& oh_enchant = p -> items[ SLOT_OFF_HAND  ].encoded_enchant_str;

  if ( mh_enchant == "berserking" )
  {
    buff_t* buff = new stat_buff_t( p, "berserking_mh", STAT_ATTACK_POWER, 400, 1, 15, 0, 0, false, false, RNG_DISTRIBUTED );

    p -> register_attack_result_callback( RESULT_HIT_MASK, new berserking_callback_t( p, SLOT_MAIN_HAND, buff ) );
  }
  if ( oh_enchant == "berserking" )
  {
    buff_t* buff = new stat_buff_t( p, "berserking_oh", STAT_ATTACK_POWER, 400, 1, 15, 0, 0, false, false, RNG_DISTRIBUTED );

    p -> register_attack_result_callback( RESULT_HIT_MASK, new berserking_callback_t( p, SLOT_OFF_HAND, buff ) );
  }
  if ( mh_enchant == "executioner" || oh_enchant == "executioner" )
  {
    // MH-OH trigger/refresh the same Executioner buff.  It does not stack.

    buff_t* buff = new stat_buff_t( p, "executioner", STAT_ARMOR_PENETRATION_RATING, 120, 1, 15, 0, 0, false, false, RNG_DISTRIBUTED );

    if ( mh_enchant == "executioner" )
    {
      p -> register_attack_result_callback( RESULT_HIT_MASK, new executioner_callback_t( p, SLOT_MAIN_HAND, buff ) );
    }
    if ( oh_enchant == "executioner" )
    {
      p -> register_attack_result_callback( RESULT_HIT_MASK, new executioner_callback_t( p, SLOT_OFF_HAND, buff ) );
    }
  }
  if ( mh_enchant == "mongoose" )
  {
    p -> register_attack_result_callback( RESULT_HIT_MASK, new mongoose_callback_t( p, SLOT_MAIN_HAND, p -> buffs.mongoose_mh ) );
  }
  if ( oh_enchant == "mongoose" )
  {
    p -> register_attack_result_callback( RESULT_HIT_MASK, new mongoose_callback_t( p, SLOT_OFF_HAND, p -> buffs.mongoose_oh ) );
  }
  if ( mh_enchant == "spellsurge" || 
       oh_enchant == "spellsurge" )
  {
    p -> register_spell_result_callback( RESULT_ALL_MASK, new spellsurge_callback_t( p ) );
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

// enchant_t::download ======================================================

bool enchant_t::download( item_t&            item,
                          const std::string& enchant_id )
{
  item.armory_enchant_str.clear();

  if ( enchant_id.empty() || enchant_id == "" || enchant_id == "0" )
    return true;

  std::string enchant_name;
  if ( get_encoding( enchant_name, item.armory_enchant_str, enchant_id ) )
  {
    armory_t::format( item.armory_enchant_str );
    return true;
  }

  return false;
}
