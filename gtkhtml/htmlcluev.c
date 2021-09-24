/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
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

#include <config.h>
#include "gtkhtml.h"
#include "htmlcluealigned.h"
#include "htmlcluev.h"
#include "htmlengine.h"
#include "htmlpainter.h"
#include "htmlcolor.h"
#include "htmlcolorset.h"
#include "htmlsettings.h"
#include "htmlstyle.h"


HTMLClueVClass html_cluev_class;

static HTMLClueClass *parent_class = NULL;


/* FIXME this must be rewritten as the multiple type casts make my head spin.
   The types in `HTMLClueAligned' are chosen wrong.  */
static void
remove_aligned_by_parent ( HTMLClueV *cluev,
			   HTMLObject *p )
{
    HTMLClueAligned *tmp;
	HTMLObject *obj;

    tmp = NULL;
    obj = cluev->align_left_list;

    while ( obj ) {
		if ( obj->parent == p ) {
			if ( tmp ) {
				tmp->next_aligned = HTML_CLUEALIGNED (obj)->next_aligned;
				tmp = HTML_CLUEALIGNED (obj);
			} else {
				cluev->align_left_list
					= HTML_OBJECT (HTML_CLUEALIGNED (obj)->next_aligned);
				tmp = NULL;
			}
		} else {
			tmp = HTML_CLUEALIGNED (obj);
		}

		obj = HTML_OBJECT (HTML_CLUEALIGNED (obj)->next_aligned);
    }

    tmp = NULL;
    obj = cluev->align_right_list;

    while ( obj ) {
		if ( obj->parent == p ) {
			if ( tmp ) {
				tmp->next_aligned = HTML_CLUEALIGNED (obj)->next_aligned;
				tmp = HTML_CLUEALIGNED (obj);
			} else {
				cluev->align_right_list
					= HTML_OBJECT (HTML_CLUEALIGNED (obj)->next_aligned);
				tmp = NULL;
			}
		} else {
			tmp = HTML_CLUEALIGNED (obj);
		}

		obj = HTML_OBJECT (HTML_CLUEALIGNED (obj)->next_aligned);
    }
}

static HTMLObject *
cluev_next_aligned (HTMLObject *aclue)
{
	return HTML_OBJECT (HTML_CLUEALIGNED (aclue)->next_aligned);
}

static gint
get_lmargin (HTMLObject *o, HTMLPainter *painter)
{
	return (HTML_CLUEV (o)->padding + HTML_CLUEV (o)->border_width) * html_painter_get_pixel_size (painter)
		+ (o->parent ?  html_object_get_left_margin (o->parent, painter, o->y, TRUE) : 0);
}

static void
add_clear_area_behind (GList **changed_objs, HTMLObject *o, gint x, gint y, gint w, gint h)
{
	HTMLObjectClearRectangle *cr;

	cr = g_new (HTMLObjectClearRectangle, 1);

	cr->object = o;
	cr->x = x;
	cr->y = y;
	cr->width = w;
	cr->height = h;

	*changed_objs = g_list_prepend (*changed_objs, cr);
	/* NULL meens: clear rectangle follows */
	*changed_objs = g_list_prepend (*changed_objs, NULL);
}

