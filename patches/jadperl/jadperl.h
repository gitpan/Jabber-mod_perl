/*
 * jabberd - Jabber Open Source Server
 * Copyright (c) 2002 Jeremie Miller, Thomas Muldowney,
 *                    Ryan Eatmon, Robert Norris
 *
 * This program is free software; you can redistribute it and/or drvify
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

#include "sx/sx.h"
#include "util/util.h"

typedef struct jp_st        *jp_t;


/* indexed known namespace values */
typedef enum {
    ns__NONE,
    ns_AUTH, ns_REGISTER, ns_ROSTER, 
    ns_AGENTS, ns_DELAY, ns_VERSION,
    ns_TIME, ns_VCARD, ns_PRIVATE,
    ns_BROWSE, ns_EVENT, ns_GATEWAY, 
    ns_LAST, ns_EXPIRE, ns_PRIVACY,
    ns_DISCO_ITEMS, ns_DISCO_INFO,
    ns__MAX
} xmlns_t;

/* packet typing using nice bitmask groupings */
typedef enum { 
    pkt_NONE = 0x00, 
    pkt_MESSAGE = 0x08, 
    pkt_PRESENCE = 0x10, 
    pkt_PRESENCE_UN = 0x11, 
    pkt_PRESENCE_INVIS = 0x12, 
    pkt_PRESENCE_PROBE = 0x14, 
    pkt_S10N = 0x20, 
    pkt_S10N_ED = 0x21, 
    pkt_S10N_UN = 0x22, 
    pkt_S10N_UNED = 0x24, 
    pkt_IQ = 0x40, 
    pkt_IQ_SET = 0x41, 
    pkt_IQ_RESULT = 0x42,
    pkt_ROUTE = 0x80,
    pkt_SESS = 0x100,
    pkt_SESS_END = 0x101,
    pkt_SESS_CREATE = 0x102,
    pkt_SESS_DELETE = 0x104,
    pkt_SESS_FAILED = 0x08,
    pkt_ERROR = 0x200
} pkt_type_t;

/* return values */
typedef enum { mod_HANDLED, mod_PASS } mod_ret_t;

/* packet summary data wrapper */
typedef struct pkt_st {
    jp_t                jp;
//    sess_t              source;
    pkt_type_t          type;
    jid_t               to, from;
    xmlns_t             ns;
    nad_t               nad;
    int                 elem;
} *pkt_t;


/* jadperl global context */
struct jp_st {
    /* our id (hostname) with the router */
    char                *id;

    /* how to connect to the router */
    char                *router_ip;
    int                 router_port;
    char                *router_secret;

    /* router's conn */
    sx_conn_t           router;

//    /* user data */
//    xht                 users;

    /* pointers to all connected sessions */
//    sess_t              sessions;
//    int                 nsessions;

    /* common namespaces */
    xht                 xmlns;

    /* features */
    xht                 features;

    /* config */
    config_t            config;

    /* logging */
    log_t               log;

    /* log data */
    int                 log_syslog;
    char                *log_ident;

//    /* storage subsystem */
//    storage_t           st;

//    /* module subsystem */
//    mm_t                mm;

//    /* access control lists */
//    xht                 acls;
};



/* turn this on to shut us down */
extern int      jp_shutdown;

int             jp_callback(sx_ctx_t ctx, sx_conn_t c, sx_event_t event, void *data, void *arg);
void            jp_timestamp(time_t t, char timestamp[18]);


pkt_t           pkt_error(pkt_t pkt, int code, char *msg);
pkt_t           pkt_tofrom(pkt_t pkt);
pkt_t           pkt_dup(pkt_t pkt, char *to, char *from);
pkt_t           pkt_reset(pkt_t pkt, int elem);
pkt_t           pkt_new(jp_t jp, nad_t nad, int elem);
void            pkt_free(pkt_t pkt);
pkt_t           pkt_create(jp_t jp, char *elem, char *type, char *to, char *from);
void            pkt_id(pkt_t src, pkt_t dest);

void            pkt_router(pkt_t pkt);
//void            pkt_sess(pkt_t pkt, sess_t sess);


// create my true and false
#ifndef false
typedef enum { false, true } mybool;
#endif

// fake up a definition of bool if it doesnt exist
#ifndef bool
typedef unsigned char    bool;
#endif

// must define these subroutines here to avoid including perl.h into 
//void mod_perl_initialise(nad_t nad, mod_instance_t mi);
int mod_perl_init_interp();
void mod_perl_initialise(nad_t nad);
//void mod_perl_run();
//mod_ret_t mod_perl_onpacket(mod_instance_t mi, pkt_t pkt);
mod_ret_t mod_perl_onpacket(pkt_t pkt);
//void my_modperl_write_nad(nad_t n);
//char* mod_perl_callback(char *subroutine, char *parm1, char *parm2, char *parm3, char *parm4);
void mod_perl_destroy();

