/*
 * jabberd - Jabber Open Source Server
 * Copyright (c) 2002 Jeremie Miller, Thomas Muldowney,
 *                    Ryan Eatmon, Robert Norris
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA02111-1307USA
 */

#include "jadperl.h"

int jp_shutdown = 0;
static int jp_restart = 1;
static int jp_logrotate = 0;

static void _jp_signal(int signum)
{
    jp_shutdown = 1;
    jp_restart = 0;
}

static void _jp_signal_hup(int signum)
{
    jp_logrotate = 1;
}

/* pull values out of the config file */
static void _jp_config_expand(jp_t jp)
{
    char *str;

    jp->id = config_get_one(jp->config, "id", 0);
    if(jp->id == NULL)
        jp->id = "localhost";

    jp->router_ip = config_get_one(jp->config, "router.ip", 0);
    if(jp->router_ip == NULL)
        jp->router_ip = "127.0.0.1";

    jp->router_port = j_atoi(config_get_one(jp->config, "router.port", 0), 5269);

    jp->router_secret = config_get_one(jp->config, "router.secret", 0);
    if(jp->router_secret == NULL)
        jp->router_secret = "secret";

    jp->log_syslog = 0;
    if(config_get(jp->config, "log") != NULL)
    {
        str = config_get_attr(jp->config, "log", 0, "type");
        if(str != NULL && strcmp(str, "syslog") == 0)
            jp->log_syslog = 1;
    }

    if(jp->log_syslog)
        jp->log_ident = config_get_one(jp->config, "log.ident", 0);
    else
        jp->log_ident = config_get_one(jp->config, "log.file", 0);

    if(jp->log_ident == NULL)
    {
        jp->log_syslog = 1;
        jp->log_ident = "jabberd/jp";
    }
}

int main(int argc, char **argv)
{
    jp_t jp;
    char optchar, *config_file;
    int err, i;
    mio_t mio;
    sx_ctx_t ctx;

    signal(SIGINT, _jp_signal);
    signal(SIGTERM, _jp_signal);
    signal(SIGHUP, _jp_signal_hup);
    signal(SIGPIPE, SIG_IGN);

    jp = (jp_t) malloc(sizeof(struct jp_st));
    memset(jp, 0, sizeof(struct jp_st));

    /* load our config */
    jp->config = config_new();

    config_file = CONFIG_DIR "/jp.xml";

    /* cmdline parsing */
    while((optchar = getopt(argc, argv, "Dc:h?")) >= 0)
    {
        switch(optchar)
        {
            case 'c':
                config_file = optarg;
                break;
            case 'D':
#ifdef DEBUG
                set_debug_flag(1);
#else
                printf("WARN: Debugging not enabled.  Ignoring -D.\n");
#endif
                break;
            case 'h': case '?': default:
                fputs(
                    "jadperl - jabberd session manager (" VERSION ")\n"
                    "Usage: jadperl <options>\n"
                    "Options are:\n"
                    "   -c <config>     config file to use [default: " CONFIG_DIR "/jadperl.xml]\n"
#ifdef DEBUG
                    "   -D              Show debug output\n"
#endif
                    ,
                    stdout);
                config_free(jp->config);
                free(jp);
                return 1;
        }
    }

    // must startup the perl interpreter here
    mod_perl_init_interp();

    if(config_load(jp->config, config_file) != 0)
    {
        fputs("jadperl: couldn't load config, aborting\n", stderr);
        config_free(jp->config);
        free(jp);
        return 2;
    }

    _jp_config_expand(jp);

    jp->log = log_new(jp->log_syslog, jp->log_ident);
    log_write(jp->log, LOG_NOTICE, "starting up");

    /* pre-index known namespaces */
    jp->xmlns = xhash_new(101);
    xhash_put(jp->xmlns, "jabber:iq:auth", (void *) ns_AUTH);
    xhash_put(jp->xmlns, "jabber:iq:register", (void *) ns_REGISTER);
    xhash_put(jp->xmlns, "jabber:iq:roster", (void *) ns_ROSTER);
    xhash_put(jp->xmlns, "jabber:iq:agents", (void *) ns_AGENTS);
    xhash_put(jp->xmlns, "jabber:x:delay", (void *) ns_DELAY);
    xhash_put(jp->xmlns, "jabber:iq:version", (void *) ns_VERSION);
    xhash_put(jp->xmlns, "jabber:iq:time", (void *) ns_TIME);
    xhash_put(jp->xmlns, "vcard-temp", (void *) ns_VCARD);
    xhash_put(jp->xmlns, "jabber:iq:private", (void *) ns_PRIVATE);
    xhash_put(jp->xmlns, "jabber:iq:browse", (void *) ns_BROWSE);
    xhash_put(jp->xmlns, "jabber:x:event", (void *) ns_EVENT);
    xhash_put(jp->xmlns, "jabber:iq:gateway", (void *) ns_GATEWAY);
    xhash_put(jp->xmlns, "jabber:iq:last", (void *) ns_LAST);
    xhash_put(jp->xmlns, "jabber:x:expire", (void *) ns_EXPIRE);
    xhash_put(jp->xmlns, "jabber:iq:privacy", (void *) ns_PRIVACY);
    xhash_put(jp->xmlns, "http://jabber.org/protocol/disco#items", (void *) ns_DISCO_ITEMS);
    xhash_put(jp->xmlns, "http://jabber.org/protocol/disco#info", (void *) ns_DISCO_INFO);

    /* supported features */
    jp->features = xhash_new(101);

    mio = mio_new(1023);

    ctx = sx_new(mio);

    while(jp_restart)
    {
        jp_shutdown = 0;
        log_write(jp->log, LOG_NOTICE, "attempting connection to router at %s:%d", jp->router_ip, jp->router_port);

        jp->router = sx_client_new(ctx, NULL, jp->id, SX_TYPE_COMPONENT | SX_AUTH_HANDSHAKE, jp_callback, jp);
        if((err = sx_client_connect(jp->router, jp->router_ip, jp->router_port)) != 0)
        {
            log_debug(ZONE, "sx_client_connect failed");
            exit(1);
        }

        // XXX  - and do the mod_perl initialisations
        log_debug(ZONE, "going to init mod_perl");
        mod_perl_initialise(jp->config->nad);

        while(!jp_shutdown)
        {
            mio_run(mio, 5);

            if(jp_logrotate)
            {
                log_write(jp->log, LOG_NOTICE, "reopening log ...");
                log_free(jp->log);
                jp->log = log_new(jp->log_syslog, jp->log_ident);
                log_write(jp->log, LOG_NOTICE, "log started");

                jp_logrotate = 0;
            }
        }

        if (jp_restart)
        {
            log_write(jp->log, LOG_NOTICE, "Connection to router lost.  Attempting a reconnect.");
            sleep(2);
        }
    }

    log_write(jp->log, LOG_NOTICE, "shutting down");


    mod_perl_destroy();

    sx_free(ctx);

    xhash_free(jp->features);
    xhash_free(jp->xmlns);

    log_free(jp->log);

    config_free(jp->config);

    free(jp);

    return 0;
}
