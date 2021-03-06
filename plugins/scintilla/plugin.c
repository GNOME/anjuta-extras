/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2000 Naba Kumar

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <config.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-encodings.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-symbol-manager.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-savable.h>
#include <libanjuta/interfaces/ianjuta-editor-factory.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>


#include "aneditor.h"
#include "lexer.h"
#include "plugin.h"
#include "style-editor.h"
#include "text_editor.h"

#define PREFS_GLADE PACKAGE_DATA_DIR "/glade/anjuta-editor-scintilla.ui"
#define ICON_FILE "anjuta-editor-scintilla-plugin-48.png"
#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-scintilla.xml"

static gpointer parent_class;

/* signals */
enum
{
        STYLE_CHANGED,
        LAST_SIGNAL
};

static unsigned int signals[LAST_SIGNAL] = { 0 };

/* Plugin types
 *---------------------------------------------------------------------------*/

struct _EditorPlugin{
	AnjutaPlugin parent;

	GtkWidget* style_button;
	
	IAnjutaSymbolQuery *query_system;
	IAnjutaSymbolQuery *query_project;

	/* Settings */
	GSettings *settings;
	GtkActionGroup *group;
	gint uiid;
	AnjutaPreferences* prefs;
};

struct _EditorPluginClass{
	AnjutaPluginClass parent_class;

	/* signals */
	void (* style_changed)     ();
};

/* Editor preferences
 *---------------------------------------------------------------------------*/

static void
on_editor_linenos_activate (GtkAction *action, gpointer user_data)
{
	gboolean state;
	EditorPlugin* plugin = ANJUTA_PLUGIN_EDITOR (user_data);

	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	g_settings_set_boolean (plugin->settings,
	                        VIEW_LINENUMBERS_MARGIN, state);
}

static void
on_editor_markers_activate (GtkAction *action, gpointer user_data)
{
	gboolean state;
	EditorPlugin* plugin = ANJUTA_PLUGIN_EDITOR (user_data);

	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	g_settings_set_boolean (plugin->settings,
	                        VIEW_MARKER_MARGIN, state);
}

static void
on_editor_codefold_activate (GtkAction *action, gpointer user_data)
{
	gboolean state;
	EditorPlugin* plugin = ANJUTA_PLUGIN_EDITOR (user_data);

	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	g_settings_set_boolean (plugin->settings,
	                        VIEW_FOLD_MARGIN, state);
}

static void
on_editor_indentguides_activate (GtkAction *action, gpointer user_data)
{
	gboolean state;
	EditorPlugin* plugin = ANJUTA_PLUGIN_EDITOR (user_data);

	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	g_settings_set_boolean (plugin->settings,
	                        VIEW_INDENTATION_GUIDES, state);
}

static void
on_editor_whitespaces_activate (GtkAction *action, gpointer user_data)
{
	gboolean state;
	EditorPlugin* plugin = ANJUTA_PLUGIN_EDITOR (user_data);

	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	g_settings_set_boolean (plugin->settings,
	                        VIEW_WHITE_SPACES, state);
}

static void
on_editor_eolchars_activate (GtkAction *action, gpointer user_data)
{
	gboolean state;
	EditorPlugin* plugin = ANJUTA_PLUGIN_EDITOR (user_data);

	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	g_settings_set_boolean (plugin->settings,
	                        VIEW_EOL, state);
}

static void
on_editor_linewrap_activate (GtkAction *action, gpointer user_data)
{
	gboolean state;
	EditorPlugin* plugin = ANJUTA_PLUGIN_EDITOR (user_data);

	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	g_settings_set_boolean (plugin->settings,
	                        VIEW_LINE_WRAP, state);
}

