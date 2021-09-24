/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library

   Copyright (C) 2000 Helix Code, Inc.

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
*/

#include <config.h>
#include "gtkhtmldebug.h"
#include "htmlclue.h"
#include "htmlcolor.h"
#include "htmlcolorset.h"
#include "htmlengine-edit.h"
#include "htmlengine-edit-fontstyle.h"
#include "htmlengine-edit-movement.h"
#include "htmlengine-edit-selection-updater.h"
#include "htmlinterval.h"
#include "htmltext.h"
#include "htmlselection.h"
#include "htmlsettings.h"
#include "htmlundo.h"

/* #define PARANOID_DEBUG */
static HTMLObject * html_engine_text_style_object (HTMLEngine *e, gint *offset);


static GtkHTMLFontStyle
get_font_style_from_selection (HTMLEngine *engine)
{
	GtkHTMLFontStyle style;
	GtkHTMLFontStyle conflicts;
	gboolean first;
	HTMLPoint p;
	gint offset;

	g_return_val_if_fail (engine->clue != NULL, GTK_HTML_FONT_STYLE_DEFAULT);
	g_return_val_if_fail (html_engine_is_selection_active (engine), GTK_HTML_FONT_STYLE_DEFAULT);

	/* printf ("%s mark %p,%d cursor %p,%d\n",
		__FUNCTION__,
		engine->mark, engine->mark->position,
		engine->cursor, engine->cursor->position); */


	style = GTK_HTML_FONT_STYLE_DEFAULT;
	conflicts = GTK_HTML_FONT_STYLE_DEFAULT;
	first = TRUE;

	p = engine->selection->from;
	offset = p.offset;

	while (1) {
		if (html_object_is_text (p.object) && p.offset != html_object_get_length (p.object)) {
			gint index = 0;
			if (first) {
				index = g_utf8_offset_to_pointer (HTML_TEXT (p.object)->text, offset) - HTML_TEXT (p.object)->text;
				style = html_text_get_fontstyle_at_index (HTML_TEXT (p.object), index);
				first = FALSE;
			}
			conflicts |= html_text_get_style_conflicts (HTML_TEXT (p.object), style, index,
								    p.object == engine->selection->to.object
								    ? engine->selection->to.offset : HTML_TEXT (p.object)->text_bytes);
		}

		if (html_point_cursor_object_eq (&p, &engine->selection->to))
			break;

		html_point_next_cursor (&p);
		offset = 0;

		if (p.object == NULL) {
			g_warning ("Unable to find style for end of selection");
			return style;
		}
	}

	return style & ~conflicts;
}

/* Return value is already html_color_ref'ed */
static HTMLColor *
get_color_from_selection (HTMLEngine *engine)
{
	HTMLColor *color = NULL;
	HTMLPoint p;

	g_return_val_if_fail (engine->clue != NULL, NULL);
	g_return_val_if_fail (html_engine_is_selection_active (engine), NULL);

	p = engine->selection->from;
	while (1) {
		if (html_object_is_text (p.object)  && p.offset != html_object_get_length (p.object)) {
			color = html_text_get_color (HTML_TEXT (p.object), engine,
						     p.object == engine->selection->from.object
						     ? g_utf8_offset_to_pointer (HTML_TEXT (p.object)->text, p.offset) - HTML_TEXT (p.object)->text : 0);
			break;
		}

		if (html_point_cursor_object_eq (&p, &engine->selection->to))
			break;
		html_point_next_cursor (&p);

		if (p.object == NULL) {
			g_warning ("Unable to find color for end of selection");
			return color;
		}
	}

	return color;
}

GtkHTMLFontStyle
html_engine_get_document_font_style (HTMLEngine *engine)
{
	if (!engine)
		return GTK_HTML_FONT_STYLE_DEFAULT;

	if (!HTML_IS_ENGINE (engine))
		return GTK_HTML_FONT_STYLE_DEFAULT;

	if (!engine->editable)
		return GTK_HTML_FONT_STYLE_DEFAULT;

	if (html_engine_is_selection_active (engine))
		return get_font_style_from_selection (engine);
	else {
		HTMLObject *curr = engine->cursor->object;
		gint offset;

		if (curr == NULL)
			return GTK_HTML_FONT_STYLE_DEFAULT;
		else if (! html_object_is_text (curr))
			return GTK_HTML_FONT_STYLE_DEFAULT;
		else {
			HTMLObject *obj;

			obj = html_engine_text_style_object (engine, &offset);
			return obj
				? html_text_get_fontstyle_at_index (HTML_TEXT (obj), g_utf8_offset_to_pointer (HTML_TEXT (obj)->text, offset) - HTML_TEXT (obj)->text)
				: GTK_HTML_FONT_STYLE_DEFAULT;
		}
	}
}

