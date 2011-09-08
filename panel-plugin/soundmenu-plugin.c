/*  $Id$
 *
 *  Copyright (c) 2011 John Doo <john@foo.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gtk/gtk.h>
#include <glib.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-hvbox.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>

#ifdef HAVE_LIBKEYBINDER
#include <keybinder.h>
#endif

#include "soundmenu-plugin.h"
#include "soundmenu-dialogs.h"

/* default settings */
#define DEFAULT_PLAYER "pragha"
#define DEFAULT_SETTING2 1
#define DEFAULT_SHOW_STOP TRUE

/* prototypes */

static void
soundmenu_construct (XfcePanelPlugin *plugin);

/* register the plugin */

XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL (soundmenu_construct);

/* Function to update the soundmenu state */

void
play_button_toggle_state (SoundmenuPlugin *soundmenu)
{
	if ((soundmenu->state == ST_PAUSED) || (soundmenu->state == ST_STOPPED))
		gtk_button_set_image(GTK_BUTTON(soundmenu->play_button), soundmenu->image_play);
	else
		gtk_button_set_image(GTK_BUTTON(soundmenu->play_button), soundmenu->image_pause);
}

gboolean status_get_tooltip_cb (GtkWidget        *widget,
					gint              x,
					gint              y,
					gboolean          keyboard_mode,
					GtkTooltip       *tooltip,
					SoundmenuPlugin *soundmenu)
{
	gchar *markup_text = NULL;

	gint volume = (soundmenu->volume*100);

	if (soundmenu->state == ST_STOPPED)
		markup_text = g_strdup_printf("%s", _("Stopped"));
	else {
		markup_text = g_markup_printf_escaped(_("<b>%s</b> (Volume: %d%%)\nby %s in %s"),
						(soundmenu->metadata->title && strlen(soundmenu->metadata->title)) ?
						soundmenu->metadata->title : soundmenu->metadata->url,
						volume,
						(soundmenu->metadata->artist && strlen(soundmenu->metadata->artist)) ?
						soundmenu->metadata->artist : _("Unknown Artist"),
						(soundmenu->metadata->album && strlen(soundmenu->metadata->album)) ?
						soundmenu->metadata->album : _("Unknown Album"));
	}

	gtk_tooltip_set_markup (tooltip, markup_text);

	g_free(markup_text);

	return TRUE;
}

static void
update_state(gchar *state, SoundmenuPlugin *soundmenu)
{
	if (0 == g_ascii_strcasecmp(state, "Playing"))
		soundmenu->state = ST_PLAYING;
	else if (0 == g_ascii_strcasecmp(state, "Paused"))
		soundmenu->state = ST_PAUSED;
	else {
		soundmenu->state = ST_STOPPED;
	}
	play_button_toggle_state(soundmenu);
}

Metadata *malloc_metadata()
{
	Metadata *m;
	m = malloc(sizeof(Metadata));

	m->trackid = NULL;
	m->url = NULL;
	m->title = NULL;
	m->artist = NULL;
	m->album = NULL;
	m->length = 0;
	m->trackNumber = 0;
	m->arturl = NULL;

	return m;
}

void free_metadata(Metadata *m)
{
	if(m == NULL)
		return;

	if(m->trackid)	free(m->trackid);
	if(m->url)		free(m->url);
	if(m->title)	free(m->title);
	if(m->artist)	free(m->artist);
	if(m->album) 	free(m->album);
	if(m->arturl)	free(m->arturl);

	free(m);
}

/* Dbus helpers to parse Metadata info, etc.. */

static void
get_meta_item_array(DBusMessageIter *dict_entry, char **item)
{
	DBusMessageIter variant, array;
	char *str_buf;

	dbus_message_iter_next(dict_entry);
	dbus_message_iter_recurse(dict_entry, &array);

	dbus_message_iter_recurse(&array, &variant);
	dbus_message_iter_get_basic(&variant, (void*) &str_buf);

	*item = malloc(strlen(str_buf) + 1);
	strcpy(*item, str_buf);
}

static void
get_meta_item_str(DBusMessageIter *dict_entry, char **item)
{
	DBusMessageIter variant;
	char *str_buf;

	dbus_message_iter_next(dict_entry);
	dbus_message_iter_recurse(dict_entry, &variant);
	dbus_message_iter_get_basic(&variant, (void*) &str_buf);

	*item = malloc(strlen(str_buf) + 1);
	strcpy(*item, str_buf);
}

