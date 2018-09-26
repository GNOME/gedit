bin_PROGRAMS += gedit/gedit

pkglib_LTLIBRARIES += gedit/libgedit.la

gedit_common_cppflags =							\
	-I$(top_srcdir)							\
	-I$(top_srcdir)/gedit						\
	-I$(top_builddir)/gedit						\
	-I$(top_srcdir)/libgd						\
	-DDATADIR=\""$(datadir)"\"					\
	-DLIBDIR=\""$(libdir)"\"

gedit_common_cflags =							\
	$(GEDIT_CFLAGS)							\
	$(GTK_MAC_CFLAGS)						\
	$(WARN_CFLAGS)							\
	$(DISABLE_DEPRECATED_CFLAGS)					\
	$(INTROSPECTION_CFLAGS)

gedit_gedit_CPPFLAGS = $(gedit_common_cppflags)
gedit_gedit_CFLAGS = $(gedit_common_cflags)

gedit_gedit_LDADD =			\
	gedit/libgedit.la		\
	$(GEDIT_LIBS)			\
	$(GTK_MAC_LIBS)			\
	$(INTROSPECTION_LIBS)

gedit_gedit_SOURCES = gedit/gedit.c

gedit_gedit_LDFLAGS = -export-dynamic -no-undefined -export-symbols-regex "^[[^_]].*"

gedit_libgedit_la_CPPFLAGS = $(gedit_common_cppflags)
gedit_libgedit_la_CFLAGS = $(gedit_common_cflags)
gedit_libgedit_la_LDFLAGS = -avoid-version -export-dynamic -no-undefined -export-symbols-regex "^[^_].*"

# GEDIT_LIBS must be the last to ensure correct order on some platforms
gedit_libgedit_la_LIBADD = 			\
	$(top_builddir)/libgd/libgd.la		\
	$(GEDIT_LIBS)				\
	$(GTK_MAC_LIBS)

# Mac OSX convenience library and ldflags
if OS_OSX
noinst_LTLIBRARIES += gedit/libosx.la

gedit_libosx_la_LDFLAGS = -framework Cocoa
gedit_libosx_la_LIBADD = -lobjc
gedit_libosx_la_CPPFLAGS = $(gedit_common_cppflags)
gedit_libosx_la_CFLAGS = -xobjective-c $(gedit_common_cflags)

gedit_libosx_la_SOURCES = 			\
	gedit/gedit-app-osx.c			\
	gedit/gedit-app-osx.h			\
	gedit/gedit-file-chooser-dialog-osx.c	\
	gedit/gedit-file-chooser-dialog-osx.h

gedit_libgedit_la_LIBADD += gedit/libosx.la
endif

# Win32 convenience library and ldflags
if OS_WIN32
gedit_gedit_LDFLAGS += -mwindows

noinst_LTLIBRARIES += gedit/libwin32.la

gedit_libwin32_la_SOURCES =	\
	gedit/gedit-app-win32.c	\
	gedit/gedit-app-win32.h

gedit_libwin32_la_CPPFLAGS = $(gedit_common_cppflags)
gedit_libwin32_la_CFLAGS = $(gedit_common_cflags)

gedit_libgedit_la_LIBADD += gedit/libwin32.la
gedit/gedit-res.o: gedit/gedit.rc
	$(WINDRES) -i $(top_srcdir)/gedit/gedit.rc --input-format=rc -o gedit/gedit-res.o -O coff

gedit_gedit_LDADD += gedit/gedit-res.o
endif

# X11 convenience library
if GDK_WINDOWING_X11
noinst_LTLIBRARIES += gedit/libx11.la

gedit_libx11_la_SOURCES =		\
	gedit/gedit-app-x11.c		\
	gedit/gedit-app-x11.h

gedit_libx11_la_CPPFLAGS = $(gedit_common_cppflags)
gedit_libx11_la_CFLAGS = $(gedit_common_cflags)

gedit_libgedit_la_LIBADD += gedit/libx11.la
endif

gedit_built_sources =			\
	gedit/gedit-enum-types.c	\
	gedit/gedit-enum-types.h