static gboolean
html_cluev_do_layout (HTMLObject *o, HTMLPainter *painter, gboolean calc_size, GList **changed_objs)
{
	HTMLClueV *cluev;
	HTMLClue *clue;
	HTMLObject *obj;
	HTMLObject *aclue;
	GList *local_changed_objs;
	gint lmargin;
	gboolean changed;
	gint old_width, old_ascent, old_descent;
	gint new_x;
	gint pixel_size;
	gint padding;
	gint padding2;
	gboolean first_change;
	gint first_y_off = 0;

	/* printf ("HTMLClueV::do_layout\n"); */

	cluev = HTML_CLUEV (o);
	clue = HTML_CLUE (o);

	pixel_size = html_painter_get_pixel_size (painter);
	padding    = pixel_size * (cluev->padding + cluev->border_width);
	padding2   = 2 * padding;

	old_width = o->width;
	old_ascent = o->ascent;
	old_descent = o->descent;

	changed = FALSE;
	first_change = TRUE;
	local_changed_objs = NULL;

	lmargin = get_lmargin (o, painter);

	/* If we have already called calc_size for the children, then just
	   continue from the last object done in previous call. */

	if (clue->curr != NULL) {
		if (clue->curr->prev)
			o->ascent = clue->curr->prev->y + clue->curr->prev->descent;
		else
			o->ascent = padding;
		remove_aligned_by_parent (cluev, clue->curr);
	} else {
		o->width = 0;
		o->ascent = padding;
		o->descent = 0;
		clue->curr = clue->head;
	}

	while (clue->curr != NULL) {
		gint old_y, old_y_off, new_y_off;
		/* Set an initial ypos so that the alignment stuff knows where
		   the top of this object is */
		old_y = clue->curr->y;
		old_y_off = clue->curr->y - clue->curr->ascent;
		clue->curr->y = o->ascent;

		switch (html_object_get_clear (clue->curr)) {
		case HTML_CLEAR_ALL: {
			gint y;

			do {
				y = clue->curr->y;
				clue->curr->y = html_clue_get_left_clear (clue, clue->curr->y);
				clue->curr->y = html_clue_get_right_clear (clue, clue->curr->y);
			} while (clue->curr->y != y);
			break;
		}
		case HTML_CLEAR_LEFT:
			clue->curr->y = html_clue_get_left_clear (clue, clue->curr->y);
			break;
		case HTML_CLEAR_RIGHT:
			clue->curr->y = html_clue_get_right_clear (clue, clue->curr->y);
			break;
		case HTML_CLEAR_NONE:
			break;
		case HTML_CLEAR_INHERIT:
			/* TODO */
			break;
		}

		o->ascent = clue->curr->y;
		lmargin = get_lmargin (o, painter);


		if (calc_size)
			changed |= html_object_calc_size (clue->curr, painter, changed_objs);

		if (o->width < clue->curr->width + padding2)
			o->width = clue->curr->width + padding2;
		o->ascent += clue->curr->ascent + clue->curr->descent;

		new_y_off = o->ascent - clue->curr->descent - clue->curr->ascent;
		if (clue->curr->x != lmargin || old_y_off != new_y_off) {
			if (changed_objs) {
				/* printf ("y: %d ", o->ascent - clue->curr->descent); */
				if (first_change) {
					first_change = FALSE;
				        /* if it's new one (y == 0) clear from new y_off, else from old one or new one,
					   which one is higher */
					first_y_off = old_y && old_y_off < new_y_off ? old_y_off : new_y_off;
					/* printf ("\nfirst_y_off: %d x %d --> %d\n", old_y_off, new_y_off, first_y_off); */
				}
				html_object_add_to_changed (&local_changed_objs, clue->curr);
			}
		}
		clue->curr->x = lmargin;
		clue->curr->y = o->ascent - clue->curr->descent;

		clue->curr = clue->curr->next;
	}

	o->ascent += padding;

	/* Remember the last object so that we can start from here next time
	   we are called. */
	clue->curr = clue->tail;

	if (o->max_width != 0 && o->width < o->max_width)
		o->width = o->max_width;

	if (clue->halign == HTML_HALIGN_CENTER) {
		for (obj = clue->head; obj != 0; obj = obj->next) {
			new_x = lmargin + (o->width - obj->width - padding2) / 2;
			if (obj->x != new_x) {
				obj->x = new_x;
				changed = TRUE;
			}
		}
	} else if (clue->halign == HTML_HALIGN_RIGHT) {
		for (obj = clue->head; obj != 0; obj = obj->next) {
			new_x = lmargin + (o->width - obj->width - padding2);
			if (obj->x != new_x) {
				obj->x = new_x;
				changed = TRUE;
			}
		}
	}

	for (aclue = cluev->align_left_list; aclue != NULL; aclue = cluev_next_aligned (aclue)) {
		if (aclue->y + aclue->parent->y - aclue->parent->ascent > o->ascent)
			o->ascent = aclue->y + aclue->parent->y - aclue->parent->ascent;
	}

	for (aclue = cluev->align_right_list; aclue != NULL; aclue = cluev_next_aligned (aclue)) {
		if (aclue->y + aclue->parent->y - aclue->parent->ascent > o->ascent)
			o->ascent = aclue->y + aclue->parent->y - aclue->parent->ascent;
	}

	if (! changed
	    && (o->ascent != old_ascent || o->descent != old_descent || o->width != old_width))
		changed = TRUE;

	if (changed_objs && local_changed_objs) {
		if (!first_change && o->width > o->max_width) {
			add_clear_area_behind (changed_objs, o, o->max_width, first_y_off,
					       o->width - o->max_width, o->ascent + o->descent - first_y_off);
		}
		*changed_objs = g_list_concat (local_changed_objs, *changed_objs);
	}

	return changed;
}


