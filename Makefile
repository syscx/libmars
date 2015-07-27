ifeq ($(CC),)
CC=gcc
endif
ifeq ($(STRIP),)
STRIP=strip
endif
ifeq ($(MARS_SRC_DIR),)
MARS_SRC_DIR=.
endif
ifeq ($(MARS_BIN_DIR),)
MARS_BIN_DIR=bin
endif
ifeq ($(MARS_TARGET_BINARY),)
MARS_TARGET_BINARY=$(MARS_BIN_DIR)/libmars.so
endif
ifeq ($(MARS_CFLAGS),)
MARS_CFLAGS:=$(CFLAGS) $(shell pkg-config --cflags dbus-1 gobject-2.0) -DCASSANDRA_KEYSPACE=\"wsg\"
endif
ifeq ($(MARS_LDFLAGS),)
MARS_LDFLAGS:=$(LDFLAGS) $(shell pkg-config --libs dbus-1 gobject-2.0)
endif

CFLAGS=$(MARS_CFLAGS) -MMD -fPIC -DPIC -g -Wall -Werror -D_GNU_SOURCE -D_REENTRANT 
LDFLAGS=$(MARS_LDFLAGS) 
VPATH = \
        $(MARS_SRC_DIR)/include \
		$(MARS_SRC_DIR)/3rdparty \
		$(MARS_SRC_DIR)/3rdparty/c_glib/src \
		$(MARS_SRC_DIR)/3rdparty/c_glib/src/thrift \
		$(MARS_SRC_DIR)/3rdparty/c_glib/src/thrift/processor \
		$(MARS_SRC_DIR)/3rdparty/c_glib/src/thrift/protocol \
		$(MARS_SRC_DIR)/3rdparty/c_glib/src/thrift/transport \
		$(MARS_SRC_DIR)/3rdparty/gen-c_glib \
        $(MARS_SRC_DIR)/src

CFLAGS := $(addprefix -I,$(VPATH)) $(CFLAGS)
LIBS=-L$(MARS_BIN_DIR) $(MARS_LDFLAGS)

MARS_OBJ_FILES := \
        $(MARS_BIN_DIR)/mars.o

THRIFT_OBJ_FILES := \
	$(MARS_BIN_DIR)/cassandra_types.o \
	$(MARS_BIN_DIR)/cassandra.o \
	$(MARS_BIN_DIR)/thrift.o \
	$(MARS_BIN_DIR)/thrift_struct.o \
	$(MARS_BIN_DIR)/thrift_application_exception.o \
	$(MARS_BIN_DIR)/thrift_processor.o \
	$(MARS_BIN_DIR)/thrift_binary_protocol_factory.o \
	$(MARS_BIN_DIR)/thrift_binary_protocol.o \
	$(MARS_BIN_DIR)/thrift_protocol_factory.o \
	$(MARS_BIN_DIR)/thrift_protocol.o \
	$(MARS_BIN_DIR)/thrift_buffered_transport.o \
	$(MARS_BIN_DIR)/thrift_framed_transport.o \
	$(MARS_BIN_DIR)/thrift_memory_buffer.o \
	$(MARS_BIN_DIR)/thrift_socket.o \
	$(MARS_BIN_DIR)/thrift_transport_factory.o \
	$(MARS_BIN_DIR)/thrift_transport.o



all:$(MARS_TARGET_BINARY) test
$(MARS_TARGET_BINARY): $(MARS_OBJ_FILES) $(THRIFT_OBJ_FILES) $(MARS_BIN_DIR)/mars.o
	$(CC) -o $@ $^ $(LIBS) -shared 

$(MARS_BIN_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(MARS_TARGET_BINARY) $(MARS_BIN_DIR)/mars.o
	$(CC) $(CFLAGS) -o $(MARS_BIN_DIR)/test demo/test.c $(LIBS) -lmars -lmemcached
	$(CC) $(CFLAGS) -o $(MARS_BIN_DIR)/udpreceiver demo/testbuildudp.c $(LIBS) -lmars -lmemcached
	$(CC) $(CFLAGS) -o $(MARS_BIN_DIR)/serviceup demo/serviceup.c $(LIBS) -lmars -lmemcached

install: all
	mkdir -p ${DESTDIR}/include
	install $(MARS_SRC_DIR)/include/*.h ${DESTDIR}/include
	install $(MARS_TARGET_BINARY) ${DESTDIR}/lib	
devinstall:
	install $(MARS_TARGET_BINARY) ${DESTDIR}/lib

clean:
	rm $(MARS_BIN_DIR)/*

-include $(MARS_BIN_DIR)/*.d

