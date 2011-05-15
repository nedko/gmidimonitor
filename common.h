/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 *   This file is part of gmidimonitor
 *
 *   Copyright (C) 2007,2008,2011 Nedko Arnaudov <nedko@arnaudov.name>
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

#ifndef COMMON_H__FAD1EC6A_8ACC_483A_BB32_DDBE32B95B42__INCLUDED
#define COMMON_H__FAD1EC6A_8ACC_483A_BB32_DDBE32B95B42__INCLUDED

extern const char * g_note_names[12];

extern GtkWidget * g_main_window_ptr;

extern gboolean g_midi_ignore;

extern int g_row_count;

#define MAX_LIST_SIZE 2000

#define COL_TIME     0
#define COL_CHANNEL  1
#define COL_MESSAGE  2
#define NUM_COLS     3

#endif /* #ifndef COMMON_H__FAD1EC6A_8ACC_483A_BB32_DDBE32B95B42__INCLUDED */
