include $(BASEDIR)/Makefile.inc
TARGETS=openinstrument_pb2.py

all: $(TARGETS)

openinstrument_pb2.py: ../../lib/openinstrument.proto
	@echo "Generating $@ from $<"
	protoc -I`dirname $<` --python_out=. $<

## DEPENDENCIES START HERE (do not remove this line)
