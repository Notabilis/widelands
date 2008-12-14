/*
 * Copyright (C) 2002-2004, 2006-2008 by the Widelands Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "productionsite.h"

#include "editor_game_base.h"
#include "game.h"
#include "helper.h"
#include "i18n.h"
#include "interactive_player.h"
#include "map.h"
#include "player.h"
#include "production_program.h"
#include "profile.h"
#include "request.h"
#include "transport.h"
#include "tribe.h"
#include "wexception.h"
#include "worker.h"
#include "warelist.h"
#include "world.h"

#include "upcast.h"

namespace Widelands {

static const size_t STATISTICS_VECTOR_LENGTH = 10;



/*
==============================================================================

ProductionSite BUILDING

==============================================================================
*/

ProductionSite_Descr::ProductionSite_Descr
	(char const * const _name, char const * const _descname,
	 std::string const & directory, Profile & prof, Section & global_s,
	 Tribe_Descr const & _tribe, EncodeData const * const encdata)
	: Building_Descr(_name, _descname, directory, prof, global_s, _tribe, encdata)
{
	while
		(Section::Value const * const op = global_s.get_next_val("output"))
		try {
			if (Ware_Index const idx = tribe().ware_index(op->get_string())) {
				if (m_output.count(idx))
					throw wexception
						("this ware type has already been declared as an output");
				m_output.insert(idx);
			} else
				throw wexception
					("tribe does not define a ware type with this name");
		} catch (_wexception const & e) {
			throw wexception("output \"%s\": %s", op->get_string(), e.what());
		}

	if (Section * const s = prof.get_section("inputs"))
		while (Section::Value const * const val = s->get_next_val())
			try {
				if (Ware_Index const idx = tribe().ware_index(val->get_name())) {
					container_iterate_const(Ware_Types, inputs(), i)
						if (i.current->first == idx)
							throw wexception("duplicated");
					int32_t const value = val->get_int();
					if (value < 1 or 255 < value)
						throw wexception("count is out of range 1 .. 255");
					m_inputs.push_back(std::pair<Ware_Index, uint8_t>(idx, value));
				} else
					throw wexception
						("tribe does not define a ware type with this name");
			} catch (_wexception const & e) {
				throw wexception
					("input \"%s=%s\": %s",
					 val->get_name(), val->get_string(), e.what());
			}

	// Are we only a production site?
	// If not, we might not have a worker
	if
		(Section * const working_positions_s =
		 	prof.get_section("working positions"))
		while
			(Section::Value const * const v = working_positions_s->get_next_val())
			try {
				if (Ware_Index const woi = tribe().worker_index(v->get_name())) {
					container_iterate_const(Ware_Types, working_positions(), i)
						if (i.current->first == woi)
							throw wexception("duplicated");
					m_working_positions.push_back
						(std::pair<Ware_Index, uint32_t>(woi, v->get_positive()));
				} else
					throw wexception("invalid");
			} catch (_wexception const & e) {
				throw wexception
					("%s=\"%s\": %s", v->get_name(), v->get_string(), e.what());
			}
	if
		(not strcmp(global_s.get_safe_string("type"), "production")
		 and
		 working_positions().empty())
		throw wexception("no working positions although only productionsite");

	// Get programs
	if
		(Section * const programs_s =
		 	prof.get_section("programs"))
	while (Section::Value const * const v = programs_s->get_next_val()) {
		std::string const program_name = v->get_name();
		try {
			if (m_programs.count(program_name))
				throw wexception("this program has already been declared");
			m_programs[program_name] =
				new ProductionProgram
					(directory, prof, program_name, v->get_string(), this, encdata);
		} catch (std::exception const & e) {
			throw wexception("program %s: %s", program_name.c_str(), e.what());
		}
	}
}

ProductionSite_Descr::~ProductionSite_Descr()
{
	while (m_programs.size()) {
		delete m_programs.begin()->second;
		m_programs.erase(m_programs.begin());
	}
}


/**
 * Get the program of the given name.
 */
const ProductionProgram * ProductionSite_Descr::get_program
	(std::string const & program_name) const
{
	Programs::const_iterator const it = programs().find(program_name);
	if (it == m_programs.end())
		throw wexception
			("%s has no program '%s'", name().c_str(), program_name.c_str());
	return it->second;
}

/**
 * Create a new building of this type
 */
Building* ProductionSite_Descr::create_object() const
{return new ProductionSite(*this);}


/*
==============================

IMPLEMENTATION

==============================
*/

ProductionSite::ProductionSite(const ProductionSite_Descr & ps_descr) :
	Building            (ps_descr),
	m_working_positions(new Working_Position[ps_descr.nr_working_positions()]),