static void
get_meta_item_gint(DBusMessageIter *dict_entry, void *item)
{
	DBusMessageIter variant;

	dbus_message_iter_next(dict_entry);
	dbus_message_iter_recurse(dict_entry, &variant);
	dbus_message_iter_get_basic(&variant, (void*) item);
}

void
demarshal_metadata (DBusMessageIter *args, SoundmenuPlugin *soundmenu)	// arg inited on Metadata string
{
	DBG ("Demarshal_metadata");

	DBusMessageIter dict, dict_entry, variant;
	Metadata *metadata;
	gchar *str_buf = NULL, *string = NULL;

	gint64 length = 0;
	gint32 trackNumber = 0;
	
	metadata = malloc_metadata();

	dbus_message_iter_recurse(args, &dict);		// Recurse => dict on fist "dict entry()"
	
	dbus_message_iter_recurse(&dict, &dict_entry);	// Recurse => dict_entry on "string "mpris:trackid""
	do
	{
		dbus_message_iter_recurse(&dict_entry, &variant);
		dbus_message_iter_get_basic(&variant, (void*) &str_buf);

		if (0 == g_ascii_strcasecmp (str_buf, "mpris:trackid"))
			get_meta_item_str(&variant, &metadata->trackid);
		else if (0 == g_ascii_strcasecmp (str_buf, "xesam:url"))
			get_meta_item_str(&variant, &metadata->url);
		else if (0 == g_ascii_strcasecmp (str_buf, "xesam:title"))
			get_meta_item_str(&variant, &metadata->title);
		else if (0 == g_ascii_strcasecmp (str_buf, "xesam:artist"))
			get_meta_item_array(&variant, &metadata->artist);
		else if (0 == g_ascii_strcasecmp (str_buf, "xesam:album"))
			get_meta_item_str(&variant, &metadata->album);
		else if (0 == g_ascii_strcasecmp (str_buf, "xesam:genre"));
			/* (List of Strings.) Not use genre */
		else if (0 == g_ascii_strcasecmp (str_buf, "xesam:albumArtist"));
			// List of Strings.
		else if (0 == g_ascii_strcasecmp (str_buf, "xesam:comment"));
			/* (List of Strings) Not use comment */
		else if (0 == g_ascii_strcasecmp (str_buf, "xesam:audioBitrate"));
			/* (uint32) Not use audioBitrate */
		else if (0 == g_ascii_strcasecmp (str_buf, "mpris:length"))
			get_meta_item_gint(&variant, &length);
		else if (0 == g_ascii_strcasecmp (str_buf, "xesam:trackNumber"))
			get_meta_item_gint(&variant, &trackNumber);
		else if (0 == g_ascii_strcasecmp (str_buf, "xesam:useCount"));
			/* (Integer) Not use useCount */
		else if (0 == g_ascii_strcasecmp (str_buf, "xesam:userRating"));
			/* (Float) Not use userRating */
		else if (0 == g_ascii_strcasecmp (str_buf, "mpris:arturl"))
			get_meta_item_str(&variant, &metadata->arturl);
		else
			DBG ("New metadata message: %s. (Investigate)\n", str_buf);
	
	} while (dbus_message_iter_next(&dict_entry));

	metadata->length = length;
	metadata->trackNumber = trackNumber;

	free_metadata(soundmenu->metadata);
	soundmenu->metadata = metadata;
}

/* Basic dbus functions for interacting with MPRIS2*/

