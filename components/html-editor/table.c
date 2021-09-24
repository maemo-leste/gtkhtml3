/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 2001 Ximian, Inc.
    Authors:           Radek Doulik (rodo@ximian.com)

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
#ifdef GNOME_GTKHTML_EDITOR_SHLIB
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif
#include <string.h>
#include <glade/glade.h>
#include "gi-color-combo.h"

#include "gtkhtml.h"

#include "htmlclue.h"
#include "htmlcolor.h"
#include "htmlcolorset.h"
#include "htmlcursor.h"
#include "htmlengine.h"
#include "htmlengine-edit-table.h"
#include "htmlengine-save.h"
#include "htmlimage.h"
#include "htmltable.h"
#include "htmlsettings.h"
#include "gtkhtml-private.h"

#include "properties.h"
#include "table.h"
#include "utils.h"

typedef struct
{
	GtkHTMLControlData *cd;

	HTMLTable *table;

	GtkWidget *combo_bg_color;
	GtkWidget *entry_bg_pixmap;
	GtkWidget *spin_spacing;
	GtkWidget *spin_padding;
	GtkWidget *spin_border;
	GtkWidget *option_align;
	GtkWidget *spin_width;
	GtkWidget *check_width;
	GtkWidget *option_width;
	GtkWidget *spin_cols;
	GtkWidget *spin_rows;

	gboolean   disable_change;
} GtkHTMLEditTableProperties;

static GtkHTMLEditTableProperties *
data_new (GtkHTMLControlData *cd, HTMLTable *table)
{
	GtkHTMLEditTableProperties *data = g_new0 (GtkHTMLEditTableProperties, 1);

	/* fill data */
	data->cd = cd;
	data->table = table;

	return data;
}

static void
changed_bg_color (GtkWidget *w, GdkColor *color, gboolean custom, gboolean by_user, gboolean is_default, GtkHTMLEditTableProperties *d)
{
	/* If the color was changed programatically there's not need to set things */
	if (!by_user)
		return;

	html_engine_table_set_bg_color (d->cd->html->engine, d->table, color);
}

static void
changed_bg_pixmap (GtkWidget *w, GtkHTMLEditTableProperties *d)
{
	char *url;
	const char *file;

	if (d->disable_change || !editor_has_html_object (d->cd, HTML_OBJECT (d->table)))
		return;

	html_cursor_forward (d->cd->html->engine->cursor, d->cd->html->engine);
	file = (const char *) gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (w));
	url = gtk_html_filename_to_uri (file);
	html_engine_table_set_bg_pixmap (d->cd->html->engine, d->table, url);
	g_free (url);
}

static void
changed_spacing (GtkWidget *w, GtkHTMLEditTableProperties *d)
{
	if (d->disable_change || !editor_has_html_object (d->cd, HTML_OBJECT (d->table)))
		return;

	html_cursor_forward (d->cd->html->engine->cursor, d->cd->html->engine);
	html_engine_table_set_spacing (d->cd->html->engine, d->table, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (d->spin_spacing)), FALSE);
}

static void
changed_padding (GtkWidget *w, GtkHTMLEditTableProperties *d)
{
	if (d->disable_change || !editor_has_html_object (d->cd, HTML_OBJECT (d->table)))
		return;

	html_cursor_forward (d->cd->html->engine->cursor, d->cd->html->engine);
	html_engine_table_set_padding (d->cd->html->engine, d->table, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (d->spin_padding)), FALSE);
}

static void
changed_border (GtkWidget *w, GtkHTMLEditTableProperties *d)
{
	if (d->disable_change || !editor_has_html_object (d->cd, HTML_OBJECT (d->table)))
		return;

	html_cursor_forward (d->cd->html->engine->cursor, d->cd->html->engine);
	html_engine_table_set_border_width (d->cd->html->engine, d->table, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (d->spin_border)), FALSE);
}

static void
changed_align (GtkComboBox *combo_box, GtkHTMLEditTableProperties *d)
{
	HTMLHAlignType align = HTML_HALIGN_LEFT;

	if (d->disable_change || !editor_has_html_object (d->cd, HTML_OBJECT (d->table)))
		return;

	align += gtk_combo_box_get_active (combo_box);
	html_cursor_forward (d->cd->html->engine->cursor, d->cd->html->engine);
	html_engine_table_set_align (d->cd->html->engine, d->table, align);
}

