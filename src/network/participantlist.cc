#include "network/participantlist.h"

#include "ai/computer_player.h"
#include "logic/game.h"
#include "logic/game_settings.h"
#include "logic/player.h"
#include "logic/playersmanager.h"

ParticipantList::ParticipantList(const GameSettings* settings, Widelands::Game*& game,
									const std::string& localplayername)
			: settings_(settings), game_(game), localplayername_(localplayername),
				human_user_count_(0) {
	assert(settings_ != nullptr);
	// The pointer referenced by game_ might be undefined here
#ifndef NDEBUG
	participant_count_ = 0;
#endif // NDEBUG
}

int16_t ParticipantList::get_participant_count() const {
	// Number of connected humans
	human_user_count_ = 0;
	for (const UserSettings& u : settings_->users) {
printf("get_count() name=%s\n", u.name.c_str());
		// settings_->users might contain disconnected humans, filter them out
		if (u.position != UserSettings::not_connected()) {
printf("get_count() using\n");
			++human_user_count_;
		}
else printf("get_count() not using\n");
	}
	int16_t n_ais = 0;
	for (size_t i = 0; i < settings_->players.size(); ++i) {
		const PlayerSettings& player = settings_->players[i];
		if (player.state != PlayerSettings::State::kComputer) {
			// Ignore open, shared or human player slots
			continue;
		}
		++n_ais;
	}
printf("users = %lu, active users = %i, AIs = %i\n", settings_->users.size(), human_user_count_, n_ais);
	assert(human_user_count_ <= static_cast<int16_t>(settings_->users.size()));
#ifndef NDEBUG
	participant_count_ = human_user_count_ + n_ais;
#endif // NDEBUG
	return human_user_count_ + n_ais;
}

ParticipantList::ParticipantType ParticipantList::get_participant_type(int16_t participant) const {
	assert(participant < participant_count_);
	if (participant >= human_user_count_) {
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
	if (participant < human_user_count_) {
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

const std::string& ParticipantList::get_local_playername() const {
	return localplayername_;
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
	assert(participant < participant_count_);
	// TODO(Notabilis): Implement this function ... and all the Ping-stuff that belongs to it
	// - Maybe show two RTTs per player: To the host and to the netrelay
	// - Offer "autoUpdatePings(bool)" method to have the ping results be periodically refreshed
	// - Add support for the ping signal
	return 0;
}

const UserSettings& ParticipantList::participant_to_user(int16_t participant) const {
	assert(participant < human_user_count_);
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
	if (participant >= human_user_count_) {
		// AI
		participant -= human_user_count_;
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
	assert(participant < participant_count_);
	assert(game_ != nullptr);
	const Widelands::PlayersManager *pm = game_->player_manager();
	assert(pm);
	const int32_t playerindex = participant_to_playerindex(participant);
	assert(playerindex >= 0);
	const Widelands::Player *p = pm->get_player(playerindex + 1);
	assert(p);
	return p;
}
