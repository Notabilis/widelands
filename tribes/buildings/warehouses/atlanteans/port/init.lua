dirname = path.dirname(__file__)

tribes:new_warehouse_type {
   msgctxt = "atlanteans_building",
   name = "atlanteans_port",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = pgettext("atlanteans_building", "Port"),
   directory = dirname,
   icon = dirname .. "menu.png",
   size = "port",

   buildcost = {
      log = 3,
      planks = 3,
      granite = 4,
      diamond = 1,
      quartz = 1,
      spidercloth = 3,
      gold = 2
   },
   return_on_dismantle = {
      log = 1,
      planks = 1,
      granite = 2,
      spidercloth = 1,
      gold = 1
   },

   animations = {
      idle = {
         template = "idle_??",
         directory = dirname,
         hotspot = { 74, 70 },
         fps = 10
      },
      build = {
         template = "build_??",
         directory = dirname,
         hotspot = { 74, 70 },
         fps = 1
      }
   },

   aihints = {
      prohibited_till = 900
   },

   conquers = 5,
   heal_per_second = 170,
}