# ---------------------------------------------------------------------------
# libpoff/Makefile
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# Directories

PASCAL      = ${shell pwd}/..

include $(PASCAL)/Make.config
include $(PASCAL)/Make.defs

INCDIR      = $(PASCAL)/include
LIBDIR      = $(PASCAL)/lib

# ---------------------------------------------------------------------------
# Objects and targets

LIBPOFFSRCS  = pfhandle.c pfproghandle.c pftprog.c pfsymhandle.c
LIBPOFFSRCS += pftsymbol.c pfrelochandle.c pftreloc.c pftgetreloc.c
LIBPOFFSRCS += pfwhdr.c pfwrodata.c pfwsymbol.c pfwfname.c
LIBPOFFSRCS += pfwprog.c pfwlineno.c pfwdbgfunc.c pfwreloc.c pfwstring.c
LIBPOFFSRCS += pfwrite.c pfrhdr.c pfrsymbol.c pfrfname.c
LIBPOFFSRCS += pfrprog.c pfrlineno.c pfrdbgfunc.c pfrrawlineno.c
LIBPOFFSRCS += pfreloc.c pfrstring.c pfread.c pfrseek.c
LIBPOFFSRCS += pfrelease.c pfdbgcontainer.c pfdbgdiscard.c
LIBPOFFSRCS += pfxprog.c pfxrodata.c pfiprog.c pfirodata.c
LIBPOFFSRCS += pfdhdr.c pfdsymbol.c pfdreloc.c pflabel.c
LIBPOFFSRCS += pflineno.c pofferr.c pfdbginfo.c pfswap.c
OBJS         = $(LIBPOFFSRCS:.c=.o)
DEPS         = $(LIBPOFFSRCS:.c=.d)

SUFFIXES    += .d

all: libpoff.a
.PHONY: all libpoff.a clean

$(DEPS): %.d: %.c
	$(CC) $(CFLAGS) -MM $< -MF $@

$(OBJS): %.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(LIBDIR)/libpoff.a: $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS) $@ $^

libpoff.a: $(DEPS) $(LIBDIR)/libpoff.a

clean:
	$(RM) libpoff.a *.o *.d core *~

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif
