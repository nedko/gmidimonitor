# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(gmidimonitor, 3)
AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE(gmidimonitor, 3)
AC_CONFIG_HEADER([config.h])

#AS_AC_EXPAND(DATADIR, $datadir)
AC_SUBST(DATADIR)

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.

PKG_CHECK_MODULES(GTK, gtk+-2.0)

PKG_CHECK_MODULES(LIBGLADE, libglade-2.0)

PKG_CHECK_MODULES(GTHREAD, gthread-2.0)

PKG_CHECK_MODULES(GMODULE, gmodule-2.0)

# ALSA MIDI
have_alsamidi="unknown"
AC_ARG_ENABLE(alsamidi, [AS_HELP_STRING(--disable-alsamidi, [Force disable ALSA MIDI support [default=no]])], [ have_alsamidi="no" ])
if test "$have_alsamidi" = "unknown"
then
  PKG_CHECK_MODULES(ALSA, alsa, AC_DEFINE([HAVE_ALSA_MIDI], [], [Defined if we have ALSA MIDI support.]) have_alsamidi="yes")
fi

# JACK MIDI
have_jackmidi="unknown"
AC_ARG_ENABLE(jackmidi, [AS_HELP_STRING(--disable-jackmidi, [Force disable JACK MIDI support [default=no]])], [ have_jackmidi="no" ])
if test "$have_jackmidi" = "unknown"
then
  PKG_CHECK_MODULES(JACK_MIDI, jack >= 0.102.0, AC_DEFINE([HAVE_JACK_MIDI], [], [Defined if we have JACK MIDI support.]) have_jackmidi="yes")
  if test "$have_jackmidi" = "yes"
  then
    PKG_CHECK_MODULES(OLD_JACK_MIDI, jack < 0.102.27, have_jackmidi="yes (old one)" AC_DEFINE(HAVE_OLD_JACK_MIDI, 1, [whether or not we building against old JACK MIDI (0.102.20)]), echo no)
  fi
fi

if test "$have_alsamidi" != "yes" -a "$have_jackmidi" != "yes"
then
  AC_MSG_ERROR([Neighter ALSA nor JACK MIDI is available.])
fi

# LASH
have_lash="unknown"
AC_ARG_ENABLE(lash, [AS_HELP_STRING(--disable-lash, [Force disable LASH support [default=no]])], [ have_lash="no" ])
if test "$have_lash" = "unknown"
then
  PKG_CHECK_MODULES(LASH, lash-1.0, AC_DEFINE([HAVE_LASH], [], [Defined if we have LASH support.]) have_lash="yes")
fi

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

AC_MSG_RESULT([])
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([])
AC_MSG_RESULT([Prefix:            $prefix])
AC_MSG_RESULT([])
AC_MSG_RESULT([JACK MIDI support: $have_jackmidi])
if test "$have_jackmidi" = "yes"
then
  AC_MSG_RESULT([JACK MIDI support NOT IMPLEMENTED YET])
fi
AC_MSG_RESULT([ALSA MIDI support: $have_alsamidi])
AC_MSG_RESULT([LASH support:      $have_lash])
AC_MSG_RESULT([])
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([])