/* HTMLObject methods.  */

static void
copy (HTMLObject *self,
      HTMLObject *dest)
{
	(* HTML_OBJECT_CLASS (parent_class)->copy) (self, dest);

	HTML_CLUEV (dest)->padding = HTML_CLUEV (self)->padding;

	HTML_CLUEV (dest)->border_width = HTML_CLUEV (self)->border_width;
	HTML_CLUEV (dest)->border_style = HTML_CLUEV (self)->border_style;
	HTML_CLUEV (dest)->border_color = HTML_CLUEV (self)->border_color;
	if (HTML_CLUEV (dest)->border_color)
		html_color_ref (HTML_CLUEV (dest)->border_color);
	HTML_CLUEV (dest)->background_color = HTML_CLUEV (self)->background_color;
	if (HTML_CLUEV (dest)->background_color)
		html_color_ref (HTML_CLUEV (dest)->background_color);

	HTML_CLUEV (dest)->align_left_list = NULL;
	HTML_CLUEV (dest)->align_right_list = NULL;

	HTML_CLUEV (dest)->dir = HTML_CLUEV (self)->dir;
}

static gboolean
html_cluev_real_calc_size (HTMLObject *o, HTMLPainter *painter, GList **changed_objs)
{
	return html_cluev_do_layout (o, painter, TRUE, changed_objs);
}

static gint
calc_min_width (HTMLObject *o,
		HTMLPainter *painter)
{
	return (* HTML_OBJECT_CLASS (parent_class)->calc_min_width) (o, painter)
		+ 2 * html_painter_get_pixel_size (painter) * (HTML_CLUEV (o)->padding + HTML_CLUEV (o)->border_width);
}

static gint
calc_preferred_width (HTMLObject *o,
		      HTMLPainter *painter)
{
	return (* HTML_OBJECT_CLASS (parent_class)->calc_preferred_width) (o, painter)
		+ 2 * html_painter_get_pixel_size (painter) * (HTML_CLUEV (o)->padding + HTML_CLUEV (o)->border_width);
}

static void
set_max_width (HTMLObject *o, HTMLPainter *painter, gint max_width)
{
	HTMLObject *obj;

	o->max_width = max_width;
	max_width   -= 2 * (HTML_CLUEV (o)->padding + HTML_CLUEV (o)->border_width) * html_painter_get_pixel_size (painter);
	for (obj = HTML_CLUE (o)->head; obj != NULL; obj = obj->next)
		html_object_set_max_width (obj, painter, max_width);
}

static void
set_max_height (HTMLObject *o, HTMLPainter *painter, gint height)
{
	HTMLClue *clue = HTML_CLUE (o);

	/* Is it necessary to calc size here? It doesn't seem to. */
	/* html_object_calc_size (o, painter, NULL); */
	if (o->ascent < height) {
		(* HTML_OBJECT_CLASS (parent_class)->set_max_height) (o, painter, height);
		clue->curr = NULL;
	}
}

static void
reset (HTMLObject *clue)
{
	HTMLClueV *cluev;

	cluev = HTML_CLUEV (clue);

	HTML_OBJECT_CLASS (&html_clue_class)->reset (clue);

	cluev->align_left_list = NULL;
	cluev->align_right_list = NULL;
}

