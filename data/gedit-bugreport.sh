#!/bin/sh

PKG_CONFIG_MODULES="glib-2.0 gtk+-3.0 gtksourceview-4 \
		    pygobject-2.0 \
		    enchant iso-codes"

echo_padded ()
{
	echo -n "  - $1 "
	N=$(echo -n $1 | wc -m)
	while test $N -le 20
	do
		echo -n " "
		N=`expr $N + 1`
	done
}

echo "Active plugins:"
gsettings get org.gnome.gedit.plugins active-plugins			\
	| sed -r -e 's/^\[(.*)\]$/\1/' -e 's/,/\n/g'			\
	| sed -e 's/^.*$/  - \0/'
echo

# Manually installed plugins (in $HOME)
if [ -d $HOME/.local/share/gedit/plugins ]
then
	echo "Plugins in \$HOME:"
	ls $HOME/.local/share/gedit/plugins/*.plugin			\
		| sed -r -e 's#.*/([^/]*)\.plugin$#  - \1#'
else
	echo "No plugin installed in \$HOME."
fi
echo

echo "Module versions:"
if (which pkg-config > /dev/null)
then
	for i in $PKG_CONFIG_MODULES
	do
		echo_padded "`echo -n $i | sed -r -e 's/^(.*)-[0-9]\.[0-9]$/\1/'`"
		pkg-config --modversion $i 2>/dev/null || echo
	done
else
	echo "  pkg-config unavailable"
fi
echo

