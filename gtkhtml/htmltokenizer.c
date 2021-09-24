/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
    Copyright (C) 1997 Martin Jones (mjones@kde.org)
              (C) 1997 Torben Weis (weis@kde.org)
	      (C) 1999 Anders Carlsson (andersca@gnu.org)
	      (C) 2000 Helix Code, Inc., Radek Doulik (rodo@helixcode.com)
	      (C) 2001 Ximian, Inc.

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

/* The HTML Tokenizer */
#include <config.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "htmltokenizer.h"
#include "htmlentity.h"

enum {
	HTML_TOKENIZER_BEGIN_SIGNAL,
	HTML_TOKENIZER_END_SIGNAL,
	HTML_TOKENIZER_LAST_SIGNAL
};

static guint html_tokenizer_signals[HTML_TOKENIZER_LAST_SIGNAL] = { 0 };

#define TOKEN_BUFFER_SIZE (1 << 10)
#define INVALID_CHARACTER_MARKER '?'

#define dt(x)

typedef struct _HTMLBlockingToken HTMLBlockingToken;
typedef struct _HTMLTokenBuffer   HTMLTokenBuffer;
typedef	enum { Table }            HTMLTokenType;

struct _HTMLTokenBuffer {
	gint size;
	gint used;
	gchar * data;
};
struct _HTMLTokenizerPrivate {

	/* token buffers list */
	GList *token_buffers;

	/* current read_buf position in list */
	GList *read_cur;

	/* current read buffer */
	HTMLTokenBuffer * read_buf;
	HTMLTokenBuffer * write_buf;

	/* position in the read_buf */
	gint read_pos;

	/* non-blocking and blocking unreaded tokens in tokenizer */
	gint tokens_num;
	gint blocking_tokens_num;

	gchar *dest;
	gchar *buffer;
	gint size;

	gboolean skipLF; /* Skip the LF par of a CRLF sequence */

	gboolean tag; /* Are we in an html tag? */
	gboolean tquote; /* Are we in quotes in an html tag? */
	gboolean startTag;
	gboolean comment; /* Are we in a comment block? */
	gboolean title; /* Are we in a <title> block? */
	gboolean style; /* Are we in a <style> block? */
	gboolean script; /* Are we in a <script> block? */
	gboolean textarea; /* Are we in a <textarea> block? */
	gint     pre; /* Are we in a <pre> block? */
	gboolean select; /* Are we in a <select> block? */
	gboolean charEntity; /* Are we in an &... sequence? */
	gboolean extension; /* Are we in an <!-- +GtkHTML: sequence? */
	gboolean aTag; /* Are we in a <a/> tag*/

	enum {
		NoneDiscard = 0,
		SpaceDiscard,
		LFDiscard
	} discard;

	enum {
		NonePending = 0,
		SpacePending,
		LFPending,
		TabPending
	} pending;


	gchar searchBuffer[20];
	gint searchCount;
	gint searchGtkHTMLCount;
	gint searchExtensionEndCount;

	gchar *scriptCode;
	gint scriptCodeSize;
	gint scriptCodeMaxSize;

	GList *blocking; /* Blocking tokens */

	const gchar *searchFor;
	gboolean utf8;
	gchar utf8_buffer[7];
	gint utf8_length;
};

static const gchar *commentStart = "<!--";
static const gchar *scriptEnd = "</script>";
static const gchar *styleEnd = "</style>";
static const gchar *gtkhtmlStart = "+gtkhtml:";

enum quoteEnum {
	NO_QUOTE = 0,
	SINGLE_QUOTE,
	DOUBLE_QUOTE
};

/* private tokenizer functions */
static void           html_tokenizer_reset        (HTMLTokenizer *t);
static void           html_tokenizer_add_pending  (HTMLTokenizer *t);
static void           html_tokenizer_append_token (HTMLTokenizer *t,
						   const gchar *string,
						   gint len);
static void           html_tokenizer_append_token_buffer (HTMLTokenizer *t,
							  gint min_size);

/* default implementations of tokenization functions */
static void     html_tokenizer_finalize             (GObject *);
static void     html_tokenizer_real_begin           (HTMLTokenizer *, gchar *content_type);
static void     html_tokenizer_real_write           (HTMLTokenizer *, const gchar *str, size_t size);
static void     html_tokenizer_real_end             (HTMLTokenizer *);
static gchar   *html_tokenizer_real_peek_token      (HTMLTokenizer *);
static gchar   *html_tokenizer_real_next_token      (HTMLTokenizer *);
static gboolean html_tokenizer_real_has_more_tokens (HTMLTokenizer *);

static HTMLTokenizer *html_tokenizer_real_clone     (HTMLTokenizer *);

/* blocking tokens */
static gchar             *html_tokenizer_blocking_get_name   (HTMLTokenizer  *t);
static void               html_tokenizer_blocking_pop        (HTMLTokenizer  *t);
static void               html_tokenizer_blocking_push       (HTMLTokenizer  *t,
							      HTMLTokenType   tt);
static void               html_tokenizer_tokenize_one_char   (HTMLTokenizer  *t,
							      const gchar  **src);

static void               add_unichar(HTMLTokenizer *t, gunichar wc);

static GObjectClass *parent_class = NULL;

