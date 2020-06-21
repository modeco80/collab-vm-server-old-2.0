#pragma once
#include "Common.h"
#include "Logger.h"

namespace CollabVM {

	struct WebsocketServer {

		typedef websocketpp::server<websocketpp::config::asio> server_type;
		typedef websocketpp::connection_hdl handle_type;
		typedef server_type::message_ptr message_type;
		typedef server_type::connection_ptr connection_type;

		inline WebsocketServer(net::io_service& ioc)
			: io_service(&ioc) {
			
		}

		void Start(uint16 port);

		void Stop();

		virtual bool OnWebsocketValidate(handle_type handle) = 0;
		
		virtual void OnWebsocketOpen(handle_type handle) = 0;

		virtual void OnWebsocketMessage(handle_type handle, message_type message) = 0;

		virtual void OnWebsocketClose(handle_type handle) = 0;

	protected:

		server_type* ws_server;

		net::io_service* io_service;

	private:
		Logger wsLogger = Logger::GetLogger("WebSocketServer");
	};

}