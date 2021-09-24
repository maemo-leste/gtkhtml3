/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 2000 Helix Code, Inc. 2001, 2002 Ximian Inc.

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

    Author: Ettore Perazzoli <ettore@helixcode.com>
            Radek Doulik <rodo@ximian.com>

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <bonobo/bonobo-shlib-factory.h>
#ifdef GNOME_GTKHTML_EDITOR_SHLIB
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif
#include "gtkhtml-private.h"

#include "editor-control-factory.h"

static void
editor_shlib_init (void)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		initialized = TRUE;

		/* Initialize the i18n support */
		bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
		bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	}
}

static BonoboObject *
editor_control_shlib_factory (BonoboGenericFactory *factory, const gchar *component_id, gpointer closure)
{
	editor_shlib_init ();

	return editor_control_factory (factory, component_id, closure);
}

BONOBO_ACTIVATION_SHLIB_FACTORY (CONTROL_FACTORY_ID, "GNOME HTML Editor factory", editor_control_shlib_factory, NULL);
