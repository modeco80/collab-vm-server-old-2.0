#include <Common.h>
#include <collabvm_generated.h>
#include <WebsocketServer.h>


namespace CollabVM::Protocol {

	// Read a message from a WebSocket message buffer.
	CollabVM::MessageT DeserializeMessage(std::shared_ptr<CollabVM::WSMessage> message);

	// Serialize message to a byte array.
	std::vector<CollabVM::byte> SerializeMessage(CollabVM::MessageT& message);

}