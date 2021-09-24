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
#include "replace.h"
#include "dialog.h"
#include "htmlengine.h"
#include "gtkhtml-private.h"


struct _GtkHTMLReplaceAskDialog {
	GtkDialog  *dialog;
	HTMLEngine *engine;
	gboolean finished;
};

typedef struct _GtkHTMLReplaceAskDialog GtkHTMLReplaceAskDialog;

struct _GtkHTMLReplaceDialog {
	GtkDialog   *dialog;
	GtkHTML     *html;
	GtkWidget   *entry_search;
	GtkWidget   *entry_replace;
	GtkWidget   *backward;
	GtkWidget   *case_sensitive;

	GtkHTMLControlData *cd;
};

static void
ask_dialog_response (GtkDialog *dialog, gint response_id, GtkHTMLReplaceAskDialog *d)
{
	HTMLReplaceQueryAnswer answer = RQA_Cancel;

	switch (response_id) {
	case 0: /* Replace */
		answer = RQA_Replace;
		break;
	case 1: /* Replace All */
		answer = RQA_ReplaceAll;
		break;
	case 2: /* Next */
		answer = RQA_Next;
		break;
	}

	d->finished = html_engine_replace_do (d->engine, answer);
}

static GtkHTMLReplaceAskDialog *
ask_dialog_new (HTMLEngine *e)
{
	GtkHTMLReplaceAskDialog *d;

	d = g_new (GtkHTMLReplaceAskDialog, 1);
	/* we use CANCEL response for close, because we want Esc to close the dialog - see gtkdialog.c */
	d->dialog = GTK_DIALOG (gtk_dialog_new_with_buttons (_("Replace confirmation"), NULL, 0,
							     _("Replace _All"), 1,
							     _("_Next"), 2,
							     GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL,
							     _("_Replace"), 0,
							     NULL));
	d->engine = e;
	d->finished = FALSE;

	gtk_window_set_icon_name (GTK_WINDOW (d->dialog), "gtk-find-and-replace");

	g_signal_connect (d->dialog, "response", G_CALLBACK (ask_dialog_response), d);

	return d;
}

static void
ask (HTMLEngine *e, gpointer data)
{
	GtkHTMLReplaceAskDialog *ask_dialog;

	ask_dialog = ask_dialog_new (e);
	while (!ask_dialog->finished)
		gtk_dialog_run (ask_dialog->dialog);
	gtk_widget_destroy (GTK_WIDGET (ask_dialog->dialog));
	gtk_widget_grab_focus (GTK_WIDGET (ask_dialog->engine->widget));
	g_free (ask_dialog);
}

static void
entry_search_changed (GtkWidget *entry, GtkHTMLReplaceDialog *d)
{
	if (d->cd->replace_text_search)
		g_free (d->cd->replace_text_search);
	d->cd->replace_text_search = g_strdup (gtk_entry_get_text (GTK_ENTRY (d->entry_search)));
}

static void
entry_replace_changed (GtkWidget *entry, GtkHTMLReplaceDialog *d)
{
	if (d->cd->replace_text_replace)
		g_free (d->cd->replace_text_replace);
	d->cd->replace_text_replace = g_strdup (gtk_entry_get_text (GTK_ENTRY (d->entry_replace)));
}

static void
entry_activate (GtkWidget *entry, GtkHTMLReplaceDialog *d)
{
	gtk_dialog_response (GTK_DIALOG (d->dialog), 0);
}

static void
replace_dialog_response (GtkDialog *dialog, gint response_id, GtkHTMLReplaceDialog *d)
{
	switch (response_id) {
	case 0: /* Replace */
		gtk_widget_hide (GTK_WIDGET (d->dialog));
		html_engine_replace (d->html->engine,
				     gtk_entry_get_text (GTK_ENTRY (d->entry_search)),
				     gtk_entry_get_text (GTK_ENTRY (d->entry_replace)),
				     GTK_TOGGLE_BUTTON (d->case_sensitive)->active,
				     GTK_TOGGLE_BUTTON (d->backward)->active == 0, FALSE,
				     ask, d);
		break;
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CLOSE:
	case GTK_RESPONSE_CANCEL:
		d->cd->replace_dialog = NULL;
		gtk_html_replace_dialog_destroy (d);
		gtk_widget_grab_focus (GTK_WIDGET (d->html));
		break;
	}
}

