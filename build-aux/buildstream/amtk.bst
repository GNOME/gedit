kind: meson

sources:
- kind: git_tag
  url: gnome_gitlab:World/amtk.git
  track: master

build-depends:
- sdk/gobject-introspection.bst
- sdk/gtk-doc.bst
- freedesktop-sdk.bst:public-stacks/buildsystem-meson.bst

depends:
- sdk/gtk+-3.bst
- freedesktop-sdk.bst:bootstrap-import.bst
