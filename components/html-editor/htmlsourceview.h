/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library

   Copyright (C) 2002 Ximian Inc.
   Author:           Larry Ewing <lewing@ximian.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHcANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#ifndef __HTML_SOURCE_VIEW__
#define __HTML_SOURCE_VIEW__

#include <gtk/gtk.h>
#include <bonobo.h>
#include "gtkhtml.h"

#define HTML_TYPE_SOURCE_VIEW                    (html_source_view_get_type ())
#define HTML_SOURCE_VIEW(w)                      (G_TYPE_CHECK_INSTANCE_CAST ((w), HTML_TYPE_SOURCE_VIEW, HTMLSourceView))
#define HTML_SOURCE_VIEW_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), HTML_TYPE_SOURCE_VIEW, HTMLSourceViewClass))
#define HTML_IS_SOURCE_VIEW(w)                   (G_TYPE_CHECK_INSTANCE_TYPE ((w), HTML_TYPE_SOURCE_VIEW))

typedef struct _HTMLSourceViewPrivate HTMLSourceViewPrivate;
typedef struct _HTMLSourceViewClass HTMLSourceViewClass;
typedef struct _HTMLSourceView HTMLSourceView;

struct _HTMLSourceView {
	GtkVBox object;

	HTMLSourceViewPrivate *priv;
};

struct _HTMLSourceViewClass {
	GtkVBoxClass parent_class;

	void (*update)(HTMLSourceView *);
};

GType        html_source_view_get_type        (void);
GtkWidget *    html_source_view_new             (void);

void           html_source_view_set_timeout     (HTMLSourceView *view, guint timeout);
void           html_source_view_set_mode     (HTMLSourceView *view, gboolean as_html);
void           html_source_view_set_source      (HTMLSourceView *view, BonoboWidget *control, char *content_type);

#endif /* __HTML_SOURCE_VUEW__ */



