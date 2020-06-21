#include "WebsocketServer.h"

namespace CollabVM {

	// NOTE: This function isn't templated like ws_server_fast
	// because we explicitly use the stream_type typedef
	// .. so there doesn't need to be any other instanation.
	inline void ConfigureStream(WebsocketServer::stream_type& stream) {
		// Enable the WebSocket permessage deflate extension.
	    ws::permessage_deflate pmd;
		pmd.client_enable = true;
		pmd.server_enable = true;
		pmd.compLevel = 3;
		stream.set_option(pmd);

		stream.auto_fragment(false);
	}

	struct WSSession {
	
	private:
		// pointer to server
		// used for callbacks
		WebsocketServer* server;
	};

	struct Listener {

	private:
		// pointer to server
		WebsocketServer* server;
	};

	void WebsocketServer::Start(tcp::endpoint& ep) {
		using namespace std::placeholders;

		wsLogger.info("Starting server on ", ep.address().to_string() ,":", ep.port());


	}


	void WebsocketServer::Stop() {
	}

}
