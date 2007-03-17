/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 *   gmidimonitor main code.
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

#include "config.h"

#include <gtk/gtk.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef HAVE_ALSA_MIDI
#include <alsa/asoundlib.h>
#endif

#ifdef HAVE_JACK_MIDI
#include <jack/jack.h>
#include <jack/midiport.h>
#endif

#ifdef HAVE_OLD_JACK_MIDI
#define jack_midi_get_event_count(port_buf, nframes) jack_midi_port_get_info(port_buf, nframes)->event_count
#endif

#ifdef HAVE_LASH
#include <lash/lash.h>
#endif

#include "path.h"
#include "glade.h"
#include "gm.h"

GtkWidget * g_main_window_ptr;

#ifdef HAVE_ALSA_MIDI
snd_seq_t * g_seq_ptr;
#endif

#ifdef HAVE_JACK_MIDI
jack_client_t * g_jack_client;
jack_port_t * g_jack_input_port;
#endif

gboolean g_midi_ignore = FALSE;

int g_row_count;

#ifdef HAVE_LASH
lash_client_t * g_lashc;
#endif

#ifdef HAVE_ALSA_MIDI
static const char * g_note_names[12] = 
{
  "C",
  "C#",
  "D",
  "D#",
  "E",
  "F",
  "F#",
  "G",
  "G#",
  "A",
  "A#",
  "B",
};
#endif

enum
{
  COL_TIME,
  COL_CHANNEL,
  COL_MESSAGE,
  NUM_COLS
};

#define MAX_LIST_SIZE 2000

void
create_mainwindow()
{
  GtkWidget * child_ptr;
  GtkListStore * list_store_ptr;
  GtkTreeViewColumn * column_ptr;
  GtkCellRenderer * text_renderer_ptr;

  g_main_window_ptr = construct_glade_widget("main_window");

  child_ptr = get_glade_widget_child(g_main_window_ptr, "list");

  text_renderer_ptr = gtk_cell_renderer_text_new();

  g_object_set(
    G_OBJECT(text_renderer_ptr),
    "family",
    "Monospace",
    NULL);

/*   column_ptr = gtk_tree_view_column_new_with_attributes( */
/*     "Time", */
/*     text_renderer_ptr, */
/*     "text", COL_TIME, */
/*     NULL); */

/*   gtk_tree_view_append_column( */
/*     GTK_TREE_VIEW(child_ptr), */
/*     column_ptr); */

  column_ptr = gtk_tree_view_column_new_with_attributes(
    "Channel",
    text_renderer_ptr,
    "text", COL_CHANNEL,
    NULL);

  gtk_tree_view_append_column(
    GTK_TREE_VIEW(child_ptr),
    column_ptr);

  column_ptr = gtk_tree_view_column_new_with_attributes(
    "Message",
    text_renderer_ptr,
    "text", COL_MESSAGE,
    NULL);

  gtk_tree_view_append_column(
    GTK_TREE_VIEW(child_ptr),
    column_ptr);

  gtk_widget_show_all(g_main_window_ptr);

  list_store_ptr = gtk_list_store_new(
    NUM_COLS,
    G_TYPE_STRING,
    G_TYPE_STRING,
    G_TYPE_STRING);

  gtk_tree_view_set_model(GTK_TREE_VIEW(child_ptr), GTK_TREE_MODEL(list_store_ptr));
}

/* stop button toggle */
void on_button_stop_toggled(
  GtkAction * action_ptr,
  GtkWidget * widget_ptr)
{
  GtkWidget * child_ptr;

  child_ptr = get_glade_widget_child(widget_ptr, "button_stop");

  if (gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(child_ptr)))
  {
    g_midi_ignore = TRUE;
  }
  else
  {
    g_midi_ignore = FALSE;
  }
}

void on_clear_clicked
(GtkButton *button, gpointer user_data)
{
  gtk_list_store_clear(
    GTK_LIST_STORE(
      gtk_tree_view_get_model(
        GTK_TREE_VIEW(
          get_glade_widget_child(
            g_main_window_ptr,
            "list")))));
  g_row_count = 0;
}

