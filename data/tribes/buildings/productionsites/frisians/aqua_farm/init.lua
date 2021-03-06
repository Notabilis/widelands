dirname = path.dirname(__file__)

tribes:new_productionsite_type {
   msgctxt = "frisians_building",
   name = "frisians_aqua_farm",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = pgettext("frisians_building", "Aqua Farm"),
   helptext_script = dirname .. "helptexts.lua",
   icon = dirname .. "menu.png",
   size = "small",

   buildcost = {
      brick = 2,
      log = 1,
      reed = 1
   },
   return_on_dismantle = {
      brick = 1,
      log = 1
   },

   animations = {
      idle = {
         pictures = path.list_files (dirname .. "idle_??.png"),
         hotspot = {49, 88},
         fps = 10,
      },
      unoccupied = {
         pictures = path.list_files (dirname .. "unoccupied_?.png"),
         hotspot = {49, 66},
      },
   },

   aihints = {
      collects_ware_from_map = "fish",
      prohibited_till = 760,
      requires_supporters = true
   },

   working_positions = {
      frisians_fisher = 1
   },

   inputs = {
      { name = "fruit", amount = 8 },
      { name = "water", amount = 8 },
   },
   outputs = {
      "fish"
   },

   indicate_workarea_overlaps = {
      frisians_aqua_farm = false,
      frisians_clay_pit = true,
   },

   programs = {
      work = {
         -- TRANSLATORS: Completed/Skipped/Did not start working because ...
         descname = _"working",
         actions = {
            "call=breed_fish",
            "call=fish_pond",
            "return=no_stats",
         },
      },
      breed_fish = {
         -- TRANSLATORS: Completed/Skipped/Did not start breeding fish because ...
         descname = _"breeding fish",
         actions = {
            "return=skipped unless economy needs fish",
            "return=failed unless site has water:2",
            "return=failed unless site has fruit",
            "callworker=breed_in_pond",
            "consume=fruit water:2",
            "sleep=23000",
         },
      },
      fish_pond = {
         -- TRANSLATORS: Completed/Skipped/Did not start fishing because ...
         descname = _"fishing",
         actions = {
            "return=skipped unless economy needs fish",
            "sleep=9000",
            "callworker=fish_in_pond",
         },
      },
   },

   out_of_resource_notification = {
      -- Translators: Short for "Out of ..." for a resource
      title = _"No Ponds",
      heading = _"Out of Fish Ponds",
      message = pgettext ("frisians_building", "The fisher working at this aqua farm can’t find any fish ponds in his work area. Please make sure there is a working clay pit nearby and the aqua farm is supplied with all needed wares, or consider dismantling or destroying this building."),
      productivity_threshold = 12
   },
}
