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

#include <config.h>
#ifdef GNOME_GTKHTML_EDITOR_SHLIB
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif
#include "control-data.h"
#include "spellchecker.h"
#include "properties.h"

GtkHTMLControlData *
gtk_html_control_data_new (GtkHTML *html, GtkWidget *vbox)
{
	GtkHTMLControlData * ncd = g_new0 (GtkHTMLControlData, 1);

	ncd->html                    = html;
	ncd->vbox                    = vbox;
	ncd->paragraph_option        = NULL;
	ncd->properties_dialog       = NULL;
	ncd->properties_types        = NULL;
	ncd->block_font_style_change = FALSE;
	ncd->dict                    = spell_new_dictionary ();
	ncd->gdk_painter             = NULL;
	ncd->plain_painter           = NULL;
	ncd->format_html             = FALSE;
	ncd->control                 = NULL;

	ncd->search_text             = NULL;
	ncd->replace_text_search     = NULL;
	ncd->replace_text_replace    = NULL;
	ncd->has_spell_control_set   = FALSE;
	ncd->language                = NULL;
	ncd->paragraph_style_store   = NULL;

	ncd->file_path = g_strdup (g_get_home_dir ());

	spell_init (html, ncd);

	return ncd;
}

void
gtk_html_control_data_destroy (GtkHTMLControlData *cd)
{
	g_assert (cd);

	if (cd->properties_dialog)
		gtk_html_edit_properties_dialog_destroy (cd->properties_dialog);

	if (cd->search_dialog)
		gtk_html_search_dialog_destroy (cd->search_dialog);
	g_free (cd->search_text);

	if (cd->replace_dialog)
		gtk_html_replace_dialog_destroy (cd->replace_dialog);
	g_free (cd->replace_text_search);
	g_free (cd->replace_text_replace);

	/* printf ("release dict\n"); */
	bonobo_object_release_unref (cd->dict, NULL);

	if (cd->plain_painter)
		g_object_unref (cd->plain_painter);

	if (cd->gdk_painter)
		g_object_unref (cd->gdk_painter);

	if (cd->languages)
		CORBA_free (cd->languages);
	g_free (cd->language);

	if (cd->paragraph_style_store)
		g_object_unref (cd->paragraph_style_store);

	g_free (cd->file_path);

	g_free (cd);
}
