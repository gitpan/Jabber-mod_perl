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

pkt_t pkt_error(pkt_t pkt, int code, char *msg)
{
    char buf[32];
    int e;

    if(pkt == NULL) return NULL;

    /* if it's an error already, log, free, return */
    if(pkt->type & pkt_ERROR)
    {
        log_debug(ZONE, "dropping error pkt");
        pkt_free(pkt);
        return NULL;
    }

    /* default message text if none specified */
    /* !!! this is ugly, but I can't think of a way to do it efficiently */
    /* !!! these are only the ones defined in the IETF spec .. specifically,
     *     409 (Conflict) and 510 (Disconnected) are not here, even though
     *     they appear in 142/jabberd/lib/lib.h */
    if(msg == NULL)
        switch(code)
        {
            case 302: msg = "Redirect";                 break;

            case 400: msg = "Bad Request";              break;
            case 401: msg = "Unauthorized";             break;
            case 402: msg = "Payment Required";         break;
            case 403: msg = "Forbidden";                break;
            case 404: msg = "Not Found";                break;
            case 405: msg = "Not Allowed";              break;
            case 406: msg = "Not Acceptable";           break;
            case 407: msg = "Registration Required";    break;
            case 408: msg = "Request Timeout";          break;

            case 500: msg = "Internal Server Error";    break;
            case 501: msg = "Not Implemented";          break;
            case 502: msg = "Remote Server Error";      break;
            case 503: msg = "Service Unvailable";       break;
            case 504: msg = "Remote Server Timeout";    break;

            default:  msg = "Unknown Error";            break;
        }

    /* update vars and attrs */
    pkt_tofrom(pkt);
    pkt->type |= pkt_ERROR;
    nad_set_attr(pkt->nad, pkt->elem, -1, "type", "error", 5);

    /* insert new error */
    e = nad_insert_elem(pkt->nad, pkt->elem, -1, "error", msg);
    sprintf(buf, "%d", code);
    nad_set_attr(pkt->nad, e, -1, "code", buf, 0);

    /* all done, error'd and addressed */
    log_debug(ZONE, "processed %d error pkt", code);

    return pkt;
}

/* swap a packet's to and from attributes */
pkt_t pkt_tofrom(pkt_t pkt)
{
    jid_t tmp;

    if(pkt == NULL) return NULL;

    /* swap vars */
    tmp = pkt->from;
    pkt->from = pkt->to;
    pkt->to = tmp;

    /* update attrs */
    if(pkt->to != NULL)
        nad_set_attr(pkt->nad, pkt->elem, -1, "to", jid_full(pkt->to), 0);
    if(pkt->from != NULL)
        nad_set_attr(pkt->nad, pkt->elem, -1, "from", jid_full(pkt->from), 0);

    return pkt;
}

/* duplicate pkt, replacing addresses */
pkt_t pkt_dup(pkt_t pkt, char *to, char *from)
{
    pkt_t pnew;

    if(pkt == NULL) return NULL;

    pnew = (pkt_t) malloc(sizeof(struct pkt_st));
    memset(pnew, 0, sizeof(struct pkt_st));

    pnew->jp = pkt->jp;
    pnew->type = pkt->type;
    pnew->nad = nad_copy(pkt->nad);
    pnew->elem = pkt->elem;

    /* set replacement attrs */
    if(to != NULL)
    {
        pnew->to = jid_new(to, 0);
        nad_set_attr(pnew->nad, pnew->elem, -1, "to", jid_full(pnew->to), 0);
    } else if(pkt->to != NULL) {
        pnew->to = jid_dup(pkt->to);
    }
    if(from != NULL)
    {
        pnew->from = jid_new(from, 0);
        nad_set_attr(pnew->nad, pnew->elem, -1, "from", jid_full(pnew->from), 0);
    } else if(pkt->from != NULL) {
        pnew->from = jid_dup(pkt->from);
    }

    log_debug(ZONE, "duplicated packet");

    return pnew;
}

