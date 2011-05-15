/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
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

#ifndef GM_H__6EE298F4_A33F_4DFD_8E38_49F00997E455__INCLUDED
#define GM_H__6EE298F4_A33F_4DFD_8E38_49F00997E455__INCLUDED

const char *
gm_get_drum_name(
  unsigned char note);

const char *
gm_get_instrument_name(
  unsigned char program);

const char *
gm_get_controller_name(
  unsigned int controller);

#endif /* #ifndef GM_H__6EE298F4_A33F_4DFD_8E38_49F00997E455__INCLUDED */
