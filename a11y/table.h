/*  This file is part of the GtkHTML library.
 *
 *  Copyright 2002 Ximian, Inc.
 *
 *  Author: Radek Doulik
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef __HTML_A11Y_TABLE_H__
#define __HTML_A11Y_TABLE_H__

#include "html.h"

#define G_TYPE_HTML_A11Y_TABLE            (html_a11y_table_get_type ())
#define HTML_A11Y_TABLE(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
									   G_TYPE_HTML_A11Y_TABLE, \
									   HTMLA11YTable))
#define HTML_A11Y_TABLE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), \
									G_TYPE_HTML_A11Y_TABLE, \
									HTMLA11YTableClass))
#define G_IS_HTML_A11Y_TABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_TYPE_HTML_A11Y_TABLE))
#define G_IS_HTML_A11Y_TABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), G_TYPE_HTML_A11Y_TABLE))
#define HTML_A11Y_TABLE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_HTML_A11Y_TABLE, \
									  HTMLA11YTableClass))

typedef struct _HTMLA11YTable      HTMLA11YTable;
typedef struct _HTMLA11YTableClass HTMLA11YTableClass;

struct _HTMLA11YTable {
	HTMLA11Y html_a11y_object;
};

GType html_a11y_table_get_type (void);

struct _HTMLA11YTableClass {
	HTMLA11YClass parent_class;
};

AtkObject* html_a11y_table_new (HTMLObject *o);

#endif
