bin_PROGRAMS = gmidimonitor

gmidimonitor_SOURCES = about.c path.c glade.c main.c

gmidimonitor_CFLAGS = @GTHREAD_CFLAGS@ @GTK_CFLAGS@ @LIBGLADE_CFLAGS@ @LASH_CFLAGS@ -DDATA_DIR='"$(pkgdatadir)"' -Wall #-Werror
gmidimonitor_LDADD = @GTHREAD_LIBS@ @GTK_LIBS@ @LIBGLADE_LIBS@ @LASH_LIBS@

gmidimonitordir = $(pkgdatadir)
gmidimonitor_DATA = gmidimonitor.glade
