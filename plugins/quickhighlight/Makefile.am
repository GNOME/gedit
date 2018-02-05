plugin_LTLIBRARIES += plugins/quickhighlight/libquickhighlight.la

plugins_quickhighlight_libquickhighlight_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
plugins_quickhighlight_libquickhighlight_la_LIBADD  =			\
	$(top_builddir)/gedit/libgedit.la				\
	$(GEDIT_LIBS)
plugins_quickhighlight_libquickhighlight_la_CPPFLAGS = -I$(top_srcdir)
plugins_quickhighlight_libquickhighlight_la_CFLAGS   =			\
	$(GEDIT_CFLAGS) 						\
	$(WARN_CFLAGS)							\
	$(DISABLE_DEPRECATED_CFLAGS)

plugins_quickhighlight_libquickhighlight_la_SOURCES =			\
	plugins/quickhighlight/gedit-quick-highlight-plugin.h		\
	plugins/quickhighlight/gedit-quick-highlight-plugin.c

plugin_in_files += plugins/quickhighlight/quickhighlight.plugin.desktop.in
