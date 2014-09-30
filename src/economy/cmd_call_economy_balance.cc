/*
 * Copyright (C) 2002-2004, 2006-2009 by the Widelands Development Team
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "economy/cmd_call_economy_balance.h"

#include "base/wexception.h"
#include "economy/economy.h"
#include "io/fileread.h"
#include "io/filewrite.h"
#include "logic/game.h"
#include "logic/player.h"
#include "map_io/map_object_loader.h"
#include "map_io/map_object_saver.h"

namespace Widelands {

CmdCallEconomyBalance::CmdCallEconomyBalance
	(int32_t const starttime, Economy * const economy, uint32_t const timerid)
	: GameLogicCommand(starttime)
{
	m_flag = economy->get_arbitrary_flag();
	m_timerid = timerid;
}

/**
 * Called by Cmd_Queue as requested by start_request_timer().
 * Call economy functions to balance supply and request.
 */
void CmdCallEconomyBalance::execute(Game & game)
{
	if (Flag * const flag = m_flag.get(game))
		flag->get_economy()->balance(m_timerid);
}

#define CURRENT_CMD_CALL_ECONOMY_VERSION 3

/**
 * Read and write
 */
void CmdCallEconomyBalance::read
	(FileRead & fr, EditorGameBase & egbase, MapObjectLoader & mol)
{
	try {
		uint16_t const packet_version = fr.unsigned_16();
		if (packet_version == CURRENT_CMD_CALL_ECONOMY_VERSION) {
			GameLogicCommand::read(fr, egbase, mol);
			uint32_t serial = fr.unsigned_32();
			if (serial)
				m_flag = &mol.get<Flag>(serial);
			m_timerid = fr.unsigned_32();
		} else if (packet_version == 1 || packet_version == 2) {
			GameLogicCommand::read(fr, egbase, mol);
			uint8_t const player_number = fr.unsigned_8();
			if (Player * const player = egbase.get_player(player_number)) {
				if (!fr.unsigned_8())
					throw wexception("0 is not allowed here");
				uint16_t const economy_number = fr.unsigned_16();
				if (economy_number < player->get_nr_economies())
					m_flag =
						player->get_economy_by_number(economy_number)
						->get_arbitrary_flag();
				else
					throw wexception("invalid economy number %u", economy_number);
			} else
				throw wexception("invalid player number %u", player_number);
			if (packet_version >= 2)
				m_timerid = fr.unsigned_32();
			else
				m_timerid = 0;
		} else
			throw GameDataError
				("unknown/unhandled version %u", packet_version);
	} catch (const WException & e) {
		throw wexception("call economy balance: %s", e.what());
	}
}
void CmdCallEconomyBalance::write
	(FileWrite & fw, EditorGameBase & egbase, MapObjectSaver & mos)
{
	fw.unsigned_16(CURRENT_CMD_CALL_ECONOMY_VERSION);

	// Write Base Commands
	GameLogicCommand::write(fw, egbase, mos);
	if (Flag * const flag = m_flag.get(egbase))
		fw.unsigned_32(mos.get_object_file_index(*flag));
	else
		fw.unsigned_32(0);
	fw.unsigned_32(m_timerid);
}

}
