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

#include <jack/jack.h>
#include <jack/midiport.h>
#include <gtk/gtk.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "config.h"

#include "common.h"
#include "jack.h"
#include "memory_atomic.h"
//#define LOG_LEVEL LOG_LEVEL_DEBUG
#include "log.h"
#include "list.h"
#include "gm.h"
#include "sysex.h"

#include "jack_compat.h"

extern GtkBuilder * g_builder;

jack_client_t * g_jack_client;
jack_port_t * g_jack_input_port;
rtsafe_memory_handle g_memory;

struct jack_midi_event_buffer
{
  struct list_head siblings;
  size_t buffer_size;
  unsigned char buffer[0];
};

pthread_mutex_t g_jack_midi_mutex;
//pthread_cond_t g_jack_midi_cond;

/* transfer from jack process callback to main thread */
/* new events are being put at list begining */
struct list_head g_jack_midi_events_pending;

/* temporary storage used by jack process callback in case g_jack_midi_events_pending is locked by main thread */
/* new events are being put at list begining */
struct list_head g_jack_midi_events_pending_rt;

pthread_t g_jack_midi_tid;      /* jack_midi_thread id */

gboolean g_jack_midi_thread_exit; /* whther jack_midi_thread shoud exit */

int
jack_process(jack_nframes_t nframes, void * context)
{
  int ret;
  void * port_buf;
  jack_midi_event_t in_event;
  jack_nframes_t event_count;
  jack_position_t pos;
  jack_nframes_t i;
  struct jack_midi_event_buffer * event_buffer;

  port_buf = jack_port_get_buffer(g_jack_input_port, nframes);
  event_count = jack_midi_get_event_count(port_buf);
  jack_transport_query(g_jack_client, &pos);
  
  for (i = 0 ; i < event_count; i++)
  {
    jack_midi_event_get(&in_event, port_buf, i);

    LOG_DEBUG("midi event with size %u received. (jack_process)", (unsigned int)in_event.size);

    /* allocate memory for buffer copy */
    event_buffer = rtsafe_memory_allocate(g_memory, sizeof(struct jack_midi_event_buffer) + in_event.size);
    if (event_buffer == NULL)
    {
      LOG_ERROR("Ignored midi event with size %u because memory allocation failed.", (unsigned int)in_event.size);
      continue;
    }

    /* copy buffer data */
    memcpy(event_buffer->buffer, in_event.buffer, in_event.size);
    event_buffer->buffer_size = in_event.size;

    /* Add event buffer to g_jack_midi_events_pending_rt list */
    list_add(&event_buffer->siblings, &g_jack_midi_events_pending_rt);
  }

  ret = pthread_mutex_trylock(&g_jack_midi_mutex);
  if (ret == 0)
  {
    /* We are lucky and we got the lock */
    LOG_DEBUG("Lucky");

    /* Move pending events in g_jack_midi_events_pending_rt to begining of g_jack_midi_events_pending */
    list_splice_init(&g_jack_midi_events_pending_rt, &g_jack_midi_events_pending);

    /* wakeup jack_midi_thread */
    //ret = pthread_cond_broadcast(&g_jack_midi_cond);
    //LOG_DEBUG("pthread_cond_broadcast() result is %d", ret);
    ret = pthread_mutex_unlock(&g_jack_midi_mutex);
    //LOG_DEBUG("pthread_mutex_unlock() result is %d", ret);
  }
  else
  {
    /* We are not lucky, jack_midi_thread has locked the mutex */
    LOG_DEBUG("Not lucky (%d)", ret);
  }

  return 0;
}