void
gtk_html_replace_dialog_destroy (GtkHTMLReplaceDialog *d)
{
	gtk_widget_destroy (GTK_WIDGET (d->dialog));
	g_free (d);
}

GtkHTMLReplaceDialog *
gtk_html_replace_dialog_new (GtkHTML *html, GtkHTMLControlData *cd)
{
	GtkHTMLReplaceDialog *dialog = g_new (GtkHTMLReplaceDialog, 1);
	GtkWidget *hbox, *vbox;
	GtkWidget *table;
	GtkWidget *label;

	/* we use CANCEL response for close, because we want Esc to close the dialog - see gtkdialog.c */
	dialog->dialog         = GTK_DIALOG (gtk_dialog_new_with_buttons (_("Replace"), NULL, 0,
									  GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL,
									  GTK_STOCK_FIND_AND_REPLACE, 0,
									  NULL));

	table = gtk_table_new (2, 2, FALSE);
	dialog->entry_search   = gtk_entry_new ();
	dialog->entry_replace  = gtk_entry_new ();
	dialog->backward       = gtk_check_button_new_with_mnemonic (_("Search _backward"));
	dialog->case_sensitive = gtk_check_button_new_with_mnemonic (_("Case _sensitive"));
	dialog->html           = html;
	dialog->cd             = cd;

	if (cd->replace_text_search)
		gtk_entry_set_text (GTK_ENTRY (dialog->entry_search), cd->replace_text_search);
	if (cd->replace_text_replace)
		gtk_entry_set_text (GTK_ENTRY (dialog->entry_replace), cd->replace_text_replace);

	gtk_table_set_col_spacings (GTK_TABLE (table), 3);
	label = gtk_label_new_with_mnemonic (_("R_eplace:"));
	gtk_label_set_mnemonic_widget ((GtkLabel *)label,dialog-> entry_search);
	gtk_misc_set_alignment (GTK_MISC (label), .0, .5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	label = gtk_label_new_with_mnemonic (_("_With:"));
	gtk_label_set_mnemonic_widget ((GtkLabel *)label, dialog->entry_replace);
	gtk_misc_set_alignment (GTK_MISC (label), .0, .5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

	gtk_table_attach_defaults (GTK_TABLE (table), dialog->entry_search,  1, 2, 0, 1);
	gtk_table_attach_defaults (GTK_TABLE (table), dialog->entry_replace, 1, 2, 1, 2);

	hbox = gtk_hbox_new (FALSE, 6);

	gtk_box_pack_start (GTK_BOX (hbox), dialog->backward, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), dialog->case_sensitive, FALSE, FALSE, 0);

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (dialog->dialog), 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
	gtk_container_set_border_width (GTK_CONTAINER (dialog->dialog->vbox), 6);
	gtk_box_set_spacing (GTK_BOX (dialog->dialog->vbox), 6);
	gtk_box_pack_start (GTK_BOX (dialog->dialog->vbox), vbox, FALSE, FALSE, 0);
	gtk_widget_show_all (table);
	gtk_widget_show_all (hbox);

	gtk_window_set_icon_name (GTK_WINDOW (dialog->dialog), "gtk-find-and-replace");

	gtk_widget_grab_focus (dialog->entry_search);

	g_signal_connect (dialog->dialog, "response", G_CALLBACK (replace_dialog_response), dialog);
	g_signal_connect (dialog->entry_search, "changed", G_CALLBACK (entry_search_changed), dialog);
	g_signal_connect (dialog->entry_search, "activate", G_CALLBACK (entry_activate), dialog);
	g_signal_connect (dialog->entry_replace, "changed", G_CALLBACK (entry_replace_changed), dialog);
	g_signal_connect (dialog->entry_replace, "activate", G_CALLBACK (entry_activate), dialog);

	return dialog;
}

void
replace (GtkHTMLControlData *cd)
{
	run_dialog (&cd->replace_dialog->dialog, cd->html, cd,
		(DialogCtor) gtk_html_replace_dialog_new, _("Replace"));
}