m_fetchfromflag     (0),
m_program_timer     (false),
m_program_time      (0),
m_post_timer        (50),
m_statistics        (STATISTICS_VECTOR_LENGTH, false),
m_statistics_changed(true),
m_last_stat_percent (0)
{}

ProductionSite::~ProductionSite() {
	delete[] m_working_positions;
}


/**
 * Display whether we're occupied.
 */
std::string ProductionSite::get_statistics_string()
{
	uint32_t const nr_working_positions = descr().nr_working_positions();
	uint32_t       nr_workers           = 0;
	for (uint32_t i = nr_working_positions; i;)
		nr_workers += m_working_positions[--i].worker ? 1 : 0;
	if (!nr_workers)
		return _("(not occupied)");
	else if (uint32_t const nr_requests = nr_working_positions - nr_workers) {
		char buffer[1000];
		snprintf
			(buffer, sizeof(buffer), _("Waiting for %u workers!"), nr_requests);
		return buffer;
	}

	if (m_statistics_changed)
		calc_statistics();

	if (m_stop)
		return _("(stopped)");

	return m_statistics_buf;
}


void ProductionSite::prefill
	(Game                 &       game,
	 uint32_t       const *       ware_counts,
	 uint32_t       const *       worker_counts,
	 Soldier_Counts const * const soldier_counts)
{
	Building::prefill(game, ware_counts, worker_counts, soldier_counts);
	Tribe_Descr const & tribe = owner().tribe();
	if (ware_counts) {
		//  FIXME filling productionsite's input queues not yet implemented
	}
	if (worker_counts) {
		Working_Position * wp = m_working_positions;
		Ware_Types const & descr_working_positions = descr().working_positions();
		container_iterate_const(Ware_Types, descr_working_positions, i) {
			uint32_t nr_workers = *worker_counts;
			assert(nr_workers <= i.current->second);
			wp += i.current->second - nr_workers;
			Worker_Descr const & wdes = *tribe.get_worker_descr(i.current->first);
			while (nr_workers--) {
				Worker & worker =
					wdes.create(game, owner(), *get_base_flag(), get_position());
				worker.start_task_idle(&game, 0, -1);
				wp->worker = &worker;
				++wp;
			}
			++worker_counts;
		}
	}
}
void ProductionSite::postfill
	(Game &, uint32_t const *, uint32_t const *, Soldier_Counts const *)
{}


/**
 * Calculate statistic.
 */
void ProductionSite::calc_statistics()
{
	uint32_t pos;
	uint32_t ok = 0;
	uint32_t lastOk = 0;

	for (pos = 0; pos < STATISTICS_VECTOR_LENGTH; ++pos) {
		if (m_statistics[pos]) {
			++ok;
			if (pos >= STATISTICS_VECTOR_LENGTH / 2)
				++lastOk;
		}
	}
	double percOk = (ok * 100) / STATISTICS_VECTOR_LENGTH;
	double lastPercOk = (lastOk * 100) / (STATISTICS_VECTOR_LENGTH / 2);

	const std::string trend =
		lastPercOk > percOk ? _("UP") : lastPercOk < percOk ? _("DOWN") : "=";

	if (0 < percOk and percOk < 100)
		snprintf
			(m_statistics_buf, sizeof(m_statistics_buf),
			 "%.0f%% %s", percOk, trend.c_str());
	else snprintf(m_statistics_buf, sizeof(m_statistics_buf), "%.0f%%", percOk);

	m_last_stat_percent = static_cast<char>(percOk); //FIXME: ARGH!

	m_statistics_changed = false;
}


/**
 * Initialize the production site.
 */
void ProductionSite::init(Editor_Game_Base* g)
{
	Building::init(g);

	if (upcast(Game, game, g)) {
		//  Request missing workers.
		Working_Position * wp = m_working_positions;
		container_iterate_const(Ware_Types, descr().working_positions(), i) {
			Ware_Index const worker_index = i.current->first;
			for (uint32_t j = i.current->second; j; --j, ++wp)
				if (Worker * const worker = wp->worker)
					worker->set_location(this);
				else
					wp->worker_request = &request_worker(worker_index);
		}

		// Init input ware queues
		Ware_Types const & inputs = descr().inputs();
		m_input_queues.reserve(inputs.size());
		container_iterate_const(Ware_Types, inputs, i)
			m_input_queues.push_back(&(new WaresQueue(this))->init(*i.current));

		try_start_working(*game);
	}
}

/**
 * Change the economy for the wares queues.
 *
 * \note Workers are dealt with in the PlayerImmovable code.
 */