gboolean
jack_midi_decode(
  guint8 * buffer,
  size_t buffer_size,
  GString * msg_str_ptr,
  GString * channel_str_ptr)
{
  size_t i;
  unsigned int channel;
  unsigned int note;
  unsigned int velocity;
  const char * note_name;
  unsigned int octave;
  unsigned int controller;
  unsigned int value;
  const char * controller_name;
  signed int pitch;
  const char * drum_name;

  if (buffer_size == 1 && buffer[0] == 0xFE)
  {
    g_string_sprintf(msg_str_ptr, "Active sensing");
    return FALSE;               /* disable */
  }

  if (buffer_size == 1 && buffer[0] == 0xF8)
  {
    g_string_sprintf(msg_str_ptr, "Timing Clock");
    return FALSE;               /* disable */
  }

  if (buffer_size == 3 && (buffer[0] >> 4) == 0x08)
  {
    channel = (buffer[0] & 0x0F) + 1; /* 1 .. 16 */
    assert(channel >= 1 && channel <= 16);

    if (channel == 10)
    {
      return FALSE;                                 /* ignore note off for drums */
    }

    note = buffer[1];
    if (note > 127)
    {
      goto unknown_event;
    }

    velocity = buffer[2];
    if (velocity > 127)
    {
      goto unknown_event;
    }

    note_name = g_note_names[note % 12];
    octave = note / 12 - 1;

    g_string_sprintf(channel_str_ptr, "%u", channel);

    g_string_sprintf(
      msg_str_ptr,
      "Note off, %s, octave %d, velocity %u",
      note_name,
      octave,
      velocity);

    return TRUE;
  }

  if (buffer_size == 3 && (buffer[0] >> 4) == 0x09)
  {
    channel = (buffer[0] & 0x0F) + 1; /* 1 .. 16 */
    assert(channel >= 1 && channel <= 16);

    note = buffer[1];
    if (note > 127)
    {
      goto unknown_event;
    }

    velocity = buffer[2];
    if (velocity > 127)
    {
      goto unknown_event;
    }

    note_name = g_note_names[note % 12];
    octave = note / 12 - 1;

    g_string_sprintf(channel_str_ptr, "%u", channel);

    if (channel == 10)
    {
      drum_name = gm_get_drum_name(note);
    }
    else
    {
      drum_name = NULL;
    }

    if (drum_name != NULL)
    {
      if (velocity == 0)
      {
        return FALSE;                                 /* ignore note off for drums */
      }

      g_string_sprintf(
        msg_str_ptr,
        "Drum: %s (%s, octave %d, velocity %u)",
        drum_name,
        note_name,
        octave,
        velocity);
    }
    else
    {
      g_string_sprintf(
        msg_str_ptr,
        "Note on, %s, octave %d, velocity %u",
        note_name,
        octave,
        velocity);
    }

    return TRUE;
  }

  if (buffer_size == 3 && (buffer[0] >> 4) == 0x0A)
  {
    channel = (buffer[0] & 0x0F) + 1; /* 1 .. 16 */
    assert(channel >= 1 && channel <= 16);

    note = buffer[1];
    if (note > 127)
    {
      goto unknown_event;
    }

    velocity = buffer[2];
    if (velocity > 127)
    {
      goto unknown_event;
    }

    note_name = g_note_names[note % 12];
    octave = note / 12 - 1;

    g_string_sprintf(channel_str_ptr, "%u", channel);

    g_string_sprintf(
      msg_str_ptr,
      "Polyphonic Key Pressure (Aftertouch), %s, octave %d, velocity %u",
      note_name,
      octave,
      velocity);

    return TRUE;
  }

  if (buffer_size == 3 && (buffer[0] >> 4) == 0x0B)
  {
    channel = (buffer[0] & 0x0F) + 1; /* 1 .. 16 */
    assert(channel >= 1 && channel <= 16);

    controller = buffer[1];
    if (controller > 127)
    {
      goto unknown_event;
    }

    value = buffer[2];
    if (value > 127)
    {
      goto unknown_event;
    }

    g_string_sprintf(channel_str_ptr, "%u", channel);

    controller_name = gm_get_controller_name(controller);

    if (controller_name != NULL)
    {
      g_string_sprintf(
        msg_str_ptr,
        "CC %s (%u), value %u",
        controller_name,
        controller,
        value);
    }
    else
    {
      g_string_sprintf(
        msg_str_ptr,
        "CC %u, value %u",
        controller,
        value);
    }

    return TRUE;
  }

  if (buffer_size == 2 && (buffer[0] >> 4) == 0x0C)
  {
    channel = (buffer[0] & 0x0F) + 1; /* 1 .. 16 */
    assert(channel >= 1 && channel <= 16);

    value = buffer[1];
    if (value > 127)
    {
      goto unknown_event;
    }

    g_string_sprintf(channel_str_ptr, "%u", channel);

    g_string_sprintf(
      msg_str_ptr,
      "Program change, %d (%s)",
      value,
      gm_get_instrument_name(value));

    return TRUE;
  }

  if (buffer_size == 3 && (buffer[0] >> 4) == 0x0E && buffer[1] <= 127 && buffer[2] <= 127)
  {
    channel = (buffer[0] & 0x0F) + 1; /* 1 .. 16 */
    assert(channel >= 1 && channel <= 16);

    pitch = buffer[1];
    pitch |= (unsigned int)buffer[2] << 7;
    pitch -= 0x2000;

    g_string_sprintf(channel_str_ptr, "%u", channel);

    g_string_sprintf(
      msg_str_ptr,
      "Pitchwheel, %d",
      pitch);

    return TRUE;
  }

  if (buffer_size > 0 && buffer[0] == 0xF0)
  {
    decode_sysex(buffer, buffer_size, msg_str_ptr);
    return TRUE;
  }

  if (buffer_size == 1 && buffer[0] == 0xFF)
  {
    g_string_sprintf(
      msg_str_ptr,
      "Reset");
   
    return TRUE;
  }

unknown_event:
  g_string_sprintf(
    msg_str_ptr,
    "unknown midi event with size %u bytes:",
    (unsigned int)buffer_size);

  for (i = 0 ; i < buffer_size ; i++)
  {
    g_string_append_printf(
      msg_str_ptr,
      " %02X",
      (unsigned int)(buffer[i]));
  }

  return TRUE;
}

