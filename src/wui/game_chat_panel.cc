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

#include "wui/game_chat_panel.h"

#include <SDL_mouse.h>

#include "base/i18n.h"
#include "graphic/playercolor.h"
#include "network/participantlist.h"
#include "sound/sound_handler.h"
#include "wui/chat_msg_layout.h"

/**
 * Create a game chat panel
 */
GameChatPanel::GameChatPanel(UI::Panel* parent,
								int32_t const x,
								int32_t const y,
								uint32_t const w,
								uint32_t const h,
								ChatProvider& chat,
								UI::PanelStyle style)
	: UI::Panel(parent, x, y, w, h),
		chat_(chat),
		chatbox(this,
			0,
			0,
			w,
			h - 30,
			style,
			"",
			UI::Align::kLeft,
			UI::MultilineTextarea::ScrollMode::kScrollLogForced),
		editbox(this, 28, h - 25, w - 28, style),
		chat_message_counter(0),
		chat_sound(SoundHandler::register_fx(SoundType::kChat, "sound/lobby_chat")),
		recipient_dropdown_(this,
			"chat_recipient_dropdown",
			0,
			h - 25,
			25,
			16,
			25,
			_("Recipient"),
			UI::DropdownType::kPictorial,
			UI::PanelStyle::kFsMenu,
			UI::ButtonStyle::kFsMenuSecondary) {

	editbox.ok.connect([this]() { key_enter(); });
	editbox.cancel.connect([this]() { key_escape(); });
	editbox.activate_history(true);

	set_handle_mouse(true);
	set_can_focus(true);

	if (chat_.participants_ == nullptr) {
		// No access to participant list. Hide the dropdown
		recipient_dropdown_.set_visible(false);
		editbox.set_pos(Vector2i(editbox.get_x() - 28, editbox.get_y()));
		editbox.set_size(editbox.get_w() + 28, editbox.get_h());
		editbox.set_text(chat_.last_recipient_);
	} else {
		recipient_dropdown_.selected.connect([this]() { set_recipient(); });
		// React to keypresses for autocompletition
		editbox.changed.connect([this]() { key_changed(); });
		// Fill the dropdown menu with usernames
		prepare_recipients();
		// In the dropdown, select the entry that was used last time the menu was open
		recipient_dropdown_.select(chat_.last_recipient_);
		// Insert "@playername " into the edit field if the dropdown has a selection
		set_recipient();
		chat_.participants_->participants_updated.connect([this]() {
				// TODO: Move this into own method
				// Create new contents for dropdown
				prepare_recipients();
				// Get the current recipient
				std::string recipient = "";
				const std::string& text = editbox.text();
				if (!text.empty() && text[0] == '@') {
					// Get the recipient string including the first space
					// If there is no space, return the whole string
					const size_t pos_space = text.find(' ');
					if (pos_space != std::string::npos) {
						recipient = text.substr(0, pos_space + 1);
					} else {
						// Append a space since that increases the chance
						// to get a match in the dropdown
						recipient = text + ' ';
					}
				}
				// Try to re-set the recipient
				recipient_dropdown_.select(recipient);
				if (recipient != recipient_dropdown_.get_selected()) {
					recipient_dropdown_.set_errored(_("Unknown Recipient"));
				}

			});
	}
	chat_message_subscriber_ =
	   Notifications::subscribe<ChatMessage>([this](const ChatMessage&) { recalculate(true); });
	recalculate();
}

/**
 * Updates the chat message area.
 */
void GameChatPanel::recalculate(bool has_new_message) {
	const std::vector<ChatMessage> msgs = chat_.get_messages();

	const size_t msgs_size = msgs.size();
	std::string str = "<rt>";
	for (uint32_t i = 0; i < msgs_size; ++i) {
		str += format_as_richtext(msgs[i]);
	}
	str += "</rt>";

	chatbox.set_text(str);

	// Play a sound if there is a new non-system message
	if (!chat_.sound_off() && has_new_message) {
		for (size_t i = chat_message_counter; i < msgs_size; ++i) {
			if (!msgs[i].sender.empty()) {
				// Got a message that is no system message. Beep
				g_sh->play_fx(SoundType::kChat, chat_sound);
				break;
			}
		}
	}
	chat_message_counter = msgs_size;
}

/**
 * Put the focus on the message input panel.
 */
void GameChatPanel::focus_edit() {
	editbox.set_can_focus(true);
	editbox.focus();
}

/**
 * Remove the focus from the message input panel.
 */
void GameChatPanel::unfocus_edit() {
	editbox.set_can_focus(false);
}

