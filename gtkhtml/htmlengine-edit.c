/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 1999, 2000 Helix Code, Inc.
    Copyright (C) 2001 Ximian, Inc.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    Authors: Ettore Perazzoli <ettore@helixcode.com>
             Radek Doulik     <rodo@ximian.com>
*/


#include <config.h>
#include <ctype.h>
#include <string.h>
#include <glib.h>

#include "gtkhtml.h"
#include "gtkhtml-properties.h"

#include "htmlclueflow.h"
#include "htmlcolorset.h"
#include "htmlcursor.h"
#include "htmlobject.h"
#include "htmltable.h"
#include "htmltablecell.h"
#include "htmltext.h"
#include "htmltextslave.h"
#include "htmlimage.h"
#include "htmlinterval.h"
#include "htmlselection.h"
#include "htmlsettings.h"
#include "htmlundo.h"

#include "htmlengine-edit.h"
#include "htmlengine-edit-cut-and-paste.h"
#include "htmlengine-edit-cursor.h"
#include "htmlengine-edit-movement.h"
#include "htmlengine-edit-selection-updater.h"
#include "htmlengine-edit-table.h"
#include "htmlengine-edit-tablecell.h"


void
html_engine_undo (HTMLEngine *e)
{
	HTMLUndo *undo;

	g_return_if_fail (e != NULL);
	g_return_if_fail (HTML_IS_ENGINE (e));
	g_return_if_fail (e->undo != NULL);
	g_return_if_fail (e->editable);

	html_engine_unselect_all (e);

	undo = e->undo;
	html_undo_do_undo (undo, e);
}

void
html_engine_redo (HTMLEngine *e)
{
	HTMLUndo *undo;

	g_return_if_fail (e != NULL);
	g_return_if_fail (HTML_IS_ENGINE (e));
	g_return_if_fail (e->undo != NULL);

	html_engine_unselect_all (e);

	undo = e->undo;
	html_undo_do_redo (undo, e);
}


void
html_engine_set_mark (HTMLEngine *e)
{
	g_return_if_fail (e != NULL);
	g_return_if_fail (HTML_IS_ENGINE (e));
	g_return_if_fail (e->editable || e->caret_mode);

	if (e->mark != NULL) {
		html_engine_unselect_all (e);
		html_cursor_destroy(e->mark);
	}

	e->mark = html_cursor_dup (e->cursor);

	html_engine_edit_selection_updater_reset (e->selection_updater);
	html_engine_edit_selection_updater_schedule (e->selection_updater);
}

void
html_engine_clipboard_push (HTMLEngine *e)
{
	e->clipboard_stack = g_list_prepend (e->clipboard_stack, GUINT_TO_POINTER (e->clipboard_len));
	e->clipboard_stack = g_list_prepend (e->clipboard_stack, e->clipboard);
	e->clipboard       = NULL;
}

void
html_engine_clipboard_pop (HTMLEngine *e)
{
	g_assert (e->clipboard_stack);

	e->clipboard       = HTML_OBJECT (e->clipboard_stack->data);
	e->clipboard_stack = g_list_remove (e->clipboard_stack, e->clipboard_stack->data);
	e->clipboard_len   = GPOINTER_TO_UINT (e->clipboard_stack->data);
	e->clipboard_stack = g_list_remove (e->clipboard_stack, e->clipboard_stack->data);
}

typedef struct
{
	gboolean active;
	gint cursor;
	gint mark;
} Selection;

void
html_engine_selection_push (HTMLEngine *e)
{
	Selection *selection = g_new (Selection, 1);

	if (html_engine_is_selection_active (e)) {
		selection->active = TRUE;
		selection->cursor = html_cursor_get_position (e->cursor);
		selection->mark = html_cursor_get_position (e->mark);
	} else {
		selection->active = FALSE;
		selection->cursor = -1;
		selection->mark = -1;
	}

	e->selection_stack = g_list_prepend (e->selection_stack, selection);
}

