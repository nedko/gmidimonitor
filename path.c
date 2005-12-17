/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 * DESCRIPTION:
 *  About box for gmidimonitor. 
 *
 * LICENSE:
 *  GNU GENERAL PUBLIC LICENSE version 2
 *
 *****************************************************************************/

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

  full_path = g_strdup_printf("%s/%s", pszPathToExecutable, filename);
  if (stat(full_path, &st) == 0)
  {
    return full_path;
  }
  g_free(full_path);

  return NULL;
}

void
path_uninit()
{
  path_check_initialization();

  g_free(pszExecutable);
  g_free(pszPathToExecutable);
}
