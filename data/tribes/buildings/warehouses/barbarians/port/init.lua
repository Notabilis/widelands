dirname = path.dirname(__file__)

tribes:new_warehouse_type {
   msgctxt = "barbarians_building",
   name = "barbarians_port",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = pgettext("barbarians_building", "Port"),
   helptext_script = dirname .. "helptexts.lua",
   icon = dirname .. "menu.png",
   size = "port",
   needs_seafaring = true,

   buildcost = {
      log = 3,
      blackwood = 3,
      granite = 5,
      grout = 2,
      iron = 2,
      reed = 4,
      gold = 2
   },
   return_on_dismantle = {
      log = 1,
      blackwood = 2,
      granite = 3,
      grout = 1,
      iron = 1,
      reed = 1,
      gold = 1
   },

   animations = {
      idle = {
         pictures = path.list_files(dirname .. "idle_??.png"),
         hotspot = { 67, 80 },
         fps = 10
      },
      build = {
         pictures = path.list_files(dirname .. "build_??.png"),
         hotspot = { 67, 80 },
      }
   },

   aihints = {
      prohibited_till = 1000
   },

   conquers = 5,
   heal_per_second = 170,
}
