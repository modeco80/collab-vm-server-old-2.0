#pragma once
#include "Common.h"
#include "Logger.h"

namespace CollabVM {

	struct WebsocketServer {

		typedef ws::stream<beast::tcp_stream> stream_type;
		typedef beast::flat_buffer message_type;
		typedef stream_type* handle_type;

		inline WebsocketServer(net::io_service& ioc)
			: io_service(&ioc) {
			
		}

		void Start(tcp::endpoint& ep);

		void Stop();

		// Callbacks run on WS/ASIO thread

		virtual void OnWebsocketOpen(handle_type handle) = 0;

		virtual void OnWebsocketMessage(handle_type handle, message_type message) = 0;

		virtual void OnWebsocketClose(handle_type handle) = 0;

		// get stream from a handle
		inline stream_type& GetStreamFromHandle(handle_type handle) {
			if (handle)
				return *handle;

			throw std::runtime_error("Attempted to get a stream from null handle");
		}

	protected:

		std::mutex connectionslock;

		// all connections are kept here
		// and managed by the internal code
		std::vector<stream_type> connections;

		net::io_service* io_service;

	private:
		Logger wsLogger = Logger::GetLogger("WebSocketServer");
	};

}