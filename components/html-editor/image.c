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
#include <unistd.h>
#include <string.h>
#include <glade/glade.h>
#include <libgnome/libgnome.h>

#include "gtkhtml.h"
#include "htmlcolorset.h"
#include "htmlcursor.h"
#include "htmlengine.h"
#include "htmlengine-edit-images.h"
#include "htmlengine-save.h"
#include "htmlimage.h"
#include "htmlsettings.h"
#include "gtkhtml-private.h"

#include "image.h"
#include "properties.h"
#include "utils.h"

struct _GtkHTMLEditImageProperties {
	GtkHTMLControlData *cd;

	HTMLImage *image;

	GtkWidget *page;
	GtkWidget *pentry;
	GtkWidget *option_template;
	GtkWidget *spin_width;
	GtkWidget *option_width_percent;
	GtkWidget *spin_height;
	GtkWidget *option_height_percent;
	GtkWidget *spin_padh;
	GtkWidget *spin_padv;
	GtkWidget *spin_border;
	GtkWidget *option_align;
	GtkWidget *entry_url;
	GtkWidget *entry_alt;

	gboolean   disable_change;
};
typedef struct _GtkHTMLEditImageProperties GtkHTMLEditImageProperties;

#define TEMPLATES 3
typedef struct {
	gchar *name;
	gint offset;

	gboolean can_set_align;
	gboolean can_set_border;
	gboolean can_set_padding;
	gboolean can_set_size;

	HTMLVAlignType align;
	gint border;
	gint padh;
	gint padv;
	gint width;
	gboolean width_percent;
	gint height;
	gboolean height_percent;

	gchar *image;
} ImageInsertTemplate;

static GtkHTMLEditImageProperties *
data_new (GtkHTMLControlData *cd, HTMLImage *image)
{
	GtkHTMLEditImageProperties *data = g_new0 (GtkHTMLEditImageProperties, 1);

	/* fill data */
	data->cd             = cd;
	data->disable_change = TRUE;
	data->image          = image;

	return data;
}

static gchar *
get_location (GtkHTMLEditImageProperties *d)
{
	gchar *file;
	gchar *url;

	file = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (d->pentry));
	if (file) {
		url = gtk_html_filename_to_uri (file);
	} else {
		url = gtk_html_filename_to_uri (gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (d->pentry)));
	}

	if (!url)
		url = g_strdup ("");
	g_free (file);

	return url;
}

static void
pentry_changed (GtkWidget *entry, GtkHTMLEditImageProperties *d)
{
	char *location;

	if (d->disable_change || !editor_has_html_object (d->cd, HTML_OBJECT (d->image)))
		return;

	location = get_location (d);
	html_image_edit_set_url (d->image, location);
	g_free (location);

}

static void
url_changed (GtkWidget *entry, GtkHTMLEditImageProperties *d)
{
	char *url, *target;

	if (d->disable_change || !editor_has_html_object (d->cd, HTML_OBJECT (d->image)))
		return;

	url = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
	target = NULL;

	if (url) {
		target = strchr (url, '#');
		if (target) {
			*target = 0;
			target ++;
		}
	}

	html_object_set_link (HTML_OBJECT (d->image),
			      url && *url
			      ? html_colorset_get_color (d->cd->html->engine->settings->color_set, HTMLLinkColor)
			      : html_colorset_get_color (d->cd->html->engine->settings->color_set, HTMLTextColor),
			      url, target);
	g_free (url);
}

static void
alt_changed (GtkWidget *entry, GtkHTMLEditImageProperties *d)
{
	if (!d->disable_change && editor_has_html_object (d->cd, HTML_OBJECT (d->image)))
		html_image_set_alt (d->image, (char *) gtk_entry_get_text (GTK_ENTRY (entry)));
}

static void
changed_align (GtkWidget *w, GtkHTMLEditImageProperties *d)
{
	if (!d->disable_change && editor_has_html_object (d->cd, HTML_OBJECT (d->image)))
		html_image_set_valign (d->image, gtk_combo_box_get_active (GTK_COMBO_BOX (d->option_align)));
}

static void
changed_size (GtkWidget *widget, GtkHTMLEditImageProperties *d)
{
	gint width, height, width_percent, height_percent;

	if (d->disable_change || !editor_has_html_object (d->cd, HTML_OBJECT (d->image)))
		return;

	width = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (d->spin_width));
	height = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (d->spin_height));
	width_percent = gtk_combo_box_get_active (GTK_COMBO_BOX (d->option_width_percent));
	height_percent = gtk_combo_box_get_active (GTK_COMBO_BOX (d->option_height_percent));
	gtk_widget_set_sensitive (d->spin_width, width_percent != 2);
	gtk_widget_set_sensitive (d->spin_height, height_percent != 2);

	html_image_set_size (d->image,
			     width_percent == 2 ? 0 : width,
			     height_percent == 2 ? 0 : height,
			     width_percent == 1, height_percent == 1);
}

