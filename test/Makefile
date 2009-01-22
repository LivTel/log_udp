# $Header: /home/cjm/cvs/log_udp/test/Makefile,v 1.4 2009-01-22 11:16:18 cjm Exp $

include ../../Makefile.common
include ../Makefile.common

INCDIR 		= $(LOG_SRC_HOME)/include
TESTDIR 	= test
BINDIR 		= $(LOG_BIN_HOME)/$(TESTDIR)/$(HOSTTYPE)
DOCSDIR 	= $(LOG_DOC_HOME)/$(TESTDIR)

#DEBUG           = 0
DEBUG           = 2
CFLAGS 		= -g -I$(INCDIR) -DDEBUG=$(DEBUG)
DOCFLAGS 	= -static

SRCS 		= ltlog.c messages_to_udp.c tcs_to_udp.c

OBJS 		= $(SRCS:%.c=%.o)
PROGS 		= $(SRCS:%.c=$(BINDIR)/%)
PROGS_STATIC 	= $(SRCS:%.c=$(BINDIR)/%_static)
DOCS 		= $(SRCS:%.c=$(DOCSDIR)/%.html)

top: $(PROGS) $(PROGS_STATIC) docs

$(BINDIR)/%: %.o
	cc -o $@ $< -L$(LT_LIB_HOME) -l$(LOG_LIBNAME) $(TIMELIB) $(SOCKETLIB) -lm -lc

$(BINDIR)/%_static: $(LT_LIB_HOME)/lib$(LOG_LIBNAME).a

$(BINDIR)/%_static: %.o
	cc -static -o $@ $< -L$(LT_LIB_HOME) -l$(LOG_LIBNAME) $(TIMELIB) $(SOCKETLIB) -lm -lc

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

