#include <Common.h>
#include "VNCClient.h"

#ifdef _MSC_VER
#define strdup _strdup
#endif

namespace CollabVM {

	// Key for user data to store the pointer to the active VNC client.
	// This is used so that all of the libvncserver code can access the VNCClient object it's tied to.
	// (I'm waiting for people to call me cheeky :P)
	const static uint32 VNCCLIENT_KEY = 0x796C694C;

	VNCClient::~VNCClient() {
		if(client)
			rfbClientCleanup(client);
	}

	void VNCClient::Connect() {
		// Connect() simply starts the thread.
		vnc_thread = std::thread(&VNCClient::ClientThread, shared_from_this());
		vnc_thread.detach();
	}

	
	void VNCClient::SetOptions(VNCClientOptions& new_options) {
		std::lock_guard<std::mutex> l(state_lock);
		options = new_options;
	}

	void VNCClient::ClientThread() {

		// set state
		auto SetState = [&](State newState) {
			std::lock_guard<std::mutex> l(state_lock);
			current_state = newState;
		};

		SetState(State::ConnectingToServer);

		// get a 32bpp client & set the client data
		client = rfbGetClient(8, 3, 4);
		rfbClientSetClientData(client, (void*)&VNCCLIENT_KEY, this);

		client->serverHost = strdup(options.hostname.data());
		client->serverPort = options.port;

		if(options.register_qemu_audio) {
			logger.info("Registering QEMU Audio extension");
		}

		if(rfbInitClient(client, 0, NULL)) {
			// Initalization succedded

			client->canHandleNewFBSize = true;
			// TODO: actually do stuff :P
		} else {
			// client initalization failed
			SetState(State::Disconnected);
			return;
		}
	}
}