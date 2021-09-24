/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gtkhtml-combo-box.c
 *
 * Copyright (C) 2008 Novell, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "gtkhtml-combo-box.h"

#include <glib/gi18n-lib.h>

#define GTKHTML_COMBO_BOX_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE \
	((obj), GTKHTML_TYPE_COMBO_BOX, GtkhtmlComboBoxPrivate))

enum {
	COLUMN_ACTION,
	COLUMN_SORT
};

enum {
	PROP_0,
	PROP_ACTION
};

struct _GtkhtmlComboBoxPrivate {
	GtkRadioAction *action;
	GtkActionGroup *action_group;
	GHashTable *index;
	guint changed_handler_id;		/* action::changed */
	guint group_sensitive_handler_id;	/* action-group::sensitive */
	guint group_visible_handler_id;		/* action-group::visible */
};

static gpointer parent_class;

static void
combo_box_action_changed_cb (GtkRadioAction *action,
                             GtkRadioAction *current,
                             GtkhtmlComboBox *combo_box)
{
	GtkTreeRowReference *reference;
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	gboolean valid;

	reference = g_hash_table_lookup (
		combo_box->priv->index, GINT_TO_POINTER (
		gtk_radio_action_get_current_value (current)));
	g_return_if_fail (reference != NULL);

	model = gtk_tree_row_reference_get_model (reference);
	path = gtk_tree_row_reference_get_path (reference);
	valid = gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_path_free (path);
	g_return_if_fail (valid);

	gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo_box), &iter);
}

static void
combo_box_action_group_notify_cb (GtkActionGroup *action_group,
                                  GParamSpec *pspec,
                                  GtkhtmlComboBox *combo_box)
{
	g_object_set (
		combo_box, "sensitive",
		gtk_action_group_get_sensitive (action_group), "visible",
		gtk_action_group_get_visible (action_group), NULL);
}

static void
combo_box_render_pixbuf (GtkCellLayout *layout,
                         GtkCellRenderer *renderer,
                         GtkTreeModel *model,
                         GtkTreeIter *iter,
                         GtkhtmlComboBox *combo_box)
{
	GtkRadioAction *action;
	gchar *icon_name;
	gchar *stock_id;
	gboolean sensitive;
	gboolean visible;

	gtk_tree_model_get (model, iter, COLUMN_ACTION, &action, -1);

	g_object_get (
		G_OBJECT (action),
		"icon-name", &icon_name,
		"sensitive", &sensitive,
		"stock-id", &stock_id,
		"visible", &visible,
		NULL);

	g_object_set (
		G_OBJECT (renderer),
		"icon-name", icon_name,
		"sensitive", sensitive,
		"stock-id", stock_id,
		"stock-size", GTK_ICON_SIZE_MENU,
		"visible", visible,
		NULL);

	g_free (icon_name);
	g_free (stock_id);
}

static void
combo_box_render_text (GtkCellLayout *layout,
                       GtkCellRenderer *renderer,
                       GtkTreeModel *model,
                       GtkTreeIter *iter,
                       GtkhtmlComboBox *combo_box)
{
	GtkRadioAction *action;
	gchar **strv;
	gchar *label;
	gboolean sensitive;
	gboolean visible;

	gtk_tree_model_get (model, iter, COLUMN_ACTION, &action, -1);

	g_object_get (
		G_OBJECT (action),
		"label", &label,
		"sensitive", &sensitive,
		"visible", &visible,
		NULL);

	/* Strip out underscores. */
	strv = g_strsplit (label, "_", -1);
	g_free (label);
	label = g_strjoinv (NULL, strv);
	g_strfreev (strv);

	g_object_set (
		G_OBJECT (renderer),
		"sensitive", sensitive,
		"text", label,
		"visible", visible,
		NULL);

	g_free (label);
}

