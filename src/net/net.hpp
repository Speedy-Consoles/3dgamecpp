#ifndef NET_HPP
#define NET_HPP

enum ClientMessageType : uint8 {
	CONNECTION_REQUEST,
	ECHO_REQUEST,
};

enum ServerMessageType : uint8 {
	CONNECTION_ACCEPTED,
	CONNECTION_REJECTED,
	CONNECTION_TIMEOUT,
	CONNECTION_RESET,
	ECHO_RESPONSE,
};

enum ConnectionRejectionReason : uint8 {
	SERVER_FULL,
	DUPLICATE_ENDPOINT,
};

struct MessageHeader {
	uint8 magic[4];
	uint8 type;
} __attribute__((__packed__ ));

struct ClientMessageHeader {
	uint32 token;
} __attribute__((__packed__ ));

struct ConnectionAcceptedResponse {
	uint8 id;
	uint32 token;
} __attribute__((__packed__ ));

struct ConnectionRejectedResponse {
	uint8 reason;
} __attribute__((__packed__ ));

#endif // NET_HPP
