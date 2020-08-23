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
	recipient_dropdown_.selected.connect([this]() { set_recipient(); });

	// TODO: The selection of the dropdown is reset everytime the chat is re-opened. Store it
	// Probably a TODO: Refreshing the recipient list even while the window stays open will reset the selection
	set_handle_mouse(true);
	set_can_focus(true);

	prepare_recipients();

	recipient_dropdown_.select(chat_.last_recipient_);
	set_recipient();

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
		recipient_dropdown_.select(recipient);
		if (recipient_dropdown_.get_selected() != recipient) {
			// Seems the user is writing to someone unknown
			recipient_dropdown_.set_errored(_("Unknown Recipient"));
		}
	}
	sent();
}

void GameChatPanel::key_escape() {
	editbox.set_text("");
	// Re-set the current selection to clean up a possible error state
	recipient_dropdown_.select(recipient_dropdown_.get_selected());
	set_recipient();
	aborted();
}

void GameChatPanel::set_recipient() {
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
	if (chat_.participants_ == nullptr) {
		// TODO: Hide dropdown-button in that case
		printf("Error: Not chat_.participants_\n");
		return;
	}

	//recipient_dropdown_.set_autoexpand_display_button();

	printf("Updating recipient drowndown\n");
	recipient_dropdown_.clear();
	recipient_dropdown_.add(_("All"), "",
		g_gr->images().get("images/wui/menus/toggle_minimap.png"), true);
	recipient_dropdown_.add(_("Team"), "@team ",
		g_gr->images().get("images/wui/buildings/menu_list_workers.png"));


	// NOCOM: Does not work for network clients. ... Hm, maybe because GameClient does not support ParticipantList...


	// Iterate over all human players (except ourselves) and add their names
	int16_t n_participants = chat_.participants_->get_participant_count();
	const std::string& local_name = chat_.participants_->get_local_playername();

	for (auto i = 0; i < n_participants; ++i) {
		if (chat_.participants_->get_participant_type(i) == ParticipantList::ParticipantType::kAI) {
			// Skip AIs
			continue;
		}
		const std::string& name = chat_.participants_->get_participant_name(i);
		if (name != local_name) {

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
