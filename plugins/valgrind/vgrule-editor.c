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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <glib/gi18n.h>

#include "vgrule-editor.h"


static void vg_rule_editor_class_init (VgRuleEditorClass *klass);
static void vg_rule_editor_init (VgRuleEditor *editor);
static void vg_rule_editor_destroy (GtkObject *obj);
static void vg_rule_editor_finalize (GObject *obj);


static GtkVBoxClass *parent_class = NULL;

enum {
  COLUMN_STRING,
  COLUMN_INT,
  N_COLUMNS
};


GType
vg_rule_editor_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		static const GTypeInfo info = {
			sizeof (VgRuleEditorClass),
			NULL, /* base_class_init */
			NULL, /* base_class_finalize */
			(GClassInitFunc) vg_rule_editor_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (VgRuleEditor),
			0,    /* n_preallocs */
			NULL /*(GInstanceInitFunc) vg_rule_editor_init*/,
		};
		
		type = g_type_register_static (GTK_TYPE_VBOX, "VgRuleEditor", &info, 0);
	}
	
	return type;
}

static void
vg_rule_editor_class_init (VgRuleEditorClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS (klass);
	
	parent_class = g_type_class_ref (GTK_TYPE_VBOX);
	
	object_class->finalize = vg_rule_editor_finalize;
	gtk_object_class->destroy = vg_rule_editor_destroy;
}


static void
type_menu_changed (GtkComboBox *widget, gpointer user_data)
{
	VgRuleEditor *editor = user_data;
	vgrule_t type;
	GtkTreeIter iter;
	
	gtk_combo_box_get_active_iter (widget, &iter);
	gtk_tree_model_get (gtk_combo_box_get_model (widget), &iter, 1, &type, -1);

	gtk_widget_set_sensitive (GTK_WIDGET (editor->syscall), type == VG_RULE_PARAM);
}

static GtkWidget *
rule_type_menu_new (VgRuleEditor *editor)
{
	GtkListStore *list_store;
	GtkWidget *omenu;
	int i;
	GtkTreeIter iter;
	GtkCellRenderer *cell;

	list_store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_INT);
	
	for (i = 0; i < VG_RULE_LAST; i++) {
		gtk_list_store_append (list_store, &iter);
		gtk_list_store_set (list_store, &iter, COLUMN_STRING, vg_rule_type_to_name (i), COLUMN_INT, i, -1);
	}

	omenu = gtk_combo_box_new_with_model(GTK_TREE_MODEL(list_store));

	cell = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(omenu), cell, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(omenu), cell, "text", 0, NULL);

	gtk_combo_box_set_active (GTK_COMBO_BOX (omenu), 0);	
	g_signal_connect (omenu, "changed", G_CALLBACK (type_menu_changed), editor);
	
	return omenu;
}

static GtkWidget *
call_stack_frame_new (vgcaller_t type, const char *name)
{
	GtkListStore *list_store;
	GtkWidget *hbox, *omenu, *entry;
	GtkTreeIter iter;
	GtkCellRenderer *cell;

	hbox = gtk_hbox_new (FALSE, 6);
	list_store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_INT);

	gtk_list_store_append (list_store, &iter);
	gtk_list_store_set (list_store, &iter, COLUMN_STRING, _("Function"), COLUMN_INT, VG_CALLER_FUNCTION, -1);

	gtk_list_store_append (list_store, &iter);
	gtk_list_store_set (list_store, &iter, COLUMN_STRING, _("Shared Object"), COLUMN_INT, VG_CALLER_OBJECT, -1);

	omenu = gtk_combo_box_new_with_model(GTK_TREE_MODEL(list_store));

	cell = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(omenu), cell, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(omenu), cell, "text", 0, NULL);
	
	gtk_combo_box_set_active (GTK_COMBO_BOX (omenu), (int) type);
	
	gtk_widget_show (omenu);
	gtk_box_pack_start (GTK_BOX (hbox), omenu, FALSE, FALSE, 0);
	
	entry = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (entry), name ? name : "");
	gtk_widget_show (entry);
	gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
	
	g_object_set_data (G_OBJECT (hbox), "omenu", omenu);
	g_object_set_data (G_OBJECT (hbox), "entry", entry);
	
	return hbox;
}

