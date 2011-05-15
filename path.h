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

#ifndef PATH_H__6A0C8189_7048_457D_9081_B0F76AD4B93C__INCLUDED
#define PATH_H__6A0C8189_7048_457D_9081_B0F76AD4B93C__INCLUDED

void
path_init(const char * argv0);

/* g_free the return value if it is not NULL */
gchar *
path_get_data_filename(const gchar * filename);

void
path_uninit();

#endif /* #ifndef PATH_H__6A0C8189_7048_457D_9081_B0F76AD4B93C__INCLUDED */