void
html_engine_selection_pop (HTMLEngine *e)
{
	Selection *selection;

	g_assert (e->selection_stack);

	selection = e->selection_stack->data;
	e->selection_stack = g_list_delete_link (e->selection_stack, e->selection_stack);

	html_engine_disable_selection (e);

	if (selection->active) {
		html_cursor_jump_to_position (e->cursor, e, selection->mark);
		html_engine_set_mark (e);
		html_cursor_jump_to_position (e->cursor, e, selection->cursor);
	}
	html_engine_edit_selection_updater_update_now (e->selection_updater);

	g_free (selection);
}

gboolean
html_engine_selection_stack_top (HTMLEngine *e, gint *cpos, gint *mpos)
{
	Selection *selection = e->selection_stack ? e->selection_stack->data : NULL;

	if (selection && selection->active) {
		if (cpos)
			*cpos = selection->cursor;
		if (mpos)
			*mpos = selection->mark;

		return TRUE;
	}

	return FALSE;
}

gboolean
html_engine_selection_stack_top_modify (HTMLEngine *e, gint delta)
{
	Selection *selection = e->selection_stack ? e->selection_stack->data : NULL;

	if (selection && selection->active) {
		selection->cursor += delta;
		selection->mark += delta;

		return TRUE;
	}

	return FALSE;
}

static void
spell_check_object (HTMLObject *o, HTMLEngine *e, gpointer data)
{
	if (HTML_OBJECT_TYPE (o) == HTML_TYPE_CLUEFLOW)
		html_clueflow_spell_check (HTML_CLUEFLOW (o), e, (HTMLInterval *) data);
}

void
html_engine_spell_check_range (HTMLEngine *e, HTMLCursor *begin, HTMLCursor *end)
{
	HTMLInterval *i;
	gboolean cited;

	e->need_spell_check = FALSE;

	if (!e->widget->editor_api || !gtk_html_get_inline_spelling (e->widget) || !begin->object->parent)
		return;

	begin = html_cursor_dup (begin);
	end   = html_cursor_dup (end);

	cited = FALSE;
	while (html_selection_spell_word (html_cursor_get_prev_char (begin), &cited) || cited) {
		if (html_cursor_backward (begin, e))
			;
		cited = FALSE;
	}

	cited = FALSE;
	while (html_selection_spell_word (html_cursor_get_current_char (end), &cited) || cited) {
		if (html_cursor_forward (end, e))
			;
		cited = FALSE;
	}

	i = html_interval_new_from_cursor (begin, end);
	if (begin->object->parent != end->object->parent)
		html_interval_forall (i, e, spell_check_object, i);
	else if (HTML_IS_CLUEFLOW (begin->object->parent))
		html_clueflow_spell_check (HTML_CLUEFLOW (begin->object->parent), e, i);
	html_interval_destroy (i);
	html_cursor_destroy (begin);
	html_cursor_destroy (end);
}

gboolean
html_is_in_word (gunichar uc)
{
	/* printf ("test %d %c => %d\n", uc, uc, g_unichar_isalnum (uc) || uc == '\''); */
	return g_unichar_isalpha (uc) || uc == '\'';
}

void
html_engine_select_word_editable (HTMLEngine *e)
{
	while (html_selection_word (html_cursor_get_prev_char (e->cursor)))
		html_cursor_backward (e->cursor, e);
	html_engine_set_mark (e);
	while (html_selection_word (html_cursor_get_current_char (e->cursor)))
		html_cursor_forward (e->cursor, e);
}

