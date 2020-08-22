/*
 * Copyright (C) 2008-2020 by the Widelands Development Team
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

#ifndef WL_NETWORK_PARTICIPANTLIST_H
#define WL_NETWORK_PARTICIPANTLIST_H

#include <string>

//#include "logic/widelands.h"

/// Base class interface to provide access to the lists of participants to the UI.
/// Is implemented by GameHost / GameClient to provide a clear interface.
/// @note This is a list of participants, not of players. Defeated players and observers are
///       also handled by this, as well as AIs. The players are indirectly present since each
///       player is always controlled by a human user or an AI.
///       If a player slot is closed before starting the game, it will not show up here.
class ParticipantList {

public:

	/// Describes the role of a participant inside the game
	enum class ParticipantType {
		/// A human user currently controlling a tribe
		kPlayer,
		/// An AI controlling a tribe
		kAI,
		/// A human observer of the game
		kObserver
	};

	// The methods do not return lists on purpose since the data isn't stored in
	// lists within GameHost either. Creating lists here and keeping them updated
	// is just asking for trouble

	/**
	 * Returns the number of currently connected participants.
	 * Return value - 1 is the highest permitted participant number for the other methods.
	 * @return The number of connected participants.
	 */
	virtual int16_t get_participant_count() const = 0;

	/**
	 * Returns the type of participant.
	 * @param participant The number of the participant get data about.
	 * @return The type of participant.
	 */
	virtual ParticipantType get_participant_type(int16_t participant) const = 0;

	/**
	 * Returns the team of the participant when the participant is a player.
	 * A value of \c 0 indicates that the participant has no team.
	 * For observers, the result is undefined.
	 * @param participant The number of the participant to get data about.
	 * @return The team of player used by the participant.
	 */
	virtual Widelands::TeamNumber get_participant_team(int16_t participant) const = 0;

	/**
	 * Returns the name of the local player.
	 * I.e., the player on the current computer.
	 * @return The player name.
	 */
	virtual const std::string& get_local_playername() const = 0;

	/**
	 * Returns the name of the participant.
	 * This is the name the participant provided when connecting to the server.
	 * The name is also used to display chat messages by this participant.
	 * For AIs this is a descriptiv name of the AI.
	 * @param participant The number of the participant get data about.
	 * @return The name of the participant.
	 */
	virtual const std::string& get_participant_name(int16_t participant) const = 0;

	/**
	 * Returns whether the player used by the participant is defeated or still playing.
	 * For observers, the result is undefined.
	 * @param participant The number of the participant get data about.
	 * @return Whether the participant has been defeated.
	 */
	virtual bool get_participant_defeated(int16_t participant) const = 0;

	virtual const RGBColor& get_participant_color(int16_t participant) const = 0;

	/**
	 * Returns the ping time of the participant.
	 * Returned is the time that it took the client to return a PING request by the network
	 * relay.
	 * For AI participant the result is undefined.
	 * In network games that don't use the network relay the result is undefined.
	 * @param participant The number of the participant get data about.
	 * @return The RTT in milliseconds for this participant up to 255ms.
	 */
	// TODO(Notabilis): Add support for LAN games
	virtual uint8_t get_participant_ping(int16_t participant) const = 0;

	/// Called when the data was updated and should be re-fetched and redrawn
	boost::signals2::signal<void()> participants_updated;
	/**
	 * Called when the RTT for a participant changed.
	 * Passed parameters are the participant number and the new RTT.
	 */
	boost::signals2::signal<void(int16_t, uint8_t)> participant_updated_rtt;

	// Dropdown Menu variants:
	//  kTextual, kTextualNarrow, kPictorial, kPictorialMenu };
	// kTextual: Text of selected entry and drop-down arrow
	// kTextualNarrow: Text of selected entry
	// kPictorial: Icon of the selected entry
	// kPictorialMenu: Displays the title defined above when not enough space. Be buggy with a lot of space
};

#endif // WL_NETWORK_PARTICIPANTLIST_H

