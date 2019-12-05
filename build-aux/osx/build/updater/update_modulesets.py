#!/usr/bin/env python

import lxml.etree as ET
import lxml.objectify

import subprocess, shutil, os, glob, urllib2, json, sys, difflib

if not os.path.exists('.gtk-osx'):
    subprocess.call(['git', 'clone', 'git://git.gnome.org/gtk-osx', '.gtk-osx'])

try:
    os.makedirs('.cache')
except:
    pass

class Merger:
    def __init__(self):
        self.repos = {}
        self.modules = {}

    def normalize_xml(self, xml):
        s = ET.tostring(xml)
        o = lxml.objectify.fromstring(s)

        return ET.tostring(o, pretty_print=True)

    def extract_repos(self, t):
        default = None

        for repo in t.findall('./repository'):
            name = repo.attrib['name']

            if 'default' in repo.attrib:
                default = repo

            if not name in self.repos:
                self.repos[name] = repo

        return default

    def parse_module(self, f):
        t = ET.parse(f)
        drepo = self.extract_repos(t)

        tags = {}

        for x in t.getroot().findall('./'):
            if x.tag == 'include' or x.tag == 'repository':
                continue

            branch = x.find('./branch')

            if not branch is None and not 'repo' in branch.attrib:
                branch.attrib['repo'] = drepo.attrib['name']

            id = x.attrib['id']

            if id in self.modules:
                print('Overriding existing module {0}:'.format(id))

                a = self.normalize_xml(self.modules[id]).splitlines(True)
                b = self.normalize_xml(x).splitlines(True)

                print(''.join(difflib.unified_diff(a, b, fromfile='old', tofile='new')))
                print('\n')

                sys.stdout.write('Do you want to keep the original, or the new file? [(o)riginal, (n)new] ')
                sys.stdout.flush()
                answer = sys.stdin.readline().rstrip()

                if answer == '' or answer == 'n' or answer == 'new':
                    self.modules[id] = x
                    print('Using new version\n')
                else:
                    print('Used original version\n')
            else:
                self.modules[id] = x

    def copy_patches(self, mod):
        # Copy patches locally
        patches = mod.findall('./branch/patch')

        if len(patches) == 0:
            return

        dname = 'modulesets/patches/' + mod.attrib['id']

        try:
            os.makedirs(dname)
        except:
            pass

        locc = 'http://git.gnome.org/browse/gtk-osx/plain/'

        for p in patches:
            f = p.attrib['file']

            pname = os.path.basename(f)

            if f.startswith(locc):
                shutil.copyfile('.gtk-osx/' + f[len(locc):], dname + '/' + pname)
            elif f.startswith('patches/'):
                shutil.copyfile(f, dname + '/' + pname)
            else:
                content = self.from_cache_or_url(os.path.join('patches', mod.attrib['id'], pname), f)

                with open(dname + '/' + pname, 'w') as patch:
                    patch.write(content)

            p.attrib['file'] = mod.attrib['id'] + '/' + pname

    def from_cache_or_url(self, filename, url):
        cfile = os.path.join('.cache', filename)

        try:
            with open(cfile) as f:
                return f.read()
        except:
            pass

        resp = urllib2.urlopen(url)
        ret = resp.read()
        resp.close()

        dname = os.path.dirname(cfile)

        try:
            os.makedirs(dname)
        except:
            pass

        try:
            with open(cfile, 'w') as f:
                f.write(ret)
        except:
            pass

        return ret

    def update_module(self, mod):
        branch = mod.find('./branch')

        if branch is None:
            return

        if not branch.attrib['repo'].endswith('.gnome.org'):
            return

        # Check for latest versions
        repo = self.repos[branch.attrib['repo']]

        if repo.attrib['type'] != 'tarball':
            return

        module = branch.attrib['module']

        modname = module.split('/', 2)[0]
        version = [int(x) for x in branch.attrib['version'].split('.')]

        # Skip updating gtk+ 2.x
        if modname == 'gtk+' and version[0] == 2:
            return

        href = repo.attrib['href']
        versions = self.from_cache_or_url(mod.attrib['id'] + '.json', href + modname + '/cache.json')
        versions = json.loads(versions)

        latest = [version, version]

        for v in versions[1][modname]:
            vv = [int(x) for x in v.split('.')]

            if vv[1] % 2 == 0:
                if vv > latest[0] and vv[0] == latest[0][0]:
                    latest[0] = vv
            else:
                if vv > latest[1] and vv[0] == latest[1][0]:
                    latest[1] = vv

        if latest[0] > version or latest[1] > version:
            choices = []

            if latest[0] > version:
                choices.append('stable = {0}'.format('.'.join([str(x) for x in latest[0]])))

            if latest[1] > version:
                choices.append('unstable = {0}'.format('.'.join([str(x) for x in latest[1]])))

            sversion = '.'.join([str(x) for x in version])
            print('Found new versions for {0} = {1}: {2}'.format(modname, sversion, ', '.join(choices)))

            sys.stdout.write('Do you want to update? [(s)table/(u)nstable/(n)o]: ')
            sys.stdout.flush()
            answer = sys.stdin.readline().rstrip()

            nv = None

            if answer == '':
                if latest[0] > latest[1]:
                    answer = 'stable'
                else:
                    answer = 'unstable'

            if (answer == 'stable' or answer == 's') and latest[0] > version:
                nv = latest[0]
            elif (answer =='unstable' or answer == 'u') and latest[1] > version:
                nv = latest[1]

            if not nv is None:
                v = '.'.join([str(x) for x in nv])
                info = versions[1][modname][v]
                branch.attrib['version'] = v
                branch.attrib['module'] = '{0}/{1}'.format(modname, info['tar.xz'])

                hfile = href + modname + '/' + info['sha256sum']

                ret = self.from_cache_or_url(os.path.join('hashes', modname, info['sha256sum']), hfile)

                for line in ret.splitlines():
                    hash, fname = line.rstrip().split(None, 2)

                    if fname == os.path.basename(info['tar.xz']):
                        branch.attrib['hash'] = 'sha256:{0}'.format(hash)

                print('Updated to version {0}\n'.format(v))
            else:
                print('Keep version {0}\n'.format(sversion))

    def merge(self, modules, entry_point, overrides):
        self.modules = {}
        self.repos = {}

        for mod in modules:
            self.parse_module(mod)

        self.required_modules = []
        processed = set()

        self.parse_module(overrides)

        process = [self.modules[entry_point]]
        processed.add(entry_point)

        while len(process) != 0:
            mod = process.pop()
            id = mod.attrib['id']

            self.required_modules.insert(0, mod)

            deps = mod.findall('./dependencies/dep') + mod.findall('./after/dep') + mod.findall('./suggests/dep')

            for dep in deps:
                package = dep.attrib['package']

                if package in processed:
                    continue

                if not package in self.modules:
                    sys.stderr.write('Package dependency is not in modules... {0}\n'.format(package))
                    sys.exit(1)

                processed.add(package)
                process.insert(0, self.modules[package])

    def write(self, f):
        needed_repos = {}

        for mod in self.required_modules:
            branch = mod.find('./branch')

            if not branch is None and 'repo' in branch.attrib:
                needed_repos[branch.attrib['repo']] = self.repos[branch.attrib['repo']]

        try:
            os.makedirs('.cache')
        except:
            pass

        for mod in self.required_modules:
            self.copy_patches(mod)
            self.update_module(mod)

        with open(f, 'w') as f:
            root = ET.Element('moduleset')

            repos = needed_repos.values()
            repos.sort(lambda a, b: cmp(a.attrib['name'], b.attrib['name']))

            for repo in repos:
                root.append(repo)

            for mod in self.required_modules:
                root.append(mod)

            ret = ET.ElementTree(root)

            content = ET.tostring(ret, pretty_print=True, xml_declaration=True, encoding='utf-8', doctype='<!DOCTYPE moduleset SYSTEM "moduleset.dtd">')
            f.write(content)

shutil.rmtree('modulesets', ignore_errors=True)

os.makedirs('modulesets')
os.makedirs('modulesets/patches')

allf = glob.glob('.gtk-osx/modulesets-stable/*.modules')

nobs = [x for x in allf if os.path.basename(x) != 'bootstrap.modules']
bs = [x for x in allf if os.path.basename(x) == 'bootstrap.modules']

m = Merger()

m.merge(nobs, 'gedit-meta', 'gedit-overrides.modules')
m.write('modulesets/gedit.modules')

m.merge(bs, 'meta-bootstrap', 'gedit-bootstrap-overrides.modules')
m.write('modulesets/bootstrap.modules')

print('New modules have been written to "modulesets"')

# vi:ts=4:et
