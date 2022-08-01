#!/bin/sh
# SPDX-FileCopyrightText: Copyright 2022 Jake Dane
# SPDX-License-Identifier: GPL-3.0-or-later

# yq: A portable command-line YAML processor.
# Currently hosted here: https://github.com/mikefarah/yq

yq eval 'org.gnome.gedit.yml' --output-format json > 'org.gnome.gedit.json'