#ifdef HAVE_ALSA_MIDI
/* The ALSA MIDI input handling thread */
void *
alsa_midi_thread(void * context_ptr)
{
  GtkTreeIter iter;
  snd_seq_event_t * event_ptr;
  GtkListStore * list_store_ptr;
  GtkWidget * child_ptr;
  GString * time_str_ptr;
  GString * msg_str_ptr;
  GString * channel_str_ptr;
  const char * note_name;
  int octave;
  const char * drum_name;
  const char * cc_name;
  int i;
  const char * mmc_command_name;

  child_ptr = get_glade_widget_child(g_main_window_ptr, "list");

  list_store_ptr = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(child_ptr)));

  g_row_count = 0;

  while (snd_seq_event_input(g_seq_ptr, &event_ptr) >= 0)
  {
    if (g_midi_ignore)
      continue;

    time_str_ptr = g_string_new("");
    g_string_sprintf(
      time_str_ptr,
      "%u:%u",
      (unsigned int)event_ptr->time.time.tv_sec,
      (unsigned int)event_ptr->time.time.tv_nsec);
    channel_str_ptr = g_string_new("");

    /* Workaround for compiler warnings... */
    drum_name = NULL;
    note_name = NULL;
    octave = 0;

    if (event_ptr->type == SND_SEQ_EVENT_NOTE ||
        event_ptr->type == SND_SEQ_EVENT_NOTEON ||
        event_ptr->type == SND_SEQ_EVENT_NOTEOFF ||
        event_ptr->type == SND_SEQ_EVENT_KEYPRESS)
    {
      g_string_sprintf(
        channel_str_ptr,
        "%u",
        (unsigned int)event_ptr->data.note.channel+1);
      if (event_ptr->data.note.channel + 1 == 10)
      {
        drum_name = gm_get_drum_name(event_ptr->data.note.note);
      }
      else
      {
        drum_name = NULL;
      }

      note_name = g_note_names[event_ptr->data.note.note % 12];
      octave = event_ptr->data.note.note / 12 - 1;
    }

    if (event_ptr->type == SND_SEQ_EVENT_CONTROLLER ||
        event_ptr->type == SND_SEQ_EVENT_PGMCHANGE ||
        event_ptr->type == SND_SEQ_EVENT_PITCHBEND)
    {
      g_string_sprintf(
        channel_str_ptr,
        "%u",
        (unsigned int)event_ptr->data.control.channel+1);
    }

    msg_str_ptr = g_string_new("unknown event");

    switch (event_ptr->type)
    {
    case SND_SEQ_EVENT_SYSTEM:
      g_string_sprintf(msg_str_ptr, "System event"); 
      break;
    case SND_SEQ_EVENT_RESULT:
      g_string_sprintf(msg_str_ptr, "Result status event"); 
      break;
    case SND_SEQ_EVENT_NOTE:
      g_string_sprintf(msg_str_ptr, "Note");
      break;
    case SND_SEQ_EVENT_NOTEON:
      if (event_ptr->data.note.velocity != 0)
      {
        if (drum_name != NULL)
        {
          g_string_sprintf(
            msg_str_ptr,
            "Drum: %s (%s, octave %d, velocity %u)",
            drum_name,
            note_name,
            octave,
            event_ptr->data.note.velocity);
        }
        else
        {
          g_string_sprintf(
            msg_str_ptr,
            "Note on , %s, octave %d, velocity %u",
            note_name,
            octave,
            event_ptr->data.note.velocity);
        }
        break;
      }
    case SND_SEQ_EVENT_NOTEOFF:
      if (drum_name != NULL)    /* ignore note off for drums */
        continue;

      g_string_sprintf(
        msg_str_ptr,
        "Note off, %s, octave %d",
        note_name,
        octave);

      break;
    case SND_SEQ_EVENT_KEYPRESS:
      g_string_sprintf(msg_str_ptr, "Key pressure change (aftertouch)");
      break;
    case SND_SEQ_EVENT_CONTROLLER:
      cc_name = NULL;
      switch (event_ptr->data.control.param)
      {
      case MIDI_CTL_MSB_BANK:
        cc_name = "Bank selection";
        break;
      case MIDI_CTL_MSB_MODWHEEL:
        cc_name = "Modulation";
        break;
      case MIDI_CTL_MSB_BREATH:
        cc_name = "Breath";
        break;
      case MIDI_CTL_MSB_FOOT:
        cc_name = "Foot";
        break;
      case MIDI_CTL_MSB_PORTAMENTO_TIME:
        cc_name = "Portamento time";
        break;
      case MIDI_CTL_MSB_DATA_ENTRY:
        cc_name = "Data entry";
        break;
      case MIDI_CTL_MSB_MAIN_VOLUME:
        cc_name = "Main volume";
        break;
      case MIDI_CTL_MSB_BALANCE:
        cc_name = "Balance";
        break;
      case MIDI_CTL_MSB_PAN:
        cc_name = "Panpot";
        break;
      case MIDI_CTL_MSB_EXPRESSION:
        cc_name = "Expression";
        break;
      case MIDI_CTL_MSB_EFFECT1:
        cc_name = "Effect1";
        break;
      case MIDI_CTL_MSB_EFFECT2:
        cc_name = "Effect2";
        break;
      case MIDI_CTL_MSB_GENERAL_PURPOSE1:
        cc_name = "General purpose 1";
        break;
      case MIDI_CTL_MSB_GENERAL_PURPOSE2:
        cc_name = "General purpose 2";
        break;
      case MIDI_CTL_MSB_GENERAL_PURPOSE3:
        cc_name = "General purpose 3";
        break;
      case MIDI_CTL_MSB_GENERAL_PURPOSE4:
        cc_name = "General purpose 4";
        break;
      case MIDI_CTL_LSB_BANK:
        cc_name = "Bank selection";
        break;
      case MIDI_CTL_LSB_MODWHEEL:
        cc_name = "Modulation";
        break;
      case MIDI_CTL_LSB_BREATH:
        cc_name = "Breath";
        break;
      case MIDI_CTL_LSB_FOOT:
        cc_name = "Foot";
        break;
      case MIDI_CTL_LSB_PORTAMENTO_TIME:
        cc_name = "Portamento time";
        break;
      case MIDI_CTL_LSB_DATA_ENTRY:
        cc_name = "Data entry";
        break;
      case MIDI_CTL_LSB_MAIN_VOLUME:
        cc_name = "Main volume";
        break;
      case MIDI_CTL_LSB_BALANCE:
        cc_name = "Balance";
        break;
      case MIDI_CTL_LSB_PAN:
        cc_name = "Panpot";
        break;
      case MIDI_CTL_LSB_EXPRESSION:
        cc_name = "Expression";
        break;
      case MIDI_CTL_LSB_EFFECT1:
        cc_name = "Effect1";
        break;
      case MIDI_CTL_LSB_EFFECT2:
        cc_name = "Effect2";
        break;
      case MIDI_CTL_LSB_GENERAL_PURPOSE1:
        cc_name = "General purpose 1";
        break;
      case MIDI_CTL_LSB_GENERAL_PURPOSE2:
        cc_name = "General purpose 2";
        break;
      case MIDI_CTL_LSB_GENERAL_PURPOSE3:
        cc_name = "General purpose 3";
        break;
      case MIDI_CTL_LSB_GENERAL_PURPOSE4:
        cc_name = "General purpose 4";
        break;
      case MIDI_CTL_SUSTAIN:
        cc_name = "Sustain pedal";
        break;
      case MIDI_CTL_PORTAMENTO:
        cc_name = "Portamento";
        break;
      case MIDI_CTL_SOSTENUTO:
        cc_name = "Sostenuto";
        break;
      case MIDI_CTL_SOFT_PEDAL:
        cc_name = "Soft pedal";
        break;
      case MIDI_CTL_LEGATO_FOOTSWITCH:
        cc_name = "Legato foot switch";
        break;
      case MIDI_CTL_HOLD2:
        cc_name = "Hold2";
        break;
      case MIDI_CTL_SC1_SOUND_VARIATION:
        cc_name = "SC1 Sound Variation";
        break;
      case MIDI_CTL_SC2_TIMBRE:
        cc_name = "SC2 Timbre";
        break;
      case MIDI_CTL_SC3_RELEASE_TIME:
        cc_name = "SC3 Release Time";
        break;
      case MIDI_CTL_SC4_ATTACK_TIME:
        cc_name = "SC4 Attack Time";
        break;
      case MIDI_CTL_SC5_BRIGHTNESS:
        cc_name = "SC5 Brightness";
        break;
      case MIDI_CTL_SC6:
        cc_name = "SC6";
        break;
      case MIDI_CTL_SC7:
        cc_name = "SC7";
        break;
      case MIDI_CTL_SC8:
        cc_name = "SC8";
        break;
      case MIDI_CTL_SC9:
        cc_name = "SC9";
        break;
      case MIDI_CTL_SC10:
        cc_name = "SC10";
        break;
      case MIDI_CTL_GENERAL_PURPOSE5:
        cc_name = "General purpose 5";
        break;
      case MIDI_CTL_GENERAL_PURPOSE6:
        cc_name = "General purpose 6";
        break;
      case MIDI_CTL_GENERAL_PURPOSE7:
        cc_name = "General purpose 7";
        break;
      case MIDI_CTL_GENERAL_PURPOSE8:
        cc_name = "General purpose 8";
        break;
      case MIDI_CTL_PORTAMENTO_CONTROL:
        cc_name = "Portamento control";
        break;
      case MIDI_CTL_E1_REVERB_DEPTH:
        cc_name = "E1 Reverb Depth";
        break;
      case MIDI_CTL_E2_TREMOLO_DEPTH:
        cc_name = "E2 Tremolo Depth";
        break;
      case MIDI_CTL_E3_CHORUS_DEPTH:
        cc_name = "E3 Chorus Depth";
        break;
      case MIDI_CTL_E4_DETUNE_DEPTH:
        cc_name = "E4 Detune Depth";
        break;
      case MIDI_CTL_E5_PHASER_DEPTH:
        cc_name = "E5 Phaser Depth";
        break;
      case MIDI_CTL_DATA_INCREMENT:
        cc_name = "Data Increment";
        break;
      case MIDI_CTL_DATA_DECREMENT:
        cc_name = "Data Decrement";
        break;
      case MIDI_CTL_NONREG_PARM_NUM_LSB:
        cc_name = "Non-registered parameter number";
        break;
      case MIDI_CTL_NONREG_PARM_NUM_MSB:
        cc_name = "Non-registered parameter number";
        break;
      case MIDI_CTL_REGIST_PARM_NUM_LSB:
        cc_name = "Registered parameter number";
        break;
      case MIDI_CTL_REGIST_PARM_NUM_MSB:
        cc_name = "Registered parameter number";
        break;
      case MIDI_CTL_ALL_SOUNDS_OFF:
        cc_name = "All sounds off";
        break;
      case MIDI_CTL_RESET_CONTROLLERS:
        cc_name = "Reset Controllers";
        break;
      case MIDI_CTL_LOCAL_CONTROL_SWITCH:
        cc_name = "Local control switch";
        break;
      case MIDI_CTL_ALL_NOTES_OFF:
        cc_name = "All notes off";
        break;
      case MIDI_CTL_OMNI_OFF:
        cc_name = "Omni off";
        break;
      case MIDI_CTL_OMNI_ON:
        cc_name = "Omni on";
        break;
      case MIDI_CTL_MONO1:
        cc_name = "Mono1";
        break;
      case MIDI_CTL_MONO2:
        cc_name = "Mono2";
        break;
      }

      if (cc_name != NULL)
      {
        g_string_sprintf(
          msg_str_ptr,
          "CC %s (%u), value %u",
          cc_name,
          (unsigned int)event_ptr->data.control.param,
          (unsigned int)event_ptr->data.control.value);
      }
      else
      {
        g_string_sprintf(
          msg_str_ptr,
          "CC %u, value %u",
          (unsigned int)event_ptr->data.control.param,
          (unsigned int)event_ptr->data.control.value);
      }
      break;
    case SND_SEQ_EVENT_PGMCHANGE:
      g_string_sprintf(
        msg_str_ptr,
        "Program change, %d (%s)",
        (unsigned int)event_ptr->data.control.value,
        event_ptr->data.control.value > 127 || event_ptr->data.control.value < 0 ? "???": gm_get_instrument_name(event_ptr->data.control.value));
      break;
    case SND_SEQ_EVENT_CHANPRESS:
      g_string_sprintf(msg_str_ptr, "Channel pressure");
      break;
    case SND_SEQ_EVENT_PITCHBEND:
      g_string_sprintf(
        msg_str_ptr,
        "Pitchwheel, %d",
        (unsigned int)event_ptr->data.control.value);
      break;
    case SND_SEQ_EVENT_CONTROL14:
      g_string_sprintf(msg_str_ptr, "14 bit controller value");
      break;
    case SND_SEQ_EVENT_NONREGPARAM:
      g_string_sprintf(msg_str_ptr, "NRPN");
      break;
    case SND_SEQ_EVENT_REGPARAM:
      g_string_sprintf(msg_str_ptr, "RPN");
      break;
    case SND_SEQ_EVENT_SONGPOS:
      g_string_sprintf(msg_str_ptr, "Song position");
      break;
    case SND_SEQ_EVENT_SONGSEL:
      g_string_sprintf(msg_str_ptr, "Song select");
      break;
    case SND_SEQ_EVENT_QFRAME:
      g_string_sprintf(msg_str_ptr, "midi time code quarter frame");
      break;
    case SND_SEQ_EVENT_TIMESIGN:
      g_string_sprintf(msg_str_ptr, "SMF Time Signature event");
      break;
    case SND_SEQ_EVENT_KEYSIGN:
      g_string_sprintf(msg_str_ptr, "SMF Key Signature event");
      break;
    case SND_SEQ_EVENT_START:
      g_string_sprintf(msg_str_ptr, "MIDI Real Time Start message");
      break;
    case SND_SEQ_EVENT_CONTINUE:
      g_string_sprintf(msg_str_ptr, "MIDI Real Time Continue message");
      break;
    case SND_SEQ_EVENT_STOP:
      g_string_sprintf(msg_str_ptr, "MIDI Real Time Stop message");
      break;
    case SND_SEQ_EVENT_SETPOS_TICK:
      g_string_sprintf(msg_str_ptr, "Set tick queue position");
      break;
    case SND_SEQ_EVENT_SETPOS_TIME:
      g_string_sprintf(msg_str_ptr, "Set real-time queue position");
      break;
    case SND_SEQ_EVENT_TEMPO:
      g_string_sprintf(msg_str_ptr, "(SMF) Tempo event");
      break;
    case SND_SEQ_EVENT_CLOCK:
      g_string_sprintf(msg_str_ptr, "MIDI Real Time Clock message");
      break;
    case SND_SEQ_EVENT_TICK:
      g_string_sprintf(msg_str_ptr, "MIDI Real Time Tick message");
      break;
    case SND_SEQ_EVENT_QUEUE_SKEW:
      g_string_sprintf(msg_str_ptr, "Queue timer skew");
      break;
    case SND_SEQ_EVENT_SYNC_POS:
      g_string_sprintf(msg_str_ptr, "Sync position changed");
      break;
    case SND_SEQ_EVENT_TUNE_REQUEST:
      g_string_sprintf(msg_str_ptr, "Tune request");
      break;
    case SND_SEQ_EVENT_RESET:
      g_string_sprintf(msg_str_ptr, "Reset");
      break;
    case SND_SEQ_EVENT_SENSING:
      continue;                 /* disable */
      g_string_sprintf(msg_str_ptr, "Active sensing");
      break;
    case SND_SEQ_EVENT_ECHO:
      g_string_sprintf(msg_str_ptr, "Echo-back event");
      break;
    case SND_SEQ_EVENT_OSS:
      g_string_sprintf(msg_str_ptr, "OSS emulation raw event");
      break;
    case SND_SEQ_EVENT_CLIENT_START:
      g_string_sprintf(msg_str_ptr, "New client has connected");
      break;
    case SND_SEQ_EVENT_CLIENT_EXIT:
      g_string_sprintf(msg_str_ptr, "Client has left the system");
      break;
    case SND_SEQ_EVENT_CLIENT_CHANGE:
      g_string_sprintf(msg_str_ptr, "Client status/info has changed");
      break;
    case SND_SEQ_EVENT_PORT_START:
      g_string_sprintf(msg_str_ptr, "New port was created");
      break;
    case SND_SEQ_EVENT_PORT_EXIT:
      g_string_sprintf(msg_str_ptr, "Port was deleted from system");
      break;
    case SND_SEQ_EVENT_PORT_CHANGE:
      g_string_sprintf(msg_str_ptr, "Port status/info has changed");
      break;
    case SND_SEQ_EVENT_PORT_SUBSCRIBED:
      g_string_sprintf(msg_str_ptr, "Port connected");
      break;
    case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
      g_string_sprintf(msg_str_ptr, "Port disconnected");
      break;
    case SND_SEQ_EVENT_SAMPLE:
      g_string_sprintf(msg_str_ptr, "Sample select");
      break;
    case SND_SEQ_EVENT_SAMPLE_CLUSTER:
      g_string_sprintf(msg_str_ptr, "Sample cluster select");
      break;
    case SND_SEQ_EVENT_SAMPLE_START:
      g_string_sprintf(msg_str_ptr, "voice start");
      break;
    case SND_SEQ_EVENT_SAMPLE_STOP:
      g_string_sprintf(msg_str_ptr, "voice stop");
      break;
    case SND_SEQ_EVENT_SAMPLE_FREQ:
      g_string_sprintf(msg_str_ptr, "playback frequency");
      break;
    case SND_SEQ_EVENT_SAMPLE_VOLUME:
      g_string_sprintf(msg_str_ptr, "volume and balance");
      break;
    case SND_SEQ_EVENT_SAMPLE_LOOP:
      g_string_sprintf(msg_str_ptr, "sample loop");
      break;
    case SND_SEQ_EVENT_SAMPLE_POSITION:
      g_string_sprintf(msg_str_ptr, "sample position");
      break;
    case SND_SEQ_EVENT_SAMPLE_PRIVATE1:
      g_string_sprintf(msg_str_ptr, "private (hardware dependent) event");
      break;
    case SND_SEQ_EVENT_USR0:
      g_string_sprintf(msg_str_ptr, "user-defined event");
      break;
    case SND_SEQ_EVENT_USR1:
      g_string_sprintf(msg_str_ptr, "user-defined event");
      break;
    case SND_SEQ_EVENT_USR2:
      g_string_sprintf(msg_str_ptr, "user-defined event");
      break;
    case SND_SEQ_EVENT_USR3:
      g_string_sprintf(msg_str_ptr, "user-defined event");
      break;
    case SND_SEQ_EVENT_USR4:
      g_string_sprintf(msg_str_ptr, "user-defined event");
      break;
    case SND_SEQ_EVENT_USR5:
      g_string_sprintf(msg_str_ptr, "user-defined event");
      break;
    case SND_SEQ_EVENT_USR6:
      g_string_sprintf(msg_str_ptr, "user-defined event");
      break;
    case SND_SEQ_EVENT_USR7:
      g_string_sprintf(msg_str_ptr, "user-defined event");
      break;
    case SND_SEQ_EVENT_USR8:
      g_string_sprintf(msg_str_ptr, "user-defined event");
      break;
    case SND_SEQ_EVENT_USR9:
      g_string_sprintf(msg_str_ptr, "user-defined event");
      break;
    case SND_SEQ_EVENT_INSTR_BEGIN:
      g_string_sprintf(msg_str_ptr, "begin of instrument management");
      break;
    case SND_SEQ_EVENT_INSTR_END:
      g_string_sprintf(msg_str_ptr, "end of instrument management");
      break;
    case SND_SEQ_EVENT_INSTR_INFO:
      g_string_sprintf(msg_str_ptr, "query instrument interface info");
      break;
    case SND_SEQ_EVENT_INSTR_INFO_RESULT:
      g_string_sprintf(msg_str_ptr, "result of instrument interface info");
      break;
    case SND_SEQ_EVENT_INSTR_FINFO:
      g_string_sprintf(msg_str_ptr, "query instrument format info");
      break;
    case SND_SEQ_EVENT_INSTR_FINFO_RESULT:
      g_string_sprintf(msg_str_ptr, "result of instrument format info");
      break;
    case SND_SEQ_EVENT_INSTR_RESET:
      g_string_sprintf(msg_str_ptr, "reset instrument instrument memory");
      break;
    case SND_SEQ_EVENT_INSTR_STATUS:
      g_string_sprintf(msg_str_ptr, "get instrument interface status");
      break;
    case SND_SEQ_EVENT_INSTR_STATUS_RESULT:
      g_string_sprintf(msg_str_ptr, "result of instrument interface status");
      break;
    case SND_SEQ_EVENT_INSTR_PUT:
      g_string_sprintf(msg_str_ptr, "put an instrument to port");
      break;
    case SND_SEQ_EVENT_INSTR_GET:
      g_string_sprintf(msg_str_ptr, "get an instrument from port");
      break;
    case SND_SEQ_EVENT_INSTR_GET_RESULT:
      g_string_sprintf(msg_str_ptr, "result of instrument query");
      break;
    case SND_SEQ_EVENT_INSTR_FREE:
      g_string_sprintf(msg_str_ptr, "free instrument(s)");
      break;
    case SND_SEQ_EVENT_INSTR_LIST:
      g_string_sprintf(msg_str_ptr, "get instrument list");
      break;
    case SND_SEQ_EVENT_INSTR_LIST_RESULT:
      g_string_sprintf(msg_str_ptr, "result of instrument list");
      break;
    case SND_SEQ_EVENT_INSTR_CLUSTER:
      g_string_sprintf(msg_str_ptr, "set cluster parameters");
      break;
    case SND_SEQ_EVENT_INSTR_CLUSTER_GET:
      g_string_sprintf(msg_str_ptr, "get cluster parameters");
      break;
    case SND_SEQ_EVENT_INSTR_CLUSTER_RESULT:
      g_string_sprintf(msg_str_ptr, "result of cluster parameters");
      break;
    case SND_SEQ_EVENT_INSTR_CHANGE:
      g_string_sprintf(msg_str_ptr, "instrument change");
      break;
    case SND_SEQ_EVENT_SYSEX:
      /* General MMC decoding, as seen at http://www.borg.com/~jglatt/tech/mmc.htm and
         extended from "Advanced User Guide for MK-449C MIDI keyboard" info */
      if (event_ptr->data.ext.len == 6 &&
          ((guint8 *)event_ptr->data.ext.ptr)[0] == 0xF0 &&
          ((guint8 *)event_ptr->data.ext.ptr)[1] == 0x7F &&
          ((guint8 *)event_ptr->data.ext.ptr)[3] == 0x06 &&
          ((guint8 *)event_ptr->data.ext.ptr)[5] == 0xF7)
      {
        switch (((guint8 *)event_ptr->data.ext.ptr)[4])
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

        if (((guint8 *)event_ptr->data.ext.ptr)[2] == 127)
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
            (unsigned int)(((guint8 *)event_ptr->data.ext.ptr)[2]));
        }
      }
      /* The goto MMC message, as seen at http://www.borg.com/~jglatt/tech/mmc.htm*/
      else if (event_ptr->data.ext.len == 13 &&
               ((guint8 *)event_ptr->data.ext.ptr)[0] == 0xF0 &&
               ((guint8 *)event_ptr->data.ext.ptr)[1] == 0x7F &&
               ((guint8 *)event_ptr->data.ext.ptr)[3] == 0x06 &&
               ((guint8 *)event_ptr->data.ext.ptr)[4] == 0x44 &&
               ((guint8 *)event_ptr->data.ext.ptr)[5] == 0x06 &&
               ((guint8 *)event_ptr->data.ext.ptr)[6] == 0x01 &&
               ((guint8 *)event_ptr->data.ext.ptr)[12] == 0xF7)
      {
        g_string_sprintf(
          msg_str_ptr,
          "MMC goto %u:%u:%u/%u:%u",
          (unsigned int)(((guint8 *)event_ptr->data.ext.ptr)[7] & 0x1F), /* fps encoding */
          (unsigned int)(((guint8 *)event_ptr->data.ext.ptr)[8]),
          (unsigned int)(((guint8 *)event_ptr->data.ext.ptr)[9]),
          (unsigned int)(((guint8 *)event_ptr->data.ext.ptr)[10] & 0x1F), /* no fps > 32, but bit 5 looks
                                                                             used for something */
          (unsigned int)(((guint8 *)event_ptr->data.ext.ptr)[11]));

        switch (((guint8 *)event_ptr->data.ext.ptr)[7] & 0x60)
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

        if (((guint8 *)event_ptr->data.ext.ptr)[2] == 127)
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
            (unsigned int)(((guint8 *)event_ptr->data.ext.ptr)[2]));
        }
      }
      else
      {
      generic_sysex:
        g_string_sprintf(
          msg_str_ptr,
          "SYSEX with size %u:",
          (unsigned int)event_ptr->data.ext.len);
        for (i = 0 ; i < event_ptr->data.ext.len ; i++)
        {
          g_string_append_printf(
            msg_str_ptr,
            " %02X",
            (unsigned int)(((guint8 *)event_ptr->data.ext.ptr)[i]));
        }
      }
      break;
    case SND_SEQ_EVENT_BOUNCE:
      g_string_sprintf(msg_str_ptr, "error event");
      break;
    case SND_SEQ_EVENT_USR_VAR0:
      g_string_sprintf(msg_str_ptr, "reserved for user apps");
      break;
    case SND_SEQ_EVENT_USR_VAR1:
      g_string_sprintf(msg_str_ptr, "reserved for user apps");
      break;
    case SND_SEQ_EVENT_USR_VAR2:
      g_string_sprintf(msg_str_ptr, "reserved for user apps");
      break;
    case SND_SEQ_EVENT_USR_VAR3:
      g_string_sprintf(msg_str_ptr, "reserved for user apps");
      break;
    case SND_SEQ_EVENT_USR_VAR4:
      g_string_sprintf(msg_str_ptr, "reserved for user apps");
      break;
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

    g_string_free(channel_str_ptr, TRUE);
    g_string_free(msg_str_ptr, TRUE);

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

    /* release GTK thread lock */
    gdk_threads_leave();

    g_row_count++;
  }

  return NULL;
}

