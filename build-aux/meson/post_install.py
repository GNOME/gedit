#!/usr/bin/env python3

import os
import subprocess
import sys

libdir = sys.argv[1]

# Packaging tools define DESTDIR and this isn't needed for them
if 'DESTDIR' not in os.environ:
    print('Compiling python modules...')
    subprocess.call([sys.executable, '-m', 'compileall', '-f', '-q',
                     os.path.join(libdir, 'gedit', 'plugins')])

    print('Compiling python modules (optimized versions) ...')
    subprocess.call([sys.executable, '-O', '-m', 'compileall', '-f', '-q',
                     os.path.join(libdir, 'gedit', 'plugins')])
