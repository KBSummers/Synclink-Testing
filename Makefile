INCLUDE_DIR=include
SYNCLINK_H=$(INCLUDE_DIR)/synclink.h


CFLAGS := -Wall -Wstrict-prototypes -fomit-frame-pointer -I $(INCLUDE_DIR)
ifdef DEBUG
CFLAGS += -g -O
else
CFLAGS += -O2
endif
.PHONY: clean

all: test loopback

.PHONY: loopback
loopback:
	make -C src/loopback/

test: src/synclink_test.c $(SYNCLINK_H)
	gcc $(CFLAGS) -o $@ $< -lpthread

clean:
	if [ -f test ]; then rm test; fi
	$(MAKE) -C src/loopback clean
