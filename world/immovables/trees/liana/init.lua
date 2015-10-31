dirname = path.dirname(__file__)

terrain_affinity = {
   preferred_temperature = 120,
   preferred_humidity = 0.2,
   preferred_fertility = 0.7,
   pickiness = 0.6,
}

world:new_immovable_type{
   name = "liana_wasteland_sapling",
   -- TRANSLATORS: This is a fictitious tree. Be creative if you want.
   descname = _ "Liana Tree (Sapling)",
   editor_category = "trees_wasteland",
   size = "small",
   attributes = { "tree_sapling" },
   terrain_affinity = terrain_affinity,
   programs = {
      program = {
         "animate=idle 60000",
         "remove=40",
         "grow=liana_wasteland_pole",
      },
   },
   animations = {
      idle = {
         template = "idle_?",
         directory = dirname .. "sapling/",
         hotspot = { 5, 12 },
         fps = 8,
      },
   },
}

world:new_immovable_type{
   name = "liana_wasteland_pole",
   -- TRANSLATORS: This is a fictitious tree. Be creative if you want.
   descname = _ "Liana Tree (Pole)",
   editor_category = "trees_wasteland",
   size = "small",
   attributes = {},
   terrain_affinity = terrain_affinity,
   programs = {
      program = {
         "animate=idle 55000",
         "remove=30",
         "grow=liana_wasteland_mature",
      },
   },
   animations = {
      idle = {
         template = "idle_?",
         directory = dirname .. "pole/",
         hotspot = { 12, 28 },
         fps = 8,
      },
   },
}

world:new_immovable_type{
   name = "liana_wasteland_mature",
   -- TRANSLATORS: This is a fictitious tree. Be creative if you want.
   descname = _ "Liana Tree (Mature)",
   editor_category = "trees_wasteland",
   size = "small",
   attributes = {},
   terrain_affinity = terrain_affinity,
   programs = {
      program = {
         "animate=idle 55000",
         "remove=10",
         "seed=liana_wasteland_sapling",
         "animate=idle 30000",
         "remove=10",
         "grow=liana_wasteland_old",
      },
   },
   animations = {
      idle = {
         template = "idle_?",
         directory = dirname .. "mature/",
         hotspot = { 18, 48 },
         fps = 8,
      },
   },
}

world:new_immovable_type{
   name = "liana_wasteland_old",
   -- TRANSLATORS: This is a fictitious tree. Be creative if you want.
   descname = _ "Liana Tree (Old)",
   editor_category = "trees_wasteland",
   size = "small",
   attributes = { "tree" },
   terrain_affinity = terrain_affinity,
   programs = {
      program = {
         "animate=idle 1455000",
         "transform=deadtree4 48",
         "seed=liana_wasteland_sapling",
      },
      fall = {
         "remove=",
      },
   },
   animations = {
      idle = {
         template = "idle_?",
         directory = dirname .. "old/",
         hotspot = { 24, 60 },
         fps = 10,
      },
   },
}