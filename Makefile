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

test: src/raw_test_old/synclink_test.c $(SYNCLINK_H)
	gcc $(CFLAGS) -o src/raw_test_old/$@ $< -lpthread

clean:
	if [ -f src/raw_test_old/test ]; then rm src/raw_test_old/test; fi
	$(MAKE) -C src/loopback clean
