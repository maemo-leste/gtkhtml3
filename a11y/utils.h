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

#ifndef __GTK_HTML_A11Y_UTILS_H__
#define __GTK_HTML_A11Y_UTILS_H__

#include <atk/atkobject.h>
#include "htmlobject.h"

#define ACCESSIBLE_ID "atk-accessible-object"
#define HTML_OBJECT_ACCESSIBLE(o) ATK_OBJECT (html_object_get_data_nocp (HTML_OBJECT (o), ACCESSIBLE_ID))

AtkObject *html_utils_get_accessible (HTMLObject *o, AtkObject *parent);

#endif
