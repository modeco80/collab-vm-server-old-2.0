#pragma once
#include "Common.h"
#include "User.h"
#include "WebsocketServer.h"
#include "Logger.h"
#include "VMControllers/Common/VMController.h"

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
				// TODO: This lock needs to somehow be re-added. Otherwise,
				// things might fall down and go boom

				//std::lock_guard<std::mutex> lock(WorkLock);

				work.push_back(newWork);
				WorkReady.notify_one();
			}
		}

	private:
		void ProcessActions();

		std::shared_ptr<IPData> FindIPData(net::ip::address& address);

		void CreateIPData(net::ip::address& address);

		void CleanupIPData();

		//void OnVMControllerStateChange();

		inline void StartIPDataTimer() {
			IPDataCleanupTimer.expires_after(net::steady_timer::duration(IPDataTimeout));
			IPDataCleanupTimer.async_wait(std::bind(&Server::CleanupIPData, this));
		}

		// TODO figure out how to make these constexpr

		// Timeout in seconds when the IPData will be cleaned up.
		const std::chrono::seconds IPDataTimeout = std::chrono::seconds(5);


		// Thread performing work.
		std::thread WorkThread;

		std::mutex WorkLock;

		std::condition_variable WorkReady;

		bool StopWorking = false;

		// deque of work
		// locked by workLock
		std::deque<std::shared_ptr<IWork>> work;

		
		std::mutex IPDataLock;

		net::steady_timer IPDataCleanupTimer;


		// IPv4 IPData
		std::map<uint64, std::shared_ptr<IPData>> ipv4data;
		
		// IPv6 IPData
		std::map<std::array<byte, 16>, std::shared_ptr<IPData>> ipv6data;

		
		std::mutex UsersLock;

		// Map of handles to users
		std::map<WebsocketServer::handle_type, std::shared_ptr<User>> users;

		std::mutex VMLock;
		std::map<int, std::shared_ptr<VMController>> vms;

		// logger
		Logger logger = Logger::GetLogger("CollabVMServer");
	};

}