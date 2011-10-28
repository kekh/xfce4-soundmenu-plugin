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

#include "soundmenu-plugin.h"

Metadata *malloc_metadata();
void free_metadata(Metadata *m);

static void get_meta_item_array(DBusMessageIter *dict_entry, char **item);
static void get_meta_item_str(DBusMessageIter *dict_entry, char **item);
static void get_meta_item_gint(DBusMessageIter *dict_entry, void *item);

void mpris2_demarshal_metadata (DBusMessageIter *args, SoundmenuPlugin *soundmenu);

DBusHandlerResult mpris2_dbus_filter (DBusConnection *connection, DBusMessage *message, void *user_data);
void mpris2_send_message (SoundmenuPlugin *soundmenu, const char *msg);

void mpris2_get_playbackstatus (SoundmenuPlugin *soundmenu);
void mpris2_get_metadata (SoundmenuPlugin *soundmenu);
void mpris2_get_volume (SoundmenuPlugin *soundmenu);
void mpris2_get_player_status (SoundmenuPlugin *soundmenu);