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

#include <string.h>
#include <gtk/gtk.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#include "pragha.h"
#include "pragha-dialogs.h"

/* the website url */
#define PLUGIN_WEBSITE "http://pragha.wikispaces.com/"



static void
pragha_configure_response (GtkWidget    *dialog,
                           gint          response,
                           PraghaPlugin *pragha)
{
  gboolean result;

  if (response == GTK_RESPONSE_HELP)
    {
      /* show help */
      result = g_spawn_command_line_async ("exo-open --launch WebBrowser " PLUGIN_WEBSITE, NULL);

      if (G_UNLIKELY (result == FALSE))
        g_warning (_("Unable to open the following url: %s"), PLUGIN_WEBSITE);
    }
  else
    {
      /* remove the dialog data from the plugin */
      g_object_set_data (G_OBJECT (pragha->plugin), "dialog", NULL);

      /* unlock the panel menu */
      xfce_panel_plugin_unblock_menu (pragha->plugin);

      /* save the plugin */
      pragha_save (pragha->plugin, pragha);

      /* destroy the properties dialog */
      gtk_widget_destroy (dialog);
    }
}

static void
toggle_show_stop(GtkToggleButton *button, PraghaPlugin    *pragha)
{
	pragha->show_stop = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

	if(pragha->show_stop)
		gtk_widget_show(pragha->stop_button);
	else
		gtk_widget_hide(pragha->stop_button);
}

void
pragha_configure (XfcePanelPlugin *plugin,
                  PraghaPlugin    *pragha)
{
  GtkWidget *dialog;
  GtkWidget *show_stop_check;

  /* block the plugin menu */
  xfce_panel_plugin_block_menu (plugin);

  /* create the dialog */
  dialog = xfce_titled_dialog_new_with_buttons (_("Pragha Plugin"),
                                                GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                                                GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
                                                GTK_STOCK_HELP, GTK_RESPONSE_HELP,
                                                GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                                                NULL);
	/* center dialog on the screen */
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

  /* set dialog icon */
  gtk_window_set_icon_name (GTK_WINDOW (dialog), "xfce4-settings");

  /* link the dialog to the plugin, so we can destroy it when the plugin
   * is closed, but the dialog is still open */
  g_object_set_data (G_OBJECT (plugin), "dialog", dialog);

  /* connect the reponse signal to the dialog */
  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK(pragha_configure_response), pragha);

  show_stop_check = gtk_check_button_new_with_label(_("Show stop button"));

	if(pragha->show_stop)
  	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_stop_check), TRUE);

	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), show_stop_check);
 
	g_signal_connect(G_OBJECT(show_stop_check), "toggled",
									G_CALLBACK(toggle_show_stop), pragha);

  /* show the entire dialog */
	gtk_widget_show_all(dialog);
}



void
pragha_about (XfcePanelPlugin *plugin)
{
	gboolean result;

	result = g_spawn_command_line_async ("exo-open --launch WebBrowser " PLUGIN_WEBSITE, NULL);

	if (G_UNLIKELY (result == FALSE))
		g_warning (_("Unable to open the following url: %s"), PLUGIN_WEBSITE);
}