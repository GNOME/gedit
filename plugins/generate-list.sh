#!/bin/sh
# SPDX-FileCopyrightText: 2020 Sébastien Wilmet <swilmet@gnome.org>
# SPDX-License-Identifier: GPL-3.0-or-later

# This script generates a Markdown file with the names and descriptions of all
# official gedit plugins.

write_list_for_plugins_dir() {
	plugins_dir=$1

	for plugin_desktop_file in `find "$plugins_dir" -name '*.plugin.desktop*'`
	do
		name=`grep -P '^Name=' "$plugin_desktop_file" | cut -d'=' -f2`
		echo -n "- **$name** - "

		desc=`grep -P '^Description=' "$plugin_desktop_file" | cut -d'=' -f2`
		echo "*$desc*"
	done | sort
}

write_content() {
	echo 'gedit plugins'
	echo '============='
	echo
	echo 'Core plugins'
	echo '------------'
	echo
	echo 'Plugins that are distributed with gedit itself.'
	echo

	write_list_for_plugins_dir '.'

	echo
	echo 'gedit-plugins package'
	echo '---------------------'
	echo
	echo 'The gedit-plugins package contains useful plugins that are (most'
	echo 'of the time) too specific to be distributed with gedit itself.'
	echo

	write_list_for_plugins_dir '../../gedit-plugins/plugins'

	echo
	echo 'Third-party plugins'
	echo '-------------------'
	echo
	echo 'There are also [third-party gedit plugins](third-party-plugins.md).'
}

write_content > list-of-gedit-plugins.md
