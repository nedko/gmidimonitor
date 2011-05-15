/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 *   Access to project specific data files.
 *
 *   This file is part of gmidimonitor
 *
 *   Copyright (C) 2005,2006,2007,2008,2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; version 2 of the License
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *****************************************************************************/

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include "path.h"

static gchar *pszPathToExecutable = NULL;
static gchar *pszExecutable = NULL;

void
path_init(const char * argv0)
{
  /* FIXME: pszPathToExecutable calculation is ugly workaround
   * that assumes that current directory is nevere changed for our process.
   * We should better get full path (/proc/pid/maps in Linux ?) */
  pszExecutable = g_path_get_basename(argv0);
  pszPathToExecutable = g_path_get_dirname(argv0);
}

static void
path_check_initialization()
{
  if (pszPathToExecutable == NULL ||
      pszExecutable == NULL)
  {
    g_warning("path_init() not called.");
    exit(1);
  }
}

gchar *
path_get_data_filename(const gchar * filename)
{
  gchar * full_path;
  struct stat st;

  path_check_initialization();

  /* check if it can be found in the source dir */
  /* This allows a not installed binary to read right data files */
  full_path = g_strdup_printf("%s/../%s", pszPathToExecutable, filename);
  if (stat(full_path, &st) == 0)
  {
    return full_path;
  }
  g_free(full_path);

#if defined(DATA_DIR)
  /* check in installation data dir */
  full_path = g_strdup_printf(DATA_DIR "/%s", filename);
  if (stat(full_path, &st) == 0)
  {
    return full_path;
  }
  g_free(full_path);
#endif

  return NULL;
}

void
path_uninit()
{
  path_check_initialization();

  g_free(pszExecutable);
  g_free(pszPathToExecutable);
}
