TARGETS=libopeninstrument.a
TESTS=string_test timer_test uri_test exported_vars_test cord_test socket_test protobuf_test counter_test
OBJS=common.o string.o uri.o exported_vars.o openinstrument.pb.o mysql_builder.o base64.o protobuf.o cord.o socket.o \
     http_message.o http_server.o http_client.o http_reply.o http_request.o store_client.o http_static_dir.o closure.o \
     mime_types.o file.o counter.o timer.o
TEST_LIBS+=libopeninstrument.a
TEST_DEPS=libopeninstrument.a
CLEAN_TARGETS=openinstrument.pb.cc openinstrument.pb.h
SUBDIRS=
EXTRA_LIBS_mysql_builder_test=-lmysql++

include $(BASEDIR)/Makefile.inc

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
counter.o: counter.cc $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
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
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/openinstrument.pb.h
exported_vars.o: exported_vars.cc \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/exported_vars.h \
 $(BASEDIR)/lib/atomic.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/store_client.h \
 $(BASEDIR)/lib/socket.h
exported_vars_test.o: exported_vars_test.cc \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/exported_vars.h \
 $(BASEDIR)/lib/atomic.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/timer.h
file.o: file.cc $(BASEDIR)/lib/common.h \
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
http_server.o: http_server.cc \
 $(BASEDIR)/lib/http_server.h \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/exported_vars.h \
 $(BASEDIR)/lib/atomic.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/timer.h \
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
 $(BASEDIR)/lib/threadpool.h \
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
 $(BASEDIR)/lib/request_handler.h \
 $(BASEDIR)/lib/http_static_dir.h \
 $(BASEDIR)/lib/mime_types.h
mime_types.o: mime_types.cc \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/mime_types.h
mysql_builder.o: mysql_builder.cc /usr/include/mysql++/mysql++.h \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/mysql_builder.h
mysql_builder_test.o: mysql_builder_test.cc \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/mysql_builder.h \
 /usr/include/mysql++/mysql++.h /usr/include/mysql++/connection.h \
 /usr/include/mysql++/common.h /usr/include/mysql/mysql_version.h \
 /usr/include/mysql/mysql.h /usr/include/mysql/mysql_version.h \
 /usr/include/mysql/mysql_com.h /usr/include/mysql/mysql_time.h \
 /usr/include/mysql/my_list.h /usr/include/mysql/typelib.h \
 /usr/include/mysql/my_alloc.h /usr/include/mysql++/noexceptions.h \
 /usr/include/mysql++/options.h /usr/include/mysql++/tcp_connection.h \
 /usr/include/mysql++/uds_connection.h \
 /usr/include/mysql++/wnp_connection.h /usr/include/mysql++/cpool.h \
 /usr/include/mysql++/beemutex.h /usr/include/mysql++/exceptions.h \
 /usr/include/mysql++/query.h /usr/include/mysql++/qparms.h \
 /usr/include/mysql++/stadapter.h /usr/include/mysql++/datetime.h \
 /usr/include/mysql++/comparable.h /usr/include/mysql++/null.h \
 /usr/include/mysql++/sql_buffer.h /usr/include/mysql++/refcounted.h \
 /usr/include/mysql++/type_info.h /usr/include/mysql++/sql_types.h \
 /usr/include/mysql++/tiny_int.h /usr/include/mysql++/querydef.h \
 /usr/include/mysql++/result.h /usr/include/mysql++/field.h \
 /usr/include/mysql++/field_names.h /usr/include/mysql++/field_types.h \
 /usr/include/mysql++/row.h /usr/include/mysql++/mystring.h \
 /usr/include/mysql++/vallist.h /usr/include/mysql++/manip.h \
 /usr/include/mysql++/myset.h /usr/include/mysql++/stream2string.h \
 /usr/include/mysql++/transaction.h
openinstrument.pb.o: openinstrument.pb.cc openinstrument.pb.h
protobuf.o: protobuf.cc $(BASEDIR)/lib/base64.h \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/file.h
protobuf_test.o: protobuf_test.cc \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/timer.h
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
store_client.o: store_client.cc \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/threadpool.h \
 $(BASEDIR)/lib/exported_vars.h \
 $(BASEDIR)/lib/atomic.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/http_client.h \
 $(BASEDIR)/lib/http_reply.h \
 $(BASEDIR)/lib/http_message.h \
 $(BASEDIR)/lib/http_headers.h \
 $(BASEDIR)/lib/http_request.h \
 $(BASEDIR)/lib/socket.h \
 $(BASEDIR)/lib/uri.h \
 $(BASEDIR)/lib/store_client.h
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
