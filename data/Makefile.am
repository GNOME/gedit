data_desktopdir = $(datadir)/applications
data_desktop_in_files = data/org.gnome.gedit.desktop.in
data_desktop_DATA = $(data_desktop_in_files:.desktop.in=.desktop)

$(data_desktop_DATA): $(data_desktop_in_files) $(wildcard $(top_srcdir)/po/*po)
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

data_appdatadir = $(datadir)/metainfo
data_appdata_in_files = data/org.gnome.gedit.appdata.xml.in
data_appdata_DATA = $(data_appdata_in_files:.xml.in=.xml)

$(data_appdata_DATA): $(data_appdata_in_files) $(wildcard $(top_srcdir)/po/*po)
	$(AM_V_GEN)$(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@

data_servicedir = $(datadir)/dbus-1/services
data_service_in_files = data/org.gnome.gedit.service.in
data_service_DATA = $(data_service_in_files:.service.in=.service)

$(data_service_DATA): $(data_service_in_files) Makefile
	@sed -e "s|\@bindir\@|$(bindir)|" $<> $@

dist_man_MANS = data/gedit.1

data_pkgconfigdir = $(libdir)/pkgconfig
data_pkgconfig_DATA = data/gedit.pc

data_bugreportdir = $(libexecdir)/gedit
dist_data_bugreport_SCRIPTS = data/gedit-bugreport.sh

gsettings_SCHEMAS += data/org.gnome.gedit.gschema.xml
gsettings_ENUMS += data/org.gnome.gedit.enums.xml
data/org.gnome.gedit.enums.xml: $(top_srcdir)/gedit/*.h

if OS_OSX
dist_gsettingsschema_DATA += data/gedit-osx.gschema.override
endif

data_convertdir = $(datadir)/GConf/gsettings
dist_data_convert_DATA = data/gedit.convert

EXTRA_DIST += 					\
	$(data_desktop_in_files)		\
	$(data_appdata_in_files)		\
	$(data_service_in_files)		\
	data/org.gnome.gedit.gschema.xml.in	\
	data/gedit.pc.in

CLEANFILES +=	 			\
	$(data_appdata_DATA)		\
	$(data_desktop_DATA)		\
	$(data_service_DATA)		\
	$(data_pkgconfig_DATA)

DISTCLEANFILES +=	 		\
	$(data_desktop_DATA)		\
	$(data_service_DATA)		\
	$(data_pkgconfig_DATA)