static GtkToggleActionEntry actions_view[] = {
  { "ActionViewEditorLinenumbers", NULL, N_("_Line Number Margin"), NULL,
	N_("Show/Hide line numbers"),
    G_CALLBACK (on_editor_linenos_activate), FALSE},
  { "ActionViewEditorMarkers", NULL, N_("_Marker Margin"), NULL,
	N_("Show/Hide marker margin"),
    G_CALLBACK (on_editor_markers_activate), FALSE},
  { "ActionViewEditorFolds", NULL, N_("_Code Fold Margin"), NULL,
	N_("Show/Hide code fold margin"),
    G_CALLBACK (on_editor_codefold_activate), FALSE},
  { "ActionViewEditorGuides", NULL, N_("_Indentation Guides"), NULL,
	N_("Show/Hide indentation guides"),
    G_CALLBACK (on_editor_indentguides_activate), FALSE},
  { "ActionViewEditorSpaces", NULL, N_("_White Space"), NULL,
	N_("Show/Hide white spaces"),
    G_CALLBACK (on_editor_whitespaces_activate), FALSE},
  { "ActionViewEditorEOL", NULL, N_("_Line End Characters"), NULL,
	N_("Show/Hide line end characters"),
    G_CALLBACK (on_editor_eolchars_activate), FALSE},
  { "ActionViewEditorWrapping", NULL, N_("Line _Wrapping"), NULL,
	N_("Enable/disable line wrapping"),
    G_CALLBACK (on_editor_linewrap_activate), FALSE}
};

static void
ui_states_init (EditorPlugin* plugin, AnjutaUI *ui)
{
	static const gchar *prefs[] = {
		VIEW_LINENUMBERS_MARGIN,
		VIEW_MARKER_MARGIN,
		VIEW_FOLD_MARGIN,
		VIEW_INDENTATION_GUIDES,
		VIEW_WHITE_SPACES,
		VIEW_EOL,
		VIEW_LINE_WRAP
	};
	gint i;

	for (i = 0; i < sizeof (prefs)/sizeof(const gchar *); i++)
	{
		GtkAction *action;
		gboolean state;

		state = g_settings_get_boolean (plugin->settings, prefs[i]);
		action = anjuta_ui_get_action (ui, "ActionGroupEditorView",
		                               actions_view[i].name);
		g_object_set (G_OBJECT (action), "sensitive", TRUE, "visible", TRUE, NULL);
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), state);
	}
}


/* Keep an up to date list of type name
 *---------------------------------------------------------------------------*/

static void
update_type_list (AnjutaShell *shell, IAnjutaIterable *iter, const gchar *name)
{
	gchar *list = NULL;
	GValue value = {0,};
	
	if (iter)
	{
		ianjuta_iterable_first (iter, NULL);
		if (ianjuta_iterable_get_length (iter, NULL) > 0)
		{
			GString *s = g_string_sized_new(ianjuta_iterable_get_length (iter, NULL) * 10);
			do {
				IAnjutaSymbol *symbol = IANJUTA_SYMBOL (iter);
				const gchar *sname = ianjuta_symbol_get_string (symbol, IANJUTA_SYMBOL_FIELD_NAME, NULL);
				g_string_append(s, sname);
				g_string_append_c(s, ' ');
			} while (ianjuta_iterable_next (iter, NULL));
			list =  g_string_free(s, FALSE);
		}
	}
	
	anjuta_shell_get_value (shell, name, &value, NULL);
	if (G_VALUE_HOLDS_STRING(&value))
	{
		const gchar *value_list = g_value_get_string (&value);
		
		if (list == NULL)
		{
			anjuta_shell_remove_value (shell, name, NULL);
		}
		else if (strcmp (list, value_list) == 0)
		{
			g_free (list);
		}
		else
		{
			g_value_take_string (&value, list);
			anjuta_shell_add_value (shell, name, &value, NULL);
		}
	}
	else
	{
		if (list != NULL)
		{
			g_value_init (&value, G_TYPE_STRING);
			g_value_take_string (&value, list);
			anjuta_shell_add_value (shell, name, &value, NULL);
		}
	}
	if (G_IS_VALUE (&value))
		g_value_unset (&value);
}

static void
project_symbol_found (IAnjutaSymbolQuery *query, IAnjutaIterable* symbols, gpointer user_data)
{
	update_type_list (ANJUTA_SHELL (user_data), symbols, TEXT_EDITOR_PROJECT_TYPE_LIST);
}

static void
system_symbol_found (IAnjutaSymbolQuery *query, IAnjutaIterable* symbols, gpointer user_data)
{
	update_type_list (ANJUTA_SHELL (user_data), symbols, TEXT_EDITOR_SYSTEM_TYPE_LIST);
}

static void
on_project_symbol_scanned (IAnjutaSymbolManager *manager, guint process, IAnjutaSymbolQuery *query_project)
{
	ianjuta_symbol_query_search_all (query_project, NULL);
}

static void
on_system_symbol_scanned (IAnjutaSymbolManager *manager, guint process, IAnjutaSymbolQuery *query_system)
{
	ianjuta_symbol_query_search_all (query_system, NULL);
}

