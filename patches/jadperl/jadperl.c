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




/* our master callback */
int jp_callback(sx_ctx_t ctx, sx_conn_t c, sx_event_t event, void *data, void *arg)
{
    jp_t jp = (jp_t) arg;
    pkt_t pkt;
    int ns, attr;
    jid_t target;
    char id[41];
    mod_ret_t ret;

    switch(event)
    {
        case event_ERROR:
            log_write(jp->log, LOG_NOTICE, "error from router: %s\n", sx_strerror((int) data));
            jp_shutdown = 1;
            break;

        case event_STREAM:
            sx_auth_handshake(c, jp->router_secret);
            break;

        case event_PACKET:

            log_debug(ZONE, "got a packet");

            pkt = pkt_new(jp, (nad_t) data, 1);

            log_debug(ZONE, "created a packet");

            if(pkt == NULL)
            {
                log_debug(ZONE, "invalid packet, bounce/drop");

                if(nad_find_attr((nad_t) data, 1, -1, "type", "error") >= 0)
                    nad_free((nad_t) data);
                else
                    sx_nad_write(c, sx_nad_tofrom(sx_nad_error((nad_t) data, "400", "Bad Request")), 1);

                return 0;
            }

            /* no to means it came from the router */
            if(pkt->to == NULL)
            {
                log_debug(ZONE, "packet from router, will do something about these later");

		// XXX I think this is a packet aimed straight at me

                /* we don't bounce these, ever */
// XXX                ret = mm_pkt_router(pkt->sm->mm, pkt);
                if(ret != mod_HANDLED)
                    pkt_free(pkt);

                return 0;
            }

            /* sanity */
            if(strcmp(pkt->to->domain, jp->id) != 0)
            {
                log_debug(ZONE, "eek! router gave us a packet destined for %s, dropping it", jid_full(pkt->to));
                pkt_free(pkt);
                return 0;
            }

            /* packet is for the jp itself */
            if(*pkt->to->node == '\0')
            {
                log_debug(ZONE, "whats this? must be addressed to me specifically - I will handle");
// XXX                ret = mm_pkt_sm(pkt->sm->mm, pkt);
                ret = mod_perl_onpacket(pkt);
                switch(ret)
                {
                    case mod_HANDLED:
                        break;
        
                    case mod_PASS:
                        ret = -501;
        
                    default:
                        ret = -501;
                        pkt_router(pkt_error(pkt, -ret, NULL));
            
                        break;
                }

                return 0;
            }


            /* its a normal packet, so dispatch it */
// XXX            ret = mm_in_router(pkt->sm->mm, pkt);
            log_debug(ZONE, "Normal packet - no frills - just handle it");
            ret = mod_perl_onpacket(pkt);
            switch(ret)
            {
                case mod_HANDLED:
                    return 0;

                case mod_PASS:
                    break;
        
                default:
                    log_debug(ZONE, "packet nothandled at all - set default");
		    ret = -501;
                    pkt_router(pkt_error(pkt, -ret, NULL));
                    return 0;
            }

            /* we never get here */
            return 0;

            break;
        
        case event_CLOSED:
            log_write(jp->log, LOG_NOTICE, "connection to router closed");
            jp_shutdown = 1;
            break;

        case event_OPEN:
            log_write(jp->log, LOG_NOTICE, "connection to router established");
            break;

        case event_ACCEPT:
        case event_RATE:
            break;
    }

    return 0;
}

/* this was jutil_timestamp in a former life */
void jp_timestamp(time_t t, char timestamp[18])
{
    struct tm gm;

    gmtime_r(&t, &gm);

    snprintf(timestamp, 18, "%d%02d%02dT%02d:%02d:%02d", 1900 + gm.tm_year,
             gm.tm_mon + 1, gm.tm_mday, gm.tm_hour, gm.tm_min, gm.tm_sec);

    return;
}

