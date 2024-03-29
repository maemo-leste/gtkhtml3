# Basic setup

gtkhtml_data = $(datadir)/gtkhtml-@GTKHTML_API_VERSION@

gladedir = $(gtkhtml_data)
glade_DATA = gtkhtml-editor-properties.glade

if EDITOR_SHLIB
EDITOR_I18N_CFLAGS = $(I18N_LIB_CFLAGS)
else
EDITOR_I18N_CFLAGS =
endif

INCLUDES = \
	-I$(top_srcdir)/gtkhtml \
	-I$(srcdir) \
	-I$(includedir) \
	$(GTKHTML_CFLAGS) \
	$(EDITOR_I18N_CFLAGS) \
	-DG_LOG_DOMAIN=\"gtkhtml\" \
	-DSRCDIR=\"$(srcdir)\" \
	-DPREFIX=\"$(prefix)\"

AM_CFLAGS = \
	$(EDITOR_CFLAGS)				\
	-DICONDIR=\"$(gtkhtml_data)/icons\"		\
	-DGTKHTML_DATADIR=\""$(gtkhtml_data)"\"		\
	-DGNOMELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"\
	-DGLADE_DATADIR=\"$(gladedir)\"			\
	-DGDK_DISABLE_DEPRECATED=1			\
	-DG_DISABLE_DEPRECATED=1			\
	-DPREFIX=\""$(prefix)"\"			\
	-DSYSCONFDIR=\""$(sysconfdir)"\"		\
	-DDATADIR=\""$(datadir)"\"			\
	-DLIBDIR=\""$(datadir)"\"			\
	-DBONOBO_DISABLE_DEPRECATED=1

# CORBA stuff
IDLS = Editor.idl
IDL_GENERATED_H = \
	Editor.h
IDL_GENERATED_C = \
	Editor-common.c		\
	Editor-skels.c		\
	Editor-stubs.c
IDL_GENERATED = $(IDL_GENERATED_C) $(IDL_GENERATED_H)

SPELL_IDLS = Spell.idl
SPELL_IDL_GENERATED_H = \
	Spell.h
SPELL_IDL_GENERATED_C = \
	Spell-common.c		\
	Spell-skels.c		\
	Spell-stubs.c
SPELL_IDL_GENERATED = $(SPELL_IDL_GENERATED_C) $(SPELL_IDL_GENERATED_H)

$(IDL_GENERATED_H): $(IDLS)
	$(ORBIT_IDL) $(BONOBO_IDL_INCLUDES) $(srcdir)/Editor.idl
$(IDL_GENERATED_C): $(IDL_GENERATED_H)

$(SPELL_IDL_GENERATED_H): $(SPELL_IDLS)
	$(ORBIT_IDL) $(BONOBO_IDL_INCLUDES) $(srcdir)/Spell.idl
$(SPELL_IDL_GENERATED_C): $(SPELL_IDL_GENERATED_H)


LDADD = \
	$(top_builddir)/gtkhtml/libgtkhtml-@GTKHTML_API_VERSION@.la	\
	$(EDITOR_LIBS)

# -lpspell-modules -lpspell_aspell -laspell -lpspell -lstdc++
#	-lccmalloc -ldl

# The Editor Control

libgnome_gtkhtml_editor_la_SOURCES = \
	$(SPELL_IDL_GENERATED)			\
	$(IDL_GENERATED)			\
	body.h					\
	body.c					\
	cell.h					\
	cell.c					\
	control-data.c				\
	control-data.h				\
	dialog.c				\
	dialog.h				\
	editor-control-factory.c		\
	editor-control-factory.h		\
	engine.h				\
	engine.c				\
	html-stream-mem.h			\
	html-stream-mem.c			\
	image.c					\
	image.h					\
	link.c					\
	link.h					\
	menubar.c				\
	menubar.h				\
	paragraph.c				\
	paragraph.h				\
	paragraph-style.c			\
	paragraph-style.h			\
	persist-file.c				\
	persist-file.h				\
	persist-stream.c			\
	persist-stream.h			\
	popup.c					\
	popup.h					\
	properties.c				\
	properties.h				\
	replace.c				\
	replace.h				\
	rule.c					\
	rule.h					\
	search.c				\
	search.h				\
	spellchecker.c				\
	spellchecker.h				\
	table.c					\
	table.h					\
	template.c				\
	template.h				\
	text.c					\
	text.h					\
	toolbar.c				\
	toolbar.h				\
	utils.c					\
	utils.h					\
	gi-utils.h				\
	gi-colors.c				\
	gi-colors.h				\
	gi-color-palette.c			\
	gi-color-palette.h			\
	gi-color-group.c			\
	gi-color-group.h			\
	gi-combo-box.c				\
	gi-combo-box.h				\
	gi-color-combo.c			\
	gi-color-combo.h

