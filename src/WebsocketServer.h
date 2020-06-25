#pragma once
#include "Common.h"
#include "Logger.h"


namespace CollabVM {

	struct WSSession;
	struct HTTPSession;
	struct Listener;

	// basic wrapper over beast::flat_buffer
	struct WSMessage {
		inline WSMessage(bool binary, beast::flat_buffer message)
			: binary(binary), buffer(message) {}

		bool binary;
		beast::flat_buffer buffer;
	};

	struct WebsocketServer : public std::enable_shared_from_this<WebsocketServer> {
		friend struct WSSession;
		friend struct HTTPSession;
		friend struct Listener;

		// message type
		typedef std::shared_ptr<WSMessage> message_type;

		// handle type
		// The handle type is a shared_ptr to the session.
		typedef std::shared_ptr<WSSession> handle_type;

		// TODO
		typedef std::shared_ptr<HTTPSession> http_handle_type;

		inline WebsocketServer(net::io_service& ioc)
			: io_service(&ioc) {
			
		}

		void Start(tcp::endpoint& ep);

		void Stop();

		// Callbacks run where the io service runs
		
		virtual bool OnVerify(handle_type handle) = 0;

		// TODO: OnHttp?

		virtual void OnOpen(handle_type handle) = 0;

		virtual void OnMessage(handle_type handle, message_type message) = 0;

		virtual void OnClose(handle_type handle) = 0;

	protected:

		net::io_service* io_service;

		// listener
		std::shared_ptr<Listener> listener;
	private:
		Logger wsLogger = Logger::GetLogger("WebSocketServer");
	};

	// WebSocket session
	struct WSSession : public std::enable_shared_from_this<WSSession> {
	
		explicit WSSession(tcp::socket&& socket, WebsocketServer* server, http::token_list& subprotocols) 
		 : stream(std::move(socket)), server(server), subprotocols(subprotocols) {
		
		}

		void Run(http::request<http::string_body> req);

		void SessionStart(http::request<http::string_body> req);

		// Get the stream this session object is managing
		ws::stream<beast::tcp_stream>& GetStream();

		void OnAccept(beast::error_code ec);

		void Read();

		void OnRead(beast::error_code ec, std::size_t bytes_transferred);

		// send a message
		void Send(WebsocketServer::message_type message);

		void OnSend(beast::error_code ec, std::size_t bytes_transferred);

		// Close connection and session
		inline void Close(ws::close_reason reason = ws::close_reason(ws::close_code::normal)) {
			stream.async_close(reason, [&](beast::error_code ec) {
				if (ec)
					return;
			});
		}

		// Get address
		inline net::ip::address GetAddress() {
			return stream.next_layer().socket().remote_endpoint().address();
		}

		inline http::token_list GetSubprotocols() {
			return subprotocols;
		}		

		inline std::string GetHandshakeSubprotocol() {
			return subprotocol;
		}

		inline void SetSubprotocol(std::string protocol) {
			subprotocol = protocol;
		}

		// All subprotocols, valid until OnVerify() returns.
		http::token_list& subprotocols;

		// Currently-handshaked subprotocol
		std::string subprotocol;

	private:
		// Handle to the WebsocketServer
		// that created us (by creating the Listener...)
		// Used to call callbacks.
		WebsocketServer* server;

		// this session's stream
		ws::stream<beast::tcp_stream> stream;

		// buffer for this session
		beast::flat_buffer buf;

		Logger logger = Logger::GetLogger("WebsocketSession");
	};

}