static DBusHandlerResult
dbus_filter (DBusConnection *connection, DBusMessage *message, void *user_data)
{
	DBusMessageIter args, dict, dict_entry;
	gchar *str_buf = NULL, *state = NULL;
	gdouble volume = 0;

	SoundmenuPlugin *soundmenu = user_data;

	if ( dbus_message_is_signal (message, "org.freedesktop.DBus.Properties", "PropertiesChanged" ) )
	{
		dbus_message_iter_init(message, &args);

		/* Ignore the interface_name*/
		dbus_message_iter_next(&args);

		dbus_message_iter_recurse(&args, &dict);
		do
		{
			dbus_message_iter_recurse(&dict, &dict_entry);
			dbus_message_iter_get_basic(&dict_entry, (void*) &str_buf);

			if (0 == g_ascii_strcasecmp (str_buf, "PlaybackStatus"))
			{
				get_meta_item_str (&dict_entry, &state);
				update_state (state, soundmenu);
			}
			else if (0 == g_ascii_strcasecmp (str_buf, "Volume"))
			{
				get_meta_item_gint(&dict_entry, &volume);
				soundmenu->volume = volume;
			}
			else if (0 == g_ascii_strcasecmp (str_buf, "Metadata"))
			{
				/* Ignore inferface string and send the pointer to metadata. */
				dbus_message_iter_next(&dict_entry);
				demarshal_metadata (&dict_entry, soundmenu);
			}
		} while (dbus_message_iter_next(&dict));

		return DBUS_HANDLER_RESULT_HANDLED;
	}
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void
send_message (SoundmenuPlugin *soundmenu, const char *msg)
{
	DBusMessage *message;
	gchar *destination = NULL;

	destination = g_strdup_printf ("org.mpris.MediaPlayer2.%s", soundmenu->player);
	message = dbus_message_new_method_call (destination, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player",  msg);
	g_free(destination);

	/* Send the message */
	dbus_connection_send (soundmenu->connection, message, NULL);
	dbus_message_unref (message);
}

void
get_playbackstatus (SoundmenuPlugin *soundmenu)
{
	DBusMessage *message = NULL, *reply_message = NULL;
	DBusMessageIter dict_entry, variant;
	gchar *destination = NULL, *state= NULL;

	const char * const interface_name = "org.mpris.MediaPlayer2.Player";
	const char * const query = "PlaybackStatus";

	destination = g_strdup_printf ("org.mpris.MediaPlayer2.%s", soundmenu->player);

	message = dbus_message_new_method_call (destination, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "Get");
	dbus_message_append_args(message,
					DBUS_TYPE_STRING, &interface_name,
					DBUS_TYPE_STRING, &query,
					DBUS_TYPE_INVALID);

	if(reply_message = dbus_connection_send_with_reply_and_block (soundmenu->connection, message, -1, NULL)) {
		dbus_message_iter_init(reply_message, &dict_entry);
		dbus_message_iter_recurse(&dict_entry, &variant);

		dbus_message_iter_get_basic(&variant, (void*) &state);
		update_state (state, soundmenu);
	}

	dbus_message_unref (message);
	g_free(destination);
}

void
get_metadata (SoundmenuPlugin *soundmenu)
{
	DBusMessage *message = NULL, *reply_message = NULL;
	DBusMessageIter args;
	gchar *destination = NULL;

	const char * const interface_name = "org.mpris.MediaPlayer2.Player";
	const char * const query = "Metadata";

	destination = g_strdup_printf ("org.mpris.MediaPlayer2.%s", soundmenu->player);

	message = dbus_message_new_method_call (destination, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "Get");
	dbus_message_append_args(message,
					DBUS_TYPE_STRING, &interface_name,
					DBUS_TYPE_STRING, &query,
					DBUS_TYPE_INVALID);

	if(reply_message = dbus_connection_send_with_reply_and_block (soundmenu->connection, message, -1, NULL)) {
		dbus_message_iter_init(reply_message, &args);
		demarshal_metadata (&args, soundmenu);
	}

	dbus_message_unref (message);
	g_free(destination);
}

void
get_volume (SoundmenuPlugin *soundmenu)
{
	DBusMessage *message = NULL, *reply_message = NULL;
	DBusMessageIter dict_entry, variant;
	gchar *destination = NULL;
	gdouble volume = 0;

	const char * const interface_name = "org.mpris.MediaPlayer2.Player";
	const char * const query = "Volume";

	destination = g_strdup_printf ("org.mpris.MediaPlayer2.%s", soundmenu->player);

	message = dbus_message_new_method_call (destination, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "Get");
	dbus_message_append_args(message,
					DBUS_TYPE_STRING, &interface_name,
					DBUS_TYPE_STRING, &query,
					DBUS_TYPE_INVALID);

	if(reply_message = dbus_connection_send_with_reply_and_block (soundmenu->connection, message, -1, NULL)) {
		dbus_message_iter_init(reply_message, &dict_entry);
		dbus_message_iter_recurse(&dict_entry, &variant);
		dbus_message_iter_get_basic(&variant, &volume);
	}
	soundmenu->volume = volume;

	dbus_message_unref (message);
	g_free(destination);
}

void update_player_status (SoundmenuPlugin *soundmenu)
{
	get_playbackstatus (soundmenu);
	get_metadata (soundmenu);
	get_volume (soundmenu);
}

/* Callbacks of button controls */

void
prev_button_handler(GtkButton *button, SoundmenuPlugin *soundmenu)
{
	send_message (soundmenu, "Previous");
}

void
play_button_handler(GtkButton *button, SoundmenuPlugin *soundmenu)
{
	send_message (soundmenu, "PlayPause");
}

void
stop_button_handler(GtkButton *button, SoundmenuPlugin    *soundmenu)
{
	send_message (soundmenu, "Stop");
}

void
next_button_handler(GtkButton *button, SoundmenuPlugin    *soundmenu)
{
	send_message (soundmenu, "Next");
}

#ifdef HAVE_LIBKEYBINDER
void keybind_play_handler (const char *keystring, SoundmenuPlugin *soundmenu)
{
	send_message (soundmenu, "PlayPause");
}
void keybind_stop_handler (const char *keystring, SoundmenuPlugin *soundmenu)
{
	send_message (soundmenu, "Stop");
}
void keybind_prev_handler (const char *keystring, SoundmenuPlugin *soundmenu)
{
	send_message (soundmenu, "Previous");
}
void keybind_next_handler (const char *keystring, SoundmenuPlugin *soundmenu)
{
	send_message (soundmenu, "Next");
}

void init_keybinder(SoundmenuPlugin *soundmenu)
{
	keybinder_init ();

	keybinder_bind("XF86AudioPlay", (KeybinderHandler) keybind_play_handler, soundmenu);
	keybinder_bind("XF86AudioStop", (KeybinderHandler) keybind_stop_handler, soundmenu);
	keybinder_bind("XF86AudioPrev", (KeybinderHandler) keybind_prev_handler, soundmenu);
	keybinder_bind("XF86AudioNext", (KeybinderHandler) keybind_next_handler, soundmenu);
}

void uninit_keybinder(SoundmenuPlugin *soundmenu)
{
	keybinder_unbind("XF86AudioPlay", (KeybinderHandler) keybind_play_handler);
	keybinder_unbind("XF86AudioStop", (KeybinderHandler) keybind_stop_handler);
	keybinder_unbind("XF86AudioPrev", (KeybinderHandler) keybind_prev_handler);
	keybinder_unbind("XF86AudioNext", (KeybinderHandler) keybind_next_handler);
}
#endif

/*
 * First intent to set the volume.
 * DBUS_TYPE_VARIANT is not implemented on dbus_message_append_args.
 * I did not know implement a variant of double into a container.
 * Any Help?????

static gboolean
panel_button_scrolled (GtkWidget        *widget,
				GdkEventScroll   *event,
				SoundmenuPlugin *soundmenu)
{
	DBusMessage *message = NULL;
	DBusMessageIter value_iter, iter_dict_entry, variant;
	gchar *destination = NULL;

	const char * const interface_name = "org.mpris.MediaPlayer2.Player";
	const char * const query = "Volume";

	switch (event->direction)
	{
	case GDK_SCROLL_UP:
	case GDK_SCROLL_RIGHT:
		soundmenu->volume += 0.02;
		break;
	case GDK_SCROLL_DOWN:
	case GDK_SCROLL_LEFT:
		soundmenu->volume -= 0.02;
		break;
	}

	soundmenu->volume = CLAMP (soundmenu->volume, 0.0, 1.0);

	destination = g_strdup_printf ("org.mpris.MediaPlayer2.%s", soundmenu->player);
	message = dbus_message_new_method_call (destination, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "Set");
	dbus_message_append_args(message,
				DBUS_TYPE_STRING, &interface_name,
				DBUS_TYPE_STRING, &query,
				DBUS_TYPE_VARIANT, &iter_dict_entry,
				DBUS_TYPE_INVALID);

	// FIXME: DBUS_TYPE_VARIANT not implemented, therefore you have to do in a container.
	if(dbus_message_iter_open_container(&iter_dict_entry, DBUS_TYPE_VARIANT, DBUS_TYPE_DOUBLE_AS_STRING, &variant)) {
		dbus_message_iter_append_basic(&variant, DBUS_TYPE_DOUBLE, &(soundmenu->volume));
		dbus_message_iter_close_container(&iter_dict_entry, &variant);
	}

	dbus_connection_send (soundmenu->connection, message, NULL);
		
	dbus_message_unref (message);
	g_free(destination);

	return TRUE;
} */

/* Sound menu plugin construct */

void
soundmenu_save (XfcePanelPlugin *plugin,
             SoundmenuPlugin    *soundmenu)
{
	XfceRc *rc;
	gchar  *file;

	/* get the config file location */
	file = xfce_panel_plugin_save_location (plugin, TRUE);

	if (G_UNLIKELY (file == NULL)) {
		DBG ("Failed to open config file");
		return;
	}

	/* open the config file, read/write */
	rc = xfce_rc_simple_open (file, FALSE);
	g_free (file);

	if (G_LIKELY (rc != NULL)) {
		/* save the settings */
		DBG(".");
		if (soundmenu->player)
			xfce_rc_write_entry    (rc, "player", soundmenu->player);

		xfce_rc_write_int_entry  (rc, "setting2", soundmenu->setting2);
		xfce_rc_write_bool_entry (rc, "show_stop", soundmenu->show_stop);

		/* close the rc file */
		xfce_rc_close (rc);
	}
}

static void
soundmenu_read (SoundmenuPlugin *soundmenu)
{
	XfceRc      *rc;
	gchar       *file;
	const gchar *value;

	/* get the plugin config file location */
	file = xfce_panel_plugin_save_location (soundmenu->plugin, TRUE);

	if (G_LIKELY (file != NULL)) {
		/* open the config file, readonly */
		rc = xfce_rc_simple_open (file, TRUE);

		/* cleanup */
		g_free (file);

		if (G_LIKELY (rc != NULL)) {
			/* read the settings */
			value = xfce_rc_read_entry (rc, "player", DEFAULT_PLAYER);
			soundmenu->player = g_strdup (value);

			soundmenu->setting2 = xfce_rc_read_int_entry (rc, "setting2", DEFAULT_SETTING2);
			soundmenu->show_stop = xfce_rc_read_bool_entry (rc, "show_stop", DEFAULT_SHOW_STOP);

			soundmenu->state = ST_STOPPED;

			/* cleanup */
			xfce_rc_close (rc);

			/* leave the function, everything went well */
			return;
		}
	}

	/* something went wrong, apply default values */
	DBG ("Applying default settings");

	soundmenu->player = g_strdup (DEFAULT_PLAYER);
	soundmenu->setting2 = DEFAULT_SETTING2;
	soundmenu->show_stop = DEFAULT_SHOW_STOP;
	soundmenu->state = ST_STOPPED;
}

static SoundmenuPlugin *
soundmenu_new (XfcePanelPlugin *plugin)
{
	SoundmenuPlugin   *soundmenu;
	GtkOrientation orientation;
	GtkWidget *play_button, *stop_button, *prev_button, *next_button;
	DBusConnection *connection;
	Metadata *metadata;
	gchar *rule = NULL;

	/* allocate memory for the plugin structure */
	soundmenu = panel_slice_new0 (SoundmenuPlugin);

	/* pointer to plugin */
	soundmenu->plugin = plugin;

	/* read the user settings */
	soundmenu_read (soundmenu);

	/* get the current orientation */
	orientation = xfce_panel_plugin_get_orientation (plugin);

	/* create some panel widgets */

	soundmenu->hvbox = xfce_hvbox_new (orientation, FALSE, 2);
	gtk_widget_show (soundmenu->hvbox);

	/* some soundmenu widgets */

	prev_button = gtk_button_new();
	play_button = gtk_button_new();
	stop_button = gtk_button_new();
	next_button = gtk_button_new();

	gtk_button_set_relief(GTK_BUTTON(prev_button), GTK_RELIEF_NONE);
	gtk_button_set_relief(GTK_BUTTON(stop_button), GTK_RELIEF_NONE);
	gtk_button_set_relief(GTK_BUTTON(next_button), GTK_RELIEF_NONE);
	gtk_button_set_relief(GTK_BUTTON(play_button), GTK_RELIEF_NONE);

	gtk_button_set_image(GTK_BUTTON(prev_button),
			     gtk_image_new_from_stock(GTK_STOCK_MEDIA_PREVIOUS,
						      GTK_ICON_SIZE_LARGE_TOOLBAR));
	gtk_button_set_image(GTK_BUTTON(stop_button),
			     gtk_image_new_from_stock(GTK_STOCK_MEDIA_STOP,
						      GTK_ICON_SIZE_LARGE_TOOLBAR));
	gtk_button_set_image(GTK_BUTTON(next_button),
			     gtk_image_new_from_stock(GTK_STOCK_MEDIA_NEXT,
						      GTK_ICON_SIZE_LARGE_TOOLBAR));

	soundmenu->image_pause =
		gtk_image_new_from_stock(GTK_STOCK_MEDIA_PAUSE,
					 GTK_ICON_SIZE_LARGE_TOOLBAR);
	soundmenu->image_play =
		gtk_image_new_from_stock(GTK_STOCK_MEDIA_PLAY,
					 GTK_ICON_SIZE_LARGE_TOOLBAR);

	g_object_ref(soundmenu->image_play);
	g_object_ref(soundmenu->image_pause);

	gtk_button_set_image(GTK_BUTTON(play_button),
			     soundmenu->image_play);

	gtk_box_pack_start(GTK_BOX(soundmenu->hvbox),
			   GTK_WIDGET(prev_button),
			   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(soundmenu->hvbox),
			   GTK_WIDGET(play_button),
			   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(soundmenu->hvbox),
			   GTK_WIDGET(stop_button),
			   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(soundmenu->hvbox),
			   GTK_WIDGET(next_button),
			   FALSE, FALSE, 0);

	gtk_widget_show(prev_button);
	gtk_widget_show(play_button);
	if(soundmenu->show_stop)
		gtk_widget_show(stop_button);
	gtk_widget_show(next_button);

	/* Signal handlers */

	g_signal_connect(G_OBJECT(prev_button), "clicked",
			 G_CALLBACK(prev_button_handler), soundmenu);
	g_signal_connect(G_OBJECT(play_button), "clicked",
			 G_CALLBACK(play_button_handler), soundmenu);
	g_signal_connect(G_OBJECT(stop_button), "clicked",
			 G_CALLBACK(stop_button_handler), soundmenu);
	g_signal_connect(G_OBJECT(next_button), "clicked",
			 G_CALLBACK(next_button_handler), soundmenu);

	xfce_panel_plugin_add_action_widget (plugin, prev_button);
	xfce_panel_plugin_add_action_widget (plugin, play_button);
	xfce_panel_plugin_add_action_widget (plugin, stop_button);
	xfce_panel_plugin_add_action_widget (plugin, next_button);

	g_object_set (G_OBJECT(prev_button), "has-tooltip", TRUE, NULL);
	g_object_set (G_OBJECT(play_button), "has-tooltip", TRUE, NULL);
	g_object_set (G_OBJECT(stop_button), "has-tooltip", TRUE, NULL);
	g_object_set (G_OBJECT(next_button), "has-tooltip", TRUE, NULL);

	g_signal_connect(G_OBJECT(prev_button), "query-tooltip",
			G_CALLBACK(status_get_tooltip_cb), soundmenu);
	g_signal_connect(G_OBJECT(play_button), "query-tooltip",
			G_CALLBACK(status_get_tooltip_cb), soundmenu);
	g_signal_connect(G_OBJECT(stop_button), "query-tooltip",
			G_CALLBACK(status_get_tooltip_cb), soundmenu);
	g_signal_connect(G_OBJECT(next_button), "query-tooltip",
			G_CALLBACK(status_get_tooltip_cb), soundmenu);

	/* FIXME:
	 * See comments in the function panel_button_scrolled.
	g_signal_connect (G_OBJECT (play_button), "scroll-event",
			G_CALLBACK (panel_button_scrolled), soundmenu);*/

	soundmenu->prev_button = prev_button;
	soundmenu->play_button = play_button;
	soundmenu->stop_button = stop_button;
	soundmenu->next_button = next_button;

	metadata = malloc_metadata();
	soundmenu->metadata = metadata;

	/* Soundmenu dbus helpers */

	connection = dbus_bus_get (DBUS_BUS_SESSION, NULL);

	soundmenu->dbus_name = g_strdup_printf("org.mpris.MediaPlayer2.%s", soundmenu->player);

	rule = g_strdup_printf ("type='signal', sender='%s'", soundmenu->dbus_name);
	dbus_bus_add_match (connection, rule, NULL);
	g_free(rule);
  
	dbus_connection_add_filter (connection, dbus_filter, soundmenu, NULL);
	dbus_connection_setup_with_g_main (connection, NULL);

	soundmenu->connection = connection;
	
	update_player_status (soundmenu);

	return soundmenu;
}

static void
soundmenu_free (XfcePanelPlugin *plugin,
             SoundmenuPlugin    *soundmenu)
{
	GtkWidget *dialog;

	#ifdef HAVE_LIBKEYBINDER
	uninit_keybinder(soundmenu);
	#endif

	/* check if the dialog is still open. if so, destroy it */
	dialog = g_object_get_data (G_OBJECT (plugin), "dialog");
	if (G_UNLIKELY (dialog != NULL))
	gtk_widget_destroy (dialog);

	/* destroy the panel widgets */
	gtk_widget_destroy (soundmenu->hvbox);

	if (G_LIKELY (soundmenu->player != NULL))
		g_free (soundmenu->player);

	/* cleanup the metadata and settings */
	if (G_LIKELY (soundmenu->player != NULL))
		g_free (soundmenu->player);
	if (G_LIKELY (soundmenu->dbus_name != NULL))
		g_free (soundmenu->dbus_name);
	free_metadata(soundmenu->metadata);

	/* free the plugin structure */
	panel_slice_free (SoundmenuPlugin, soundmenu);
}



static void
soundmenu_orientation_changed (XfcePanelPlugin *plugin,
                            GtkOrientation   orientation,
                            SoundmenuPlugin    *soundmenu)
{
	/* change the orienation of the box */
	xfce_hvbox_set_orientation (XFCE_HVBOX (soundmenu->hvbox), orientation);
}



static gboolean
soundmenu_size_changed (XfcePanelPlugin *plugin,
                     gint             size,
                     SoundmenuPlugin    *soundmenu)
{
	GtkOrientation orientation;

	/* get the orientation of the plugin */
	orientation = xfce_panel_plugin_get_orientation (plugin);

	/* set the widget size */
	if (orientation == GTK_ORIENTATION_HORIZONTAL)
		gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, size);
	else
		gtk_widget_set_size_request (GTK_WIDGET (plugin), size, -1);

	/* we handled the orientation */
	return TRUE;
}



static void
soundmenu_construct (XfcePanelPlugin *plugin)
{
	SoundmenuPlugin *soundmenu;

	/* setup transation domain */
	xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

	/* create the plugin */
	soundmenu = soundmenu_new (plugin);

	/* add the hvbox to the panel */
	gtk_container_add (GTK_CONTAINER (plugin), soundmenu->hvbox);

	/* connect plugin signals */
	g_signal_connect (G_OBJECT (plugin), "free-data",
				G_CALLBACK (soundmenu_free), soundmenu);

	g_signal_connect (G_OBJECT (plugin), "save",
				G_CALLBACK (soundmenu_save), soundmenu);

	g_signal_connect (G_OBJECT (plugin), "size-changed",
				G_CALLBACK (soundmenu_size_changed), soundmenu);

	g_signal_connect (G_OBJECT (plugin), "orientation-changed",
				G_CALLBACK (soundmenu_orientation_changed), soundmenu);

	#ifdef HAVE_LIBKEYBINDER
	init_keybinder(soundmenu);
	#endif

	/* show the configure menu item and connect signal */
	xfce_panel_plugin_menu_show_configure (plugin);

	g_signal_connect (G_OBJECT (plugin), "configure-plugin",
				G_CALLBACK (soundmenu_configure), soundmenu);

	/* show the about menu item and connect signal */
	xfce_panel_plugin_menu_show_about (plugin);

	g_signal_connect (G_OBJECT (plugin), "about",
				G_CALLBACK (soundmenu_about), NULL);
}
