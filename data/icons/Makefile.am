iconscalabledir = $(datadir)/icons/hicolor/scalable/apps
iconscalable_DATA = data/icons/hicolor/scalable/apps/org.gnome.Gedit.svg
iconsymbolicdir = $(datadir)/icons/hicolor/symbolic/apps
iconsymbolic_DATA = data/icons/hicolor/symbolic/apps/org.gnome.Gedit-symbolic.svg

icon_files = \
	$(iconscalable_DATA) \
	$(iconsymbolic_DATA)

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

EXTRA_DIST += $(icon_files)
