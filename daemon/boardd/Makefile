SRCROOT=	../..
.include "$(SRCROOT)/pttbbs.mk"

.SUFFIXES: .proto .pb.cc .pb.h .grpc.pb.cc .grpc.pb.h

UTILDIR=	$(SRCROOT)/util
UTILOBJ=	$(UTILDIR)/util_var.o

COMMONFLAGS= -I$(SRCROOT) -I. -Wno-unused-parameter
CXXFLAGS+=	-std=c++17 $(COMMONFLAGS)
CFLAGS+=	$(LIBEVENT_CFLAGS) $(GFLAGS_CFLAGS) \
		$(GRPCPP_CFLAGS) $(PROTOBUF_CFLAGS) $(COMMONFLAGS)
LDFLAGS+=	$(LIBEVENT_LIBS_L) $(GFLAGS_LIBS_L) \
		$(GRPCPP_LIBS_L) $(PROTOBUF_LIBS_L)

LDADD:=  $(UTILOBJ) \
	$(SRCROOT)/common/bbs/libcmbbs.a \
	$(SRCROOT)/common/sys/libcmsys.a \
	$(SRCROOT)/common/osdep/libosdep.a \
	$(LIBEVENT_LIBS_l) $(GFLAGS_LIBS_l) \
	$(GRPCPP_LIBS_l) $(PROTOBUF_LIBS_l) \
	$(LDADD)

GEN_SRCS=board.pb.cc board.grpc.pb.cc
GEN_HDRS=board.pb.h board.grpc.pb.h
GEN_OBJS=board.pb.o board.grpc.pb.o

BOARDC_OBJS=	boardc.o
BOARDD_OBJS=	boardd.o convert.o article.o strings.o board_service_impl.o bbs++.o evbuffer.o

$(GEN_SRCS) $(GEN_HDRS): board.proto
	protoc -I. --cpp_out=. --grpc_out=. --plugin=protoc-gen-grpc=/usr/bin/grpc_cpp_plugin board.proto

boardc: $(GEN_OBJS) $(BOARDC_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(GEN_OBJS) $(BOARDC_OBJS) $(LDFLAGS) $(LDADD)

boardd: $(GEN_OBJS) $(BOARDD_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(GEN_OBJS) $(BOARDD_OBJS) $(LDFLAGS) $(LDADD)

clean:
	rm -f $(BOARDC_OBJS) $(BOARDD_OBJS) $(GEN_SRCS) $(GEN_HDRS) $(GEN_OBJS) boardc boardd

all: boardc boardd
