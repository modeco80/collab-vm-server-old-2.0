#include "WebsocketServer.h"

using namespace std::placeholders;

namespace CollabVM {

	inline void ConfigureStream(ws::stream<beast::tcp_stream>& stream) {
		// Enable the WebSocket permessage deflate extension.
		ws::permessage_deflate pmd;
		pmd.client_enable = true;
		pmd.server_enable = true;
		pmd.compLevel = 3;
		stream.set_option(pmd);

		stream.auto_fragment(false);
	}

	// return a mime type for the specific file
	inline beast::string_view MimeType(beast::string_view path) {
		auto ext = [&path] {
			auto const pos = path.rfind(".");
			if(pos == beast::string_view::npos)
				return beast::string_view{};
			return path.substr(pos);
		}();
#define EXT(extension, mime) if(beast::iequals(ext, extension)) return mime;
		// Subset that should be "good" enough
		EXT(".htm", "text/html")
		EXT(".html", "text/html")
		EXT(".css", "text/css")
		EXT(".txt", "text/plain")
		EXT(".js", "application/javascript")
		EXT(".json", "application/json")
		EXT(".png", "image/png")
		EXT(".ico", "image/vnd.microsoft.icon")
#undef EXT
		return "application/text";
	}


	// WSSession

	void WSSession::Run(http::request<http::string_body> req) {
		// already running on the right strand
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

		stream.async_accept(req, beast::bind_front_handler(&WSSession::OnAccept, shared_from_this()));
	}

	ws::stream<beast::tcp_stream>& WSSession::GetStream() {
		return stream;
	}

	void WSSession::OnAccept(beast::error_code ec) {
		if(ec)
			return;

		logger.verbose("Accepted session for ", GetAddress().to_string());

		server->OnOpen(shared_from_this());

		Read();
	}

	void WSSession::Read() {
		// make message
		auto message = std::make_shared<WSMessage>();

		stream.async_read(message->buffer, beast::bind_front_handler(&WSSession::OnRead, shared_from_this(), message));
	}

	void WSSession::OnRead(WebsocketServer::message_type message, beast::error_code ec, std::size_t bytes_transferred) {
		if(ec == ws::error::closed) {
			message.reset();
			server->OnClose(shared_from_this());
			return;
		}

		if(ec)
			return;

		message->binary = stream.got_binary();

		server->OnMessage(shared_from_this(), message);

		Read();
	}

	void WSSession::Send(WebsocketServer::message_type message) {
		if (message->binary)
			stream.binary(true);
		else
			stream.text(true);

		stream.async_write(message->buffer.data(), beast::bind_front_handler(&WSSession::OnSend, shared_from_this()));
	}

	void WSSession::OnSend(beast::error_code ec, std::size_t bytes_transferred) {
		if(ec == ws::error::closed)
			server->OnClose(shared_from_this());
	}

	// http session
	struct HTTPSession : public std::enable_shared_from_this<HTTPSession> {
		
		explicit HTTPSession(WebsocketServer* server, tcp::socket&& socket)
			: server(server), stream(std::move(socket)) {
		
		}

		void Run() {
			// Ensure we're on the right strand
			net::dispatch(stream.get_executor(), beast::bind_front_handler(&HTTPSession::Read, shared_from_this()));
		}


	private:

		void Read() {
			req = {};

			http::async_read(stream, request_buffer, req, beast::bind_front_handler(&HTTPSession::OnRead, shared_from_this()));
		}

		void OnRead(beast::error_code ec, size_t bytes_written) {
			if (ec == http::error::end_of_stream) {
				beast::error_code ec;
				stream.socket().shutdown(tcp::socket::shutdown_send, ec);
			}

			if (ec)
				return;

			// TODO: here would be a good place to add code to check for the X-Forwarded-For
			// header. If this is found, we could have a optional shared_ptr<net::ip::address> argument or something
			// that overrides what IP address the server will treat the user as using

			if (ws::is_upgrade(req)) {

				auto subprotocols = http::token_list(req[http::field::sec_websocket_protocol]);
				auto session = std::make_shared<WSSession>(stream.release_socket(), server, subprotocols);

				// At this point, there is no actual Websocket connection handshaked,
				// but we can have the server verify if this connection should be accepted.
				//
				// If the verify callback returns false, then we should close the session
				// and wait for another accept.
				if (!server->OnVerify(session)) {
					beast::error_code ec;
					stream.socket().shutdown(tcp::socket::shutdown_send, ec);
					return;
				}

				session->Run(req);
			} else {
				// TODO: implement a basic static file host. (NOTE: make sure to allow chunk/resume stuff?
				// (above todo may be discarded..)
				auto target = req.target();
				auto address = stream.socket().remote_endpoint().address().to_string();
				http::response<http::string_body> res;

				res.set(http::field::server, "collab-vm-server/2.0");
				
				logger.info(address, " Requested (", req.method_string() , ") ", target);

				switch(req.method()) {
					case http::verb::get:
						res.result(http::status::ok);
						res.body() = "CollabVM 2.0";
						break;

					case http::verb::head:
						res.result(http::status::ok);
						break;

					default: {
						res.result(http::status::bad_request);
						res.body() = "Bad Request";
					} break;
				}

				Write(res);
			}

			// Session stops existing after this
		}
		
		void Write(http::response<http::string_body>& response) {
			beast::error_code ec;
			http::write(stream.socket(), response, ec);
			
			if(ec)
				return;

			if(req.need_eof()) {
				beast::error_code ec;
				stream.socket().shutdown(tcp::socket::shutdown_send, ec);
			}
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

	struct Listener : public std::enable_shared_from_this<Listener> {

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
			// new connection runs on its own strand
			acceptor.async_accept(net::make_strand(ioc), beast::bind_front_handler(&Listener::OnAccept, shared_from_this()));
		}


		void OnAccept(beast::error_code ec, tcp::socket socket_) {
			if(ec)
				return;
			
			auto session = std::make_shared<HTTPSession>(server, std::move(socket_));
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

		// The listener doesn't have to worry about freeing,
		// the WebsocketServer will handle that
		listener = std::make_shared<Listener>(*io_service, ep, this);
		listener->Run();
		io_service->run();
	}


	void WebsocketServer::Stop() {
		listener.reset();
	}

}
