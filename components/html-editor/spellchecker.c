/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library

   Copyright (C) 2000 Helix Code, Inc.
   Authors:           Radek Doulik (rodo@helixcode.com)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHcANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <config.h>
#include <glib.h>
#ifdef GNOME_GTKHTML_EDITOR_SHLIB
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#include <string.h>

#include "gtkhtml.h"
#include "gtkhtml-properties.h"

#include "htmlclueflow.h"
#include "htmlcursor.h"
#include "htmlengine.h"
#include "htmlengine-edit.h"
#include "htmlengine-edit-cursor.h"
#include "htmlengine-edit-movement.h"
#include "htmlengine-edit-selection-updater.h"
#include "htmlobject.h"
#include "htmlselection.h"

#include "menubar.h"
#include "spellchecker.h"

#define CONTROL_IID "OAFIID:GNOME_Spell_Control:0.3"

void
spell_suggestion_request (GtkHTML *html, gpointer data)
{
	GtkHTMLControlData *cd = (GtkHTMLControlData *) data;

	spell_check_dialog (cd, FALSE);
}

GNOME_Spell_Dictionary
spell_new_dictionary (void)
{
#define DICTIONARY_IID "OAFIID:GNOME_Spell_Dictionary:0.3"

	GNOME_Spell_Dictionary dictionary = bonobo_get_object (DICTIONARY_IID, "GNOME/Spell/Dictionary", NULL);

	if (dictionary == CORBA_OBJECT_NIL) {
		g_warning ("Cannot create spell dictionary instance (iid:%s)", DICTIONARY_IID);
	}
#undef DICTIONARY_IID

	return dictionary;
}

gboolean
spell_check_word (GtkHTML *html, const gchar *word, gpointer data)
{
	GtkHTMLControlData *cd = (GtkHTMLControlData *) data;
	CORBA_Environment   ev;
	gboolean rv;

	if (!cd->dict)
		return TRUE;

	/* printf ("check word: %s\n", word); */
	CORBA_exception_init (&ev);
	rv = GNOME_Spell_Dictionary_checkWord (cd->dict, word, &ev);
	if (ev._major == CORBA_SYSTEM_EXCEPTION)
		rv = TRUE;
	CORBA_exception_free (&ev);

	return rv;
}

void
spell_add_to_session (GtkHTML *html, const gchar *word, gpointer data)
{
	GtkHTMLControlData *cd = (GtkHTMLControlData *) data;
	CORBA_Environment   ev;

	g_return_if_fail (word);

	if (!cd->dict)
		return;

	CORBA_exception_init (&ev);
	GNOME_Spell_Dictionary_addWordToSession (cd->dict, word, &ev);
	CORBA_exception_free (&ev);
}

static void
spell_add_to_personal_no_language (void)
{
	GtkWidget *info;

	info = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO,
				       GTK_BUTTONS_OK, _("Unable to add word to dictionary,\nlanguage settings are broken.\n"));
	gtk_dialog_run (GTK_DIALOG (info));
	gtk_widget_destroy (info);
}

void
spell_add_to_personal (GtkHTML *html, const gchar *word, const gchar *language, gpointer data)
{
	GtkHTMLControlData *cd = (GtkHTMLControlData *) data;
	CORBA_Environment   ev;

	g_return_if_fail (word);

	if (!cd->dict)
		return;

	if (language) {
		CORBA_exception_init (&ev);
		GNOME_Spell_Dictionary_addWordToPersonal (cd->dict, word, language, &ev);
		CORBA_exception_free (&ev);
	} else
		spell_add_to_personal_no_language ();
}

void
spell_set_language (GtkHTML *html, const gchar *language, gpointer data)
{
	GtkHTMLControlData *cd = (GtkHTMLControlData *) data;
	CORBA_Environment   ev;

	/* printf ("spell_set_language\n"); */
	if (!cd->dict)
		return;

	/* printf ("spell_set_language '%s'\n", language); */

	CORBA_exception_init (&ev);
	GNOME_Spell_Dictionary_setLanguage (cd->dict, language, &ev);
	CORBA_exception_free (&ev);

	g_free (cd->language);
	cd->language = g_strdup (language);

	menubar_set_languages (cd);
}

void
spell_init (GtkHTML *html, GtkHTMLControlData *cd)
{
}

static void
set_word (GtkHTMLControlData *cd)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);
	html_engine_select_spell_word_editable (cd->html->engine);
	bonobo_property_bag_client_set_value_string (cd->spell_control_pb, "word",
						     html_engine_get_spell_word (cd->html->engine), &ev);
	CORBA_exception_free (&ev);
}

static gboolean
next_word (GtkHTMLControlData *cd, gboolean forward)
{
	gboolean rv = TRUE;
	if (!forward)
		html_engine_backward_word (cd->html->engine);
	while ((forward ? html_engine_forward_word (cd->html->engine) : html_engine_backward_word (cd->html->engine))
	       && (rv = html_engine_spell_word_is_valid (cd->html->engine)))
		;

	return rv;
}