static void
draw (HTMLObject *o,
      HTMLPainter *p,
      gint x, gint y,
      gint width, gint height,
      gint tx, gint ty)
{
	HTMLObject *aclue;
	HTMLClueV *cluev;
	GdkRectangle paint;

	cluev = HTML_CLUEV (o);

	if (!html_object_intersect (o, &paint, x, y, width, height))
		return;

	if (cluev->background_color) {
		html_painter_alloc_color (p, &cluev->background_color->color);
		html_painter_draw_background (p,
					      &cluev->background_color->color,
					      NULL, tx + paint.x, ty + paint.y, paint.width, paint.height, 0, 0);
	}

	HTML_OBJECT_CLASS (&html_clue_class)->draw (o,
						    p,
						    x, y ,
						    width, height,
						    tx, ty);

	tx += o->x;
	ty += o->y - o->ascent;

	for ( aclue = HTML_CLUEV (o)->align_left_list;
	      aclue != NULL;
	      aclue = cluev_next_aligned (aclue) ) {
		html_object_draw (aclue,
				  p,
				  x - o->x - aclue->parent->x,
				  y - (o->y - o->ascent) - (aclue->parent->y - aclue->parent->ascent),
				  width - aclue->parent->x, height,
				  tx + aclue->parent->x,
				  ty + aclue->parent->y - aclue->parent->ascent);
	}

	for (aclue = HTML_CLUEV (o)->align_right_list;
	     aclue != NULL;
	     aclue = cluev_next_aligned (aclue)) {
		html_object_draw (aclue,
				  p,
				  x - o->x - aclue->parent->x,
				  y - (o->y - o->ascent) - (aclue->parent->y - aclue->parent->ascent),
				  width - aclue->parent->x, height,
				  tx + aclue->parent->x,
				  ty + aclue->parent->y - aclue->parent->ascent);
	}

	if (cluev->border_style != HTML_BORDER_NONE && cluev->border_width > 0) {
		GdkColor *color;

		if (cluev->border_color) {
			html_painter_alloc_color (p, &cluev->border_color->color);
			color = &cluev->border_color->color;
		} else {
			HTMLEngine *e = html_object_engine (o, GTK_HTML (p->widget)->engine);
			color = &html_colorset_get_color_allocated (e->settings->color_set,
								    p, HTMLTextColor)->color;
		}

		html_painter_draw_border (p, color,
					  tx, ty,
					  o->width,
					  o->ascent + o->descent,
					  cluev->border_style,
					  html_painter_get_pixel_size (p) * cluev->border_width);
	}
}

static HTMLObject *
check_point (HTMLObject *self,
	     HTMLPainter *painter,
	     gint x, gint y,
	     guint *offset_return,
	     gboolean for_cursor)
{
	HTMLObject *p;
	HTMLObject *obj;
	HTMLClueAligned *clue;
	int padding = HTML_CLUEV (self)->padding;

	if (x < self->x || x >= self->x + self->width
	    || y < self->y - self->ascent || y >= self->y + self->descent)
		return NULL;

	x = x - self->x;
	y = y - self->y + self->ascent;

	if (!for_cursor) {
		if (x < padding || y < padding) {
			if (offset_return)
				*offset_return = 0;
			return self;
		}
		if (x >= self->width - padding || y >= self->ascent + self->descent - padding) {
			if (offset_return)
				*offset_return = 1;
			return self;
		}
	}

	for (clue = HTML_CLUEALIGNED (HTML_CLUEV (self)->align_left_list);
	     clue != NULL;
	     clue = clue->next_aligned) {
		HTMLObject *parent;

		parent = HTML_OBJECT (clue)->parent;
		obj = html_object_check_point (HTML_OBJECT (clue),
					       painter,
					       x - HTML_OBJECT (clue)->parent->x,
					       y - HTML_OBJECT (clue)->parent->y + HTML_OBJECT (clue)->parent->ascent,
					       offset_return,
					       for_cursor);
		if (obj != NULL) {
			return obj;
		}
	}

	for (clue = HTML_CLUEALIGNED (HTML_CLUEV (self)->align_right_list);
	     clue != NULL;
	     clue = clue->next_aligned) {
		HTMLObject *parent;

		parent = HTML_OBJECT (clue)->parent;
		obj = html_object_check_point (HTML_OBJECT (clue),
					       painter,
					       x - HTML_OBJECT (clue)->parent->x,
					       y - HTML_OBJECT (clue)->parent->y + HTML_OBJECT (clue)->parent->ascent,
					       offset_return,
					       for_cursor);
		if (obj != NULL) {
			return obj;
		}
	}

	for (p = HTML_CLUE (self)->head; p != 0; p = p->next) {
		gint x1, y1;

		if (! for_cursor) {
			x1 = x;
			y1 = y;
		} else {
			if (x >= p->x + p->width) {
				x1 = MAX (0, p->x + p->width - 1);
			} else if (x < p->x) {
				x1 = p->x;
			} else {
				x1 = x;
			}

			if (p->next == NULL && y > p->y + p->descent - 1) {
				x1 = MAX (0, p->x + p->width - 1);
				y1 = p->y + p->descent - 1;
			} else if (p->prev == NULL && y < p->y - p->ascent) {
				y1 = p->y - p->ascent;
			} else {
				y1 = y;
			}
		}

		obj = html_object_check_point (p, painter, x1, y1, offset_return, for_cursor);
		if (obj != NULL)
			return obj;
	}

	if (!for_cursor) {
		if (x >= 0 && y >= 0 && x < self->width && y < self->ascent + self->descent) {
			if (offset_return) {
				if (x < self->width/2)
					*offset_return = 0;
				else
					*offset_return = 1;
			}
			return self;
		}
	}

	return NULL;
}