pkt_t pkt_reset(pkt_t pkt, int elem)
{
    int attr, ns;

    log_debug(ZONE, "resetting packet");

    pkt->elem = elem;

    if(pkt->to != NULL)
    {
        jid_free(pkt->to);
        pkt->to = NULL;
    }
    if(pkt->from != NULL)
    {
        jid_free(pkt->from);
        pkt->from = NULL;
    }
        
    /* get initial addresses */
    if((attr = nad_find_attr(pkt->nad, elem, -1, "to", NULL)) >= 0)
        pkt->to = jid_new(NAD_AVAL(pkt->nad, attr), NAD_AVAL_L(pkt->nad, attr));
    log_debug(ZONE, "find on to got %d", attr);
    if((attr = nad_find_attr(pkt->nad, elem, -1, "from", NULL)) >= 0)
        pkt->from = jid_new(NAD_AVAL(pkt->nad, attr), NAD_AVAL_L(pkt->nad, attr));
    log_debug(ZONE, "find on from got %d", attr);

    /* find type, if any */
    attr = nad_find_attr(pkt->nad, elem, -1, "type", NULL);

    /* messages are simple */
    if(NAD_ENAME_L(pkt->nad, elem) == 7 && strncmp("message", NAD_ENAME(pkt->nad, elem), NAD_ENAME_L(pkt->nad, elem)) == 0)
    {
        pkt->type = pkt_MESSAGE;
        if(attr >= 0 && NAD_AVAL_L(pkt->nad, attr) == 5 && strncmp("error", NAD_AVAL(pkt->nad, attr), NAD_AVAL_L(pkt->nad, attr)) == 0)
            pkt->type |= pkt_ERROR;
        return pkt;
    }

    /* presence is a mixed bag, s10ns in here too */
    if(NAD_ENAME_L(pkt->nad, elem) == 8 && strncmp("presence", NAD_ENAME(pkt->nad,elem), NAD_ENAME_L(pkt->nad, elem)) == 0)
    {
        if(NAD_AVAL_L(pkt->nad, attr) == 11 && strncmp("unavailable", NAD_AVAL(pkt->nad, attr), NAD_AVAL_L(pkt->nad, attr)) == 0)
            pkt->type = pkt_PRESENCE_UN;
        else if(NAD_AVAL_L(pkt->nad, attr) == 5 && strncmp("probe", NAD_AVAL(pkt->nad, attr), NAD_AVAL_L(pkt->nad, attr)) == 0)
            pkt->type = pkt_PRESENCE_PROBE;
        else if(NAD_AVAL_L(pkt->nad, attr) == 9 && strncmp("invisible", NAD_AVAL(pkt->nad, attr), NAD_AVAL_L(pkt->nad, attr)) == 0)
            pkt->type = pkt_PRESENCE_INVIS;
        else if(NAD_AVAL_L(pkt->nad, attr) == 9 && strncmp("subscribe", NAD_AVAL(pkt->nad, attr), NAD_AVAL_L(pkt->nad, attr)) == 0)
            pkt->type = pkt_S10N;
        else if(NAD_AVAL_L(pkt->nad, attr) == 10 && strncmp("subscribed", NAD_AVAL(pkt->nad, attr), NAD_AVAL_L(pkt->nad, attr)) == 0)
            pkt->type = pkt_S10N_ED;
        else if(NAD_AVAL_L(pkt->nad, attr) == 11 && strncmp("unsubscribe", NAD_AVAL(pkt->nad, attr), NAD_AVAL_L(pkt->nad, attr)) == 0)
            pkt->type = pkt_S10N_UN;
        else if(NAD_AVAL_L(pkt->nad, attr) == 12 && strncmp("unsubscribed", NAD_AVAL(pkt->nad, attr), NAD_AVAL_L(pkt->nad, attr)) == 0)
            pkt->type = pkt_S10N_UNED;
        else if(NAD_AVAL_L(pkt->nad, attr) == 5 && strncmp("error", NAD_AVAL(pkt->nad, attr), NAD_AVAL_L(pkt->nad, attr)) == 0)
            pkt->type = pkt_PRESENCE | pkt_ERROR;
        else
            pkt->type = pkt_PRESENCE;

        return pkt;
    }

    /* iq's are pretty easy, but also set xmlns */
    if(NAD_ENAME_L(pkt->nad, elem) == 2 && strncmp("iq", NAD_ENAME(pkt->nad, elem), NAD_ENAME_L(pkt->nad, elem)) == 0)
    {
        if(NAD_AVAL_L(pkt->nad, attr) == 3 && strncmp("set", NAD_AVAL(pkt->nad, attr), NAD_AVAL_L(pkt->nad, attr)) == 0)
            pkt->type = pkt_IQ_SET;
        else if(NAD_AVAL_L(pkt->nad, attr) == 6 && strncmp("result", NAD_AVAL(pkt->nad, attr), NAD_AVAL_L(pkt->nad, attr)) == 0)
            pkt->type = pkt_IQ_RESULT;
        else if(NAD_AVAL_L(pkt->nad, attr) == 5 && strncmp("error", NAD_AVAL(pkt->nad, attr), NAD_AVAL_L(pkt->nad, attr)) == 0)
            pkt->type = pkt_IQ | pkt_ERROR;
        else
            pkt->type = pkt_IQ;

        if(elem + 1 < pkt->nad->ecur && (ns = NAD_ENS(pkt->nad, elem + 1)) >= 0)
            pkt->ns = (xmlns_t) xhash_getx(pkt->jp->xmlns, NAD_NURI(pkt->nad, ns), NAD_NURI_L(pkt->nad, ns));

        return pkt;
    }

    /* routes */
    if(NAD_ENAME_L(pkt->nad, elem) == 5 && strncmp("route", NAD_ENAME(pkt->nad, elem), NAD_ENAME_L(pkt->nad, elem)) == 0)
    {
        ns = nad_find_scoped_namespace(pkt->nad, "http://jabberd.jabberstudio.org/ns/session/1.0", NULL);

        if(NAD_AVAL_L(pkt->nad, attr) == 5 && strncmp("error", NAD_AVAL(pkt->nad, attr), NAD_AVAL_L(pkt->nad, attr)) == 0)
            pkt->type = pkt_ROUTE | pkt_ERROR;

        else if(ns >= 0)
        {
            if(nad_find_attr(pkt->nad, pkt->elem, ns, "action", "start") >= 0)
                pkt->type = pkt_SESS;
            else if(nad_find_attr(pkt->nad, pkt->elem, ns, "action", "end") >= 0)
                pkt->type = pkt_SESS_END;
            else if(nad_find_attr(pkt->nad, pkt->elem, ns, "action", "create") >= 0)
                pkt->type = pkt_SESS_CREATE;
            else if(nad_find_attr(pkt->nad, pkt->elem, ns, "action", "delete") >= 0)
                pkt->type = pkt_SESS_DELETE;
            else if(nad_find_attr(pkt->nad, pkt->elem, ns, "action", "started") >= 0)
                pkt->type = pkt_SESS | pkt_SESS_FAILED;
            else if(nad_find_attr(pkt->nad, pkt->elem, ns, "action", "ended") >= 0)
                pkt->type = pkt_SESS_END | pkt_SESS_FAILED;
            else if(nad_find_attr(pkt->nad, pkt->elem, ns, "action", "created") >= 0)
                pkt->type = pkt_SESS_CREATE | pkt_SESS_FAILED;
            else if(nad_find_attr(pkt->nad, pkt->elem, ns, "action", "deleted") >= 0)
                pkt->type = pkt_SESS_DELETE | pkt_SESS_FAILED;
        }

        else
            pkt->type = pkt_ROUTE;
    }

    return pkt;
}

