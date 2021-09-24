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

    Author: Ettore Perazzoli <ettore@helixcode.com>
*/

#ifndef _TOOLBAR_H
#define _TOOLBAR_H

#include <bonobo.h>

#include "gtkhtml.h"
#include "control-data.h"

GtkWidget *toolbar_style          (GtkHTMLControlData *cd);
void       toolbar_update_format  (GtkHTMLControlData *cd);
void       toolbar_apply_color    (GtkHTMLControlData *cd);
#endif /* _TOOLBAR_H */