static void
test_url_clicked (GtkWidget *w, GtkHTMLEditImageProperties *d)
{
	const char *url = gtk_entry_get_text (GTK_ENTRY (d->entry_url));

	if (url)
		gnome_url_show (url, NULL);
}

static void
changed_border (GtkWidget *check, GtkHTMLEditImageProperties *d)
{
	if (!d->disable_change && editor_has_html_object (d->cd, HTML_OBJECT (d->image)))
		html_image_set_border (d->image, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (d->spin_border)));
}

static void
changed_padding (GtkWidget *check, GtkHTMLEditImageProperties *d)
{
	if (!d->disable_change && editor_has_html_object (d->cd, HTML_OBJECT (d->image)))
		html_image_set_spacing  (d->image,
					 gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (d->spin_padh)),
					 gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (d->spin_padv)));
}

static void
image_set_ui (GtkHTMLEditImageProperties *d)
{
	HTMLImage *image = d->image;

	if (image) {
		HTMLImagePointer *ip = image->image_ptr;

		d->disable_change = TRUE;

		if (image->percent_width) {
			gtk_combo_box_set_active (GTK_COMBO_BOX (d->option_width_percent), 1);
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_width), image->specified_width);
		} else if (image->specified_width > 0) {
			gtk_combo_box_set_active (GTK_COMBO_BOX (d->option_width_percent), 0);
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_width), image->specified_width);
		} else {
			gtk_combo_box_set_active (GTK_COMBO_BOX (d->option_width_percent), 2);
			gtk_widget_set_sensitive (d->spin_width, FALSE);
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_width), html_image_get_actual_width (image, NULL));
		}

		if (image->percent_height) {
			gtk_combo_box_set_active (GTK_COMBO_BOX (d->option_height_percent), 1);
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_height), image->specified_height);
		} else if (image->specified_height > 0) {
			gtk_combo_box_set_active (GTK_COMBO_BOX (d->option_height_percent), 0);
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_height), image->specified_height);
		} else {
			gtk_combo_box_set_active (GTK_COMBO_BOX (d->option_height_percent), 2);
			gtk_widget_set_sensitive (d->spin_height, FALSE);
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_height), html_image_get_actual_height (image, NULL));
		}

		gtk_combo_box_set_active (GTK_COMBO_BOX (d->option_align), image->valign);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_padh), image->hspace);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_padv), image->vspace);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_border), image->border);

		if (image->url) {
			char *url;
			url = g_strconcat (image->url, image->target ? "#" : NULL, image->target, NULL);
			gtk_entry_set_text (GTK_ENTRY (d->entry_url), url);
			g_free (url);
		}
		if (image->alt)
			gtk_entry_set_text (GTK_ENTRY (d->entry_alt), image->alt);

		if (!HTML_OBJECT (image)->parent || !html_object_get_data (HTML_OBJECT (image)->parent, "template_image")) {

			if (ip->url) {
				gchar *filename = gtk_html_filename_from_uri (ip->url);

				gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (d->pentry), filename);
				g_free (filename);
			}
		}

		d->disable_change = FALSE;
	}
}

static void
set_size_all (HTMLObject *o, HTMLEngine *e, GtkHTMLEditImageProperties *d)
{
	gchar *location = get_location (d);
	printf ("all: %s\n", location);
	if (location && HTML_IS_IMAGE (o) && HTML_IMAGE (o)->image_ptr && HTML_IMAGE (o)->image_ptr->url) {
		if (!strcmp (HTML_IMAGE (o)->image_ptr->url, location)) {
			HTMLImage *i = HTML_IMAGE (o);
			gint width, height, width_percent, height_percent;

			width = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (d->spin_width));
			height = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (d->spin_height));
			width_percent = gtk_combo_box_get_active (GTK_COMBO_BOX (d->option_width_percent));
			height_percent = gtk_combo_box_get_active (GTK_COMBO_BOX (d->option_height_percent));

			d->disable_change = TRUE;
			if ((width == 0 || width_percent == 2) && width_percent != 1) {
				width = html_image_get_actual_width (i, NULL);
				gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_width), width);
			}
			if ((height == 0 || height_percent == 2) && height_percent != 1) {
				height = html_image_get_actual_height (i, NULL);
				gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_height), height);
			}
			d->disable_change = FALSE;
		}
	}
	g_free (location);
}