static gboolean
relayout (HTMLObject *self,
	  HTMLEngine *engine,
	  HTMLObject *child)
{
	gint prev_width, prev_ascent, prev_descent;
	gboolean changed;

	if (html_engine_frozen (engine))
		return FALSE;

	if (child == NULL)
		child = HTML_CLUE (self)->head;
	html_object_calc_size (child, engine->painter, NULL);

	HTML_CLUE (self)->curr = NULL;

	prev_width = self->width;
	prev_ascent = self->ascent;
	prev_descent = self->descent;

	changed = html_cluev_do_layout (self, engine->painter, FALSE, NULL);
	if (changed)
		html_engine_queue_draw (engine, self);

	if (prev_width == self->width
	    && prev_ascent == self->ascent
	    && prev_descent == self->descent)
		return FALSE;

	if (self->parent == NULL) {
		/* FIXME resize the widget, e.g. scrollbars and such.  */
		html_engine_queue_draw (engine, self);

		/* FIXME extreme ugliness.  */
		self->x = 0;
		self->y = self->ascent;
	} else {
		/* Relayout our parent starting from us.  */
		if (! html_object_relayout (self->parent, engine, self))
			html_engine_queue_draw (engine, self);
	}

	/* If the object has shrunk, we have to clean the areas around
	   it so that we don't leave garbage on the screen.  FIXME:
	   this wastes some time if there is an object on the right of
	   or under this one.  */

	if (prev_ascent + prev_descent > self->ascent + self->descent)
		html_engine_queue_clear (engine,
					 self->x,
					 self->y + self->descent,
					 self->width,
					 (prev_ascent + prev_descent
					  - (self->ascent + self->descent)));

	if (prev_width > self->width)
		html_engine_queue_clear (engine,
					 self->x + self->width,
					 self->y - self->ascent,
					 prev_width - self->width,
					 self->ascent + self->descent);

	return TRUE;
}

static gint
get_left_margin (HTMLObject *self, HTMLPainter *painter, gint y, gboolean with_aligned)
{
	HTMLObject *aclue;
	HTMLClueV *cluev;
	gint margin;

	cluev = HTML_CLUEV (self);
	margin = 0;

	if (with_aligned)
		for (aclue = cluev->align_left_list;
		     aclue != NULL;
		     aclue = cluev_next_aligned (aclue)) {
			if ((aclue->y - aclue->ascent + aclue->parent->y - aclue->parent->ascent
			     <= y)
			    && (aclue->y + aclue->parent->y - aclue->parent->ascent
				> y))
				margin = aclue->x + aclue->width;
		}

	return margin;
}

