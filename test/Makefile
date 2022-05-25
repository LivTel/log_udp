# $Header: /home/cjm/cvs/log_udp/test/Makefile,v 1.5 2015-06-05 14:52:52 cjm Exp $

include ../../Makefile.common
include ../Makefile.common

INCDIR 		= $(LOG_SRC_HOME)/include
TESTDIR 	= test
BINDIR 		= $(LOG_BIN_HOME)/$(TESTDIR)/$(HOSTTYPE)
DOCSDIR 	= $(LOG_DOC_HOME)/$(TESTDIR)

DEBUG           = 0
#DEBUG           = 2
CFLAGS 		= -g -I$(INCDIR) -DDEBUG=$(DEBUG)
DOCFLAGS 	= -static

SRCS 		= ltlog.c messages_to_udp.c tcs_to_udp.c log_buffer.c

OBJS 		= $(SRCS:%.c=$(BINDIR)/%.o)
PROGS 		= $(SRCS:%.c=$(BINDIR)/%)
PROGS_STATIC 	= $(SRCS:%.c=$(BINDIR)/%_static)
DOCS 		= $(SRCS:%.c=$(DOCSDIR)/%.html)

top: $(PROGS) $(PROGS_STATIC) docs

$(BINDIR)/log_buffer: $(BINDIR)/log_buffer.o
	cc -o $@ $< -L$(LT_LIB_HOME) -l$(LOG_LIBNAME) $(TIMELIB) $(SOCKETLIB) -lcommandserver -lpthread -lm -lc

$(BINDIR)/%: $(BINDIR)/%.o
	cc -o $@ $< -L$(LT_LIB_HOME) -l$(LOG_LIBNAME) $(TIMELIB) $(SOCKETLIB) -lm -lc

$(BINDIR)/%_static: $(LT_LIB_HOME)/lib$(LOG_LIBNAME).a

$(BINDIR)/log_buffer_static: $(BINDIR)/log_buffer.o
	cc -static -o $@ $< -L$(LT_LIB_HOME) -l$(LOG_LIBNAME) $(TIMELIB) $(SOCKETLIB)  -lcommandserver -lpthread -lm -lc

$(BINDIR)/%_static: $(BINDIR)/%.o
	cc -static -o $@ $< -L$(LT_LIB_HOME) -l$(LOG_LIBNAME) $(TIMELIB) $(SOCKETLIB) -lm -lc

$(BINDIR)/log_buffer.o: log_buffer.c
	$(CC) $(CFLAGS) -I$(LT_SRC_HOME)/commandserver/include -c $< -o $@

$(BINDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

docs: $(DOCS)

$(DOCS): $(SRCS)
	-$(CDOC) -d $(DOCSDIR) -h $(INCDIR) $(DOCFLAGS) $(SRCS)

depend:
	makedepend $(MAKEDEPENDFLAGS) -- $(CFLAGS) -- $(SRCS)

clean:
	$(RM) $(RM_OPTIONS) $(PROGS) $(PROGS_STATIC) $(OBJS) $(TIDY_OPTIONS)

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)

backup: tidy
	$(RM) $(RM_OPTIONS) $(OBJS)

checkin:
	-$(CI) $(CI_OPTIONS) $(SRCS)

checkout:
	-$(CO) $(CO_OPTIONS) $(SRCS)

# DO NOT DELETE

ltlog.o: /usr/include/stdio.h /usr/include/features.h
ltlog.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
ltlog.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stddef.h
ltlog.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
ltlog.o: /usr/include/bits/typesizes.h /usr/include/libio.h
ltlog.o: /usr/include/_G_config.h /usr/include/wchar.h
ltlog.o: /usr/include/bits/wchar.h /usr/include/gconv.h
ltlog.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stdarg.h
ltlog.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
ltlog.o: /usr/include/string.h /home/dev/src/log_udp/include/log_general.h
ltlog.o: /home/dev/src/log_udp/include/log_udp.h
ltlog.o: /home/dev/src/log_udp/include/log_create.h /usr/include/time.h
ltlog.o: /usr/include/bits/time.h
messages_to_udp.o: /usr/include/errno.h /usr/include/features.h
messages_to_udp.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
messages_to_udp.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
messages_to_udp.o: /usr/include/asm/errno.h /usr/include/stdio.h
messages_to_udp.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stddef.h
messages_to_udp.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
messages_to_udp.o: /usr/include/bits/typesizes.h /usr/include/libio.h
messages_to_udp.o: /usr/include/_G_config.h /usr/include/wchar.h
messages_to_udp.o: /usr/include/bits/wchar.h /usr/include/gconv.h
messages_to_udp.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stdarg.h
messages_to_udp.o: /usr/include/bits/stdio_lim.h
messages_to_udp.o: /usr/include/bits/sys_errlist.h /usr/include/string.h
messages_to_udp.o: /usr/include/time.h /usr/include/bits/time.h
messages_to_udp.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
messages_to_udp.o: /usr/include/bits/confname.h /usr/include/sys/types.h
messages_to_udp.o: /usr/include/sys/stat.h /usr/include/bits/stat.h
messages_to_udp.o: /home/dev/src/log_udp/include/log_general.h
messages_to_udp.o: /home/dev/src/log_udp/include/log_udp.h
messages_to_udp.o: /home/dev/src/log_udp/include/log_create.h
tcs_to_udp.o: /usr/include/errno.h /usr/include/features.h
tcs_to_udp.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
tcs_to_udp.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
tcs_to_udp.o: /usr/include/asm/errno.h /usr/include/stdio.h
tcs_to_udp.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stddef.h
tcs_to_udp.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
tcs_to_udp.o: /usr/include/bits/typesizes.h /usr/include/libio.h
tcs_to_udp.o: /usr/include/_G_config.h /usr/include/wchar.h
tcs_to_udp.o: /usr/include/bits/wchar.h /usr/include/gconv.h
tcs_to_udp.o: /usr/lib/gcc-lib/i386-redhat-linux/3.2.2/include/stdarg.h
tcs_to_udp.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
tcs_to_udp.o: /usr/include/string.h /usr/include/time.h
tcs_to_udp.o: /usr/include/bits/time.h /usr/include/unistd.h
tcs_to_udp.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
tcs_to_udp.o: /usr/include/sys/types.h /usr/include/sys/stat.h
tcs_to_udp.o: /usr/include/bits/stat.h
tcs_to_udp.o: /home/dev/src/log_udp/include/log_general.h
tcs_to_udp.o: /home/dev/src/log_udp/include/log_udp.h
tcs_to_udp.o: /home/dev/src/log_udp/include/log_create.h
