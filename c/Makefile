# $Header: /home/cjm/cvs/log_udp/c/Makefile,v 1.1 2009-01-09 14:54:43 cjm Exp $

include	../../Makefile.common
include	../Makefile.common

CNAME		= c
BINDIR		= $(LOG_BIN_HOME)/$(CNAME)/$(HOSTTYPE)
INCDIR 		= $(LOG_SRC_HOME)/include
DOCSDIR 	= $(LOG_DOC_HOME)/cdocs

DEBUG           = 0
#DEBUG           = 2
CFLAGS = -g $(CCHECKFLAG) -I$(INCDIR) -L$(LT_LIB_HOME) -DDEBUG=$(DEBUG)

LINTFLAGS = -I$(INCDIR) -I$(JNIINCDIR) -I$(JNIMDINCDIR)
DOCFLAGS = -static
SRCS 		= 	log_general.c log_udp.c log_create.c
HEADERS		=	$(SRCS:%.c=%.h)
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
#

# DO NOT DELETE

