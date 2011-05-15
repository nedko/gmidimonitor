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

#include <gtk/gtk.h>

#include "sysex.h"

/* General MMC decoding, as seen at http://www.borg.com/~jglatt/tech/mmc.htm and
   extended from "Advanced User Guide for MK-449C MIDI keyboard" info */
void
decode_sysex(
  guint8 * buffer,
  size_t buffer_size,
  GString * msg_str_ptr)
{
  const char * mmc_command_name;
  size_t i;

  if (buffer_size == 6 &&
      buffer[0] == 0xF0 &&
      buffer[1] == 0x7F &&
      buffer[3] == 0x06 &&
      buffer[5] == 0xF7)
  {
    switch (buffer[4])
    {
    case 1:
      mmc_command_name = "Stop";
      break;
    case 2:
      mmc_command_name = "Play";
      break;
    case 3:
      mmc_command_name = "Deferred Play";
      break;
    case 4:
      mmc_command_name = "Fast Forward";
      break;
    case 5:
      mmc_command_name = "Rewind";
      break;
    case 6:
      mmc_command_name = "Record Strobe (Punch In)";
      break;
    case 7:
      mmc_command_name = "Record Exit (Punch Out)";
      break;
    case 8:
      mmc_command_name = "Record Pause";
      break;
    case 9:
      mmc_command_name = "Pause";
      break;
    case 10:
      mmc_command_name = "Eject";
      break;
    case 11:
      mmc_command_name = "Chase";
      break;
    case 12:
      mmc_command_name = "Command Error Reset";
      break;
    case 13:
      mmc_command_name = "Reset";
      break;
    default:
      goto generic_sysex;
    }
    g_string_sprintf(
      msg_str_ptr,
      "MMC %s, for ",
      mmc_command_name);

    if (buffer[2] == 127)
    {
      g_string_append(
        msg_str_ptr,
        "all devices");
    }
    else
    {
      g_string_append_printf(
        msg_str_ptr,
        "device %u",
        (unsigned int)(buffer[2]));
    }
  }
  /* The goto MMC message, as seen at http://www.borg.com/~jglatt/tech/mmc.htm */
  else if (buffer_size == 13 &&
           buffer[0] == 0xF0 &&
           buffer[1] == 0x7F &&
           buffer[3] == 0x06 &&
           buffer[4] == 0x44 &&
           buffer[5] == 0x06 &&
           buffer[6] == 0x01 &&
           buffer[12] == 0xF7)
  {
    g_string_sprintf(
      msg_str_ptr,
      "MMC goto %u:%u:%u/%u:%u",
      (unsigned int)(buffer[7] & 0x1F), /* fps encoding */
      (unsigned int)(buffer[8]),
      (unsigned int)(buffer[9]),
      (unsigned int)(buffer[10] & 0x1F), /* no fps > 32, but bit 5 looks used for something */
      (unsigned int)(buffer[11]));

    switch (buffer[7] & 0x60)
    {
    case 0:
      g_string_append(
        msg_str_ptr,
        ", 24 fps");
      break;
    case 1:
      g_string_append(
        msg_str_ptr,
        ", 25 fps");
      break;
    case 2:
      g_string_append(
        msg_str_ptr,
        ", 29.97 fps");
      break;
    case 3:
      g_string_append(
        msg_str_ptr,
        ", 30 fps");
      break;
    }

    if (buffer[2] == 127)
    {
      g_string_append(
        msg_str_ptr,
        ", for all devices");
    }
    else
    {
      g_string_append_printf(
        msg_str_ptr,
        ", for device %u",
        (unsigned int)(buffer[2]));
    }
  }
  else
  {
  generic_sysex:
    g_string_sprintf(
      msg_str_ptr,
      "SYSEX with size %u:",
      (unsigned int)buffer_size);
    for (i = 0 ; i < buffer_size ; i++)
    {
      g_string_append_printf(
        msg_str_ptr,
        " %02X",
        (unsigned int)(buffer[i]));
    }
  }
}
