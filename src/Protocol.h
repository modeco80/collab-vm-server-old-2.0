#include <Common.h>
#include <collabvm_generated.h>
#include <WebsocketServer.h>


namespace CollabVM::Protocol {

	// Read a message from a WebSocket message buffer.
	CollabVM::MessageT DeserializeMessage(std::shared_ptr<CollabVM::WSMessage> message);

	// Serialize message to a byte array.
	std::vector<CollabVM::byte> SerializeMessage(CollabVM::MessageT& message);


	// INLINE PROTOCOL MESSAGE BUILDS HERE!!!

	inline static CollabVM::MessageT BuildAddUserMessage(std::string username) {
		MessageT m;
		m.which = CollabVM::MessageType::adduser;
		m.adduser = std::make_unique<AdduserOpT>();
		m.adduser->username = username;
		return m;
	}

}