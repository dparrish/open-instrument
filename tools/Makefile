TARGETS=primetest dump_recordlog dump_store_config reindex_store_file
TESTS=
EXTRA_DEPS=$(BASEDIR)/lib/libopeninstrument.a
EXTRA_LIBS_dump_recordlog := $(BASEDIR)/server/record_log.o
EXTRA_LIBS_reindex_store_file := $(BASEDIR)/server/record_log.o $(BASEDIR)/server/disk_datastore.o $(BASEDIR)/server/indexed_store_file.o

include $(BASEDIR)/Makefile.inc

dump_recordlog: dump_recordlog.o

reindex_store_file: reindex_store_file.o $(BASEDIR)/server/record_log.o $(BASEDIR)/server/disk_datastore.o $(BASEDIR)/server/indexed_store_file.o

## DEPENDENCIES START HERE (do not remove this line)
dump_recordlog.o: dump_recordlog.cc \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/variable.h \
 $(BASEDIR)/server/record_log.h \
 $(BASEDIR)/lib/trie.h
primetest.o: primetest.cc $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/socket.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/exported_vars.h \
 $(BASEDIR)/lib/atomic.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/variable.h
reindex_store_file.o: reindex_store_file.cc \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/variable.h \
 $(BASEDIR)/server/disk_datastore.h \
 $(BASEDIR)/server/indexed_store_file.h \
 $(BASEDIR)/lib/trie.h \
 $(BASEDIR)/server/record_log.h
