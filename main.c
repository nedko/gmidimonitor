/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 *   gmidimonitor main code.
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

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef HAVE_LASH_1_0
#include <lash/lash.h>
#endif

#include "common.h"
#include "path.h"
#include "gm.h"
#include "jack.h"
#include "alsa.h"

GtkBuilder * g_builder = NULL;
GtkWidget * g_main_window_ptr;

gboolean g_midi_ignore = FALSE;

int g_row_count;

#ifdef HAVE_LASH_1_0
lash_client_t * g_lashc;
#endif

const char * g_note_names[12] = 
{
  "C",
  "C#",
  "D",
  "D#",
  "E",
  "F",
  "F#",
  "G",
  "G#",
  "A",
  "A#",
  "B",
};

void
create_mainwindow()
{
  GtkWidget * child_ptr;
  GtkListStore * list_store_ptr;
  GtkTreeViewColumn * column_ptr;
  GtkCellRenderer * text_renderer_ptr;
  GError * error;
  gchar * ui_filename;

  ui_filename = path_get_data_filename("gmidimonitor.ui");
  if (ui_filename == NULL)
  {
    g_warning("Cannot find UI description file.");
    exit(1);
  }

  /* load the interface builder */
  error = NULL;
  g_builder = gtk_builder_new();
  if (!gtk_builder_add_from_file (g_builder, ui_filename, &error))
  {
    g_warning ("Couldn't load builder file: %s", error->message);
    g_error_free (error);
  }
  g_free(ui_filename);
  
  /* Retrieve the main window and connect the signals in the interface */
  g_main_window_ptr = GTK_WIDGET (gtk_builder_get_object (g_builder, "main_window"));
  gtk_builder_connect_signals(g_builder, NULL);

  child_ptr = GTK_WIDGET (gtk_builder_get_object (g_builder, "list"));

  text_renderer_ptr = gtk_cell_renderer_text_new();

  g_object_set(
    G_OBJECT(text_renderer_ptr),
    "family",
    "Monospace",
    NULL);

/*   column_ptr = gtk_tree_view_column_new_with_attributes( */
/*     "Time", */
/*     text_renderer_ptr, */
/*     "text", COL_TIME, */
/*     NULL); */

/*   gtk_tree_view_append_column( */
/*     GTK_TREE_VIEW(child_ptr), */
/*     column_ptr); */

  column_ptr = gtk_tree_view_column_new_with_attributes(
    "Channel",
    text_renderer_ptr,
    "text", COL_CHANNEL,
    NULL);

  gtk_tree_view_append_column(
    GTK_TREE_VIEW(child_ptr),
    column_ptr);

  column_ptr = gtk_tree_view_column_new_with_attributes(
    "Message",
    text_renderer_ptr,
    "text", COL_MESSAGE,
    NULL);

  gtk_tree_view_append_column(
    GTK_TREE_VIEW(child_ptr),
    column_ptr);

  gtk_widget_show_all(g_main_window_ptr);

  list_store_ptr = gtk_list_store_new(
    NUM_COLS,
    G_TYPE_STRING,
    G_TYPE_STRING,
    G_TYPE_STRING);

  gtk_tree_view_set_model(GTK_TREE_VIEW(child_ptr), GTK_TREE_MODEL(list_store_ptr));
}

/* stop button toggle */
void on_button_stop_toggled(
  GtkAction * action_ptr,
  GtkWidget * widget_ptr)
{
  GtkWidget * child_ptr;

  child_ptr = GTK_WIDGET (gtk_builder_get_object (g_builder, "button_stop"));

  if (gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(child_ptr)))
  {
    g_midi_ignore = TRUE;
  }
  else
  {
    g_midi_ignore = FALSE;
  }
}

void on_clear_clicked
(GtkButton *button, gpointer user_data)
{
  gtk_list_store_clear(
    GTK_LIST_STORE(
      gtk_tree_view_get_model(
        GTK_TREE_VIEW(
          gtk_builder_get_object(
            g_builder,
            "list")))));
  g_row_count = 0;
}

#ifdef HAVE_LASH_1_0

void
process_lash_event(lash_event_t * event_ptr)
{
  enum LASH_Event_Type type;
  const char * str;

  type = lash_event_get_type(event_ptr);
  str = lash_event_get_string(event_ptr);

  switch (type)
  {
  case LASH_Quit:
    g_warning("LASH_Quit received.\n");
    g_lashc = NULL;
    gtk_main_quit();
    break;
  case LASH_Save_File:
  case LASH_Restore_File:
  case LASH_Save_Data_Set:
  default:
    g_warning("LASH Event. Type = %u, string = \"%s\"\n",
              (unsigned int)type,
              (str == NULL)?"":str);
  }
}