static void
combo_box_update_model (GtkhtmlComboBox *combo_box)
{
	GtkListStore *list_store;
	GSList *list;

	g_hash_table_remove_all (combo_box->priv->index);

	if (combo_box->priv->action == NULL) {
		gtk_combo_box_set_model (GTK_COMBO_BOX (combo_box), NULL);
		return;
	}

	list_store = gtk_list_store_new (
		2, GTK_TYPE_RADIO_ACTION, G_TYPE_INT);

	list = gtk_radio_action_get_group (combo_box->priv->action);

	while (list != NULL) {
		GtkTreeRowReference *reference;
		GtkRadioAction *action = list->data;
		GtkTreePath *path;
		GtkTreeIter iter;
		gint value;

		gtk_list_store_append (list_store, &iter);
		g_object_get (G_OBJECT (action), "value", &value, NULL);
		gtk_list_store_set (
			list_store, &iter, COLUMN_ACTION,
			list->data, COLUMN_SORT, value, -1);

		path = gtk_tree_model_get_path (
			GTK_TREE_MODEL (list_store), &iter);
		reference = gtk_tree_row_reference_new (
			GTK_TREE_MODEL (list_store), path);
		g_hash_table_insert (
			combo_box->priv->index,
			GINT_TO_POINTER (value), reference);
		gtk_tree_path_free (path);

		list = g_slist_next (list);
	}

	gtk_tree_sortable_set_sort_column_id (
		GTK_TREE_SORTABLE (list_store),
		COLUMN_SORT, GTK_SORT_ASCENDING);
	gtk_combo_box_set_model (
		GTK_COMBO_BOX (combo_box), GTK_TREE_MODEL (list_store));

	combo_box_action_changed_cb (
		combo_box->priv->action,
		combo_box->priv->action,
		combo_box);
}

