INCLUDES = -DCONFIG_DIR=\"$(sysconfdir)\"

bin_PROGRAMS = jadperl

noinst_HEADERS = mod_perl.h jadperl.h
jadperl_SOURCES = main.c \
             pkt.c \
             perlxsi.c \
             Jabber_pkt.c \
             Jabber_NADs.c \
             mod_perl.c \
             jadperl.c

jadperl_LDADD = $(top_builddir)/sx/libsx.a \
           $(top_builddir)/mio/libmio.a \
           $(top_builddir)/util/libutil.a \
           $(top_builddir)/expat/libexpat.a

