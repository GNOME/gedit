gresource_config_data = configuration_data()

if host_machine.system() == 'darwin'
  gresource_config_data.set(
    'OS_DEPENDENT_RESOURCE_FILES',
    '<file preprocess="xml-stripblanks">gtk/menus-traditional.ui</file>'
  )
else
  gresource_config_data.set('OS_DEPENDENT_RESOURCE_FILES', '')
endif

gresource_xml_file = configure_file(
  input: 'gedit.gresource.xml.in',
  output: 'gedit.gresource.xml',
  configuration: gresource_config_data
)

libgedit_gresources = gnome.compile_resources(
  'gedit-resources',
  gresource_xml_file,
)

# Windows PE (Portable Executable) header icon
if host_machine.system() == 'windows'
  windows = import('windows')
  gedit_sources += windows.compile_resources('gedit.rc')
endif
