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
	@$(LD) -o $@ $< $(EXTRA_LIBS_$@) $(LDFLAGS) $(LIBS) $(EXTRA_LIBS)

%.o: %.cc
	@echo "Compiling $@"
	@$(CXX) -o $@ $< -c $(CXXFLAGS)

%_test: %_test.o $(TEST_DEPS)
	@echo "Linking test $@"
	@$(LD) -o $@ $< $(LDFLAGS) $(LIBS) $(TEST_LIBS) $(EXTRA_LIBS_$@)
	@echo "Running $@"
	@if test -f /`which valgrind 2>/dev/null`; then \
		valgrind -q ./$@ || (rm -f $@ && exit 1) \
	else \
		./$@ || (rm -f $@ && exit 1) \
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

