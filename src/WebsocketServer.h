#pragma once
#include "Common.h"
#include "Logger.h"

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

namespace CollabVM {

	struct WebsocketServer {

		typedef websocketpp::server<websocketpp::config::asio> server_type;
		typedef websocketpp::connection_hdl handle_type;
		typedef server_type::message_ptr message_type;

		void Start(net::io_service& ioc, uint16 port);

		void Stop();

		virtual bool OnWebsocketValidate(handle_type handle) = 0;
		
		virtual void OnWebsocketOpen(handle_type handle) = 0;

		virtual void OnWebsocketMessage(handle_type handle, message_type message) = 0;

		virtual void OnWebsocketClose(handle_type handle) = 0;


	protected:

		server_type* server;
		std::thread* server_thread;

	private:
		Logger wsLogger = Logger::GetLogger("WebSocketServer");
	};

}