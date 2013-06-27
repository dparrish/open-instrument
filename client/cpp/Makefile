include $(BASEDIR)/Makefile.inc
EXTRA_DEPS=$(BASEDIR)/lib/libopeninstrument.a
EXTRA_LIBS=$(BASEDIR)/lib/libopeninstrument.a
TARGETS=add get list

all: $(TARGETS)

## DEPENDENCIES START HERE (do not remove this line)
add.o: add.cc $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/socket.h \
 $(BASEDIR)/lib/store_client.h \
 $(BASEDIR)/lib/exported_vars.h \
 $(BASEDIR)/lib/atomic.h \
 $(BASEDIR)/lib/variable.h
get.o: get.cc $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/store_client.h \
 $(BASEDIR)/lib/exported_vars.h \
 $(BASEDIR)/lib/atomic.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/variable.h \
 $(BASEDIR)/lib/socket.h \
 $(BASEDIR)/lib/store_config.h \
 $(BASEDIR)/lib/hash.h
list.o: list.cc $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/store_client.h \
 $(BASEDIR)/lib/exported_vars.h \
 $(BASEDIR)/lib/atomic.h \
 $(BASEDIR)/lib/variable.h \
 $(BASEDIR)/lib/socket.h \
 $(BASEDIR)/lib/store_config.h \
 $(BASEDIR)/lib/hash.h
