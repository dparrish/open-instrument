TARGETS=libopeninstrument.a $(BASEDIR)/static/openinstrument.pb.js
TESTS=file_test variable_test string_test timer_test uri_test exported_vars_test cord_test socket_test protobuf_test \
      counter_test trie_test
OBJS=openinstrument.pb.o common.o string.o uri.o exported_vars.o base64.o protobuf.o cord.o socket.o \
     http_message.o http_server.o http_client.o http_reply.o http_request.o store_client.o http_static_dir.o closure.o \
     mime_types.o file.o counter.o timer.o variable.o
TEST_LIBS+=libopeninstrument.a
TEST_DEPS=libopeninstrument.a
CLEAN_TARGETS=openinstrument.pb.cc openinstrument.pb.h
SUBDIRS=

include $(BASEDIR)/Makefile.inc

$(BASEDIR)/static/openinstrument.pb.js: openinstrument.proto
	@echo "Generating $@ from $<"
	@../static/protojs/pbj $< $@

libopeninstrument.a: $(OBJS)

## DEPENDENCIES START HERE (do not remove this line)
base64.o: base64.cc base64.h
closure.o: closure.cc $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h
common.o: common.cc $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h
cord.o: cord.cc $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/cord.h
cord_test.o: cord_test.cc $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h
file.o: file.cc $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/file.h
file_test.o: file_test.cc $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/file.h
http_client.o: http_client.cc \
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
http_message.o: http_message.cc \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/http_message.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/http_headers.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/socket.h
http_reply.o: http_reply.cc \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/http_message.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/http_headers.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/http_reply.h \
 $(BASEDIR)/lib/socket.h
http_request.o: http_request.cc \
 $(BASEDIR)/lib/http_headers.h \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/http_message.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/http_request.h \
 $(BASEDIR)/lib/socket.h \
 $(BASEDIR)/lib/uri.h
mime_types.o: mime_types.cc \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/mime_types.h \
 $(BASEDIR)/lib/trie.h
socket.o: socket.cc $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/socket.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/timer.h
socket_test.o: socket_test.cc \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/socket.h
string.o: string.cc $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/threadpool.h
string_test.o: string_test.cc \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/threadpool.h
timer.o: timer.cc $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/timer.h
timer_test.o: timer_test.cc $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h
trie_test.o: trie_test.cc $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/trie.h
uri.o: uri.cc $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/uri.h
uri_test.o: uri_test.cc $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/uri.h