static void
html_tokenizer_class_init (HTMLTokenizerClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;

	parent_class = g_type_class_ref (G_TYPE_OBJECT);

	html_tokenizer_signals[HTML_TOKENIZER_BEGIN_SIGNAL] =
		g_signal_new ("begin",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HTMLTokenizerClass, begin),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE,
			      1, G_TYPE_POINTER);

	html_tokenizer_signals [HTML_TOKENIZER_END_SIGNAL] =
		g_signal_new ("end",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HTMLTokenizerClass, end),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);

	object_class->finalize = html_tokenizer_finalize;

	klass->begin      = html_tokenizer_real_begin;
	klass->end        = html_tokenizer_real_end;

	klass->write      = html_tokenizer_real_write;
	klass->peek_token = html_tokenizer_real_peek_token;
	klass->next_token = html_tokenizer_real_next_token;
	klass->has_more   = html_tokenizer_real_has_more_tokens;
	klass->clone      = html_tokenizer_real_clone;
}

static void
html_tokenizer_init (HTMLTokenizer *t)
{
	struct _HTMLTokenizerPrivate *p;

	t->priv = p = g_new0 (struct _HTMLTokenizerPrivate, 1);

	p->token_buffers = NULL;
	p->read_cur  = NULL;
	p->read_buf  = NULL;
	p->write_buf = NULL;
	p->read_pos  = 0;

	p->dest = NULL;
	p->buffer = NULL;
	p->size = 0;

	p->skipLF = FALSE;
	p->tag = FALSE;
	p->tquote = FALSE;
	p->startTag = FALSE;
	p->comment = FALSE;
	p->title = FALSE;
	p->style = FALSE;
	p->script = FALSE;
	p->textarea = FALSE;
	p->pre = 0;
	p->select = FALSE;
	p->charEntity = FALSE;
	p->extension = FALSE;
	p->aTag = FALSE;

	p->discard = NoneDiscard;
	p->pending = NonePending;

	memset (p->searchBuffer, 0, sizeof (p->searchBuffer));
	p->searchCount = 0;
	p->searchGtkHTMLCount = 0;

	p->scriptCode = NULL;
	p->scriptCodeSize = 0;
	p->scriptCodeMaxSize = 0;

	p->blocking = NULL;

	p->searchFor = NULL;
}

static void
html_tokenizer_finalize (GObject *obj)
{
	HTMLTokenizer *t = HTML_TOKENIZER (obj);

	html_tokenizer_reset (t);

	g_free (t->priv);
	t->priv = NULL;

        G_OBJECT_CLASS (parent_class)->finalize (obj);
}

GType
html_tokenizer_get_type (void)
{
	static GType html_tokenizer_type = 0;

	if (!html_tokenizer_type) {
		static const GTypeInfo html_tokenizer_info = {
			sizeof (HTMLTokenizerClass),
			NULL,
			NULL,
			(GClassInitFunc) html_tokenizer_class_init,
			NULL,
			NULL,
			sizeof (HTMLTokenizer),
			1,
			(GInstanceInitFunc) html_tokenizer_init,
		};
		html_tokenizer_type = g_type_register_static (G_TYPE_OBJECT, "HTMLTokenizer", &html_tokenizer_info, 0);
	}

	return html_tokenizer_type;
}

static HTMLTokenBuffer *
html_token_buffer_new (gint size)
{
	HTMLTokenBuffer *nb = g_new (HTMLTokenBuffer, 1);

	nb->data = g_new (gchar, size);
	nb->size = size;
	nb->used = 0;

	return nb;
}

static void
html_token_buffer_destroy (HTMLTokenBuffer *tb)
{
	g_free (tb->data);
	g_free (tb);
}

static gboolean
html_token_buffer_append_token (HTMLTokenBuffer * buf, const gchar *token, gint len)
{

	/* check if we have enough free space */
	if (len + 1 > buf->size - buf->used) {
		return FALSE;
	}

	/* copy token and terminate with zero */
	strncpy (buf->data + buf->used, token, len);
	buf->used += len;
	buf->data [buf->used] = 0;
	buf->used ++;

	dt(printf ("html_token_buffer_append_token: '%s'\n", buf->data + buf->used - 1 - len));

	return TRUE;
}

HTMLTokenizer *
html_tokenizer_new (void)
{
	return (HTMLTokenizer *) g_object_new (HTML_TYPE_TOKENIZER, NULL);
}

void
html_tokenizer_destroy (HTMLTokenizer *t)
{
	g_return_if_fail (t && HTML_IS_TOKENIZER (t));

	g_object_unref (G_OBJECT (t));
}

static gchar *
html_tokenizer_real_peek_token (HTMLTokenizer *t)
{
	struct _HTMLTokenizerPrivate *p = t->priv;
	gchar *token;

	g_assert (p->read_buf);

	if (p->read_buf->used > p->read_pos) {
		token = p->read_buf->data + p->read_pos;
	} else {
		GList *next;
		HTMLTokenBuffer *buffer;

		g_assert (p->read_cur);
		g_assert (p->read_buf);

		/* lookup for next buffer */
		next = p->read_cur->next;
		g_assert (next);

		buffer = (HTMLTokenBuffer *) next->data;

		g_return_val_if_fail (buffer->used != 0, NULL);

		/* finally get first token */
		token = buffer->data;
	}

	return token;
}

