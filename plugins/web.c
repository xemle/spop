/*
 * Copyright (C) 2013 Thomas Jost
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

#include <glib.h>
#include <gmodule.h>
#include <libsoup/soup.h>
#include <string.h>

#include "spop.h"
#include "commands.h"
#include "config.h"
#include "spotify.h"

#define WEB_DEFAULT_IP   "127.0.0.1"
#define WEB_DEFAULT_PORT 8080

/* Command finalizer (for async commands) */
typedef struct {
    SoupServer* server;
    SoupMessage* msg;
} web_command_context;
static void web_command_finalize(gchar* json_result, web_command_context* ctx) {
    /* Send the JSON response to the client */
    soup_message_set_status(ctx->msg, SOUP_STATUS_OK);
    soup_message_set_response(ctx->msg, "application/json", SOUP_MEMORY_COPY,
                              json_result, strlen(json_result));
    soup_server_unpause_message(ctx->server, ctx->msg);
    g_free(ctx);
}

/* Requests handler */
static void web_api_handler(SoupServer* server, SoupMessage* msg,
                            const char* path, GHashTable* query,
                            SoupClientContext* client, gpointer user_data) {
    size_t i;

    if (msg->method != SOUP_METHOD_GET) {
        soup_message_set_status(msg, SOUP_STATUS_NOT_IMPLEMENTED);
        return;
    }

    /* Get infos about the client and log the request */
    g_debug("web: %s GET %s", soup_client_context_get_host(client), path);

    /* Parse the path */
    const gchar* subpath = path+5; /* Strip "/api/" */
    gchar** cmd = g_strsplit(subpath, "/", -1);
    guint cmd_len = g_strv_length(cmd);
    if (cmd_len == 0) {
        /* No command */
        g_strfreev(cmd);
        soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
        return;
    }

    /* Decode each part of the path */
    for (i=0; i < cmd_len; i++) {
        gchar* decoded = soup_uri_decode(cmd[i]);
        g_free(cmd[i]);
        cmd[i] = decoded;
    }

    /* Lookup the command */
    command_full_descriptor* cmd_desc = NULL;
    for (i=0; commands_descriptors[i].name != NULL; i++) {
        int nb_args = 0;
        while ((nb_args < MAX_CMD_ARGS) && (commands_descriptors[i].desc.args[nb_args] != CA_NONE))
            nb_args += 1;
        if ((strcmp(commands_descriptors[i].name, cmd[0]) == 0) && (nb_args == cmd_len-1)) {
            cmd_desc = &(commands_descriptors[i]);
            break;
        }
    }
    if (!cmd_desc) {
        /* Unknown command */
        g_strfreev(cmd);
        soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
        return;
    }

    g_debug("web: found command %s with %d parameter(s)", cmd_desc->name, cmd_len-1);

    /* Run the command if possible */
    web_command_context* ctx = NULL;
    switch(cmd_desc->type) {
    case CT_FUNC:
        /* Run the command asynchronously */
        ctx = g_new0(web_command_context, 1);
        ctx->server = server;
        ctx->msg = msg;
        soup_server_pause_message(server, msg);
        command_run((command_finalize_func) web_command_finalize, ctx,
                    &(cmd_desc->desc), cmd_len, cmd);
        break;

    case CT_IDLE:
        break;

    default:
        soup_message_set_status(msg, SOUP_STATUS_FORBIDDEN);
    }
    g_strfreev(cmd);
}

/* Plugin initialization */
G_MODULE_EXPORT void spop_web_init() {
    /* Get IP/port to listen on */
    gchar* default_ip = g_strdup(WEB_DEFAULT_IP);
    gchar* web_ip = config_get_string_opt_group("web", "ip", default_ip);
    guint web_port = config_get_int_opt_group("web", "port", WEB_DEFAULT_PORT);

    SoupAddress* addr = soup_address_new(web_ip, web_port);
    g_free(default_ip);
    if (soup_address_resolve_sync(addr, NULL) != SOUP_STATUS_OK)
        g_error("Could not resolve hostname for the web server");

    /* Init the server and add URL handlers */
    SoupServer* server = soup_server_new("interface", addr,
                                         "port", web_port,
                                         "server-header", "spop/" SPOP_VERSION " ",
                                         "raw-paths", TRUE,
                                         NULL);
    if (!server)
        g_error("Could not initialize web server");

    soup_server_add_handler(server, "/api", web_api_handler, NULL, NULL);

    /* Start the server */
    soup_server_run_async(server);
    g_info("web: Listening on %s:%d", soup_address_get_physical(addr), web_port);
}

G_MODULE_EXPORT void spop_web_close() {
}