static void 
on_style_button_clicked(GtkWidget* button, EditorPlugin *plugin)
{
	StyleEditor* se = style_editor_new(ANJUTA_PLUGIN (plugin), plugin->prefs, plugin->settings);
	style_editor_show(se);
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	static IAnjutaSymbolField query_fields[] = {
		IANJUTA_SYMBOL_FIELD_ID,
		IANJUTA_SYMBOL_FIELD_NAME,
		IANJUTA_SYMBOL_FIELD_KIND,
		IANJUTA_SYMBOL_FIELD_TYPE
	};
	EditorPlugin* editor = ANJUTA_PLUGIN_EDITOR (plugin);
	IAnjutaSymbolManager *manager;
	IAnjutaSymbolQuery *query_project;
	IAnjutaSymbolQuery *query_system;
	AnjutaUI *ui;
	
	manager = anjuta_shell_get_interface (plugin->shell, 
	    IAnjutaSymbolManager, NULL);

	/* query project */
	query_project =
		ianjuta_symbol_manager_create_query (manager,
		                                     IANJUTA_SYMBOL_QUERY_SEARCH_ALL,
		                                     IANJUTA_SYMBOL_QUERY_DB_PROJECT,
		                                     NULL);

	ianjuta_symbol_query_set_fields (query_project,
	                                 G_N_ELEMENTS (query_fields),
	                                 query_fields, NULL);
	ianjuta_symbol_query_set_file_scope (query_project,
	                                     IANJUTA_SYMBOL_QUERY_SEARCH_FS_IGNORE, NULL);
	ianjuta_symbol_query_set_mode (query_project,
	                               IANJUTA_SYMBOL_QUERY_MODE_QUEUED, NULL);
	ianjuta_symbol_query_set_filters (query_project, IANJUTA_SYMBOL_TYPE_CLASS | IANJUTA_SYMBOL_TYPE_ENUM | IANJUTA_SYMBOL_TYPE_STRUCT | IANJUTA_SYMBOL_TYPE_TYPEDEF | IANJUTA_SYMBOL_TYPE_UNION,  
	    							  TRUE, NULL);	
	g_signal_connect (query_project, "async-result",
	                  G_CALLBACK (project_symbol_found), plugin->shell);
	
	/* query system */
	query_system =
		ianjuta_symbol_manager_create_query (manager,
		                                     IANJUTA_SYMBOL_QUERY_SEARCH_ALL,
		                                     IANJUTA_SYMBOL_QUERY_DB_SYSTEM,
		                                     NULL);

	ianjuta_symbol_query_set_fields (query_system,
	                                 G_N_ELEMENTS (query_fields),
	                                 query_fields, NULL);
	ianjuta_symbol_query_set_file_scope (query_system,
	                                     IANJUTA_SYMBOL_QUERY_SEARCH_FS_IGNORE, NULL);
	ianjuta_symbol_query_set_mode (query_system,
	                               IANJUTA_SYMBOL_QUERY_MODE_QUEUED, NULL);
	ianjuta_symbol_query_set_filters (query_system, IANJUTA_SYMBOL_TYPE_CLASS | IANJUTA_SYMBOL_TYPE_ENUM | IANJUTA_SYMBOL_TYPE_STRUCT | IANJUTA_SYMBOL_TYPE_TYPEDEF | IANJUTA_SYMBOL_TYPE_UNION,  
	    							  TRUE, NULL);	
	
	g_signal_connect (query_system, "async-result",
	                  G_CALLBACK (system_symbol_found), plugin->shell);


	/* Get notified when scan end, to update type list */
	g_signal_connect (G_OBJECT (manager), "prj_scan_end", G_CALLBACK (on_project_symbol_scanned), query_project);
	g_signal_connect (G_OBJECT (manager), "sys_scan_end", G_CALLBACK (on_system_symbol_scanned), query_system);
	
	/* Initialize type list */
	on_project_symbol_scanned (manager, 0, query_project);
	on_system_symbol_scanned (manager, 0, query_system);

	/* Keep queries to be able to unref them when needed */
	editor->query_project = query_project;
	editor->query_system = query_system;

	/* Add menu entries */
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	editor->group = anjuta_ui_add_toggle_action_group_entries (ui, "ActionGroupEditorView",
	                                                           _("Editor view settings"),
	                                                           actions_view,
	                                                           G_N_ELEMENTS (actions_view),
	                                                           GETTEXT_PACKAGE, TRUE, editor);
	ui_states_init (editor, ui);
	editor->uiid = anjuta_ui_merge (ui, UI_FILE);

	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	EditorPlugin* editor = ANJUTA_PLUGIN_EDITOR (plugin);
	IAnjutaSymbolManager *manager = anjuta_shell_get_interface (plugin->shell, IAnjutaSymbolManager, NULL);
	AnjutaUI *ui;

	/* Remove menu items */
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (plugin)->shell, NULL);
	anjuta_ui_unmerge (ui, editor->uiid);
	anjuta_ui_remove_action_group (ui, editor->group);

	/* Disconnect signals */
	g_signal_handlers_disconnect_by_func (G_OBJECT (manager), G_CALLBACK (on_project_symbol_scanned), editor->query_project);
	g_signal_handlers_disconnect_by_func (G_OBJECT (manager), G_CALLBACK (on_system_symbol_scanned), editor->query_system);

	g_object_unref (editor->query_project);
	g_object_unref (editor->query_system);	
	
	return TRUE;
}