static void
set_width (GtkHTMLEditTableProperties *d)
{
	if (d->disable_change || !editor_has_html_object (d->cd, HTML_OBJECT (d->table)))
		return;

	html_cursor_forward (d->cd->html->engine->cursor, d->cd->html->engine);
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (d->check_width)))
		html_engine_table_set_width (
			d->cd->html->engine, d->table,
			gtk_spin_button_get_value_as_int (
				GTK_SPIN_BUTTON (d->spin_width)),
			gtk_combo_box_get_active (
				GTK_COMBO_BOX (d->option_width)) > 0);
	else
		html_engine_table_set_width (
			d->cd->html->engine, d->table, 0, FALSE);
}

static void
changed_width (GtkWidget *w, GtkHTMLEditTableProperties *d)
{
	set_width (d);
}

static void
set_has_width (GtkWidget *check, GtkHTMLEditTableProperties *d)
{
	set_width (d);
}

static void
changed_width_percent (GtkWidget *w, GtkHTMLEditTableProperties *d)
{
	set_width (d);
}

static void
changed_cols (GtkWidget *w, GtkHTMLEditTableProperties *d)
{
	if (d->disable_change || !editor_has_html_object (d->cd, HTML_OBJECT (d->table)))
		return;

	html_cursor_jump_to (d->cd->html->engine->cursor, d->cd->html->engine, (HTMLObject *)d->table, 1);
	html_cursor_backward (d->cd->html->engine->cursor, d->cd->html->engine);
	html_engine_table_set_cols (d->cd->html->engine, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (d->spin_cols)));
}

static void
changed_rows (GtkWidget *w, GtkHTMLEditTableProperties *d)
{
	if (d->disable_change || !editor_has_html_object (d->cd, HTML_OBJECT (d->table)))
		return;

	html_cursor_jump_to (d->cd->html->engine->cursor, d->cd->html->engine, (HTMLObject *)d->table, 1);
	html_cursor_backward (d->cd->html->engine->cursor, d->cd->html->engine);
	html_engine_table_set_rows (d->cd->html->engine, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (d->spin_rows)));
}

/*
 * FIX: set spin adjustment upper to 100000
 *      as glade cannot set it now
 */
#define UPPER_FIX(x) gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (d->spin_ ## x))->upper = 100000.0

