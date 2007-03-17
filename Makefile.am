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

gmidimonitor_SOURCES = about.c path.c glade.c main.c gm.c

gmidimonitor_CFLAGS = @GTHREAD_CFLAGS@ @GMODULE_CFLAGS@ @GTK_CFLAGS@ @LIBGLADE_CFLAGS@ @LASH_CFLAGS@ @ALSA_CFLAGS@ @JACK_MIDI_CFLAGS@ -DDATA_DIR='"$(pkgdatadir)"' -Wall #-Werror
gmidimonitor_LDADD = @GTHREAD_LIBS@ @GMODULE_LIBS@ @GTK_LIBS@ @LIBGLADE_LIBS@ @LASH_LIBS@ @ALSA_LIBS@ @JACK_MIDI_LIBS@

gmidimonitordir = $(pkgdatadir)
gmidimonitor_DATA = gmidimonitor.glade
