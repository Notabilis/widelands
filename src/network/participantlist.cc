#include "network/participantlist.h"

#include "ai/computer_player.h"
#include "logic/game.h"
#include "logic/game_settings.h"
#include "logic/player.h"
#include "logic/playersmanager.h"

ParticipantList::ParticipantList(const GameSettings* settings, Widelands::Game*& game,
									const std::string& localplayername)
			: settings_(settings), game_(game), localplayername_(localplayername),
				participant_counts_{-1} {
	assert(settings_ != nullptr);
	// The pointer referenced by game_ might be undefined here
}

const int16_t* ParticipantList::get_participant_counts() const {
printf("get_participant_counts()\n");
	// Number of connected humans
	participant_counts_[0] = 0;
	for (const UserSettings& u : settings_->users) {
printf("get_count() name=%s\n", u.name.c_str());
		// settings_->users might contain disconnected humans, filter them out
		if (u.position != UserSettings::not_connected()) {
printf("get_count() using\n");
			++participant_counts_[0];
		}
else printf("get_count() not using\n");
	}
	participant_counts_[1] = 0;
	for (size_t i = 0; i < settings_->players.size(); ++i) {
		const PlayerSettings& player = settings_->players[i];
		if (player.state != PlayerSettings::State::kComputer) {
			// Ignore open, shared or human player slots
			continue;
		}
		++participant_counts_[1];
	}
	assert(participant_counts_[0] <= static_cast<int16_t>(settings_->users.size()));
	participant_counts_[2] = participant_counts_[0] + participant_counts_[1];
	return participant_counts_;
}

ParticipantList::ParticipantType ParticipantList::get_participant_type(int16_t participant) const {
	assert(participant < participant_counts_[2]);
	if (participant >= participant_counts_[0]) {
		return ParticipantList::ParticipantType::kAI;
	}
	if (participant_to_user(participant).position == UserSettings::none()) {
		return ParticipantList::ParticipantType::kSpectator;
	}
	return ParticipantList::ParticipantType::kPlayer;
}

Widelands::TeamNumber ParticipantList::get_participant_team(int16_t participant) const {
	const size_t index = participant_to_playerindex(participant);
	assert(index <= settings_->players.size());
	const Widelands::TeamNumber team = settings_->players[index].team;
#ifndef NDEBUG
	if (is_ingame()) {
		/// NOCOM: Remove
		assert(team == participant_to_player(participant)->team_number());
	}
#endif // NDEBUG
	return team;
}

const std::string& ParticipantList::get_participant_name(int16_t participant) const {
	if (participant < participant_counts_[0]) {
		// We can't use the Player entry for normal users since it isn't the selected user name
		// but instead a combined name of all the users for this player
		return participant_to_user(participant).name;
	}
	// It is an AI player. Get its type and resolve it to a pretty name
	const PlayerSettings& ps = settings_->players[participant_to_playerindex(participant)];
	assert(ps.state == PlayerSettings::State::kComputer);
#ifndef NDEBUG
	if (is_ingame()) {
		/// NOCOM: Remove
		const Widelands::Player* p = participant_to_player(participant);
		assert(p->get_ai() == ps.ai);
	}
#endif // NDEBUG
	return ComputerPlayer::get_implementation(ps.ai)->descname ;
}