static gchar *
html_tokenizer_real_next_token (HTMLTokenizer *t)
{
	struct _HTMLTokenizerPrivate *p = t->priv;
	gchar *token;

	g_assert (p->read_buf);

	/* token is in current read_buf */
	if (p->read_buf->used > p->read_pos) {
		token = p->read_buf->data + p->read_pos;
		p->read_pos += strlen (token) + 1;
	} else {
		GList *new;

		g_assert (p->read_cur);
		g_assert (p->read_buf);

		/* lookup for next buffer */
		new = p->read_cur->next;
		g_assert (new);

		/* destroy current buffer */
		p->token_buffers = g_list_remove (p->token_buffers, p->read_buf);
		html_token_buffer_destroy (p->read_buf);

		p->read_cur = new;
		p->read_buf = (HTMLTokenBuffer *) new->data;

		g_return_val_if_fail (p->read_buf->used != 0, NULL);

		/* finally get first token */
		token = p->read_buf->data;
		p->read_pos = strlen (token) + 1;
	}

	p->tokens_num--;
	g_assert (p->tokens_num >= 0);

	return token;
}

static gboolean
html_tokenizer_real_has_more_tokens (HTMLTokenizer *t)
{
	return t->priv->tokens_num > 0;
}

static HTMLTokenizer *
html_tokenizer_real_clone (HTMLTokenizer *t)
{
	return html_tokenizer_new ();
}

static void
html_tokenizer_reset (HTMLTokenizer *t)
{
	struct _HTMLTokenizerPrivate *p = t->priv;
	GList *cur = p->token_buffers;

	/* free remaining token buffers */
	while (cur) {
		g_assert (cur->data);
		html_token_buffer_destroy ((HTMLTokenBuffer *) cur->data);
		cur = cur->next;
	}

	/* reset buffer list */
	g_list_free (p->token_buffers);
	p->token_buffers = p->read_cur = NULL;
	p->read_buf = p->write_buf = NULL;
	p->read_pos = 0;

	/* reset token counters */
	p->tokens_num = p->blocking_tokens_num = 0;

	if (p->buffer)
		g_free (p->buffer);
	p->buffer = NULL;
	p->dest = NULL;
	p->size = 0;

	if (p->scriptCode)
		g_free (p->scriptCode);
	p->scriptCode = NULL;
}

static gint
charset_is_utf8 (gchar *content_type)
{
	return content_type && strstr (content_type, "charset=utf-8") != NULL;
}

static void
html_tokenizer_real_begin (HTMLTokenizer *t, gchar *content_type)
{
	struct _HTMLTokenizerPrivate *p = t->priv;

	html_tokenizer_reset (t);

	p->dest = p->buffer;
	p->tag = FALSE;
	p->pending = NonePending;
	p->discard = NoneDiscard;
	p->pre = 0;
	p->script = FALSE;
	p->style = FALSE;
	p->skipLF = FALSE;
	p->select = FALSE;
	p->comment = FALSE;
	p->textarea = FALSE;
	p->startTag = FALSE;
	p->extension = FALSE;
	p->tquote = NO_QUOTE;
	p->searchCount = 0;
	p->searchGtkHTMLCount = 0;
	p->title = FALSE;
	p->charEntity = FALSE;

	p->utf8 = charset_is_utf8 (content_type);
	p->utf8_length = 0;
#if 0
	if (p->utf8)
		g_warning ("Trying UTF-8");
	else
		g_warning ("Trying ISO-8859-1");
#endif

}

static void
destroy_blocking (gpointer data, gpointer user_data)
{
	g_free (data);
}

static void
html_tokenizer_real_end (HTMLTokenizer *t)
{
	struct _HTMLTokenizerPrivate *p = t->priv;

	if (p->buffer == 0)
		return;

	if (p->dest > p->buffer) {
		html_tokenizer_append_token (t, p->buffer, p->dest - p->buffer);
	}

	g_free (p->buffer);

	p->buffer = NULL;
	p->dest = NULL;
	p->size = 0;

	if (p->blocking) {
		g_list_foreach (p->blocking, destroy_blocking, NULL);
		p->tokens_num += p->blocking_tokens_num;
		p->blocking_tokens_num = 0;
	}
	p->blocking = NULL;
}

static void
html_tokenizer_append_token (HTMLTokenizer *t, const gchar *string, gint len)
{
	struct _HTMLTokenizerPrivate *p = t->priv;

	if (len < 1)
		return;

	/* allocate first buffer */
	if (p->write_buf == NULL)
		html_tokenizer_append_token_buffer (t, len);

	/* try append token to current buffer, if not successful, create append new token buffer */
	if (!html_token_buffer_append_token (p->write_buf, string, len)) {
		html_tokenizer_append_token_buffer (t, len+1);
		/* now it must pass as we have enough space */
		g_assert (html_token_buffer_append_token (p->write_buf, string, len));
	}

	if (p->blocking) {
		p->blocking_tokens_num++;
	} else {
		p->tokens_num++;
	}
}