void
process_lash_config(lash_config_t * config_ptr)
{
  const char * key;
  const void * data;
  size_t data_size;

  key = lash_config_get_key(config_ptr);
  data = lash_config_get_value(config_ptr);
  data_size = lash_config_get_value_size(config_ptr);

  g_warning("LASH Config. Key = \"%s\"\n", key);
}

/* process lash events callback */
gboolean
process_lash_events(gpointer data)
{
  lash_event_t * event_ptr;
  lash_config_t * config_ptr;

  /* Process events */
  while ((event_ptr = lash_get_event(g_lashc)) != NULL)
  {
    process_lash_event(event_ptr);
    lash_event_destroy(event_ptr); 
  }

  /* Process configs */
  while ((config_ptr = lash_get_config(g_lashc)) != NULL)
  {
    process_lash_config(config_ptr);
    lash_config_destroy(config_ptr);  
  }

  return TRUE;
}

#endif  /* #ifdef HAVE_LASH_1_0 */

static
void
sigint_handler(
  int i)
{
  gtk_main_quit();
}

int
main(int argc, char *argv[])
{
#ifdef HAVE_LASH_1_0
  lash_event_t * lash_event_ptr;
#endif
  GString * client_name_str_ptr;

  int i;
  gboolean want_any = TRUE;
#ifdef HAVE_JACK
  gboolean want_jack = FALSE;
  gboolean jack_enabled = FALSE;
#endif
#ifdef HAVE_ALSA
  gboolean want_alsa = FALSE;
  gboolean alsa_enabled = FALSE;
#endif
  gboolean io_enabled = FALSE;
  int ret = 0;

  for (i = 1; i < argc; i++)
  {
#ifdef HAVE_JACK
    if (strcmp(argv[i], "--jack") == 0)
    {
      want_jack = TRUE;
      want_any = FALSE;
      continue;
    }
#endif
#ifdef HAVE_ALSA
    if (strcmp(argv[i], "--alsa") == 0)
    {
      want_alsa = TRUE;
      want_any = FALSE;
      continue;
    }
#endif
    if (strcmp(argv[i], "--help") != 0)
    {
      fprintf(stderr, "Unrecognized parameter \"%s\"\n", argv[i]);
      ret = 1;
    }

    fprintf(
      ret == 0 ? stdout : stderr,
      "Usage: %s"
#ifdef HAVE_JACK
      " [--jack]"
#endif
#ifdef HAVE_ALSA
      " [--alsa]"
#endif
      "\n",
      argv[0]);

    exit(ret);
  }

  /* init threads */
  g_thread_init(NULL);
  gdk_threads_init();
  gdk_threads_enter();

  gtk_init(&argc, &argv);

  path_init(argv[0]);

#ifdef HAVE_LASH_1_0
  if (getenv("LADISH_APPNAME") != NULL)
  {
    g_lashc = lash_init(
      lash_extract_args(&argc, &argv),
      "gmidimonitor",
      0,
      LASH_PROTOCOL_VERSION); 
    if (g_lashc == NULL)
    {
      g_warning("Failed to connect to LASH. Session management will not occur.\n");
    }
    else
    {
      lash_event_ptr = lash_event_new_with_type(LASH_Client_Name);
      lash_event_set_string(lash_event_ptr, "GMIDImonitor");
      lash_send_event(g_lashc, lash_event_ptr);
      g_timeout_add(250, process_lash_events, NULL);
    }
  }
#endif

  /* interface creation */
  create_mainwindow();

  client_name_str_ptr = g_string_new("");
  g_string_sprintf(client_name_str_ptr, "MIDI monitor");

  g_row_count = 0;

#ifdef HAVE_JACK
  if (want_jack || want_any)
  {
    jack_enabled = jack_init(client_name_str_ptr->str);
    if (jack_enabled)
    {
      g_printf("JACK MIDI enabled\n");
      io_enabled = TRUE;
    }
    else if (want_jack)
    {
      goto failed_jack;
    }
  }
#endif

#ifdef HAVE_ALSA
  if (want_alsa || (want_any && !io_enabled))
  {
    alsa_enabled = alsa_init(client_name_str_ptr->str);
    if (alsa_enabled)
    {
      g_printf("ALSA MIDI enabled\n");
      io_enabled = TRUE;
    }
    else if (want_alsa)
    {
      goto failed_alsa;
    }
  }
#endif

  if (io_enabled)
  {
    signal(SIGINT, &sigint_handler);
    signal(SIGTERM, &sigint_handler);

    /* main loop */
    gtk_main();
  }

#ifdef HAVE_ALSA
  if (alsa_enabled)
  {
    alsa_uninit();
  }
failed_alsa:
#endif

#ifdef HAVE_JACK
  if (jack_enabled)
  {
    jack_uninit();
  }
failed_jack:
#endif

  gdk_threads_leave();

  path_init(argv[0]);

  return ret;
}