bool ParticipantList::needs_teamchat() const {
	if (participant_counts_[0] <= 1) {
		// If <0: Not initialized yet
		// If 0: We have just connected and don't know anything about the users yet
		// If 1: We are the only participant, so there can't be any teams
		return false;
	}

	const int16_t my_index = get_local_playerindex();
	assert(my_index >= 0);
	assert(my_index < participant_counts_[0]);

	// Check whether we are a player or a spectator
	bool found_someone = false;
	if (get_participant_type(my_index) == ParticipantType::kSpectator) {
		// We are a spectator. Check if there are other spectators
		for (int16_t i = 0; i < participant_counts_[0]; ++i) {
			if (get_participant_type(i) == ParticipantType::kSpectator) {
				if (found_someone) {
					// The first one we find might be we. If we find a second one,
					// we know that there is a teammate
					return true;
				}
				found_someone = true;
			}
		}
	} else {
		// We are a player. Get our team
		const Widelands::TeamNumber my_team = get_participant_team(my_index);
		if (my_team == 0) {
			// Team 0 is the "no team" entry
			// TODO(Notabilis): Check whether we are a shared player
			return false;
		}
		// Search for other players with the same team
		for (int16_t i = 0; i < participant_counts_[0]; ++i) {
			if (get_participant_type(i) != ParticipantType::kPlayer) {
				// Skip spectators, they are no team players
				continue;
			}
			if (get_participant_team(i) == my_team) {
				if (found_someone) {
					return true;
				}
				found_someone = true;
			}
		}
	}
	return false;
}

const std::string& ParticipantList::get_local_playername() const {
	return localplayername_;
}

int16_t ParticipantList::get_local_playerindex() const {
	assert(!localplayername_.empty());
	// Find our player index
	for (int16_t my_index = 0; my_index < participant_counts_[0]; ++my_index) {
		if (get_participant_name(my_index) == localplayername_) {
			return my_index;
		}
	}
	// Not found, return error
	return -1;
}

bool ParticipantList::get_participant_defeated(int16_t participant) const {
	assert(is_ingame());
	return participant_to_player(participant)->is_defeated();
}

const RGBColor& ParticipantList::get_participant_color(int16_t participant) const {
	assert(get_participant_type(participant) != ParticipantList::ParticipantType::kSpectator);
	// Partially copied code from Player class, but this way also works in lobby
	return kPlayerColors[participant_to_playerindex(participant)];
}

bool ParticipantList::is_ingame() const {
	return (game_ != nullptr);
}

uint8_t ParticipantList::get_participant_ping(int16_t participant) const {
	assert(participant < participant_counts_[2]);
	// TODO(Notabilis): Implement this function ... and all the Ping-stuff that belongs to it
	// - Maybe show two RTTs per player: To the host and to the netrelay
	// - Offer "autoUpdatePings(bool)" method to have the ping results be periodically refreshed
	// - Add support for the ping signal
	return 0;
}

const UserSettings& ParticipantList::participant_to_user(int16_t participant) const {
	assert(participant < participant_counts_[0]);
	assert(participant < static_cast<int16_t>(settings_->users.size()));
	for (const UserSettings& u : settings_->users) {
		if (u.position == UserSettings::not_connected()) {
			continue;
		}
		if (participant == 0) {
			return u;
		}
		--participant;
	}
	NEVER_HERE();
}

int32_t ParticipantList::participant_to_playerindex(int16_t participant) const {
	if (participant >= participant_counts_[0]) {
		// AI
		participant -= participant_counts_[0];
		assert(settings_ != nullptr);
		assert(participant >= 0);
		assert(static_cast<size_t>(participant) < settings_->players.size());
		for (size_t i = 0; i < settings_->players.size(); ++i) {
			const PlayerSettings& player = settings_->players[i];
			if (player.state != PlayerSettings::State::kComputer) {
				// Ignore open, shared or human player slots
				continue;
			}
			// Found a non-empty player slot
			if (participant == 0) {
				return i;
			}
			--participant;
			assert(participant >= 0);
		}
		NEVER_HERE();
	} else {
		// Human user

		// No useful result possible for spectators or semi-connected users
		assert(participant_to_user(participant).position <= UserSettings::highest_playernum());

		// .position is the index within settings_->players and also
		// as .position+1 the index inside game_->player_manager()
		return participant_to_user(participant).position;
	}
}

const Widelands::Player* ParticipantList::participant_to_player(int16_t participant) const {
	assert(participant < participant_counts_[2]);
	assert(game_ != nullptr);
	const Widelands::PlayersManager *pm = game_->player_manager();
	assert(pm);
	const int32_t playerindex = participant_to_playerindex(participant);
	assert(playerindex >= 0);
	const Widelands::Player *p = pm->get_player(playerindex + 1);
	assert(p);
	return p;
}
