#include <Common.h>

#ifdef _WIN32
// Windows headers are garbage, 
// and clobber Flatbuffers functions...
// Moral of the story,
// this is why the A/W naming system shouldn't have ever existed,
// and this is why Windows headers are regarded as garbage.
#undef GetMessage
#endif

#include <collabvm_generated.h>
#include <WebsocketServer.h>

namespace CollabVM::Protocol {

	MessageT DeserializeMessage(std::shared_ptr<WSMessage> ws_message) {
		MessageT message;
		auto buffer = ws_message->buffer.data().data();

		GetMessage(buffer)->UnPackTo(&message);

		return message;
	}

	std::vector<byte> SerializeMessage(MessageT& message) {
		std::vector<byte> managed;
		flatbuffers::FlatBufferBuilder builder(1024);

		// TODO: Verify that only one message component exists.
		// More than one = error out, since that's an invalid state.
		builder.Finish(Message::Pack(builder, &message));

		// Copy the flatbuffer buffer into another buffer that we manage
		managed.resize(builder.GetSize());
		memcpy((byte*)&managed[0], (byte*)builder.GetBufferPointer(), builder.GetSize());

		// Once buffer builder goes out of scope the memory is deleted
		// so return our managed byte array instead
		return managed;
	}

}