/* The JACK MIDI input handling thread */
void *
jack_midi_thread(void * context_ptr)
{
  struct jack_midi_event_buffer * event_buffer;
  struct list_head * node_ptr;
  GtkListStore * list_store_ptr;
  GtkWidget * child_ptr;
  GtkTreeIter iter;
  GString * time_str_ptr;
  GString * msg_str_ptr;
  GString * channel_str_ptr;

  LOG_DEBUG("jack_midi_thread started");

  child_ptr = GTK_WIDGET (gtk_builder_get_object (g_builder, "list"));

  list_store_ptr = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(child_ptr)));

loop:
  pthread_mutex_lock(&g_jack_midi_mutex);

  if (g_jack_midi_thread_exit)
  {
    pthread_mutex_unlock(&g_jack_midi_mutex);

    LOG_DEBUG("jack_midi_thread exiting");
    return NULL;
  }

  rtsafe_memory_sleepy(g_memory);

  //LOG_DEBUG("checking events...");

  while (!list_empty(&g_jack_midi_events_pending))
  {
    node_ptr = g_jack_midi_events_pending.prev;

    list_del(node_ptr);

    event_buffer = list_entry(node_ptr, struct jack_midi_event_buffer, siblings);

    if (g_midi_ignore)
    {
      goto deallocate_event;
    }

    LOG_DEBUG("midi event with size %u received.", (unsigned int)event_buffer->buffer_size);

    time_str_ptr = g_string_new("");
    channel_str_ptr = g_string_new("");
    msg_str_ptr = g_string_new("");

    if (!jack_midi_decode(
          event_buffer->buffer,
          event_buffer->buffer_size,
          msg_str_ptr,
          channel_str_ptr))
    {
      /* ignoring specific event */
      goto deallocate_strings;
    }

    /* get GTK thread lock */
    gdk_threads_enter();

    if (g_row_count >= MAX_LIST_SIZE)
    {
      gtk_tree_model_get_iter_first(
        GTK_TREE_MODEL(list_store_ptr),
        &iter);

      gtk_list_store_remove(
        list_store_ptr,
        &iter);
    }

    /* Append an empty row to the list store. Iter will point to the new row */
    gtk_list_store_append(list_store_ptr, &iter);

    gtk_list_store_set(
      list_store_ptr,
      &iter,
      COL_TIME, time_str_ptr->str,
      COL_CHANNEL, channel_str_ptr->str,
      COL_MESSAGE, msg_str_ptr->str,
      -1);

    gtk_tree_view_scroll_to_cell(
      GTK_TREE_VIEW(child_ptr),
      gtk_tree_model_get_path(
        gtk_tree_view_get_model(GTK_TREE_VIEW(child_ptr)),
        &iter),
      NULL,
      TRUE,
      0.0,
      1.0);

    /* Force update of scroll position. */
    /* Is it a bug that it does not update automagically ? */
    gtk_container_check_resize(GTK_CONTAINER(child_ptr));

    g_row_count++;

    /* release GTK thread lock */
    gdk_threads_leave();

  deallocate_strings:
    g_string_free(channel_str_ptr, TRUE);
    g_string_free(msg_str_ptr, TRUE);
    g_string_free(time_str_ptr, TRUE);

  deallocate_event:
    rtsafe_memory_deallocate(event_buffer);
  }

  pthread_mutex_unlock(&g_jack_midi_mutex);

  //LOG_DEBUG("waiting for more events...");

  //pthread_cond_wait(&g_jack_midi_cond, &g_jack_midi_mutex);
  usleep(10000);

  goto loop;
}

