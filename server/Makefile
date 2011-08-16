TARGETS=store
TESTS=
EXTRA_DEPS=store.o record_log.o disk_datastore.o datastore.o $(BASEDIR)/lib/libopenstrument.a
EXTRA_LIBS_store=record_log.o disk_datastore.o datastore.o $(BASEDIR)/lib/libopenstrument.a
EXTRA_DEPS_store=disk_datastore.o datastore.o record_log.o $(BASEDIR)/lib/libopenstrument.a

include $(BASEDIR)/Makefile.inc

## DEPENDENCIES START HERE (do not remove this line)
datastore.o: datastore.cc $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/server/datastore.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/openinstrument.pb.h
datastore_test.o: datastore_test.cc \
 $(BASEDIR)/server/datastore.h \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/openinstrument.pb.h
disk_datastore.o: disk_datastore.cc \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/server/datastore.h \
 $(BASEDIR)/server/disk_datastore.h \
 $(BASEDIR)/server/record_log.h
record_log.o: record_log.cc \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/server/record_log.h
store.o: store.cc $(BASEDIR)/lib/atomic.h \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/counter.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/http_server.h \
 $(BASEDIR)/lib/exported_vars.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/request_handler.h \
 $(BASEDIR)/lib/http_headers.h \
 $(BASEDIR)/lib/http_reply.h \
 $(BASEDIR)/lib/http_message.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/http_request.h \
 $(BASEDIR)/lib/socket.h \
 $(BASEDIR)/lib/uri.h \
 $(BASEDIR)/lib/http_static_dir.h \
 $(BASEDIR)/lib/mime_types.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/server/datastore.h \
 $(BASEDIR)/server/disk_datastore.h \
 $(BASEDIR)/server/record_log.h