gedit_NOINST_H_FILES =					\
	gedit/gedit-app-private.h			\
	gedit/gedit-close-confirmation-dialog.h		\
	gedit/gedit-commands-private.h			\
	gedit/gedit-dirs.h				\
	gedit/gedit-document-private.h			\
	gedit/gedit-documents-panel.h			\
	gedit/gedit-encoding-items.h			\
	gedit/gedit-encodings-dialog.h			\
	gedit/gedit-file-chooser-dialog-gtk.h		\
	gedit/gedit-file-chooser-dialog.h		\
	gedit/gedit-highlight-mode-dialog.h		\
	gedit/gedit-highlight-mode-selector.h		\
	gedit/gedit-history-entry.h			\
	gedit/gedit-io-error-info-bar.h			\
	gedit/gedit-menu-stack-switcher.h		\
	gedit/gedit-metadata-manager.h			\
	gedit/gedit-multi-notebook.h			\
	gedit/gedit-notebook.h				\
	gedit/gedit-notebook-popup-menu.h		\
	gedit/gedit-notebook-stack-switcher.h		\
	gedit/gedit-open-document-selector.h		\
	gedit/gedit-open-document-selector-helper.h	\
	gedit/gedit-open-document-selector-store.h	\
	gedit/gedit-plugins-engine.h			\
	gedit/gedit-preferences-dialog.h		\
	gedit/gedit-print-job.h				\
	gedit/gedit-print-preview.h			\
	gedit/gedit-recent.h				\
	gedit/gedit-replace-dialog.h			\
	gedit/gedit-settings.h				\
	gedit/gedit-status-menu-button.h		\
	gedit/gedit-tab-label.h				\
	gedit/gedit-tab-private.h			\
	gedit/gedit-view-centering.h			\
	gedit/gedit-view-frame.h			\
	gedit/gedit-window-private.h

gedit_INST_H_FILES =				\
	gedit/gedit-app-activatable.h		\
	gedit/gedit-app.h			\
	gedit/gedit-commands.h			\
	gedit/gedit-debug.h			\
	gedit/gedit-document.h 			\
	gedit/gedit-encodings-combo-box.h	\
	gedit/gedit-menu-extension.h		\
	gedit/gedit-message-bus.h		\
	gedit/gedit-message.h			\
	gedit/gedit-progress-info-bar.h		\
	gedit/gedit-statusbar.h			\
	gedit/gedit-tab.h 			\
	gedit/gedit-utils.h 			\
	gedit/gedit-view-activatable.h		\
	gedit/gedit-view.h 			\
	gedit/gedit-window-activatable.h	\
	gedit/gedit-window.h

gedit_headerdir = $(prefix)/include/gedit-$(GEDIT_API_VERSION)/gedit
gedit_header_HEADERS = $(gedit_INST_H_FILES)

gedit_libgedit_c_files =				\
	gedit/gedit-app-activatable.c			\
	gedit/gedit-app.c				\
	gedit/gedit-close-confirmation-dialog.c		\
	gedit/gedit-commands-documents.c		\
	gedit/gedit-commands-edit.c			\
	gedit/gedit-commands-file.c			\
	gedit/gedit-commands-file-print.c		\
	gedit/gedit-commands-help.c			\
	gedit/gedit-commands-search.c			\
	gedit/gedit-commands-view.c			\
	gedit/gedit-debug.c				\
	gedit/gedit-dirs.c				\
	gedit/gedit-document.c 				\
	gedit/gedit-documents-panel.c			\
	gedit/gedit-encoding-items.c			\
	gedit/gedit-encodings-combo-box.c		\
	gedit/gedit-encodings-dialog.c			\
	gedit/gedit-file-chooser-dialog.c		\
	gedit/gedit-file-chooser-dialog-gtk.c		\
	gedit/gedit-highlight-mode-dialog.c		\
	gedit/gedit-highlight-mode-selector.c		\
	gedit/gedit-history-entry.c			\
	gedit/gedit-io-error-info-bar.c			\
	gedit/gedit-menu-extension.c			\
	gedit/gedit-menu-stack-switcher.c		\
	gedit/gedit-message-bus.c			\
	gedit/gedit-message.c				\
	gedit/gedit-metadata-manager.c			\
	gedit/gedit-multi-notebook.c			\
	gedit/gedit-notebook.c				\
	gedit/gedit-notebook-popup-menu.c		\
	gedit/gedit-notebook-stack-switcher.c		\
	gedit/gedit-open-document-selector.c		\
	gedit/gedit-open-document-selector-helper.c	\
	gedit/gedit-open-document-selector-store.c	\
	gedit/gedit-plugins-engine.c			\
	gedit/gedit-preferences-dialog.c		\
	gedit/gedit-print-job.c				\
	gedit/gedit-print-preview.c			\
	gedit/gedit-progress-info-bar.c			\
	gedit/gedit-recent.c				\
	gedit/gedit-replace-dialog.c			\
	gedit/gedit-resources.c				\
	gedit/gedit-settings.c				\
	gedit/gedit-statusbar.c				\
	gedit/gedit-status-menu-button.c		\
	gedit/gedit-tab.c 				\
	gedit/gedit-tab-label.c				\
	gedit/gedit-utils.c 				\
	gedit/gedit-view-activatable.c			\
	gedit/gedit-view.c 				\
	gedit/gedit-view-centering.c			\
	gedit/gedit-view-frame.c			\
	gedit/gedit-window-activatable.c		\
	gedit/gedit-window.c

gedit_libgedit_la_SOURCES = 		\
	$(gedit_built_sources)		\
	$(gedit_libgedit_c_files)	\
	$(gedit_NOINST_H_FILES)		\
	$(gedit_INST_H_FILES)

gedit_ENUM_TYPES = 		\
	gedit/gedit-notebook.h	\
	$(gedit_INST_H_FILES)

