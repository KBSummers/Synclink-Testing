#
# Makefile for SyncLink use tools:
#
# mgslutil    set SyncLink configuration options
# mgsltest    test SyncLink devices
# sethdlc     network interface setup tool (PPP/Cisco HDLC/frame relay)
#
# Microgate and SyncLink are registered trademarks
# of Microgate Systems Ltd
#

# Destination to install utility programs
INSTALL_DIR = /usr/local/sbin

# Uncomment this to add debugging information to programs
#DEBUG=1

CFLAGS := -Wall -Wstrict-prototypes -fomit-frame-pointer
ifdef DEBUG
CFLAGS += -g -O
else
CFLAGS += -O2
endif

INCLUDE_DIR = ../../include
SYNCLINK_H = $(INCLUDE_DIR)/synclink.h

all: utils

TOOLS := mgslutil mgsltest sethdlc
utils: $(TOOLS)

mgslutil: mgslutil.c $(SYNCLINK_H)
	gcc $(CFLAGS) -I $(INCLUDE_DIR) -o $@ $<

mgsltest: mgsltest.c $(SYNCLINK_H)
	gcc $(CFLAGS) -I $(INCLUDE_DIR) -o $@ $<

sethdlc: sethdlc.c
	gcc  $(CFLAGS) -o $@ $<

install : utils

	@if [ ! -d $(INSTALL_DIR) ]; then \
		echo ' ** ' directory "$(INSTALL_DIR) does not exist"; \
		exit -1; \
	fi

	@for f in $(TOOLS) ; do \
		if [ -f $$f ] ; then \
	    		cp -av $$f $(INSTALL_DIR) ; \
		fi \
	done;

#
# Remove generated and backup files
#
CLEAN_TARGETS = *~ core $(TOOLS) *.o *.ko *.mod.c *.mod.o .*.o.flags .*o.cmd \
		*.unsigned* .tmp_versions modules.order Module.symvers
clean:
	@for f in $(CLEAN_TARGETS) ; do \
		find . -name "$$f" | xargs rm -rf ; \
	done;
	@find . -type l | xargs rm -f
	@rm -rf drivers/linux