static void
grow_cb (GtkButton *button, VgRuleEditor *editor)
{
	GtkWidget *caller;
	int len;
	
	len = editor->callers->len;
	caller = call_stack_frame_new (0, NULL);
	g_ptr_array_add (editor->callers, caller);
	gtk_widget_show (caller);
	gtk_box_pack_start (editor->call_stack, caller, FALSE, FALSE, 0);
}

static void
shrink_cb (GtkButton *button, VgRuleEditor *editor)
{
	int i;
	
	if (editor->callers->len == 1)
		return;
	
	i = editor->callers->len - 1;
	gtk_widget_destroy (editor->callers->pdata[i]);
	g_ptr_array_remove_index (editor->callers, editor->callers->len - 1);
}

static GtkWidget *
call_stack_new (VgRuleEditor *editor)
{
	GtkWidget *vbox, *hbox, *button;
	GtkWidget *widget;
	int i;
	
	vbox = gtk_vbox_new (FALSE, 3);
	
	hbox = gtk_hbox_new (FALSE, 6);
	button = gtk_button_new_with_label (_("Grow"));
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	g_signal_connect (button, "clicked", G_CALLBACK (grow_cb), editor);
	button = gtk_button_new_with_label (_("Shrink"));
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	g_signal_connect (button, "clicked", G_CALLBACK (shrink_cb), editor);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	
	for (i = 0; i < editor->callers->len; i++) {
		widget = editor->callers->pdata[i];
		gtk_widget_show (widget);
		gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
	}
	
	return vbox;
}

