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

#include "htmlclue.h"
#include "htmlcolor.h"
#include "htmlcolorset.h"
#include "htmlcursor.h"
#include "htmlengine.h"
#include "htmlengine-edit.h"
#include "htmlengine-edit-table.h"
#include "htmlengine-edit-tablecell.h"
#include "htmlengine-save.h"
#include "htmlimage.h"
#include "htmltable.h"
#include "htmltablecell.h"
#include "htmlsettings.h"
#include "gtkhtml-private.h"

#include "properties.h"
#include "cell.h"
#include "utils.h"

typedef enum
{
	CELL_SCOPE_CELL,
	CELL_SCOPE_ROW,
	CELL_SCOPE_COLUMN,
	CELL_SCOPE_TABLE
} CellScope;

typedef struct
{
	GtkHTMLControlData *cd;
	HTMLTableCell *cell;
	HTMLTable *table;
	CellScope  scope;

	GtkWidget *combo_bg_color;
	GtkWidget *entry_bg_pixmap;
	GtkWidget *option_halign;
	GtkWidget *option_valign;
	GtkWidget *spin_width;
	GtkWidget *check_width;
	GtkWidget *option_width;
	GtkWidget *spin_cspan;
	GtkWidget *spin_rspan;
	GtkWidget *check_wrap;
	GtkWidget *check_header;

	gboolean   disable_change;

} GtkHTMLEditCellProperties;

static GtkHTMLEditCellProperties *
data_new (GtkHTMLControlData *cd, HTMLTableCell *cell)
{
	GtkHTMLEditCellProperties *data = g_new0 (GtkHTMLEditCellProperties, 1);

	/* fill data */
	data->cd = cd;
	data->scope = CELL_SCOPE_CELL;
	data->cell = cell;
	g_return_val_if_fail (data->cell, NULL);
	data->table = HTML_TABLE (HTML_OBJECT (data->cell)->parent);
	g_return_val_if_fail (data->table && HTML_IS_TABLE (data->table), NULL);

	return data;
}

static void
cell_set_prop (GtkHTMLEditCellProperties *d, void (*set_fn)(HTMLTableCell *, GtkHTMLEditCellProperties *))
{
	HTMLEngine *e = d->cd->html->engine;
	guint position;
	if (d->disable_change || !editor_has_html_object (d->cd, HTML_OBJECT (d->table)))
		return;

	position = d->cd->html->engine->cursor->position;
	switch (d->scope) {
	case CELL_SCOPE_CELL:
		(*set_fn) (d->cell, d);
		break;
	case CELL_SCOPE_ROW:
		if (html_engine_table_goto_row (e, d->table, d->cell->row)) {
			HTMLTableCell *cell = html_engine_get_table_cell (e);

			while (cell && cell->row == d->cell->row) {
				if (HTML_OBJECT (cell)->parent == HTML_OBJECT (d->table))
					(*set_fn) (cell, d);
				html_engine_next_cell (e, FALSE);
				cell = html_engine_get_table_cell (e);
			}
		}
		break;
	case CELL_SCOPE_COLUMN:
		if (html_engine_table_goto_col (e, d->table, d->cell->col)) {
			HTMLTableCell *cell = html_engine_get_table_cell (e);

			while (cell) {
				if (cell->col == d->cell->col && HTML_OBJECT (cell)->parent == HTML_OBJECT (d->table))
					(*set_fn) (cell, d);
				html_engine_next_cell (e, FALSE);
				cell = html_engine_get_table_cell (e);
			}
		}
		break;
	case CELL_SCOPE_TABLE:
		if (html_engine_goto_table_0 (e, d->table)) {
			HTMLTableCell *cell;

			html_cursor_forward (e->cursor, e);
			cell = html_engine_get_table_cell (e);
			while (cell) {
				if (HTML_OBJECT (cell)->parent == HTML_OBJECT (d->table))
					(*set_fn) (cell, d);
				html_engine_next_cell (e, FALSE);
				cell = html_engine_get_table_cell (e);
			}
		}
		break;
	}

	html_cursor_jump_to_position (e->cursor, e, position);
}

