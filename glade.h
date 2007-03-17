/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 *   libglade helpers
 *
 *   This file is part of gmidimonitor
 *
 *   Copyright (C) 2005,2006,2007 Nedko Arnaudov <nedko@arnaudov.name>
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
