# ----------------------------------------------------------------------
# Makefile
# ----------------------------------------------------------------------
#
# Directories
#
PASDIR   = ${shell pwd}
PASCAL   = $(PASDIR)/..

include $(PASCAL)/.config
include $(PASCAL)/tools/Config.mk

all: clean
.PHONY: clean distclean

clean:
	$(Q) rm -f core *~ *.stackdump
	$(Q) rm -f students.dat contact.txt raindata.dat
	$(Q) rm -f src/*.err units/*.err
	$(Q) rm -f src/*.lst units/*.lst
	$(Q) rm -f src/*.pex units/*.pex
	$(Q) rm -f src/*.o1 units/*.o1
	$(Q) rm -f src/*.o units/*.o
	$(Q) rm -f src/*.d units/*.d

distclean: clean
