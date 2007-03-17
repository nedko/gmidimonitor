bin_PROGRAMS = gmidimonitor

gmidimonitor_SOURCES = about.c path.c glade.c main.c

gmidimonitor_CFLAGS = @GTHREAD_CFLAGS@ @GMODULE_CFLAGS@ @GTK_CFLAGS@ @LIBGLADE_CFLAGS@ @LASH_CFLAGS@ @ALSA_CFLAGS@ @JACK_MIDI_CFLAGS@ -DDATA_DIR='"$(pkgdatadir)"' -Wall #-Werror
gmidimonitor_LDADD = @GTHREAD_LIBS@ @GMODULE_LIBS@ @GTK_LIBS@ @LIBGLADE_LIBS@ @LASH_LIBS@ @ALSA_LIBS@ @JACK_MIDI_LIBS@

gmidimonitordir = $(pkgdatadir)
gmidimonitor_DATA = gmidimonitor.glade
