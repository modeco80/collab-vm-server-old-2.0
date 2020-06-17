#include "WebsocketServer.h"

namespace CollabVM {

	void WebsocketServer::Start(net::io_service& ioc, uint16 port) {
		using namespace std::placeholders;

		wsLogger.info("Starting server on 0.0.0.0:", port);

		server = new WebsocketServer::server_type();
		
		server->init_asio(&ioc);

		// clear all debugging messages from websocketpp
		server->clear_access_channels(websocketpp::log::alevel::all);
		server->clear_error_channels(websocketpp::log::elevel::all);
		
		// set handlers
		server->set_validate_handler(std::bind(&WebsocketServer::OnWebsocketValidate, this, _1));
		server->set_open_handler(std::bind(&WebsocketServer::OnWebsocketOpen, this, _1));
		server->set_message_handler(std::bind(&WebsocketServer::OnWebsocketMessage, this, _1, _2));
		server->set_close_handler(std::bind(&WebsocketServer::OnWebsocketClose, this, _1));
		
		std::error_code ec;
		server->listen(port, ec);

		if(ec) {
			wsLogger.error("Error listening: ", ec.message());
			return;
		}

		server->start_accept(ec);
		if(ec) {
			wsLogger.error("Error starting accept: ", ec.message());
		}

	}


	void WebsocketServer::Stop() {
		server->stop();
	}

}