#endif  /* #ifdef HAVE_ALSA_MIDI */

#ifdef HAVE_JACK_MIDI

void
jack_print_raw(unsigned char* buf, size_t len)
{
  size_t i;
  for (i = 0 ; i < len; i++)
  {
    printf("0x%X ", buf[i] & 0xFF);
  }

  printf("\n");
}

void
jack_print_sysex(unsigned char* buf, size_t len)
{
  size_t i;

  assert(buf[0] == 0xF0 && buf[1] == 0x7F);

  if (buf[3] == 0x01 && buf[4] == 0x01)
  {
    printf("MTC Full Frame (%hhu:%hhu:%hhu:%hhu)\n", buf[5] &0xF, buf[6], buf[7], buf[8]);
  }
  else if (buf[3] == 0x06 && buf[4] == 0x01)
  {
    printf("MMC Stop\n");
  }
  else if (buf[3] == 0x06 && buf[4] == 0x02)
  {
    printf("MMC Play\n");
  }
  else if (buf[3] == 0x06 && buf[4] == 0x03)
  {
    printf("MMC Deferred Play\n");
  }
  else if (buf[3] == 0x06 && buf[4] == 0x04)
  {
    printf("MMC Fast Forward\n");
  }
  else if (buf[3] == 0x06 && buf[4] == 0x05)
  {
    printf("MMC Rewind\n");
  }
  else if (buf[3] == 0x06 && buf[4] == 0x06)
  {
    printf("MMC Record Strobe (Punch In)\n");
  }
  else if (buf[3] == 0x06 && buf[4] == 0x07)
  {
    printf("MMC Record Exit (Punch Out)\n");
  }
  else if (buf[3] == 0x06 && buf[4] == 0x09)
  {
    printf("MMC Pause\n");
  }
  else if (buf[3] == 0x06 && buf[4] == 0x44 && buf[5] == 0x06 && buf[6] == 0x01)
  {
    printf("MMC Goto (%hhu:%hhu:%hhu:%hhu:%hhu)\n", buf[7], buf[8], buf[9], buf[10], buf[11]);
  }
  else
  {
    printf("Unknown sysex: ");
    for (i = 0; i < len; i++)
    {
      printf("0x%X ", buf[i] & 0xFF);

      if (buf[i] == 0xF7)
      {
        break;
      }
    }

    printf("\n");
  }
}


