/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Jeffrey Stedfast <fejj@ximian.com>
 *
 *  Copyright 2003 Ximian, Inc. (www.ximian.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <limits.h>

#include <gconf/gconf-client.h>
#include <glib/gi18n.h>

#include "vgmemcheckprefs.h"


#define LEAK_CHECK_KEY             "/apps/anjuta/valgrind/memcheck/leak-check"
#define SHOW_REACHABLE_KEY         "/apps/anjuta/valgrind/memcheck/show-reachable"
#define LEAK_RESOLUTION_KEY        "/apps/anjuta/valgrind/memcheck/leak-resolution"
#define FREELIST_VOL_KEY           "/apps/anjuta/valgrind/memcheck/freelist-vol"
#define WORKAROUND_GCC296_BUGS_KEY "/apps/anjuta/valgrind/memcheck/workaround-gcc296-bugs"
#define AVOID_STRLEN_ERRORS_KEY    "/apps/anjuta/valgrind/memcheck/avoid-strlen-errors"

static void vg_memcheck_prefs_class_init (VgMemcheckPrefsClass *klass);
static void vg_memcheck_prefs_init (VgMemcheckPrefs *prefs);
static void vg_memcheck_prefs_destroy (GtkObject *obj);
static void vg_memcheck_prefs_finalize (GObject *obj);

static void memcheck_prefs_apply (VgToolPrefs *prefs);
static void memcheck_prefs_get_argv (VgToolPrefs *prefs, const char *tool, GPtrArray *argv);


static VgToolPrefsClass *parent_class = NULL;

enum {
  COLUMN_STRING,
  COLUMN_INT,
  N_COLUMNS
};

GType
vg_memcheck_prefs_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		static const GTypeInfo info = {
			sizeof (VgMemcheckPrefsClass),
			NULL, /* base_class_init */
			NULL, /* base_class_finalize */
			(GClassInitFunc) vg_memcheck_prefs_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (VgMemcheckPrefs),
			0,    /* n_preallocs */
			(GInstanceInitFunc) vg_memcheck_prefs_init,
		};
		
		type = g_type_register_static (VG_TYPE_TOOL_PREFS, "VgMemcheckPrefs", &info, 0);
	}
	
	return type;
}

static void
vg_memcheck_prefs_class_init (VgMemcheckPrefsClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS (klass);
	VgToolPrefsClass *tool_class = VG_TOOL_PREFS_CLASS (klass);
	
	parent_class = g_type_class_ref (VG_TYPE_TOOL_PREFS);
	
	object_class->finalize = vg_memcheck_prefs_finalize;
	gtk_object_class->destroy = vg_memcheck_prefs_destroy;
	
	/* virtual methods */
	tool_class->apply = memcheck_prefs_apply;
	tool_class->get_argv = memcheck_prefs_get_argv;
}


static void
toggle_button_toggled (GtkToggleButton *toggle, const char *key)
{
	GConfClient *gconf;
	gboolean bool;
	
	gconf = gconf_client_get_default ();
	
	bool = gtk_toggle_button_get_active (toggle);
	gconf_client_set_bool (gconf, key, bool, NULL);
	
	g_object_unref (gconf);
}

static void
menu_item_activated (GtkComboBox *widget, const char *key)
{
	int i = 0;
	GConfClient *gconf;
	const char *str;
	GtkTreeIter iter;
	
	gconf = gconf_client_get_default ();

	gtk_combo_box_get_active_iter (widget, &iter);
	gtk_tree_model_get (gtk_combo_box_get_model (widget), &iter, 1, &i, -1);

	str = GINT_TO_POINTER (i);

	gconf_client_set_string (gconf, key, str, NULL);
	
	g_object_unref (gconf);
}

static gboolean
spin_focus_out (GtkSpinButton *spin, GdkEventFocus *event, const char *key)
{
	GConfClient *gconf;
	int num;
	
	gconf = gconf_client_get_default ();
	
	num = gtk_spin_button_get_value_as_int (spin);
	gconf_client_set_int (gconf, key, num, NULL);
	
	g_object_unref (gconf);
	
	return FALSE;
}

