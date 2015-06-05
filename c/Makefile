# $Header: /home/cjm/cvs/log_udp/c/Makefile,v 1.3 2015-06-05 14:52:48 cjm Exp $

include	../../Makefile.common
include	../Makefile.common

CNAME		= c
BINDIR		= $(LOG_BIN_HOME)/$(CNAME)/$(HOSTTYPE)
INCDIR 		= $(LOG_SRC_HOME)/include
DOCSDIR 	= $(LOG_DOC_HOME)/cdocs

DEBUG           = 0
#DEBUG           = 2
CFLAGS = -g $(CCHECKFLAG) $(SHARED_LIB_CFLAGS) -I$(INCDIR) -L$(LT_LIB_HOME) -DDEBUG=$(DEBUG)

LINTFLAGS = -I$(INCDIR) -I$(JNIINCDIR) -I$(JNIMDINCDIR)
DOCFLAGS = -static
SRCS 		= 	log_general.c log_udp.c log_create.c
HEADERS		=	$(SRCS:%.c=$(INCDIR)/%.h)
OBJS		=	$(SRCS:%.c=$(BINDIR)/%.o)
DOCS 		= 	$(SRCS:%.c=$(DOCSDIR)/%.html)

top: shared static docs 

shared: $(LT_LIB_HOME)/lib$(LOG_LIBNAME).so

$(LT_LIB_HOME)/lib$(LOG_LIBNAME).so: $(OBJS)
	cc $(CCSHAREDFLAG) $(CFLAGS) $(OBJS) -o $@ $(TIMELIB)

static: $(LT_LIB_HOME)/lib$(LOG_LIBNAME).a

$(LT_LIB_HOME)/lib$(LOG_LIBNAME).a: $(OBJS)
	ar rcv $@ $?

$(BINDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

docs: $(DOCS)

$(DOCS): $(SRCS)
	-$(CDOC) -d $(DOCSDIR) -h $(INCDIR) $(DOCFLAGS) $(SRCS)

$(DOCS) : $(SRCS)

checkout:
	-$(CO) $(CO_OPTIONS) $(SRCS)
	-(cd $(INCDIR); $(CO) $(CO_OPTIONS) $(HEADERS);)

checkin:
	-$(CI) $(CI_OPTIONS) $(SRCS)
	-(cd $(INCDIR); $(CI) $(CI_OPTIONS) $(HEADERS);)

staticdepend:
	makedepend $(MAKEDEPENDFLAGS) -- $(CFLAGS) -- $(SRCS)

depend:
	makedepend $(MAKEDEPENDFLAGS) -- $(CFLAGS) -- $(SRCS)

lint:
	$(LINT)	$(LINTFLAGS) $(SRCS)

clean:
	-$(RM) $(RM_OPTIONS) $(OBJS) $(LT_LIB_HOME)/lib$(LOG_LIBNAME).so $(LT_LIB_HOME)/lib$(LOG_LIBNAME)_g.so $(LT_LIB_HOME)/lib$(LOG_LIBNAME).a $(TIDY_OPTIONS)

tidy:
	-$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	-(cd $(INCDIR); $(RM) $(RM_OPTIONS) $(TIDY_OPTIONS);)

backup: tidy
	-$(RM) $(RM_OPTIONS) $(JAVAOBJS) $(INCDIR)/ngat_eip_EIPPLC.h

#
# $Log: not supported by cvs2svn $
# Revision 1.2  2015/06/05 11:33:32  cjm
# Changed HEADERS to full path.
#
# Revision 1.1  2009/01/09 14:54:43  cjm
# Initial revision
#
#

# DO NOT DELETE

log_general.o: /usr/include/errno.h /usr/include/features.h
log_general.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
log_general.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
log_general.o: /usr/include/asm/errno.h /usr/include/stdio.h
log_general.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stddef.h
log_general.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
log_general.o: /usr/include/bits/typesizes.h /usr/include/libio.h
log_general.o: /usr/include/_G_config.h /usr/include/wchar.h
log_general.o: /usr/include/bits/wchar.h /usr/include/gconv.h
log_general.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stdarg.h
log_general.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
log_general.o: /usr/include/stdlib.h /usr/include/string.h
log_general.o: /usr/include/time.h /usr/include/bits/time.h
log_general.o: /home/dev/src/log_udp/include/log_general.h
log_udp.o: /usr/include/endian.h /usr/include/features.h
log_udp.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
log_udp.o: /usr/include/bits/endian.h /usr/include/byteswap.h
log_udp.o: /usr/include/bits/byteswap.h /usr/include/errno.h
log_udp.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
log_udp.o: /usr/include/asm/errno.h /usr/include/fcntl.h
log_udp.o: /usr/include/bits/fcntl.h /usr/include/sys/types.h
log_udp.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
log_udp.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stddef.h
log_udp.o: /usr/include/bits/typesizes.h /usr/include/time.h
log_udp.o: /usr/include/bits/time.h /usr/include/netdb.h
log_udp.o: /usr/include/netinet/in.h /usr/include/stdint.h
log_udp.o: /usr/include/bits/wchar.h /usr/include/bits/socket.h
log_udp.o: /usr/include/limits.h
log_udp.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/limits.h
log_udp.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
log_udp.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
log_udp.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
log_udp.o: /usr/include/asm/sockios.h /usr/include/bits/in.h
log_udp.o: /usr/include/bits/netdb.h /usr/include/pthread.h
log_udp.o: /usr/include/sched.h /usr/include/bits/sched.h
log_udp.o: /usr/include/signal.h /usr/include/bits/sigset.h
log_udp.o: /usr/include/bits/pthreadtypes.h /usr/include/bits/initspin.h
log_udp.o: /usr/include/bits/sigthread.h /usr/include/stdio.h
log_udp.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
log_udp.o: /usr/include/gconv.h
log_udp.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stdarg.h
log_udp.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
log_udp.o: /usr/include/stdlib.h /usr/include/string.h
log_udp.o: /usr/include/arpa/inet.h /usr/include/sys/socket.h
log_udp.o: /usr/include/sys/uio.h /usr/include/bits/uio.h
log_udp.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
log_udp.o: /usr/include/bits/confname.h /usr/include/getopt.h
log_udp.o: /home/dev/src/log_udp/include/log_general.h
log_udp.o: /home/dev/src/log_udp/include/log_udp.h
log_create.o: /usr/include/errno.h /usr/include/features.h
log_create.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
log_create.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
log_create.o: /usr/include/asm/errno.h /usr/include/stdio.h
log_create.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stddef.h
log_create.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
log_create.o: /usr/include/bits/typesizes.h /usr/include/libio.h
log_create.o: /usr/include/_G_config.h /usr/include/wchar.h
log_create.o: /usr/include/bits/wchar.h /usr/include/gconv.h
log_create.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stdarg.h
log_create.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
log_create.o: /usr/include/stdlib.h /usr/include/string.h /usr/include/time.h
log_create.o: /usr/include/bits/time.h /usr/include/sys/time.h
log_create.o: /usr/include/sys/select.h /usr/include/bits/select.h
log_create.o: /usr/include/bits/sigset.h
log_create.o: /home/dev/src/log_udp/include/log_general.h
log_create.o: /home/dev/src/log_udp/include/log_udp.h
log_create.o: /home/dev/src/log_udp/include/log_create.h
