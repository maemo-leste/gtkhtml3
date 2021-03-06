/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * widget-color-combo.c - A color selector combo box
 * Copyright 2000, 2001, Ximian, Inc.
 *
 * Authors:
 *   Miguel de Icaza (miguel@kernel.org)
 *   Dom Lachowicz (dominicl@seas.upenn.edu)
 *
 * Reworked and split up into a separate ColorPalette object:
 *   Michael Levy (mlevy@genoscope.cns.fr)
 *
 * And later revised and polished by:
 *   Almer S. Tigelaar (almer@gnome.org)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License, version 2, as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <config.h>

#include <libgnomecanvas/gnome-canvas.h>
#include <libgnomecanvas/gnome-canvas-pixbuf.h>
#include <libgnomecanvas/gnome-canvas-rect-ellipse.h>
#ifdef GNOME_GTKHTML_EDITOR_SHLIB
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif
#include "htmlmarshal.h"
#include "gi-colors.h"
#include "gi-color-combo.h"
#include "gi-utils.h"

enum {
	CHANGED,
	LAST_SIGNAL
};

static guint gi_color_combo_signals [LAST_SIGNAL] = { 0, };

#define PARENT_TYPE GI_COMBO_BOX_TYPE
static GObjectClass *gi_color_combo_parent_class;

#define make_color(CC,COL) (((COL) != NULL) ? (COL) : ((CC) ? ((CC)->default_color) : NULL))

static void
gi_color_combo_set_color_internal (GiColorCombo *cc, GdkColor *color)
{
	GdkColor *new_color;
	GdkColor *outline_color;

	new_color = make_color (cc,color);
	/* If the new and the default are NULL draw an outline */
	outline_color = (new_color) ? new_color : &e_dark_gray;

	gnome_canvas_item_set (cc->preview_color_item,
			       "fill_color_gdk", new_color,
			       "outline_color_gdk", outline_color,
			       NULL);
}

