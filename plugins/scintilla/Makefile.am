if HAVE_PLUGIN_SCINTILLA

SUBDIRS = scintilla properties

# Plugin glade file
editor_gladedir = $(anjuta_glade_dir)
editor_glade_DATA = anjuta-editor-scintilla.ui

# Plugin UI file
editor_uidir = $(anjuta_ui_dir)
editor_ui_DATA = anjuta-scintilla.xml

# Plugin Icon file
editor_pixmapsdir = $(anjuta_image_dir)
editor_pixmaps_DATA = \
	anjuta-editor-scintilla-plugin.svg \
	anjuta-editor-scintilla-plugin-48.png

plugin_in_files = anjuta-editor.plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

editor_plugindir = $(anjuta_plugin_dir)
editor_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

plugindir = $(anjuta_plugin_dir)
plugin_LTLIBRARIES = libanjuta-editor.la

AM_CPPFLAGS = \
	-I$(srcdir)/scintilla/include \
	$(LIBANJUTA_CFLAGS) \
	$(EXTRA_CFLAGS) \
	-DG_LOG_DOMAIN=\"libanjuta-scintilla\"

libanjuta_editor_la_LDFLAGS = $(ANJUTA_PLUGIN_LDFLAGS)

libanjuta_editor_la_LIBADD = \
	$(LIBANJUTA_LIBS)\
	scintilla/libanjuta-scintilla.la

libanjuta_editor_la_SOURCES= \
	properties.cxx \
	properties.h \
	aneditor.h \
	aneditor-priv.h \
	aneditor.cxx \
	aneditor-autocomplete.cxx \
	aneditor-calltip.cxx \
	aneditor-indent.cxx \
	text_editor_prefs.c \
	text_editor_prefs.h \
	text_editor.c \
	text_editor.h \
	text_editor_cbs.c \
	text_editor_cbs.h \
	text-editor-iterable.c \
	text-editor-iterable.h \
	lexer.h \
	plugin.c \
	plugin.h \
	style-editor.c \
	style-editor.h \
	print.c \
	print.h \
	PropSetFile.h \
	PropSetFile.cxx \
	PropSet.h \
	SString.h \
	FilePath.cxx \
	FilePath.h \
	anjuta-bookmark-16.xpm \
	anjuta-breakpoint-disabled-16.xpm \
	anjuta-breakpoint-enabled-16.xpm \
	anjuta-pcmark-16.xpm \
	anjuta-linemark-16.xpm

gsettings_in_file = org.gnome.anjuta.plugins.scintilla.gschema.xml.in
gsettings_SCHEMAS = $(gsettings_in_file:.xml.in=.xml)
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

endif

EXTRA_DIST = \
	$(editor_pixmaps_DATA) \
	$(plugin_in_files) \
	$(editor_plugin_DATA) \
	$(editor_ui_DATA) \
	$(editor_ui_DATA) \
	$(editor_glade_DATA) \
	$(gsettings_in_file) \
	License.txt

CLEANFILES = \
        $(gsettings_SCHEMAS)
