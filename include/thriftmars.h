#include <protocol/thrift_binary_protocol.h>
#include <transport/thrift_framed_transport.h>
#include <transport/thrift_socket.h>

#include "cassandra.h"

typedef struct thrift_connection {
    ThriftSocket *tsocket;
    ThriftTransport *transport;
    ThriftProtocol *protocol;
    CassandraClient *client;
    CassandraIf *service;
} thriftConnection;
int thriftConnectionReset(thriftConnection *thrift);
int thriftConnectionSetup(thriftConnection *thrift);
void thriftConnectionTearDown(thriftConnection *thrift);