void ProductionSite::set_economy(Economy* e)
{
	if (Economy * const old = get_economy()) {
		for (uint32_t i = 0; i < m_input_queues.size(); ++i)
			m_input_queues[i]->remove_from_economy(old);
	}

	Building::set_economy(e);
	for (uint32_t i = descr().nr_working_positions(); i;)
		if (Request * const r = m_working_positions[--i].worker_request)
			r->set_economy(e);

	if (e)
		container_iterate_const(std::vector<WaresQueue *>, m_input_queues, i)
			(*i.current)->add_to_economy(e);
}

/**
 * Cleanup after a production site is removed
 */
void ProductionSite::cleanup(Editor_Game_Base* g)
{
	for (uint32_t i = descr().nr_working_positions(); i;) {
		--i;
		delete m_working_positions[i].worker_request;
		Worker * const w = m_working_positions[i].worker;

		//  Ensure we do not re-request the worker when remove_worker is called.
		m_working_positions[i].worker = 0;

		// Actually remove the worker
		if (g->objects().object_still_available(w))
			w->set_location(0);
	}

	// Cleanup the wares queues
	for (uint32_t i = 0; i < m_input_queues.size(); ++i) {
		m_input_queues[i]->cleanup();
		delete m_input_queues[i];
	}
	m_input_queues.clear();


	Building::cleanup(g);
}


/**
 * Intercept remove_worker() calls to unassign our worker, if necessary.
 */
void ProductionSite::remove_worker(Worker* w)
{
	molog("%s leaving\n", w->descname().c_str());
	Working_Position * current = m_working_positions;
	for
		(Working_Position * const end = current + descr().nr_working_positions();
		 current < end;
		 ++current)
		if (current->worker == w) {
			*current =
				Working_Position
					(&request_worker
					 	(descr().tribe().worker_index(w->name().c_str())),
					 0);
			break;
		}

	Building::remove_worker(w);
}


/**
 * Issue the worker requests
 */
Request & ProductionSite::request_worker(Ware_Index const wareid) {
	return
		*new Request
			(this,
			 wareid,
			 &ProductionSite::request_worker_callback,
			 this,
			 Request::WORKER);
}


/**
 * Called when our worker arrives.
 */
void ProductionSite::request_worker_callback
	(Game * g, Request * rq, Ware_Index, Worker * const w, void * const data)
{
	ProductionSite & psite = *static_cast<ProductionSite *>(data);

	assert(w);
	assert(w->get_location(g) == &psite);

	for (Working_Position * wp = psite.m_working_positions;; ++wp)
		//  Assume that rq must be in worker_requests.
		if (wp->worker_request == rq) {
			delete rq;
			*wp = Working_Position(0, w);
			break;
		}

	// It's always the first worker doing building work,
	// the others only idle. Still, we need to wake up the
	// primary worker if the worker that has just arrived is
	// the last one we need to start working.
	w->start_task_idle(g, 0, -1);
	psite.try_start_working(*g);
}


/**
 * Advance the program state if applicable.
 */
void ProductionSite::act(Game* g, uint32_t data)
{
	Building::act(g, data);

	if
		(m_program_timer
		 and
		 static_cast<int32_t>(g->get_gametime() - m_program_time) >= 0)
	{
		m_program_timer = false;

		if (!m_program.size())
			return find_and_start_next_program(*g);

		State* state = get_current_program();

		assert(state);

		if (state->ip >= state->program->get_size())
			return program_end(*g, Completed);

		if (m_anim != descr().get_animation("idle")) {
			// Restart idle animation, which is the default
			start_animation(g, descr().get_animation("idle"));
		}

		return program_act(*g);
	}
}


void ProductionSite::find_and_start_next_program(Game & game)
{
	program_start(game, "work");
}


/**
 * Perform the current program action.
 *
 * \pre The program is running and in a valid state.
 * \post (Potentially indirect) scheduling for the next step has been done.
 */
void ProductionSite::program_act(Game & game)
{
	assert(get_current_program());
	State & state = *get_current_program();
#if 0
	molog
		("PSITE: program %s#%i\n", state.program->get_name().c_str(), state.ip);
#endif
	if (m_stop) {
		program_end(game, Failed);
		m_program_timer = true;
		m_program_time = schedule_act(&game, 20000);
	} else
		(*state.program)[state.ip].execute(game, *this);
}


/**
 * Remember that we need to fetch an item from the flag.
 */
bool ProductionSite::fetch_from_flag(Game* g)
{
	++m_fetchfromflag;

	if (can_start_working())
		m_working_positions[0].worker->update_task_buildingwork(g);

	return true;
}