static void
gi_color_combo_class_init (GObjectClass *object_class)
{
	gi_color_combo_parent_class = g_type_class_ref (PARENT_TYPE);

	gi_color_combo_signals [CHANGED] =
		g_signal_new ("color_changed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GiColorComboClass, color_changed),
			      NULL, NULL,
			      html_g_cclosure_marshal_VOID__POINTER_BOOLEAN_BOOLEAN_BOOLEAN,
			      G_TYPE_NONE, 4, G_TYPE_POINTER,
			      G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
}

TMP_GI_MAKE_TYPE (gi_color_combo,
		   "GiColorCombo",
		   GiColorCombo,
		   gi_color_combo_class_init,
		   NULL,
		   PARENT_TYPE)

/*
 * Fires signal "color_changed" with the current color as its param
 */
static void
emit_color_changed (GiColorCombo *cc, GdkColor *color,
		    gboolean is_custom, gboolean by_user, gboolean is_default)
{
  	g_signal_emit (cc,
		       gi_color_combo_signals [CHANGED], 0,
		       color, is_custom, by_user, is_default);
	gi_combo_box_popup_hide (GI_COMBO_BOX (cc));
}

static void
cb_palette_color_changed (ColorPalette *P, GdkColor *color,
		 gboolean custom, gboolean by_user, gboolean is_default,
		 GiColorCombo *cc)
{
	gi_color_combo_set_color_internal (cc, color);
	emit_color_changed (cc, color, custom, by_user, is_default);
}

static void
preview_clicked (GtkWidget *button, GiColorCombo *cc)
{
	gboolean is_default;
	GdkColor *color = color_palette_get_current_color (cc->palette, &is_default);
	emit_color_changed (cc, color, FALSE, TRUE, is_default);
	if (color)
		gdk_color_free (color);
}

static void
cb_cust_color_clicked (GtkWidget *widget, GiColorCombo *cc)
{
	gi_combo_box_popup_hide (GI_COMBO_BOX (cc));
}

/*
 * Creates the color table
 */
static void
color_table_setup (GiColorCombo *cc,
		   char const *no_color_label, ColorGroup *color_group)
{
	g_return_if_fail (cc != NULL);

	/* Tell the palette that we will be changing it's custom colors */
	cc->palette =
		COLOR_PALETTE (color_palette_new (no_color_label,
						  cc->default_color,
						  color_group));

	{
		GtkWidget *color_button = color_palette_get_color_picker (cc->palette);
		g_signal_connect (color_button, "clicked",
				  G_CALLBACK (cb_cust_color_clicked), cc);
	}

	g_signal_connect (cc->palette, "color_changed",
			  G_CALLBACK (cb_palette_color_changed), cc);

	gtk_widget_show_all (GTK_WIDGET (cc->palette));

	return;
}

void
gi_color_combo_box_set_preview_relief (GiColorCombo *cc, GtkReliefStyle relief)
{
	g_return_if_fail (cc != NULL);
	g_return_if_fail (IS_GI_COLOR_COMBO (cc));

	gtk_button_set_relief (GTK_BUTTON (cc->preview_button), relief);
}

/*
 * Where the actual construction goes on
 */
static void
gi_color_combo_construct (GiColorCombo *cc, GdkPixbuf *icon,
		       char const *no_color_label,
		       ColorGroup *color_group)
{
	GdkColor *color;
	g_return_if_fail (cc != NULL);
	g_return_if_fail (IS_GI_COLOR_COMBO (cc));

	/*
	 * Our button with the canvas preview
	 */
	cc->preview_button = gtk_button_new ();
	atk_object_set_name (gtk_widget_get_accessible (cc->preview_button), _("color preview"));
	gtk_button_set_relief (GTK_BUTTON (cc->preview_button), GTK_RELIEF_NONE);
	g_object_set (G_OBJECT (cc->preview_button), "focus-on-click", FALSE, NULL);

	cc->preview_canvas = GNOME_CANVAS (gnome_canvas_new ());

	gnome_canvas_set_scroll_region (cc->preview_canvas, 0, 0, 24, 24);
	if (icon) {
		gnome_canvas_item_new (
			GNOME_CANVAS_GROUP (gnome_canvas_root (cc->preview_canvas)),
			GNOME_TYPE_CANVAS_PIXBUF,
			"pixbuf", icon,
			"x",      0.0,
			"y",      0.0,
			"anchor", GTK_ANCHOR_NW,
			NULL);
		g_object_unref (icon);

		cc->preview_color_item = gnome_canvas_item_new (
			GNOME_CANVAS_GROUP (gnome_canvas_root (cc->preview_canvas)),
			gnome_canvas_rect_get_type (),
			"x1",         3.0,
			"y1",         19.0,
			"x2",         20.0,
			"y2",         22.0,
			"fill_color", "black",
			"width_pixels", 1,
			NULL);
	} else
		cc->preview_color_item = gnome_canvas_item_new (
			GNOME_CANVAS_GROUP (gnome_canvas_root (cc->preview_canvas)),
			gnome_canvas_rect_get_type (),
			"x1",         2.0,
			"y1",         1.0,
			"x2",         21.0,
			"y2",         22.0,
			"fill_color", "black",
			"width_pixels", 1,
			NULL);

	gtk_container_add (GTK_CONTAINER (cc->preview_button), GTK_WIDGET (cc->preview_canvas));
	gtk_widget_set_size_request (GTK_WIDGET (cc->preview_canvas), 24, 22);
	g_signal_connect (cc->preview_button, "clicked",
			  G_CALLBACK (preview_clicked), cc);

	color_table_setup (cc, no_color_label, color_group);

	gtk_widget_show_all (cc->preview_button);

	gi_combo_box_construct (GI_COMBO_BOX (cc),
				 cc->preview_button,
				 GTK_WIDGET (cc->palette));

	color = color_palette_get_current_color (cc->palette, NULL);
	gi_color_combo_set_color_internal (cc, color);
	if (color) gdk_color_free (color);
}

/* gi_color_combo_get_color:
 *
 * Return current color, result must be freed with gdk_color_free !
 */
GdkColor *
gi_color_combo_get_color (GiColorCombo *cc, gboolean *is_default)
{
	return color_palette_get_current_color (cc->palette, is_default);
}

/**
 * gi_color_combo_set_color
 * @cc     The combo
 * @color  The color
 *
 * Set the color of the combo to the given color. Causes the color_changed
 * signal to be emitted.
 */
void
gi_color_combo_set_color (GiColorCombo *cc, GdkColor *color)
{
	/* This will change the color on the palette than it will invoke
	 * cb_palette_color_changed which will call emit_color_changed and
	 * set_color_internal which will change the color on our preview and
	 * will let the users of the combo know that the current color has
	 * changed
	 */
	if (color != NULL)
		gdk_rgb_find_color (gtk_widget_get_colormap (GTK_WIDGET (cc)), color);
	color_palette_set_current_color (cc->palette, color);
}

/**
 * gi_color_combo_set_color_to_default
 * @cc  The combo
 *
 * Set the color of the combo to the default color. Causes the color_changed
 * signal to be emitted.
 */
void
gi_color_combo_set_color_to_default (GiColorCombo *cc)
{
	color_palette_set_color_to_default (cc->palette);
}

/**
 * gi_color_combo_new :
 * icon : optionally NULL.
 * , const char *no_color_label,
 * Default constructor. Pass an optional icon and an optional label for the
 * no/auto color button.
 */
GtkWidget *
gi_color_combo_new (GdkPixbuf *icon, char const *no_color_label,
		 GdkColor *default_color,
		 ColorGroup *color_group)
{
	GiColorCombo *cc;

	cc = g_object_new (GI_COLOR_COMBO_TYPE, NULL);

        cc->default_color = default_color;

	gi_color_combo_construct (cc, icon, no_color_label, color_group);

	return GTK_WIDGET (cc);
}