void
jack_print_midi_message(unsigned char* buf, size_t len)
{ 
  if (buf[0] == 0xF0 && buf[1] == 0x7F)
  {
    jack_print_sysex(buf, len);
  }
  else if (buf[0] == 0xF1)
  {
    printf("MTC Quarter Frame %d\n", (buf[1] & 0xF0) >> 4);
  }
  else
  {
    printf("Unknown Raw: ");
    jack_print_raw(buf, len);
  }
}

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

    jack_print_midi_message(in_event.buffer, in_event.size);
  }

  return 0;
}

#endif

#ifdef HAVE_LASH

void
process_lash_event(lash_event_t * event_ptr)
{
  enum LASH_Event_Type type;
  const char * str;

  type = lash_event_get_type(event_ptr);
  str = lash_event_get_string(event_ptr);

  switch (type)
  {
  case LASH_Quit:
    g_warning("LASH_Quit received.\n");
    g_lashc = NULL;
    gtk_main_quit();
    break;
  case LASH_Save_File:
  case LASH_Restore_File:
  case LASH_Save_Data_Set:
  default:
    g_warning("LASH Event. Type = %u, string = \"%s\"\n",
              (unsigned int)type,
              (str == NULL)?"":str);
  }
}

void
process_lash_config(lash_config_t * config_ptr)
{
  const char * key;
  const void * data;
  size_t data_size;

  key = lash_config_get_key(config_ptr);
  data = lash_config_get_value(config_ptr);
  data_size = lash_config_get_value_size(config_ptr);

  g_warning("LASH Config. Key = \"%s\"\n", key);
}