static void
html_tokenizer_append_token_buffer (HTMLTokenizer *t, gint min_size)
{
	struct _HTMLTokenizerPrivate *p = t->priv;
	HTMLTokenBuffer *nb;
	gint size = TOKEN_BUFFER_SIZE;

	if (min_size > size)
		size = min_size + (min_size >> 2);

	/* create new buffer and add it to list */
	nb = html_token_buffer_new (size);
	p->token_buffers = g_list_append (p->token_buffers, nb);

	/* this one is now write_buf */
	p->write_buf = nb;

	/* if we don't have read_buf already set it to this one */
	if (p->read_buf == NULL) {
		p->read_buf = nb;
		p->read_cur = p->token_buffers;
	}
}

/* EP CHECK: OK.  */
static void
html_tokenizer_add_pending (HTMLTokenizer *t)
{
	struct _HTMLTokenizerPrivate *p = t->priv;

	if (p->tag || p->select) {
		add_unichar (t, ' ');
	}
	else if (p->textarea) {
		if (p->pending == LFPending)
			add_unichar (t, '\n');
		else
			add_unichar (t, ' ');
	}
	else if (p->pre) {
		switch (p->pending) {
		case SpacePending:
			add_unichar (t, ' ');
			break;
		case LFPending:
			if (p->dest > p->buffer) {
				html_tokenizer_append_token (t, p->buffer, p->dest - p->buffer);
			}
			p->dest = p->buffer;
			add_unichar (t, TAG_ESCAPE);
			add_unichar (t, '\n');
			html_tokenizer_append_token (t, p->buffer, 2);
			p->dest = p->buffer;
			break;
		case TabPending:
			add_unichar (t, '\t');
			break;
		default:
			g_warning ("Unknown pending type: %d\n", (gint) p->pending);
			break;
		}
	}
	else {
		add_unichar (t, ' ');
	}

	p->pending = NonePending;
}

static void
prepare_enough_space (HTMLTokenizer *t)
{
	struct _HTMLTokenizerPrivate *p = t->priv;

	if ((p->dest - p->buffer + 32) > p->size) {
		guint off = p->dest - p->buffer;

		p->size  += (p->size >> 2) + 32;
		p->buffer = g_realloc (p->buffer, p->size);
		p->dest   = p->buffer + off;
	}
}

static void
in_comment (HTMLTokenizer *t, const gchar **src)
{
	struct _HTMLTokenizerPrivate *p = t->priv;

	if (**src == '-') {	             /* Look for "-->" */
		if (p->searchCount < 2)
			p->searchCount++;
	} else if (p->searchCount == 2 && (**src == '>')) {
		p->comment = FALSE;          /* We've got a "-->" sequence */
	} else if (tolower (**src) == gtkhtmlStart [p->searchGtkHTMLCount]) {
		if (p->searchGtkHTMLCount == 8) {
			p->extension    = TRUE;
			p->comment = FALSE;
			p->searchCount = 0;
			p->searchExtensionEndCount = 0;
			p->searchGtkHTMLCount = 0;
		} else
			p->searchGtkHTMLCount ++;
	} else {
		p->searchGtkHTMLCount = 0;
		if (p->searchCount < 2)
			p->searchCount = 0;
	}

	(*src)++;
}

static inline void
extension_one_char (HTMLTokenizer *t, const gchar **src)
{
	struct _HTMLTokenizerPrivate *p = t->priv;

	p->extension = FALSE;
	html_tokenizer_tokenize_one_char (t, src);
	p->extension = TRUE;
}

static void
in_extension (HTMLTokenizer *t, const gchar **src)
{
	struct _HTMLTokenizerPrivate *p = t->priv;

	/* check for "-->" */
	if (!p->tquote && **src == '-') {
		if (p->searchExtensionEndCount < 2)
			p->searchExtensionEndCount ++;
		(*src) ++;
	} else if (!p->tquote && p->searchExtensionEndCount == 2 && **src == '>') {
		p->extension = FALSE;
		(*src) ++;
	} else {
		if (p->searchExtensionEndCount > 0) {
			if (p->extension) {
				const gchar *c = "-->";

				while (p->searchExtensionEndCount) {
					extension_one_char (t, &c);
					p->searchExtensionEndCount --;
				}
			}
		}
		extension_one_char (t, src);
	}
}

static void
in_script_or_style (HTMLTokenizer *t, const gchar **src)
{
	struct _HTMLTokenizerPrivate *p = t->priv;

	/* Allocate memory to store the script or style */
	if (p->scriptCodeSize + 11 > p->scriptCodeMaxSize)
		p->scriptCode = g_realloc (p->scriptCode, p->scriptCodeMaxSize += 1024);

	if ((**src == '>' ) && ( p->searchFor [p->searchCount] == '>')) {
		(*src)++;
		p->scriptCode [p->scriptCodeSize] = 0;
		p->scriptCode [p->scriptCodeSize + 1] = 0;
		if (p->script) {
			p->script = FALSE;
		}
		else {
			p->style = FALSE;
		}
		g_free (p->scriptCode);
		p->scriptCode = NULL;
	}
	/* Check if a </script> tag is on its way */
	else if (p->searchCount > 0) {
		gboolean put_to_script = FALSE;
		if (tolower (**src) == p->searchFor [p->searchCount]) {
			p->searchBuffer [p->searchCount] = **src;
			p->searchCount++;
			(*src)++;
		}
		else if (p->searchFor [p->searchCount] == '>') {
			/* There can be any number of white-space characters between
			   tag name and closing '>' so try to move through them, if possible */

			const gchar **p = src;
			while (isspace (**p))
				(*p)++;

			
			if (**p == '>')
				*src = *p;
			else
				put_to_script = TRUE;
		}
		else 
			put_to_script = TRUE;

		if (put_to_script) {
			gchar *c;

			p->searchBuffer [p->searchCount] = 0;
			c = p->searchBuffer;
			while (*c)
				p->scriptCode [p->scriptCodeSize++] = *c++;
			p->scriptCode [p->scriptCodeSize] = **src; (*src)++;
			p->searchCount = 0;
		}
	}
	else if (**src == '<') {
		p->searchCount = 1;
		p->searchBuffer [0] = '<';
		(*src)++;
	}
	else {
		p->scriptCode [p->scriptCodeSize] = **src;
		(*src)++;
	}
}