static void
check_next_word (GtkHTMLControlData *cd, gboolean update, gboolean forward)
{
	HTMLEngine *e = cd->html->engine;

	html_engine_disable_selection (e);
	if (update)
		html_engine_spell_check (e);

	if (!cd->spell_check_next || next_word (cd, forward)) {
		gtk_dialog_response (GTK_DIALOG (cd->spell_dialog), GTK_RESPONSE_CLOSE);
	} else {
		set_word (cd);
	}
}

static void
replace_cb (BonoboListener *listener, const char *event_name, const CORBA_any *arg, CORBA_Environment *ev, gpointer user_data)
{
	GtkHTMLControlData *cd = (GtkHTMLControlData *) user_data;

	html_engine_replace_spell_word_with (cd->html->engine, BONOBO_ARG_GET_STRING (arg));
	check_next_word (cd, FALSE, TRUE);
}

static void 
replace_all_cb (BonoboListener *listener, const char *event_name, const CORBA_any *arg, CORBA_Environment *ev, gpointer user_data)
{
	GtkHTMLControlData *cd = (GtkHTMLControlData *) user_data;
	const gchar * err_word = NULL;

	err_word = html_engine_get_spell_word (cd->html->engine);
	g_return_if_fail (err_word);

	html_engine_replace_spell_word_with (cd->html->engine, BONOBO_ARG_GET_STRING (arg)); 
	while (!next_word (cd, TRUE))	
		if (!strcmp (err_word, html_engine_get_spell_word (cd->html->engine)))
			html_engine_replace_spell_word_with (cd->html->engine, BONOBO_ARG_GET_STRING (arg));
	html_engine_beginning_of_document (cd->html->engine);
	check_next_word (cd, FALSE, TRUE);
}

static void
skip_cb (BonoboListener *listener, const char *event_name, const CORBA_any *arg, CORBA_Environment *ev, gpointer user_data)
{
	check_next_word ((GtkHTMLControlData *) user_data, FALSE, TRUE);
}

static void
back_cb (BonoboListener *listener, const char *event_name, const CORBA_any *arg, CORBA_Environment *ev, gpointer user_data)
{
	check_next_word ((GtkHTMLControlData *) user_data, FALSE, FALSE);
}

static void
add_cb (BonoboListener *listener, const char *event_name, const CORBA_any *arg, CORBA_Environment *ev, gpointer user_data)
{
	GtkHTMLControlData *cd = (GtkHTMLControlData *) user_data;
	gchar *word, *language;

	word = html_engine_get_spell_word (cd->html->engine);
	g_return_if_fail (word);

	language = BONOBO_ARG_GET_STRING (arg);

	if (language) {
		GNOME_Spell_Dictionary_addWordToPersonal (cd->dict, word, language, ev);
		check_next_word ((GtkHTMLControlData *) user_data, TRUE, TRUE);
	} else
		spell_add_to_personal_no_language ();

	g_free (word);
}

static void
ignore_cb (BonoboListener *listener, const char *event_name, const CORBA_any *arg, CORBA_Environment *ev, gpointer user_data)
{
	GtkHTMLControlData *cd = (GtkHTMLControlData *) user_data;
	gchar *word;

	word = html_engine_get_spell_word (cd->html->engine);
	g_return_if_fail (word);

	GNOME_Spell_Dictionary_addWordToSession (cd->dict, word, ev);
	g_free (word);
	check_next_word ((GtkHTMLControlData *) user_data, TRUE, TRUE);
}

gboolean
spell_has_control (void)
{
	GtkWidget *control;
	gboolean rv;

	control = bonobo_widget_new_control (CONTROL_IID, CORBA_OBJECT_NIL);
	rv = control != NULL;

	if (control) {
		g_object_ref_sink (control);
		g_object_unref (control);
	}
	return rv;
}

