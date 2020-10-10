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

#include <boost/signals2/signal.hpp>

#include "logic/widelands.h"

// Avoid as many includes as possible
class GameSettings;
class UserSettings;
class ComputerPlayer;
class RGBColor;

namespace Widelands {
	class Game;
	class Player;
}

/**
 * Class interface to provide access to the lists of network game participants to the UI.
 * @note This is a list of participants, not of players. Defeated players and spectators are
 *       also handled by this, as well as AIs. The players are indirectly present since each
 *      player is always controlled by a human user or an AI.
 *       If a player slot is closed, it will not show up here.
 * @note The \c participant indices used by this class are not compatible to other indices,
 *       e.g., in GameSettings::players.
 * When passing a \c participant index, the results are ordered: First all human participants
 * are returned, then the AIs.
 */
class ParticipantList {

public:

	/// Describes the role of a participant inside the game
	enum class ParticipantType {
		/// A human user currently controlling a tribe
		kPlayer,
		/// An AI controlling a tribe
		kAI,
		/// A human spectator of the game
		kSpectator
	};

	/**
	 * Constructor.
	 * @param settings The settings of the current network game.
	 *                 The game might be in the lobby or already started.
	 * @param game A *reference* to a pointer to the game. This way the pointer might be \c nullptr
	 *             when creating the class but will get the right value later on by itself.
	 *             Also, the class can check for \c nullptr this way -> whether a game is running.
	 * @param localplayername The name of the network player on this computer.
	 */
	ParticipantList(const GameSettings* settings, Widelands::Game*& game,
					const std::string& localplayername);

	// The methods do not return lists on purpose since the data isn't stored in
	// lists within GameHost/GameClient either. Creating lists here and keeping
	// them updated is just asking for trouble

	/**
	 * Returns the number of currently connected participants.
	 * Return value [2] - 1 is the highest permitted participant number for the other methods.
	 * @return An pointer to an array with 3 elements containing the numbers of connected
	 *         participants for humans, AIs, and a total.
	 */
	const int16_t* get_participant_counts() const;

	/**
	 * Returns the type of participant.
	 * @param participant The number of the participant get data about.
	 * @return The type of participant.
	 */
	ParticipantType get_participant_type(int16_t participant) const;

	/**
	 * Returns the team of the participant when the participant is a player.
	 * A value of \c 0 indicates that the participant has no team.
	 * For spectators, the result is undefined.
	 * @param participant The number of the participant to get data about.
	 * @return The team of player used by the participant.
	 */
	Widelands::TeamNumber get_participant_team(int16_t participant) const;

	/**
	 * Returns whether the local player has someone to chat in its team.
	 * For spectators this is true if there are other spectators.
	 * For players this is true when there are teammembers that are no AI.
	 * For player slots shared between multiple humans, this is always true.
	 * @param participant The number of the participant to get data about.
	 * @return Whether the participant has someone to chat in its team.
	 */
	bool needs_teamchat() const;

	/**
	 * Returns the name of the local player.
	 * I.e., the player on the current computer.
	 * @note This method is much more efficient than getting the name for the local playerindex.
	 * @return The player name.
	 */
	const std::string& get_local_playername() const;

	/**
	 * Returns the participant index of the local player.
	 * Might return -1 (invalid index) when the current state of the game settings is invalid.
	 * This can happen for a moment after a client connected to a game.
	 * @note This method is relatively expensive (looping through players).
	 * @return The index of the local player or -1.
	 */
	int16_t get_local_playerindex() const;

	/**
	 * Returns the name of the participant.
	 * This is the name the participant provided when connecting to the server.
	 * The name is also used to display chat messages by this participant.
	 * For AIs this is a descriptiv name of the AI.
	 * @param participant The number of the participant get data about.
	 * @return The name of the participant.
	 */
	const std::string& get_participant_name(int16_t participant) const;

	/**
	 * Returns whether the player used by the participant is defeated or still playing.
	 * For spectators, the result is undefined.
	 * @warning Must only be called when ingame.
	 * @param participant The number of the participant get data about.
	 * @return Whether the participant has been defeated.
	 */
	bool get_participant_defeated(int16_t participant) const;

	/**
	 * Returns the color of the player used by the participant.
	 * For spectators, the result is undefined.
	 * @param participant The number of the participant get data about.
	 * @return The playercolor.
	 */
	const RGBColor& get_participant_color(int16_t participant) const;

	/**
	 * Returns whether a game is currently running.
	 * @return Whether a game is running.
	 */
	bool is_ingame() const;

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
	uint8_t get_participant_ping(int16_t participant) const;

	/// Called when the data was updated and should be re-fetched and redrawn
	boost::signals2::signal<void()> participants_updated;

	/**
	 * Called when the RTT for a participant changed.
	 * Passed parameters are the participant number and the new RTT.
	 */
	boost::signals2::signal<void(int16_t, uint8_t)> participant_updated_rtt;

private:

	/**
	 * Fetches the UserSettings belonging to the given participant index.
	 * The caller has to make sure that \c participant refers to a human user.
	 * @param participant The index to fetch the data for.
	 * @return The UserSettings entry for the given participant.
	 */
	const UserSettings& participant_to_user(int16_t participant) const;

	/**
	 * Gets the index within GameSettings::players that belongs to the given participant index.
	 * \c participant might refer to a human or AI, but it must be a player and no spectator.
	 * @param participant The index to fetch the data for.
	 * @return The index within GameSettings::players for the given participant.
	 */
	int32_t participant_to_playerindex(int16_t participant) const;

	/**
	 * Fetches the Player belonging to the given participant index.
	 * \c participant might refer to a human or AI, but it must be a player and no spectator.
	 * @note This method must only be used while in a game.
	 * @param participant The index to fetch the data for.
	 * @return A pointer to the Player entry for the given participant.
	 */
	const Widelands::Player* participant_to_player(int16_t participant) const;

	/// A reference to the settings of the current game (running or being prepared).
	const GameSettings* settings_;
	/// A reference to the pointer of a currently runnning game.
	Widelands::Game*& game_;
	/// A reference to the user name of the human on this computer.
	const std::string& localplayername_;
	/// Counts of participants: humans, AIs, total
	mutable int16_t participant_counts_[3];
};

#endif // WL_NETWORK_PARTICIPANTLIST_H

