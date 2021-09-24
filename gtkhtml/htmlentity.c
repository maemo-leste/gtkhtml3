/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* htmlentity.c
 *
 * This file is part of the GtkHTML library.
 *
 * Copyright (C) 1999  Helix Code, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Ettore Perazzoli
 */

#include <config.h>
#include <string.h>
#include <stdlib.h>
#include "gtkhtml-compat.h"

#include <glib.h>
#include "htmlentity.h"


struct _EntityEntry {
	guint value;
	const gchar *str;
};
typedef struct _EntityEntry EntityEntry;

static EntityEntry entity_table[] = {

	/* Latin1 */
	{ 160,	"nbsp" },
	{ 161,	"iexcl" },
	{ 162,	"cent" },
	{ 163,	"pound" },
	{ 164,	"curren" },
	{ 165,	"yen" },
	{ 166,	"brvbar" },
	{ 167,	"sect" },
	{ 168,	"uml" },
	{ 169,	"copy" },
	{ 170,	"ordf" },
	{ 171,	"laquo" },
	{ 172,	"not" },
	{ 173,	"shy" },
	{ 174,	"reg" },
	{ 175,	"macr" },
	{ 176,	"deg" },
	{ 177,	"plusmn" },
	{ 178,	"sup2" },
	{ 179,	"sup3" },
	{ 180,	"acute" },
	{ 181,	"micro" },
	{ 182,	"para" },
	{ 183,	"middot" },
	{ 184,	"cedil" },
	{ 185,	"sup1" },
	{ 186,	"ordm" },
	{ 187,	"raquo" },
	{ 188,	"frac14" },
	{ 189,	"frac12" },
	{ 190,	"frac34" },
	{ 191,	"iquest" },
	{ 192,	"Agrave" },
	{ 193,	"Aacute" },
	{ 194,	"Acirc" },
	{ 195,	"Atilde" },
	{ 196,	"Auml" },
	{ 197,	"Aring" },
	{ 198,	"AElig" },
	{ 199,	"Ccedil" },
	{ 200,	"Egrave" },
	{ 201,	"Eacute" },
	{ 202,	"Ecirc" },
	{ 203,	"Euml" },
	{ 204,	"Igrave" },
	{ 205,	"Iacute" },
	{ 206,	"Icirc" },
	{ 207,	"Iuml" },
	{ 208,	"ETH" },
	{ 209,	"Ntilde" },
	{ 210,	"Ograve" },
	{ 211,	"Oacute" },
	{ 212,	"Ocirc" },
	{ 213,	"Otilde" },
	{ 214,	"Ouml" },
	{ 215,	"times" },
	{ 216,	"Oslash" },
	{ 217,	"Ugrave" },
	{ 218,	"Uacute" },
	{ 219,	"Ucirc" },
	{ 220,	"Uuml" },
	{ 221,	"Yacute" },
	{ 222,	"THORN" },
	{ 223,	"szlig" },
	{ 224,	"agrave" },
	{ 225,	"aacute" },
	{ 226,	"acirc" },
	{ 227,	"atilde" },
	{ 228,	"auml" },
	{ 229,	"aring" },
	{ 230,	"aelig" },
	{ 231,	"ccedil" },
	{ 232,	"egrave" },
	{ 233,	"eacute" },
	{ 234,	"ecirc" },
	{ 235,	"euml" },
	{ 236,	"igrave" },
	{ 237,	"iacute" },
	{ 238,	"icirc" },
	{ 239,	"iuml" },
	{ 240,	"eth" },
	{ 241,	"ntilde" },
	{ 242,	"ograve" },
	{ 243,	"oacute" },
	{ 244,	"ocirc" },
	{ 245,	"otilde" },
	{ 246,	"ouml" },
	{ 247,	"divide" },
	{ 248,	"oslash" },
	{ 249,	"ugrave" },
	{ 250,	"uacute" },
	{ 251,	"ucirc" },
	{ 252,	"uuml" },
	{ 253,	"yacute" },
	{ 254,	"thorn" },
	{ 255,	"yuml" },

	/* special charactes */
	{ 34,    "quot" },
	{ 38,    "amp" },
	{ 39,    "apos" },
	{ 60,    "lt" },
	{ 62,    "gt" },
	{ 338,   "OElig" },
	{ 339,   "oelig" },
	{ 352,   "Scaron" },
	{ 353,   "scaron" },
	{ 376,   "Yuml" },
	{ 710,   "circ" },
	{ 732,   "tilde" },
	{ 8194,  "ensp" },
	{ 8195,  "emsp" },
	{ 8201,  "thinsp" },
	{ 8204,  "zwnj" },
	{ 8205,  "zwj" },
	{ 8206,  "lrm" },
	{ 8207,  "rlm" },
	{ 8211,  "ndash" },
	{ 8212,  "mdash" },
	{ 8216,  "lsquo" },
	{ 8217,  "rsquo" },
	{ 8218,  "sbquo" },
	{ 8220,  "ldquo" },
	{ 8221,  "rdquo" },
	{ 8222,  "bdquo" },
	{ 8224,  "dagger" },
	{ 8225,  "Dagger" },
	{ 8240,  "permil" },
	{ 8249,  "lsaquo" },
	{ 8250,  "rsaquo" },
	{ 8364,  "euro" },

	/* symbols */
	{ 402,   "fnof" },
	{ 913,   "Alpha" },
	{ 914,   "Beta" },
	{ 915,   "Gamma" },
	{ 916,   "Delta" },
	{ 917,   "Epsilon" },
	{ 918,   "Zeta" },
	{ 919,   "Eta" },
	{ 920,   "Theta" },
	{ 921,   "Iota" },
	{ 922,   "Kappa" },
	{ 923,   "Lambda" },
	{ 924,   "Mu" },
	{ 925,   "Nu" },
	{ 926,   "Xi" },
	{ 927,   "Omicron" },
	{ 928,   "Pi" },
	{ 929,   "Rho" },
	{ 931,   "Sigma" },
	{ 932,   "Tau" },
	{ 933,   "Upsilon" },
	{ 934,   "Phi" },
	{ 935,   "Chi" },
	{ 936,   "Psi" },
	{ 937,   "Omega" },
	{ 945,   "alpha" },
	{ 946,   "beta" },
	{ 947,   "gamma" },
	{ 948,   "delta" },
	{ 949,   "epsilon" },
	{ 950,   "zeta" },
	{ 951,   "eta" },
	{ 952,   "theta" },
	{ 953,   "iota" },
	{ 954,   "kappa" },
	{ 955,   "lambda" },
	{ 956,   "mu" },
	{ 957,   "nu" },
	{ 958,   "xi" },
	{ 959,   "omicron" },
	{ 960,   "pi" },
	{ 961,   "rho" },
	{ 962,   "sigmaf" },
	{ 963,   "sigma" },
	{ 964,   "tau" },
	{ 965,   "upsilon" },
	{ 966,   "phi" },
	{ 967,   "chi" },
	{ 968,   "psi" },
	{ 969,   "omega" },
	{ 977,   "thetasym" },
	{ 978,   "upsih" },
	{ 982,   "piv" },
	{ 8226,  "bull" },
	{ 8230,  "hellip" },
	{ 8242,  "prime" },
	{ 8243,  "Prime" },
	{ 8254,  "oline" },
	{ 8260,  "frasl" },
	{ 8472,  "weierp" },
	{ 8465,  "image" },
	{ 8476,  "real" },
	{ 8482,  "trade" },
	{ 8501,  "alefsym" },
	{ 8592,  "larr" },
	{ 8593,  "uarr" },
	{ 8594,  "rarr" },
	{ 8595,  "darr" },
	{ 8596,  "harr" },
	{ 8629,  "crarr" },
	{ 8656,  "lArr" },
	{ 8657,  "uArr" },
	{ 8658,  "rArr" },
	{ 8659,  "dArr" },
	{ 8660,  "hArr" },
	{ 8704,  "forall" },
	{ 8706,  "part" },
	{ 8707,  "exist" },
	{ 8709,  "empty" },
	{ 8711,  "nabla" },
	{ 8712,  "isin" },
	{ 8713,  "notin" },
	{ 8715,  "ni" },
	{ 8719,  "prod" },
	{ 8721,  "sum" },
	{ 8722,  "minus" },
	{ 8727,  "lowast" },
	{ 8730,  "radic" },
	{ 8733,  "prop" },
	{ 8734,  "infin" },
	{ 8736,  "ang" },
	{ 8743,  "and" },
	{ 8744,  "or" },
	{ 8745,  "cap" },
	{ 8746,  "cup" },
	{ 8747,  "int" },
	{ 8756,  "there4" },
	{ 8764,  "sim" },
	{ 8773,  "cong" },
	{ 8776,  "asymp" },
	{ 8800,  "ne" },
	{ 8801,  "equiv" },
	{ 8804,  "le" },
	{ 8805,  "ge" },
	{ 8834,  "sub" },
	{ 8835,  "sup" },
	{ 8836,  "nsub" },
	{ 8838,  "sube" },
	{ 8839,  "supe" },
	{ 8853,  "oplus" },
	{ 8855,  "otimes" },
	{ 8869,  "perp" },
	{ 8901,  "sdot" },
	{ 8968,  "lceil" },
	{ 8969,  "rceil" },
	{ 8970,  "lfloor" },
	{ 8971,  "rfloor" },
	{ 9001,  "lang" },
	{ 9002,  "rang" },
	{ 9674,  "loz" },
	{ 9824,  "spades" },
	{ 9827,  "clubs" },
	{ 9829,  "hearts" },
	{ 9830,  "diams" },
};


/* FIXME FIXME this function just sucks.  We should use gperf or something instead.  */

static gint
html_g_str_case_equal (gconstpointer v, gconstpointer v2)
{
	return g_ascii_strcasecmp ((const gchar*) v, (const gchar*)v2) == 0;
}

gulong
html_entity_parse (const gchar *s, guint len)
{
	static GHashTable *ehash = NULL;
	gchar *t;

	if (!ehash) {
		gint i;

		ehash = g_hash_table_new (g_str_hash, html_g_str_case_equal);

		for (i = 0; i < sizeof (entity_table) / sizeof (entity_table[0]); i++)
			g_hash_table_insert (ehash, (gpointer) entity_table[i].str, GINT_TO_POINTER (entity_table[i].value));
	}

	if (len > 0) {
		t = alloca (len + 1);
		memcpy (t, s, len);
		*(t + len) = '\0';
	} else {
		t = (gchar *) s;
	}

	return GPOINTER_TO_INT (g_hash_table_lookup (ehash, t));
}
