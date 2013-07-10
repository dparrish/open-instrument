TARGETS=src/code.google.com/p/open-instrument/proto/openinstrument_proto.pb.go bin/list bin/store bin/get bin/add
export GOPATH=go

include $(BASEDIR)/Makefile.inc

%.pb.go: $(BASEDIR)/lib/openinstrument.proto
	@echo "Generating $@ from $<"
	$(shell PATH=$(PATH):/usr/lib/go/bin protoc --go_out=src --proto_path=$(BASEDIR)/lib $(BASEDIR)/lib/openinstrument.proto)
	@mv src/openinstrument.pb.go $@


bin/list: src/list.go
	go build -o $@ $^

bin/add: src/add.go
	go build -o $@ $^

bin/store: src/store.go
	go build -o $@ $^

bin/get: src/get.go
	go build -o $@ $^