static gunichar win1252_to_unicode [32] = {
	0x20ac,
	0x81,
	0x201a,
	0x0192,
	0x201e,
	0x2026,
	0x2020,
	0x2021,
	0x02c6,
	0x2030,
	0x0160,
	0x2039,
	0x0152,
	0x8d,
	0x017d,
	0x8f,
	0x90,
	0x2018,
	0x2019,
	0x201c,
	0x201d,
	0x2022,
	0x2013,
	0x2014,
	0x02dc,
	0x2122,
	0x0161,
	0x203a,
	0x0153,
	0x9d,
	0x017e,
	0x0178
};

static void
add_unichar (HTMLTokenizer *t, gunichar wc)
{
	struct _HTMLTokenizerPrivate *p = t->priv;

	p->utf8_length = 0;

	/*
	  chars in range 128 - 159 are control characters in unicode,
	  but most browsers treat them as windows 1252
	  encoded characters and translate them to unicode
	  it's broken, but we do the same here
	*/
	if (wc > 127 && wc < 160)
		wc = win1252_to_unicode [wc - 128];

	if (wc != '\0') {
		p->dest += g_unichar_to_utf8 (wc, p->dest);
		*(p->dest) = 0;
	}
}

static void
add_byte (HTMLTokenizer *t, const gchar **src)
{
	gunichar wc;
	struct _HTMLTokenizerPrivate *p = t->priv;

	if (p->utf8) {
		p->utf8_buffer[p->utf8_length] = **src;
		p->utf8_length++;

		wc = g_utf8_get_char_validated ((const gchar *)p->utf8_buffer, p->utf8_length);
		if (wc == -1 || p->utf8_length >= (sizeof(p->utf8_buffer)/sizeof(p->utf8_buffer[0]))) {
			add_unichar (t, INVALID_CHARACTER_MARKER);
			(*src)++;
			return;
		} else if (wc == -2) {
			/* incomplete character check again */
			(*src)++;
			return;
		}
	} else {
		wc = (guchar)**src;
	}

	add_unichar (t, wc);
	(*src)++;
}

static void
flush_entity (HTMLTokenizer *t)
{
	struct _HTMLTokenizerPrivate *p = t->priv;
	/* ignore the TAG_ESCAPE when flushing */
	const char *str = p->searchBuffer + 1;

	 while (p->searchCount--) {
		add_byte (t, &str);
	}
}

static gboolean
add_unichar_validated (HTMLTokenizer *t, gunichar uc)
{
	if (g_unichar_validate (uc)) {
		add_unichar (t, uc);
		return TRUE;
	}

	g_warning ("invalid character value: x%xd", uc);
	return FALSE;
}

static void
in_entity (HTMLTokenizer *t, const gchar **src)
{
	struct _HTMLTokenizerPrivate *p = t->priv;
	gunichar entityValue = 0;

	/* See http://www.mozilla.org/newlayout/testcases/layout/entities.html for a complete entity list,
	   ftp://ftp.unicode.org/Public/MAPPINGS/ISO8859/8859-1.TXT
	   (or 'man iso_8859_1') for the character encodings. */

	p->searchBuffer [p->searchCount + 1] = **src;
	p->searchBuffer [p->searchCount + 2] = '\0';

	/* Check for &#0000 sequence */
	if (p->searchBuffer[2] == '#') {
		if ((p->searchCount > 1) &&
		    (!isdigit (**src)) &&
		    (p->searchBuffer[3] != 'x')) {
			/* &#123 */
			p->searchBuffer [p->searchCount + 1] = '\0';
			entityValue = strtoul (&(p->searchBuffer [3]),
					       NULL, 10);
			p->charEntity = FALSE;
		}
		if ((p->searchCount > 1) &&
		    (!isalnum (**src)) &&
		    (p->searchBuffer[3] == 'x')) {
			/* &x12AB */
			p->searchBuffer [p->searchCount + 1] = '\0';

			entityValue = strtoul (&(p->searchBuffer [4]),
					       NULL, 16);
			p->charEntity = FALSE;
		}
	}
	else {
		/* Check for &abc12 sequence */
		if (!isalnum (**src)) {
			p->charEntity = FALSE;
			if ((p->searchBuffer [p->searchCount + 1] == ';') ||
			    (!p->tag)) {
				char *ename = p->searchBuffer + 2;

				p->searchBuffer [p->searchCount + 1] = '\0'; /* FIXME sucks */
				entityValue = html_entity_parse (ename, 0);
			}
		}

	}

	if (p->searchCount > 13) {
		/* Ignore this sequence since it's too long */
		p->charEntity = FALSE;
		flush_entity (t);
	}
	else if (p->charEntity) {
				/* Keep searching for end of character entity */
		p->searchCount++;
		(*src)++;
	}
	else {
		/*
		 * my reading of http://www.w3.org/TR/html4/intro/sgmltut.html#h-3.2.2 makes
		 * seem correct to always collapse entity references, even in element names
		 * and attributes.
		 */
		if (entityValue) {
			if (entityValue != TAG_ESCAPE)
				/* make sure the entity value is a valid character value */
				if (!add_unichar_validated (t, entityValue))
					add_unichar (t, INVALID_CHARACTER_MARKER);

			if (**src == ';')
				(*src)++;
		} else {
			/* Ignore the sequence, just add it as plaintext */
			flush_entity (t);
		}
	}
}