static gint
get_right_margin (HTMLObject *self, HTMLPainter *painter, gint y, gboolean with_aligned)
{
	HTMLClueV *cluev;
	/* FIXME: Should be HTMLAligned */
	HTMLObject *aclue;
	gint margin;

	cluev = HTML_CLUEV (self);
	margin = self->max_width - 2 * (cluev->padding + cluev->border_width)* html_painter_get_pixel_size (painter);

	if (with_aligned)
		for (aclue = cluev->align_right_list;
		     aclue != NULL;
		     aclue = cluev_next_aligned (aclue)) {
			if ((aclue->y - aclue->ascent + aclue->parent->y - aclue->parent->ascent) <= y
			    && aclue->y + aclue->parent->y - aclue->parent->ascent > y)
				margin = aclue->x;
		}

	return margin;
}


/* HTMLClue methods.  */

static void
find_free_area (HTMLClue *clue, HTMLPainter *painter, gint y, gint width, gint height,
		gint indent, gint *y_pos, gint *_lmargin, gint *_rmargin)
{
	HTMLClueV *cluev = HTML_CLUEV (clue);
	gint try_y = y;
	gint lmargin;
	gint rmargin;
	gint lm, rm;
	HTMLObject *aclue;
	gint next_y, top_y, base_y=0;

	next_y = 0;
	while (1) {
		lmargin = indent;
		rmargin = HTML_OBJECT (clue)->max_width - 2 * (cluev->padding + cluev->border_width) * html_painter_get_pixel_size (painter);

		for (aclue = cluev->align_left_list; aclue != 0; aclue = cluev_next_aligned (aclue)) {
			base_y = (aclue->y + aclue->parent->y
				  - aclue->parent->ascent);
			top_y = base_y - aclue->ascent;

			if ((top_y < try_y + height) && (base_y > try_y)) {
				lm = aclue->x + aclue->width;
				if (lm > lmargin)
					lmargin = lm;

				if ((next_y == 0) || (base_y < next_y)) {
					next_y = base_y;

				}
			}
		}

		for (aclue = cluev->align_right_list; aclue != 0; aclue = cluev_next_aligned (aclue)) {
			base_y = (aclue->y + aclue->parent->y
				  - aclue->parent->ascent);
			top_y = base_y - aclue->ascent;

			if ((top_y < try_y + height) && (base_y > try_y)) {
				rm = aclue->x;
				if (rm < rmargin)
					rmargin = rm;

				if ((next_y == 0) || (base_y < next_y)) {
					next_y = base_y;
				}
			}
		}

		if (lmargin == indent
		    && rmargin == MAX (HTML_OBJECT (clue)->max_width, HTML_OBJECT (clue)->width))
			break;

		if ((rmargin - lmargin) >= width)
			break;

		if (try_y == next_y)
			break;

		try_y = next_y;
	}

	*y_pos = MAX (y, try_y);

	*_rmargin = rmargin;
	*_lmargin = lmargin;
}

static gboolean
appended (HTMLClue *clue, HTMLClue *aclue)
{
	/* Returns whether aclue is already in the alignList */
	HTMLClueAligned *aligned;

	if (aclue->halign == HTML_HALIGN_LEFT) {
		aligned = HTML_CLUEALIGNED (HTML_CLUEV (clue)->align_left_list);
	}
	else {
		aligned = HTML_CLUEALIGNED (HTML_CLUEV (clue)->align_right_list);
	}

	while (aligned) {
		if (aligned == HTML_CLUEALIGNED (aclue))
			return TRUE;
		aligned = HTML_CLUEALIGNED (aligned->next_aligned);
	}
	return FALSE;
}