static void
set_bg_color (HTMLTableCell *cell, GtkHTMLEditCellProperties *d)
{
	html_engine_table_cell_set_bg_color (d->cd->html->engine, cell, gi_color_combo_get_color (GI_COLOR_COMBO (d->combo_bg_color), NULL));
}

static void
changed_bg_color (GtkWidget *w, GdkColor *color, gboolean custom, gboolean by_user, gboolean is_default, GtkHTMLEditCellProperties *d)
{
	cell_set_prop (d, set_bg_color);
}

static void
set_bg_pixmap (HTMLTableCell *cell, GtkHTMLEditCellProperties *d)
{
	const char *file;
	char *url = NULL;

	file = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (d->entry_bg_pixmap));
	url = gtk_html_filename_to_uri (file);

	html_engine_table_cell_set_bg_pixmap (d->cd->html->engine, cell, url);
	g_free (url);
}

static void
changed_bg_pixmap (GtkWidget *w, GtkHTMLEditCellProperties *d)
{
	cell_set_prop (d, set_bg_pixmap);
}

static void
set_halign (HTMLTableCell *cell, GtkHTMLEditCellProperties *d)
{
	HTMLHAlignType halign = HTML_HALIGN_LEFT;

	halign += gtk_combo_box_get_active (GTK_COMBO_BOX (d->option_halign));
	html_engine_table_cell_set_halign (d->cd->html->engine, cell, halign);
}

static void
changed_halign (GtkWidget *w, GtkHTMLEditCellProperties *d)
{
	cell_set_prop (d, set_halign);
}

static void
set_valign (HTMLTableCell *cell, GtkHTMLEditCellProperties *d)
{
	HTMLVAlignType valign = HTML_VALIGN_TOP;

	valign += gtk_combo_box_get_active (GTK_COMBO_BOX (d->option_valign));
	html_engine_table_cell_set_valign (d->cd->html->engine, cell, valign);
}

static void
changed_valign (GtkWidget *w, GtkHTMLEditCellProperties *d)
{
	cell_set_prop (d, set_valign);
}

static void
set_cspan (HTMLTableCell *cell, GtkHTMLEditCellProperties *d)
{
	html_engine_set_cspan (d->cd->html->engine, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (d->spin_cspan)));
}

static void
changed_cspan (GtkWidget *w, GtkHTMLEditCellProperties *d)
{
	cell_set_prop (d, set_cspan);
}

static void
set_rspan (HTMLTableCell *cell, GtkHTMLEditCellProperties *d)
{
	html_engine_set_rspan (d->cd->html->engine, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (d->spin_rspan)));
}

static void
changed_rspan (GtkWidget *w, GtkHTMLEditCellProperties *d)
{
	cell_set_prop (d, set_rspan);
}

static void
set_width (HTMLTableCell *cell, GtkHTMLEditCellProperties *d)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (d->check_width))) {
		gint index = gtk_combo_box_get_active (
			GTK_COMBO_BOX (d->option_width));
		html_engine_table_cell_set_width (
			d->cd->html->engine, cell,
			gtk_spin_button_get_value_as_int (
				GTK_SPIN_BUTTON (d->spin_width)),
			(index > 0));
	} else
		html_engine_table_cell_set_width (d->cd->html->engine, cell, 0, FALSE);
}

static void
changed_width (GtkWidget *w, GtkHTMLEditCellProperties *d)
{
	cell_set_prop (d, set_width);
}

static void
set_has_width (GtkWidget *check, GtkHTMLEditCellProperties *d)
{
	cell_set_prop (d, set_width);
}

static void
changed_width_percent (GtkWidget *w, GtkHTMLEditCellProperties *d)
{
	cell_set_prop (d, set_width);
}