static void
in_tag (HTMLTokenizer *t, const gchar **src)
{
	struct _HTMLTokenizerPrivate *p = t->priv;

	p->startTag = FALSE;
	if (**src == '/') {
		if (p->pending == LFPending  && !p->pre) {
			p->pending = NonePending;
		}
	}
	else if (((**src >= 'a') && (**src <= 'z'))
		 || ((**src >= 'A') && (**src <= 'Z'))) {
				/* Start of a start tag */
	}
	else if (**src == '!') {
				/* <!-- comment --> */
	}
	else if (**src == '?') {
				/* <? meta ?> */
	}
	else {
				/* Invalid tag, just add it */
		if (p->pending)
			html_tokenizer_add_pending (t);
		add_unichar (t, '<');
		add_byte (t, src);
		return;
	}

	if (p->pending)
		html_tokenizer_add_pending (t);

	if (p->dest > p->buffer) {
		html_tokenizer_append_token (t, p->buffer, p->dest - p->buffer);
		p->dest = p->buffer;
	}
	add_unichar (t, TAG_ESCAPE);
	add_unichar (t, '<');
	p->tag = TRUE;
	p->searchCount = 1; /* Look for <!-- to start comment */
}

static void
start_entity (HTMLTokenizer *t, const gchar **src)
{
	struct _HTMLTokenizerPrivate *p = t->priv;

	(*src)++;

	p->discard = NoneDiscard;

	if (p->pending)
		html_tokenizer_add_pending (t);

	p->charEntity      = TRUE;
	p->searchBuffer[0] = TAG_ESCAPE;
	p->searchBuffer[1] = '&';
	p->searchCount     = 1;
}

static void
start_tag (HTMLTokenizer *t, const gchar **src)
{
	(*src)++;
	t->priv->startTag = TRUE;
	t->priv->discard  = NoneDiscard;
}

static void
end_tag (HTMLTokenizer *t, const gchar **src)
{
	struct _HTMLTokenizerPrivate *p = t->priv;
	gchar *ptr;

	p->searchCount = 0; /* Stop looking for <!-- sequence */

	add_unichar (t, '>');

	/* Make the tag lower case */
	ptr = p->buffer + 2;
	if (p->pre || *ptr == '/') {
	    	/* End tag */
		p->discard = NoneDiscard;
	}
	else {
		/* Start tag */
		/* Ignore CRLFs after a start tag */
		p->discard = LFDiscard;
	}

	while (*ptr && *ptr !=' ') {
		*ptr = tolower (*ptr);
		ptr++;
	}
	html_tokenizer_append_token (t, p->buffer, p->dest - p->buffer);
	p->dest = p->buffer;

	p->tag = FALSE;
	p->aTag = FALSE;
	p->pending = NonePending;
	(*src)++;

	if (strncmp (p->buffer + 2, "pre", 3) == 0) {
		p->pre++;
	}
	else if (strncmp (p->buffer + 2, "/pre", 4) == 0) {
		p->pre--;
	}
	else if (strncmp (p->buffer + 2, "textarea", 8) == 0) {
		p->textarea = TRUE;
	}
	else if (strncmp (p->buffer + 2, "/textarea", 9) == 0) {
		p->textarea = FALSE;
	}
	else if (strncmp (p->buffer + 2, "title", 5) == 0) {
		p->title = TRUE;
	}
	else if (strncmp (p->buffer + 2, "/title", 6) == 0) {
		p->title = FALSE;
	}
	else if (strncmp (p->buffer + 2, "script", 6) == 0) {
		p->script = TRUE;
		p->searchCount = 0;
		p->searchFor = scriptEnd;
		p->scriptCode = g_malloc (1024);
		p->scriptCodeSize = 0;
		p->scriptCodeMaxSize = 1024;
	}
	else if (strncmp (p->buffer + 2, "style", 5) == 0) {
		p->style = TRUE;
		p->searchCount = 0;
		p->searchFor = styleEnd;
		p->scriptCode = g_malloc (1024);
		p->scriptCodeSize = 0;
		p->scriptCodeMaxSize = 1024;
	}
	else if (strncmp (p->buffer + 2, "select", 6) == 0) {
		p->select = TRUE;
	}
	else if (strncmp (p->buffer + 2, "/select", 7) == 0) {
		p->select = FALSE;
	}
	else if (strncmp (p->buffer + 2, "tablesdkl", 9) == 0) {
		html_tokenizer_blocking_push (t, Table);
	}
	else {
		if (p->blocking) {
			const gchar *bn = html_tokenizer_blocking_get_name (t);

			if (strncmp (p->buffer + 1, bn, strlen (bn)) == 0) {
				html_tokenizer_blocking_pop (t);
			}
		}
	}
}

