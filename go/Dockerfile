FROM golang:1.3
MAINTAINER David Parrish <david@dparrish.com>
ENV DEBIAN_FRONTEND noninteractive

VOLUME /store

WORKDIR /opt
RUN apt-get -y update && apt-get -y install protobuf-compiler
RUN git clone http://code.google.com/p/open-instrument/
ENV GOPATH /opt/open-instrument/go
RUN go get github.com/willf/bloom
RUN go get github.com/willf/bitset
RUN go get github.com/joaojeronimo/go-crc16
RUN go get github.com/stathat/treap
RUN go get github.com/howeyc/fsnotify
RUN go get github.com/nu7hatch/gouuid
RUN go get github.com/joaojeronimo/go-crc16
RUN go get code.google.com/p/goprotobuf/proto
RUN go get code.google.com/p/goprotobuf/protoc-gen-go
RUN go install code.google.com/p/goprotobuf/protoc-gen-go

WORKDIR /opt/open-instrument/go
RUN PATH=$PATH:/opt/open-instrument/go/bin protoc --go_out=src --proto_path=/opt/open-instrument/lib /opt/open-instrument/lib/openinstrument.proto && mv src/openinstrument.pb.go /opt/open-instrument/go/src/code.google.com/p/open-instrument/proto/
RUN go build -o bin/store src/store.go
EXPOSE 8001
ENTRYPOINT ["/opt/open-instrument/go/bin/store"]

