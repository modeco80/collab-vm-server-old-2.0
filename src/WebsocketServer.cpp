#include "WebsocketServer.h"

using namespace std::placeholders;

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

	// WSSession

	void WSSession::Run(http::request<http::string_body> req) {
		SessionStart(req);
	}

	void WSSession::SessionStart(http::request<http::string_body> req) {
		ConfigureStream(stream);

		stream.set_option(ws::stream_base::timeout::suggested(beast::role_type::server));

		stream.set_option(ws::stream_base::decorator([&](ws::response_type& res) {
			// Set the subprotocol if we need to
			if(!subprotocol.empty())
				res.set(http::field::sec_websocket_protocol, subprotocol);

			
			// TODO(maybe): Add version information
			res.set(http::field::server, "collab-vm-server/2.0");
		}));

		stream.async_accept(req, beast::bind_front_handler(&WSSession::OnAccept, this));
	}

	WebsocketServer::stream_type& WSSession::GetStream() {
		return stream;
	}

	void WSSession::OnAccept(beast::error_code ec) {
		if(ec)
			return;

		logger.verbose("Accepted session for ", GetAddress().to_string());

		server->OnOpen(this);

		Read();
	}

	void WSSession::Read() {
		stream.async_read(buf, beast::bind_front_handler(&WSSession::OnRead, this));
	}

	void WSSession::OnRead(beast::error_code ec, std::size_t bytes_transferred) {
		if(ec == ws::error::closed) {
			CloseSession();
			return;
		}

		if(ec)
			return;

		// make message
		message = WSMessage { stream.got_text(), buf };

		server->OnMessage(this, message);

		buf.consume(buf.size());

		Read();
	}

	void WSSession::Send(WebsocketServer::message_type& message) {
		if (message.binary)
			stream.binary(true);
		else
			stream.text(true);

		stream.async_write(message.message.data(), beast::bind_front_handler(&WSSession::OnSend, this));
	}

	void WSSession::OnSend(beast::error_code ec, std::size_t bytes_transferred) {
		if(ec == ws::error::closed)
			CloseSession();
	}

	struct HTTPSession {
		
		explicit HTTPSession(WebsocketServer* server, tcp::socket&& socket)
			: server(server), stream(std::move(socket)) {
		
		}
	
		void Run() {
			// Ensure we're on the right strand
			net::dispatch(stream.get_executor(), beast::bind_front_handler(&HTTPSession::Read, this));
		}

		void Close() {
			beast::error_code ec;
			stream.socket().shutdown(tcp::socket::shutdown_send, ec);

			server->OnHttpClose(this);
		}

	private:

		void Read() {
			req = {};

			http::async_read(stream, request_buffer, req, beast::bind_front_handler(&HTTPSession::OnRead, this));
		}

		void OnRead(beast::error_code ec, size_t bytes_written) {
			if (ec == http::error::end_of_stream)
				Close();

			if (ec)
				return;

			if (ws::is_upgrade(req)) {

				auto subprotocols = http::token_list(req[http::field::sec_websocket_protocol]);

				server->sessions.push_back(new WSSession(std::move(stream.socket()), server, subprotocols));

				// Hold pointer to session
				auto session = server->sessions.back();

				// At this point, there is no actual Websocket connection ready quite yet,
				// but we can verify if the user has the right subprotocol and/or any limits.
				//
				// If the verify callback returns false, then we should close the session
				// and wait for another accept.
				if (!server->OnVerify(session)) {
					session->CloseSession();
					return;
				}

				session->Run(req);
			} else {
				// TODO: implement a basic static file host. (NOTE: make sure to allow chunk/resume stuff?

				logger.info(stream.socket().remote_endpoint().address().to_string(), " Requested ", req.target());
				http::response<http::string_body> res;

				res.result(http::status::ok);
				res.body() = "CollabVM 2.0";

				http::async_write(stream.socket(), res, [&](beast::error_code ec, std::size_t) {
					if (ec)
						return;

					// Close regardless of the header. We don't care (yet)
					Close();
				});
			}

			//Read();
		}
		
		// Handle to the WebsocketServer
		// that created us.
		WebsocketServer* server;
		
		beast::tcp_stream stream;

		// HTTP request buffer
		beast::flat_buffer request_buffer;

		http::request<http::string_body> req;

		Logger logger = Logger::GetLogger("HTTP");
	};

	struct Listener {

		Listener(net::io_service& ioc, tcp::endpoint& ep, WebsocketServer* srv)
			: ioc(ioc),
			acceptor(ioc), server(srv) {
			beast::error_code ec;

			acceptor.open(ep.protocol(), ec);
			acceptor.set_option(net::socket_base::reuse_address(true), ec);
			acceptor.bind(ep, ec);
			acceptor.listen(net::socket_base::max_listen_connections, ec);
		}

		void Run() {
			DoAccept();
		}

	private:

		void DoAccept() {
			acceptor.async_accept(net::make_strand(ioc), beast::bind_front_handler(&Listener::OnAccept, this));
		}


		void OnAccept(beast::error_code ec, tcp::socket socket_) {
			if(ec)
				return;
			
			std::lock_guard<std::mutex> lock(server->SessionsLock);
			
			server->http_sessions.push_back(new HTTPSession(server, std::move(socket_)));
			auto session = server->http_sessions.back();

			session->Run();

			// accept another connection after we start the HTTP session
			DoAccept();

		}

		// pointer to server
		WebsocketServer* server;

		tcp::acceptor acceptor;
		net::io_service& ioc;
	};

	// WebsocketServer

	void WebsocketServer::Start(tcp::endpoint& ep) {
		using namespace std::placeholders;

		wsLogger.info("Starting server on ", ep.address().to_string() ,":", ep.port());

		listener = new Listener(*io_service, ep, this);
		listener->Run();
		io_service->run();
	}


	void WebsocketServer::Stop() {
		if (listener)
			delete listener;
	}

	void WebsocketServer::Broadcast(message_type& message) {
		std::lock_guard<std::mutex> lock(SessionsLock);

		for(auto session : sessions)
			session->Send(message);
	}

	void WebsocketServer::OnSessionClose(WSSession* session) {
		if(!session)
			return;

		std::lock_guard<std::mutex> lock(SessionsLock);

		// find session in list
		auto FindSession = [&]() {
			for (auto it = sessions.begin(); it != sessions.end(); ++it)
				if (*it == session)
					return it;

			return sessions.end();
		};

		auto it = FindSession();

		// call close handler before removing session
		OnClose(*it);

		if(it != sessions.end()) {
			// remove session and free it
			delete (*it);
			sessions.erase(it);
		}
	}	
	
	void WebsocketServer::OnHttpClose(HTTPSession* session) {
		if(!session)
			return;

		std::lock_guard<std::mutex> lock(SessionsLock);

		// find session in list
		auto FindSession = [&]() {
			for (auto it = http_sessions.begin(); it != http_sessions.end(); ++it)
				if (*it == session)
					return it;

			return http_sessions.end();
		};

		auto it = FindSession();


		if(it != http_sessions.end()) {
			// remove session and free it
			delete (*it);
			http_sessions.erase(it);
		}
	}
}