/* Return value is already html_color_ref'ed */
HTMLColor *
html_engine_get_document_color (HTMLEngine *engine)
{
	g_return_val_if_fail (engine != NULL, NULL);
	g_return_val_if_fail (HTML_IS_ENGINE (engine), NULL);
	g_return_val_if_fail (engine->editable, NULL);

	if (html_engine_is_selection_active (engine))
		return get_color_from_selection (engine);
	else {
		HTMLObject *curr = engine->cursor->object;

		if (curr == NULL)
			return NULL;
		else if (! html_object_is_text (curr))
			return NULL;
		else {
			HTMLObject *obj;
			gint offset;

			obj = html_engine_text_style_object (engine, &offset);
			if (obj) {
				return html_text_get_color_at_index (HTML_TEXT (obj), engine, g_utf8_offset_to_pointer (HTML_TEXT (obj)->text, offset) - HTML_TEXT (obj)->text);
			} else {
				HTMLColor *color = html_colorset_get_color (engine->settings->color_set, HTMLTextColor);
				html_color_ref(color);
				return color;
			}
		}
	}
}

GtkHTMLFontStyle
html_engine_get_font_style (HTMLEngine *engine)
{
	return (engine->insertion_font_style == GTK_HTML_FONT_STYLE_DEFAULT)
		? html_engine_get_document_font_style (engine)
		: engine->insertion_font_style;
}

HTMLColor *
html_engine_get_color (HTMLEngine *engine)
{
	return engine->insertion_color;
}

/**
 * html_engine_update_insertion_font_style:
 * @engine: An HTMLEngine
 *
 * Update @engine's current insertion font style according to the
 * current selection and cursor position.
 *
 * Return value:
 **/
gboolean
html_engine_update_insertion_font_style (HTMLEngine *engine)
{
	GtkHTMLFontStyle new_style;

	new_style = html_engine_get_document_font_style (engine);

	if (new_style != engine->insertion_font_style) {
		engine->insertion_font_style = new_style;
		return TRUE;
	}

	return FALSE;
}

/**
 * html_engine_update_insertion_style:
 * @engine: An HTMLEngine
 *
 * Update @engine's current insertion font style/color according to the
 * current selection and cursor position.
 *
 * Return value:
 **/
gboolean
html_engine_update_insertion_color (HTMLEngine *engine)
{
	HTMLColor *new_color;

	new_color = html_engine_get_document_color (engine);

	if (new_color && !html_color_equal (new_color, engine->insertion_color)) {
		html_color_unref (engine->insertion_color);
		engine->insertion_color = new_color;
		return TRUE;
	}

	if (new_color)
		html_color_unref (new_color);
	return FALSE;
}

/**
 * html_engine_set_font_style:
 * @engine: An HTMLEngine
 * @style: An HTMLFontStyle
 *
 * Set the current font style for `engine'.  This has the same semantics as the
 * bold, italics etc. buttons in a word processor, i.e.:
 *
 * - If there is a selection, the selection gets the specified style.
 *
 * - If there is no selection, the style gets "attached" to the cursor.  So
 *   inserting text after this will cause text to have this style.
 *
 * Instead of specifying an "absolute" style, we specify it as a "difference"
 * from the current one, through an AND mask and an OR mask.
 *
 **/
struct tmp_font {
	GtkHTMLFontStyle and_mask;
	GtkHTMLFontStyle or_mask;
};

static void
object_set_font_style (HTMLObject *o, HTMLEngine *e, gpointer data)
{
	if (html_object_is_text (o)) {
		struct tmp_font *tf = (struct tmp_font *) data;

		html_text_unset_style (HTML_TEXT (o), ~tf->and_mask);
		html_text_set_style (HTML_TEXT (o), tf->or_mask, e);
	}
}

struct _HTMLEmptyParaSetStyle {
	HTMLUndoData data;

	GtkHTMLFontStyle and_mask;
	GtkHTMLFontStyle or_mask;
};
typedef struct _HTMLEmptyParaSetStyle HTMLEmptyParaSetStyle;

static void set_empty_flow_style (HTMLEngine *e, GtkHTMLFontStyle and_mask, GtkHTMLFontStyle or_mask, HTMLUndoDirection dir);

static void
set_empty_flow_style_undo_action (HTMLEngine *e, HTMLUndoData *undo_data, HTMLUndoDirection dir, guint position_after)
{
	HTMLEmptyParaSetStyle *undo = (HTMLEmptyParaSetStyle *) undo_data;

	set_empty_flow_style (e, undo->and_mask, undo->or_mask, html_undo_direction_reverse (dir));
}

