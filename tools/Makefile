TARGETS=primetest
TESTS=
EXTRA_DEPS=$(BASEDIR)/lib/libopenstrument.a

include $(BASEDIR)/Makefile.inc

## DEPENDENCIES START HERE (do not remove this line)
primetest.o: primetest.cc $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/socket.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/exported_vars.h \
 $(BASEDIR)/lib/atomic.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/timer.h
