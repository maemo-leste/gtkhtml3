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
#ifndef _HTML_CLUEALIGNED_H_
#define _HTML_CLUEALIGNED_H_

#include "htmlclue.h"

#define HTML_CLUEALIGNED(x)       ((HTMLClueAligned *)(x))
#define HTML_CLUEALIGNED_CLASS(x) ((HTMLClueAlignedClass *)(x))
#define HTML_IS_CLUEALIGNED(x) (HTML_CHECK_TYPE ((x), HTML_TYPE_CLUEALIGNED))

struct _HTMLClueAligned {
	HTMLClue clue;

	HTMLClueAligned *next_aligned;
};

struct _HTMLClueAlignedClass {
	HTMLClueClass clue_class;
};

void        html_cluealigned_type_init   (void);
void        html_cluealigned_class_init  (HTMLClueAlignedClass *klass,
					  HTMLType              type,
					  guint                 object_size);
void        html_cluealigned_init        (HTMLClueAligned      *aligned,
					  HTMLClueAlignedClass *klass,
					  HTMLObject           *parent,
					  gint                  x,
					  gint                  y,
					  gint                  max_width,
					  gint                  percent);
HTMLObject *html_cluealigned_new         (HTMLObject           *parent,
					  gint                  x,
					  gint                  y,
					  gint                  max_width,
					  gint                  percent);

#endif /* _HTML_CLUEALIGNED_H_ */