void
html_engine_select_spell_word_editable (HTMLEngine *e)
{
	gboolean cited, cited2;

	cited = cited2 = FALSE;
	while (html_selection_spell_word (html_cursor_get_prev_char (e->cursor), &cited) || cited) {
		html_cursor_backward (e->cursor, e);
		cited2 = cited;
		cited = FALSE;
	}

	if (cited2) {
		html_cursor_forward (e->cursor, e);
		cited = TRUE;
	}

	html_engine_set_mark (e);
	while (html_selection_spell_word (html_cursor_get_current_char (e->cursor), &cited2) || (!cited && cited2)) {
		html_cursor_forward (e->cursor, e);
		cited2 = FALSE;
	}
}

void
html_engine_select_line_editable (HTMLEngine *e)
{
	html_engine_beginning_of_line (e);
	html_engine_set_mark (e);
	html_engine_end_of_line (e);
}

void
html_engine_select_paragraph_editable (HTMLEngine *e)
{
	html_engine_beginning_of_paragraph (e);
	html_engine_set_mark (e);
	html_engine_end_of_paragraph (e);
}

void
html_engine_select_paragraph_extended (HTMLEngine *e)
{
	gboolean fw;

	html_engine_hide_cursor (e);
	html_engine_beginning_of_paragraph (e);
	fw = html_cursor_backward (e->cursor, e);
	html_engine_set_mark (e);
	if (fw)
		html_cursor_forward (e->cursor, e);
	html_engine_end_of_paragraph (e);
	html_cursor_forward (e->cursor, e);
	html_engine_show_cursor (e);

	html_engine_update_selection_if_necessary (e);
}

void
html_engine_select_all_editable (HTMLEngine *e)
{
	html_engine_beginning_of_document (e);
	html_engine_set_mark (e);
	html_engine_end_of_document (e);
}

struct SetData {
	HTMLType      object_type;
	const gchar  *key;
	const gchar  *value;
};

static void
set_data (HTMLObject *o, HTMLEngine *e, gpointer p)
{
	struct SetData *data = (struct SetData *) p;

	if (HTML_OBJECT_TYPE (o) == data->object_type) {
		/* printf ("set data %s --> %p\n", data->key, data->value); */
		html_object_set_data (o, data->key, data->value);
	}
}

void
html_engine_set_data_by_type (HTMLEngine *e, HTMLType object_type, const gchar *key, const gchar * value)
{
	struct SetData *data = g_new (struct SetData, 1);

	/* printf ("html_engine_set_data_by_type %s\n", key); */

	data->object_type = object_type;
	data->key         = key;
	data->value       = value;

	html_object_forall (e->clue, NULL, set_data, data);

	g_free (data);
}

void
html_engine_clipboard_clear (HTMLEngine *e)
{
	if (e->clipboard) {
		html_object_destroy (e->clipboard);
		e->clipboard = NULL;
	}
}

HTMLObject *
html_engine_new_text (HTMLEngine *e, const gchar *text, gint len)
{
	HTMLObject *to;

	to = html_text_new_with_len (text, len, e->insertion_font_style, e->insertion_color);
	if (e->insertion_font_style != GTK_HTML_FONT_STYLE_DEFAULT)
		html_text_set_style_in_range (HTML_TEXT (to), e->insertion_font_style, e, 0, HTML_TEXT (to)->text_bytes);
	if (e->insertion_color &&
	    e->insertion_color != html_colorset_get_color (e->settings->color_set, HTMLTextColor))
		html_text_set_color_in_range (HTML_TEXT (to), e->insertion_color, 0, HTML_TEXT (to)->text_bytes);
	if (e->insertion_url)
		html_text_append_link (HTML_TEXT (to), e->insertion_url, e->insertion_target, 0, HTML_TEXT (to)->text_len);

	return to;
}

HTMLObject *
html_engine_new_link (HTMLEngine *e, const gchar *text, gint len, gchar *url)
{
	HTMLObject *link;
	gchar *real_url, *real_target;

	real_target = strchr (text, '#');
	if (real_target) {
		real_url = g_strndup (url, real_target - url);
		real_target ++;
	} else
		real_url = url;

	link = html_text_new_with_len (text, len, e->insertion_font_style,
				       html_colorset_get_color (e->settings->color_set, HTMLLinkColor));
	html_text_append_link (HTML_TEXT (link), real_url, real_target, 0, HTML_TEXT (link)->text_len);

	if (real_target)
		g_free (real_url);

	return link;
}

