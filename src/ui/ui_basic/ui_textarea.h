/*
 * Copyright (C) 2002, 2006 by the Widelands Development Team
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

#ifndef __S__TEXTAREA_H
#define __S__TEXTAREA_H

#include <string>
#include "constants.h"
#include "font_handler.h"
#include "ui_panel.h"

namespace UI {
/**
 * This defines a non responsive (to clicks) text area, where a text
 * can easily be printed
 */
struct Textarea : public Panel {

	/**
	 * For non-multiline textareas, the dimensions are set automatically,
	 * depending on the text. For multiline textareas, only the height and
	 * vertical position is adjusted automatically. A multiline Textarea differs
	 * from a Multiline_Textarea in that Multiline_Textarea provides scrollbars.
	 */
	Textarea
		(Panel * const parent,
		 const int x, const int y,
		 const std::string & text,
		 const Align align = Align_Left, const bool multiline = false) __attribute__((noinline))
		:
		Panel      (parent, x, y, 0, 0),
		m_text     (text),
		m_align    (align),
		m_multiline(multiline)
	{
		set_handle_mouse(false);
		set_think       (false);
		set_font        (UI_FONT_SMALL, UI_FONT_CLR_FG);
	}

	Textarea
		(Panel *  const parent,
		 const int x, const int y, const uint w, const uint h,
		 const Align align = Align_Left, const bool multiline = false) __attribute__((noinline))
		:
		Panel      (parent, x, y, w, h),
		m_align    (align),
		m_multiline(multiline),
		m_fontname (UI_FONT_NAME),
		m_fontsize (UI_FONT_SIZE_SMALL),
		m_fcolor   (UI_FONT_CLR_FG)
	{set_handle_mouse(false); set_think(false);}

	Textarea
		(Panel *  const parent,
		 const int x, const int y, const uint w, const uint h,
		 const std::string & text,
		 const Align align = Align_Left, const bool multiline = false) __attribute__((noinline))
		:
		Panel      (parent, x, y, w, h),
		m_align    (align),
		m_multiline(multiline),
		m_fontname (UI_FONT_NAME),
		m_fontsize (UI_FONT_SIZE_SMALL),
		m_fcolor   (UI_FONT_CLR_FG)
	{set_handle_mouse(false); set_think(false); set_text(text);}

	void set_text(const std::string &);
	void set_align(const Align);

	// Drawing and event handlers
	void draw(RenderTarget* dst);

	void set_font(const std::string & name, const int size, const RGBColor fg) {
		m_fontname = name;
		m_fontsize = size;
		m_fcolor   = fg;
		set_text(m_text);
	}

private:
	void collapse();
	void expand();

	std::string m_text;
	Align       m_align;
	bool        m_multiline;
   std::string    m_fontname;
   int            m_fontsize;
   RGBColor       m_fcolor;
};
};

#endif // __S__TEXTAREA_H
