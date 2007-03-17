/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 * DESCRIPTION:
 *  libglade helpers
 *
 * LICENSE:
 *  GNU GENERAL PUBLIC LICENSE version 2
 *
 *****************************************************************************/

#include <stdlib.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include "glade.h"
#include "path.h"

static void
glade_signal_connect_func(
  const gchar *cb_name, GObject *obj, 
  const gchar *signal_name, const gchar *signal_data,
  GObject *conn_obj, gboolean conn_after,
  gpointer user_data)
{
  /** Module with all the symbols of the program */
  static GModule *mod_self = NULL;
  gpointer handler_func;

  /* initialize gmodule */
  if (mod_self == NULL)
  {
    mod_self = g_module_open(NULL, 0);
    g_assert(mod_self != NULL);
/*     g_warning("%p", mod_self); */
  }

/*   g_print("glade_signal_connect_func:" */
/*           " cb_name = '%s', signal_name = '%s', " */
/*           "signal_data = '%s', obj = 0x%08X, conn_after = %s\n", */
/*           cb_name, signal_name, */
/*           signal_data, (unsigned int)obj, conn_after?"true":"false"); */

/*   g_warning("%s", g_module_name(mod_self)); */

  if (!g_module_symbol(mod_self, cb_name, &handler_func))
  {
    g_warning("callback function not found: %s", cb_name);
    return;
  }

  /* found callback */
  if (conn_obj)
  {
    if (conn_after)
    {
      g_signal_connect_object(
        obj,
        signal_name, 
        handler_func,
        conn_obj,
        G_CONNECT_AFTER);
    }
    else
    {
      g_signal_connect_object(
        obj,
        signal_name, 
        handler_func,
        conn_obj,
        G_CONNECT_SWAPPED);
    }
  }
  else
  {
    /* no conn_obj; use standard connect */
    gpointer data = NULL;
      
    data = user_data;
      
    if (conn_after)
    {
      g_signal_connect_after(
        obj,
        signal_name, 
        handler_func,
        data);
    }
    else
    {
      g_signal_connect(
        obj,
        signal_name, 
        handler_func,
        data);
    }
  }
}

GtkWidget *
construct_glade_widget(
  const gchar * id)
{
  gchar * glade_filename;
  GtkWidget * widget;
  GladeXML * xml;

  glade_filename = path_get_data_filename("gmidimonitor.glade");
  if (glade_filename == NULL)
  {
    g_warning("Cannot find glade UI description file.");
    exit(1);
  }

  /* load the interface */
  xml = glade_xml_new(glade_filename, id, NULL);

  g_free(glade_filename);

  widget = glade_xml_get_widget(xml, id);

  /* connect the signals in the interface */
  glade_xml_signal_autoconnect_full(
    xml,
    (GladeXMLConnectFunc)glade_signal_connect_func,
    widget);

  return widget;
}

GtkWidget *
get_glade_widget_child(
  GtkWidget * root,
  const gchar * id)
{
  GladeXML * xml;
  GtkWidget * widget;

  xml = glade_get_widget_tree(root);

  widget = glade_xml_get_widget(xml, id);

  return widget;
}
