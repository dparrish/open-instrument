include Makefile.inc

export CXX=g++-4.6
export INCLUDE_DIRS += -I$(BASEDIR) -I$(BASEDIR)/deps/gtest/include
export LD=g++-4.6
export LDFLAGS=-g $(LIB_DIRS)
export LIBS=-lopeninstrument -lboost_regex -lboost_system \
	-lboost_date_time-mt -lprotobuf -lrt -lboost_thread -lgflags -lglog -lpthread
export LDLIBS=$(LIBS) $(EXTRA_LIBS_$@)
export LIB_DIRS += -L$(BASEDIR)/lib -L/usr/lib
export TEST_LIBS=$(BASEDIR)/build/libgtest.a
export CXXFLAGS=\
  $(INCLUDE_DIRS) \
  -O0 \
  -Wall \
  -Werror \
  -fno-omit-frame-pointer \
  -fno-builtin-calloc \
  -fno-builtin-free \
  -fno-builtin-malloc \
  -fno-builtin-realloc \
  -fstack-protector \
  -g \
  -march=x86-64 \
  -std=c++0x \

SUBDIRS=lib server client tools go

## DEPENDENCIES START HERE (do not remove this line)
