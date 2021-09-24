/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 2000 Helix Code, Inc.
    Authors:           Radek Doulik (rodo@helixcode.com)

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
#include "gi-color-combo.h"
#include "htmlengine-edit.h"
#include "htmlengine-edit-clueflowstyle.h"
#include "htmlimage.h"
#include "htmlcolor.h"
#include "htmlcolorset.h"
#include "htmlsettings.h"
#include "gtkhtml-private.h"

#include "body.h"
#include "properties.h"
#include "utils.h"

struct _GtkHTMLEditBodyProperties {
	GtkHTMLControlData *cd;

	GtkWidget *pixmap_entry;
	GtkWidget *option_template;
	GtkWidget *combo [3];
	GtkWidget *entry_title;
};
typedef struct _GtkHTMLEditBodyProperties GtkHTMLEditBodyProperties;

typedef struct {
	gchar *name;
	gchar *bg_pixmap;
	GdkColor bg_color;
	GdkColor text_color;
	GdkColor link_color;
	gint left_margin;
} BodyTemplate;

static BodyTemplate body_templates[] = {
	{
		N_("None"),
		NULL,
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		10,
	},
	{
		N_("Perforated paper"),
		"paper.png",
		{0, 0xffff, 0xffff, 0xffff},
		{0, 0, 0, 0},
		{0, 0, 0x3380, 0x6680},
		30,
	},
	{
		N_("Blue ink"),
		"texture.png",
		{0, 0xffff, 0xffff, 0xffff},
		{0, 0x1fff, 0x1fff, 0x8fff},
		{0, 0, 0, 0xffff},
		10,
	},
	{
		N_("Paper"),
		"rect.png",
		{0, 0xffff, 0xffff, 0xffff},
		{0, 0, 0, 0},
		{0, 0, 0, 0xffff},
		10,
  	},
	{
		N_("Ribbon"),
		"ribbon.jpg",
		{0, 0xffff, 0xffff, 0xffff},
		{0, 0, 0, 0},
		{0, 0x9900, 0x3300, 0x6600},
		70,
  	},
	{
		N_("Midnight"),
		"midnight-stars.jpg",
		{0, 0, 0, 0},
		{0, 0xffff, 0xffff, 0xffff},
		{0, 0xffff, 0x9900, 0},
		10,
  	},
	{
		N_("Confidential"),
		"confidential-stamp.jpg",
		{0, 0xffff, 0xffff, 0xffff},
		{0, 0, 0, 0},
		{0, 0, 0, 0xffff},
		10,
  	},
	{
		N_("Draft"),
		"draft-stamp.jpg",
		{0, 0xffff, 0xffff, 0xffff},
		{0, 0, 0, 0},
		{0, 0, 0, 0xffff},
		10,
  	},
	{
		N_("Graph paper"),
		"draft-paper.png",
		{0, 0xffff, 0xffff, 0xffff},
		{0, 0, 0, 0x8000},
		{0, 0xe300, 0x2100, 0x2300},
		10,
  	},
};

static void
color_changed (GtkWidget *w, GdkColor *color, gboolean custom, gboolean by_user, gboolean is_default,
	       GtkHTMLEditBodyProperties *data)
{
	gint idx;

	idx = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (w), "type"));
	if (color)
		html_colorset_set_color (data->cd->html->engine->settings->color_set, color, idx);
	else
		html_colorset_set_color (data->cd->html->engine->settings->color_set,
					 &html_colorset_get_color (data->cd->html->engine->defaultSettings->color_set, idx)->color,
					 idx);
	html_object_change_set_down (data->cd->html->engine->clue, HTML_CHANGE_RECALC_PI);
	gtk_widget_queue_draw (GTK_WIDGET (data->cd->html));
}

static void
entry_changed (GtkWidget *w, GtkHTMLEditBodyProperties *data)
{
	HTMLEngine *e = data->cd->html->engine;
	const char *fname;

	/* FIXME: extend the API with bg image query/setting */
	if (e->bgPixmapPtr != NULL) {
		html_image_factory_unregister (e->image_factory, e->bgPixmapPtr, NULL);
		e->bgPixmapPtr = NULL;
	}

	fname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (w));
	if (fname && *fname) {
		gchar *file = gtk_html_filename_to_uri (fname);

		e->bgPixmapPtr = html_image_factory_register (e->image_factory, NULL, file, TRUE);
		g_free (file);
	}
	gtk_widget_queue_draw (GTK_WIDGET (data->cd->html));
}

static void
changed_template (GtkComboBox *combo_box, GtkHTMLEditBodyProperties *d)
{
	gint template, left_margin = 10;
	gchar *filename;

	template = gtk_combo_box_get_active (combo_box);

	filename = (body_templates [template].bg_pixmap ?
		    g_build_filename (ICONDIR, body_templates [template].bg_pixmap, NULL) :
		    g_strdup (""));
	gtk_file_chooser_set_filename (GTK_FILE_CHOOSER ((d->pixmap_entry)),
			    filename);
	g_free (filename);

	if (template) {
		gi_color_combo_set_color (GI_COLOR_COMBO (d->combo [2]), &body_templates [template].bg_color);
		gi_color_combo_set_color (GI_COLOR_COMBO (d->combo [0]), &body_templates [template].text_color);
		gi_color_combo_set_color (GI_COLOR_COMBO (d->combo [1]), &body_templates [template].link_color);
		left_margin = body_templates [template].left_margin;
	} else {
		gi_color_combo_set_color (GI_COLOR_COMBO (d->combo [2]),
				       &html_colorset_get_color_allocated (d->cd->html->engine->settings->color_set,
									   d->cd->html->engine->painter,
									   HTMLBgColor)->color);
		gi_color_combo_set_color (GI_COLOR_COMBO (d->combo [0]),
				       &html_colorset_get_color_allocated (d->cd->html->engine->settings->color_set,
									   d->cd->html->engine->painter,
									   HTMLTextColor)->color);
		gi_color_combo_set_color (GI_COLOR_COMBO (d->combo [1]),
				       &html_colorset_get_color_allocated (d->cd->html->engine->settings->color_set,
									   d->cd->html->engine->painter,
									   HTMLLinkColor)->color);
	}

	/* FIXME: add API for margins query/setting to libgtkhtml */
	d->cd->html->engine->leftBorder = left_margin;
}

