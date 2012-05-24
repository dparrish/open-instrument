include Makefile.inc

export CXX=g++
export INCLUDE_DIRS += -I$(BASEDIR)
export LD=g++
export LDFLAGS=-g $(LIB_DIRS)
export LIBS=-lopeninstrument -lboost_regex -lboost_system \
	-lboost_date_time-mt -lprotobuf -lrt -lboost_thread -lgflags -lglog 
export LDLIBS=$(LIBS) $(EXTRA_LIBS_$@)
export LIB_DIRS += -L$(BASEDIR)/lib -L/usr/lib
export TEST_LIBS=-lgtest
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
  -march=native \
  -std=c++0x \

SUBDIRS=lib server client tools

## DEPENDENCIES START HERE (do not remove this line)