void
spell_check_dialog (GtkHTMLControlData *cd, gboolean whole_document)
{
	GtkWidget *control;
	GtkWidget *dialog;
	guint position;
	gboolean inline_spelling = gtk_html_get_inline_spelling (cd->html);

	position = cd->html->engine->cursor->position;
	cd->spell_check_next = whole_document;
	if (whole_document) {
		html_engine_disable_selection (cd->html->engine);
		html_engine_beginning_of_document (cd->html->engine);
		if (!inline_spelling)
			gtk_html_set_inline_spelling (cd->html, TRUE);
	}

	if (html_engine_spell_word_is_valid (cd->html->engine))
		if (next_word (cd, TRUE)) {
			GtkWidget *info;

			html_engine_hide_cursor (cd->html->engine);
			html_cursor_jump_to_position (cd->html->engine->cursor, cd->html->engine, position);
			html_engine_show_cursor (cd->html->engine);

			info = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO,
						       GTK_BUTTONS_OK, _("No misspelled word found"));
			gtk_dialog_run (GTK_DIALOG (info));
			gtk_widget_destroy (info);

			goto end;
		}

	dialog  = gtk_dialog_new_with_buttons (_("Spell checker"), NULL, 0, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);
	control = bonobo_widget_new_control (CONTROL_IID, CORBA_OBJECT_NIL);

	if (!control) {
		g_warning ("Cannot create spell control");
		g_object_unref (dialog);
		goto end;
	}

	cd->spell_dialog = dialog;
        cd->spell_control_pb = bonobo_control_frame_get_control_property_bag
		(bonobo_widget_get_control_frame (BONOBO_WIDGET (control)), NULL);
	bonobo_property_bag_client_set_value_string (cd->spell_control_pb, "language", html_engine_get_language (cd->html->engine), NULL);
	bonobo_property_bag_client_set_value_gboolean (cd->spell_control_pb, "single", !whole_document, NULL);

	bonobo_event_source_client_add_listener (cd->spell_control_pb, replace_cb, "Bonobo/Property:change:replace", NULL, cd);
	bonobo_event_source_client_add_listener (cd->spell_control_pb, replace_all_cb, "Bonobo/Property:change:repall", NULL, cd);
	bonobo_event_source_client_add_listener (cd->spell_control_pb, add_cb, "Bonobo/Property:change:add", NULL, cd);
	bonobo_event_source_client_add_listener (cd->spell_control_pb, ignore_cb, "Bonobo/Property:change:ignore", NULL, cd);
	bonobo_event_source_client_add_listener (cd->spell_control_pb, skip_cb, "Bonobo/Property:change:skip", NULL, cd);
	bonobo_event_source_client_add_listener (cd->spell_control_pb, back_cb, "Bonobo/Property:change:back", NULL, cd);
	set_word (cd);

	gtk_widget_show (control);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), control);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	bonobo_object_release_unref (cd->spell_control_pb, NULL);
	cd->spell_control_pb = CORBA_OBJECT_NIL;

 end:
	if (!inline_spelling)
		gtk_html_set_inline_spelling (cd->html, inline_spelling);
}

static void
language_cb (BonoboUIComponent *uic, const char *path, Bonobo_UIComponent_EventType type,
	     const char *state, gpointer user_data)
{
	GtkHTMLControlData *cd = (GtkHTMLControlData *) user_data;
	GString *str, *lang;
	gchar *val;
	gint i;

	if (cd->block_language_changes || !cd->languages)
		return;

	/* printf ("language callback %s\n", path); */

	str = g_string_new (NULL);
	lang = g_string_new (NULL);
	for (i = 0; i < cd->languages->_length; i ++) {
		g_string_printf (lang, "/commands/SpellLanguage%d", i + 1);
		val = bonobo_ui_component_get_prop (cd->uic, lang->str, "state", NULL);
		if (val && *val == '1') {
			g_string_append (str, cd->languages->_buffer [i].abbreviation);
			g_string_append_c (str, ' ');
		}
		g_free (val);
	}

	html_engine_set_language (cd->html->engine, str->str);
	g_free (cd->language);
	cd->language = str->str;

	g_string_free (str, FALSE);
	g_string_free (lang, TRUE);
}

void
spell_create_language_menu (GtkHTMLControlData *cd)
{
	CORBA_sequence_GNOME_Spell_Language *seq;
	CORBA_Environment ev;

	if (cd->dict == CORBA_OBJECT_NIL)
		return;

	if (cd->languages)
		CORBA_free (cd->languages);

	CORBA_exception_init (&ev);
	cd->languages = seq = GNOME_Spell_Dictionary_getLanguages (cd->dict, &ev);

	if (BONOBO_EX (&ev))
		cd->languages = seq = NULL;

	CORBA_exception_free (&ev);

	if (seq && seq->_length > 0) {
		GString *str;
		gchar *line;
		gint i;

		str = g_string_new ("<submenu name=\"EditSpellLanguagesSubmenu\" _label=\"");
		g_string_append (str, _("Current _Languages"));
		g_string_append (str, "\">\n");
		for (i = 0; i < seq->_length; i ++) {
			line = g_strdup_printf ("<menuitem name=\"SpellLanguage%d\" _label=\"%s\""
						" verb=\"SpellLanguage%d\" type=\"toggle\"/>\n",
						i + 1, seq->_buffer [i].name, i + 1);
			g_string_append (str, line);
			g_free (line);
		}
		g_string_append (str, "</submenu>\n");

		bonobo_ui_component_set_translate (cd->uic, "/menu/Edit/EditMisc/EditSpellLanguages/", str->str, NULL);

		for (i = 0; i < seq->_length; i ++) {
			g_string_printf (str, "SpellLanguage%d", i + 1);
			bonobo_ui_component_add_listener (cd->uic, str->str, language_cb, cd);
		}
		g_string_free (str, TRUE);
	}
}
