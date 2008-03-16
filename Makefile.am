#
# This file is part of gmidimonitor
#
# Copyright (C) 2007 Nedko Arnaudov <nedko@arnaudov.name>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
#

bin_PROGRAMS = gmidimonitor

if HAVE_ALSA_MIDI
gmidimonitor_alsamidi_sources = alsa.c alsa.h
else
gmidimonitor_alsamidi_sources =
endif

if HAVE_JACK_MIDI
gmidimonitor_jackmidi_sources = jack.c jack.h jack_compat.h
else
gmidimonitor_jackmidi_sources =
endif

gmidimonitor_SOURCES = main.c
gmidimonitor_SOURCES += common.h
gmidimonitor_SOURCES += list.h
gmidimonitor_SOURCES += log.h
gmidimonitor_SOURCES += about.c
gmidimonitor_SOURCES += path.c path.h
gmidimonitor_SOURCES += glade.c glade.h
gmidimonitor_SOURCES += gm.c gm.h
gmidimonitor_SOURCES += log.c log.h
gmidimonitor_SOURCES += memory_atomic.c memory_atomic.h
gmidimonitor_SOURCES += sysex.c sysex.h
gmidimonitor_SOURCES += $(gmidimonitor_alsamidi_sources)
gmidimonitor_SOURCES += $(gmidimonitor_jackmidi_sources)

gmidimonitor_CFLAGS =  -DDATA_DIR='"$(pkgdatadir)"' -Wall
#gmidimonitor_CFLAGS += -Werror
gmidimonitor_CFLAGS += @GTHREAD_CFLAGS@
gmidimonitor_CFLAGS += @GMODULE_CFLAGS@
gmidimonitor_CFLAGS += @GTK_CFLAGS@
gmidimonitor_CFLAGS += @LIBGLADE_CFLAGS@
gmidimonitor_CFLAGS += @LASH_CFLAGS@
gmidimonitor_CFLAGS += @ALSA_CFLAGS@
gmidimonitor_CFLAGS += @JACK_MIDI_CFLAGS@

gmidimonitor_LDADD = @GTHREAD_LIBS@
gmidimonitor_LDADD += @GMODULE_LIBS@
gmidimonitor_LDADD += @GTK_LIBS@
gmidimonitor_LDADD += @LIBGLADE_LIBS@
gmidimonitor_LDADD += @LASH_LIBS@
gmidimonitor_LDADD += @ALSA_LIBS@
gmidimonitor_LDADD += @JACK_MIDI_LIBS@

gmidimonitordir = $(pkgdatadir)
dist_gmidimonitor_DATA = gmidimonitor.glade