static void
set_empty_flow_style (HTMLEngine *e, GtkHTMLFontStyle and_mask, GtkHTMLFontStyle or_mask, HTMLUndoDirection dir)
{
	HTMLEmptyParaSetStyle *undo;
	GtkHTMLFontStyle old_or_mask;

	g_return_if_fail (html_object_is_text (e->cursor->object));

	old_or_mask = HTML_TEXT (e->cursor->object)->font_style & ~and_mask;
	HTML_TEXT (e->cursor->object)->font_style &= and_mask;
	HTML_TEXT (e->cursor->object)->font_style |= or_mask;

	undo = g_new (HTMLEmptyParaSetStyle, 1);
	html_undo_data_init (HTML_UNDO_DATA (undo));
	undo->and_mask = and_mask;
	undo->or_mask = old_or_mask;
	undo->data.destroy = NULL;
	html_undo_add_action (e->undo,
			      html_undo_action_new ("Set empty paragraph text style", set_empty_flow_style_undo_action,
						    HTML_UNDO_DATA (undo), html_cursor_get_position (e->cursor),
						    html_cursor_get_position (e->cursor)), dir);
}

gboolean
html_engine_set_font_style (HTMLEngine *e,
			    GtkHTMLFontStyle and_mask,
			    GtkHTMLFontStyle or_mask)
{
	gboolean rv;
	GtkHTMLFontStyle old = e->insertion_font_style;

	if (!e)
		return FALSE;

	if (!HTML_IS_ENGINE (e))
		return FALSE;

	if (!e->editable)
		return FALSE;

	/* printf ("and %d or %d\n", and_mask, or_mask); */
	e->insertion_font_style &= and_mask;
	e->insertion_font_style |= or_mask;

	if (html_engine_is_selection_active (e)) {
		struct tmp_font *tf = g_new (struct tmp_font, 1);
		tf->and_mask = and_mask;
		tf->or_mask  = or_mask;
		html_engine_cut_and_paste (e, "Set font style", "Unset font style", object_set_font_style, tf);
		g_free (tf);
		rv = TRUE;
	} else {
		if (e->cursor->object->parent && html_clueflow_is_empty (HTML_CLUEFLOW (e->cursor->object->parent))) {
			set_empty_flow_style (e, and_mask, or_mask, HTML_UNDO_UNDO);
		}
		rv = (old == e->insertion_font_style) ? FALSE : TRUE;
	}
	return rv;
}

gboolean
html_engine_toggle_font_style (HTMLEngine *engine, GtkHTMLFontStyle style)
{
	GtkHTMLFontStyle cur_style;

	cur_style = html_engine_get_font_style (engine);

	if (cur_style & style)
		return html_engine_set_font_style (engine, GTK_HTML_FONT_STYLE_MAX & ~style, 0);
	else
		return html_engine_set_font_style (engine, GTK_HTML_FONT_STYLE_MAX, style);
}

static GtkHTMLFontStyle
inc_dec_size (GtkHTMLFontStyle style, gboolean inc)
{
	GtkHTMLFontStyle size;

	if (style == GTK_HTML_FONT_STYLE_DEFAULT)
		style = GTK_HTML_FONT_STYLE_SIZE_3;

	size = style & GTK_HTML_FONT_STYLE_SIZE_MASK;
	if (inc && size < GTK_HTML_FONT_STYLE_SIZE_7)
		size++;
	else if (!inc && size > GTK_HTML_FONT_STYLE_SIZE_1)
		size--;

	style &= ~GTK_HTML_FONT_STYLE_SIZE_MASK;
	style |= size;

	return style;
}

static void
inc_dec_size_cb (HTMLObject *o, HTMLEngine *e, gpointer data)
{
	if (html_object_is_text (o)) {
		html_text_set_font_style (HTML_TEXT (o), e, inc_dec_size (HTML_TEXT (o)->font_style, GPOINTER_TO_INT (data)));
		if (o->prev)
			html_object_merge (o->prev, o, e, NULL, NULL, NULL);
	}
}

void
html_engine_font_size_inc_dec (HTMLEngine *e, gboolean inc)
{
	if (html_engine_is_selection_active (e))
		html_engine_cut_and_paste (e,
					   inc ? "Increase font size" : "Decrease font size",
					   inc ? "Decrease font size" : "Increase font size",
					   inc_dec_size_cb, GINT_TO_POINTER (inc));
	else
		e->insertion_font_style = inc_dec_size (e->insertion_font_style, inc);
}

