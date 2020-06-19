#include "WebsocketServer.h"

namespace CollabVM {

	void WebsocketServer::Start(uint16 port) {
		using namespace std::placeholders;

		wsLogger.info("Starting server on 0.0.0.0:", port);

		ws_server = new WebsocketServer::server_type();
		
		ws_server->init_asio(io_service);

		// clear all debugging messages from websocketpp
		ws_server->clear_access_channels(websocketpp::log::alevel::all);
		ws_server->clear_error_channels(websocketpp::log::elevel::all);
		
		// set handlers
		ws_server->set_validate_handler(std::bind(&WebsocketServer::OnWebsocketValidate, this, _1));
		ws_server->set_open_handler(std::bind(&WebsocketServer::OnWebsocketOpen, this, _1));
		ws_server->set_message_handler(std::bind(&WebsocketServer::OnWebsocketMessage, this, _1, _2));
		ws_server->set_close_handler(std::bind(&WebsocketServer::OnWebsocketClose, this, _1));
		
		std::error_code ec;
		ws_server->listen(port, ec);

		if(ec) {
			wsLogger.error("Error listening: ", ec.message());
			return;
		}

		ws_server->start_accept(ec);
		if(ec) {
			wsLogger.error("Error starting accept: ", ec.message());
			return;
		}

	}


	void WebsocketServer::Stop() {
		ws_server->stop();
	}

}
