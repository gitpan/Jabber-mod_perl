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

/*   Declare up the perl stuff    */

#include "mod_perl.h"


/*----------------------------------------------------------------------------------*

  initialise all the perl handlers that will be used by mod_perl
  pass them the configuration nad, and the element number pointing
  to the <mod_perl/> configuration node

*-----------------------------------------------------------------------------------*/

void mod_perl_initialise(nad_t nad)
{

    int result;
    int el;
    SV* sv_rvalue;
    SV* sv_init_subroutine;
    dSP;

    log_debug(ZONE, "mod_perl - mod_perl_init: started");

    /*
     * stash a copy of the config nad
     * need to register config nad 
     * look here for the modules to load
     * and what to run as the init handler
     */

    log_debug(ZONE, "mod_perl_init - do copy");
    mod_perl_config = nad_copy(nad);

    // initial the argument stack
    ENTER;
    SAVETMPS;
    PUSHMARK(SP);

    // push the NAD onto the stack
    XPUSHs(
       sv_2mortal(
	 sv_bless(
           sv_setref_pv(newSViv(0), Nullch, (void *)nad),
	   gv_stashpv("Jabber::NADs", 0)
	   )
	 )
       );

    // push the instance no. of this module in the chain on
// XXX    XPUSHs( sv_2mortal( newSViv(mi->seq) ) );
    XPUSHs( sv_2mortal( newSViv(0) ) );

    // push the chain type that we are in on to the stack
    XPUSHs( sv_2mortal( newSVpv("JADPERL_PKT", 0) ) );

    // hunt down the handlers and push them onto the stack
    el = nad_find_elem(nad, 0, -1,  "modules", 1);
   
    // give the element no. for mod_perl
    XPUSHs(sv_2mortal(newSViv(el)));
    log_debug(ZONE, "mod_perl config is at el: %d", el);
 
    // push the Perl handler module names onto the stack
    el = nad_find_elem(nad, el, -1, "module", 1);
    while(el >= 0)
    {
       XPUSHs(sv_2mortal(newSVpvn(NAD_CDATA(nad, el), NAD_CDATA_L(nad, el))));
       log_debug(ZONE, "mod_perl handler is at el: %d", el);
       el = nad_find_elem(nad, el, -1, "module", 0);
    }
    
    // stash away the stack pointer
    PUTBACK;

    // do the perl call
    sv_init_subroutine = newSVpv(mod_perl_method_init, PL_na);
    log_debug(ZONE, "mod_perl - mod_perl_init: Calling routine: %s", SvPV(sv_init_subroutine,PL_na));
    result = perl_call_sv(sv_init_subroutine, G_EVAL | G_DISCARD );

    // disassemble the call results
    log_debug(ZONE, "mod_perl - mod_perl_init: Called routine: %s results: %d", SvPV(sv_init_subroutine,PL_na), result);
    if(SvTRUE(ERRSV))
        log_debug(ZONE, "mod_perl - mod_perl_init: perl call errored: %s", SvPV(ERRSV,PL_na));
    SPAGAIN;
    if (result > 0){
      sv_rvalue = POPs;
        log_debug(ZONE, "mod_perl - mod_perl_init: after call (%s): %d - %s", SvPV(sv_init_subroutine,PL_na), result, SvPV(sv_rvalue,SvCUR(sv_rvalue)));
    }
    PUTBACK;
    FREETMPS;
    LEAVE;

    log_debug(ZONE, "mod_perl_init - mod_perl has been initialised");

}


