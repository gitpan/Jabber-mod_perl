/*
 * Jabber::mod_perl - mod_perl for jabberd
 * Copyright (c) 2002 Piers Harding
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


/*   Declare up the perl stuff    */
#include <EXTERN.h>
#include <XSUB.h>
#include <perl.h>

// only declared here so that compiling mod_perl doesnt complain - the 
// real business is happening in perlxsi.c
EXTERN_C void xs_init (pTHXo);

static PerlInterpreter *mod_perl_interpreter;

SV* sv_on_packet_subroutine = (SV*)NULL;

// special function definitions for booting C Perl modules
XS(boot_Jabber__pkt);
XS(boot_Jabber__NADs);

#include "jadperl.h"

// no. of parms pased to interpreter initialisation
#define MOD_PERL_NO_PARMS 4

char* mod_perl_realname;
char* mod_perl_method_init = "Jabber::mod_perl::initialise";
char* mod_perl_method_onpacket = "Jabber::mod_perl::onPacket";
nad_t mod_perl_config;

char *embedding[] = { "", "-e", "1" };
char* use_mod_perl = "use Jabber::Reload; use Jabber::mod_perl;";


/*----------------------------------------------------------------------------------*

  code generated from /usr/bin/perl -MExtUtils::Embed -e xsinit -- -std -o perlxsi.c
  to facilitate the loading of other modules with C extensions - this gives:

EXTERN_C void boot_DynaLoader (pTHXo_ CV* cv);

EXTERN_C void
xs_init(pTHXo);

*-----------------------------------------------------------------------------------*/

