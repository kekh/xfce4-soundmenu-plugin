/*
 *  Copyright (c) 2011-2013 matias <mati86dl@gmail.com>
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
 *  Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef __SOUNDMENU_H__
#define __SOUNDMENU_H__

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-hvbox.h>

#include "soundmenu-album-art.h"
#include "soundmenu-metadata.h"
//#include "soundmenu-simple-async.h"

#ifdef HAVE_LIBCLASTFM
#include <clastfm.h>
#endif

#ifdef HAVE_LIBGLYR
#include <glyr/glyr.h>
#endif

#ifdef HAVE_LIBNOTIFY
#include <libnotify/notify.h>
#endif

G_BEGIN_DECLS

#ifndef NOTIFY_CHECK_VERSION
#define NOTIFY_CHECK_VERSION(x,y,z) 0
#endif

/*
 * TODO: Move them to soundmenu-simple-async.h
 * If try it, resulting in cross reference..
 */

#if GLIB_CHECK_VERSION (2, 32, 0)
#define SOUNDMENU_MUTEX(mtx) GMutex mtx
#define soundmenu_mutex_free(mtx) g_mutex_clear (&(mtx))
#define soundmenu_mutex_lock(mtx) g_mutex_lock (&(mtx))
#define soundmenu_mutex_unlock(mtx) g_mutex_unlock (&(mtx))
#define soundmenu_mutex_create(mtx) g_mutex_init (&(mtx))
#else
#define SOUNDMENU_MUTEX(mtx) GMutex *mtx
#define soundmenu_mutex_free(mtx) g_mutex_free (mtx)
#define soundmenu_mutex_lock(mtx) g_mutex_lock (mtx)
#define soundmenu_mutex_unlock(mtx) g_mutex_unlock (mtx)
#define soundmenu_mutex_create(mtx) (mtx) = g_mutex_new ()
#endif

#define LASTFM_API_KEY             "70c479ab2632e597fd9215cf35963c1b"
#define LASTFM_SECRET              "4cb5255d955edc8f651de339fd2f335b"

#ifdef HAVE_LIBCLASTFM
struct lastfm_pref {
	GtkWidget *lastfm_w;
	GtkWidget *lastfm_uname_w;
	GtkWidget *lastfm_pass_w;
};

struct con_lastfm {
	gboolean lastfm_support;
	gchar *lastfm_user;
	gchar *lastfm_pass;
	LASTFM_SESSION *session_id;
	enum LASTFM_STATUS_CODES status;
	gint lastfm_handler_id;
	time_t playback_started;
	GtkWidget *lastfm_love_item;
	GtkWidget *lastfm_unlove_item;
};
#endif

typedef enum {
	ST_PLAYING = 1,
	ST_STOPPED,
	ST_PAUSED
} PlaybackStatus;

typedef enum {
	LOOP_PLAYLIST = 1,
	LOOP_NONE
} PlaybackLoop;

/* plugin structure */
typedef struct
{
	XfcePanelPlugin	*plugin;

	/* panel widgets */
	GtkWidget		*hvbox;
	GtkWidget		*hvbox_buttons;
	SoundmenuAlbumArt *album_art;
	GtkWidget		*ev_album_art;
	GtkWidget		*prev_button;
	GtkWidget		*play_button;
	GtkWidget		*stop_button;
	GtkWidget		*next_button;
	GtkWidget		*image_pause;
	GtkWidget		*image_play;
	GtkWidget       *loop_menu_item;
	GtkWidget       *shuffle_menu_item;
	GtkWidget       *tools_submenu;

	/* Helper to obtain player name of preferences */
	GtkWidget	*w_player;
	#ifdef HAVE_LIBCLASTFM
	struct lastfm_pref lw;
	struct con_lastfm *clastfm;
	#endif

	/* Player states */
	PlaybackStatus       state;
	SoundmenuMetadata   *metadata;
	SOUNDMENU_MUTEX     (metadata_mtx);
	PlaybackLoop         loops_status;
	gboolean             shuffle;
	gdouble              volume;

	/* Dbus conecction */
	GDBusConnection *gconnection;
	GDBusProxy      *proxy;
	gchar			*dbus_name;
	guint            watch_id;
	gboolean         connected;

	/* soundmenu settings */
	gchar			*player;
	gboolean		show_album_art;
	gboolean		huge_on_deskbar_mode;
	gboolean		show_stop;
	gboolean		hide_controls_if_loose;
	gboolean		use_global_keys;
}
SoundmenuPlugin;

void update_panel_album_art(SoundmenuPlugin *soundmenu);

void
soundmenu_update_layout_changes (SoundmenuPlugin    *soundmenu);

void
soundmenu_update_state(const gchar *state, SoundmenuPlugin *soundmenu);

void soundmenu_update_loop_status (SoundmenuPlugin *soundmenu, const gchar *loop_status);
void soundmenu_update_shuffle (SoundmenuPlugin *soundmenu, gboolean shuffle);

void
soundmenu_save (XfcePanelPlugin *plugin,
             SoundmenuPlugin    *soundmenu);

G_END_DECLS

#endif /* !__SOUNDMENU_H__ */
