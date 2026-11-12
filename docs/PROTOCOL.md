# LinkOS protocol

The TCP socket foundation exists, but no application wire protocol has been
implemented. LinkOS currently does not open listeners from its CLI or exchange
application messages. TCP is a byte stream; socket send/receive calls do not define
message boundaries.

Before network features are added, this document will define the versioned message
envelope, byte order, maximum frame size, message types, request correlation,
timeouts, error behavior, and authentication state transitions. Network data must
be encoded field by field; raw C structures are not a wire format. Discovery will
remain an untrusted hint and will not grant access to a peer.