static void
in_crlf (HTMLTokenizer *t, const gchar **src)
{
	struct _HTMLTokenizerPrivate *p = t->priv;

	if (p->tquote) {
		if (p->discard == NoneDiscard)
			p->pending = SpacePending;
	}
	else if (p->tag) {
		p->searchCount = 0; /* Stop looking for <!-- sequence */
		if (p->discard == NoneDiscard)
			p->pending = SpacePending; /* Treat LFs inside tags as spaces */
	}
	else if (p->pre || p->textarea) {
		if (p->discard == LFDiscard) {
			/* Ignore this LF */
			p->discard = NoneDiscard; /*  We have discarded 1 LF */
		} else {
			/* Process this LF */
			if (p->pending)
				html_tokenizer_add_pending (t);
			p->pending = LFPending;
		}
	}
	else {
		if (p->discard == LFDiscard) {
			/* Ignore this LF */
			p->discard = NoneDiscard; /* We have discarded 1 LF */
		} else {
			/* Process this LF */
			if (p->pending == NonePending)
				p->pending = LFPending;
		}
	}
	/* Check for MS-DOS CRLF sequence */
	if (**src == '\r') {
		p->skipLF = TRUE;
	}
	(*src)++;
}

static void
in_space_or_tab (HTMLTokenizer *t, const gchar **src)
{
	gchar *ptr;

	if (t->priv->tquote) {
		if (t->priv->discard == NoneDiscard)
			t->priv->pending = SpacePending;
	}
	else if (t->priv->tag) {
		t->priv->searchCount = 0; /* Stop looking for <!-- sequence */
		if (t->priv->discard == NoneDiscard)
			t->priv->pending = SpacePending;
		ptr = t->priv->buffer;
		if (strlen (ptr) == 3 && ptr[1] == '<' && ptr[2] == 'a')
			t->priv->aTag = TRUE;
	}
	else if (t->priv->pre || t->priv->textarea) {
		if (t->priv->pending)
			html_tokenizer_add_pending (t);
		if (**src == ' ')
			t->priv->pending = SpacePending;
		else
			t->priv->pending = TabPending;
	}
	else {
		t->priv->pending = SpacePending;
	}
	(*src)++;
}

static void
in_quoted (HTMLTokenizer *t, const gchar **src)
{
	/* We treat ' and " the same in tags " */
	t->priv->discard = NoneDiscard;
	if (t->priv->tag) {
		t->priv->searchCount = 0; /* Stop looking for <!-- sequence */
		if ((t->priv->tquote == SINGLE_QUOTE && **src == '\"') /* match " */
		    || (t->priv->tquote == DOUBLE_QUOTE && **src == '\'')) {
			add_unichar (t, **src);
			(*src)++;
		} else if (*(t->priv->dest-1) == '=' && !t->priv->tquote) {
			t->priv->discard = SpaceDiscard;
			t->priv->pending = NonePending;

			if (**src == '\"') /* match " */
				t->priv->tquote = DOUBLE_QUOTE;
			else
				t->priv->tquote = SINGLE_QUOTE;
			add_unichar (t, **src);
			(*src)++;
		}
		else if (t->priv->tquote) {
			t->priv->tquote = NO_QUOTE;
			add_byte (t, src);
			t->priv->pending = SpacePending;
		}
		else {
			/* Ignore stray "\'" */
			(*src)++;
		}
	}
	else {
		if (t->priv->pending)
			html_tokenizer_add_pending (t);

		add_byte (t, src);
	}
}

static void
in_assignment (HTMLTokenizer *t, const gchar **src)
{
	t->priv->discard = NoneDiscard;
	if (t->priv->tag) {
		t->priv->searchCount = 0; /* Stop looking for <!-- sequence */
		add_unichar (t, '=');
		if (!t->priv->tquote) {
			t->priv->pending = NonePending;
			t->priv->discard = SpaceDiscard;
		}
	}
	else {
		if (t->priv->pending)
			html_tokenizer_add_pending (t);

		add_unichar (t, '=');
	}
	(*src)++;
}

inline static void
in_plain (HTMLTokenizer *t, const gchar **src)
{
	struct _HTMLTokenizerPrivate *p = t->priv;

	p->discard = NoneDiscard;
	if (p->pending)
		html_tokenizer_add_pending (t);

	if (p->tag) {
		if (p->searchCount > 0) {
			if (**src == commentStart[p->searchCount]) {
				p->searchCount++;
				if (p->searchCount == 4) {
					/* Found <!-- sequence */
					p->comment = TRUE;
					p->dest = p->buffer;
					p->tag = FALSE;
					p->searchCount = 0;
					return;
				}
			}
			else {
				p->searchCount = 0; /* Stop lookinf for <!-- sequence */
			}
		}
	}

	add_byte (t, src);
}