gedit/gedit-enum-types.h: gedit/gedit-enum-types.h.template $(gedit_ENUM_TYPES) $(GLIB_MKENUMS)
	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) --template gedit/gedit-enum-types.h.template $(gedit_ENUM_TYPES)) > $@

gedit/gedit-enum-types.c: gedit/gedit-enum-types.c.template $(gedit_ENUM_TYPES) $(GLIB_MKENUMS)
	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) --template gedit/gedit-enum-types.c.template $(gedit_ENUM_TYPES)) > $@

gedit_resource_deps = $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir $(srcdir)/gedit/resources --sourcedir $(builddir)/gedit/resources $(builddir)/gedit/resources/gedit.gresource.xml)

gedit_resources_c_deps = $(builddir)/gedit/resources/gedit.gresource.xml $(gedit_resource_deps)

gedit/gedit-resources.c: $(gedit_resources_c_deps)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES)				\
		--target="$@"						\
		--sourcedir="$(srcdir)/gedit/resources"			\
		--sourcedir="$(builddir)/gedit/resources"		\
		--generate-source "$(builddir)/gedit/resources/gedit.gresource.xml"

if ENABLE_PYTHON
gedit_overridesdir = $(pyoverridesdir)
gedit_overrides_PYTHON = gedit/Gedit.py
endif

gedit_dist_resource_deps = 							\
	$(filter-out %gedit/resources/css/gedit-style-os.css,$(gedit_resource_deps))	\
	gedit/resources/css/gedit-style-osx.css

EXTRA_DIST += 					\
	gedit/gedit-enum-types.h.template	\
	gedit/gedit-enum-types.c.template	\
	gedit/gedit.rc				\
	gedit/Gedit-3.0.metadata		\
	gedit/resources/gedit.gresource.xml.in	\
	gedit/resources/pixmaps/gedit.ico	\
	$(gedit_dist_resource_deps)

if HAVE_INTROSPECTION
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS = gedit/Gedit-3.0.gir
INTROSPECTION_SCANNER_ENV = CC="$(CC)"

gedit/Gedit-3.0.gir: gedit/gedit$(EXEEXT)
INTROSPECTION_SCANNER_ARGS = -I$(top_srcdir) --warn-all

gedit_Gedit_3_0_gir_NAMESPACE = Gedit
gedit_Gedit_3_0_gir_VERSION = 3.0
gedit_Gedit_3_0_gir_PROGRAM = $(builddir)/gedit/gedit$(EXEEXT)
gedit_Gedit_3_0_gir_INCLUDES = Gtk-3.0 GtkSource-4
gedit_Gedit_3_0_gir_EXPORT_PACKAGES = gedit
gedit_Gedit_3_0_gir_SCANNERFLAGS = $(GEDIT_CFLAGS) $(foreach header,$(gedit_INST_H_FILES),--c-include="$(header)")
gedit_Gedit_3_0_gir_FILES = 		\
	$(gedit_INST_H_FILES) 		\
	$(gedit_libgedit_c_files)	\
	$(gedit_built_sources)

gedit_girdir = $(datadir)/gedit/gir-1.0
gedit_gir_DATA = $(INTROSPECTION_GIRS)

gedit_typelibdir = $(libdir)/gedit/girepository-1.0
gedit_typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES +=			\
	$(gedit_gir_DATA)	\
	$(gedit_typelib_DATA)
endif

if ENABLE_VAPIGEN
include $(VAPIGEN_MAKEFILE)

gedit/gedit.vapi: gedit/Gedit-3.0.gir

VAPIGEN_VAPIS = gedit/gedit.vapi

gedit_gedit_vapi_DEPS = atk gdk-3.0 gio-2.0 gtk+-3.0 gtksourceview-4
gedit_gedit_vapi_METADATADIRS = $(top_srcdir)/gedit
gedit_gedit_vapi_FILES = gedit/Gedit-3.0.gir

vapidir = $(datadir)/vala/vapi
vapi_DATA = $(VAPIGEN_VAPIS) gedit/gedit.deps

gedit/gedit.deps:
	$(AM_V_GEN) echo $(gedit_gedit_vapi_DEPS) | tr ' ' '\n' > $@

CLEANFILES += $(VAPIGEN_VAPIS) gedit/gedit.deps gedit/gedit.deps

endif # ENABLE_VAPIGEN

install-exec-hook:
if PLATFORM_WIN32
	$(mkinstalldirs) "$(DESTDIR)$(libdir)/gedit"
	mv -t $(DESTDIR)$(libdir)/gedit $(DESTDIR)$(libdir)/bin/*
	rm -rf $(DESTDIR)$(libdir)/bin
else
	rm -f $(DESTDIR)$(bindir)/gnome-text-editor
	ln -s gedit $(DESTDIR)$(bindir)/gnome-text-editor
endif

BUILT_SOURCES += $(gedit_built_sources) gedit/gedit-resources.c
