dirname = path.dirname(__file__)

tribes:new_militarysite_type {
   msgctxt = "empire_building",
   name = "empire_fortress",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = pgettext("empire_building", "Fortress"),
   helptext_script = dirname .. "helptexts.lua",
   icon = dirname .. "menu.png",
   size = "big",
   enhancement = "empire_castle",

   buildcost = {
      planks = 5,
      log = 2,
      granite = 8,
      marble = 2,
      marble_column = 2
   },
   return_on_dismantle = {
      planks = 2,
      log = 1,
      granite = 5,
      marble = 1,
      marble_column = 1
   },

   animations = {
      idle = {
         pictures = path.list_files(dirname .. "idle_??.png"),
         hotspot = { 90, 105 }
      },
      build = {
         pictures = path.list_files(dirname .. "build_??.png"),
         hotspot = { 90, 105 },
      }
   },

   aihints = {
      expansion = true,
      fighting = true,
      mountain_conqueror = true,
      prohibited_till=1400
   },

   max_soldiers = 8,
   heal_per_second = 170,
   conquers = 11,
   prefer_heroes = true,

   messages = {
      occupied = _"Your soldiers have occupied your fortress.",
      aggressor = _"Your fortress discovered an aggressor.",
      attack = _"Your fortress is under attack.",
      defeated_enemy = _"The enemy defeated your soldiers at the fortress.",
      defeated_you = _"Your soldiers defeated the enemy at the fortress."
   },
}