static void
combo_box_set_property (GObject *object,
                        guint property_id,
                        const GValue *value,
                        GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_ACTION:
			gtkhtml_combo_box_set_action (
				GTKHTML_COMBO_BOX (object),
				g_value_get_object (value));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
combo_box_get_property (GObject *object,
                        guint property_id,
                        GValue *value,
                        GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_ACTION:
			g_value_set_object (
				value, gtkhtml_combo_box_get_action (
				GTKHTML_COMBO_BOX (object)));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
combo_box_dispose (GObject *object)
{
	GtkhtmlComboBoxPrivate *priv = GTKHTML_COMBO_BOX_GET_PRIVATE (object);

	if (priv->action != NULL) {
		g_object_unref (priv->action);
		priv->action = NULL;
	}

	if (priv->action_group != NULL) {
		g_object_unref (priv->action_group);
		priv->action_group = NULL;
	}

	g_hash_table_remove_all (priv->index);

	/* Chain up to parent's dispose() method. */
	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
combo_box_finalize (GObject *object)
{
	GtkhtmlComboBoxPrivate *priv = GTKHTML_COMBO_BOX_GET_PRIVATE (object);

	g_hash_table_destroy (priv->index);

	/* Chain up to parent's finalize() method. */
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
combo_box_changed (GtkComboBox *combo_box)
{
	GtkRadioAction *action;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gint value;

	/* This method is virtual, so no need to chain up. */

	if (!gtk_combo_box_get_active_iter (combo_box, &iter))
		return;

	model = gtk_combo_box_get_model (combo_box);
	gtk_tree_model_get (model, &iter, COLUMN_ACTION, &action, -1);
	g_object_get (G_OBJECT (action), "value", &value, NULL);
	gtk_radio_action_set_current_value (action, value);
}

static void
combo_box_class_init (GtkhtmlComboBoxClass *class)
{
	GObjectClass *object_class;
	GtkComboBoxClass *combo_box_class;

	parent_class = g_type_class_peek_parent (class);
	g_type_class_add_private (class, sizeof (GtkhtmlComboBoxPrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->set_property = combo_box_set_property;
	object_class->get_property = combo_box_get_property;
	object_class->dispose = combo_box_dispose;
	object_class->finalize = combo_box_finalize;

	combo_box_class = GTK_COMBO_BOX_CLASS (class);
	combo_box_class->changed = combo_box_changed;

	g_object_class_install_property (
		object_class,
		PROP_ACTION,
		g_param_spec_object (
			"action",
			_("Action"),
			_("A GtkRadioAction"),
			GTK_TYPE_RADIO_ACTION,
			G_PARAM_READWRITE));
}

static void
combo_box_init (GtkhtmlComboBox *combo_box)
{
	GtkCellRenderer *renderer;

	combo_box->priv = GTKHTML_COMBO_BOX_GET_PRIVATE (combo_box);

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (
		GTK_CELL_LAYOUT (combo_box), renderer, FALSE);
	gtk_cell_layout_set_cell_data_func (
		GTK_CELL_LAYOUT (combo_box), renderer,
		(GtkCellLayoutDataFunc) combo_box_render_pixbuf,
		combo_box, NULL);

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (
		GTK_CELL_LAYOUT (combo_box), renderer, TRUE);
	gtk_cell_layout_set_cell_data_func (
		GTK_CELL_LAYOUT (combo_box), renderer,
		(GtkCellLayoutDataFunc) combo_box_render_text,
		combo_box, NULL);

	combo_box->priv->index = g_hash_table_new_full (
		g_direct_hash, g_direct_equal,
		(GDestroyNotify) NULL,
		(GDestroyNotify) gtk_tree_row_reference_free);
}

GType
gtkhtml_combo_box_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0)) {
		static const GTypeInfo type_info = {
			sizeof (GtkhtmlComboBoxClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) combo_box_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,  /* class_data */
			sizeof (GtkhtmlComboBox),
			0,     /* n_preallocs */
			(GInstanceInitFunc) combo_box_init,
			NULL   /* value_table */
		};

		type = g_type_register_static (
			GTK_TYPE_COMBO_BOX, "GtkhtmlComboBox",
			&type_info, 0);
	}

	return type;
}

GtkWidget *
gtkhtml_combo_box_new (void)
{
	return gtkhtml_combo_box_new_with_action (NULL);
}

GtkWidget *
gtkhtml_combo_box_new_with_action (GtkRadioAction *action)
{
	return g_object_new (GTKHTML_TYPE_COMBO_BOX, "action", action, NULL);
}

GtkRadioAction *
gtkhtml_combo_box_get_action (GtkhtmlComboBox *combo_box)
{
	g_return_val_if_fail (GTKHTML_IS_COMBO_BOX (combo_box), NULL);

	return combo_box->priv->action;
}

void
gtkhtml_combo_box_set_action (GtkhtmlComboBox *combo_box,
                              GtkRadioAction *action)
{
	g_return_if_fail (GTKHTML_IS_COMBO_BOX (combo_box));

	if (action != NULL)
		g_return_if_fail (GTK_IS_RADIO_ACTION (action));

	if (combo_box->priv->action != NULL) {
		g_signal_handler_disconnect (
			combo_box->priv->action,
			combo_box->priv->changed_handler_id);
		g_object_unref (combo_box->priv->action);
	}

	if (combo_box->priv->action_group != NULL) {
		g_signal_handler_disconnect (
			combo_box->priv->action_group,
			combo_box->priv->group_sensitive_handler_id);
		g_signal_handler_disconnect (
			combo_box->priv->action_group,
			combo_box->priv->group_visible_handler_id);
		g_object_unref (combo_box->priv->action_group);
		combo_box->priv->action_group = NULL;
	}

	if (action != NULL)
		g_object_get (
			g_object_ref (action), "action-group",
			&combo_box->priv->action_group, NULL);
	combo_box->priv->action = action;
	combo_box_update_model (combo_box);

	if (combo_box->priv->action != NULL)
		combo_box->priv->changed_handler_id = g_signal_connect (
			combo_box->priv->action, "changed",
			G_CALLBACK (combo_box_action_changed_cb), combo_box);

	if (combo_box->priv->action_group != NULL) {
		combo_box->priv->group_sensitive_handler_id =
			g_signal_connect (
				combo_box->priv->action_group,
				"notify::sensitive",
				G_CALLBACK (combo_box_action_group_notify_cb),
				combo_box);
		combo_box->priv->group_visible_handler_id =
			g_signal_connect (
				combo_box->priv->action_group,
				"notify::visible",
				G_CALLBACK (combo_box_action_group_notify_cb),
				combo_box);
	}
}

gint
gtkhtml_combo_box_get_current_value (GtkhtmlComboBox *combo_box)
{
	g_return_val_if_fail (GTKHTML_IS_COMBO_BOX (combo_box), 0);
	g_return_val_if_fail (combo_box->priv->action != NULL, 0);

	return gtk_radio_action_get_current_value (combo_box->priv->action);
}

void
gtkhtml_combo_box_set_current_value (GtkhtmlComboBox *combo_box,
                                     gint current_value)
{
	g_return_if_fail (GTKHTML_IS_COMBO_BOX (combo_box));
	g_return_if_fail (combo_box->priv->action != NULL);

	gtk_radio_action_set_current_value (
		combo_box->priv->action, current_value);
}
