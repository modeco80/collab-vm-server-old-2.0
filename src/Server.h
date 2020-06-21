#pragma once
#include "Common.h"
#include "User.h"
#include "WebsocketServer.h"
#include "Logger.h"

namespace CollabVM {
	
	enum class WorkType {
		AddConnection,
		RemoveConnection,
		Message
	};

	struct IWork {
		WorkType type;

		IWork(WorkType t)
			: type(t) {
		
		}
	};

	struct ConnectionAddWork : public IWork {
		WebsocketServer::connection_type conPtr;

		ConnectionAddWork(WebsocketServer::connection_type con) 
			: IWork(WorkType::AddConnection), conPtr(con) {
		
		}
	};	
	
	struct ConnectionRemoveWork : public IWork {
		WebsocketServer::connection_type conPtr;

		ConnectionRemoveWork(WebsocketServer::connection_type con) 
			: IWork(WorkType::RemoveConnection), conPtr(con) {
		
		}
	};

	struct WebsocketMessageWork : public IWork {
		WebsocketServer::stream_type s;
		WebsocketServer::message_type message;

		WebsocketMessageWork(WebsocketServer::connection_type con, WebsocketServer::message_type msg)
			: IWork(WorkType::Message), conPtr(con), message(msg) {
		
		}
	};

	struct Server : public WebsocketServer {
		typedef WebsocketServer BaseServer;

		inline Server(net::io_service& ioc)
			: BaseServer(ioc),
			IPDataCleanupTimer(ioc) {
			
		}

		~Server();

		void Start(tcp::endpoint& ep);

		void Stop();

		void OnWebsocketOpen(BaseServer::handle_type userHdl);

		void OnWebsocketMessage(BaseServer::handle_type userHdl, BaseServer::message_type message);

		void OnWebsocketClose(BaseServer::handle_type userHdl);

	private:
		void ProcessActions();

		IPData* FindIPData(net::ip::address& address);

		void CreateIPData(net::ip::address& address);

		void CleanupIPData();

		// Shorthand to add work to the processing queue
		inline void AddWork(IWork* newWork) {
			// Only add action to the work queue
			if(newWork) {
				std::lock_guard<std::mutex> lock(WorkLock);

				work.push_back((IWork*)newWork);
				WorkReady.notify_one();
			}
		}

		inline void StartIPDataTimer() {
			IPDataCleanupTimer.expires_after(net::steady_timer::duration(IPDataTimeout));
			IPDataCleanupTimer.async_wait(std::bind(&Server::CleanupIPData, this));
		}

		// TODO figure out how to make these constexpr

		const std::chrono::seconds IPDataTimeout = std::chrono::seconds(5);

		// Timeout in seconds of when a user will be disconnected.
		// TODO: Instead of using a special `nop` instruction,
		// use the built-in Websocket ping frames.
		const std::chrono::seconds PingTimeout = std::chrono::seconds(15);

		// processing thread
		std::thread WorkThread;

		// mutex locking work
		// if locked by thread, thread
		// can modify to it's own will
		std::mutex WorkLock;

		std::condition_variable WorkReady;

		bool StopWorking = false;

		// deque of work
		// locked by workLock
		std::deque<IWork*> work;

		
		std::mutex IPDataLock;

		net::steady_timer IPDataCleanupTimer;

		// IPv4 IPData
		std::map<uint64, IPData*> ipv4data;
		
		// IPv6 IPData
		std::map<std::array<byte, 16>, IPData*> ipv6data;

		
		std::mutex UsersLock;

		// maps handles of streams to users
		std::map<WebsocketServer::handle_type, User*> users;

		Logger logger = Logger::GetLogger("CollabVMServer");
	};

}