static void
append_left_aligned (HTMLClue *clue, HTMLPainter *painter, HTMLClue *aclue, gint *lmargin, gint *rmargin, gint indent)
{
	gint y_pos, y_inc;

	html_clue_find_free_area (clue, painter,
				  HTML_OBJECT (aclue)->parent->y,
				  HTML_OBJECT (aclue)->width,
				  HTML_OBJECT (aclue)->ascent + HTML_OBJECT (aclue)->descent,
				  indent, &y_pos, lmargin, rmargin);

	/* Set position */
	y_inc = y_pos - HTML_OBJECT (aclue)->parent->y;

	HTML_OBJECT (aclue)->x = *lmargin;
	HTML_OBJECT (aclue)->y = HTML_OBJECT (aclue)->parent->ascent + HTML_OBJECT (aclue)->ascent + y_inc;

	/* Insert clue in align list */
	if (!HTML_CLUEV (clue)->align_left_list) {
		HTML_CLUEV (clue)->align_left_list = HTML_OBJECT (aclue);
		HTML_CLUEALIGNED (aclue)->next_aligned = NULL;
	} else {
		HTMLClueAligned *obj = HTML_CLUEALIGNED (HTML_CLUEV (clue)->align_left_list);
		while (obj->next_aligned) {
			if (obj == HTML_CLUEALIGNED (aclue))
				return;
			obj = obj->next_aligned;
		}
		if (obj == HTML_CLUEALIGNED (aclue))
			return;

		obj->next_aligned = HTML_CLUEALIGNED (aclue);
		HTML_CLUEALIGNED (aclue)->next_aligned = NULL;
	}

	*lmargin += HTML_OBJECT (aclue)->width;
}

static void
append_right_aligned (HTMLClue *clue, HTMLPainter *painter, HTMLClue *aclue, gint *lmargin, gint *rmargin, gint indent)
{
	gint y_pos, y_inc;

	html_clue_find_free_area (clue, painter, HTML_OBJECT (aclue)->parent->y,
				  HTML_OBJECT (aclue)->width,
				  HTML_OBJECT (aclue)->ascent + HTML_OBJECT (aclue)->descent, indent,
				  &y_pos, lmargin, rmargin);

	*rmargin -= HTML_OBJECT (aclue)->width;

	/* Set position */
	y_inc = y_pos - HTML_OBJECT (aclue)->parent->y;

	HTML_OBJECT (aclue)->x = *rmargin;
	HTML_OBJECT (aclue)->y = HTML_OBJECT (aclue)->parent->ascent + HTML_OBJECT (aclue)->ascent + y_inc;

	/* Insert clue in align list */
	if (!HTML_CLUEV (clue)->align_right_list) {
		HTML_CLUEV (clue)->align_right_list = HTML_OBJECT (aclue);
		HTML_CLUEALIGNED (aclue)->next_aligned = NULL;
	} else {
		HTMLClueAligned *obj = HTML_CLUEALIGNED (HTML_CLUEV (clue)->align_right_list);
		while (obj->next_aligned) {
			if (obj == HTML_CLUEALIGNED (aclue))
				return;
			obj = obj->next_aligned;
		}
		if (obj == HTML_CLUEALIGNED (aclue))
			return;

		obj->next_aligned = HTML_CLUEALIGNED (aclue);
		HTML_CLUEALIGNED (aclue)->next_aligned = NULL;
	}
}

static gint
get_left_clear (HTMLClue *self,
		gint y)
{
	HTMLObject *p;
	gint top_y, base_y;

	/* XXX we assume the parent's size has already been calculated here.  */

	for (p = HTML_CLUEV (self)->align_left_list;
	     p != NULL;
	     p = HTML_OBJECT (HTML_CLUEALIGNED (p)->next_aligned)) {
		base_y = p->y + p->parent->y - p->parent->ascent;
		top_y = base_y - p->ascent;

		if (top_y <= y && y < base_y + p->descent)
			y = base_y + p->descent;
	}

	return y;
}

static gint
get_right_clear (HTMLClue *self,
		 gint y)
{
	HTMLObject *p;
	gint top_y, base_y;

	/* XXX we assume the parent's size has already been calculated here.  */

	for (p = HTML_CLUEV (self)->align_right_list;
	     p != NULL;
	     p = HTML_OBJECT (HTML_CLUEALIGNED (p)->next_aligned)) {
		base_y = p->y + p->parent->y - p->parent->ascent;
		top_y = base_y - p->ascent;

		if (top_y <= y && y < base_y + p->descent)
			y = base_y + p->descent;
	}

	return y;
}

static HTMLDirection
html_cluev_real_get_direction (HTMLObject *o)
{
	if (HTML_CLUEV (o)->dir == HTML_DIRECTION_DERIVED && o->parent) {
		return html_object_get_direction (o->parent);
	}

	return HTML_CLUEV (o)->dir;
}

