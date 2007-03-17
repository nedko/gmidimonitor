/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 *   This file is part of gmidimonitor
 *
 *   Copyright (C) 2007 Nedko Arnaudov <nedko@arnaudov.name>
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

#include <jack/jack.h>
#include <jack/midiport.h>
#include <gtk/gtk.h>

#include "jack.h"

#ifdef HAVE_OLD_JACK_MIDI
#define jack_midi_get_event_count(port_buf, nframes) jack_midi_port_get_info(port_buf, nframes)->event_count
#endif

jack_client_t * g_jack_client;
jack_port_t * g_jack_input_port;

int
jack_process(jack_nframes_t nframes, void * context)
{
  void * port_buf;
  jack_midi_event_t in_event;
  jack_nframes_t event_count;
  jack_position_t pos;
  jack_nframes_t i;

  port_buf = jack_port_get_buffer(g_jack_input_port, nframes);
  event_count = jack_midi_get_event_count(port_buf, nframes);
  jack_transport_query(g_jack_client, &pos);
  
  for (i = 0 ; i < event_count; i++)
  {
    jack_midi_event_get(&in_event, port_buf, i, nframes);

    /* allocate memory for buffer copy */

    /* copy buffer data */

    //jack_process_midi_message(in_event.buffer, in_event.size);
  }

  return 0;
}

gboolean
jack_init(const char * name)
{
  int ret;

  if ((g_jack_client = jack_client_new(name)) == 0)
  {
    g_warning("jack server not running?\n");
    return FALSE;
  }

  ret = jack_set_process_callback(g_jack_client, jack_process, 0);
  /* TODO: error handling */

/*     jack_set_sample_rate_callback(g_jack_client, srate, 0); */

/*     jack_on_shutdown(g_jack_client, jack_shutdown, 0); */

  g_jack_input_port = jack_port_register(g_jack_client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);

  if (jack_activate(g_jack_client))
  {
    /* TODO: proper (goto) error handling */
    g_warning("cannot activate client\n");
    return FALSE;
  }

  return TRUE;
}

void
jack_uninit()
{
  jack_client_close(g_jack_client);
}