/* FIXME
   we need image_loaded signal in gtkhtml so that we can update width/height values in the UI when new image is loaded
*/
static gboolean
load_done (GtkHTML *html, GtkHTMLEditImageProperties *d)
{
	printf ("load done\n");
	if (d->cd->html->engine->clue) {
		html_object_forall (d->cd->html->engine->clue, d->cd->html->engine, (HTMLObjectForallFunc) set_size_all, d);
	}
	return FALSE;
}

#define UPPER_FIX(x) gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (d->spin_ ## x))->upper = 100000.0

static GtkWidget *
image_widget (GtkHTMLEditImageProperties *d, gboolean insert)
{
	GladeXML *xml;
	GtkWidget *button;
	gchar *filename;

	filename = g_build_filename (GLADE_DATADIR, "gtkhtml-editor-properties.glade", NULL);
	xml = glade_xml_new (filename, "image_page", GETTEXT_PACKAGE);
	g_free (filename);
	if (!xml)
		g_error (_("Could not load glade file."));

	d->page = glade_xml_get_widget (xml, "image_page");

	d->option_align = glade_xml_get_widget (xml, "option_image_align");
	g_signal_connect (d->option_align, "changed", G_CALLBACK (changed_align), d);
	d->option_width_percent = glade_xml_get_widget (xml, "option_image_width_percent");
	g_signal_connect (d->option_width_percent, "changed", G_CALLBACK (changed_size), d);
	d->option_height_percent = glade_xml_get_widget (xml, "option_image_height_percent");
	g_signal_connect (d->option_height_percent, "changed", G_CALLBACK (changed_size), d);

	d->spin_border = glade_xml_get_widget (xml, "spin_image_border");
	UPPER_FIX (border);
	g_signal_connect (d->spin_border, "value_changed", G_CALLBACK (changed_border), d);
	d->spin_width = glade_xml_get_widget (xml, "spin_image_width");
	UPPER_FIX (width);
	g_signal_connect (d->spin_width, "value_changed", G_CALLBACK (changed_size), d);
	d->spin_height = glade_xml_get_widget (xml, "spin_image_height");
	UPPER_FIX (height);
	g_signal_connect (d->spin_height, "value_changed", G_CALLBACK (changed_size), d);
	d->spin_padh = glade_xml_get_widget (xml, "spin_image_padh");
	UPPER_FIX (padh);
	g_signal_connect (d->spin_padh, "value_changed", G_CALLBACK (changed_padding), d);
	d->spin_padv = glade_xml_get_widget (xml, "spin_image_padv");
	UPPER_FIX (padv);
	g_signal_connect (d->spin_padv, "value_changed", G_CALLBACK (changed_padding), d);

	d->entry_url = glade_xml_get_widget (xml, "entry_image_url");
	g_signal_connect (GTK_OBJECT (d->entry_url), "changed", G_CALLBACK (url_changed), d);

	d->entry_alt = glade_xml_get_widget (xml, "entry_image_alt");
	g_signal_connect (d->entry_alt, "changed", G_CALLBACK (alt_changed), d);

	d->pentry = glade_xml_get_widget (xml, "pentry_image_location");
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (d->pentry), g_get_home_dir ());
	g_signal_connect (GTK_OBJECT (GTK_FILE_CHOOSER_BUTTON (d->pentry)),
			    "selection-changed", G_CALLBACK (pentry_changed), d);

	gtk_widget_show_all (d->page);
	gtk_file_chooser_set_preview_widget_active (GTK_FILE_CHOOSER (d->pentry), FALSE);

	editor_check_stock ();
	button = gtk_button_new_from_stock (GTKHTML_STOCK_TEST_URL);
	g_signal_connect (button, "clicked", G_CALLBACK (test_url_clicked), d);
	gtk_widget_show (button);
	gtk_table_attach (GTK_TABLE (glade_xml_get_widget (xml, "image_table")), button, 2, 3, 0, 1, 0, 0, 0, 0);

	g_signal_connect (d->cd->html, "load_done", G_CALLBACK (load_done), d);

	return d->page;
}

GtkWidget *
image_properties (GtkHTMLControlData *cd, gpointer *set_data)
{
	GtkWidget *w;
	GtkHTMLEditImageProperties *d;
	HTMLImage *image = HTML_IMAGE (cd->html->engine->cursor->object);

	g_assert (HTML_OBJECT_TYPE (cd->html->engine->cursor->object) == HTML_TYPE_IMAGE);

	*set_data = d = data_new (cd, image);
	w = image_widget (d, FALSE);
	image_set_ui (d);
	gtk_widget_show (w);

	return w;
}

void
image_close_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	GtkHTMLEditImageProperties *d = (GtkHTMLEditImageProperties *) get_data;

	g_free (d);
}