static GtkWidget *
combo_box_new (GConfClient *gconf, char *key, char **values, int n, int def)
{
	GtkListStore *list_store;
	GtkWidget *omenu;
	GtkTreeIter iter;
	GtkCellRenderer *cell;
	int history = def;
	char *str;
	int i;
	

	list_store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_INT);
	str = gconf_client_get_string (gconf, key, NULL);
	
	for (i = 0; i < n; i++) {
		if (str && !strcmp (values[i], str))
			history = i;
		
		gtk_list_store_append (list_store, &iter);
		gtk_list_store_set (list_store, &iter, COLUMN_STRING, _(values[i]), COLUMN_INT, GPOINTER_TO_INT (g_strdup (_(values[i]))), -1);
	}
	
	omenu = gtk_combo_box_new_with_model(GTK_TREE_MODEL(list_store));

	cell = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(omenu), cell, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(omenu), cell, "text", 0, NULL);

	gtk_combo_box_set_active (GTK_COMBO_BOX (omenu), history);
	g_signal_connect (omenu, "changed", G_CALLBACK (menu_item_activated), key);
	
	g_free (str);
	
	return omenu;
}

static char *leak_checks[] = { "no", "summary", "full" };
static char *leak_resolutions[] = { "low", "med", "high" };

static void
vg_memcheck_prefs_init (VgMemcheckPrefs *prefs)
{
	GtkWidget *vbox, *hbox, *label, *frame;
	GConfClient *gconf;
	GtkWidget *widget;
	gboolean bool;
	int num;
	
	gconf = gconf_client_get_default ();
	
	VG_TOOL_PREFS (prefs)->label = _("Memcheck");
	
	vbox = GTK_WIDGET (prefs);
	gtk_box_set_spacing (GTK_BOX (vbox), 6);
	
	frame = gtk_frame_new (_("Memory leaks"));
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
	
	hbox = gtk_hbox_new (FALSE, 6);
	label = gtk_label_new (_("Leak check:"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	widget = combo_box_new (gconf, LEAK_CHECK_KEY, leak_checks, G_N_ELEMENTS (leak_checks), 1);
	prefs->leak_check = GTK_COMBO_BOX (widget);
	gtk_widget_show (widget);
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	
	bool = gconf_client_get_bool (gconf, SHOW_REACHABLE_KEY, NULL);
	widget = gtk_check_button_new_with_label (_("Show reachable blocks in leak check"));
	g_signal_connect (widget, "toggled", G_CALLBACK (toggle_button_toggled), SHOW_REACHABLE_KEY);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), bool);
	prefs->show_reachable = GTK_TOGGLE_BUTTON (widget);
	gtk_widget_show (widget);
	gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 6);
	label = gtk_label_new (_("Leak resolution:"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	widget = combo_box_new (gconf, LEAK_RESOLUTION_KEY, leak_resolutions, G_N_ELEMENTS (leak_resolutions), 0);
	prefs->leak_resolution = GTK_COMBO_BOX (widget);
	gtk_widget_show (widget);
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	
	gtk_widget_show (vbox);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	vbox = GTK_WIDGET (prefs);
	
	gtk_widget_show (frame);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 6);
	label = gtk_label_new (_("Keep up to"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	num = gconf_client_get_int (gconf, FREELIST_VOL_KEY, NULL);
	widget = gtk_spin_button_new_with_range (0, (gdouble) INT_MAX, 4);
	gtk_widget_show (widget);
	prefs->freelist_vol = GTK_SPIN_BUTTON (widget);
	gtk_spin_button_set_digits (prefs->freelist_vol, 0);
	gtk_spin_button_set_numeric (prefs->freelist_vol, TRUE);
	gtk_spin_button_set_value (prefs->freelist_vol, (gdouble) num);
	g_signal_connect (widget, "focus-out-event", G_CALLBACK (spin_focus_out), FREELIST_VOL_KEY);
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
	label = gtk_label_new (_("bytes in the queue after being free()'d"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	
	bool = gconf_client_get_bool (gconf, WORKAROUND_GCC296_BUGS_KEY, NULL);
	widget = gtk_check_button_new_with_label (_("Work around bugs generated by gcc 2.96"));
	g_signal_connect (widget, "toggled", G_CALLBACK (toggle_button_toggled), WORKAROUND_GCC296_BUGS_KEY);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), bool);
	prefs->workaround_gcc296_bugs = GTK_TOGGLE_BUTTON (widget);
	gtk_widget_show (widget);
	gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
/*/	
	bool = gconf_client_get_bool (gconf, AVOID_STRLEN_ERRORS_KEY, NULL);
	widget = gtk_check_button_new_with_label (_("Ignore errors produced by inline strlen() calls"));
	g_signal_connect (widget, "toggled", G_CALLBACK (toggle_button_toggled), AVOID_STRLEN_ERRORS_KEY);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), bool);
	prefs->avoid_strlen_errors = GTK_TOGGLE_BUTTON (widget);
	gtk_widget_show (widget);
	gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
/*/	
	g_object_unref (gconf);
}

static void
vg_memcheck_prefs_finalize (GObject *obj)
{
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
vg_memcheck_prefs_destroy (GtkObject *obj)
{
	GTK_OBJECT_CLASS (parent_class)->destroy (obj);
}


static void
memcheck_prefs_apply (VgToolPrefs *prefs)
{
	;
}


enum {
	ARG_TYPE_BOOL,
	ARG_TYPE_INT,
	ARG_TYPE_STRING
};

enum {
	ADDRCHECK = 1,
	MEMCHECK  = 2,
	BOTH      = 3
};

static struct {
	const char *key;
	const char *arg;
	unsigned int mask;
	char *buf;
	int type;
	int dval;
} memcheck_args[] = {
	{ LEAK_CHECK_KEY,             "--leak-check",             BOTH,     NULL, ARG_TYPE_STRING, 0       },
	{ SHOW_REACHABLE_KEY,         "--show-reachable",         BOTH,     NULL, ARG_TYPE_BOOL,   0       },
	{ LEAK_RESOLUTION_KEY,        "--leak-resolution",        BOTH,     NULL, ARG_TYPE_STRING, 0       },
	{ FREELIST_VOL_KEY,           "--freelist-vol",           BOTH,     NULL, ARG_TYPE_INT,    1000000 },
	{ WORKAROUND_GCC296_BUGS_KEY, "--workaround-gcc296-bugs", BOTH,     NULL, ARG_TYPE_BOOL,   0       }/*,
	{ AVOID_STRLEN_ERRORS_KEY,    "--avoid-strlen-errors",    MEMCHECK, NULL, ARG_TYPE_BOOL,   1       },*/
};

static void
memcheck_prefs_get_argv (VgToolPrefs *prefs, const char *tool, GPtrArray *argv)
{
	GConfClient *gconf;
	unsigned int mode;
        int bool, num, i;
	char *str;
	
	if (tool != NULL && !strcmp (tool, "addrcheck"))
		mode = ADDRCHECK;
	else
		mode = MEMCHECK;
	
	gconf = gconf_client_get_default ();
	
	for (i = 0; i < G_N_ELEMENTS (memcheck_args); i++) {
		const char *arg = memcheck_args[i].arg;
		const char *key = memcheck_args[i].key;
		
		g_free (memcheck_args[i].buf);
		if (memcheck_args[i].mask & mode) {
			if (memcheck_args[i].type == ARG_TYPE_INT) {
				num = gconf_client_get_int (gconf, key, NULL);
				if (num == memcheck_args[i].dval)
					continue;
				
				memcheck_args[i].buf = g_strdup_printf ("%s=%d", arg, num);
			} else if (memcheck_args[i].type == ARG_TYPE_BOOL) {
				bool = gconf_client_get_bool (gconf, key, NULL) ? 1 : 0;
				if (bool == memcheck_args[i].dval)
					continue;
				
				memcheck_args[i].buf = g_strdup_printf ("%s=%s", arg, bool ? "yes" : "no");
			} else {
				if (!(str = gconf_client_get_string (gconf, key, NULL)) || *str == '\0') {
					memcheck_args[i].buf = NULL;
					g_free (str);
					continue;
				}
				
				memcheck_args[i].buf = g_strdup_printf ("%s=%s", arg, str);
				g_free (str);
			}
			
			g_ptr_array_add (argv, memcheck_args[i].buf);
		} else {
			memcheck_args[i].buf = NULL;
		}
	}
	
	g_object_unref (gconf);
}