GtkWidget *
body_properties (GtkHTMLControlData *cd, gpointer *set_data)
{
	GtkHTMLEditBodyProperties *data = g_new0 (GtkHTMLEditBodyProperties, 1);
	GtkWidget *main_vbox, *hbox, *combo, *label, *t1;
	HTMLColor *color;
	gint i;

	*set_data = data;
	data->cd = cd;

	main_vbox = gtk_vbox_new (FALSE, 12);
	gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);

	t1 = gtk_table_new (3, 2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (t1), 6);
	gtk_table_set_row_spacings (GTK_TABLE (t1), 6);

	i = 0;
#define ADD_COLOR(x, ct, g) \
        color = html_colorset_get_color (cd->html->engine->settings->color_set, ct); \
        html_color_alloc (color, cd->html->engine->painter); \
	data->combo [i] = combo = gi_color_combo_new (NULL, _("Automatic"), \
				 &color->color, \
				 color_group_fetch ("body_" g, cd)); \
        gi_color_combo_box_set_preview_relief (GI_COLOR_COMBO (data->combo [i]), GTK_RELIEF_NORMAL); \
        g_object_set_data (G_OBJECT (combo), "type", GINT_TO_POINTER (ct)); \
	hbox = gtk_hbox_new (FALSE, 3); \
        label = gtk_label_new_with_mnemonic (x); \
        gtk_misc_set_alignment (GTK_MISC (label), .0, .5); \
        atk_object_add_relationship (gtk_widget_get_accessible (GI_COLOR_COMBO(combo)->preview_button), ATK_RELATION_LABELLED_BY, gtk_widget_get_accessible (label)); \
        gtk_table_attach (GTK_TABLE (t1), label, 0, 1, i, i + 1, GTK_FILL, GTK_FILL, 0, 0); \
        gtk_table_attach (GTK_TABLE (t1), combo, 1, 2, i, i + 1, GTK_FILL, GTK_FILL, 0, 0); \
        i ++

	ADD_COLOR (_("_Text:"), HTMLTextColor, "text");
	ADD_COLOR (_("_Link:"), HTMLLinkColor, "link");
	ADD_COLOR (_("_Background:"), HTMLBgColor, "bg");

	gtk_box_pack_start (GTK_BOX (main_vbox), editor_hig_vbox (_("Colors"), t1), FALSE, FALSE, 0);

	data->pixmap_entry = gtk_file_chooser_button_new (_("Background Image"), GTK_FILE_CHOOSER_ACTION_OPEN);
	if (cd->html->engine->bgPixmapPtr) {
		HTMLImagePointer *ip = (HTMLImagePointer *) cd->html->engine->bgPixmapPtr;
		gchar *filename = gtk_html_filename_from_uri (ip->url);

		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER ((data->pixmap_entry)),
				    filename);
		g_free (filename);
	}


	atk_object_set_name (gtk_widget_get_accessible (data->pixmap_entry), _("Background Image File Path"));

	t1 = gtk_table_new (2, 2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (t1), 6);
	gtk_table_set_row_spacings (GTK_TABLE (t1), 6);

	hbox = gtk_hbox_new (FALSE, 6);
	data->option_template = gtk_combo_box_new_text ();
	atk_object_set_name (gtk_widget_get_accessible (data->option_template), _("Template"));
	for (i = 0; i < G_N_ELEMENTS (body_templates); i++)
		gtk_combo_box_append_text (
			GTK_COMBO_BOX (data->option_template),
			_(body_templates[i].name));
	gtk_combo_box_set_active (GTK_COMBO_BOX (data->option_template), 0);
	gtk_box_pack_start (GTK_BOX (hbox), data->option_template, FALSE, FALSE, 0);
	editor_hig_attach_row (t1, _("T_emplate:"), hbox, 0);

	hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox), data->pixmap_entry, TRUE, TRUE, 0);
	editor_hig_attach_row (t1, _("C_ustom:"), hbox, 1);

	gtk_box_pack_start (GTK_BOX (main_vbox), editor_hig_vbox (_("Background Image"), t1), FALSE, FALSE, 0);

	/* set ui */
#define SET_COLOR(ct) \
        gi_color_combo_set_color (GI_COLOR_COMBO (combo), &html_colorset_get_color_allocated (cd->html->engine->settings->color_set, cd->html->engine->painter, ct)->color);

	SET_COLOR (HTMLTextColor);
	SET_COLOR (HTMLLinkColor);
	SET_COLOR (HTMLBgColor);

	/* connect signal handlers */
	gtk_widget_show_all (main_vbox);

	g_signal_connect (data->option_template, "changed", G_CALLBACK (changed_template), data);
        g_signal_connect (data->combo [0], "color_changed", G_CALLBACK (color_changed), data);
        g_signal_connect (data->combo [1], "color_changed", G_CALLBACK (color_changed), data);
        g_signal_connect (data->combo [2], "color_changed", G_CALLBACK (color_changed), data);
	g_signal_connect (GTK_FILE_CHOOSER_BUTTON (data->pixmap_entry),
			  "selection-changed", G_CALLBACK (entry_changed), data);

	return main_vbox;
}

void
body_close_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	g_free (get_data);
}