if EDITOR_SHLIB
MY_PRGS =
MY_LIBS = libgnome-gtkhtml-editor-3.14.la
location = "${privlibdir_in_server_file}/libgnome-gtkhtml-editor-$(EDITOR_API_VERSION)$(SOEXT)"
else
MY_PRGS = gnome-gtkhtml-editor-3.14
MY_LIBS =
location = "${libexecdir}/$(MY_PRGS)$(EXEEXT)"
endif

libgnome_gtkhtml_editor_3_14_la_SOURCES = \
	editor-control-shlib.c

libgnome_gtkhtml_editor_3_14_la_LDFLAGS = \
	-avoid-version -module -no-undefined

libgnome_gtkhtml_editor_3_14_la_LIBADD = \
	$(LDADD)			\
	libgnome-gtkhtml-editor.la

gnome_gtkhtml_editor_3_14_LDADD = \
	$(LDADD)			\
	libgnome-gtkhtml-editor.la

gnome_gtkhtml_editor_3_14_SOURCES = \
	gnome-gtkhtml-editor.c

noinst_LTLIBRARIES = libgnome-gtkhtml-editor.la

if OS_WIN32
privlibdir_in_server_file = "../../../lib/gtkhtml"
else
privlibdir_in_server_file = "$(libdir)/gtkhtml"
endif
privlibdir = "$(libdir)/gtkhtml"
privlib_LTLIBRARIES = $(MY_LIBS)

noinst_PROGRAMS = test_editor

libexec_PROGRAMS = $(MY_PRGS)

serverdir = $(libdir)/bonobo/servers
server_in_files = GNOME_GtkHTML_Editor.server.in.in

server_DATA = $(server_in_files:.server.in.in=-$(EDITOR_API_VERSION).server)
%-$(EDITOR_API_VERSION).server: %.server
	mv $< $@

%.server.in: %.server.in.in
	sed -e "s|\@GNOME_GTKHTML_EDITOR_LOCATION\@|$(location)|g" \
	-e "s|\@EDITOR_API_VERSION\@|$(EDITOR_API_VERSION)|g" \
	-e "s|\@GNOME_GTKHTML_EDITOR_TYPE\@|$(GNOME_GTKHTML_EDITOR_TYPE)|g" $< > $@

@INTLTOOL_SERVER_RULE@

uidir = $(gtkhtml_data)
ui_in_files = GNOME_GtkHTML_Editor.xml.in GNOME_GtkHTML_Editor-emacs.xml.in
ui_DATA = $(ui_in_files:.xml.in=.xml)

%.xml: %.xml.in
if OS_WIN32
# Use a relative path to the icons dir in the xml files, so it works
# on Win32 where the installation path is freely chosen by the
# end-user. The path is relative to "$prefix/share/pixmaps" (see
# gnome-program.c:gnome_program_locate_file()).
	sed -e 's|\@ICONSDIR\@|../gtkhtml-@GTKHTML_API_VERSION@/icons|g' $< > $@
else
# On Unix we use an absolute path, as the paths to be used on the
# end-user systems are decided at build time.
	sed -e 's|\@ICONSDIR\@|$(gtkhtml_data)/icons|g' $< > $@
endif

# The Test program

test_editor_SOURCES =				\
	e-html-utils.c				\
	e-html-utils.h				\
	htmlsourceview.c			\
	htmlsourceview.h			\
	test-html-editor-control.c

test_editor_LDADD = libgnome-gtkhtml-editor.la $(LDADD)

# Extra stuff to distribute.

EXTRA_DIST =					\
	$(IDLS)					\
	$(SPELL_IDLS)				\
	ChangeLog				\
	gtkhtml-editor-properties.glade		\
	$(ui_in_files)				\
	$(server_in_files)

idldir = $(gtkhtml_data)
idl_DATA = $(IDLS)

dist-hook:
	cd $(distdir); rm -f $(BUILT_SOURCES)

BUILT_SOURCES = $(IDL_GENERATED) $(SPELL_IDL_GENERATED)

CLEANFILES = \
	$(server_DATA)					\
	$(ui_DATA)					\
	$(BUILT_SOURCES)