static void
dispose (GObject *obj)
{
	EditorPlugin *plugin = ANJUTA_PLUGIN_EDITOR (obj);

	g_object_unref (plugin->settings);

	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
finalize (GObject *obj)
{
	/* Finalization codes here */
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
editor_plugin_instance_init (GObject *obj)
{
	EditorPlugin *plugin = ANJUTA_PLUGIN_EDITOR (obj);

	plugin->settings = g_settings_new (PREF_SCHEMA);
	plugin->prefs = NULL;
}

static void
editor_plugin_class_init (GObjectClass *klass) 
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
	klass->finalize = finalize;
	
	signals[STYLE_CHANGED] = g_signal_new ("style-changed",
                                                G_OBJECT_CLASS_TYPE (object_class),
                                                G_SIGNAL_RUN_FIRST,
                                                G_STRUCT_OFFSET (EditorPluginClass, style_changed),
                                                NULL, NULL,
                                                g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static IAnjutaEditor*
itext_editor_factory_new_editor(IAnjutaEditorFactory* factory, 
								GFile* file,
								const gchar* filename, 
								GError** error)
{
	/* file can be NULL, if we open a buffer, not a file */
	gchar* uri = file ? g_file_get_uri (file) : NULL;
	IAnjutaEditor* editor = IANJUTA_EDITOR(text_editor_new(ANJUTA_PLUGIN (factory),
														   uri, filename));
	g_free(uri);
	return editor;
}

static void
itext_editor_factory_iface_init (IAnjutaEditorFactoryIface *iface)
{
	iface->new_editor = itext_editor_factory_new_editor;
}

static void
ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	GtkBuilder* bxml = gtk_builder_new ();
	EditorPlugin* plugin = ANJUTA_PLUGIN_EDITOR (ipref);
	GError* error = NULL;
	if (!gtk_builder_add_from_file (bxml, PREFS_GLADE, &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}
	plugin->prefs = prefs;
	plugin->style_button = GTK_WIDGET (gtk_builder_get_object (bxml, "style_button"));
	g_signal_connect(G_OBJECT(plugin->style_button), "clicked", 
		G_CALLBACK(on_style_button_clicked), plugin);
	anjuta_preferences_add_from_builder (prefs,
								 bxml, plugin->settings, "prefs_editor", _("Scintilla Editor"),  ICON_FILE);
	g_object_unref(bxml);
}

static void
ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	EditorPlugin* plugin = ANJUTA_PLUGIN_EDITOR (ipref);
	g_signal_handlers_disconnect_by_func(G_OBJECT(plugin->style_button), 
		G_CALLBACK(on_style_button_clicked), 
		anjuta_shell_get_preferences(ANJUTA_PLUGIN(plugin)->shell, NULL));
	
	anjuta_preferences_remove_page(prefs, _("Scintilla Editor"));
}

static void
ipreferences_iface_init(IAnjutaPreferencesIface* iface)
{
	iface->merge = ipreferences_merge;
	iface->unmerge = ipreferences_unmerge;	
}

ANJUTA_PLUGIN_BEGIN (EditorPlugin, editor_plugin);
ANJUTA_TYPE_ADD_INTERFACE(itext_editor_factory, IANJUTA_TYPE_EDITOR_FACTORY);
ANJUTA_TYPE_ADD_INTERFACE(ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (EditorPlugin, editor_plugin);
