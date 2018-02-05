plugindir = $(GEDIT_PLUGINS_LIBS_DIR)

plugin_in_files               =
dist_plugin_in_files          =
plugin_LTLIBRARIES            =
plugin_gsettings_SCHEMAS      =
dist_plugin_gsettings_SCHEMAS =

include plugins/checkupdate/Makefile.am
include plugins/docinfo/Makefile.am
include plugins/externaltools/Makefile.am
include plugins/filebrowser/Makefile.am
include plugins/modelines/Makefile.am
include plugins/pythonconsole/Makefile.am
include plugins/quickhighlight/Makefile.am
include plugins/quickopen/Makefile.am
include plugins/snippets/Makefile.am
include plugins/sort/Makefile.am
include plugins/spell/Makefile.am
include plugins/time/Makefile.am

gsettings_SCHEMAS += $(plugin_gsettings_SCHEMAS)

%.plugin: %.plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po)
	$(AM_V_GEN) \
	$(MKDIR_P) "$(dir $@)"; \
	$(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

GENERATED_PLUGIN_FILES = $(plugin_in_files:.plugin.desktop.in=.plugin)

plugin_DATA = $(GENERATED_PLUGIN_FILES)

DISTCLEANFILES +=			\
	$(GENERATED_PLUGIN_FILES)

CLEANFILES +=				\
	$(GENERATED_PLUGIN_FILES)

EXTRA_DIST +=									\
	$(plugin_in_files)							\
	$(dist_plugin_in_files)							\
	$(plugin_gsettings_SCHEMAS:%.gschema.xml=%.gschema.xml.in)		\
	$(dist_plugin_gsettings_SCHEMAS:%.gschema.xml=%.gschema.xml.in)
