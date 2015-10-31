dirname = path.dirname(__file__)

animations = {
   idle = {
      template = "idle_??",
      directory = dirname,
      hotspot = { 3, 23 }
   }
}
add_worker_animations(animations, "walk", dirname, "walk", {10, 24}, 10)
add_worker_animations(animations, "walkload", dirname, "walkload", {10, 24}, 10)


tribes:new_worker_type {
   msgctxt = "atlanteans_worker",
   name = "atlanteans_miner",
   -- TRANSLATORS: This is a worker name used in lists of workers
   descname = pgettext("atlanteans_worker", "Miner"),
   directory = dirname,
   icon = dirname .. "menu.png",
   vision_range = 2,

   buildcost = {
      atlanteans_carrier = 1,
      pick = 1
   },

   animations = animations,
}