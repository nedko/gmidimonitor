project(
  'gmidimonitor', 'c',
        version : '3.6',
        license : 'LGPL2',
  meson_version : '>= 0.48.0'
)

# mandatory dependencies
gtkdep = dependency('gtk+-2.0')
gthreaddep = dependency('gthread-2.0')
gmodule = dependency('gmodule-2.0')

# optiona (more or less) dependencies
jack_dep = dependency('jack', required : get_option('jack'))
alsa_dep = dependency('alsa', required : get_option('alsa'))
lash_dep = dependency('lash', required : get_option('lash'))

# we need at least one of jack/alsa
if not jack_dep.found() and not alsa_dep.found()
  error('Neither JACK nor ALSA is available')
endif

# define paths
gmidimonitor_prefix = get_option('prefix')
gmidimonitor_bindir = join_paths(gmidimonitor_prefix, get_option('bindir'))
gmidimonitor_datadir = join_paths(gmidimonitor_prefix, get_option('datadir') / meson.project_name())
gmidimonitor_desktopdir = join_paths(get_option('datadir'), 'applications')
gmidimonitor_pixmapsdir = join_paths(get_option('datadir'), 'pixmaps')

# configure.h
conf_data = configuration_data()
conf_data.set('version', meson.project_version())
conf_data.set('appname', meson.project_name())
conf_data.set('datadir', gmidimonitor_datadir)
configure_file(input : 'config.h.in',
               output : 'config.h',
               configuration : conf_data)

# sources
src = [
  'main.c',
  'about.c',
  'path.c',
  'gm.c',
  'log.c',
  'memory_atomic.c',
  'sysex.c'
]

if jack_dep.found()
  src += [ 'jack.c' ]
endif

if alsa_dep.found()
  src += [ 'alsa.c' ]
endif

# target
executable(
  'gmidimonitor',
  src,
  dependencies : [gtkdep, gthreaddep, gmodule, jack_dep, alsa_dep, lash_dep],
  c_args : '-Wall',
  install : true
)

# additional installs
install_data('gmidimonitor.ui')

message('')
message('==================')
message('Prefix: ' + gmidimonitor_prefix)
message('Jack: ' + jack_dep.found().to_string())
message('Alsa: ' + alsa_dep.found().to_string())
message('Lash: ' + lash_dep.found().to_string())
message('==================')
message('')