static void
vg_rule_editor_init (VgRuleEditor *editor)
{
	GtkWidget *vbox, *hbox, *label;
	GtkWidget *widget;
	
	vbox = GTK_WIDGET (editor);
	gtk_box_set_spacing (GTK_BOX (vbox), 6);
	
	hbox = gtk_hbox_new (FALSE, 6);
	label = gtk_label_new (_("Rule name:"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	editor->name = GTK_ENTRY (widget = gtk_entry_new ());
	gtk_widget_show (widget);
	gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 6);
	label = gtk_label_new (_("Suppress messages of type:"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	editor->type = GTK_COMBO_BOX (widget = rule_type_menu_new (editor));
	gtk_widget_show (widget);
	gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 6);
	label = gtk_label_new (_("Suppress when using:"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	editor->addrcheck = GTK_TOGGLE_BUTTON (widget = gtk_check_button_new_with_label ("Addrcheck"));
	gtk_widget_show (widget);
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
	editor->memcheck = GTK_TOGGLE_BUTTON (widget = gtk_check_button_new_with_label ("Memcheck"));
	gtk_widget_show (widget);
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 6);
	label = gtk_label_new (_("System call:"));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	editor->syscall = GTK_ENTRY (widget = gtk_entry_new ());
	gtk_widget_show (widget);
	gtk_widget_set_sensitive (widget, FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	
	label = gtk_label_new (_("Call chain:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
	
	/*editor->callers = g_ptr_array_new ();
	  g_ptr_array_add (editor->callers, call_stack_frame_new (0, NULL));*/
	
	editor->call_stack = GTK_BOX (widget = call_stack_new (editor));
	gtk_widget_show (widget);
	gtk_box_pack_start (GTK_BOX (vbox), widget, TRUE, TRUE, 0);
}

static void
vg_rule_editor_finalize (GObject *obj)
{
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
vg_rule_editor_destroy (GtkObject *obj)
{
	GTK_OBJECT_CLASS (parent_class)->destroy (obj);
}


GtkWidget *
vg_rule_editor_new (void)
{
	VgRuleEditor *editor;
	
	editor = g_object_new (VG_TYPE_RULE_EDITOR, NULL);
	editor->callers = g_ptr_array_new ();
	g_ptr_array_add (editor->callers, call_stack_frame_new (0, NULL));
	
	vg_rule_editor_init (editor);
	
	gtk_toggle_button_set_active (editor->addrcheck, TRUE);
	gtk_toggle_button_set_active (editor->memcheck, TRUE);
	
	return GTK_WIDGET (editor);
}


GtkWidget *
vg_rule_editor_new_from_rule (VgRule *rule)
{
	VgRuleEditor *editor;
	VgCaller *caller;
	VgTool *tool;
	
	editor = g_object_new (VG_TYPE_RULE_EDITOR, NULL);
	editor->callers = g_ptr_array_new ();
	
	caller = rule->callers;
	while (caller != NULL) {
		g_ptr_array_add (editor->callers, call_stack_frame_new (caller->type, caller->name));
		
		caller = caller->next;
	}
	
	if (editor->callers->len == 0)
		g_ptr_array_add (editor->callers, call_stack_frame_new (0, NULL));
	
	vg_rule_editor_init (editor);
	
	vg_rule_editor_set_type (editor, rule->type);
	vg_rule_editor_set_name (editor, rule->name);
	vg_rule_editor_set_syscall (editor, rule->syscall);
	
	tool = rule->tools;
	while (tool != NULL) {
		if (!strcasecmp (tool->name, "core")) {
			/* special case... */
			g_object_set_data (G_OBJECT (editor), "core", GINT_TO_POINTER (TRUE));
		} else if (!strcasecmp (tool->name, "Addrcheck")) {
			gtk_toggle_button_set_active (editor->addrcheck, TRUE);
		} else if (!strcasecmp (tool->name, "Memcheck")) {
			gtk_toggle_button_set_active (editor->memcheck, TRUE);
		}
		
		tool = tool->next;
	}
	
	return GTK_WIDGET (editor);
}


GtkWidget *
vg_rule_editor_new_from_summary (VgErrorSummary *summary)
{
	VgRuleEditor *editor;
	VgErrorStack *stack;
	GString *rule_name;
	vgrule_t rtype;
	char *syscall;
	
	editor = g_object_new (VG_TYPE_RULE_EDITOR, NULL);
	editor->callers = g_ptr_array_new ();
	
	rule_name = g_string_new ("");
	
	stack = summary->frames;
	while (stack != NULL) {
		const char *name = NULL;
		const char *basename;
		vgcaller_t ctype = 0;
		
		/* if we can get a symbol, use it - otherwise try and use the shared object */
		if (stack->symbol) {
			name = stack->symbol;
			ctype = VG_CALLER_FUNCTION;
			g_string_append (rule_name, name);
		} else if (stack->type == VG_STACK_OBJECT) {
			name = stack->info.object;
			ctype = VG_CALLER_OBJECT;
			
			if (!(basename = strrchr (name, '/')))
				basename = name;
			else
				basename++;
			
			g_string_append (rule_name, basename);
		}
		
		if (stack->next)
			g_string_append_c (rule_name, '/');
		
		g_ptr_array_add (editor->callers, call_stack_frame_new (ctype, name));
		
		stack = stack->next;
	}
	
	if (editor->callers->len == 0)
		g_ptr_array_add (editor->callers, call_stack_frame_new (0, NULL));
	
	vg_rule_editor_init (editor);
	
	gtk_toggle_button_set_active (editor->addrcheck, TRUE);
	gtk_toggle_button_set_active (editor->memcheck, TRUE);
	
	syscall = NULL;
	if (vg_rule_type_from_report (summary->report, &rtype, &syscall)) {
		vg_rule_editor_set_type (editor, rtype);
		if (syscall != NULL) {
			vg_rule_editor_set_syscall (editor, syscall);
			g_free (syscall);
		}
		
		g_string_append_c (rule_name, '(');
		g_string_append (rule_name, vg_rule_type_to_name (rtype));
		g_string_append_c (rule_name, ')');
	}
	
	vg_rule_editor_set_name (editor, rule_name->str);
	g_string_free (rule_name, TRUE);
	
	return GTK_WIDGET (editor);
}


const char *
vg_rule_editor_get_name (VgRuleEditor *editor)
{
	return gtk_entry_get_text (editor->name);
}

void
vg_rule_editor_set_name (VgRuleEditor *editor, const char *name)
{
	gtk_entry_set_text (editor->name, name ? name : "");
}

void
vg_rule_editor_set_type (VgRuleEditor *editor, vgrule_t type)
{
	gtk_combo_box_set_active (editor->type, (int) type);
}

void
vg_rule_editor_set_syscall (VgRuleEditor *editor, const char *syscall)
{
	gtk_entry_set_text (editor->syscall, syscall ? syscall : "");
}

void
vg_rule_editor_add_caller (VgRuleEditor *editor, vgcaller_t type, const char *name)
{
	GtkWidget *caller;
	int len;
	
	len = editor->callers->len;
	caller = call_stack_frame_new (0, NULL);
	g_ptr_array_add (editor->callers, caller);
	gtk_widget_show (caller);
	gtk_box_pack_start (editor->call_stack, caller, FALSE, FALSE, 0);
}

VgRule *
vg_rule_editor_get_rule (VgRuleEditor *editor)
{
	GtkWidget *omenu, *entry;
	VgCaller *caller, *tail;
	const char *name;
	VgRule *rule;
	int type, i;
	
	name = gtk_entry_get_text (editor->name);
	type = gtk_combo_box_get_active (editor->type);
	rule = vg_rule_new (type, name);
	
	if (type == VG_RULE_PARAM)
		rule->syscall = g_strdup (gtk_entry_get_text (editor->syscall));
	
	if (gtk_toggle_button_get_active (editor->addrcheck))
		vg_rule_add_tool (rule, "Addrcheck");
	
	if (gtk_toggle_button_get_active (editor->memcheck))
		vg_rule_add_tool (rule, "Memcheck");
	
	if (!rule->tools && g_object_get_data (G_OBJECT (editor), "core")) {
		/* this means we are editing a valgrind 1.9.x versioned supp file
		   which needs at least 1 'tool' specified to suppress */
		vg_rule_add_tool (rule, "core");
	}
	
	tail = (VgCaller *) &rule->callers;
	
	for (i = 0; i < editor->callers->len; i++) {
		omenu = g_object_get_data (G_OBJECT (editor->callers->pdata[i]), "omenu");
		entry = g_object_get_data (G_OBJECT (editor->callers->pdata[i]), "entry");
		
		name = gtk_entry_get_text (GTK_ENTRY (entry));
		type = gtk_combo_box_get_active (GTK_COMBO_BOX (omenu));
		
		caller = vg_caller_new (type, name);
		tail->next = caller;
		tail = caller;
	}
	
	return rule;
}

void
vg_rule_editor_save (VgRuleEditor *editor, const char *filename)
{
	GtkWindow *parent;
	GtkWidget *dialog;
	VgRule *rule;
	off_t offset;
	int fd;
	
	parent = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (editor)));
	
	if ((fd = open (filename, O_WRONLY | O_APPEND, 0666)) == -1) {
		dialog = gtk_message_dialog_new (parent, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
						 _("Error saving to suppression file '%s': %s"),
						 filename, g_strerror (errno));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		return;
	}
	
	rule = vg_rule_editor_get_rule (editor);
	
	/* find out where we currently are */
	offset = lseek (fd, 0, SEEK_END);
	
	if (vg_suppressions_file_append_rule (fd, rule) == -1 || fsync (fd) == -1) {
		dialog = gtk_message_dialog_new (parent, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
						 _("Error saving to suppression file '%s': %s"),
						 filename, g_strerror (errno));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		
		ftruncate (fd, offset);
	}
	
	vg_rule_free (rule);
	
	close (fd);
}