HTMLObject *
html_engine_new_text_empty (HTMLEngine *e)
{
	return html_engine_new_text (e, "", 0);
}

gboolean
html_engine_cursor_on_bop (HTMLEngine *e)
{
	g_assert (e);
	g_assert (e->cursor);
	g_assert (e->cursor->object);

	return e->cursor->offset == 0 && html_object_prev_not_slave (e->cursor->object) == NULL;
}

guint
html_engine_get_indent (HTMLEngine *e)
{
	g_assert (e);
	g_assert (e->cursor);
	g_assert (e->cursor->object);

	return e->cursor->object && e->cursor->object->parent
		&& HTML_OBJECT_TYPE (e->cursor->object->parent) == HTML_TYPE_CLUEFLOW
		? html_clueflow_get_indentation (HTML_CLUEFLOW (e->cursor->object->parent)) : 0;
}

#define LINE_LEN 71

static inline guint
inc_line_offset (guint line_offset, gunichar uc)
{
	return uc == '\t'
		? line_offset + 8 - (line_offset % 8)
		: line_offset + 1;
}

static guint
try_break_this_line (HTMLEngine *e, guint line_offset, guint last_space)
{
	HTMLObject *flow;
	gunichar uc;

	flow = e->cursor->object->parent;

	while (html_cursor_forward (e->cursor, e) && e->cursor->object->parent == flow) {
		uc = html_cursor_get_current_char (e->cursor);
		line_offset = inc_line_offset (line_offset, uc);
		if (uc == ' ' || uc == '\t')
			last_space = line_offset;
		if (uc && line_offset >= LINE_LEN) {
			if (last_space) {
				html_cursor_backward_n (e->cursor, e, line_offset - last_space);
				uc = ' ';
			} else {
				/* go to end of word */
				while (html_cursor_forward (e->cursor, e)) {
					line_offset = inc_line_offset (line_offset, uc);
					uc = html_cursor_get_current_char (e->cursor);
					if (uc == ' ' || uc == '\t' || !uc)
						break;
				}
			}
			if (uc == ' ' || uc == '\t') {
				html_engine_insert_empty_paragraph (e);
				html_engine_delete_n (e, 1, TRUE);

				flow        = e->cursor->object->parent;
				last_space  = 0;
				line_offset = 0;
			}
		}
		if (!uc)
			return line_offset;
	}

	return line_offset;
}

static void
go_to_begin_of_para (HTMLEngine *e)
{
	HTMLObject *prev;

	do {
		gint offset;
		html_cursor_beginning_of_paragraph (e->cursor, e);
		offset = 0;
		prev = html_object_prev_cursor (e->cursor->object, &offset);
		if (prev && !html_object_is_container (prev) && html_object_get_length (prev)
		    && html_clueflow_style_equals (HTML_CLUEFLOW (e->cursor->object->parent), HTML_CLUEFLOW (prev->parent)))
			html_cursor_backward (e->cursor, e);
		else
			break;
	} while (1);
}

