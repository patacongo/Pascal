# ----------------------------------------------------------------------
# Makefile
# ----------------------------------------------------------------------

# ----------------------------------------------------------------------
# Directories

PASCAL			= ${shell pwd}

include $(PASCAL)/Make.config
include $(PASCAL)/Make.defs

INCDIR			= $(PASCAL)/include
LIBDIR			= $(PASCAL)/lib
BINDIR			= $(PASCAL)/bin16
LIBPOFFDIR		= $(PASCAL)/libpoff
LIBPASDIR		= $(PASCAL)/libpas
PASDIR			= $(PASCAL)/pascal
PLINKDIR		= $(PASCAL)/plink
TESTDIR			= $(PASCAL)/tests
INSNDIR			= $(PASCAL)/insn16
LIBINSNDIR		= $(INSNDIR)/libinsn

# ----------------------------------------------------------------------
# Objects and targets

LIBS			= $(LIBDIR)/libpoff.a $(LIBDIR)/libpas.a \
			  $(LIBDIR)/libinsn.a

all: pascal popt regm plink plist prun
.PHONY: all config.h libpoff.a libpas.a libinsn.a pascal popt regm plink plist prun clean deep-clean

$(INCDIR)/config.h: Make.config
	@$(MAKE) -f Make.config.h

config.h: $(INCDIR)/config.h

$(LIBDIR):
	mkdir $(LIBDIR)

$(LIBDIR)/libpoff.a: $(LIBDIR) config.h
	@$(MAKE) -C $(LIBPOFFDIR) libpoff.a

libpoff.a: $(LIBDIR)/libpoff.a

$(LIBDIR)/libpas.a: $(LIBDIR) config.h
	@$(MAKE) -C $(LIBPASDIR) libpas.a

libpas.a: $(LIBDIR)/libpas.a

$(LIBDIR)/libinsn.a: $(LIBDIR) config.h
	@$(MAKE) -C $(LIBINSNDIR) libinsn.a

libinsn.a: $(LIBDIR)/libinsn.a

$(BINDIR):
	mkdir $(BINDIR)

$(BINDIR)/pascal: $(BINDIR) config.h $(LIBS)
	@$(MAKE) -C $(PASDIR)

pascal: $(BINDIR)/pascal

$(BINDIR)/popt: $(BINDIR) config.h $(LIBS)
	@$(MAKE) -C $(INSNDIR) popt

popt: $(BINDIR)/popt

$(BINDIR)/regm: $(BINDIR) config.h $(LIBS)
ifeq ($(CONFIG_REGM),y)
	@$(MAKE) -C $(INSNDIR) regm
endif

regm: $(BINDIR)/regm

$(BINDIR)/plink: $(BINDIR) config.h $(LIBS)
	@$(MAKE) -C $(PLINKDIR)

plink: $(BINDIR)/plink

$(BINDIR)/prun: $(BINDIR) config.h $(LIBS)
	@$(MAKE) -C $(INSNDIR) prun

prun: $(BINDIR)/prun

$(BINDIR)/plist: $(BINDIR) config.h $(LIBS)
	@$(MAKE) -C $(INSNDIR) plist

plist: $(BINDIR)/plist

clean:
	$(RM) -f core *~
	$(RM) -rf $(LIBDIR)
	$(RM) -rf bin16 bin32
	$(MAKE) -f Make.config.h clean
	$(MAKE) -C $(LIBPOFFDIR) clean
	$(MAKE) -C $(LIBPASDIR) clean
	$(MAKE) -C $(PASDIR) clean
	$(MAKE) -C $(PLINKDIR) clean
	$(MAKE) -C $(INSNDIR) clean
	find . -name \*~ -exec rm -f {} \;
	find tests -name "*.err" -exec rm -f {} \;
	find tests -name "*.lst" -exec rm -f {} \;
	find tests -name "*.pex" -exec rm -f {} \;
	find tests -name "*.o1" -exec rm -f {} \;
	find tests -name "*.o" -exec rm -f {} \;
	find tests -name "*.d" -exec rm -f {} \;

deep-clean: clean
	rm -f .config include/config.h Make.config
	$(RM) bin16/*
	$(RM) bin32/*

# ----------------------------------------------------------------------
