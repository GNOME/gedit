#!/usr/bin/env python

from xml.etree import ElementTree
import sys
import re
import os
from optparse import OptionParser
import pwd
import datetime

class ParamSpec:
    def __init__(self, name, nick, desc, flags, **kwargs):
        self.name = name
        self.nick = nick
        self.desc = desc
        self.flags = flags

        self.args = []

    def finalizer(self, container):
        return None

    def get_value(self, val, container):
        return 'g_value_set_pointer (%s, %s->%s);' % (val, container, self.cname())

    def set_value(self, val, container):
        return '%s->%s = g_value_get_pointer (%s);' % (container, self.cname(), val)

    def prop_enum(self):
        return 'PROP_' + self.name.upper().replace('-', '_')

    def cname(self):
        return self.name.replace('-', '_')

    def ctype(self):
        return 'gpointer'

    def spec_name(self):
        name = self.__class__.__name__

        if name.startswith('ParamSpec'):
            name = name[len('ParamSpec'):]

        return name.lower()

    def format_str(self, s):
        if s.startswith('"') or s.startswith('_("'):
            return s

        return '"%s"' % (s.replace('"', '\\"'),)

    def __str__(self):
        name = "g_param_spec_" + self.spec_name()
        indent = " " * (len(name) + 2)

        argstr = ''

        if self.args:
            ret = (",\n%s" % (indent,)).join(str(x) for x in self.args)
            argstr = "\n%s%s," % (indent, ret)

        flags = (' |\n%s' % (indent,)).join(x.strip() for x in self.flags.split('|'))

        return "%s (%s,\n%s%s,\n%s%s,%s\n%s%s)" % (name,
                                                   self.format_str(self.name),
                                                   indent,
                                                   self.format_str(self.nick),
                                                   indent,
                                                   self.format_str(self.desc),
                                                   argstr,
                                                   indent,
                                                   flags)

    def write(self):
        delim = "\n" + (" " * 33)
        spec = delim.join(str(self).splitlines())

        return """
g_object_class_install_property (object_class,
                                 %s,
                                 %s);""" % (self.prop_enum(), spec)

class ParamSpecTyped(ParamSpec):
    def __init__(self, name, nick, desc, flags, **kwargs):
        ParamSpec.__init__(self, name, nick, desc, flags, **kwargs)

        if 'ctype' in kwargs:
            self._ctype = kwargs['ctype']
        else:
            parts = kwargs['gtype'].split('_')
            del parts[1]

            self._ctype = '%s *' % (''.join(x.title() for x in parts),)

        self.args.append(kwargs['gtype'])

    def ctype(self):
        return self._ctype

class ParamSpecBoolean(ParamSpec):
    def __init__(self, name, nick, desc, flags, **kwargs):
        ParamSpec.__init__(self, name, nick, desc, flags, **kwargs)

        if 'default' in kwargs:
            if kwargs['default'].lower() in ['true', '1']:
                self.args = ['TRUE']
            else:
                self.args = ['FALSE']
        else:
            self.args = ['FALSE']

    def get_value(self, val, container):
        return 'g_value_set_boolean (%s, %s->%s);' % (val, container, self.cname())

    def set_value(self, val, container):
        return '%s->%s = g_value_get_boolean (%s);' % (container, self.cname(), val)

    def ctype(self):
        return 'gboolean'

class ParamSpecBoxed(ParamSpecTyped):
    def __init__(self, name, nick, desc, flags, **kwargs):
        ParamSpecTyped.__init__(self, name, nick, desc, flags, **kwargs)

    def finalizer(self, container):
        return 'if (%s->%s != NULL)\n{\n\tg_boxed_free (%s->%s);\n}' % (container, self.cname(), container, self.cname())

    def get_value(self, val, container):
        return 'g_value_set_boxed (%s, %s->%s);' % (val, container, self.cname())

    def set_value(self, val, container):
        return '%s->%s = g_value_dup_boxed (%s);' % (container, self.cname(), val)

