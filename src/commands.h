/*
 * Copyright (C) 2010, 2011 Thomas Jost
 *
 * This file is part of spop.
 *
 * spop is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * spop is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * spop. If not, see <http://www.gnu.org/licenses/>.
 *
 * Additional permission under GNU GPL version 3 section 7
 *
 * If you modify this Program, or any covered work, by linking or combining it
 * with libspotify (or a modified version of that library), containing parts
 * covered by the terms of the Libspotify Terms of Use, the licensors of this
 * Program grant you additional permission to convey the resulting work.
 */

#ifndef COMMANDS_H
#define COMMANDS_H

#include <glib.h>
#include <json-glib/json-glib.h>
#include <libspotify/api.h>

/* Commands management */
#define MAX_CMD_ARGS 2
typedef enum { CA_NONE=0, CA_INT, CA_STR, CA_URI } command_arg;
typedef struct {
    void*       func;
    command_arg args[MAX_CMD_ARGS];
} command_descriptor;
typedef struct {
    GIOChannel* chan;
    JsonBuilder* jb;
} command_context;

gboolean command_run(GIOChannel* chan, command_descriptor* desc, int argc, char** argv);
void command_end(command_context* ctx);

/* Actual commands */
gboolean list_playlists(command_context* ctx);
gboolean list_tracks(command_context* ctx, guint idx);

gboolean status(command_context* ctx);
gboolean repeat(command_context* ctx);
gboolean shuffle(command_context* ctx);

gboolean list_queue(command_context* ctx);
gboolean clear_queue(command_context* ctx);
gboolean remove_queue_items(command_context* ctx, guint first, guint last);
gboolean remove_queue_item(command_context* ctx, guint idx);

gboolean play_playlist(command_context* ctx, guint idx);
gboolean play_track(command_context* ctx, guint pl_idx, guint tr_idx);

gboolean add_playlist(command_context* ctx, guint idx);
gboolean add_track(command_context* ctx, guint pl_idx, guint tr_idx);

gboolean play(command_context* ctx);
gboolean toggle(command_context* ctx);
gboolean stop(command_context* ctx);
gboolean seek(command_context* ctx, guint pos);

gboolean goto_next(command_context* ctx);
gboolean goto_prev(command_context* ctx);
gboolean goto_nb(command_context* ctx, guint nb);

gboolean image(command_context* ctx);

gboolean uri_info(command_context* ctx, sp_link* lnk);
gboolean uri_add(command_context* ctx, sp_link* lnk);
gboolean uri_play(command_context* ctx, sp_link* lnk);

#endif
