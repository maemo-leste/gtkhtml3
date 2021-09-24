/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

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

    Author: Radek Doulik <rodo@helixcode.com>
*/

#ifndef _CONTROL_DATA_H
#define _CONTROL_DATA_H

typedef struct _GtkHTMLControlData            GtkHTMLControlData;
typedef enum   _GtkHTMLEditPropertyType       GtkHTMLEditPropertyType;
typedef struct _GtkHTMLEditPropertiesDialog   GtkHTMLEditPropertiesDialog;

#include <gtkhtml.h>
#include <bonobo.h>
#include "persist-stream.h"
#include "htmlgdkpainter.h"
#include "search.h"
#include "replace.h"
#include "image.h"
#include "link.h"
#include "rule.h"
#include "engine.h"
#include "Spell.h"

struct _GtkHTMLControlData {
	GtkHTML    *html;
	GtkWidget  *vbox;
	GtkWidget  *cpicker;
	GtkWidget  *combo;
	GtkWidget  *paragraph_option;

	BonoboUIComponent *uic;

	GtkHTMLEditPropertiesDialog   *properties_dialog;
	GList                         *properties_types;

	/* search & replace dialogs */
	GtkHTMLSearchDialog     *search_dialog;
	GtkHTMLReplaceDialog    *replace_dialog;
	gboolean regular;
	gchar *search_text;
	gchar *replace_text_search;
	gchar *replace_text_replace;

	/* html/plain mode settings */
	gboolean format_html;
	HTMLGdkPainter *gdk_painter;
	HTMLGdkPainter *plain_painter;

	/* object from last button press event */
	HTMLObject *obj;

	/* button release signal id */
	guint releaseId;

	/* toolbars */
	GtkWidget *toolbar_commands, *toolbar_style;

	GtkWidget *tt_button;
	GtkWidget *bold_button;
	GtkWidget *italic_button;
	GtkWidget *underline_button;
	GtkWidget *strikeout_button;

	GtkWidget *left_align_button;
	GtkWidget *center_button;
	GtkWidget *right_align_button;

	GtkWidget *indent_button;
	GtkWidget *unindent_button;

	GtkWidget *font_size_menu;

	guint font_style_changed_connection_id;
	gboolean block_font_style_change;

	CORBA_sequence_GNOME_Spell_Language *languages;
	gboolean                block_language_changes;
	gchar                  *language;
	GNOME_Spell_Dictionary  dict;
	EditorEngine           *editor_bonobo_engine;
	BonoboObject           *persist_stream;
        BonoboObject           *persist_file;
	BonoboControl          *control;

	GtkWidget *spell_dialog;
	Bonobo_PropertyBag spell_control_pb;
	gboolean has_spell_control;
	gboolean has_spell_control_set;
	gboolean spell_check_next;

	GtkWidget *file_dialog;
	gboolean file_html;

	GtkListStore *paragraph_style_store;

	/* file path used in dialog when choosing files */
	gchar *file_path;
};

GtkHTMLControlData * gtk_html_control_data_new       (GtkHTML *html, GtkWidget *vbox);
void                 gtk_html_control_data_destroy   (GtkHTMLControlData *cd);

#endif
