TARGETS=store
TESTS=
EXTRA_LIBS_store=record_log.o disk_datastore.o datastore.o $(BASEDIR)/lib/libopenstrument.a
EXTRA_DEPS_store=disk_datastore.o datastore.o record_log.o $(BASEDIR)/lib/libopenstrument.a

include $(BASEDIR)/Makefile.inc

store: store.o record_log.o disk_datastore.o datastore.o $(BASEDIR)/lib/libopenstrument.a

## DEPENDENCIES START HERE (do not remove this line)
