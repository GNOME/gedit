gresource_xml = configuration_data()
if windowing_targets.contains('quartz')
  gresource_xml.set(
    'OS_DEPENDENT_RESOURCE_FILES',
    '<file preprocess="xml-stripblanks">gtk/menus-traditional.ui</file>'
  )
else
  gresource_xml.set('OS_DEPENDENT_RESOURCE_FILES', '')
endif

libgedit_res_xml = configure_file(
  input: 'gedit.gresource.xml.in',
  output: 'gedit.gresource.xml',
  configuration: gresource_xml
)

libgedit_res = gnome.compile_resources(
  'gedit-resources',
  libgedit_res_xml,
)
