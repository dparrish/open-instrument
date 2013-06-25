# vim:noet sw=8

all: subdirs_all $(TARGETS)

ifeq (0,${MAKELEVEL})
export BASEDIR := $(shell pwd)
endif

ifeq ($(MAKE),)
  MAKE := make
endif

%: %.o $(EXTRA_DEPS) $(EXTRA_DEPS_$@)
	@echo "Linking $@"
	$(LD) -o $@ $< $(EXTRA_LIBS_$@) $(LDFLAGS) $(LIBS) $(EXTRA_LIBS)

%.o: %.cc
	@echo "Compiling $@"
	@$(CXX) -o $@ $< -c $(CXXFLAGS)

%_test: %_test.o $(TEST_DEPS) $(TEST_LIBS) $(EXTRA_DEPS_$@) $(EXTRA_LIBS_$@)
	@echo "Linking test $@"
	@$(LD) -o $@ $< $(EXTRA_LIBS_$@) $(LDFLAGS) $(LIBS) $(TEST_LIBS)
	@echo "Running $@"
	@if test -f /`which valgrindx 2>/dev/null`; then \
		valgrind -q ./$@ $(TEST_ARGS) || (rm -f $@ && exit 1) \
	else \
		./$@ $(TEST_ARGS) || (rm -f $@ && exit 1) \
	fi

%.a:
	@echo "Creating library $@"
	@ar rcs $@ $^

%_pb2.py: %.proto
	@echo "Generating $@ from $<"
	@protoc -I`dirname $<` --python_out=`dirname $@` `basename $<`

%.pb.h: %.proto
	@echo "Generating $@ from $<"
	@protoc -I. --cpp_out=. `basename $<`

%.pb.cc: %.proto
	@echo "Generating $@ from $<"
	@protoc -I. --cpp_out=. `basename $<`

clean: subdirs_clean
	@rm -f *.o $(CLEAN_TARGETS) $(TARGETS) $(TESTS)

lint: subdirs_lint
	@-cpplint.py *.cc *.h

test: $(TESTS) subdirs_test

depend: subdirs_depend
	@head -n+`grep -n "^## DEPENDENCIES START HERE" Makefile | sed 's/:.*//'` Makefile > Makefile.new
	@-$(CXX) $(CXXFLAGS) -MM *.cc 2>/dev/null | sed 's!$(BASEDIR)/!$$(BASEDIR)/!' >> Makefile.new && mv Makefile.new Makefile
	@rm -f Makefile.new

.PHONY:
subdirs_all:
	@for DIR in $(SUBDIRS); do $(MAKE) -C $$DIR all || exit $?; done

.PHONY:
subdirs_clean:
	@for DIR in $(SUBDIRS); do $(MAKE) -C $$DIR clean || exit $?; done

.PHONY:
subdirs_test:
	@for DIR in $(SUBDIRS); do $(MAKE) -C $$DIR test || exit $?; done

.PHONY:
subdirs_depend:
	@for DIR in $(SUBDIRS); do $(MAKE) -C $$DIR depend || exit $?; done

.PHONY:
subdirs_lint:
	@for DIR in $(SUBDIRS); do $(MAKE) -C $$DIR lint || exit $?; done

GTEST_DIR=$(BASEDIR)/deps/gtest
$(BASEDIR)/build/libgtest.a: $(GTEST_DIR)/src/gtest-all.cc
	@echo "Generating gtest"
	@$(CXX) -o $(GTEST_DIR)/src/gtest-all.o -c -I$(GTEST_DIR)/include -I$(GTEST_DIR) $(GTEST_DIR)/src/gtest-all.cc
	@ar -rv $@ $(GTEST_DIR)/src/gtest-all.o
