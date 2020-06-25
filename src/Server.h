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

	// Interface that work follows.
	struct IWork {
		// Which work type.
		// If you're adding work, make sure to add a value to the above WorkType enum
		WorkType type;

		IWork(WorkType t)
			: type(t) {
		
		}
	};
	
	struct ConnectionAddWork : public IWork {
		WebsocketServer::handle_type handle;

		ConnectionAddWork(WebsocketServer::handle_type handle) 
			: IWork(WorkType::AddConnection), handle(handle) {
		}

	};	

	struct ConnectionRemoveWork : public IWork {
		WebsocketServer::handle_type handle;

		ConnectionRemoveWork(WebsocketServer::handle_type handle) 
			: IWork(WorkType::RemoveConnection), handle(handle) {
		}

	};

	struct WSMessageWork : public IWork {
		WebsocketServer::handle_type handle;
		WebsocketServer::message_type message;

		WSMessageWork(WebsocketServer::handle_type handle, WebsocketServer::message_type message)
			: IWork(WorkType::Message), handle(handle), message(message) {
		
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

		bool OnVerify(BaseServer::handle_type handle);

		void OnOpen(BaseServer::handle_type handle);

		void OnMessage(BaseServer::handle_type handle, BaseServer::message_type message);

		void OnClose(BaseServer::handle_type handle);

		// Shorthand to add work to the work queue
		inline void AddWork(std::shared_ptr<IWork> newWork) {
			// Only add action to the work queue if
			// the work to add isn't nullptr
			if(newWork) {
				//std::lock_guard<std::mutex> lock(WorkLock);

				work.push_back(newWork);
				WorkReady.notify_one();
			}
		}

	private:
		void ProcessActions();

		IPData* FindIPData(net::ip::address& address);

		void CreateIPData(net::ip::address& address);

		void CleanupIPData();

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
		std::deque<std::shared_ptr<IWork>> work;

		
		std::mutex IPDataLock;

		net::steady_timer IPDataCleanupTimer;

		// TODO: make thse shared_ptrs along with *everything* else

		// IPv4 IPData
		std::map<uint64, IPData*> ipv4data;
		
		// IPv6 IPData
		std::map<std::array<byte, 16>, IPData*> ipv6data;

		
		std::mutex UsersLock;

		// maps handles of streams to users
		std::map<WebsocketServer::handle_type, std::shared_ptr<User>> users;

		// logger
		Logger logger = Logger::GetLogger("CollabVMServer");
	};

}