void GameChatPanel::key_enter() {

	// TODO: Remove this block
	if (chat_.participants_ != nullptr) {
		int16_t n = chat_.participants_->get_participant_count();
		printf("Player name: %s\n", chat_.participants_->get_local_playername().c_str());
		printf("#participants: %i\n", n);
		if (chat_.participants_->is_ingame()) {
			printf("#\tName\t\tType\tPing\tStatus\tColor\tTeam\n");
			for (auto i = 0; i < n; ++i) {
				if (chat_.participants_->get_participant_type(i)
					== ParticipantList::ParticipantType::kObserver) {
					// It is an observer, so there is not team, color or defeated-status
					printf("%i.\t%s\t\tObserver\t%u\n", i,
						chat_.participants_->get_participant_name(i).c_str(),
						chat_.participants_->get_participant_ping(i)
						);
				} else {
					// It is a player, so all data should be available
					printf("%i.\t%s\t\t%s\t%u\t%s\t%s\t%u\n", i,
						chat_.participants_->get_participant_name(i).c_str(),
						(chat_.participants_->get_participant_type(i)
							== ParticipantList::ParticipantType::kPlayer ? "Player" : "AI"),
						chat_.participants_->get_participant_ping(i),
						(chat_.participants_->get_participant_defeated(i) ? "Defeated" : "Playing"),
						chat_.participants_->get_participant_color(i).hex_value().c_str(),
						chat_.participants_->get_participant_team(i)
						);
				}
			}
		} else {
			printf("#\tName\n");
			for (auto i = 0; i < n; ++i) {
				printf("%i.\t%s\n", i, chat_.participants_->get_participant_name(i).c_str());
			}
		}
	}

	const std::string& str = editbox.text();
	if (str.size()) {
		const size_t pos_first_space = str.find(' ');

		std::string recipient;
		// Reset message to only the recipient
		if (str[0] == '@') {
			recipient = str.substr(0, pos_first_space + 1);
		}

		// Make sure we have a chat message to send and it is more than just the recipient
		// Either it has no recipient or there is text behind the recipient
		if (str[0] != '@' || pos_first_space < str.size() - 1) {
			chat_.send(str);
			chat_.last_recipient_ = recipient;
		}

		editbox.set_text(recipient);
		// Set selection of dropdown to entered recipient, if possible
		if (chat_.participants_ != nullptr) {
			recipient_dropdown_.select(recipient);
			if (recipient_dropdown_.get_selected() != recipient) {
				// Seems the user is writing to someone unknown
				recipient_dropdown_.set_errored(_("Unknown Recipient"));
			}
		}
	}
	sent();
}

void GameChatPanel::key_escape() {
	editbox.set_text("");
	// Re-set the current selection to clean up a possible error state
	if (chat_.participants_ != nullptr) {
		recipient_dropdown_.select(recipient_dropdown_.get_selected());
		set_recipient();
	}
	aborted();
}

