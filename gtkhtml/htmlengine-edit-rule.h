/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 2000 Helix Code, Inc.

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

#ifndef _HTMLENGINE_EDIT_RULE_H
#define _HTMLENGINE_EDIT_RULE_H

#include "htmltypes.h"
#include "htmlenums.h"

void  html_engine_insert_rule  (HTMLEngine     *e,
				gint            length,
				gint            percent,
				gint            size,
				gboolean        shade,
				HTMLHAlignType  halign);
void  html_rule_set            (HTMLRule       *rule,
				HTMLEngine     *e,
				gint            length,
				gint            percent,
				gint            size,
				gboolean        shade,
				HTMLHAlignType  halign);
void  html_rule_set_length     (HTMLRule       *rule,
				HTMLEngine     *e,
				gint            length,
				gint            percent);
void  html_rule_set_size       (HTMLRule       *rule,
				HTMLEngine     *e,
				gint            size);
void  html_rule_set_shade      (HTMLRule       *rule,
				HTMLEngine     *e,
				gboolean        shade);
void  html_rule_set_align      (HTMLRule       *rule,
				HTMLEngine     *e,
				HTMLHAlignType  halign);

#endif