void
html_engine_indent_paragraph (HTMLEngine *e)
{
	guint position;
	guint line_offset;
	guint last_space;

	g_assert (e->cursor->object);
	if (!HTML_IS_CLUEFLOW (e->cursor->object->parent))
		return;

	html_engine_disable_selection (e);
	position = e->cursor->position;

	html_undo_level_begin (e->undo, "Indent paragraph", "Reverse paragraph indentation");
	html_engine_freeze (e);

	go_to_begin_of_para (e);

	line_offset = 0;
	last_space  = 0;
	do {
		HTMLObject *flow;

		line_offset = try_break_this_line (e, line_offset, last_space);
		flow = e->cursor->object->parent;
		if (html_cursor_forward (e->cursor, e)
		    && e->cursor->offset == 0 && html_object_get_length (e->cursor->object)
		    && !html_object_is_container (e->cursor->object)
		    && html_clueflow_style_equals (HTML_CLUEFLOW (e->cursor->object->parent), HTML_CLUEFLOW (flow))
		    && html_object_prev_not_slave (e->cursor->object) == NULL) {
			if (line_offset < LINE_LEN - 1) {
				gunichar prev;
				html_engine_delete_n (e, 1, FALSE);
				prev = html_cursor_get_prev_char (e->cursor);
				if (prev != ' ' && prev != '\t') {
					html_engine_insert_text (e, " ", 1);
					line_offset ++;
				} else if (position > e->cursor->position)
					position --;
				last_space = line_offset - 1;
			} else {
				line_offset = 0;
				last_space  = 0;
			}
		} else
			break;
	} while (1);

	html_cursor_jump_to_position (e->cursor, e, position);
	html_engine_thaw (e);
	html_undo_level_end (e->undo);
}

void
html_engine_indent_pre_line (HTMLEngine *e)
{
	guint position;
	guint line_offset;
	guint last_space;
	HTMLObject *flow;
	gunichar uc;

	g_assert (e->cursor->object);
	if (HTML_OBJECT_TYPE (e->cursor->object->parent) != HTML_TYPE_CLUEFLOW
	    || html_clueflow_get_style (HTML_CLUEFLOW (e->cursor->object->parent)) != HTML_CLUEFLOW_STYLE_PRE)
		return;

	html_engine_disable_selection (e);
	position = e->cursor->position;

	html_undo_level_begin (e->undo, "Indent PRE paragraph", "Reverse paragraph indentation");
	html_engine_freeze (e);

	last_space  = 0;
	line_offset = 0;

	html_cursor_beginning_of_paragraph (e->cursor, e);

	flow = e->cursor->object->parent;
	while (html_cursor_forward (e->cursor, e) && e->cursor->object->parent == flow) {
		uc = html_cursor_get_current_char (e->cursor);
		line_offset = inc_line_offset (line_offset, uc);

		if (uc == ' ' || uc == '\t') {
			last_space = line_offset;
		}

		if (line_offset >= LINE_LEN) {
			if (last_space) {
				html_cursor_backward_n (e->cursor, e, line_offset - last_space);

				html_cursor_forward (e->cursor, e);
				if ((uc = html_cursor_get_current_char (e->cursor))) {
					html_engine_insert_empty_paragraph (e);
					if (position >= e->cursor->position)
						position++;
				}
			}
		}
		if (!uc)
			break;
	}

	html_cursor_jump_to_position (e->cursor, e, position);
	html_engine_thaw (e);
	html_undo_level_end (e->undo);
}

void
html_engine_fill_pre_line (HTMLEngine *e)
{
	guint position;
	guint line_offset;
	guint last_space;
	gunichar uc;

	g_assert (e->cursor->object);
	position = e->cursor->position;

	if (HTML_OBJECT_TYPE (e->cursor->object->parent) != HTML_TYPE_CLUEFLOW
	    || html_clueflow_get_style (HTML_CLUEFLOW (e->cursor->object->parent)) != HTML_CLUEFLOW_STYLE_PRE) {
		return;
	}

	last_space  = 0;
	line_offset = 0;

	html_cursor_beginning_of_paragraph (e->cursor, e);

	while (html_cursor_forward (e->cursor, e) && (e->cursor->position < position - 1)) {
		uc = html_cursor_get_current_char (e->cursor);
		line_offset = inc_line_offset (line_offset, uc);

		if (uc == ' ' || uc == '\t') {
			last_space = line_offset;
		}

		if (line_offset >= LINE_LEN) {
			if (last_space) {
				html_cursor_backward_n (e->cursor, e, line_offset - last_space);

				html_cursor_forward (e->cursor, e);
				if ((uc = html_cursor_get_current_char (e->cursor))) {
					html_engine_insert_empty_paragraph (e);
					if (position >= e->cursor->position)
						position++;

					line_offset = 0;
					last_space = 0;
				}
			}
		}
		if (!uc)
			break;
	}
	html_cursor_jump_to_position (e->cursor, e, position);
}