class ParamSpecNumeric(ParamSpec):
    def __init__(self, name, nick, desc, flags, **kwargs):
        ParamSpec.__init__(self, name, nick, desc, flags, **kwargs)

        if 'min' in kwargs:
            minv = kwargs['min']
        else:
            minv = self.min_value()

        if 'max' in kwargs:
            maxv = kwargs['max']
        else:
            maxv = self.max_value()

        if 'default' in kwargs:
            default = kwargs['default']
        else:
            default = self.default_value()

        self.args = [minv, maxv, default]

    def min_value(self):
        return '0'

    def max_value(self):
        return '0'

    def default_value(self):
        return '0'

class ParamSpecDouble(ParamSpecNumeric):
    def __init__(self, name, nick, desc, flags, **kwargs):
        ParamSpecNumeric.__init__(self, name, nick, desc, flags, **kwargs)

    def min_value(self):
        return 'G_MINDOUBLE'

    def max_value(self):
        return 'G_MAXDOUBLE'

    def ctype(self):
        return 'gdouble'

    def get_value(self, val, container):
        return 'g_value_set_double (%s, %s->%s);' % (val, container, self.cname())

    def set_value(self, val, container):
        return '%s->%s = g_value_get_double (%s);' % (container, self.cname(), val)

class ParamSpecEnum(ParamSpecTyped):
    def __init__(self, name, nick, desc, flags, **kwargs):
        ParamSpecTyped.__init__(self, name, nick, desc, flags, **kwargs)

        if 'default' in kwargs:
            default = kwargs['default']
        else:
            default = '0'

        self.args.append(default)

    def get_value(self, val, container):
        return 'g_value_set_enum (%s, %s->%s);' % (val, container, self.cname())

    def set_value(self, val, container):
        return '%s->%s = g_value_get_enum (%s);' % (container, self.cname(), val)

class ParamSpecFlags(ParamSpecEnum):
    def __init__(self, name, nick, desc, flags, **kwargs):
        ParamSpecEnum.__init__(self, name, nick, desc, flags, **kwargs)

    def get_value(self, val, container):
        return 'g_value_set_flags (%s, %s->%s);' % (val, container, self.cname())

    def set_value(self, val, container):
        return '%s->%s = g_value_get_flags (%s);' % (container, self.cname(), val)

class ParamSpecFloat(ParamSpecNumeric):
    def __init__(self, name, nick, desc, flags, **kwargs):
        ParamSpecNumeric.__init__(self, name, nick, desc, flags, **kwargs)

    def min_value(self):
        return 'G_MINFLOAT'

    def max_value(self):
        return 'G_MAXFLOAT'

    def ctype(self):
        return 'gfloat'

    def get_value(self, val, container):
        return 'g_value_set_float (%s, %s->%s);' % (val, container, self.cname())

    def set_value(self, val, container):
        return '%s->%s = g_value_get_float (%s);' % (container, self.cname(), val)

class ParamSpecInt(ParamSpecNumeric):
    def __init__(self, name, nick, desc, flags, **kwargs):
        ParamSpecNumeric.__init__(self, name, nick, desc, flags, **kwargs)

    def min_value(self):
        return 'G_MININT'

    def max_value(self):
        return 'G_MAXINT'

    def ctype(self):
        return 'gint'

    def get_value(self, val, container):
        return 'g_value_set_int (%s, %s->%s);' % (val, container, self.cname())

    def set_value(self, val, container):
        return '%s->%s = g_value_get_int (%s);' % (container, self.cname(), val)

class ParamSpecUInt(ParamSpecNumeric):
    def __init__(self, name, nick, desc, flags, **kwargs):
        ParamSpecNumeric.__init__(self, name, nick, desc, flags, **kwargs)

    def min_value(self):
        return '0'

    def max_value(self):
        return 'G_MAXUINT'

    def ctype(self):
        return 'guint'

    def get_value(self, val, container):
        return 'g_value_set_uint (%s, %s->%s);' % (val, container, self.cname())

    def set_value(self, val, container):
        return '%s->%s = g_value_get_uint (%s);' % (container, self.cname(), val)