pkt_t pkt_new(jp_t jp, nad_t nad, int elem)
{
    pkt_t pkt;

    if(elem == nad->ecur)
    {
        log_debug(ZONE, "no element at %d", elem);
        return NULL;
    }

    log_debug(ZONE, "new pkt from %.*s nad", NAD_ENAME_L(nad, elem), NAD_ENAME(nad, elem));

    if(NAD_ENS(nad, elem) != nad_find_namespace(nad, 0, jp->router->ns, NULL))
    {
        log_debug(ZONE, "nad from %d not in %s ns", elem, jp->router->ns);
        return NULL;
    }

    /* create the pkt holder */
    pkt = (pkt_t) malloc(sizeof(struct pkt_st));
    memset(pkt, 0, sizeof(struct pkt_st));

    pkt->jp = jp;
    pkt->nad = nad;
    
    return pkt_reset(pkt, elem);
}

void pkt_free(pkt_t pkt)
{
    log_debug(ZONE, "freeing pkt");

    if(pkt->to != NULL) jid_free(pkt->to);
    if(pkt->from != NULL) jid_free(pkt->from);
    nad_free(pkt->nad);
    free(pkt);
}

pkt_t pkt_create(jp_t jp, char *elem, char *type, char *to, char *from)
{
    nad_t nad;
    int ns;

    nad = sx_nad_new(jp->router);
    ns = nad_find_namespace(nad, 0, jp->router->ns, NULL);

    nad_append_elem(nad, ns, elem, 1);
    if(type != NULL)
        nad_append_attr(nad, -1, "type", type);
    if(to != NULL)
        nad_append_attr(nad, -1, "to", to);
    if(from != NULL)
        nad_append_attr(nad, -1, "from", from);

    return pkt_new(jp, nad, 1);
}


/* convenience - copy the packet id from src to dest */
void pkt_id(pkt_t src, pkt_t dest)
{
    int attr;

    attr = nad_find_attr(src->nad, src->elem, -1, "id", NULL);
    if(attr >= 0)
        nad_set_attr(dest->nad, dest->elem, -1, "id", NAD_AVAL(src->nad, attr), NAD_AVAL_L(src->nad, attr));
    else
        nad_set_attr(dest->nad, dest->elem, -1, "id", NULL, 0);
}

void pkt_router(pkt_t pkt)
{
// XXX    mod_ret_t ret;

    if(pkt == NULL) return;

    log_debug(ZONE, "delivering pkt to router");

// XXX    ret = mm_out_router(pkt->sm->mm, pkt);
// XXX    switch(ret)
// XXX    {
// XXX        case mod_HANDLED:
// XXX            return;
// XXX
// XXX        case mod_PASS:
            sx_nad_write(pkt->jp->router, pkt->nad, pkt->elem);
    
            jid_free(pkt->to);
            jid_free(pkt->from);
            free(pkt);
// XXX
// XXX            break;
// XXX
// XXX        default:
// XXX            pkt_router(pkt_error(pkt, -ret, NULL));
// XXX
// XXX            break;
// XXX    }
}



/*
void pkt_sess(pkt_t pkt, sess_t sess)
{
    mod_ret_t ret;

    if(pkt == NULL) return;

    log_debug(ZONE, "delivering pkt to session %s", jid_full(sess->jid));

    ret = mm_out_sess(pkt->sm->mm, sess, pkt);
    switch(ret)
    {
        case mod_HANDLED:
            return;

        case mod_PASS:
            sess_route(sess, pkt);

            break;

        default:
            pkt_router(pkt_error(pkt, -ret, NULL));

            break;
    }
}

*/