void GameChatPanel::key_changed() {

	if (chat_.participants_ == nullptr) {
		// Nothing to do here
		return;
	}

	// If the last two characters are spaces, do an autocomplete
	std::string str = editbox.text();
	if (str.size() < 3 || str[str.size() - 1] != ' ' || str[str.size() - 2] != ' ') {
		return;
	}

	// If input is "pl  " complete to "playername ", if the start of the name is unique

	// Extract the name to complete. If it starts with '@', remove the '@'
	size_t namepart_pos = str.find_last_of(" @", str.size() - 3);
printf("namepart_pos1=%lu\n", namepart_pos);
	if (namepart_pos == std::string::npos) {
		namepart_pos = 0;
	} else {
		namepart_pos += 1;
	}
	// Also remove trailing spaces
	const std::string namepart = str.substr(namepart_pos, str.size() - 2 - namepart_pos);
printf("text=_%s_, part=_%s_ namepart_pos=%lu\n", str.c_str(), namepart.c_str(), namepart_pos);

	if (namepart.empty()) {
		// Nothing left. Maybe a single '@' or 3+ spaces in a row
		return;
	}

	// Count the number of equal chars ignoring case
	static const auto count_equal_chars = [] (const std::string& a, const std::string& b) {
		const size_t len = std::min(a.size(), b.size());
		for (size_t i = 0; i < len; ++i) {
			if (std::tolower(a[i]) != std::tolower(b[i]))
				return i;
		}
		return len;
	};

	std::string candidate = "";

	// Iterate over all possible completitions (i.e., usernames).
	// For each, check whether they start with $namepart. If it does, store as candidate.
	// If there already is a candidate, create a new candidate from the part that is
	// the same for both candidates. (Note that the merged candidate might have wrong case,
    // but that is fixed on the next completition)
    const int16_t n_participants = chat_.participants_->get_participant_count();
	//const std::string& local_name = chat_.participants_->get_local_playername();
printf("namepart=%s\n", namepart.c_str());
	for (auto i = 0; i < n_participants; ++i) {
		if (chat_.participants_->get_participant_type(i) == ParticipantList::ParticipantType::kAI) {
			// Skip AIs
			continue;
		}
		const std::string& name = chat_.participants_->get_participant_name(i);
		//if (name == local_name) {
		//	continue;
		//}
		size_t equal_chars = count_equal_chars(namepart, name);
printf("name=%s count=%lu\n", namepart.c_str(), equal_chars);
		if (equal_chars == namepart.size()) {
			// We have a candidate!
printf("MAtch!\n");
			// Check if we already have a candidate. If not, use this one
printf("old_candidate=%s\n", candidate.c_str());
			if (candidate.empty()) {
				// Append a space so the user can continue typing after the completition
				candidate = name + " ";
			} else {
				// We already have one. Create an new candidate that is the combination of the two
				equal_chars = count_equal_chars(candidate, name);
				// No space appended here since the name is not complete yet
				candidate = candidate.substr(0, equal_chars);
			}
printf("new_candidate=%s\n", candidate.c_str());
		}
	}

	// If we have a candidate, set the new text for the input box
	if (!candidate.empty()) {
		str.replace(namepart_pos, std::string::npos, candidate);
printf("str=%s\n", str.c_str());
		editbox.set_text(str);
	}
}

void GameChatPanel::set_recipient() {
	assert(chat_.participants_ != nullptr);
	assert(recipient_dropdown_.has_selection());
	// Something has been selected. Re-focus the input box
	// Replace the old recipient, if any

	const std::string& recipient = recipient_dropdown_.get_selected();
	std::string str = editbox.text();

	// We have a recipient already
	if (str[0] == '@') {
		size_t pos_first_space = str.find(' ');
		if (pos_first_space == std::string::npos) {
			// Its only the recipient in the input field (no space separating the message).
			// Replace it completely.
			// If we want to sent to @all, recipient is empty so we basically clear the input
			str = recipient;
		} else {
			// There is some message, so replace the old with the new (possibly empty) recipient
			str.replace(0, pos_first_space + 1, recipient);
		}
	} else {
		// No recipient yet, prepend it
		// The separating space is already in recipient (see prepare_recipients())
		str = recipient + str;
	}

	// Set the updated string
	editbox.set_text(str);
	editbox.focus();
}

void GameChatPanel::prepare_recipients() {
	assert(chat_.participants_ != nullptr);

	printf("Updating recipient drowndown\n");
	recipient_dropdown_.clear();
	recipient_dropdown_.add(_("All"), "",
		g_gr->images().get("images/wui/menus/toggle_minimap.png"));
	// Select the "All" entry by default. Do *not* use the add() parameter for it since
	// it calls the listener for selected()
	recipient_dropdown_.select("");
	recipient_dropdown_.add(_("Team"), "@team ",
		g_gr->images().get("images/wui/buildings/menu_list_workers.png"));

	// Iterate over all human players (except ourselves) and add their names
	const int16_t n_participants = chat_.participants_->get_participant_count();
	const std::string& local_name = chat_.participants_->get_local_playername();

	for (auto i = 0; i < n_participants; ++i) {
		if (chat_.participants_->get_participant_type(i) == ParticipantList::ParticipantType::kAI) {
			// Skip AIs
			continue;
		}
		const std::string& name = chat_.participants_->get_participant_name(i);
		if (name == local_name) {
			continue;
		}

		if (chat_.participants_->get_participant_type(i)
			== ParticipantList::ParticipantType::kObserver) {
			recipient_dropdown_.add(name, "@" + name + " ",
				g_gr->images().get("images/wui/fieldaction/menu_tab_watch.png"));
		} else {
			recipient_dropdown_.add(name, "@" + name + " ",
				playercolor_image(chat_.participants_->get_participant_color(i),
					"images/players/genstats_player.png"));
		}
	}
}

/**
 * The mouse was clicked on this chatbox
 */
bool GameChatPanel::handle_mousepress(const uint8_t btn, int32_t, int32_t) {
	if (btn == SDL_BUTTON_LEFT && get_can_focus()) {
		focus_edit();
		return true;
	}

	return false;
}
