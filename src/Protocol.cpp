#include <Common.h>
#include <collabvm_generated.h>
#include <WebsocketServer.h>


namespace CollabVM {

	const CollabVM::Message* ReadMessageFromBuffer(std::shared_ptr<WSMessage> ws_message) {
		auto buffer = ws_message->buffer;
		auto message = CollabVM::GetMessageA(buffer.data().data());

		if(VerifyMessageBuffer(flatbuffers::Verifier((byte*)buffer.data().data(), buffer.size()))) {
			return message;
		}
	
		return nullptr;
	}

}