class ParamSpecObject(ParamSpecTyped):
    def __init__(self, name, nick, desc, flags, **kwargs):
        ParamSpecTyped.__init__(self, name, nick, desc, flags, **kwargs)

    def finalizer(self, container):
        return 'if (%s->%s)\n{\n\tg_object_unref (%s->%s);\n}' % (container, self.cname(), container, self.cname())

    def get_value(self, val, container):
        return 'g_value_set_object (%s, %s->%s);' % (val, container, self.cname())

    def set_value(self, val, container):
        return '%s->%s = g_value_dup_object (%s);' % (container, self.cname(), val)

class ParamSpecPointer(ParamSpec):
    def __init__(self, name, nick, desc, flags, **kwargs):
        ParamSpec.__init__(self, name, nick, desc, flags, **kwargs)

class ParamSpecString(ParamSpec):
    def __init__(self, name, nick, desc, flags, **kwargs):
        ParamSpec.__init__(self, name, nick, desc, flags, **kwargs)

        if 'default' in kwargs:
            default = kwargs['default']
        else:
            default = 'NULL'

        if not default.startswith('"') and not default.startswith('_("') and default != 'NULL':
            default = '"' + default.replace('"', '\\"') + '"'

        self.args.append(default)

    def ctype(self):
        return 'gchar *'

    def finalizer(self, container):
        return 'g_free (%s->%s);' % (container, self.cname())

    def get_value(self, val, container):
        return 'g_value_set_string (%s, %s->%s);' % (val, container, self.cname())

    def set_value(self, val, container):
        return '%s->%s = g_value_dup_string (%s);' % (container, self.cname(), val)

_prop_types = {
    'boolean': ParamSpecBoolean,
    'boxed': ParamSpecBoxed,
    'double': ParamSpecDouble,
    'enum': ParamSpecEnum,
    'flags': ParamSpecFlags,
    'float': ParamSpecFloat,
    'int': ParamSpecInt,
    'object': ParamSpecObject,
    'pointer': ParamSpecPointer,
    'string': ParamSpecString,
    'uint': ParamSpecUInt
}

GPL = """
/*
 * #FILENAME
 * This file is part of #PROGRAM
 *
 * Copyright (C) #YEAR - #AUTHOR
 *
 * #PROGRAM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * #PROGRAM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with #PROGRAM; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */
"""

LGPL = """
/*
 * #FILENAME
 * This file is part of #PROGRAM
 *
 * Copyright (C) #YEAR - #AUTHOR
 *
 * ${2} is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * #PROGRAM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
"""

class Include:
    def __init__(self, elem):
        self.path = elem.text
        self.is_system = elem.get('system') != None

    def __str__(self):
        if self.is_system:
            return '#include <%s>' % (self.path,)
        else:
            return '#include "%s"' % (self.path,)

