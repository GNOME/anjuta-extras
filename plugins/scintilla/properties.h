/*
 * properties.h Copyright (C) 2000  Kh. Naba Kumar Singh
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _PROPERTIES_H_
#define _PROPERTIES_H_


#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Editor preferences */
#define DISABLE_SYNTAX_HILIGHTING  "disable-syntax-hilighting"
/*
#define INDENT_AUTOMATIC           "indent-automatic"
*/
#define BRACES_CHECK               "braces-check"
#define DOS_EOL_CHECK              "editor-doseol"
#define WRAP_BOOKMARKS             "editor-wrapbookmarks"
/*
#define INDENT_OPENING             "indent-opening"
#define INDENT_CLOSING             "indent-closing"
*/
#define INDENT_MAINTAIN            "indent-maintain"

#define TAB_INDENTS                "tab-indents"
#define BACKSPACE_UNINDENTS        "backspace-unindents"

#define FOLD_SYMBOLS               "fold-symbols"
#define FOLD_UNDERLINE             "fold-underline"

#define STRIP_TRAILING_SPACES      "strip-trailing-spaces"
#define FOLD_ON_OPEN               "fold-on-open"
#define CARET_FORE_COLOR           "caret.fore"
#define CALLTIP_BACK_COLOR         "calltip.back"
#define SELECTION_FORE_COLOR       "selection.fore"
#define SELECTION_BACK_COLOR       "selection.back"

#define VIEW_LINENUMBERS_MARGIN    "margin-linenumber-visible"
#define VIEW_MARKER_MARGIN         "margin-marker-visible"
#define VIEW_FOLD_MARGIN           "margin-fold-visible"
#define VIEW_INDENTATION_GUIDES    "view-indentation-guides"
#define VIEW_WHITE_SPACES          "view-whitespace"
#define VIEW_EOL                   "view-eol"
#define VIEW_LINE_WRAP             "view-line-wrap"
#define EDGE_COLUMN                "edge-column"


typedef gint PropsID;

PropsID sci_prop_set_new (void);
void sci_prop_set_destroy (PropsID p);
gpointer sci_prop_get_pointer (PropsID p);

void sci_prop_set_with_key (PropsID p, const gchar *key, const gchar *val);
void sci_prop_set_int_with_key (PropsID p, const gchar *key, int val);
void sci_prop_set (PropsID p, const gchar *keyval);
gchar* sci_prop_get (PropsID p, const gchar *key);
gchar* sci_prop_get_expanded (PropsID p, const gchar *key);
gchar* sci_prop_expand (PropsID p, const gchar *withvars);
int sci_prop_get_int (PropsID p, const gchar *key, gint defaultValue);
gchar* sci_prop_get_wild (PropsID p, const gchar *keybase, const gchar *filename);
gchar* sci_prop_get_new_expand (PropsID p, const gchar *keybase, const gchar *filename);
void sci_prop_clear (PropsID p);
void sci_prop_read_from_memory (PropsID p, const gchar *data,
							gint len, const gchar *directoryForImports);
void sci_prop_read (PropsID p, const gchar *filename, const gchar *directoryForImports);
void sci_prop_set_parent (PropsID p1, PropsID p2);
GList* sci_prop_glist_from_data (guint props, const gchar* id);

#ifdef __cplusplus
}
#endif

#endif /* _PROPERTIES_H_ */
