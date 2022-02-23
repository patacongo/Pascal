# ---------------------------------------------------------------------------
# libpoff/Makefile
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# Directories

PASCAL      = ${shell pwd}/..
include $(PASCAL)/tools/Config.mk

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
LIBPOFFSRCS += pfdhdr.c pfdsymbol.c pfdreloc.c pfdtreloc.c pfdlineno.c
LIBPOFFSRCS += pflabel.c pflineno.c pofferr.c pfdbginfo.c pfswap.c
OBJS         = $(LIBPOFFSRCS:.c=.o)
DEPS         = $(LIBPOFFSRCS:.c=.d)

SUFFIXES    += .d

#
# Targets
#

all: libpoff.a
.PHONY: all libpoff.a clean distclean

$(DEPS): %.d: %.c
	$(Q) echo "  $@"
	$(Q) $(CC) $(CFLAGS) -MM $< -MF $@

$(OBJS): %.o: %.c
	$(Q) echo "  $@"
	$(Q) $(CC) -c $(CFLAGS) $< -o $@

$(LIBDIR)/libpoff.a: $(OBJS)
	$(Q) echo "  $@"
	$(Q) $(RM) $@
	$(Q) $(AR) $(ARFLAGS) $@ $^ 1>/dev/null

libpoff.a: $(DEPS) $(LIBDIR)/libpoff.a

clean:
	$(Q) $(RM) libpoff.a *.o core *~

distclean: clean
	$(Q) $(RM) *.d .depend Make.dep

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),distclean)
-include $(DEPS)
endif
endif