static void
set_wrap (HTMLTableCell *cell, GtkHTMLEditCellProperties *d)
{
	html_engine_table_cell_set_no_wrap (d->cd->html->engine, cell, !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (d->check_wrap)));
}

static void
changed_wrap (GtkWidget *w, GtkHTMLEditCellProperties *d)
{
	cell_set_prop (d, set_wrap);
}

static void
set_header (HTMLTableCell *cell, GtkHTMLEditCellProperties *d)
{
	html_engine_table_cell_set_heading (d->cd->html->engine, cell, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (d->check_header)));
}

static void
changed_heading (GtkWidget *w, GtkHTMLEditCellProperties *d)
{
	cell_set_prop (d, set_header);
}

/*
 * FIX: set spin adjustment upper to 100000
 *      as glade cannot set it now
 */
#define UPPER_FIX(x) gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (d->spin_ ## x))->upper = 100000.0

static void
cell_scope_cell (GtkWidget *w, GtkHTMLEditCellProperties *d)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
		d->scope = CELL_SCOPE_CELL;
}

static void
cell_scope_table (GtkWidget *w, GtkHTMLEditCellProperties *d)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
		d->scope = CELL_SCOPE_TABLE;
}

static void
cell_scope_row (GtkWidget *w, GtkHTMLEditCellProperties *d)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
		d->scope = CELL_SCOPE_ROW;
}

static void
cell_scope_column (GtkWidget *w, GtkHTMLEditCellProperties *d)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
		d->scope = CELL_SCOPE_COLUMN;
}

static GtkWidget *
cell_widget (GtkHTMLEditCellProperties *d)
{
	GtkWidget *cell_page;
	GladeXML *xml;
	gchar *filename;

	filename = g_build_filename (GLADE_DATADIR, "gtkhtml-editor-properties.glade", NULL);
	xml = glade_xml_new (filename, "cell_page", GETTEXT_PACKAGE);
	g_free (filename);
	if (!xml)
		g_error (_("Could not load glade file."));

	cell_page          = glade_xml_get_widget (xml, "cell_page");

	d->combo_bg_color = gi_color_combo_new (NULL, _("Transparent"), NULL,
					     color_group_fetch ("cell_bg_color", d->cd));
        gi_color_combo_box_set_preview_relief (GI_COLOR_COMBO (d->combo_bg_color), GTK_RELIEF_NORMAL); \
        g_signal_connect (d->combo_bg_color, "color_changed", G_CALLBACK (changed_bg_color), d);
	gtk_box_pack_start (GTK_BOX (glade_xml_get_widget (xml, "bg_color_hbox")), d->combo_bg_color, FALSE, FALSE, 0);

	d->entry_bg_pixmap = glade_xml_get_widget (xml, "entry_cell_bg_pixmap");
	g_signal_connect (GTK_FILE_CHOOSER_BUTTON (d->entry_bg_pixmap),
			    "selection-changed", G_CALLBACK (changed_bg_pixmap), d);

	d->option_halign = glade_xml_get_widget (xml, "option_cell_halign");
	g_signal_connect (d->option_halign, "changed", G_CALLBACK (changed_halign), d);
	d->option_valign = glade_xml_get_widget (xml, "option_cell_valign");
	g_signal_connect (d->option_valign, "changed", G_CALLBACK (changed_valign), d);

	d->spin_width   = glade_xml_get_widget (xml, "spin_cell_width");
	UPPER_FIX (width);
	g_signal_connect (d->spin_width, "value_changed", G_CALLBACK (changed_width), d);
	d->check_width  = glade_xml_get_widget (xml, "check_cell_width");
	g_signal_connect (d->check_width, "toggled", G_CALLBACK (set_has_width), d);
	d->option_width = glade_xml_get_widget (xml, "option_cell_width");
	gtk_combo_box_set_active (GTK_COMBO_BOX (d->option_width), 0);
	g_signal_connect (d->option_width, "changed", G_CALLBACK (changed_width_percent), d);

	d->check_wrap = glade_xml_get_widget (xml, "check_cell_wrap");
	d->check_header = glade_xml_get_widget (xml, "check_cell_header");
	g_signal_connect (d->check_wrap, "toggled", G_CALLBACK (changed_wrap), d);
	g_signal_connect (d->check_header, "toggled", G_CALLBACK (changed_heading), d);

        g_signal_connect (glade_xml_get_widget (xml, "cell_radio"), "toggled", G_CALLBACK (cell_scope_cell), d);
        g_signal_connect (glade_xml_get_widget (xml, "table_radio"), "toggled", G_CALLBACK (cell_scope_table), d);
        g_signal_connect (glade_xml_get_widget (xml, "row_radio"), "toggled", G_CALLBACK (cell_scope_row), d);
        g_signal_connect (glade_xml_get_widget (xml, "col_radio"), "toggled", G_CALLBACK (cell_scope_column), d);

	d->spin_cspan   = glade_xml_get_widget (xml, "spin_cell_cspan");
	d->spin_rspan   = glade_xml_get_widget (xml, "spin_cell_rspan");
	g_signal_connect (d->spin_cspan, "value_changed", G_CALLBACK (changed_cspan), d);
	g_signal_connect (d->spin_rspan, "value_changed", G_CALLBACK (changed_rspan), d);

	gtk_widget_show_all (cell_page);
	gtk_file_chooser_set_preview_widget_active (GTK_FILE_CHOOSER (d->entry_bg_pixmap), FALSE);

	return cell_page;
}

