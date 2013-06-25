TARGETS=store
TESTS=datastore_test
TEST_DEPS=
EXTRA_LIBS_store=record_log.o disk_datastore.o indexed_store_file.o $(BASEDIR)/lib/libopeninstrument.a -lctemplate
EXTRA_DEPS_store=disk_datastore.o indexed_store_file.o record_log.o $(BASEDIR)/lib/libopeninstrument.a
EXTRA_LIBS_datastore_test=disk_datastore.o indexed_store_file.o record_log.o

include $(BASEDIR)/Makefile.inc

store: store.o record_log.o disk_datastore.o indexed_store_file.o $(BASEDIR)/lib/libopeninstrument.a

## DEPENDENCIES START HERE (do not remove this line)
datastore_test.o: datastore_test.cc \
 $(BASEDIR)/deps/gtest/include/gtest/gtest.h \
 $(BASEDIR)/deps/gtest/include/gtest/internal/gtest-internal.h \
 $(BASEDIR)/deps/gtest/include/gtest/internal/gtest-port.h \
 $(BASEDIR)/deps/gtest/include/gtest/internal/gtest-string.h \
 $(BASEDIR)/deps/gtest/include/gtest/internal/gtest-filepath.h \
 $(BASEDIR)/deps/gtest/include/gtest/internal/gtest-type-util.h \
 $(BASEDIR)/deps/gtest/include/gtest/gtest-death-test.h \
 $(BASEDIR)/deps/gtest/include/gtest/internal/gtest-death-test-internal.h \
 $(BASEDIR)/deps/gtest/include/gtest/gtest-message.h \
 $(BASEDIR)/deps/gtest/include/gtest/gtest-param-test.h \
 $(BASEDIR)/deps/gtest/include/gtest/internal/gtest-param-util.h \
 $(BASEDIR)/deps/gtest/include/gtest/internal/gtest-linked_ptr.h \
 $(BASEDIR)/deps/gtest/include/gtest/gtest-printers.h \
 $(BASEDIR)/deps/gtest/include/gtest/internal/gtest-param-util-generated.h \
 $(BASEDIR)/deps/gtest/include/gtest/gtest_prod.h \
 $(BASEDIR)/deps/gtest/include/gtest/gtest-test-part.h \
 $(BASEDIR)/deps/gtest/include/gtest/gtest-typed-test.h \
 $(BASEDIR)/deps/gtest/include/gtest/gtest_pred_impl.h \
 $(BASEDIR)/server/disk_datastore.h \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/variable.h \
 $(BASEDIR)/server/indexed_store_file.h \
 $(BASEDIR)/lib/trie.h \
 $(BASEDIR)/server/record_log.h
disk_datastore.o: disk_datastore.cc \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/server/disk_datastore.h \
 $(BASEDIR)/lib/variable.h \
 $(BASEDIR)/server/indexed_store_file.h \
 $(BASEDIR)/lib/trie.h \
 $(BASEDIR)/server/record_log.h
indexed_store_file.o: indexed_store_file.cc \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/server/indexed_store_file.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/trie.h \
 $(BASEDIR)/lib/variable.h
record_log.o: record_log.cc \
 $(BASEDIR)/lib/common.h \
 $(BASEDIR)/lib/closure.h \
 $(BASEDIR)/lib/timer.h \
 $(BASEDIR)/lib/string.h \
 $(BASEDIR)/lib/openinstrument.pb.h \
 $(BASEDIR)/lib/protobuf.h \
 $(BASEDIR)/lib/cord.h \
 $(BASEDIR)/lib/file.h \
 $(BASEDIR)/lib/trie.h \
 $(BASEDIR)/lib/variable.h \
 $(BASEDIR)/server/record_log.h
