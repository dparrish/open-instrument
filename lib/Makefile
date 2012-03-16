TARGETS=libopeninstrument.a $(BASEDIR)/static/openinstrument.pb.js
TESTS=hash_test file_test variable_test string_test timer_test uri_test exported_vars_test cord_test \
      socket_test protobuf_test counter_test trie_test
OBJS=openinstrument.pb.o common.o hash.o threadpool.o string.o uri.o exported_vars.o base64.o protobuf.o \
     cord.o socket.o http_message.o http_server.o http_client.o http_reply.o http_request.o store_client.o \
     http_static_dir.o closure.o mime_types.o file.o counter.o timer.o variable.o
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
 $(BASEDIR)/lib/string.h
common.o: common.cc $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h
cord.o: cord.cc $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/cord.h
cord_test.o: cord_test.cc $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h
counter.o: counter.cc $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/counter.h
counter_test.o: counter_test.cc \
 $(BASEDIR)/lib/counter.h \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/openinstrument.pb.h
exported_vars.o: exported_vars.cc \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/exported_vars.h \
 $(BASEDIR)/lib/atomic.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/variable.h \
 $(BASEDIR)/lib/store_client.h \
 $(BASEDIR)/lib/socket.h
exported_vars_test.o: exported_vars_test.cc \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/exported_vars.h \
 $(BASEDIR)/lib/atomic.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/variable.h
file.o: file.cc $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/file.h
file_test.o: file_test.cc $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/file.h
hash.o: hash.cc $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/hash.h lookup3.c
hash_test.o: hash_test.cc $(BASEDIR)/lib/hash.h \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h
http_client.o: http_client.cc \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
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
 $(BASEDIR)/lib/http_message.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/http_headers.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/socket.h
http_reply.o: http_reply.cc \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
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
 $(BASEDIR)/lib/http_message.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/http_request.h \
 $(BASEDIR)/lib/socket.h \
 $(BASEDIR)/lib/uri.h
http_server.o: http_server.cc \
 $(BASEDIR)/lib/http_server.h \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/exported_vars.h \
 $(BASEDIR)/lib/atomic.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/variable.h \
 $(BASEDIR)/lib/request_handler.h \
 $(BASEDIR)/lib/http_headers.h \
 $(BASEDIR)/lib/http_reply.h \
 $(BASEDIR)/lib/http_message.h \
 $(BASEDIR)/lib/http_request.h \
 $(BASEDIR)/lib/socket.h \
 $(BASEDIR)/lib/uri.h
http_static_dir.o: http_static_dir.cc \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/http_reply.h \
 $(BASEDIR)/lib/http_message.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/http_headers.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/http_request.h \
 $(BASEDIR)/lib/socket.h \
 $(BASEDIR)/lib/uri.h \
 $(BASEDIR)/lib/http_server.h \
 $(BASEDIR)/lib/exported_vars.h \
 $(BASEDIR)/lib/atomic.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/variable.h \
 $(BASEDIR)/lib/request_handler.h \
 $(BASEDIR)/lib/http_static_dir.h \
 $(BASEDIR)/lib/mime_types.h \
 $(BASEDIR)/lib/trie.h
mime_types.o: mime_types.cc \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/mime_types.h \
 $(BASEDIR)/lib/trie.h
openinstrument.pb.o: openinstrument.pb.cc openinstrument.pb.h
protobuf.o: protobuf.cc $(BASEDIR)/lib/base64.h \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/variable.h
protobuf_test.o: protobuf_test.cc \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/variable.h
socket.o: socket.cc $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/socket.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/timer.h
socket_test.o: socket_test.cc \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/socket.h
store_client.o: store_client.cc \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/exported_vars.h \
 $(BASEDIR)/lib/atomic.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/variable.h \
 $(BASEDIR)/lib/http_client.h \
 $(BASEDIR)/lib/http_reply.h \
 $(BASEDIR)/lib/http_message.h \
 $(BASEDIR)/lib/http_headers.h \
 $(BASEDIR)/lib/http_request.h \
 $(BASEDIR)/lib/socket.h \
 $(BASEDIR)/lib/uri.h \
 $(BASEDIR)/lib/store_client.h \
 $(BASEDIR)/server/store_config.h \
 $(BASEDIR)/lib/hash.h
string.o: string.cc $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h
string_test.o: string_test.cc \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h
threadpool.o: threadpool.cc \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/atomic.h \
 $(BASEDIR)/lib/exported_vars.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/variable.h
timer.o: timer.cc $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/timer.h
timer_test.o: timer_test.cc $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h
trie_test.o: trie_test.cc $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/trie.h
uri.o: uri.cc $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/uri.h
uri_test.o: uri_test.cc $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/uri.h
variable.o: variable.cc $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/variable.h
variable_test.o: variable_test.cc \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/variable.h
