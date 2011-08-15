TARGETS=store store_client
TESTS=
EXTRA_DEPS=store.o record_log.o $(BASEDIR)/lib/libopenstrument.a
EXTRA_LIBS_store=record_log.o -lopeninstrument
EXTRA_DEPS_store=datastore.o record_log.o $(BASEDIR)/lib/libopenstrument.a
EXTRA_LIBS_hostport_test=hostport.o
EXTRA_LIBS_datastore_test=hostport.o

include $(BASEDIR)/Makefile.inc

## DEPENDENCIES START HERE (do not remove this line)
datastore.o: datastore.cc \
 $(BASEDIR)/server/datastore.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/server/hostport.h
datastore_test.o: datastore_test.cc \
 $(BASEDIR)/server/datastore.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/server/hostport.h
hostport.o: hostport.cc \
 $(BASEDIR)/server/hostport.h \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h
hostport_test.o: hostport_test.cc hostport.h \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h
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
 $(BASEDIR)/server/hostport.h \
 $(BASEDIR)/server/record_log.h
store_client.o: store_client.cc \
 $(BASEDIR)/lib/atomic.h \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/http_client.h \
 $(BASEDIR)/lib/http_reply.h \
 $(BASEDIR)/lib/http_message.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/http_headers.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/http_request.h \
 $(BASEDIR)/lib/socket.h \
 $(BASEDIR)/lib/uri.h