/* process lash events callback */
gboolean
process_lash_events(gpointer data)
{
  lash_event_t * event_ptr;
  lash_config_t * config_ptr;

  /* Process events */
  while ((event_ptr = lash_get_event(g_lashc)) != NULL)
  {
    process_lash_event(event_ptr);
    lash_event_destroy(event_ptr); 
  }

  /* Process configs */
  while ((config_ptr = lash_get_config(g_lashc)) != NULL)
  {
    process_lash_config(config_ptr);
    lash_config_destroy(config_ptr);  
  }

  return TRUE;
}

#endif  /* #ifdef HAVE_LASH */

int
main(int argc, char *argv[])
{
  int ret;
#ifdef HAVE_ALSA_MIDI
  snd_seq_port_info_t * port_info = NULL;
  pthread_t midi_tid;
#endif
#ifdef HAVE_LASH
  lash_event_t * lash_event_ptr;
#endif
  GString * client_name_str_ptr;

  /* init threads */
  g_thread_init(NULL);
  gdk_threads_init();
  gdk_threads_enter();

  gtk_init(&argc, &argv);

  path_init(argv[0]);

#ifdef HAVE_LASH
  g_lashc = lash_init(
    lash_extract_args(&argc, &argv),
    "gmidimonitor",
    0,
    LASH_PROTOCOL_VERSION); 

  if (g_lashc == NULL)
  {
    g_warning("Failed to connect to LASH. Session management will not occur.\n");
  }
  else
  {
    lash_event_ptr = lash_event_new_with_type(LASH_Client_Name);
    lash_event_set_string(lash_event_ptr, "GMIDImonitor");
    lash_send_event(g_lashc, lash_event_ptr);
    g_timeout_add(250, process_lash_events, NULL);
  }
#endif

  /* interface creation */
  create_mainwindow();

  client_name_str_ptr = g_string_new("");
  g_string_sprintf(client_name_str_ptr, "MIDI monitor (%u)", (unsigned int)getpid());

#ifdef HAVE_JACK_MIDI
  /* JACK client initialization */
  {
    if ((g_jack_client = jack_client_new(client_name_str_ptr->str)) == 0)
    {
      fprintf(stderr, "jack server not running?\n");
      return 1;
    }

    ret = jack_set_process_callback(g_jack_client, jack_process, 0);
    /* TODO: error handling */

/*     jack_set_sample_rate_callback(g_jack_client, srate, 0); */

/*     jack_on_shutdown(g_jack_client, jack_shutdown, 0); */

    g_jack_input_port = jack_port_register(g_jack_client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);

    if (jack_activate(g_jack_client))
    {
      /* TODO: proper (goto) error handling */
      fprintf(stderr, "cannot activate client");
      return 1;
    }
  }
#endif  /* #ifdef HAVE_JACK_MIDI */

#ifdef HAVE_ALSA_MIDI
  /* ALSA MIDI initialisation */
  ret = snd_seq_open(
    &g_seq_ptr,
    "default",
    SND_SEQ_OPEN_INPUT,
    0);
  if (ret < 0)
  {
    g_warning("Cannot open sequncer, %s\n", snd_strerror(ret));
    goto path_uninit;
  }

  snd_seq_set_client_name(g_seq_ptr, client_name_str_ptr->str);

#ifdef HAVE_LASH
  lash_alsa_client_id(g_lashc, snd_seq_client_id(g_seq_ptr));
#endif

  snd_seq_port_info_alloca(&port_info);

  snd_seq_port_info_set_capability(
    port_info, 
    SND_SEQ_PORT_CAP_WRITE | 
    SND_SEQ_PORT_CAP_SUBS_WRITE);
  snd_seq_port_info_set_type(
    port_info, 
    SND_SEQ_PORT_TYPE_APPLICATION);
  snd_seq_port_info_set_midi_channels(port_info, 16);
  snd_seq_port_info_set_port_specified(port_info, 1);

  snd_seq_port_info_set_name(port_info, "midi in");
  snd_seq_port_info_set_port(port_info, 0);

  ret = snd_seq_create_port(g_seq_ptr, port_info);
  if (ret < 0)
  {
    g_warning("Error creating ALSA sequencer port, %s\n", snd_strerror(ret));
    goto close_seq;
  }

  /* Start midi thread */
  ret = pthread_create(&midi_tid, NULL, alsa_midi_thread, NULL);
#endif  /* #ifdef HAVE_ALSA_MIDI */

  /* main loop */
  gtk_main();

#ifdef HAVE_ALSA_MIDI
  /* Cancel the thread. Don't know better way.
     Poll or unblock mechanisms seem to not be
     available for alsa sequencer */
  pthread_cancel(midi_tid);

  /* Wait midi thread to finish */
  ret = pthread_join(midi_tid, NULL);
#endif  /* #ifdef HAVE_ALSA_MIDI */

#ifdef HAVE_JACK_MIDI
  jack_client_close(g_jack_client);
#endif  /* #ifdef HAVE_JACK_MIDI */

  gdk_threads_leave();

#ifdef HAVE_ALSA_MIDI
close_seq:
  ret = snd_seq_close(g_seq_ptr);
  if (ret < 0)
  {
    g_warning("Cannot close sequncer, %s\n", snd_strerror(ret));
  }

path_uninit:
#endif  /* #ifdef HAVE_ALSA_MIDI */
  path_init(argv[0]);

  return 0;
}