static void
set_color (HTMLObject *o, HTMLEngine *e, gpointer data)
{
	if (html_object_is_text (o)) {
		HTMLObject *prev;

		html_text_set_color (HTML_TEXT (o), (HTMLColor *) data);

		if (o->parent) {
			prev = html_object_prev_not_slave (o);
			if (prev) {
				html_object_merge (prev, o, e, NULL, NULL, NULL);
			}
		}
	}
}

gboolean
html_engine_set_color (HTMLEngine *e, HTMLColor *color)
{
	gboolean rv = TRUE;

	if (!color)
		color = html_colorset_get_color (e->settings->color_set, HTMLTextColor);

	if (html_engine_is_selection_active (e))
		html_engine_cut_and_paste (e, "Set color", "Unset color", set_color, color);
	else {
		if (gdk_color_equal (&e->insertion_color->color, &color->color))
			rv = FALSE;
	}
	html_color_unref (e->insertion_color);

	e->insertion_color = color;
	html_color_ref (e->insertion_color);

	return rv;
}

/* URL/Target

   get actual url/target
*/

const gchar *
html_engine_get_url (HTMLEngine *e)
{
	return e->insertion_url;
}

const gchar *
html_engine_get_target (HTMLEngine *e)
{
	return e->insertion_target;
}

void
html_engine_set_url (HTMLEngine *e, const gchar *url)
{
	if (e->insertion_url)
		g_free (e->insertion_url);
	e->insertion_url = g_strdup (url);
}

void
html_engine_set_target (HTMLEngine *e, const gchar *target)
{
	if (e->insertion_target)
		g_free (e->insertion_target);
	e->insertion_target = g_strdup (target);
}

/* get url/target from document */

static const gchar *
get_url_or_target_from_selection (HTMLEngine *e, gboolean get_url)
{
	const gchar *str = NULL;
	HTMLPoint p;

	g_return_val_if_fail (e->clue != NULL, NULL);
	g_return_val_if_fail (html_engine_is_selection_active (e), NULL);

	p = e->selection->from;
	while (1) {
		str = get_url ? html_object_get_url (p.object, p.offset) : html_object_get_target (p.object, p.offset);
		if (str || html_point_cursor_object_eq (&p, &e->selection->to))
			break;
		html_point_next_cursor (&p);

		if (p.object == NULL) {
			g_warning ("Unable to find url by end of selection");
			return str;
		}
	}

	return str;
}

static HTMLObject *
html_engine_text_style_object (HTMLEngine *e, gint *offset)
{
	if (HTML_IS_TEXT (e->cursor->object)
	    || (e->cursor->offset && e->cursor->offset != html_object_get_length (e->cursor->object))) {
		if (offset)
			*offset = e->cursor->offset;
		return e->cursor->object;
	}

	if (e->cursor->offset) {
		HTMLObject *next;

		next = html_object_next_not_slave (e->cursor->object);
		if (next && HTML_IS_TEXT (next)) {
			if (offset)
				*offset = 0;
			return next;
		}
	} else {
		HTMLObject *prev;

		prev = html_object_prev_not_slave (e->cursor->object);
		if (prev && HTML_IS_TEXT (prev)) {
			if (offset)
				*offset = html_object_get_length (prev);
			return prev;
		}
	}

	return NULL;
}

const gchar *
html_engine_get_document_url (HTMLEngine *e)
{
	if (html_engine_is_selection_active (e))
		return get_url_or_target_from_selection (e, TRUE);
	else {
		HTMLObject *obj;
		gint offset;

		obj = html_engine_text_style_object (e, &offset);
		return obj ? html_object_get_url (obj, offset) : NULL;
	}
}

const gchar *
html_engine_get_document_target (HTMLEngine *e)
{
	if (html_engine_is_selection_active (e))
		return get_url_or_target_from_selection (e, FALSE);
	else {
		HTMLObject *obj;
		gint offset;

		obj = html_engine_text_style_object (e, &offset);
		return obj ? html_object_get_target (obj, offset) : NULL;
	}
}

gboolean
html_engine_update_insertion_url_and_target (HTMLEngine *engine)
{
	const gchar *url, *target;
	gboolean retval = FALSE;

	url    = html_engine_get_document_url    (engine);
	target = html_engine_get_document_target (engine);

	if (url != engine->insertion_url) {
		html_engine_set_url (engine, url);
		retval = TRUE;
	}

	if (target != engine->insertion_target) {
		html_engine_set_target (engine, target);
		retval = TRUE;
	}

	return retval;
}
