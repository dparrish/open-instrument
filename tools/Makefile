TARGETS=primetest dump_recordlog dump_store_config
TESTS=
EXTRA_DEPS=$(BASEDIR)/lib/libopeninstrument.a
EXTRA_LIBS_dump_recordlog := $(BASEDIR)/server/record_log.o

include $(BASEDIR)/Makefile.inc

dump_recordlog: dump_recordlog.o

## DEPENDENCIES START HERE (do not remove this line)
dump_recordlog.o: dump_recordlog.cc \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/variable.h \
 $(BASEDIR)/server/record_log.h
dump_store_config.o: dump_store_config.cc \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/store_client.h \
 $(BASEDIR)/lib/exported_vars.h \
 $(BASEDIR)/lib/atomic.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/variable.h \
 $(BASEDIR)/lib/socket.h \
 $(BASEDIR)/server/store_config.h \
 $(BASEDIR)/lib/hash.h
primetest.o: primetest.cc $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/socket.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/exported_vars.h \
 $(BASEDIR)/lib/atomic.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/variable.h
