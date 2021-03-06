/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 2000,2001,2002 Ximian, Inc.
    Authors:  Radek Doulik (rodo@ximian.com)

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
#include "htmlcolor.h"
#include "htmlcolorset.h"
#include "htmlengine-edit.h"
#include "htmlengine-edit-cut-and-paste.h"
#include "htmlengine-edit-fontstyle.h"
#include "htmlengine-save.h"
#include "htmlselection.h"
#include "htmlsettings.h"
#include "htmltext.h"
#include "gtkhtml-private.h"

#include "text.h"
#include "properties.h"
#include "utils.h"

struct _GtkHTMLEditTextProperties
{

	GtkHTMLControlData *cd;

	GtkWidget *combo_color;
	GtkWidget *option_size;
	GtkWidget *check_bold;
	GtkWidget *check_italic;
	GtkWidget *check_underline;
	GtkWidget *check_strikeout;

	gboolean disable_change;
};
typedef struct _GtkHTMLEditTextProperties GtkHTMLEditTextProperties;

static void
color_changed (GtkWidget *w, GdkColor *color, gboolean custom, gboolean by_user, gboolean is_default,
	       GtkHTMLEditTextProperties *d)
{
	HTMLColor *html_color;

	if (d->disable_change)
		return;

	html_color = html_color_new_from_gdk_color (color);
	gtk_html_set_color (d->cd->html, html_color);
	html_color_unref (html_color);
}

static void
set_style (GtkHTMLFontStyle mask, GtkHTMLFontStyle style, GtkHTMLEditTextProperties *d)
{
	if (d->disable_change)
		return;

	gtk_html_set_font_style (d->cd->html, mask, style);
}

static void
size_changed (GtkComboBox *combo_box, GtkHTMLEditTextProperties *d)
{
	GtkHTMLFontStyle style = GTK_HTML_FONT_STYLE_SIZE_1;

	style += gtk_combo_box_get_active (combo_box);

	set_style (~GTK_HTML_FONT_STYLE_SIZE_MASK, style, d);
}

static void
bold_changed (GtkWidget *w, GtkHTMLEditTextProperties *d)
{
	set_style (~GTK_HTML_FONT_STYLE_BOLD, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)) ? GTK_HTML_FONT_STYLE_BOLD : 0, d);
}

static void
italic_changed (GtkWidget *w, GtkHTMLEditTextProperties *d)
{
	set_style (~GTK_HTML_FONT_STYLE_ITALIC, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)) ? GTK_HTML_FONT_STYLE_ITALIC : 0, d);
}

static void
underline_changed (GtkWidget *w, GtkHTMLEditTextProperties *d)
{
	set_style (~GTK_HTML_FONT_STYLE_UNDERLINE, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)) ? GTK_HTML_FONT_STYLE_UNDERLINE : 0, d);
}

static void
strikeout_changed (GtkWidget *w, GtkHTMLEditTextProperties *d)
{
	set_style (~GTK_HTML_FONT_STYLE_STRIKEOUT, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)) ? GTK_HTML_FONT_STYLE_STRIKEOUT : 0, d);
}

static gint
get_size (GtkHTMLFontStyle s)
{
	return (s & GTK_HTML_FONT_STYLE_SIZE_MASK)
		? (s & GTK_HTML_FONT_STYLE_SIZE_MASK) - GTK_HTML_FONT_STYLE_SIZE_1
		: 2;
}

static void
set_ui (GtkHTMLEditTextProperties *d)
{
	HTMLEngine *e = d->cd->html->engine;
	HTMLColor *color = html_engine_get_color (e);

	d->disable_change = TRUE;

	if (color)
		gi_color_combo_set_color (GI_COLOR_COMBO (d->combo_color), &color->color);
	else
		gi_color_combo_set_color (GI_COLOR_COMBO (d->combo_color), NULL);

	gtk_combo_box_set_active (
		GTK_COMBO_BOX (d->option_size),
		get_size (html_engine_get_font_style (e)));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (d->check_bold), (html_engine_get_font_style (e) & GTK_HTML_FONT_STYLE_BOLD) != 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (d->check_italic), (html_engine_get_font_style (e) & GTK_HTML_FONT_STYLE_ITALIC) != 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (d->check_underline), (html_engine_get_font_style (e) & GTK_HTML_FONT_STYLE_UNDERLINE) != 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (d->check_strikeout), (html_engine_get_font_style (e) & GTK_HTML_FONT_STYLE_STRIKEOUT) != 0);

	d->disable_change = FALSE;
}

GtkWidget *
text_properties (GtkHTMLControlData *cd, gpointer *set_data)
{
	GtkHTMLEditTextProperties *d = g_new (GtkHTMLEditTextProperties, 1);
	GtkWidget *text_page;
	GladeXML *xml;
	gchar *filename;

	d->cd = cd;
	*set_data = d;
	filename = g_build_filename (GLADE_DATADIR, "gtkhtml-editor-properties.glade", NULL);
	xml = glade_xml_new (filename, "text_page", GETTEXT_PACKAGE);
	g_free (filename);
	if (!xml)
		g_error (_("Could not load glade file."));

	text_page = glade_xml_get_widget (xml, "text_page");
	d->combo_color = gi_color_combo_new (NULL, _("Automatic"), &html_colorset_get_color (cd->html->engine->defaultSettings->color_set, HTMLTextColor)->color,
					     color_group_fetch ("text_color", d->cd));
        gi_color_combo_box_set_preview_relief (GI_COLOR_COMBO (d->combo_color), GTK_RELIEF_NORMAL);
        g_signal_connect (d->combo_color, "color_changed", G_CALLBACK (color_changed), d);
	gtk_box_pack_start (GTK_BOX (glade_xml_get_widget (xml, "text_color_hbox")), d->combo_color, FALSE, FALSE, 0);

	d->check_bold = glade_xml_get_widget (xml, "check_bold");
	g_signal_connect (d->check_bold, "toggled", G_CALLBACK (bold_changed), d);

	d->check_italic = glade_xml_get_widget (xml, "check_italic");
	g_signal_connect (d->check_italic, "toggled", G_CALLBACK (italic_changed), d);

	d->check_underline = glade_xml_get_widget (xml, "check_underline");
	g_signal_connect (d->check_underline, "toggled", G_CALLBACK (underline_changed), d);

	d->check_strikeout = glade_xml_get_widget (xml, "check_strikeout");
	g_signal_connect (d->check_strikeout, "toggled", G_CALLBACK (strikeout_changed), d);

	d->option_size = glade_xml_get_widget (xml, "option_size");
	g_signal_connect (d->option_size, "changed", G_CALLBACK (size_changed), d);

	gtk_widget_show_all (text_page);

	set_ui (d);

	return text_page;
}

void
text_close_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	GtkHTMLEditTextProperties *data = (GtkHTMLEditTextProperties *) get_data;

	g_free (data);
}