void
html_engine_space_and_fill_line (HTMLEngine *e)
{

	g_assert (e->cursor->object);
	html_undo_level_begin (e->undo, "insert and fill", "reverse insert and fill");


	html_engine_disable_selection (e);
	html_engine_freeze (e);
	html_engine_insert_text (e, " ", 1);

	html_engine_fill_pre_line (e);

	html_engine_thaw (e);
	html_undo_level_end (e->undo);
}

void
html_engine_break_and_fill_line (HTMLEngine *e)
{
	html_undo_level_begin (e->undo, "break and fill", "reverse break and fill");

	html_engine_disable_selection (e);
	html_engine_freeze (e);

	html_engine_fill_pre_line (e);

	html_engine_insert_empty_paragraph (e);
	html_engine_thaw (e);
	html_undo_level_end (e->undo);
}

gboolean
html_engine_next_cell (HTMLEngine *e, gboolean create)
{
	HTMLTableCell *cell, *current_cell;

	cell = html_engine_get_table_cell (e);
	if (cell) {
		html_engine_hide_cursor (e);
		do {
			html_cursor_end_of_line (e->cursor, e);
			html_cursor_forward (e->cursor, e);
			current_cell = html_engine_get_table_cell (e);
		} while (current_cell == cell);

		if (create && HTML_IS_TABLE (e->cursor->object)) {
			html_cursor_backward (e->cursor, e);
			html_engine_insert_table_row (e, TRUE);
			html_cursor_forward (e->cursor, e);
		}
		html_engine_show_cursor (e);

		return TRUE;
	}

	return FALSE;
}

gboolean
html_engine_prev_cell (HTMLEngine *e)
{
	HTMLTableCell *cell, *current_cell;

	cell = html_engine_get_table_cell (e);
	if (cell) {
		html_engine_hide_cursor (e);
		do {
			html_cursor_beginning_of_line (e->cursor, e);
			html_cursor_backward (e->cursor, e);
			current_cell = html_engine_get_table_cell (e);
		} while (current_cell == cell);

		html_engine_show_cursor (e);

		return TRUE;
	}

	return FALSE;
}

void
html_engine_set_title (HTMLEngine *e, const gchar *title)
{
	if (e->title)
		g_string_free (e->title, TRUE);
	e->title = g_string_new (title);
	g_signal_emit_by_name (e, "title_changed");
}

void html_engine_edit_set_direction (HTMLEngine *e,
				     HTMLDirection dir)
{
	HTMLClueFlow *cf = html_object_get_flow (e->cursor->object);

	if (cf && cf->dir != dir && html_clueflow_is_empty (cf)) {
		html_engine_freeze (e);
		cf->dir = dir;
		html_engine_thaw (e);
	}
}

int
html_engine_get_insert_level_for_object (HTMLEngine *e, HTMLObject *o)
{
	int cursor_level = 3, level = html_object_get_insert_level (o);

	if (level > 3) {
		if (e && e->cursor->object && e->cursor->object->parent && e->cursor->object->parent->parent && html_object_is_clue (e->cursor->object->parent->parent)) {
			HTMLObject *clue = e->cursor->object->parent->parent;

			while (clue && clue->parent && (HTML_IS_CLUEV (clue->parent) || HTML_IS_TABLE_CELL (clue->parent))) {
				clue = clue->parent;
				cursor_level ++;
			}
		}
	}

	return MIN (level, cursor_level);
}
