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
gmidimonitor_alsamidi_sources = alsa.c
else
gmidimonitor_alsamidi_sources =
endif

if HAVE_JACK_MIDI
gmidimonitor_jackmidi_sources = jack.c
else
gmidimonitor_jackmidi_sources =
endif

gmidimonitor_SOURCES = about.c path.c glade.c main.c gm.c log.c memory_atomic.c $(gmidimonitor_alsamidi_sources) $(gmidimonitor_jackmidi_sources)

gmidimonitor_CFLAGS = @GTHREAD_CFLAGS@ @GMODULE_CFLAGS@ @GTK_CFLAGS@ @LIBGLADE_CFLAGS@ @LASH_CFLAGS@ @ALSA_CFLAGS@ @JACK_MIDI_CFLAGS@ -DDATA_DIR='"$(pkgdatadir)"' -Wall #-Werror
gmidimonitor_LDADD = @GTHREAD_LIBS@ @GMODULE_LIBS@ @GTK_LIBS@ @LIBGLADE_LIBS@ @LASH_LIBS@ @ALSA_LIBS@ @JACK_MIDI_LIBS@

gmidimonitordir = $(pkgdatadir)
gmidimonitor_DATA = gmidimonitor.glade
