#!/bin/sh
# generate-manifest.sh: Generate JSON manifest from YAML manifest.
#
# SPDX-FileCopyrightText: Copyright 2022 Jake Dane
# SPDX-License-Identifier: GPL-3.0-or-later
set -e

YAML_FILE='org.gnome.gedit.yml'
JSON_FILE='org.gnome.gedit.json'

if ! type yq &>/dev/null; then
	echo "command 'yq' not found" 1>&2
	exit 1
fi

if [ ! -e "$YAML_FILE" ]; then
	echo "'$YAML_FILE' manifest not found" 1>&2
	exit 1
fi

temp_file=`mktemp`
yq '.' "$YAML_FILE" > "$temp_file"
if ! diff -q "$temp_file" "$JSON_FILE" &>/dev/null; then
	mv -f "$temp_file" "$JSON_FILE"
	echo "generated '$JSON_FILE' manifest"
else
	rm -f "$temp_file"
	echo "nothing to do"
fi