static GtkWidget *
table_widget (GtkHTMLEditTableProperties *d)
{
	GtkWidget *table_page;
	GladeXML *xml;
	gchar *filename;

	filename = g_build_filename (GLADE_DATADIR, "gtkhtml-editor-properties.glade", NULL);
	xml = glade_xml_new (filename, "table_page", GETTEXT_PACKAGE);
	g_free (filename);
	if (!xml)
		g_error (_("Could not load glade file."));

	table_page = glade_xml_get_widget (xml, "table_page");

	d->combo_bg_color = gi_color_combo_new (NULL, _("Transparent"), NULL,
					     color_group_fetch ("table_bg_color", d->cd));
        gi_color_combo_box_set_preview_relief (GI_COLOR_COMBO (d->combo_bg_color), GTK_RELIEF_NORMAL); \
        g_signal_connect (d->combo_bg_color, "color_changed", G_CALLBACK (changed_bg_color), d);
        gtk_label_set_mnemonic_widget (GTK_LABEL (glade_xml_get_widget (xml, "label141")), GTK_WIDGET (d->combo_bg_color));
	gtk_box_pack_start (GTK_BOX (glade_xml_get_widget (xml, "bg_color_hbox")), d->combo_bg_color, FALSE, FALSE, 0);

	d->entry_bg_pixmap = glade_xml_get_widget (xml, "entry_table_bg_pixmap");
	g_signal_connect (GTK_FILE_CHOOSER_BUTTON (d->entry_bg_pixmap),
			  "selection-changed", G_CALLBACK (changed_bg_pixmap), d);

	d->spin_spacing = glade_xml_get_widget (xml, "spin_spacing");
	g_signal_connect (d->spin_spacing, "value_changed", G_CALLBACK (changed_spacing), d);
	d->spin_padding = glade_xml_get_widget (xml, "spin_padding");
	g_signal_connect (d->spin_padding, "value_changed", G_CALLBACK (changed_padding), d);
	d->spin_border  = glade_xml_get_widget (xml, "spin_border");
	g_signal_connect (d->spin_border, "value_changed", G_CALLBACK (changed_border), d);
	UPPER_FIX (padding);
	UPPER_FIX (spacing);
	UPPER_FIX (border);

	d->option_align = glade_xml_get_widget (xml, "option_table_align");
	g_signal_connect (d->option_align, "changed", G_CALLBACK (changed_align), d);

	d->spin_width   = glade_xml_get_widget (xml, "spin_table_width");
	g_signal_connect (d->spin_width, "value_changed", G_CALLBACK (changed_width), d);
	UPPER_FIX (width);
	d->check_width  = glade_xml_get_widget (xml, "check_table_width");
	g_signal_connect (d->check_width, "toggled", G_CALLBACK (set_has_width), d);
	d->option_width = glade_xml_get_widget (xml, "option_table_width");
	g_signal_connect (d->option_width, "changed", G_CALLBACK (changed_width_percent), d);

	d->spin_cols = glade_xml_get_widget (xml, "spin_table_columns");
	g_signal_connect (d->spin_cols, "value_changed", G_CALLBACK (changed_cols), d);
	d->spin_rows = glade_xml_get_widget (xml, "spin_table_rows");
	g_signal_connect (d->spin_rows, "value_changed", G_CALLBACK (changed_rows), d);
	UPPER_FIX (cols);
	UPPER_FIX (rows);

	gtk_widget_show_all (table_page);
	gtk_file_chooser_set_preview_widget_active (GTK_FILE_CHOOSER (d->entry_bg_pixmap), FALSE);

	return table_page;
}

static void
set_ui (GtkHTMLEditTableProperties *d)
{
	if (editor_has_html_object (d->cd, HTML_OBJECT (d->table))) {
		HTMLHAlignType halign;
		int width = 0;
		gboolean percent = FALSE, has_width = FALSE;

		d->disable_change = TRUE;

		html_cursor_forward (d->cd->html->engine->cursor, d->cd->html->engine);
		gi_color_combo_set_color (GI_COLOR_COMBO (d->combo_bg_color), d->table->bgColor);

		if (d->table->bgPixmap) {
			gchar *filename = gtk_html_filename_from_uri (d->table->bgPixmap->url);

			gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (d->entry_bg_pixmap),
					    filename);
			g_free (filename);
		}

		gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_spacing), d->table->spacing);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_padding), d->table->padding);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_border),  d->table->border);

		g_return_if_fail (HTML_OBJECT (d->table)->parent);
		halign = HTML_CLUE (HTML_OBJECT (d->table)->parent)->halign;
		if (halign == HTML_HALIGN_NONE)
			halign = HTML_HALIGN_LEFT;
		gtk_combo_box_set_active (
			GTK_COMBO_BOX (d->option_align),
			halign - HTML_HALIGN_LEFT);

		if (HTML_OBJECT (d->table)->percent) {
			width = HTML_OBJECT (d->table)->percent;
			percent = TRUE;
			has_width = TRUE;
		} else if (d->table->specified_width) {
			width = d->table->specified_width;
			percent = FALSE;
			has_width = TRUE;
		} else
			has_width = FALSE;

		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (d->check_width), has_width);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_width),  width);
		gtk_combo_box_set_active (
			GTK_COMBO_BOX (d->option_width),
			percent ? 1 : 0);

		gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_cols),  d->table->totalCols);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_rows),  d->table->totalRows);

		d->disable_change = FALSE;
	}
}

GtkWidget *
table_properties (GtkHTMLControlData *cd, gpointer *set_data)
{
	GtkHTMLEditTableProperties *data = data_new (cd, html_engine_get_table (cd->html->engine));
	GtkWidget *rv;

	*set_data = data;
	rv = table_widget (data);
	set_ui (data);

	return rv;
}

void
table_close_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	g_free (get_data);
}