/**
 * \return True if this production site could theoretically start working (if
 * all workers are present)
 */
bool ProductionSite::can_start_working() const throw ()
{
	for (uint32_t i = descr().nr_working_positions(); i;)
		if (m_working_positions[--i].worker_request)
			return false;
	return true;
}


void ProductionSite::try_start_working(Game & game) {
	if (can_start_working() and descr().working_positions().size()) {
		Worker & main_worker = *m_working_positions[0].worker;
		main_worker.reset_tasks(&game);
		main_worker.start_task_buildingwork(&game);
	}
}

/**
 * There's currently nothing to do for the worker.
 *
 * \note We assume that the worker is inside the building when this is called.
 */
bool ProductionSite::get_building_work(Game* g, Worker* w, bool success)
{
	assert(descr().working_positions().size());
	assert(w == m_working_positions[0].worker);

	State* state = get_current_program();

	// If unsuccessful: Check if we need to abort current program
	if (!success && state)
		if
			(dynamic_cast<ProductionProgram::ActWorker const *>
			 	(&(*state->program)[state->ip]))
		{
			program_end(*g, Failed);
			state = 0;
		}

	// Default actions first
	if (WareInstance * const item = w->fetch_carried_item(g)) {

		w->start_task_dropoff(*g, *item);
		return true;
	}

	if (m_fetchfromflag) {
		--m_fetchfromflag;
		w->start_task_fetchfromflag(g);
		return true;
	}

	if (m_produced_items.size()) {
		//  There is still a produced item waiting for delivery. Carry it out
		//  before continuing with the program.
		std::pair<Ware_Index, uint8_t> & ware_type_with_count =
			*m_produced_items.rbegin();
		{
			Ware_Index const ware_index = ware_type_with_count.first;
			Tribe_Descr const & tribe = owner().tribe();
			Item_Ware_Descr const & item_ware_descr =
				*tribe.get_ware_descr(ware_type_with_count.first);
			{
				WareInstance & item =
					*new WareInstance(ware_index, &item_ware_descr);
				item.init(g);
				w->start_task_dropoff(*g, item);
			}
			owner().ware_produced(ware_index); //  for statistics
		}
		assert(ware_type_with_count.second);
		if (--ware_type_with_count.second == 0)
			m_produced_items.pop_back();
		return true;
	}

	// Check if all workers are there
	if (!can_start_working())
		return false;

	// Start program if we haven't already done so
	if (!state)
	{
		m_program_timer = true;
		m_program_time = schedule_act(g, 10);
	}
	else if (state->ip < state->program->get_size())
	{
		ProductionProgram::Action const & action = (*state->program)[state->ip];

		if (upcast(ProductionProgram::ActWorker const, worker_action, &action)) {
			if (state->phase == 0) {
				w->start_task_program(g, worker_action->program());
				++state->phase;
				return true;
			} else
				program_step(*g);
		} else if (dynamic_cast<ProductionProgram::ActProduce const *>(&action))
		{
			//  All the wares that we produced have been carried out, so continue
			//  with the program.
			program_step(*g);
		}
	}
	return false;
}


/**
 * Advance the program to the next step.
 */
void ProductionSite::program_step
	(Game & game, uint32_t const delay, uint32_t const phase)
{
	State* state = get_current_program();

	assert(state);

	++state->ip;
	state->phase = phase;
	m_program_timer = true;
	m_program_time  = schedule_act(&game, delay);
}


/**
 * Push the given program onto the stack and schedule acting.
 */
void ProductionSite::program_start
	(Game & game, std::string const & program_name)
{
	State state;

	state.program = descr().get_program(program_name);
	state.ip = 0;
	state.phase = 0;

	m_program.push_back(state);

	m_program_timer = true;
	m_program_time = schedule_act(&game, 10);
}


/**
 * Ends the current program now and updates the productivity statistics.
 *
 * \pre Any program is running
 * \post No program is running, acting is scheduled
 */
void ProductionSite::program_end(Game & game, Program_Result const result)
{
	assert(m_program.size());

	m_program.pop_back();
	if (m_program.size())
		m_program.rbegin()->phase = result;

	if (result != Skipped) {
		m_statistics_changed = true;
		m_statistics.erase(m_statistics.begin(), m_statistics.begin() + 1);
		m_statistics.push_back(result == Completed);

		if (result == Completed)
			for (uint32_t i = descr().nr_working_positions(); i;)
				m_working_positions[--i].worker->gain_experience(game);
	}

	m_program_timer = true;
	m_program_time = schedule_act(&game, m_post_timer);
}

};