static void
html_tokenizer_tokenize_one_char (HTMLTokenizer *t, const gchar **src)
{
	struct _HTMLTokenizerPrivate *p = t->priv;

	prepare_enough_space (t);

	if (p->skipLF && **src != '\n')
		p->skipLF = FALSE;

	if (p->skipLF)
		(*src) ++;
	else if (p->comment)
		in_comment (t, src);
	else if (p->extension)
		in_extension (t, src);
	else if (p->script || p->style)
		in_script_or_style (t, src);
	else if (p->charEntity)
		in_entity (t, src);
	else if (p->startTag)
		in_tag (t, src);
	else if (**src == '&' && !p->aTag)
		start_entity (t, src);
	else if (**src == '<' && !p->tag)
		start_tag (t, src);
	else if (**src == '>' && p->tag && !p->tquote)
		end_tag (t, src);
	else if ((**src == '\n') || (**src == '\r'))
		in_crlf (t, src);
	else if ((**src == ' ') || (**src == '\t'))
		in_space_or_tab (t, src);
	else if (**src == '\"' || **src == '\'') /* match " ' */
		in_quoted (t, src);
	else if (**src == '=')
		in_assignment (t, src);
	else
		in_plain (t, src);
}

static void
html_tokenizer_real_write (HTMLTokenizer *t, const gchar *string, size_t size)
{
	const gchar *src = string;

	while ((src - string) < size)
		html_tokenizer_tokenize_one_char (t, &src);
}

static gchar *
html_tokenizer_blocking_get_name (HTMLTokenizer *t)
{
	switch (GPOINTER_TO_INT (t->priv->blocking->data)) {
	case Table:
		return "</tabledkdk";
	}

	return "";
}

static void
html_tokenizer_blocking_push (HTMLTokenizer *t, HTMLTokenType tt)
{
	struct _HTMLTokenizerPrivate *p = t->priv;

	/* block tokenizer - we must block last token in buffers as it was already added */
	if (!p->blocking) {
		p->tokens_num--;
		p->blocking_tokens_num++;
	}
	p->blocking = g_list_prepend (p->blocking, GINT_TO_POINTER (tt));
}

static void
html_tokenizer_blocking_pop (HTMLTokenizer *t)
{
	struct _HTMLTokenizerPrivate *p = t->priv;

	p->blocking = g_list_remove (p->blocking, p->blocking->data);

	/* unblock tokenizer */
	if (!p->blocking) {
		p->tokens_num += p->blocking_tokens_num;
		p->blocking_tokens_num = 0;
	}
}

/** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **/

void
html_tokenizer_begin (HTMLTokenizer *t, gchar *content_type)
{
	g_return_if_fail (t && HTML_IS_TOKENIZER (t));

	g_signal_emit (t, html_tokenizer_signals [HTML_TOKENIZER_BEGIN_SIGNAL], 0, content_type);
}

void
html_tokenizer_end (HTMLTokenizer *t)
{
	g_return_if_fail (t && HTML_IS_TOKENIZER (t));

	g_signal_emit (t, html_tokenizer_signals[HTML_TOKENIZER_END_SIGNAL], 0);
}

void
html_tokenizer_write (HTMLTokenizer *t, const gchar *str, size_t size)
{
	HTMLTokenizerClass *klass;

	g_return_if_fail (t && HTML_IS_TOKENIZER (t));
	klass = HTML_TOKENIZER_CLASS (G_OBJECT_GET_CLASS (t));

	if (klass->write)
		klass->write (t, str, size);
	else
		g_warning ("No write method defined.");
}

gchar *
html_tokenizer_peek_token (HTMLTokenizer *t)
{
	HTMLTokenizerClass *klass;

	g_return_val_if_fail (t && HTML_IS_TOKENIZER (t), NULL);

	klass = HTML_TOKENIZER_CLASS (G_OBJECT_GET_CLASS (t));

	if (klass->peek_token)
		return klass->peek_token (t);

	g_warning ("No peek_token method defined.");
	return NULL;

}

gchar *
html_tokenizer_next_token (HTMLTokenizer *t)
{
	HTMLTokenizerClass *klass;

	g_return_val_if_fail (t && HTML_IS_TOKENIZER (t), NULL);

	klass = HTML_TOKENIZER_CLASS (G_OBJECT_GET_CLASS (t));

	if (klass->next_token)
		return klass->next_token (t);

	g_warning ("No next_token method defined.");
	return NULL;
}

gboolean
html_tokenizer_has_more_tokens (HTMLTokenizer *t)
{
	HTMLTokenizerClass *klass;

	g_return_val_if_fail (t && HTML_IS_TOKENIZER (t), FALSE);

	klass = HTML_TOKENIZER_CLASS (G_OBJECT_GET_CLASS (t));

	if (klass->has_more) {
		return klass->has_more (t);
	}

	g_warning ("No has_more method defined.");
	return FALSE;

}

HTMLTokenizer *
html_tokenizer_clone (HTMLTokenizer *t)
{
	HTMLTokenizerClass *klass;

	if (t == NULL)
		return NULL;
	g_return_val_if_fail (HTML_IS_TOKENIZER (t), NULL);

	klass = HTML_TOKENIZER_CLASS (G_OBJECT_GET_CLASS (t));

	if (klass->clone)
		return klass->clone (t);

	g_warning ("No clone method defined.");
	return NULL;
}
