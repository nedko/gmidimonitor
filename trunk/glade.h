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

#ifndef GLADE_H__732DAEDD_44BD_48CC_B8CA_B43FBB672A1D__INCLUDED
#define GLADE_H__732DAEDD_44BD_48CC_B8CA_B43FBB672A1D__INCLUDED

GtkWidget *
construct_glade_widget(
  const gchar * id);

GtkWidget *
get_glade_widget_child(
  GtkWidget * root,
  const gchar * id);

#endif /* #ifndef GLADE_H__732DAEDD_44BD_48CC_B8CA_B43FBB672A1D__INCLUDED */
