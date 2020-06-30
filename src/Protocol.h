#include <Common.h>
#include <collabvm_generated.h>
#include <WebsocketServer.h>


namespace CollabVM {

	// Read a message from a WebSocket message buffer.
	const CollabVM::Message* ReadMessageFromBuffer(std::shared_ptr<WSMessage> message);

}