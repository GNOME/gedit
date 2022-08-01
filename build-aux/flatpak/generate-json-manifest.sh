#!/bin/sh
# SPDX-FileCopyrightText: Copyright 2022 Jake Dane
# SPDX-License-Identifier: GPL-3.0-or-later

# yq: A portable command-line YAML processor.
# Currently hosted here: https://github.com/mikefarah/yq

# At the time of writing, some tools around Flatpak (for example gnome-builder)
# only supports the *.json format, but *.yml is easier to edit.

yq eval 'org.gnome.gedit.yml' --output-format json > 'org.gnome.gedit.json'