void
jack_destroy_pending_events()
{
  struct jack_midi_event_buffer * event_buffer;
  struct list_head * node_ptr;

  while (!list_empty(&g_jack_midi_events_pending))
  {
    LOG_DEBUG("Destroying pending event");
    node_ptr = g_jack_midi_events_pending.next;

    list_del(node_ptr);

    event_buffer = list_entry(node_ptr, struct jack_midi_event_buffer, siblings);

    rtsafe_memory_deallocate(event_buffer);
  }

  while (!list_empty(&g_jack_midi_events_pending_rt))
  {
    LOG_DEBUG("Destroying realtime pending event");
    node_ptr = g_jack_midi_events_pending_rt.next;

    list_del(node_ptr);

    event_buffer = list_entry(node_ptr, struct jack_midi_event_buffer, siblings);

    rtsafe_memory_deallocate(event_buffer);
  }
}

gboolean
jack_init(const char * name)
{
  int ret;

  LOG_DEBUG("jack_init(\"%s\") called.", name);

  if (!rtsafe_memory_init(
        100 * 1024,
        100,
        1000,
        &g_memory))
  {
    LOG_ERROR("RT-safe memory initialization failed.");
    goto fail;
  }

  INIT_LIST_HEAD(&g_jack_midi_events_pending);
  INIT_LIST_HEAD(&g_jack_midi_events_pending_rt);

  ret = pthread_mutex_init(&g_jack_midi_mutex,  NULL);
  if (ret != 0)
  {
    LOG_ERROR("Cannot initialize mutex.");
    goto fail_uninit_memory;
  }

/*   ret = pthread_cond_init(&g_jack_midi_cond,  NULL); */
/*   if (ret != 0) */
/*   { */
/*     LOG_ERROR("Cannot initialize condition (variable)."); */
/*     goto fail_uninit_mutex; */
/*   } */

  g_jack_midi_thread_exit = FALSE;

  ret = pthread_create(&g_jack_midi_tid, NULL, jack_midi_thread, NULL);
  if (ret != 0)
  {
    LOG_ERROR("Cannot start JACK MIDI thread.");
    goto fail_uninit_cond;
  }

  g_jack_client = jack_client_open(name, JackNoStartServer, NULL);
  if (g_jack_client == NULL)
  {
    LOG_ERROR("Cannot create JACK client.");
    goto fail_stop_thread;
  }

  ret = jack_set_process_callback(g_jack_client, jack_process, 0);
  if (ret != 0)
  {
    LOG_ERROR("Cannot set JACK process callback.");
    goto fail_close;
  }

  g_jack_input_port = jack_port_register(g_jack_client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
  if (g_jack_input_port == NULL)
  {
    LOG_ERROR("Failed to create input JACK MIDI port");
    goto fail_close;
  }

  ret = jack_activate(g_jack_client);
  if (ret != 0)
  {
    LOG_ERROR("Cannot activate JACK client.");
    goto fail_close;
  }

  return TRUE;

fail_close:
  jack_client_close(g_jack_client);

fail_stop_thread:
  jack_destroy_pending_events();

  /* Tell jack_midi_thread to exit */
  pthread_mutex_lock(&g_jack_midi_mutex);
  g_jack_midi_thread_exit = TRUE;
  pthread_mutex_unlock(&g_jack_midi_mutex);

  /* wakeup jack_midi_thread */
/*   pthread_cond_broadcast(&g_jack_midi_cond); */

  /* wait thread to finish */
  pthread_join(g_jack_midi_tid, NULL);

fail_uninit_cond:
/*   pthread_cond_destroy(&g_jack_midi_cond); */

/* fail_uninit_mutex: */
  pthread_mutex_destroy(&g_jack_midi_mutex);

fail_uninit_memory:
  rtsafe_memory_uninit(g_memory);

fail:
  return FALSE;
}

void
jack_uninit()
{
  LOG_DEBUG("jack_uninit() called.");

  jack_deactivate(g_jack_client);

  jack_client_close(g_jack_client);

  /* Tell jack_midi_thread to exit */
  pthread_mutex_lock(&g_jack_midi_mutex);
  g_jack_midi_thread_exit = TRUE;
  pthread_mutex_unlock(&g_jack_midi_mutex);

  /* wakeup jack_midi_thread */
/*   pthread_cond_broadcast(&g_jack_midi_cond); */

  /* wait thread to finish */
  pthread_join(g_jack_midi_tid, NULL);

  jack_destroy_pending_events();

/*   pthread_cond_destroy(&g_jack_midi_cond); */

  pthread_mutex_destroy(&g_jack_midi_mutex);

  rtsafe_memory_uninit(g_memory);
}
