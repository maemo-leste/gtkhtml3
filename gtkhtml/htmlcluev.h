/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the KDE libraries
    Copyright (C) 1997 Martin Jones (mjones@kde.org)
              (C) 1997 Torben Weis (weis@kde.org)

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
#ifndef _HTMLCLUEV_H_
#define _HTMLCLUEV_H_

#include "htmltypes.h"
#include "htmlclue.h"

#define HTML_CLUEV(x) ((HTMLClueV *)(x))
#define HTML_CLUEV_CLASS(x) ((HTMLClueVClass *)(x))
#define HTML_IS_CLUEV(x) (HTML_CHECK_TYPE ((x), HTML_TYPE_CLUEV))

struct _HTMLClueV {
	HTMLClue clue;

	/* FIXME: htmlcluealigned */
	HTMLObject *align_left_list;
	HTMLObject *align_right_list;
	gushort padding;

	/* border */
	int border_width;
	HTMLBorderStyle border_style;
	HTMLColor *border_color;

	HTMLColor *background_color;

	HTMLDirection dir;
};

struct _HTMLClueVClass {
	HTMLClueClass clue_class;
};


extern HTMLClueVClass html_cluev_class;


void        html_cluev_type_init   (void);
void        html_cluev_class_init  (HTMLClueVClass *klass,
				    HTMLType        type,
				    guint           object_size);
void        html_cluev_init        (HTMLClueV      *cluev,
				    HTMLClueVClass *klass,
				    gint            x,
				    gint            y,
				    gint            percent);
HTMLObject *html_cluev_new         (gint            x,
				    gint            y,
				    gint            percent);

void        html_cluev_set_style   (HTMLClueV      *cluev,
				    HTMLStyle      *style);

#endif /* _HTMLCLUEV_H_ */