class Message:
    def __init__(self, elem):
        self.name = elem.get('name')
        self.namespace = elem.get('namespace')

        if not self.name:
            sys.stderr.write("Name for message is required...\n")
            return

        if not self.namespace:
            sys.stderr.write("Namespace for message is required (%s)...\n" % (self.name,))

        self.properties = []
        self.includes = []

        for prop in elem.findall('property'):
            self.add_prop(prop)

        for inc in elem.findall('include'):
            self.includes.append(Include(inc))

        self.setup()

    def setup(self):
        parts = [x.group(0) for x in re.finditer('[A-Z]+[a-z0-9]*', self.name)]

        self.ns_upper = self.namespace.upper()
        self.ns_lower = self.namespace.lower()

        self.name_upper = '_'.join(x.upper() for x in parts)
        self.name_lower = self.name_upper.lower()

        self.cname_upper = '%s_%s' % (self.ns_upper, self.name_upper)
        self.cname_lower = self.cname_upper.lower()

        self.ctype = '%s_%s_%s' % (self.ns_upper, 'TYPE', self.name_upper)
        self.cclass = '%s%sClass' % (self.namespace, self.name)

        self.cobj = '%s%s' % (self.namespace, self.name)
        self.filename = '%s-%s' % (self.ns_lower, '-'.join(x.lower() for x in parts))

    def normalize_name(self, name):
        if not name:
            return name

        return name.replace('_', '-')

    def parse_flags(self, flags):
        return ' | '.join('G_PARAM_' + x.strip().replace('-', '_').upper() for x in flags.split('|'))

    def add_prop(self, elem):
        name = self.normalize_name(elem.get('name'))
        typ = elem.get('type')

        if not name:
            sys.stderr.write("Name for property not specified...\n")
            return False

        if not typ:
            sys.stderr.write("Type for property `%s' not specified...\n" % (name,))
            return False

        if not typ in _prop_types:
            sys.stderr.write("Invalid property type `%s'...\n" % (typ,))

        blurb = elem.get('blurb')
        nick = elem.get('nick')
        flags = elem.get('flags')

        if not flags:
            flags = 'G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS'
        else:
            flags = self.parse_flags(flags)

        if not nick:
            nick = ' '.join(map(lambda x: x.title(), name.split('-')))

        if not blurb:
            blurb = str(nick)

        attr = elem.attrib

        for k in ['blurb', 'nick', 'flags', 'name', 'type']:
            if k in attr:
                del attr[k]

        self.properties.append(_prop_types[typ](name, nick, blurb, flags, **attr))

    def _write(self, *args):
        if len(args) > 1:
            self.f.write(args[0] % args[1:])
        elif len(args) > 0:
            self.f.write(args[0])

        self.f.write("\n")

    def _write_license(self, text, options, filename):
        text = text.replace('#PROGRAM', options.program)
        text = text.replace('#YEAR', str(datetime.date.today().year))
        text = text.replace('#AUTHOR', options.author)
        text = text.replace('#FILENAME', filename)

        self._write(text)

    def _write_gpl(self, options, filename):
        self._write_license(GPL, options, filename)

    def _write_lgpl(self, options, filename):
        self._write_license(LGPL, options, filename)

    def write_header(self, outdir, options):
        fname = '%s.h' % (self.filename,)
        self.f = file(os.path.join(outdir, fname), 'w')

        if options.gpl:
            self._write_gpl(options, fname)
        elif options.lgpl:
            self._write_lgpl(options, fname)

        self._write("#ifndef __%s_H__", self.cname_upper)
        self._write("#define __%s_H__", self.cname_upper)
        self._write()

        self._write("#include <gedit/gedit-message.h>\n")
        self._write("G_BEGIN_DECLS\n")

        alignon = len('%s_IS_%s_CLASS(klass)' % (self.ns_upper, self.name_upper))
        alignsec = ' ' * (alignon + len('#define   '))

        self._write("#define %s (%s_get_type ())", self.ctype.ljust(alignon), self.cname_lower)
        self._write("#define %s (G_TYPE_CHECK_INSTANCE_CAST ((obj),\\\n%s%s,\\\n%s%s))",
                    ('%s(obj)' % (self.cname_upper,)).ljust(alignon),
                    alignsec, self.ctype,
                    alignsec, self.cobj)

        self._write("#define %s (G_TYPE_CHECK_INSTANCE_CAST ((obj),\\\n%s%s,\\\n%s%s))",
                    ('%s_CONST(obj)' % (self.cname_upper,)).ljust(alignon),
                    alignsec, self.ctype,
                    alignsec, '%s const' % (self.cobj,))

        self._write("#define %s (G_TYPE_CHECK_CLASS_CAST ((klass),\\\n%s%s,\\\n%s%s))",
                    ('%s_CLASS(klass)' % (self.cname_upper,)).ljust(alignon),
                    alignsec, self.ctype,
                    alignsec, self.cclass)

        self._write("#define %s (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\\\n%s%s))",
                    ('%s_IS_%s(obj)' % (self.ns_upper, self.name_upper)).ljust(alignon),
                    alignsec, self.ctype)

        self._write("#define %s (G_TYPE_CHECK_CLASS_TYPE ((klass),\\\n%s%s))",
                    ('%s_IS_%s_CLASS(klass)' % (self.ns_upper, self.name_upper)).ljust(alignon),
                    alignsec, self.ctype)

        self._write("#define %s (G_TYPE_INSTANCE_GET_CLASS ((obj),\\\n%s%s,\\\n%s%s))",
                    ('%s_GET_CLASS(obj)' % (self.cname_upper,)).ljust(alignon),
                    alignsec, self.ctype,
                    alignsec, self.cclass)

        thepriv = '%sPrivate' % (self.cobj,)
        alignon = len(thepriv)

        self._write()
        self._write('typedef struct _%s %s;', self.cobj.ljust(alignon), self.cobj)
        self._write('typedef struct _%s %s;', self.cclass.ljust(alignon), self.cclass)
        self._write('typedef struct _%s %s;', thepriv.ljust(alignon), thepriv)
        self._write()

        self._write('struct _%s', self.cobj)
        self._write('{')
        self._write('\tGeditMessage parent;')
        self._write()
        self._write('\t%s *priv;', thepriv)
        self._write('};')
        self._write()
        self._write('struct _%s', self.cclass)
        self._write('{')
        self._write('\tGeditMessageClass parent_class;')
        self._write('};')
        self._write()

        self._write('GType %s_get_type (void) G_GNUC_CONST;', self.cname_lower)
        self._write()
        self._write('G_END_DECLS')
        self._write()
        self._write("#endif /* __%s_H__ */", self.cname_upper)

        self.f.close()

    def needs_finalize(self):
        for prop in self.properties:
            out = prop.finalizer('ignore')

            if out:
                return True

        return False

    def write_body(self, outdir, options):
        fname = '%s.c' % (self.filename,)
        self.f = file(os.path.join(outdir, fname), 'w')

        if options.gpl:
            self._write_gpl(options, fname)
        elif options.lgpl:
            self._write_lgpl(options, fname)

        self._write('#include "config.h"')
        self._write('#include "%s.h"', self.filename)

        for inc in self.includes:
            self._write('%s', inc)

        self._write()

        self._write('enum')
        self._write('{')
        self._write('\tPROP_0,')
        self._write()

        for prop in self.properties:
            self._write('\t%s,', prop.prop_enum())

        self._write('};')
        self._write()

        self._write('struct _%sPrivate', self.cobj)
        self._write('{')

        for prop in self.properties:
            ct = prop.ctype()

            if not ct.endswith('*'):
                ct += ' '

            self._write('\t%s%s;', ct, prop.cname())

        self._write('};')

        self._write('''
G_DEFINE_TYPE_EXTENDED ({0},
                        {1},
                        GEDIT_TYPE_MESSAGE,
                        0,
                        G_ADD_PRIVATE ({0}))
'''.format(self.cobj, self.cname_lower))

        if self.needs_finalize():
            self._write('static void')
            self._write('%s_finalize (GObject *obj)', self.cname_lower)
            self._write('{')
            self._write('\t%s *msg = %s (obj);', self.cobj, self.cname_upper)
            self._write()

            haswritten = False

            for prop in self.properties:
                out = prop.finalizer('msg->priv')

                if out:
                    haswritten = True
                    self._write('\t%s', '\n\t'.join(out.splitlines()))

            if haswritten:
                self._write()

            self._write('\tG_OBJECT_CLASS (%s_parent_class)->finalize (obj);', self.cname_lower)
            self._write('}')
            self._write()

        ll = ' ' * (len(self.cname_lower))

        self._write('static void')
        self._write('%s_get_property (GObject    *obj,', self.cname_lower)
        self._write('%s               guint       prop_id,', ll)
        self._write('%s               GValue     *value,', ll)
        self._write('%s               GParamSpec *pspec)', ll)
        self._write('{')
        self._write('\t%s *msg;', self.cobj)
        self._write()
        self._write('\tmsg = %s (obj);', self.cname_upper)
        self._write()
        self._write('\tswitch (prop_id)')
        self._write('\t{')

        for prop in self.properties:
            self._write('\t\tcase %s:', prop.prop_enum())
            self._write('\t\t\t%s', prop.get_value('value', 'msg->priv'))
            self._write('\t\t\tbreak;')

        self._write('\t}')
        self._write('}')
        self._write()
        self._write('static void')
        self._write('%s_set_property (GObject      *obj,', self.cname_lower)
        self._write('%s               guint         prop_id,', ll)
        self._write('%s               GValue const *value,', ll)
        self._write('%s               GParamSpec   *pspec)', ll)
        self._write('{')
        self._write('\t%s *msg;', self.cobj)
        self._write()
        self._write('\tmsg = %s (obj);', self.cname_upper)
        self._write()
        self._write('\tswitch (prop_id)')
        self._write('\t{')

        for prop in self.properties:
            self._write('\t\tcase %s:', prop.prop_enum())

            out = prop.finalizer('msg->priv')

            if out:
                out = '\n\t\t\t'.join(out.splitlines())
                self._write('\t\t{')
                self._write('\t\t\t%s' % (out,))

            self._write('\t\t\t%s', prop.set_value('value', 'msg->priv'))
            self._write('\t\t\tbreak;')

            if out:
                self._write('\t\t}')

        self._write('\t}')
        self._write('}')
        self._write()

        self._write('static void')
        self._write('%s_class_init (%s *klass)', self.cname_lower, self.cclass)
        self._write('{')
        self._write('\tGObjectClass *object_class = G_OBJECT_CLASS(klass);')
        self._write()

        if self.needs_finalize():
            self._write('\tobject_class->finalize = %s_finalize;', self.cname_lower)
            self._write()

        self._write('\tobject_class->get_property = %s_get_property;', self.cname_lower)
        self._write('\tobject_class->set_property = %s_set_property;', self.cname_lower)

        pp = 'g_object_class_install_property ('
        prefix = '\t%s' % (' ' * len(pp),)

        for prop in self.properties:
            self._write()

            out = str(prop)
            out = ("\n%s" % (prefix,)).join(out.splitlines())

            self._write('\tg_object_class_install_property (object_class,\n%s%s,', prefix, prop.prop_enum())
            self._write('%s%s);', prefix, out)

        if len(self.properties) == 0:
            self._write()

        self._write('}')
        self._write()

        self._write('static void')
        self._write('%s_init (%s *message)', self.cname_lower, self.cobj)
        self._write('{')
        self._write('\tmessage->priv = %s_get_instance_private (message);', self.cname_lower)
        self._write('}')

        self.f.close()

class Generator:
    def __init__(self, filename, options):
        self.filename = filename
        self.doc = ElementTree.parse(filename)
        self.options = options

    def write_message(self, outputdir, elem):
        msg = Message(elem)

        msg.write_header(outputdir, options)
        msg.write_body(outputdir, options)

    def write(self, outputdir = '.'):
        for message in self.doc.findall('message'):
            self.write_message(outputdir, message)

author = pwd.getpwuid(os.getuid()).pw_gecos.split(',')[0]

parser = OptionParser()
parser.add_option("-o", "--output-directory", dest="output", help="Output directory", default=".")
parser.add_option("-g", "--gpl", dest="gpl", action="store_true", help="Add GPL stub to generated files", default=False)
parser.add_option("-l", "--lgpl", dest="lgpl", action="store_true", help="Add LGPL stub to generated files", default=False)
parser.add_option("-p", "--program", dest="program", help="Program name used in license snippet", default="program")
parser.add_option("-a", "--author", dest="author", help="Author used in license snippet", default=author)

(options, args) = parser.parse_args()

for arg in args:
    generator = Generator(arg, options)
    generator.write(options.output)

# vi:ex:ts=4:et