static void
set_ui (GtkHTMLEditCellProperties *d)
{
	if (!editor_has_html_object (d->cd, HTML_OBJECT (d->table)))
		return;

	d->disable_change = TRUE;

	if (d->cell->have_bg)
		gi_color_combo_set_color (GI_COLOR_COMBO (d->combo_bg_color), &d->cell->bg);

	if (d->cell->have_bgPixmap) {
		char *filename = gtk_html_filename_from_uri (d->cell->bgPixmap->url);

		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (d->entry_bg_pixmap),
				    filename);
		g_free (filename);
	}

	if (HTML_CLUE (d->cell)->halign == HTML_HALIGN_NONE)
		gtk_combo_box_set_active (
			GTK_COMBO_BOX (d->option_halign),
			HTML_HALIGN_LEFT);
	else
		gtk_combo_box_set_active (
			GTK_COMBO_BOX (d->option_halign),
			HTML_CLUE (d->cell)->halign - HTML_HALIGN_LEFT);
	gtk_combo_box_set_active (
		GTK_COMBO_BOX (d->option_valign),
		HTML_CLUE (d->cell)->valign - HTML_VALIGN_TOP);

	if (d->cell->percent_width) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (d->check_width), TRUE);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_width), d->cell->fixed_width);
		gtk_combo_box_set_active (GTK_COMBO_BOX (d->option_width), 1);
	} else if (d->cell->fixed_width) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (d->check_width), TRUE);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_width), d->cell->fixed_width);
		gtk_combo_box_set_active (GTK_COMBO_BOX (d->option_width), 0);
	} else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (d->check_width), FALSE);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (d->check_wrap), !d->cell->no_wrap);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (d->check_header), d->cell->heading);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_cspan),  d->cell->cspan);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_rspan),  d->cell->rspan);

	d->disable_change = FALSE;
}

GtkWidget *
cell_properties (GtkHTMLControlData *cd, gpointer *set_data)
{
	GtkHTMLEditCellProperties *data = data_new (cd, html_engine_get_table_cell (cd->html->engine));
	GtkWidget *rv;

	*set_data = data;
	rv        = cell_widget (data);
	set_ui (data);

	return rv;
}

void
cell_close_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	g_free (get_data);
}