static void
html_cluev_destroy (HTMLObject *self)
{
	HTMLClueV *cluev = HTML_CLUEV (self);

	if (cluev->border_color)
		html_color_unref (cluev->border_color);
	cluev->border_color = NULL;

	if (cluev->background_color)
		html_color_unref (cluev->background_color);
	cluev->background_color = NULL;

	(* HTML_OBJECT_CLASS (parent_class)->destroy) (self);
}


void
html_cluev_type_init (void)
{
	html_cluev_class_init (&html_cluev_class, HTML_TYPE_CLUEV, sizeof (HTMLClueV));
}

void
html_cluev_class_init (HTMLClueVClass *klass,
		       HTMLType type,
		       guint size)
{
	HTMLObjectClass *object_class;
	HTMLClueClass *clue_class;

	object_class = HTML_OBJECT_CLASS (klass);
	clue_class = HTML_CLUE_CLASS (klass);

	html_clue_class_init (clue_class, type, size);

	object_class->copy = copy;
	object_class->calc_size = html_cluev_real_calc_size;
	object_class->calc_min_width = calc_min_width;
	object_class->calc_preferred_width = calc_preferred_width;
	object_class->relayout = relayout;
	object_class->set_max_width = set_max_width;
	object_class->set_max_height = set_max_height;
	object_class->reset = reset;
	object_class->draw = draw;
	object_class->check_point = check_point;
	object_class->get_left_margin = get_left_margin;
	object_class->get_right_margin = get_right_margin;
	object_class->get_direction = html_cluev_real_get_direction;
	object_class->destroy = html_cluev_destroy;

	clue_class->get_left_clear = get_left_clear;
	clue_class->get_right_clear = get_right_clear;
	clue_class->find_free_area = find_free_area;
	clue_class->appended = appended;
	clue_class->append_left_aligned = append_left_aligned;
	clue_class->append_right_aligned = append_right_aligned;

	parent_class = &html_clue_class;
}

void
html_cluev_init (HTMLClueV *cluev,
		 HTMLClueVClass *klass,
		 gint x, gint y,
		 gint percent)
{
	HTMLObject *object;
	HTMLClue *clue;

	object = HTML_OBJECT (cluev);
	clue = HTML_CLUE (cluev);

	html_clue_init (clue, HTML_CLUE_CLASS (klass));

	object->x = x;
	object->y = y;
	object->percent = percent;

	clue->valign = HTML_VALIGN_BOTTOM;
	clue->halign = HTML_HALIGN_NONE;
	cluev->dir = HTML_DIRECTION_DERIVED;
	clue->head = clue->tail = clue->curr = NULL;

	cluev->align_left_list = NULL;
	cluev->align_right_list = NULL;
	cluev->padding = 0;
	cluev->border_style = HTML_BORDER_NONE;
	cluev->border_width = 0;
	cluev->border_color = NULL;
	cluev->background_color = NULL;
}

HTMLObject *
html_cluev_new (gint x, gint y, gint percent)
{
	HTMLClueV *cluev;

	cluev = g_new (HTMLClueV, 1);
	html_cluev_init (cluev, &html_cluev_class, x, y, percent);

	return HTML_OBJECT (cluev);
}

void
html_cluev_set_style (HTMLClueV *cluev, HTMLStyle *style)
{
	if (style != NULL) {
		if (cluev->border_color)
			html_color_unref (cluev->border_color);

		if (cluev->background_color)
			html_color_unref (cluev->background_color);

		cluev->padding = style->padding;

		cluev->border_style = style->border_style;
		cluev->border_width = style->border_width;
		cluev->border_color = style->border_color;
		if (cluev->border_color)
			html_color_ref (cluev->border_color);

		cluev->background_color = style->bg_color;
		if (cluev->background_color)
			html_color_ref (cluev->background_color);
	} else {
		if (cluev->border_color)
			html_color_unref (cluev->border_color);

		if (cluev->background_color)
			html_color_unref (cluev->background_color);

		cluev->border_style = HTML_BORDER_NONE;
		cluev->border_width = 0;
		cluev->border_color = NULL;
		cluev->background_color = NULL;
	}
}
