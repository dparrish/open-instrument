TARGETS=src/code.google.com/p/open-instrument/proto/openinstrument_proto.pb.go list store get
export GOPATH=go

include $(BASEDIR)/Makefile.inc

%.pb.go: $(BASEDIR)/lib/openinstrument.proto
	@echo "Generating $@ from $<"
	$(shell PATH=$(PATH):/usr/lib/go/bin protoc --go_out=src --proto_path=$(BASEDIR)/lib $(BASEDIR)/lib/openinstrument.proto)
	@mv src/openinstrument.pb.go $@

.PHONY: list add store get

list: src/list.go
	go build -o $@ $<

add: src/add.go
	go build -o $@ $<

store: src/store.go
	go build -o $@ $<

get: src/get.go
	go build -o $@ $^
