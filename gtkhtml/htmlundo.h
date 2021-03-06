/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library

   Copyright (C) 2000 Helix Code, Inc.

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

#ifndef _HTML_UNDO_H
#define _HTML_UNDO_H

#define HTML_UNDO_LIMIT 1024

#include "htmlundo-action.h"
#include "htmlenums.h"

#define HTML_UNDO_DATA(x) ((HTMLUndoData *) x)
struct _HTMLUndoData {
	HTMLUndoDataDestroyFunc destroy;
	gint ref_count;
};

HTMLUndo *html_undo_new              (void);
void      html_undo_destroy          (HTMLUndo          *undo);
void      html_undo_do_undo          (HTMLUndo          *undo,
				      HTMLEngine        *engine);
void      html_undo_do_redo          (HTMLUndo          *undo,
				      HTMLEngine        *engine);
void      html_undo_discard_redo     (HTMLUndo          *undo);
void      html_undo_add_undo_action  (HTMLUndo          *undo,
				      HTMLUndoAction    *action);
void      html_undo_add_redo_action  (HTMLUndo          *undo,
				      HTMLUndoAction    *action);
void      html_undo_add_action       (HTMLUndo          *undo,
				      HTMLUndoAction    *action,
				      HTMLUndoDirection  dir);
gboolean  html_undo_has_undo_steps   (HTMLUndo          *undo);
void      html_undo_reset            (HTMLUndo          *undo);
void      html_undo_level_begin      (HTMLUndo          *undo,
				      const gchar       *undo_description,
				      const gchar       *redo_description);
void      html_undo_level_end        (HTMLUndo          *undo);
gint      html_undo_get_step_count   (HTMLUndo          *undo);
void      html_undo_freeze           (HTMLUndo          *undo);
void      html_undo_thaw             (HTMLUndo          *undo);
/*
 *  Undo Data
 */
void               html_undo_data_init          (HTMLUndoData      *data);
void               html_undo_data_ref           (HTMLUndoData      *data);
void               html_undo_data_unref         (HTMLUndoData      *data);
/*
 * Undo Direction
 */
HTMLUndoDirection  html_undo_direction_reverse  (HTMLUndoDirection  dir);

#endif /* _HTML_UNDO_H */
