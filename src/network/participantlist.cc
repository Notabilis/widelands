#include "network/participantlist.h"

#include "ai/computer_player.h"
#include "logic/game.h"
#include "logic/game_settings.h"
#include "logic/playersmanager.h"
#include "logic/player.h"

ParticipantList::ParticipantList(const GameSettings* settings, Widelands::Game*& game,
									const std::string& localplayername)
			: settings_(settings), game_(game), localplayername_(localplayername),
				human_user_count_(0) {
	assert(settings_ != nullptr);
	// game_ might still be a nullpointer around here
	// The pointer referenced by game_ might be undefined here
#ifndef NDEBUG
	participant_count_ = 0;
#endif // NDEBUG
}

// TODO(Notabilis): (unrelated) Bug: Changing anything in the multiplayer lobby resets the "shared-in" setting

/* TODOs:
 - For the pings:
   - Maybe show two RTTs per player: To the host and to the netrelay
   - Offer "autoUpdatePings(bool)" method to have the ping results be periodically refreshed
   - Add support for the ping signal
 */

int16_t ParticipantList::get_participant_count() const {
	// Number of connected humans
	// settings_->users might contain disconnected humans, filter them out
	human_user_count_ = 0;
	for (const UserSettings& u : settings_->users) {
printf("get_count() name=%s\n", u.name.c_str());
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
	const Widelands::TeamNumber team = settings_->players[index - 1].team;
//assert(is_ingame());
if (is_ingame()) {
/// NOCOM: Remove
assert(team == participant_to_player(participant)->team_number());
}
	return team;
}

const std::string& ParticipantList::get_participant_name(int16_t participant) const {
	if (participant < human_user_count_) {
		// We can't use the Player entry for normal users since it isn't the selected user name
		// but instead a combined name of all the users for this player
		return participant_to_user(participant).name;
	}
	// It is an AI player. Get its type and resolve it to a pretty name
	const PlayerSettings& ps = settings_->players[participant_to_playerindex(participant) - 1];
	assert(ps.state == PlayerSettings::State::kComputer);
if (is_ingame()) {
// Old way
/// NOCOM: Remove
const Widelands::Player* p = participant_to_player(participant);
assert(p->get_ai() == ps.ai);
}
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
	return kPlayerColors[participant_to_playerindex(participant) - 1];
}

bool ParticipantList::is_ingame() const {
	return (game_ != nullptr);
}

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
uint8_t ParticipantList::get_participant_ping(int16_t participant) const {
	//assert(is_ingame());
	assert(participant < participant_count_);
	// TODO(Notabilis): Implement this function ... and all the Ping-stuff that belongs to it
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
		// TODO(Notabilis): Can I remove computerplayers_ and use the entry in settings_ ?
		// Yes, I can. Its only an optimization. Remove it?
		/*if (computerplayers_ != nullptr) {
//printf("bbb %i %i %i\n", participant, (*computerplayers_)[participant]->player_number(), pm->get_number_of_players());
//assert((*computerplayers_)[participant]->player_number() <= pm->get_number_of_players());
			return (*computerplayers_)[participant]->player_number();
		} else {*/
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
					return i + 1;
				}
				--participant;
				assert(participant >= 0);
			}
		//}
		NEVER_HERE();
	} else {
		// No useful result possible for spectators or semi-connected users
		assert(participant_to_user(participant).position <= UserSettings::highest_playernum());

//printf("aaa %i %i %i\n", participant, settings_->users[participant].position, pm->get_number_of_players());
// Useless, vector has always max size: assert(settings_->users[participant].position <= pm->get_number_of_players());
		// .position is the index within settings_->players and also
		// as .position+1 the index inside game_->player_manager()
		return participant_to_user(participant).position + 1;
	}
}

const Widelands::Player* ParticipantList::participant_to_player(int16_t participant) const {
	assert(participant < participant_count_);
	assert(game_ != nullptr);
	const Widelands::PlayersManager *pm = game_->player_manager();
	assert(pm);
	const int32_t playerindex = participant_to_playerindex(participant);
	assert(playerindex > 0);
	const Widelands::Player *p = pm->get_player(playerindex);
	assert(p);
	return p;
}
