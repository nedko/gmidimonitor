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
#include <string.h>
#include <unistd.h>

#include "jack.h"
#include "memory_atomic.h"
#define LOG_LEVEL LOG_LEVEL_DEBUG
#include "log.h"
#include "list.h"

#ifdef HAVE_OLD_JACK_MIDI
#define jack_midi_get_event_count(port_buf, nframes) jack_midi_port_get_info(port_buf, nframes)->event_count
#endif

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
  event_count = jack_midi_get_event_count(port_buf, nframes);
  jack_transport_query(g_jack_client, &pos);
  
  for (i = 0 ; i < event_count; i++)
  {
    jack_midi_event_get(&in_event, port_buf, i, nframes);

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

    ret = pthread_mutex_trylock(&g_jack_midi_mutex);
    if (ret == 0)
    {
      /* We are lucky and we got the lock */
      LOG_DEBUG("Lucky");

      /* Move pending events in g_jack_midi_events_pending_rt to begining of g_jack_midi_events_pending */
      list_splice_init(&g_jack_midi_events_pending_rt, &g_jack_midi_events_pending);

      /* Add event buffer to g_jack_midi_events_pending list */
      list_add(&event_buffer->siblings, &g_jack_midi_events_pending);

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

      /* Add event buffer to g_jack_midi_events_pending_rt list */
      list_add(&event_buffer->siblings, &g_jack_midi_events_pending_rt);
    }
  }

  return 0;
}

/* The JACK MIDI input handling thread */
void *
jack_midi_thread(void * context_ptr)
{
  struct jack_midi_event_buffer * event_buffer;
  struct list_head * node_ptr;

  LOG_DEBUG("jack_midi_thread started");

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

    LOG_DEBUG("midi event with size %u received.", (unsigned int)event_buffer->buffer_size);

    rtsafe_memory_deallocate(event_buffer);
  }

  pthread_mutex_unlock(&g_jack_midi_mutex);

  //LOG_DEBUG("waiting for more events...");

  //pthread_cond_wait(&g_jack_midi_cond, &g_jack_midi_mutex);
  usleep(100000);

  goto loop;
}

void
jack_destroy_pending_events()
{
  struct jack_midi_event_buffer * event_buffer;
  struct list_head * node_ptr;

  while (!list_empty(&g_jack_midi_events_pending))
  {
    node_ptr = g_jack_midi_events_pending.next;

    list_del(node_ptr);

    event_buffer = list_entry(node_ptr, struct jack_midi_event_buffer, siblings);

    rtsafe_memory_deallocate(event_buffer);
  }

  while (!list_empty(&g_jack_midi_events_pending_rt))
  {
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

  g_jack_client = jack_client_new(name);
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
