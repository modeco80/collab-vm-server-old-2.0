#pragma once
#include "Common.h"
#include "User.h"
#include "WebsocketServer.h"
#include "Logger.h"

namespace CollabVM {
	
	enum class ActionType {
		AddConnection,
		RemoveConnection,
		Message
	};

	struct IAction {
		ActionType type;

		IAction(ActionType t)
			: type(t) {
		
		}
	};

	struct AddConnectionAction : public IAction {
		WebsocketServer::connection_type conPtr;

		AddConnectionAction(WebsocketServer::connection_type con) 
			: IAction(ActionType::AddConnection), conPtr(con) {
		
		}
	};	
	
	struct RemoveConnectionAction : public IAction {
		WebsocketServer::connection_type conPtr;

		RemoveConnectionAction(WebsocketServer::connection_type con) 
			: IAction(ActionType::RemoveConnection), conPtr(con) {
		
		}
	};

	struct MessageAction : public IAction {
		WebsocketServer::connection_type conPtr;
		WebsocketServer::message_type message;

		MessageAction(WebsocketServer::connection_type con, WebsocketServer::message_type msg)
			: IAction(ActionType::Message), conPtr(con), message(msg) {
		
		}
	};

	struct Server : public WebsocketServer {
		typedef WebsocketServer base;

		~Server();

		void Start(net::io_service& ioc, uint16 port);

		void Stop();

		bool OnWebsocketValidate(base::handle_type userHdl);

		void OnWebsocketOpen(base::handle_type userHdl);

		void OnWebsocketMessage(base::handle_type userHdl, base::message_type message);

		void OnWebsocketClose(base::handle_type userHdl);

	private:
		void ProcessActions();

		IPData* FindIPData(net::ip::address& address);

		void CreateIPData(net::ip::address& address);

		void CleanupIPData();

		// Shorthand
		inline void AddWork(IAction* action) {
			if(action) {
				work.push_back(action);
				workReady.notify_one();
			}
		}

		// processing thread
		std::thread processingThread;

		// mutex locking work
		// if locked by thread, thread
		// can modify to it's own will
		std::mutex workLock;

		std::condition_variable workReady;

		bool stopProcessing = false;

		// deque of work
		// locked by workLock
		std::deque<IAction*> work;

		
		std::mutex ipDataLock;

		std::map<uint64, IPData*> ipv4data;
		std::map<std::array<byte, 16>, IPData*> ipv6data;

		
		std::mutex usersLock;

		std::map<WebsocketServer::connection_type, User*> users;

		Logger logger = Logger::GetLogger("CollabVMServer");
	};

}