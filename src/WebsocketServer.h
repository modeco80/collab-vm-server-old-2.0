#pragma once
#include "Common.h"
#include "Logger.h"

namespace CollabVM {

	struct WSSession;
	struct HTTPSession;
	struct Listener;

	// wrapper over flat_buffer that stores a bit more info about the message
	// TODO: This should be passed by reference, and message should be a ref as well.
	struct WSMessage {

		~WSMessage() {
			message.reserve(0);
		}

		bool binary;
		beast::flat_buffer message;
	};

	struct WebsocketServer {
		friend struct WSSession;
		friend struct HTTPSession;
		friend struct Listener;

		typedef ws::stream<beast::tcp_stream> stream_type;

		// Message type.
		typedef WSMessage message_type;

		// handle type
		// The handle type is a handle to the session.
		typedef WSSession* handle_type;
		typedef HTTPSession* http_handle_type;

		inline WebsocketServer(net::io_service& ioc)
			: io_service(&ioc) {
			
		}

		void Start(tcp::endpoint& ep);

		void Stop();

		// Callbacks run where the io service runs
		
		virtual bool OnVerify(handle_type handle) = 0;

		// TODO: OnHttp?

		virtual void OnOpen(handle_type handle) = 0;

		virtual void OnMessage(handle_type handle, message_type& message) = 0;

		virtual void OnClose(handle_type handle) = 0;	
		
		// get session from a handle (with sanity checking)
		inline static WSSession& GetSessionFromHandle(handle_type handle) {
			if (handle)
				return *handle;

			throw std::runtime_error("Attempted to get a session from null handle");
		}


		// broadcast to every connected person
		void Broadcast(message_type& message);

	protected:

		net::io_service* io_service;

		// these are only protected because they're accessed by Listener
		// and WSSession

		// lock on sessions
		std::mutex SessionsLock;

		// all running WSsessions
		std::vector<WSSession*> sessions;

		// All running HTTP sessions (type erased)
		std::vector<HTTPSession*> http_sessions;

		// listener
		Listener* listener;
		
		// internal callback, 
		// called when a session requests close
		// frees said session
		void OnSessionClose(WSSession* session);

		// internal callback, 
		// called when a session requests close
		// frees said session
		void OnHttpClose(HTTPSession* session);

	private:
		Logger wsLogger = Logger::GetLogger("WebSocketServer");
	};

	// WebSocket session
	struct WSSession {
	
		explicit WSSession(tcp::socket&& socket, WebsocketServer* server, http::token_list& subprotocols) 
		 : stream(std::move(socket)), server(server), subprotocols(subprotocols) {
		
		}

		void Run(http::request<http::string_body> req);

		void SessionStart(http::request<http::string_body> req);

		// Get the stream this session object is managing
		WebsocketServer::stream_type& GetStream();

		void OnAccept(beast::error_code& ec);

		void Read();

		void OnRead(beast::error_code ec, std::size_t bytes_transferred);

		// send a message
		void Send(WebsocketServer::message_type& message);

		void OnSend(beast::error_code ec, std::size_t bytes_transferred);

		// Close the session (this handles deleting memory as well)
		inline void CloseSession() {
			buf.clear();
			server->OnSessionClose(this);
		}

		// Close connection and session
		inline void Close(ws::close_reason reason = ws::close_reason(ws::close_code::normal)) {
			stream.async_close(reason, [&](beast::error_code ec) {
				if (ec)
					return;

				CloseSession();
			});
		}

		// Get address
		inline net::ip::address GetAddress() {
			return stream.next_layer().socket().remote_endpoint().address();
		}

		inline http::token_list GetSubprotocols() {
			return subprotocols;
		}

		inline void SetSubprotocol(std::string protocol) {
			subprotocol = protocol;
		}

		// All subprotocols
		http::token_list& subprotocols;

		// subprotocol in use
		std::string subprotocol;

	private:
		// Handle to the WebsocketServer
		// that created us (by creating the Listener...)
		// Used to call callbacks.
		WebsocketServer* server;

		// this session's stream
		WebsocketServer::stream_type stream;

		// message. A reference to this is given out
		WSMessage message;

		// buffer for this session
		beast::flat_buffer buf;

		Logger logger = Logger::GetLogger("WebsocketSession");
	};

}