/*----------------------------------------------------------------------------------*

  do the onPacket callback - push the pkt oject onto the argument stack
  and the user must pass back HANDLED or PASS to hand back to jadperl

*-----------------------------------------------------------------------------------*/
//mod_ret_t mod_perl_onpacket(mod_instance_t mi, pkt_t pkt)
mod_ret_t mod_perl_onpacket(pkt_t pkt)
{

    int result;
    SV* sv_rvalue;
    mod_ret_t retr;
    dSP;

    log_debug(ZONE, "mod_perl - mod_perl_onpacket");

    // initialising the argument stack
    ENTER;
    SAVETMPS;
    PUSHMARK(SP);

    // push the pkt onto the stack
    XPUSHs(
       sv_2mortal(
	 sv_bless(
           sv_setref_pv(newSViv(0), Nullch, (void *)pkt),
	   gv_stashpv("Jabber::pkt", 0)
	   )
	 )
       );

    // push the instance no. of this module in the chain on
    //  - 0 for JADPERL_PKT
    XPUSHs( sv_2mortal( newSViv(0) ) );

    // declare the packet region
    XPUSHs( sv_2mortal( newSVpv("JADPERL_PKT", 0) ) );

    // push the module instance args on - blank for jadperl
    XPUSHs( sv_2mortal( newSVpv("", 0) ) );

    // stash the stack point
    PUTBACK;

    // do the onPacket call
    log_debug(ZONE, "mod_perl - mod_perl_onpacket: Calling routine: %s",
		    SvPV(sv_on_packet_subroutine,PL_na));
    result = perl_call_sv(sv_on_packet_subroutine, G_EVAL | G_SCALAR );

    // disassemble the results off the argument stack
    log_debug(ZONE, "mod_perl - mod_perl_onpacket: Called routine: %s results: %d",
		    SvPV(sv_on_packet_subroutine,PL_na),
		    result);
    if(SvTRUE(ERRSV))
        log_debug(ZONE, "mod_perl - mod_perl_onpacket: perl call errored: %s",
		       	SvPV(ERRSV,PL_na));
    SPAGAIN;

    // was this handled or passed?
    if (result > 0){
      sv_rvalue = POPs;
        log_debug(ZONE, "mod_perl - mod_perl_onpacket: after call (%s): returns(%d) - code(%s)",
		       	SvPV(sv_on_packet_subroutine,PL_na),
		       	result,
		       	SvPV(sv_rvalue,SvCUR(sv_rvalue)));
	if (SvIV(sv_rvalue) == 1){
          //pkt_free( pkt );
          log_debug(ZONE, "mod_perl - mod_perl_onpacket: PACKET HANDLED ");
          retr = mod_HANDLED;
        } else if (SvIV(sv_rvalue) == 2){
          retr = mod_PASS;
        } else {
          log_debug(ZONE, "mod_perl - mod_perl_onpacket: PACKET PASSED ");
          retr = mod_PASS;
        }
    } else {
        log_debug(ZONE, "mod_perl - mod_perl_onpacket: after call (%s): %d - NO RETURN BAD ERROR",
		       	SvPV(sv_on_packet_subroutine,PL_na),
		       	result);
	retr = mod_PASS;
    }
    PUTBACK;
    FREETMPS;
    LEAVE;

    return retr;

}


/*----------------------------------------------------------------------------------*

  Run the perl evaluated code snippet - return the scalar value result

*-----------------------------------------------------------------------------------*/
SV* mod_perl_eval_pv(char *subroutine)
{

    SV* my_sv;
    log_debug(ZONE, "mod_perl Perl eval (PV): %s", subroutine);
    my_sv = eval_pv(subroutine, FALSE );
    if(SvTRUE(ERRSV)) {
        log_debug(ZONE, "mod_perl Perl eval error (PV): %s", SvPV(ERRSV,PL_na));
	//exit(0);
    } else {
        log_debug(ZONE, "mod_perl Perl eval successful (PV)");
        return my_sv;
    }

}


/*----------------------------------------------------------------------------------*

  Clean up the interpreter at the end of the process life

*-----------------------------------------------------------------------------------*/
void mod_perl_destroy()
{

    perl_destruct(mod_perl_interpreter);
    perl_free(mod_perl_interpreter);
    log_debug(ZONE, "mod_perl has been cleaned up");

}



/*----------------------------------------------------------------------------------*

  jadperl mod_perl initialisation routine

*-----------------------------------------------------------------------------------*/
//int mod_perl_init_interp(nad_t nad)
int mod_perl_init_interp()
{

    log_debug(ZONE, "mod_perl_init - init");

    mod_perl_interpreter = perl_alloc();
    perl_construct( mod_perl_interpreter );
    perl_parse(mod_perl_interpreter, xs_init, MOD_PERL_NO_PARMS, embedding, NULL);
    perl_run(mod_perl_interpreter);
    mod_perl_eval_pv(use_mod_perl);

    // initialise the pure C Perl modules
    boot_Jabber__pkt(Nullcv);
    boot_Jabber__NADs(Nullcv);

    // setup the sv of onpacket code - parse the callback
    sv_on_packet_subroutine = newSVpv(mod_perl_method_onpacket, PL_na);
    sv_setpv(sv_on_packet_subroutine, mod_perl_method_onpacket);

    log_debug(ZONE, "mod_perl_init - Perl interpreter has been initialised");

    return 0;
}

